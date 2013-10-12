/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file afterload.cpp Code updating data after game load */

#include "../stdafx.h"
#include "../signs_base.h"
#include "../depot_base.h"
#include "../fios.h"
#include "../gamelog.h"
#include "../network/network.h"
#include "../gfxinit.h"
#include "../viewport_func.h"
#include "../industry.h"
#include "../map/ground.h"
#include "../map/slope.h"
#include "../vehicle_func.h"
#include "../string_func.h"
#include "../date_func.h"
#include "../roadveh.h"
#include "../train.h"
#include "../ship.h"
#include "../station_base.h"
#include "../waypoint_base.h"
#include "../roadstop_base.h"
#include "../map/road.h"
#include "../map/bridge.h"
#include "../map/tunnelbridge.h"
#include "../pathfinder/yapf/yapf_cache.h"
#include "../elrail_func.h"
#include "../signs_func.h"
#include "../aircraft.h"
#include "../map/object.h"
#include "../object_base.h"
#include "../company_func.h"
#include "../road_cmd.h"
#include "../ai/ai.hpp"
#include "../ai/ai_gui.hpp"
#include "../town.h"
#include "../economy_base.h"
#include "../animated_tile_func.h"
#include "../subsidy_base.h"
#include "../subsidy_func.h"
#include "../newgrf.h"
#include "../engine_func.h"
#include "../rail_gui.h"
#include "../core/backup_type.hpp"
#include "../smallmap_gui.h"
#include "../news_func.h"
#include "../error.h"


#include "saveload_internal.h"
#include "saveload_error.h"

extern Company *DoStartupNewCompany(bool is_ai, CompanyID company = INVALID_COMPANY);

/**
 * Makes a tile canal or water depending on the surroundings.
 *
 * Must only be used for converting old savegames. Use WaterClass now.
 *
 * This as for example docks and shipdepots do not store
 * whether the tile used to be canal or 'normal' water.
 * @param t the tile to change.
 * @param allow_invalid Also consider WATER_CLASS_INVALID, i.e. industry tiles on land
 */
static void GuessWaterClass(TileIndex t, bool allow_invalid)
{
	/* If the slope is not flat, we always assume 'land' (if allowed). Also for one-corner-raised-shores.
	 * Note: Wrt. autosloping under industry tiles this is the most fool-proof behaviour. */
	if (!IsTileFlat(t)) {
		if (allow_invalid) {
			SetWaterClass(t, WATER_CLASS_INVALID);
			return;
		} else {
			throw SlCorrupt("Invalid water class for dry tile");
		}
	}

	/* Mark tile dirty in all cases */
	MarkTileDirtyByTile(t);

	if (TileX(t) == 0 || TileY(t) == 0 || TileX(t) == MapMaxX() - 1 || TileY(t) == MapMaxY() - 1) {
		/* tiles at map borders are always WATER_CLASS_SEA */
		SetWaterClass(t, WATER_CLASS_SEA);
		return;
	}

	bool has_water = false;
	bool has_canal = false;
	bool has_river = false;

	for (DiagDirection dir = DIAGDIR_BEGIN; dir < DIAGDIR_END; dir++) {
		TileIndex neighbour = TileAddByDiagDir(t, dir);
		switch (GetTileType(neighbour)) {
			case TT_WATER:
				/* clear water and shipdepots have already a WaterClass associated */
				if (IsCoast(neighbour)) {
					has_water = true;
				} else if (!IsLock(neighbour)) {
					switch (GetWaterClass(neighbour)) {
						case WATER_CLASS_SEA:   has_water = true; break;
						case WATER_CLASS_CANAL: has_canal = true; break;
						case WATER_CLASS_RIVER: has_river = true; break;
						default: throw SlCorrupt("Invalid water class for tile");
					}
				}
				break;

			case TT_RAILWAY:
				/* Shore or flooded halftile */
				has_water |= IsTileSubtype(neighbour, TT_TRACK) && (GetRailGroundType(neighbour) == RAIL_GROUND_WATER);
				break;

			case TT_GROUND:
				/* trees on shore */
				has_water |= IsTreeTile(neighbour) && (GetClearGround(neighbour) == GROUND_SHORE);
				break;

			default: break;
		}
	}

	if (!has_water && !has_canal && !has_river && allow_invalid) {
		SetWaterClass(t, WATER_CLASS_INVALID);
		return;
	}

	if (has_river && !has_canal) {
		SetWaterClass(t, WATER_CLASS_RIVER);
	} else if (has_canal || !has_water) {
		SetWaterClass(t, WATER_CLASS_CANAL);
	} else {
		SetWaterClass(t, WATER_CLASS_SEA);
	}
}

/**
 * Update the viewport coordinates of all signs.
 */
void UpdateAllVirtCoords()
{
	UpdateAllStationVirtCoords();
	UpdateAllSignVirtCoords();
	UpdateAllTownVirtCoords();
}

/**
 * Initialization of the windows and several kinds of caches.
 * This is not done directly in AfterLoadGame because these
 * functions require that all saveload conversions have been
 * done. As people tend to add savegame conversion stuff after
 * the intialization of the windows and caches quite some bugs
 * had been made.
 * Moving this out of there is both cleaner and less bug-prone.
 */
static void InitializeWindowsAndCaches()
{
	/* Initialize windows */
	ResetWindowSystem();
	SetupColoursAndInitialWindow();

	/* Update coordinates of the signs. */
	UpdateAllVirtCoords();
	ResetViewportAfterLoadGame();

	Company *c;
	FOR_ALL_COMPANIES(c) {
		/* For each company, verify (while loading a scenario) that the inauguration date is the current year and set it
		 * accordingly if it is not the case.  No need to set it on companies that are not been used already,
		 * thus the MIN_YEAR (which is really nothing more than Zero, initialized value) test */
		if (_file_to_saveload.filetype == FT_SCENARIO && c->inaugurated_year != MIN_YEAR) {
			c->inaugurated_year = _cur_year;
		}
	}

	RecomputePrices();

	GroupStatistics::UpdateAfterLoad();

	Station::RecomputeIndustriesNearForAll();
	RebuildSubsidisedSourceAndDestinationCache();

	/* Towns have a noise controlled number of airports system
	 * So each airport's noise value must be added to the town->noise_reached value
	 * Reset each town's noise_reached value to '0' before. */
	UpdateAirportsNoise();

	CheckTrainsLengths();
	ShowNewGRFError();
	ShowAIDebugWindowIfAIError();

	/* Rebuild the smallmap list of owners. */
	BuildOwnerLegend();
}

/**
 * Tries to change owner of this rail tile to a valid owner. In very old versions it could happen that
 * a rail track had an invalid owner. When conversion isn't possible, track is removed.
 * @param t tile to update
 */
static void FixOwnerOfRailTrack(TileIndex t)
{
	assert(!Company::IsValidID(GetTileOwner(t)));
	assert(IsLevelCrossingTile(t) || IsNormalRailTile(t));

	/* remove leftover rail piece from crossing (from very old savegames) */
	Train *v = NULL, *w;
	FOR_ALL_TRAINS(w) {
		if (w->tile == t) {
			v = w;
			break;
		}
	}

	if (v != NULL) {
		/* when there is a train on crossing (it could happen in TTD), set owner of crossing to train owner */
		SetTileOwner(t, v->owner);
		return;
	}

	/* try to find any connected rail */
	for (DiagDirection dd = DIAGDIR_BEGIN; dd < DIAGDIR_END; dd++) {
		TileIndex tt = t + TileOffsByDiagDir(dd);
		if (GetTileRailwayStatus(t, dd) != 0 &&
				GetTileRailwayStatus(tt, ReverseDiagDir(dd)) != 0 &&
				Company::IsValidID(GetTileOwner(tt))) {
			SetTileOwner(t, GetTileOwner(tt));
			return;
		}
	}

	if (IsLevelCrossingTile(t)) {
		/* else change the crossing to normal road (road vehicles won't care) */
		MakeRoadNormal(t, GetCrossingRoadBits(t), GetRoadTypes(t), GetTownIndex(t),
			GetRoadOwner(t, ROADTYPE_ROAD), GetRoadOwner(t, ROADTYPE_TRAM));
		return;
	}

	/* if it's not a crossing, make it clean land */
	MakeClear(t, GROUND_GRASS, 0);
}

/**
 * Fixes inclination of a vehicle. Older OpenTTD versions didn't update the bits correctly.
 * @param v vehicle
 * @param dir vehicle's direction, or # INVALID_DIR if it can be ignored
 * @return inclination bits to set
 */
static uint FixVehicleInclination(Vehicle *v, Direction dir)
{
	/* Compute place where this vehicle entered the tile */
	int entry_x = v->x_pos;
	int entry_y = v->y_pos;
	switch (dir) {
		case DIR_NE: entry_x |= TILE_UNIT_MASK; break;
		case DIR_NW: entry_y |= TILE_UNIT_MASK; break;
		case DIR_SW: entry_x &= ~TILE_UNIT_MASK; break;
		case DIR_SE: entry_y &= ~TILE_UNIT_MASK; break;
		case INVALID_DIR: break;
		default: NOT_REACHED();
	}
	byte entry_z = GetSlopePixelZ(entry_x, entry_y);

	/* Compute middle of the tile. */
	int middle_x = (v->x_pos & ~TILE_UNIT_MASK) + TILE_SIZE / 2;
	int middle_y = (v->y_pos & ~TILE_UNIT_MASK) + TILE_SIZE / 2;
	byte middle_z = GetSlopePixelZ(middle_x, middle_y);

	/* middle_z == entry_z, no height change. */
	if (middle_z == entry_z) return 0;

	/* middle_z < entry_z, we are going downwards. */
	if (middle_z < entry_z) return 1U << GVF_GOINGDOWN_BIT;

	/* middle_z > entry_z, we are going upwards. */
	return 1U << GVF_GOINGUP_BIT;
}

/**
 * Perform a (large) amount of savegame conversion *magic* in order to
 * load older savegames and to fill the caches for various purposes.
 * @return True iff conversion went without a problem.
 */
void AfterLoadGame(const SavegameTypeVersion *stv)
{
	TileIndex map_size = MapSize();

	if (IsOTTDSavegameVersionBefore(stv, 98)) GamelogOldver(stv);

	GamelogTestRevision();
	GamelogTestMode();

	if (IsOTTDSavegameVersionBefore(stv, 98)) GamelogGRFAddList(_grfconfig);

	if (IsOTTDSavegameVersionBefore(stv, 119)) {
		_pause_mode = (_pause_mode == 2) ? PM_PAUSED_NORMAL : PM_UNPAUSED;
	} else if (_network_dedicated && (_pause_mode & PM_PAUSED_ERROR) != 0) {
		DEBUG(net, 0, "The loading savegame was paused due to an error state.");
		DEBUG(net, 0, "  The savegame cannot be used for multiplayer!");
		throw SlCorrupt("Savegame paused due to an error state");
	} else if (!_networking || _network_server) {
		/* If we are in single player, i.e. not networking, and loading the
		 * savegame or we are loading the savegame as network server we do
		 * not want to be bothered by being paused because of the automatic
		 * reason of a network server, e.g. joining clients or too few
		 * active clients. Note that resetting these values for a network
		 * client are very bad because then the client is going to execute
		 * the game loop when the server is not, i.e. it desyncs. */
		_pause_mode &= ~PMB_PAUSED_NETWORK;
	}

	extern TileIndex _cur_tileloop_tile; // From landscape.cpp.
	/* The LFSR used in RunTileLoop iteration cannot have a zeroed state, make it non-zeroed. */
	if (_cur_tileloop_tile == 0) _cur_tileloop_tile = 1;

	/* Adjust map array for changes since the savegame was made. */
	AfterLoadMap(stv);

	/* In very old versions, size of train stations was stored differently.
	 * They had swapped width and height if station was built along the Y axis.
	 * TTO and TTD used 3 bits for width/height, while OpenTTD used 4.
	 * Because the data stored by TTDPatch are unusable for rail stations > 7x7,
	 * recompute the width and height. Doing this unconditionally for all old
	 * savegames simplifies the code. */
	if (IsOTTDSavegameVersionBefore(stv, 2)) {
		Station *st;
		FOR_ALL_STATIONS(st) {
			st->train_station.w = st->train_station.h = 0;
		}
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsStationTile(t)) continue;
			if (GetStationType(t) != STATION_RAIL) continue;
			st = Station::Get(_mc[t].m2);
			assert(st->train_station.tile != 0);
			int dx = TileX(t) - TileX(st->train_station.tile);
			int dy = TileY(t) - TileY(st->train_station.tile);
			assert(dx >= 0 && dy >= 0);
			st->train_station.w = max<uint>(st->train_station.w, dx + 1);
			st->train_station.h = max<uint>(st->train_station.h, dy + 1);
		}
	}

	/* from legacy version 4.1 of the savegame, exclusive rights are stored at towns */
	if (IsOTTDSavegameVersionBefore(stv, 4, 1)) {
		Town *t;

		FOR_ALL_TOWNS(t) {
			t->exclusivity = INVALID_COMPANY;
		}

		/* FIXME old exclusive rights status is not being imported (stored in s->blocked_months_obsolete)
		 *   could be implemented this way:
		 * 1.) Go through all stations
		 *     Build an array town_blocked[ town_id ][ company_id ]
		 *     that stores if at least one station in that town is blocked for a company
		 * 2.) Go through that array, if you find a town that is not blocked for
		 *     one company, but for all others, then give him exclusivity.
		 */
	}

	/* from legacy version 4.2 of the savegame, currencies are in a different order */
	if (IsOTTDSavegameVersionBefore(stv, 4, 2)) {
		static const byte convert_currency[] = {
			 0,  1, 12,  8,  3,
			10, 14, 19,  4,  5,
			 9, 11, 13,  6, 17,
			16, 22, 21,  7, 15,
			18,  2, 20,
		};

		_settings_game.locale.currency = convert_currency[_settings_game.locale.currency];
	}

	/* In old version there seems to be a problem that water is owned by
	 * OWNER_NONE, not OWNER_WATER.. I can't replicate it for the current
	 * (4.3) version, so I just check when versions are older, and then
	 * walk through the whole map.. */
	if (IsOTTDSavegameVersionBefore(stv, 4, 3)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsWaterTile(t) && GetTileOwner(t) >= MAX_COMPANIES) {
				SetTileOwner(t, OWNER_WATER);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 84)) {
		Company *c;
		FOR_ALL_COMPANIES(c) {
			c->name = CopyFromOldName(stv, c->name_1);
			if (c->name != NULL) c->name_1 = STR_SV_UNNAMED;
			c->president_name = CopyFromOldName(stv, c->president_name_1);
			if (c->president_name != NULL) c->president_name_1 = SPECSTR_PRESIDENT_NAME;
		}

		Station *st;
		FOR_ALL_STATIONS(st) {
			st->name = CopyFromOldName(stv, st->string_id);
			/* generating new name would be too much work for little effect, use the station name fallback */
			if (st->name != NULL) st->string_id = STR_SV_STNAME_FALLBACK;
		}

		Town *t;
		FOR_ALL_TOWNS(t) {
			t->name = CopyFromOldName(stv, t->townnametype);
			if (t->name != NULL) t->townnametype = SPECSTR_TOWNNAME_START + _settings_game.game_creation.town_name;
		}
	}

	/* From this point the old names array is cleared. */
	ResetOldNames();

	if (IsOTTDSavegameVersionBefore(stv, 106)) {
		/* no station is determined by 'tile == INVALID_TILE' now (instead of '0') */
		Station *st;
		FOR_ALL_STATIONS(st) {
			if (st->airport.tile       == 0) st->airport.tile = INVALID_TILE;
			if (st->dock_tile          == 0) st->dock_tile    = INVALID_TILE;
			if (st->train_station.tile == 0) st->train_station.tile   = INVALID_TILE;
		}

		/* the same applies to Company::location_of_HQ */
		Company *c;
		FOR_ALL_COMPANIES(c) {
			if (c->location_of_HQ == 0 || (IsOTTDSavegameVersionBefore(stv, 4) && c->location_of_HQ == 0xFFFF)) {
				c->location_of_HQ = INVALID_TILE;
			}
		}
	}

	/* convert road side to my format. */
	if (_settings_game.vehicle.road_side) _settings_game.vehicle.road_side = 1;

	/* Check if all NewGRFs are present, we are very strict in MP mode */
	GRFListCompatibility gcf_res = IsGoodGRFConfigList(_grfconfig);
	for (GRFConfig *c = _grfconfig; c != NULL; c = c->next) {
		if (c->status == GCS_NOT_FOUND) {
			GamelogGRFRemove(c->ident.grfid);
		} else if (HasBit(c->flags, GCF_COMPATIBLE)) {
			GamelogGRFCompatible(&c->ident);
		}
	}

	if (_networking && gcf_res != GLC_ALL_GOOD) {
		throw SlException(STR_NETWORK_ERROR_CLIENT_NEWGRF_MISMATCH);
	}

	switch (gcf_res) {
		case GLC_COMPATIBLE: ShowErrorMessage(STR_NEWGRF_COMPATIBLE_LOAD_WARNING, INVALID_STRING_ID, WL_CRITICAL); break;
		case GLC_NOT_FOUND:  ShowErrorMessage(STR_NEWGRF_DISABLED_WARNING, INVALID_STRING_ID, WL_CRITICAL); _pause_mode = PM_PAUSED_ERROR; break;
		default: break;
	}

	/* The value of _date_fract got divided, so make sure that old games are converted correctly. */
	if (IsOTTDSavegameVersionBefore(stv, 11, 1) || (IsOTTDSavegameVersionBefore(stv, 147) && _date_fract > DAY_TICKS)) _date_fract /= 885;

	/* Update current year
	 * must be done before loading sprites as some newgrfs check it */
	SetDate(_date, _date_fract);

	/*
	 * Force the old behaviour for compatibility reasons with old savegames. As new
	 * settings can only be loaded from new savegames loading old savegames with new
	 * versions of OpenTTD will normally initialize settings newer than the savegame
	 * version with "new game" defaults which the player can define to their liking.
	 * For some settings we override that to keep the behaviour the same as when the
	 * game was saved.
	 *
	 * Note that there is no non-stop in here. This is because the setting could have
	 * either value in TTDPatch. To convert it properly the user has to make sure the
	 * right value has been chosen in the settings. Otherwise we will be converting
	 * it incorrectly in half of the times without a means to correct that.
	 */
	if (IsOTTDSavegameVersionBefore(stv, 4, 2)) _settings_game.station.modified_catchment = false;
	if (IsOTTDSavegameVersionBefore(stv, 6, 1)) _settings_game.pf.forbid_90_deg = false;
	if (IsOTTDSavegameVersionBefore(stv, 21))   _settings_game.vehicle.train_acceleration_model = 0;
	if (IsOTTDSavegameVersionBefore(stv, 90))   _settings_game.vehicle.plane_speed = 4;
	if (IsOTTDSavegameVersionBefore(stv, 95))   _settings_game.vehicle.dynamic_engines = 0;
	if (IsOTTDSavegameVersionBefore(stv, 96))   _settings_game.economy.station_noise_level = false;
	if (IsOTTDSavegameVersionBefore(stv, 133)) {
		_settings_game.vehicle.roadveh_acceleration_model = 0;
		_settings_game.vehicle.train_slope_steepness = 3;
	}
	if (IsOTTDSavegameVersionBefore(stv, 134))  _settings_game.economy.feeder_payment_share = 75;
	if (IsOTTDSavegameVersionBefore(stv, 138))  _settings_game.vehicle.plane_crashes = 2;
	if (IsOTTDSavegameVersionBefore(stv, 139))  _settings_game.vehicle.roadveh_slope_steepness = 7;
	if (IsOTTDSavegameVersionBefore(stv, 143))  _settings_game.economy.allow_town_level_crossings = true;
	if (IsOTTDSavegameVersionBefore(stv, 159)) {
		_settings_game.vehicle.max_train_length = 50;
		_settings_game.construction.max_bridge_length = 64;
		_settings_game.construction.max_tunnel_length = 64;
	}
	if (IsOTTDSavegameVersionBefore(stv, 166))  _settings_game.economy.infrastructure_maintenance = false;
	if (IsOTTDSavegameVersionBefore(stv, 183)) {
		_settings_game.linkgraph.distribution_pax = DT_MANUAL;
		_settings_game.linkgraph.distribution_mail = DT_MANUAL;
		_settings_game.linkgraph.distribution_armoured = DT_MANUAL;
		_settings_game.linkgraph.distribution_default = DT_MANUAL;
	}

	/* Load the sprites */
	GfxLoadSprites();
	LoadStringWidthTable();

	/* Copy temporary data to Engine pool */
	CopyTempEngineData();

	/* Connect front and rear engines of multiheaded trains and converts
	 * subtype to the new format */
	if (IsOTTDSavegameVersionBefore(stv, 17, 1)) ConvertOldMultiheadToNew();

	/* Connect front and rear engines of multiheaded trains */
	ConnectMultiheadedTrains();

	/* Fix the CargoPackets *and* fix the caches of CargoLists.
	 * If this isn't done before Stations and especially Vehicles are
	 * running their AfterLoad we might get in trouble. In the case of
	 * vehicles we could give the wrong (cached) count of items in a
	 * vehicle which causes different results when getting their caches
	 * filled; and that could eventually lead to desyncs. */
	CargoPacket::AfterLoad(stv);

	if (IsOTTDSavegameVersionBefore(stv, 42)) {
		Vehicle *v;
		FOR_ALL_VEHICLES(v) {
			if (v->IsGroundVehicle() && v->z_pos > GetSlopePixelZ(v->x_pos, v->y_pos)) {
				v->tile = GetNorthernBridgeEnd(v->tile);
				if (v->type == VEH_TRAIN) {
					Train::From(v)->trackdir = TRACKDIR_WORMHOLE;
				} else {
					RoadVehicle::From(v)->state = RVSB_WORMHOLE;
				}
			}
		}
	}

	/* Oilrig was moved from id 15 to 9. We have to do this conversion
	 * here as AfterLoadVehicles can check it indirectly via the newgrf
	 * code. */
	if (IsOTTDSavegameVersionBefore(stv, 139)) {
		Station *st;
		FOR_ALL_STATIONS(st) {
			if (st->airport.tile != INVALID_TILE && st->airport.type == 15) {
				st->airport.type = AT_OILRIG;
			}
		}
	}

	/* Update all vehicles */
	AfterLoadVehicles(stv);

	/* Make sure there is an AI attached to an AI company */
	{
		Company *c;
		FOR_ALL_COMPANIES(c) {
			if (c->is_ai && c->ai_instance == NULL) AI::StartNew(c->index);
		}
	}

	/* make sure there is a town in the game */
	if (_game_mode == GM_NORMAL && Town::GetNumItems() == 0) {
		throw SlException(STR_ERROR_NO_TOWN_IN_SCENARIO);
	}

	/* If Load Scenario / New (Scenario) Game is used,
	 *  a company does not exist yet. So create one here.
	 * 1 exception: network-games. Those can have 0 companies
	 *   But this exception is not true for non-dedicated network servers! */
	if (!Company::IsValidID(COMPANY_FIRST) && (!_networking || (_networking && _network_server && !_network_dedicated))) {
		DoStartupNewCompany(false);
		Company *c = Company::Get(COMPANY_FIRST);
		c->settings = _settings_client.company;
	}

	/* Fix the cache for cargo payments. */
	CargoPayment *cp;
	FOR_ALL_CARGO_PAYMENTS(cp) {
		cp->front->cargo_payment = cp;
		cp->current_station = cp->front->last_station_visited;
	}

	if (IsOTTDSavegameVersionBefore(stv, 123)) {
		/* Waypoints became subclasses of stations ... */
		MoveWaypointsToBaseStations(stv);
		/* ... and buoys were moved to waypoints. */
		MoveBuoysToWaypoints();
	}

	for (TileIndex t = 0; t < map_size; t++) {
		if (IsStationTile(t)) {
			BaseStation *bst = BaseStation::GetByTile(t);

			/* Set up station spread */
			bst->rect.BeforeAddTile(t, StationRect::ADD_FORCE);

			/* Waypoints don't have road stops/oil rigs in the old format */
			if (!Station::IsExpected(bst)) continue;
			Station *st = Station::From(bst);

			switch (GetStationType(t)) {
				case STATION_TRUCK:
				case STATION_BUS:
					if (IsOTTDSavegameVersionBefore(stv, 6)) {
						/* Before legacy version 5 you could not have more than 250 stations.
						 * Version 6 adds large maps, so you could only place 253*253
						 * road stops on a map (no freeform edges) = 64009. So, yes
						 * someone could in theory create such a full map to trigger
						 * this assertion, it's safe to assume that's only something
						 * theoretical and does not happen in normal games. */
						assert(RoadStop::CanAllocateItem());

						/* From this version on there can be multiple road stops of the
						 * same type per station. Convert the existing stops to the new
						 * internal data structure. */
						RoadStop *rs = new RoadStop(t);

						RoadStop **head =
							IsTruckStop(t) ? &st->truck_stops : &st->bus_stops;
						*head = rs;
					}
					break;

				case STATION_OILRIG: {
					/* Very old savegames sometimes have phantom oil rigs, i.e.
					 * an oil rig which got shut down, but not completely removed from
					 * the map
					 */
					TileIndex t1 = TILE_ADDXY(t, 0, 1);
					if (IsIndustryTile(t1) &&
							GetIndustryGfx(t1) == GFX_OILRIG_1) {
						/* The internal encoding of oil rigs was changed twice.
						 * It was 3 (till 2.2) and later 5 (till 5.1).
						 * Setting it unconditionally does not hurt.
						 */
						Station::GetByTile(t)->airport.type = AT_OILRIG;
					} else {
						DeleteOilRig(t);
					}
					break;
				}

				default: break;
			}
		}
	}

	/* In legacy version 2.2 of the savegame, we have new airports, so status of all aircraft is reset.
	 * This has to be called after the oilrig airport_type update above ^^^ ! */
	if (IsOTTDSavegameVersionBefore(stv, 2, 2)) UpdateOldAircraft();

	/* In legacy version 6.1 we put the town index in the map-array. To do this, we need
	 *  to use m2 (16bit big), so we need to clean m2, and that is where this is
	 *  all about ;) */
	if (IsOTTDSavegameVersionBefore(stv, 6, 1)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsHouseTile(t) || ((IsRoadTile(t) || IsLevelCrossingTile(t)) && GetRoadOwner(t, ROADTYPE_ROAD) == OWNER_TOWN)) {
				SetTownIndex(t, CalcClosestTownFromTile(t)->index);
			}
		}
	}

	/* Force the freeform edges to false for old savegames. */
	if (IsOTTDSavegameVersionBefore(stv, 111)) {
		_settings_game.construction.freeform_edges = false;
	}

	/* From legacy version 9.0, we update the max passengers of a town (was sometimes negative
	 *  before that. */
	if (IsOTTDSavegameVersionBefore(stv, 9)) {
		Town *t;
		FOR_ALL_TOWNS(t) UpdateTownMaxPass(t);
	}

	/* From legacy version 16.0, we included autorenew on engines, which are now saved, but
	 *  of course, we do need to initialize them for older savegames. */
	if (IsOTTDSavegameVersionBefore(stv, 16)) {
		Company *c;
		FOR_ALL_COMPANIES(c) {
			c->engine_renew_list            = NULL;
			c->settings.engine_renew        = false;
			c->settings.engine_renew_months = 6;
			c->settings.engine_renew_money  = 100000;
		}

		/* When loading a game, _local_company is not yet set to the correct value.
		 * However, in a dedicated server we are a spectator, so nothing needs to
		 * happen. In case we are not a dedicated server, the local company always
		 * becomes company 0, unless we are in the scenario editor where all the
		 * companies are 'invalid'.
		 */
		c = Company::GetIfValid(COMPANY_FIRST);
		if (!_network_dedicated && c != NULL) {
			c->settings = _settings_client.company;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 114)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if ((IsRoadTile(t) || IsLevelCrossingTile(t)) && !HasTownOwnedRoad(t)) {
				const Town *town = CalcClosestTownFromTile(t);
				if (town != NULL) SetTownIndex(t, town->index);
			}
		}
	}

	if (IsFullSavegameVersionBefore(stv, 6)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsRoadBridgeTile(t)) {
				const Town *town = CalcClosestTownFromTile(t);
				if (town != NULL) SetTownIndex(t, town->index);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 42)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsRoadTile(t) && GetTownIndex(t) == INVALID_TOWN) {
				SetTownIndex(t, IsTileOwner(t, OWNER_TOWN) ? ClosestTownFromTile(t, UINT_MAX)->index : 0);
			}
		}
	}

	/* Elrails got added in legacy version 24 */
	if (IsOTTDSavegameVersionBefore(stv, 24)) {
		RailType min_rail = RAILTYPE_ELECTRIC;

		Train *v;
		FOR_ALL_TRAINS(v) {
			RailType rt = RailVehInfo(v->engine_type)->railtype;

			v->railtype = rt;
			if (rt == RAILTYPE_ELECTRIC) min_rail = RAILTYPE_RAIL;
		}

		/* .. so we convert the entire map from normal to elrail (so maintain "fairness") */
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				case TT_RAILWAY:
					break;

				case TT_MISC:
					switch (GetTileSubtype(t)) {
						default: NOT_REACHED();
						case TT_MISC_CROSSING: break;
						case TT_MISC_AQUEDUCT: continue;
						case TT_MISC_TUNNEL:
							if (GetTunnelTransportType(t) != TRANSPORT_RAIL) continue;
							break;
						case TT_MISC_DEPOT:
							if (!IsRailDepot(t)) continue;
							break;
					}
					break;

				case TT_STATION:
					if (!HasStationRail(t)) continue;
					break;

				default:
					continue;
			}

			RailType rt = GetRailType(t);
			if (rt >= min_rail) {
				SetRailType(t, (RailType)(rt + 1));
			}
		}

		FOR_ALL_TRAINS(v) {
			if (v->IsFrontEngine() || v->IsFreeWagon()) v->ConsistChanged(true);
		}

	}

	/* In legacy version 16.1 of the savegame a company can decide if trains, which get
	 * replaced, shall keep their old length. In all prior versions, just default
	 * to false */
	if (IsOTTDSavegameVersionBefore(stv, 16, 1)) {
		Company *c;
		FOR_ALL_COMPANIES(c) c->settings.renew_keep_length = false;
	}

	if (IsOTTDSavegameVersionBefore(stv, 25)) {
		RoadVehicle *rv;
		FOR_ALL_ROADVEHICLES(rv) {
			rv->vehstatus &= ~0x40;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 26)) {
		Station *st;
		FOR_ALL_STATIONS(st) {
			st->last_vehicle_type = VEH_INVALID;
		}
	}

	YapfNotifyTrackLayoutChange(INVALID_TILE, INVALID_TRACK);

	if (IsOTTDSavegameVersionBefore(stv, 34)) {
		Company *c;
		FOR_ALL_COMPANIES(c) ResetCompanyLivery(c);
	}

	Company *c;
	FOR_ALL_COMPANIES(c) {
		c->avail_railtypes = GetCompanyRailtypes(c->index);
		c->avail_roadtypes = GetCompanyRoadtypes(c->index);
	}

	if (!IsOTTDSavegameVersionBefore(stv, 27)) AfterLoadStations();

	/* Time starts at 0 instead of 1920.
	 * Account for this in older games by adding an offset */
	if (IsOTTDSavegameVersionBefore(stv, 31)) {
		Station *st;
		Waypoint *wp;
		Engine *e;
		Industry *i;
		Vehicle *v;

		_date += DAYS_TILL_ORIGINAL_BASE_YEAR;
		_cur_year += ORIGINAL_BASE_YEAR;

		FOR_ALL_STATIONS(st)  st->build_date      += DAYS_TILL_ORIGINAL_BASE_YEAR;
		FOR_ALL_WAYPOINTS(wp) wp->build_date      += DAYS_TILL_ORIGINAL_BASE_YEAR;
		FOR_ALL_ENGINES(e)    e->intro_date       += DAYS_TILL_ORIGINAL_BASE_YEAR;
		FOR_ALL_COMPANIES(c)  c->inaugurated_year += ORIGINAL_BASE_YEAR;
		FOR_ALL_INDUSTRIES(i) i->last_prod_year   += ORIGINAL_BASE_YEAR;

		FOR_ALL_VEHICLES(v) {
			v->date_of_last_service += DAYS_TILL_ORIGINAL_BASE_YEAR;
			v->build_year += ORIGINAL_BASE_YEAR;
		}
	}

	/* From 32 on we save the industry who made the farmland.
	 *  To give this prettiness to old savegames, we remove all farmfields and
	 *  plant new ones. */
	if (IsOTTDSavegameVersionBefore(stv, 32)) {
		Industry *i;

		for (TileIndex t = 0; t < map_size; t++) {
			if (IsFieldsTile(t)) {
				/* remove fields */
				MakeClear(t, GROUND_GRASS, 3);
			}
		}

		FOR_ALL_INDUSTRIES(i) {
			uint j;

			if (GetIndustrySpec(i->type)->behaviour & INDUSTRYBEH_PLANT_ON_BUILT) {
				for (j = 0; j != 50; j++) PlantRandomFarmField(i);
			}
		}
	}

	/* Setting no refit flags to all orders in savegames from before refit in orders were added */
	if (IsOTTDSavegameVersionBefore(stv, 36)) {
		Order *order;
		Vehicle *v;

		FOR_ALL_ORDERS(order) {
			order->SetRefit(CT_NO_REFIT);
		}

		FOR_ALL_VEHICLES(v) {
			v->current_order.SetRefit(CT_NO_REFIT);
		}
	}

	/* from legacy version 38 we have optional elrails, since we cannot know the
	 * preference of a user, let elrails enabled; it can be disabled manually */
	if (IsOTTDSavegameVersionBefore(stv, 38)) _settings_game.vehicle.disable_elrails = false;
	/* do the same as when elrails were enabled/disabled manually just now */
	SettingsDisableElrail(_settings_game.vehicle.disable_elrails);
	InitializeRailGUI();

	/* Check and update house and town values */
	UpdateHousesAndTowns();

	if (IsOTTDSavegameVersionBefore(stv, 43)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsIndustryTile(t)) {
				switch (GetIndustryGfx(t)) {
					case GFX_POWERPLANT_SPARKS:
						_mc[t].m3 = GB(_mc[t].m1, 2, 5);
						break;

					case GFX_OILWELL_ANIMATED_1:
					case GFX_OILWELL_ANIMATED_2:
					case GFX_OILWELL_ANIMATED_3:
						_mc[t].m3 = GB(_mc[t].m1, 0, 2);
						break;

					case GFX_COAL_MINE_TOWER_ANIMATED:
					case GFX_COPPER_MINE_TOWER_ANIMATED:
					case GFX_GOLD_MINE_TOWER_ANIMATED:
						 _mc[t].m3 = _mc[t].m1;
						 break;

					default: // No animation states to change
						break;
				}
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 45)) {
		Vehicle *v;
		/* Originally just the fact that some cargo had been paid for was
		 * stored to stop people cheating and cashing in several times. This
		 * wasn't enough though as it was cleared when the vehicle started
		 * loading again, even if it didn't actually load anything, so now the
		 * amount that has been paid is stored. */
		FOR_ALL_VEHICLES(v) {
			ClrBit(v->vehicle_flags, 2);
		}
	}

	/* Buoys do now store the owner of the previous water tile, which can never
	 * be OWNER_NONE. So replace OWNER_NONE with OWNER_WATER. */
	if (IsOTTDSavegameVersionBefore(stv, 46)) {
		Waypoint *wp;
		FOR_ALL_WAYPOINTS(wp) {
			if ((wp->facilities & FACIL_DOCK) != 0 && IsTileOwner(wp->xy, OWNER_NONE) && TileHeight(wp->xy) == 0) SetTileOwner(wp->xy, OWNER_WATER);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 50)) {
		Aircraft *v;
		/* Aircraft units changed from 8 mph to 1 km-ish/h */
		FOR_ALL_AIRCRAFT(v) {
			if (v->subtype <= AIR_AIRCRAFT) {
				const AircraftVehicleInfo *avi = AircraftVehInfo(v->engine_type);
				v->cur_speed *= 128;
				v->cur_speed /= 10;
				v->acceleration = avi->acceleration;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 49)) FOR_ALL_COMPANIES(c) c->face = ConvertFromOldCompanyManagerFace(c->face);

	if (IsOTTDSavegameVersionBefore(stv, 52)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsObjectTypeTile(t, OBJECT_STATUE)) {
				_mc[t].m2 = CalcClosestTownFromTile(t)->index;
			}
		}
	}

	/* A setting containing the proportion of towns that grow twice as
	 * fast was added in legacy version 54. From version 56 this is now saved in the
	 * town as cities can be built specifically in the scenario editor. */
	if (IsOTTDSavegameVersionBefore(stv, 56)) {
		Town *t;

		FOR_ALL_TOWNS(t) {
			if (_settings_game.economy.larger_towns != 0 && (t->index % _settings_game.economy.larger_towns) == 0) {
				t->larger_town = true;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 57)) {
		Vehicle *v;
		/* Added a FIFO queue of vehicles loading at stations */
		FOR_ALL_VEHICLES(v) {
			if ((v->type != VEH_TRAIN || Train::From(v)->IsFrontEngine()) &&  // for all locs
					!(v->vehstatus & (VS_STOPPED | VS_CRASHED)) && // not stopped or crashed
					v->current_order.IsType(OT_LOADING)) {         // loading
				Station::Get(v->last_station_visited)->loading_vehicles.push_back(v);

				/* The loading finished flag is *only* set when actually completely
				 * finished. Because the vehicle is loading, it is not finished. */
				ClrBit(v->vehicle_flags, VF_LOADING_FINISHED);
			}
		}
	} else if (IsOTTDSavegameVersionBefore(stv, 59)) {
		/* For some reason non-loading vehicles could be in the station's loading vehicle list */

		Station *st;
		FOR_ALL_STATIONS(st) {
			std::list<Vehicle *>::iterator iter;
			for (iter = st->loading_vehicles.begin(); iter != st->loading_vehicles.end();) {
				Vehicle *v = *iter;
				iter++;
				if (!v->current_order.IsType(OT_LOADING)) st->loading_vehicles.remove(v);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 58)) {
		/* Setting difficulty industry_density other than zero get bumped to +1
		 * since a new option (very low at position 1) has been added */
		if (_settings_game.difficulty.industry_density > 0) {
			_settings_game.difficulty.industry_density++;
		}

		/* Same goes for number of towns, although no test is needed, just an increment */
		_settings_game.difficulty.number_towns++;
	}

	if (IsOTTDSavegameVersionBefore(stv, 69)) {
		/* In some old savegames a bit was cleared when it should not be cleared */
		RoadVehicle *rv;
		FOR_ALL_ROADVEHICLES(rv) {
			if (rv->state == 250 || rv->state == 251) {
				SetBit(rv->state, 2);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 70)) {
		/* Added variables to support newindustries */
		Industry *i;
		FOR_ALL_INDUSTRIES(i) i->founder = OWNER_NONE;
	}

	if (IsOTTDSavegameVersionBefore(stv, 74)) {
		Station *st;
		FOR_ALL_STATIONS(st) {
			for (CargoID c = 0; c < NUM_CARGO; c++) {
				st->goods[c].last_speed = 0;
				if (st->goods[c].cargo.AvailableCount() != 0) SetBit(st->goods[c].acceptance_pickup, GoodsEntry::GES_PICKUP);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 78)) {
		Industry *i;
		uint j;
		FOR_ALL_INDUSTRIES(i) {
			const IndustrySpec *indsp = GetIndustrySpec(i->type);
			for (j = 0; j < lengthof(i->produced_cargo); j++) {
				i->produced_cargo[j] = indsp->produced_cargo[j];
			}
			for (j = 0; j < lengthof(i->accepts_cargo); j++) {
				i->accepts_cargo[j] = indsp->accepts_cargo[j];
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 93)) {
		/* Rework of orders. */
		Order *order;
		FOR_ALL_ORDERS(order) order->ConvertFromOldSavegame(stv);

		Vehicle *v;
		FOR_ALL_VEHICLES(v) {
			if (v->orders.list != NULL && v->orders.list->GetFirstOrder() != NULL && v->orders.list->GetFirstOrder()->IsType(OT_NOTHING)) {
				v->orders.list->FreeChain();
				v->orders.list = NULL;
			}

			v->current_order.ConvertFromOldSavegame(stv);
			if (v->type == VEH_ROAD && v->IsPrimaryVehicle() && v->FirstShared() == v) {
				FOR_VEHICLE_ORDERS(v, order) order->SetNonStopType(ONSF_NO_STOP_AT_INTERMEDIATE_STATIONS);
			}
		}
	} else if (IsOTTDSavegameVersionBefore(stv, 94)) {
		/* Unload and transfer are now mutual exclusive. */
		Order *order;
		FOR_ALL_ORDERS(order) {
			if ((order->GetUnloadType() & (OUFB_UNLOAD | OUFB_TRANSFER)) == (OUFB_UNLOAD | OUFB_TRANSFER)) {
				order->SetUnloadType(OUFB_TRANSFER);
				order->SetLoadType(OLFB_NO_LOAD);
			}
		}

		Vehicle *v;
		FOR_ALL_VEHICLES(v) {
			if ((v->current_order.GetUnloadType() & (OUFB_UNLOAD | OUFB_TRANSFER)) == (OUFB_UNLOAD | OUFB_TRANSFER)) {
				v->current_order.SetUnloadType(OUFB_TRANSFER);
				v->current_order.SetLoadType(OLFB_NO_LOAD);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 84)) {
		/* Set all share owners to INVALID_COMPANY for
		 * 1) all inactive companies
		 *     (when inactive companies were stored in the savegame - TTD, TTDP and some
		 *      *really* old revisions of OTTD; else it is already set in InitializeCompanies())
		 * 2) shares that are owned by inactive companies or self
		 *     (caused by cheating clients in earlier revisions) */
		FOR_ALL_COMPANIES(c) {
			for (uint i = 0; i < 4; i++) {
				CompanyID company = c->share_owners[i];
				if (company == INVALID_COMPANY) continue;
				if (!Company::IsValidID(company) || company == c->index) c->share_owners[i] = INVALID_COMPANY;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 86)) {
		/* Update locks, depots, docks and buoys to have a water class based
		 * on its neighbouring tiles. Done after river and canal updates to
		 * ensure neighbours are correct. */
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsTileFlat(t)) continue;

			if (IsWaterTile(t) && IsLock(t)) GuessWaterClass(t, false);
			if (IsStationTile(t) && (IsDock(t) || IsBuoy(t))) GuessWaterClass(t, false);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 87)) {
		for (TileIndex t = 0; t < map_size; t++) {
			/* skip oil rigs at borders! */
			if ((IsWaterTile(t) || IsBuoyTile(t)) &&
					(TileX(t) == 0 || TileY(t) == 0 || TileX(t) == MapMaxX() - 1 || TileY(t) == MapMaxY() - 1)) {
				/* Some version 86 savegames have wrong water class at map borders (under buoy, or after removing buoy).
				 * This conversion has to be done before buoys with invalid owner are removed. */
				SetWaterClass(t, WATER_CLASS_SEA);
			}

			if (IsBuoyTile(t) || IsDriveThroughStopTile(t) || IsWaterTile(t)) {
				Owner o = GetTileOwner(t);
				if (o < MAX_COMPANIES && !Company::IsValidID(o)) {
					Backup<CompanyByte> cur_company(_current_company, o, FILE_LINE);
					ChangeTileOwner(t, o, INVALID_OWNER);
					cur_company.Restore();
				}
				if (IsBuoyTile(t)) {
					/* reset buoy owner to OWNER_NONE in the station struct
					 * (even if it is owned by active company) */
					Waypoint::GetByTile(t)->owner = OWNER_NONE;
				}
			} else if (IsRoadTile(t) || IsLevelCrossingTile(t)) {
				/* works for all RoadTileType */
				for (RoadType rt = ROADTYPE_ROAD; rt < ROADTYPE_END; rt++) {
					/* update even non-existing road types to update tile owner too */
					Owner o = GetRoadOwner(t, rt);
					if (o < MAX_COMPANIES && !Company::IsValidID(o)) SetRoadOwner(t, rt, OWNER_NONE);
				}
				if (IsLevelCrossingTile(t)) {
					if (!Company::IsValidID(GetTileOwner(t))) FixOwnerOfRailTrack(t);
				}
			} else if (IsNormalRailTile(t)) {
				if (!Company::IsValidID(GetTileOwner(t))) FixOwnerOfRailTrack(t);
			}
		}

		/* Convert old PF settings to new */
		if (_settings_game.pf.yapf.rail_use_yapf || IsOTTDSavegameVersionBefore(stv, 28)) {
			_settings_game.pf.pathfinder_for_trains = VPF_YAPF;
		} else {
			_settings_game.pf.pathfinder_for_trains = VPF_NPF;
		}

		if (_settings_game.pf.yapf.road_use_yapf || IsOTTDSavegameVersionBefore(stv, 28)) {
			_settings_game.pf.pathfinder_for_roadvehs = VPF_YAPF;
		} else {
			_settings_game.pf.pathfinder_for_roadvehs = VPF_NPF;
		}

		if (_settings_game.pf.yapf.ship_use_yapf) {
			_settings_game.pf.pathfinder_for_ships = VPF_YAPF;
		} else {
			_settings_game.pf.pathfinder_for_ships = (_settings_game.pf.new_pathfinding_all ? VPF_NPF : VPF_OPF);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 88)) {
		/* Profits are now with 8 bit fract */
		Vehicle *v;
		FOR_ALL_VEHICLES(v) {
			v->profit_this_year <<= 8;
			v->profit_last_year <<= 8;
			v->running_ticks = 0;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 91)) {
		/* Increase HouseAnimationFrame from 5 to 7 bits */
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsHouseTile(t) && GetHouseType(t) >= NEW_HOUSE_OFFSET) {
				SB(_mc[t].m1, 0, 6, GB(_mc[t].m1, 1, 5));
				SB(_mc[t].m0, 5, 1, 0);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 62)) {
		/* Remove all trams from savegames without tram support.
		 * There would be trams without tram track under causing crashes sooner or later. */
		RoadVehicle *v;
		FOR_ALL_ROADVEHICLES(v) {
			if (v->First() == v && HasBit(EngInfo(v->engine_type)->misc_flags, EF_ROAD_TRAM)) {
				ShowErrorMessage(STR_WARNING_LOADGAME_REMOVED_TRAMS, INVALID_STRING_ID, WL_CRITICAL);
				delete v;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 99)) {
		for (TileIndex t = 0; t < map_size; t++) {
			/* Set newly introduced WaterClass of industry tiles */
			if (IsStationTile(t) && IsOilRig(t)) {
				GuessWaterClass(t, true);
			}
			if (IsIndustryTile(t)) {
				if ((GetIndustrySpec(GetIndustryType(t))->behaviour & INDUSTRYBEH_BUILT_ONWATER) != 0) {
					GuessWaterClass(t, true);
				} else {
					SetWaterClass(t, WATER_CLASS_INVALID);
				}
			}

			/* Replace "house construction year" with "house age" */
			if (IsHouseTile(t) && IsHouseCompleted(t)) {
				_mc[t].m5 = Clamp(_cur_year - (_mc[t].m5 + ORIGINAL_BASE_YEAR), 0, 0xFF);
			}
		}
	}

	/* Reserve all tracks trains are currently on. */
	if (IsOTTDSavegameVersionBefore(stv, 101)) {
		const Train *t;
		FOR_ALL_TRAINS(t) {
			if (t->First() == t) t->ReserveTrackUnderConsist();
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 102)) {
		for (TileIndex t = 0; t < map_size; t++) {
			/* Now all crossings should be in correct state */
			if (IsLevelCrossingTile(t)) UpdateLevelCrossing(t, false);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 103)) {
		/* Non-town-owned roads now store the closest town */
		UpdateNearestTownForRoadTiles(false);

		/* signs with invalid owner left from older savegames */
		Sign *si;
		FOR_ALL_SIGNS(si) {
			if (si->owner != OWNER_NONE && !Company::IsValidID(si->owner)) si->owner = OWNER_NONE;
		}

		/* Station can get named based on an industry type, but the current ones
		 * are not, so mark them as if they are not named by an industry. */
		Station *st;
		FOR_ALL_STATIONS(st) {
			st->indtype = IT_INVALID;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 104)) {
		Aircraft *a;
		FOR_ALL_AIRCRAFT(a) {
			/* Set engine_type of shadow and rotor */
			if (!a->IsNormalAircraft()) {
				a->engine_type = a->First()->engine_type;
			}
		}

		/* More companies ... */
		Company *c;
		FOR_ALL_COMPANIES(c) {
			if (c->bankrupt_asked == 0xFF) c->bankrupt_asked = 0xFFFF;
		}

		Engine *e;
		FOR_ALL_ENGINES(e) {
			if (e->company_avail == 0xFF) e->company_avail = 0xFFFF;
		}

		Town *t;
		FOR_ALL_TOWNS(t) {
			if (t->have_ratings == 0xFF) t->have_ratings = 0xFFFF;
			for (uint i = 8; i != MAX_COMPANIES; i++) t->ratings[i] = RATING_INITIAL;
		}
	}

	/* Count objects, and delete stale objects in old versions. */
	AfterLoadObjects(stv);

	if (IsOTTDSavegameVersionBefore(stv, 147) && Object::GetNumItems() == 0) {
		/* Make real objects for object tiles. */
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsObjectTile(t)) continue;

			if (Town::GetNumItems() == 0) {
				/* No towns, so remove all objects! */
				DoClearSquare(t);
			} else {
				uint offset = _mc[t].m4;
				_mc[t].m4 = 0;

				if (offset == 0) {
					/* No offset, so make the object. */
					ObjectType type = GetObjectType(t);
					int size = type == OBJECT_HQ ? 2 : 1;

					if (!Object::CanAllocateItem()) {
						/* Nice... you managed to place 64k lighthouses and
						 * antennae on the map... boohoo. */
						throw SlException(STR_ERROR_TOO_MANY_OBJECTS);
					}

					Object *o = new Object();
					o->location.tile = t;
					o->location.w    = size;
					o->location.h    = size;
					o->build_date    = _date;
					o->town          = type == OBJECT_STATUE ? Town::Get(_mc[t].m2) : CalcClosestTownFromTile(t, UINT_MAX);
					_mc[t].m2 = o->index;
					Object::IncTypeCount(type);
				} else {
					/* We're at an offset, so get the ID from our "root". */
					TileIndex northern_tile = t - TileXY(GB(offset, 0, 4), GB(offset, 4, 4));
					assert(IsObjectTile(northern_tile));
					_mc[t].m2 = _mc[northern_tile].m2;
				}
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 113)) {
		/* allow_town_roads is added, set it if town_layout wasn't TL_NO_ROADS */
		if (_settings_game.economy.town_layout == 0) { // was TL_NO_ROADS
			_settings_game.economy.allow_town_roads = false;
			_settings_game.economy.town_layout = TL_BETTER_ROADS;
		} else {
			_settings_game.economy.allow_town_roads = true;
			_settings_game.economy.town_layout = _settings_game.economy.town_layout - 1;
		}

		/* Initialize layout of all towns. Older versions were using different
		 * generator for random town layout, use it if needed. */
		Town *t;
		FOR_ALL_TOWNS(t) {
			if (_settings_game.economy.town_layout != TL_RANDOM) {
				t->layout = _settings_game.economy.town_layout;
				continue;
			}

			/* Use old layout randomizer code */
			byte layout = TileHash(TileX(t->xy), TileY(t->xy)) % 6;
			switch (layout) {
				default: break;
				case 5: layout = 1; break;
				case 0: layout = 2; break;
			}
			t->layout = layout - 1;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 114)) {
		/* There could be (deleted) stations with invalid owner, set owner to OWNER NONE.
		 * The conversion affects oil rigs and buoys too, but it doesn't matter as
		 * they have st->owner == OWNER_NONE already. */
		Station *st;
		FOR_ALL_STATIONS(st) {
			if (!Company::IsValidID(st->owner)) st->owner = OWNER_NONE;
		}
	}

	/* Trains could now stop in a specific location. */
	if (IsOTTDSavegameVersionBefore(stv, 117)) {
		Order *o;
		FOR_ALL_ORDERS(o) {
			if (o->IsType(OT_GOTO_STATION)) o->SetStopLocation(OSL_PLATFORM_FAR_END);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 120)) {
		extern VehicleDefaultSettings _old_vds;
		Company *c;
		FOR_ALL_COMPANIES(c) {
			c->settings.vehicle = _old_vds;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 121)) {
		/* Delete small ufos heading for non-existing vehicles */
		Vehicle *v;
		FOR_ALL_DISASTERVEHICLES(v) {
			if (v->subtype == 2/*ST_SMALL_UFO*/ && v->current_order.GetDestination() != 0) {
				const Vehicle *u = Vehicle::GetIfValid(v->dest_tile);
				if (u == NULL || u->type != VEH_ROAD || !RoadVehicle::From(u)->IsFrontEngine()) {
					delete v;
				}
			}
		}

		/* We didn't store cargo payment yet, so make them for vehicles that are
		 * currently at a station and loading/unloading. If they don't get any
		 * payment anymore they just removed in the next load/unload cycle.
		 * However, some 0.7 versions might have cargo payment. For those we just
		 * add cargopayment for the vehicles that don't have it.
		 */
		Station *st;
		FOR_ALL_STATIONS(st) {
			std::list<Vehicle *>::iterator iter;
			for (iter = st->loading_vehicles.begin(); iter != st->loading_vehicles.end(); ++iter) {
				/* There are always as many CargoPayments as Vehicles. We need to make the
				 * assert() in Pool::GetNew() happy by calling CanAllocateItem(). */
				assert_compile(CargoPaymentPool::MAX_SIZE == VehiclePool::MAX_SIZE);
				assert(CargoPayment::CanAllocateItem());
				Vehicle *v = *iter;
				if (v->cargo_payment == NULL) v->cargo_payment = new CargoPayment(v);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 122)) {
		/* Animated tiles would sometimes not be actually animated or
		 * in case of old savegames duplicate. */

		extern TileIndex *_animated_tile_list;
		extern uint _animated_tile_count;

		for (uint i = 0; i < _animated_tile_count; /* Nothing */) {
			/* Remove if tile is not animated */
			bool remove = GetTileProcs(_animated_tile_list[i])->animate_tile_proc == NULL;

			/* and remove if duplicate */
			for (uint j = 0; !remove && j < i; j++) {
				remove = _animated_tile_list[i] == _animated_tile_list[j];
			}

			if (remove) {
				DeleteAnimatedTile(_animated_tile_list[i]);
			} else {
				i++;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 124) && !IsOTTDSavegameVersionBefore(stv, 1)) {
		/* The train station tile area was added, but for really old (TTDPatch) it's already valid. */
		Waypoint *wp;
		FOR_ALL_WAYPOINTS(wp) {
			if (wp->facilities & FACIL_TRAIN) {
				wp->train_station.tile = wp->xy;
				wp->train_station.w = 1;
				wp->train_station.h = 1;
			} else {
				wp->train_station.tile = INVALID_TILE;
				wp->train_station.w = 0;
				wp->train_station.h = 0;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 125)) {
		/* Convert old subsidies */
		Subsidy *s;
		FOR_ALL_SUBSIDIES(s) {
			if (s->remaining < 12) {
				/* Converting nonawarded subsidy */
				s->remaining = 12 - s->remaining; // convert "age" to "remaining"
				s->awarded = INVALID_COMPANY; // not awarded to anyone
				const CargoSpec *cs = CargoSpec::Get(s->cargo_type);
				switch (cs->town_effect) {
					case TE_PASSENGERS:
					case TE_MAIL:
						/* Town -> Town */
						s->src_type = s->dst_type = ST_TOWN;
						if (Town::IsValidID(s->src) && Town::IsValidID(s->dst)) continue;
						break;
					case TE_GOODS:
					case TE_FOOD:
						/* Industry -> Town */
						s->src_type = ST_INDUSTRY;
						s->dst_type = ST_TOWN;
						if (Industry::IsValidID(s->src) && Town::IsValidID(s->dst)) continue;
						break;
					default:
						/* Industry -> Industry */
						s->src_type = s->dst_type = ST_INDUSTRY;
						if (Industry::IsValidID(s->src) && Industry::IsValidID(s->dst)) continue;
						break;
				}
			} else {
				/* Do our best for awarded subsidies. The original source or destination industry
				 * can't be determined anymore for awarded subsidies, so invalidate them.
				 * Town -> Town subsidies are converted using simple heuristic */
				s->remaining = 24 - s->remaining; // convert "age of awarded subsidy" to "remaining"
				const CargoSpec *cs = CargoSpec::Get(s->cargo_type);
				switch (cs->town_effect) {
					case TE_PASSENGERS:
					case TE_MAIL: {
						/* Town -> Town */
						const Station *ss = Station::GetIfValid(s->src);
						const Station *sd = Station::GetIfValid(s->dst);
						if (ss != NULL && sd != NULL && ss->owner == sd->owner &&
								Company::IsValidID(ss->owner)) {
							s->src_type = s->dst_type = ST_TOWN;
							s->src = ss->town->index;
							s->dst = sd->town->index;
							s->awarded = ss->owner;
							continue;
						}
						break;
					}
					default:
						break;
				}
			}
			/* Awarded non-town subsidy or invalid source/destination, invalidate */
			delete s;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 126)) {
		/* Recompute inflation based on old unround loan limit
		 * Note: Max loan is 500000. With an inflation of 4% across 170 years
		 *       that results in a max loan of about 0.7 * 2^31.
		 *       So taking the 16 bit fractional part into account there are plenty of bits left
		 *       for unmodified savegames ...
		 */
		uint64 aimed_inflation = (_economy.old_max_loan_unround << 16 | _economy.old_max_loan_unround_fract) / _settings_game.difficulty.max_loan;

		/* ... well, just clamp it then. */
		if (aimed_inflation > MAX_INFLATION) aimed_inflation = MAX_INFLATION;

		/* Simulate the inflation, so we also get the payment inflation */
		while (_economy.inflation_prices < aimed_inflation) {
			if (AddInflation(false)) break;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 127)) {
		Station *st;
		FOR_ALL_STATIONS(st) UpdateStationAcceptance(st, false);
	}

	if (IsOTTDSavegameVersionBefore(stv, 128)) {
		const Depot *d;
		FOR_ALL_DEPOTS(d) {
			_mc[d->xy].m2 = d->index;
			if (IsWaterTile(d->xy)) _mc[GetOtherShipDepotTile(d->xy)].m2 = d->index;
		}
	}

	/* The behaviour of force_proceed has been changed. Now
	 * it counts signals instead of some random time out. */
	if (IsOTTDSavegameVersionBefore(stv, 131)) {
		Train *t;
		FOR_ALL_TRAINS(t) {
			if (t->force_proceed != TFP_NONE) {
				t->force_proceed = TFP_STUCK;
			}
		}
	}

	/* Wait counter and load/unload ticks got split. */
	if (IsOTTDSavegameVersionBefore(stv, 136)) {
		Aircraft *a;
		FOR_ALL_AIRCRAFT(a) {
			a->turn_counter = a->current_order.IsType(OT_LOADING) ? 0 : a->load_unload_ticks;
		}

		Train *t;
		FOR_ALL_TRAINS(t) {
			t->wait_counter = t->current_order.IsType(OT_LOADING) ? 0 : t->load_unload_ticks;
		}
	}

	/* Airport tile animation uses animation frame instead of other graphics id */
	if (IsOTTDSavegameVersionBefore(stv, 137)) {
		struct AirportTileConversion {
			byte old_start;
			byte num_frames;
		};
		static const AirportTileConversion atc[] = {
			{31,  12}, // APT_RADAR_GRASS_FENCE_SW
			{50,   4}, // APT_GRASS_FENCE_NE_FLAG
			{62,   2}, // 1 unused tile
			{66,  12}, // APT_RADAR_FENCE_SW
			{78,  12}, // APT_RADAR_FENCE_NE
			{101, 10}, // 9 unused tiles
			{111,  8}, // 7 unused tiles
			{119, 15}, // 14 unused tiles (radar)
			{140,  4}, // APT_GRASS_FENCE_NE_FLAG_2
		};
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsAirportTile(t)) {
				StationGfx old_gfx = GetStationGfx(t);
				byte offset = 0;
				for (uint i = 0; i < lengthof(atc); i++) {
					if (old_gfx < atc[i].old_start) {
						SetStationGfx(t, old_gfx - offset);
						break;
					}
					if (old_gfx < atc[i].old_start + atc[i].num_frames) {
						SetAnimationFrame(t, old_gfx - atc[i].old_start);
						SetStationGfx(t, atc[i].old_start - offset);
						break;
					}
					offset += atc[i].num_frames - 1;
				}
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 140)) {
		Station *st;
		FOR_ALL_STATIONS(st) {
			if (st->airport.tile != INVALID_TILE) {
				st->airport.w = st->airport.GetSpec()->size_x;
				st->airport.h = st->airport.GetSpec()->size_y;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 141)) {
		/* We need to properly number/name the depots.
		 * The first step is making sure none of the depots uses the
		 * 'default' names, after that we can assign the names. */
		Depot *d;
		FOR_ALL_DEPOTS(d) d->town_cn = UINT16_MAX;

		FOR_ALL_DEPOTS(d) MakeDefaultName(d);
	}

	if (IsOTTDSavegameVersionBefore(stv, 142)) {
		Depot *d;
		FOR_ALL_DEPOTS(d) d->build_date = _date;
	}

	/* In old versions it was possible to remove an airport while a plane was
	 * taking off or landing. This gives all kind of problems when building
	 * another airport in the same station so we don't allow that anymore.
	 * For old savegames with such aircraft we just throw them in the air and
	 * treat the aircraft like they were flying already. */
	if (IsOTTDSavegameVersionBefore(stv, 146)) {
		Aircraft *v;
		FOR_ALL_AIRCRAFT(v) {
			if (!v->IsNormalAircraft()) continue;
			Station *st = GetTargetAirportIfValid(v);
			if (st == NULL && v->state != FLYING) {
				v->state = FLYING;
				UpdateAircraftCache(v);
				AircraftNextAirportPos_and_Order(v);
				/* get aircraft back on running altitude */
				if ((v->vehstatus & VS_CRASHED) == 0) SetAircraftPosition(v, v->x_pos, v->y_pos, GetAircraftFlyingAltitude(v));
			}
		}
	}

	/* Move the animation frame to the same location (m7) for all objects. */
	if (IsOTTDSavegameVersionBefore(stv, 147)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsHouseTile(t) && GetHouseType(t) >= NEW_HOUSE_OFFSET) {
				uint per_proc = _mc[t].m7;
				_mc[t].m7 = GB(_mc[t].m1, 0, 6) | (GB(_mc[t].m0, 5, 1) << 6);
				SB(_mc[t].m0, 5, 1, 0);
				SB(_mc[t].m1, 0, 6, min(per_proc, 63));
			}
		}
	}

	/* Add (random) colour to all objects. */
	if (IsOTTDSavegameVersionBefore(stv, 148)) {
		Object *o;
		FOR_ALL_OBJECTS(o) {
			Owner owner = GetTileOwner(o->location.tile);
			o->colour = (owner == OWNER_NONE) ? Random() & 0xF : Company::Get(owner)->livery->colour1;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 149)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsStationTile(t)) continue;
			if (!IsBuoy(t) && !IsOilRig(t) && !(IsDock(t) && IsTileFlat(t))) {
				SetWaterClass(t, WATER_CLASS_INVALID);
			}
		}

		/* Waypoints with custom name may have a non-unique town_cn,
		 * renumber those. First set all affected waypoints to the
		 * highest possible number to get them numbered in the
		 * order they have in the pool. */
		Waypoint *wp;
		FOR_ALL_WAYPOINTS(wp) {
			if (wp->name != NULL) wp->town_cn = UINT16_MAX;
		}

		FOR_ALL_WAYPOINTS(wp) {
			if (wp->name != NULL) MakeDefaultName(wp);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 152)) {
		_industry_builder.Reset(); // Initialize industry build data.

		/* The moment vehicles go from hidden to visible changed. This means
		 * that vehicles don't always get visible anymore causing things to
		 * get messed up just after loading the savegame. This fixes that. */
		Vehicle *v;
		FOR_ALL_VEHICLES(v) {
			/* Not all vehicle types can be inside a tunnel. Furthermore,
			 * testing IsTunnelTile() for invalid tiles causes a crash. */
			if (!v->IsGroundVehicle()) continue;

			/* Is the vehicle in a tunnel? */
			if (!IsTunnelTile(v->tile)) continue;

			/* Is the vehicle actually at a tunnel entrance/exit? */
			TileIndex vtile = TileVirtXY(v->x_pos, v->y_pos);
			if (!IsTunnelTile(vtile)) continue;

			/* Are we actually in this tunnel? Or maybe a lower tunnel? */
			if (GetSlopePixelZ(v->x_pos, v->y_pos) != v->z_pos) continue;

			/* What way are we going? */
			const DiagDirection dir = GetTunnelBridgeDirection(vtile);
			const DiagDirection vdir = DirToDiagDir(v->direction);

			/* Have we passed the visibility "switch" state already? */
			byte pos = (DiagDirToAxis(vdir) == AXIS_X ? v->x_pos : v->y_pos) & TILE_UNIT_MASK;
			byte frame = (vdir == DIAGDIR_NE || vdir == DIAGDIR_NW) ? TILE_SIZE - 1 - pos : pos;
			extern const byte _tunnel_visibility_frame[DIAGDIR_END];

			/* Should the vehicle be hidden or not? */
			bool hidden;
			if (dir == vdir) { // Entering tunnel
				hidden = frame >= _tunnel_visibility_frame[dir];
				v->tile = vtile;
			} else if (dir == ReverseDiagDir(vdir)) { // Leaving tunnel
				hidden = frame < TILE_SIZE - _tunnel_visibility_frame[dir];
				/* v->tile changes at the moment when the vehicle leaves the tunnel. */
				v->tile = hidden ? GetOtherTunnelEnd(vtile) : vtile;
			} else {
				/* We could get here in two cases:
				 * - for road vehicles, it is reversing at the end of the tunnel
				 * - it is crashed in the tunnel entry (both train or RV destroyed by UFO)
				 * Whatever case it is, do not change anything and use the old values.
				 * Especially changing RV's state would break its reversing in the middle. */
				continue;
			}

			if (hidden) {
				v->vehstatus |= VS_HIDDEN;

				switch (v->type) {
					case VEH_TRAIN: Train::From(v)->trackdir    = TRACKDIR_WORMHOLE; break;
					case VEH_ROAD:  RoadVehicle::From(v)->state = RVSB_WORMHOLE;     break;
					default: NOT_REACHED();
				}
			} else {
				v->vehstatus &= ~VS_HIDDEN;

				switch (v->type) {
					case VEH_TRAIN: Train::From(v)->trackdir    = DiagDirToDiagTrackdir(vdir); break;
					case VEH_ROAD:  RoadVehicle::From(v)->state = DiagDirToDiagTrackdir(vdir); RoadVehicle::From(v)->frame = frame; break;
					default: NOT_REACHED();
				}
			}
		}
	}

	if (IsFullSavegameVersionBefore(stv, 5)) {
		Vehicle *v;

		FOR_ALL_VEHICLES(v) {
			switch (v->type) {
				case VEH_TRAIN: {
					Train *t = Train::From(v);
					if (t->trackdir == TRACKDIR_WORMHOLE) {
						TileIndex other_end = GetOtherTunnelBridgeEnd(v->tile);
						TileIndex vt = TileVirtXY(v->x_pos, v->y_pos);
						if (vt == v->tile || vt == other_end) {
							v->tile = vt;
							t->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
						} else if (v->direction == DiagDirToDir(GetTunnelBridgeDirection(v->tile))) {
							v->tile = other_end;
						}
					}
					break;
				}

				case VEH_ROAD: {
					RoadVehicle *rv = RoadVehicle::From(v);
					if (rv->state == RVSB_WORMHOLE) {
						TileIndex other_end = GetOtherTunnelBridgeEnd(v->tile);
						TileIndex vt = TileVirtXY(v->x_pos, v->y_pos);
						if (vt == v->tile || vt == other_end) {
							DiagDirection dir = DirToDiagDir(v->direction);
							v->tile = vt;
							rv->state = DiagDirToDiagTrackdir(dir);
							rv->frame = DistanceFromTileEdge(ReverseDiagDir(dir), v->x_pos & TILE_UNIT_MASK, v->y_pos & TILE_UNIT_MASK);
						} else if (v->direction == DiagDirToDir(GetTunnelBridgeDirection(v->tile))) {
							v->tile = other_end;
						}
					}
					break;
				}

				case VEH_SHIP: {
					Ship *s = Ship::From(v);
					if (s->trackdir == TRACKDIR_WORMHOLE) {
						TileIndex other_end = GetOtherBridgeEnd(v->tile);
						TileIndex vt = TileVirtXY(v->x_pos, v->y_pos);
						if (vt == v->tile || vt == other_end) {
							v->tile = vt;
							s->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
						} else if (v->direction == DiagDirToDir(GetTunnelBridgeDirection(v->tile))) {
							v->tile = other_end;
						}
					}
					break;
				}

				default: break;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 153)) {
		RoadVehicle *rv;
		FOR_ALL_ROADVEHICLES(rv) {
			if (rv->state == RVSB_IN_DEPOT || rv->state == RVSB_WORMHOLE) continue;

			bool loading = rv->current_order.IsType(OT_LOADING) || rv->current_order.IsType(OT_LEAVESTATION);
			if (HasBit(rv->state, RVS_IN_ROAD_STOP)) {
				extern const byte _road_stop_stop_frame[2][TRACKDIR_END];
				SB(rv->state, RVS_ENTERED_STOP, 1, loading || rv->frame > _road_stop_stop_frame[_settings_game.vehicle.road_side][rv->state & RVSB_TRACKDIR_MASK]);
			} else if (HasBit(rv->state, RVS_IN_DT_ROAD_STOP)) {
				SB(rv->state, RVS_ENTERED_STOP, 1, loading || rv->frame > RVC_DRIVE_THROUGH_STOP_FRAME);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 156)) {
		/* The train's pathfinder lost flag got moved. */
		Train *t;
		FOR_ALL_TRAINS(t) {
			if (!HasBit(t->flags, 5)) continue;

			ClrBit(t->flags, 5);
			SetBit(t->vehicle_flags, VF_PATHFINDER_LOST);
		}

		/* Introduced terraform/clear limits. */
		Company *c;
		FOR_ALL_COMPANIES(c) {
			c->terraform_limit = _settings_game.construction.terraform_frame_burst << 16;
			c->clear_limit     = _settings_game.construction.clear_frame_burst << 16;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 158)) {
		Vehicle *v;
		FOR_ALL_VEHICLES(v) {
			switch (v->type) {
				case VEH_TRAIN: {
					Train *t = Train::From(v);

					/* Clear old GOINGUP / GOINGDOWN flags.
					 * It was changed in savegame version 139, but savegame
					 * version 158 doesn't use these bits, so it doesn't hurt
					 * to clear them unconditionally. */
					ClrBit(t->flags, 1);
					ClrBit(t->flags, 2);

					/* Clear both bits first. */
					ClrBit(t->gv_flags, GVF_GOINGUP_BIT);
					ClrBit(t->gv_flags, GVF_GOINGDOWN_BIT);

					/* Crashed vehicles can't be going up/down. */
					if (t->vehstatus & VS_CRASHED) break;

					/* Only X/Y tracks can be sloped. */
					if (!IsDiagonalTrackdir(t->trackdir)) break;

					t->gv_flags |= FixVehicleInclination(t, t->direction);
					break;
				}
				case VEH_ROAD: {
					RoadVehicle *rv = RoadVehicle::From(v);
					ClrBit(rv->gv_flags, GVF_GOINGUP_BIT);
					ClrBit(rv->gv_flags, GVF_GOINGDOWN_BIT);

					/* Crashed vehicles can't be going up/down. */
					if (rv->vehstatus & VS_CRASHED) break;

					if (rv->state == RVSB_IN_DEPOT || rv->state == RVSB_WORMHOLE) break;

					TrackStatus ts = GetTileRoadStatus(rv->tile, rv->compatible_roadtypes);
					TrackBits trackbits = TrackStatusToTrackBits(ts);

					/* Only X/Y tracks can be sloped. */
					if (trackbits != TRACK_BIT_X && trackbits != TRACK_BIT_Y) break;

					Direction dir = rv->direction;

					/* Test if we are reversing. */
					Axis a = trackbits == TRACK_BIT_X ? AXIS_X : AXIS_Y;
					if (AxisToDirection(a) != dir &&
							AxisToDirection(a) != ReverseDir(dir)) {
						/* When reversing, the road vehicle is on the edge of the tile,
						 * so it can be safely compared to the middle of the tile. */
						dir = INVALID_DIR;
					}

					rv->gv_flags |= FixVehicleInclination(rv, dir);
					break;
				}
				case VEH_SHIP:
					break;

				default:
					continue;
			}

			if (IsBridgeHeadTile(v->tile) && TileVirtXY(v->x_pos, v->y_pos) == v->tile) {
				/* In old versions, z_pos was 1 unit lower on bridge heads.
				 * However, this invalid state could be converted to new savegames
				 * by loading and saving the game in a new version. */
				v->z_pos = GetSlopePixelZ(v->x_pos, v->y_pos);
				DiagDirection dir = GetTunnelBridgeDirection(v->tile);
				if (v->type == VEH_TRAIN && !(v->vehstatus & VS_CRASHED) &&
						v->direction != DiagDirToDir(dir)) {
					/* If the train has left the bridge, it shouldn't have
					 * trackdir == TRACKDIR_WORMHOLE - this could happen
					 * when the train was reversed while on the last "tick"
					 * on the ramp before leaving the ramp to the bridge. */
					Train::From(v)->trackdir = DiagDirToDiagTrackdir(ReverseDiagDir(dir));
				}
			}

			/* If the vehicle is really above v->tile (not in a wormhole),
			 * it should have set v->z_pos correctly. */
			assert(v->tile != TileVirtXY(v->x_pos, v->y_pos) || v->z_pos == GetSlopePixelZ(v->x_pos, v->y_pos));
		}

		/* Fill Vehicle::cur_real_order_index */
		FOR_ALL_VEHICLES(v) {
			if (!v->IsPrimaryVehicle()) continue;

			/* Older versions are less strict with indices being in range and fix them on the fly */
			if (v->cur_implicit_order_index >= v->GetNumOrders()) v->cur_implicit_order_index = 0;

			v->cur_real_order_index = v->cur_implicit_order_index;
			v->UpdateRealOrderIndex();
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 159)) {
		/* If the savegame is old (before legacy version 100), then the value of 255
		 * for these settings did not mean "disabled". As such everything
		 * before then did reverse.
		 * To simplify stuff we disable all turning around or we do not
		 * disable anything at all. So, if some reversing was disabled we
		 * will keep reversing disabled, otherwise it'll be turned on. */
		_settings_game.pf.reverse_at_signals = IsOTTDSavegameVersionBefore(stv, 100) || (_settings_game.pf.wait_oneway_signal != 255 && _settings_game.pf.wait_twoway_signal != 255 && _settings_game.pf.wait_for_pbs_path != 255);

		Train *t;
		FOR_ALL_TRAINS(t) {
			_settings_game.vehicle.max_train_length = max<uint8>(_settings_game.vehicle.max_train_length, CeilDiv(t->gcache.cached_total_length, TILE_SIZE));
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 160)) {
		/* Setting difficulty industry_density other than zero get bumped to +1
		 * since a new option (minimal at position 1) has been added */
		if (_settings_game.difficulty.industry_density > 0) {
			_settings_game.difficulty.industry_density++;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 161)) {
		/* Before savegame version 161, persistent storages were not stored in a pool. */

		if (!IsOTTDSavegameVersionBefore(stv, 76)) {
			Industry *ind;
			FOR_ALL_INDUSTRIES(ind) {
				assert(ind->psa != NULL);

				/* Check if the old storage was empty. */
				bool is_empty = true;
				for (uint i = 0; i < sizeof(ind->psa->storage); i++) {
					if (ind->psa->GetValue(i) != 0) {
						is_empty = false;
						break;
					}
				}

				if (!is_empty) {
					ind->psa->grfid = _industry_mngr.GetGRFID(ind->type);
				} else {
					delete ind->psa;
					ind->psa = NULL;
				}
			}
		}

		if (!IsOTTDSavegameVersionBefore(stv, 145)) {
			Station *st;
			FOR_ALL_STATIONS(st) {
				if (!(st->facilities & FACIL_AIRPORT)) continue;
				assert(st->airport.psa != NULL);

				/* Check if the old storage was empty. */
				bool is_empty = true;
				for (uint i = 0; i < sizeof(st->airport.psa->storage); i++) {
					if (st->airport.psa->GetValue(i) != 0) {
						is_empty = false;
						break;
					}
				}

				if (!is_empty) {
					st->airport.psa->grfid = _airport_mngr.GetGRFID(st->airport.type);
				} else {
					delete st->airport.psa;
					st->airport.psa = NULL;

				}
			}
		}
	}

	/* This triggers only when old snow_lines were copied into the snow_line_height. */
	if (IsOTTDSavegameVersionBefore(stv, 164) && _settings_game.game_creation.snow_line_height >= MIN_SNOWLINE_HEIGHT * TILE_HEIGHT) {
		_settings_game.game_creation.snow_line_height /= TILE_HEIGHT;
	}

	/* The center of train vehicles was changed, fix up spacing. */
	if (IsOTTDSavegameVersionBefore(stv, 164)) FixupTrainLengths();

	if (IsOTTDSavegameVersionBefore(stv, 165)) {
		Town *t;

		FOR_ALL_TOWNS(t) {
			/* Set the default cargo requirement for town growth */
			switch (_settings_game.game_creation.landscape) {
				case LT_ARCTIC:
					if (FindFirstCargoWithTownEffect(TE_FOOD) != NULL) t->goal[TE_FOOD] = TOWN_GROWTH_WINTER;
					break;

				case LT_TROPIC:
					if (FindFirstCargoWithTownEffect(TE_FOOD) != NULL) t->goal[TE_FOOD] = TOWN_GROWTH_DESERT;
					if (FindFirstCargoWithTownEffect(TE_WATER) != NULL) t->goal[TE_WATER] = TOWN_GROWTH_DESERT;
					break;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 165)) {
		/* Adjust zoom level to account for new levels */
		_saved_scrollpos_zoom = _saved_scrollpos_zoom + ZOOM_LVL_SHIFT;
		_saved_scrollpos_x *= ZOOM_LVL_BASE;
		_saved_scrollpos_y *= ZOOM_LVL_BASE;
	}

	if (IsFullSavegameVersionBefore(stv, 10)) {
		Train *t;
		FOR_ALL_TRAINS(t) {
			Train *last = t->Last();
			if (IsRailBridgeTile(last->tile) && (DirToDiagDir(last->direction) == ReverseDiagDir(GetTunnelBridgeDirection(last->tile)))) {
				/* Clear reservation for already left bridge parts */
				TileIndex other_end = GetOtherBridgeEnd(last->tile);
				SetTrackReservation(other_end, TRACK_BIT_NONE);
				if (last->trackdir != TRACKDIR_WORMHOLE) {
					SetBridgeMiddleReservation(last->tile, false);
					SetBridgeMiddleReservation(other_end, false);
				}
			} else if (IsTunnelTile(last->tile) && (DirToDiagDir(last->direction) == ReverseDiagDir(GetTunnelBridgeDirection(last->tile)))) {
				/* Clear reservation for already left tunnel parts */
				TileIndex other_end = GetOtherTunnelEnd(last->tile);
				SetTunnelHeadReservation(other_end, false);
				if (last->trackdir != TRACKDIR_WORMHOLE) {
					SetTunnelMiddleReservation(last->tile, false);
					SetTunnelMiddleReservation(other_end, false);
				}
			}
		}
	}

	/* When any NewGRF has been changed the availability of some vehicles might
	 * have been changed too. e->company_avail must be set to 0 in that case
	 * which is done by StartupEngines(). */
	if (gcf_res != GLC_ALL_GOOD) StartupEngines();

	if (IsOTTDSavegameVersionBefore(stv, 166)) {
		/* Update cargo acceptance map of towns. */
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsHouseTile(t)) continue;
			Town::Get(GetTownIndex(t))->cargo_accepted.Add(t);
		}

		Town *town;
		FOR_ALL_TOWNS(town) {
			UpdateTownCargoes(town);
		}
	}

	/* The road owner of standard road stops was not properly accounted for. */
	if (IsOTTDSavegameVersionBefore(stv, 172)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsStandardRoadStopTile(t)) continue;
			Owner o = GetTileOwner(t);
			SetRoadOwner(t, ROADTYPE_ROAD, o);
			SetRoadOwner(t, ROADTYPE_TRAM, o);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 175)) {
		/* Introduced tree planting limit. */
		Company *c;
		FOR_ALL_COMPANIES(c) c->tree_limit = _settings_game.construction.tree_frame_burst << 16;
	}

	if (IsOTTDSavegameVersionBefore(stv, 177)) {
		/* Fix too high inflation rates */
		if (_economy.inflation_prices > MAX_INFLATION) _economy.inflation_prices = MAX_INFLATION;
		if (_economy.inflation_payment > MAX_INFLATION) _economy.inflation_payment = MAX_INFLATION;

		/* We have to convert the quarters of bankruptcy into months of bankruptcy */
		FOR_ALL_COMPANIES(c) {
			c->months_of_bankruptcy = 3 * c->months_of_bankruptcy;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 178)) {
		extern uint8 _old_diff_level;
		/* Initialise script settings profile */
		_settings_game.script.settings_profile = IsInsideMM(_old_diff_level, SP_BEGIN, SP_END) ? _old_diff_level : (uint)SP_MEDIUM;
	}

	if (IsOTTDSavegameVersionBefore(stv, 182)) {
		Aircraft *v;
		/* Aircraft acceleration variable was bonkers */
		FOR_ALL_AIRCRAFT(v) {
			if (v->subtype <= AIR_AIRCRAFT) {
				const AircraftVehicleInfo *avi = AircraftVehInfo(v->engine_type);
				v->acceleration = avi->acceleration;
			}
		}

		/* Blocked tiles could be reserved due to a bug, which causes
		 * other places to assert upon e.g. station reconstruction. */
		for (TileIndex t = 0; t < map_size; t++) {
			if (HasStationTileRail(t) && IsStationTileBlocked(t)) {
				SetRailStationReservation(t, false);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 184)) {
		/* The global units configuration is split up in multiple configurations. */
		extern uint8 _old_units;
		_settings_game.locale.units_velocity = Clamp(_old_units, 0, 2);
		_settings_game.locale.units_power    = Clamp(_old_units, 0, 2);
		_settings_game.locale.units_weight   = Clamp(_old_units, 1, 2);
		_settings_game.locale.units_volume   = Clamp(_old_units, 1, 2);
		_settings_game.locale.units_force    = 2;
		_settings_game.locale.units_height   = Clamp(_old_units, 0, 2);
	}

	/* Rearrange lift destination bits for houses. */
	if (IsFullSavegameVersionBefore(stv, 1)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsHouseTile(t) && GetHouseType(t) < NEW_HOUSE_OFFSET) {
				SB(_mc[t].m7, 0, 4, GB(_mc[t].m7, 1, 3) | (GB(_mc[t].m7, 0, 1) << 3));
			}
		}
	}

	/* Road stops is 'only' updating some caches */
	AfterLoadRoadStops();
	AfterLoadLabelMaps();
	AfterLoadCompanyStats();
	AfterLoadStoryBook(stv);

	GamelogPrintDebug(1);

	InitializeWindowsAndCaches();

	AfterLoadLinkGraphs();
}

/**
 * Reload all NewGRF files during a running game. This is a cut-down
 * version of AfterLoadGame().
 * XXX - We need to reset the vehicle position hash because with a non-empty
 * hash AfterLoadVehicles() will loop infinitely. We need AfterLoadVehicles()
 * to recalculate vehicle data as some NewGRF vehicle sets could have been
 * removed or added and changed statistics
 */
void ReloadNewGRFData()
{
	/* reload grf data */
	GfxLoadSprites();
	LoadStringWidthTable();
	RecomputePrices();
	/* reload vehicles */
	ResetVehicleHash();
	AfterLoadVehicles(NULL);
	StartupEngines();
	GroupStatistics::UpdateAfterLoad();
	/* update station graphics */
	AfterLoadStations();
	/* Update company statistics. */
	AfterLoadCompanyStats();
	/* Check and update house and town values */
	UpdateHousesAndTowns();
	/* Delete news referring to no longer existing entities */
	DeleteInvalidEngineNews();
	/* Update livery selection windows */
	for (CompanyID i = COMPANY_FIRST; i < MAX_COMPANIES; i++) InvalidateWindowData(WC_COMPANY_COLOUR, i);
	/* Update company infrastructure counts. */
	InvalidateWindowClassesData(WC_COMPANY_INFRASTRUCTURE);
	/* redraw the whole screen */
	MarkWholeScreenDirty();
	CheckTrainsLengths();
}
