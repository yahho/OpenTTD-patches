/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ship_cmd.cpp Handling of ships. */

#include "stdafx.h"
#include "ship.h"
#include "landscape.h"
#include "timetable.h"
#include "news_func.h"
#include "company_func.h"
#include "depot_base.h"
#include "station_base.h"
#include "newgrf_engine.h"
#include "pathfinder/yapf/yapf.h"
#include "newgrf_sound.h"
#include "spritecache.h"
#include "strings_func.h"
#include "window_func.h"
#include "date_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "ai/ai.hpp"
#include "game/game.hpp"
#include "pathfinder/opf/opf_ship.h"
#include "engine_base.h"
#include "company_base.h"
#include "zoom_func.h"
#include "map/bridge.h"

#include "table/strings.h"

/**
 * Determine the effective #WaterClass for a ship travelling on a tile.
 * @param tile Tile of interest
 * @return the waterclass to be used by the ship.
 */
WaterClass GetEffectiveWaterClass(TileIndex tile)
{
	if (HasTileWaterClass(tile)) return GetWaterClass(tile);
	if (IsAqueductTile(tile)) return WATER_CLASS_CANAL;
	if (IsNormalRailTile(tile)) {
		assert(GetRailGroundType(tile) == RAIL_GROUND_WATER);
		return WATER_CLASS_SEA;
	}
	NOT_REACHED();
}

static const uint16 _ship_sprites[] = {0x0E5D, 0x0E55, 0x0E65, 0x0E6D};

template <>
bool IsValidImageIndex<VEH_SHIP>(uint8 image_index)
{
	return image_index < lengthof(_ship_sprites);
}

static inline TrackdirBits GetAvailShipTrackdirs(TileIndex tile, DiagDirection enterdir)
{
	return GetTileWaterwayStatus(tile) & DiagdirReachesTrackdirs(enterdir);
}

static void GetShipIcon(EngineID engine, EngineImageType image_type, VehicleSpriteSeq *result)
{
	const Engine *e = Engine::Get(engine);
	uint8 spritenum = e->u.ship.image_index;

	if (is_custom_sprite(spritenum)) {
		GetCustomVehicleIcon(engine, DIR_W, image_type, result);
		if (result->IsValid()) return;

		spritenum = e->original_image_index;
	}

	assert(IsValidImageIndex<VEH_SHIP>(spritenum));
	result->Set(DIR_W + _ship_sprites[spritenum]);
}

void DrawShipEngine (BlitArea *dpi, int left, int right, int preferred_x,
	int y, EngineID engine, PaletteID pal, EngineImageType image_type)
{
	VehicleSpriteSeq seq;
	GetShipIcon(engine, image_type, &seq);

	Rect rect;
	seq.GetBounds(&rect);
	preferred_x = Clamp(preferred_x,
			left - UnScaleGUI(rect.left),
			right - UnScaleGUI(rect.right));

	seq.Draw (dpi, preferred_x, y, pal, pal == PALETTE_CRASH);
}

/**
 * Get the size of the sprite of a ship sprite heading west (used for lists).
 * @param engine The engine to get the sprite from.
 * @param[out] width The width of the sprite.
 * @param[out] height The height of the sprite.
 * @param[out] xoffs Number of pixels to shift the sprite to the right.
 * @param[out] yoffs Number of pixels to shift the sprite downwards.
 * @param image_type Context the sprite is used in.
 */
void GetShipSpriteSize(EngineID engine, uint &width, uint &height, int &xoffs, int &yoffs, EngineImageType image_type)
{
	VehicleSpriteSeq seq;
	GetShipIcon(engine, image_type, &seq);

	Rect rect;
	seq.GetBounds(&rect);

	width  = UnScaleGUI(rect.right - rect.left + 1);
	height = UnScaleGUI(rect.bottom - rect.top + 1);
	xoffs  = UnScaleGUI(rect.left);
	yoffs  = UnScaleGUI(rect.top);
}

void Ship::GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const
{
	uint8 spritenum = this->spritenum;

	if (is_custom_sprite(spritenum)) {
		GetCustomVehicleSprite(this, direction, image_type, result);
		if (result->IsValid()) return;

		spritenum = this->GetEngine()->original_image_index;
	}

	assert(IsValidImageIndex<VEH_SHIP>(spritenum));
	result->Set(_ship_sprites[spritenum] + direction);
}

static const Depot *FindClosestShipDepot (const Ship *v, bool nearby = false)
{
	if (_settings_game.pf.pathfinder_for_ships != VPF_OPF) {
		assert (_settings_game.pf.pathfinder_for_ships == VPF_YAPF);

		TileIndex depot = YapfShipFindNearestDepot (v, nearby ? _settings_game.pf.yapf.maximum_go_to_depot_penalty : 0);
		return depot == INVALID_TILE ? NULL : Depot::GetByTile(depot);
	}

	/* The original pathfinder cannot look for the nearest depot. */
	const Depot *depot;
	const Depot *best_depot = NULL;

	/* If we don't have a maximum distance, we want to find any depot
	 * so the best distance of no depot must be more than any correct
	 * distance. On the other hand if we have set a maximum distance,
	 * any depot further away than the maximum allowed distance can
	 * safely be ignored. */
	uint best_dist = nearby ? (12 + 1) : UINT_MAX;
	FOR_ALL_DEPOTS(depot) {
		TileIndex tile = depot->xy;
		if (IsShipDepotTile(tile) && IsTileOwner(tile, v->owner)) {
			uint dist = DistanceManhattan(tile, v->tile);
			if (dist < best_dist) {
				best_dist = dist;
				best_depot = depot;
			}
		}
	}

	return best_depot;
}

static void CheckIfShipNeedsService (Ship *v)
{
	if (Company::Get(v->owner)->settings.vehicle.servint_ships == 0 || !v->NeedsAutomaticServicing()) return;
	if (v->IsChainInDepot()) {
		VehicleServiceInDepot(v);
		return;
	}

	const Depot *depot = FindClosestShipDepot (v, true);

	if (depot == NULL) {
		if (v->current_order.IsType(OT_GOTO_DEPOT)) {
			v->current_order.MakeDummy();
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		}
		return;
	}

	v->current_order.MakeGoToDepot(depot->index, ODTFB_SERVICE);
	v->dest_tile = depot->xy;
	SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
}

/**
 * Update the caches of this ship.
 */
void Ship::UpdateCache()
{
	const ShipVehicleInfo *svi = ShipVehInfo(this->engine_type);

	/* Get speed fraction for the current water type. Aqueducts are always canals. */
	bool is_ocean = GetEffectiveWaterClass(this->tile) == WATER_CLASS_SEA;
	uint raw_speed = GetVehicleProperty(this, PROP_SHIP_SPEED, svi->max_speed);
	this->vcache.cached_max_speed = svi->ApplyWaterClassSpeedFrac(raw_speed, is_ocean);

	/* Update cargo aging period. */
	this->vcache.cached_cargo_age_period = GetVehicleProperty(this, PROP_SHIP_CARGO_AGE_PERIOD, EngInfo(this->engine_type)->cargo_age_period);

	this->UpdateVisualEffect();
}

Money Ship::GetRunningCost() const
{
	const Engine *e = this->GetEngine();
	uint cost_factor = GetVehicleProperty(this, PROP_SHIP_RUNNING_COST_FACTOR, e->u.ship.running_cost);
	return GetPrice(PR_RUNNING_SHIP, cost_factor, e->GetGRF());
}

void Ship::OnNewDay()
{
	if ((++this->day_counter & 7) == 0) {
		DecreaseVehicleValue(this);
	}

	CheckVehicleBreakdown(this);
	AgeVehicle(this);
	CheckIfShipNeedsService(this);

	CheckOrders(this);

	if (this->running_ticks == 0) return;

	CommandCost cost(EXPENSES_SHIP_RUN, this->GetRunningCost() * this->running_ticks / (DAYS_IN_YEAR * DAY_TICKS));

	this->profit_this_year -= cost.GetCost();
	this->running_ticks = 0;

	SubtractMoneyFromCompanyFract(this->owner, cost);

	SetWindowDirty(WC_VEHICLE_DETAILS, this->index);
	/* we need this for the profit */
	SetWindowClassesDirty(WC_SHIPS_LIST);
}

ShipPathPos Ship::GetPos() const
{
	if (this->vehstatus & VS_CRASHED) return ShipPathPos();

	Trackdir td;

	if (this->IsInDepot()) {
		/* We'll assume the ship is facing outwards */
		td = DiagDirToDiagTrackdir(GetShipDepotDirection(this->tile));
	} else if (this->trackdir == TRACKDIR_WORMHOLE) {
		/* ship on aqueduct, so just use his direction and assume a diagonal track */
		td = DiagDirToDiagTrackdir(DirToDiagDir(this->direction));
	} else {
		td = this->trackdir;
	}

	return ShipPathPos(this->tile, td);
}

void Ship::MarkDirty()
{
	this->colourmap = PAL_NONE;
	this->UpdateViewport(true, false);
	this->UpdateCache();
}

static void PlayShipSound(const Vehicle *v)
{
	if (!PlayVehicleSound(v, VSE_START)) {
		SndPlayVehicleFx(ShipVehInfo(v->engine_type)->sfx, v);
	}
}

void Ship::PlayLeaveStationSound() const
{
	PlayShipSound(this);
}

TileIndex Ship::GetOrderStationLocation(StationID station)
{
	if (station == this->last_station_visited) this->last_station_visited = INVALID_STATION;

	const Station *st = Station::Get(station);
	if (!(st->facilities & FACIL_DOCK)) {
		this->IncrementRealOrderIndex();
		return 0;
	}

	return st->xy;
}

void Ship::UpdateDeltaXY(Direction direction)
{
	static const int8 _delta_xy_table[8][4] = {
		/* y_extent, x_extent, y_offs, x_offs */
		{ 6,  6,  -3,  -3}, // N
		{ 6, 32,  -3, -16}, // NE
		{ 6,  6,  -3,  -3}, // E
		{32,  6, -16,  -3}, // SE
		{ 6,  6,  -3,  -3}, // S
		{ 6, 32,  -3, -16}, // SW
		{ 6,  6,  -3,  -3}, // W
		{32,  6, -16,  -3}, // NW
	};

	const int8 *bb = _delta_xy_table[direction];
	this->x_offs        = bb[3];
	this->y_offs        = bb[2];
	this->x_extent      = bb[1];
	this->y_extent      = bb[0];
	this->z_extent      = 6;
}

/**
 * Ship entirely entered the depot, update its status, orders, vehicle windows, service it, etc.
 * @param v Ship that entered a depot.
 */
static void ShipEnterDepot (Ship *v)
{
	SetWindowClassesDirty (WC_SHIPS_LIST);

	v->trackdir = TRACKDIR_DEPOT;
	v->UpdateCache();
	v->UpdateViewport (true, true);
	SetWindowDirty (WC_VEHICLE_DEPOT, v->tile);

	VehicleEnterDepot (v);
}

static bool CheckShipLeaveDepot(Ship *v)
{
	if (!v->IsChainInDepot()) return false;

	/* We are leaving a depot, but have to go to the exact same one; re-enter */
	if (v->current_order.IsType(OT_GOTO_DEPOT) &&
			IsShipDepotTile(v->tile) && GetDepotIndex(v->tile) == v->current_order.GetDestination()) {
		ShipEnterDepot (v);
		return true;
	}

	TileIndex tile = v->tile;

	DiagDirection north_dir = GetShipDepotDirection(tile);
	TileIndex north_neighbour = TILE_ADD(tile, TileOffsByDiagDir(north_dir));
	DiagDirection south_dir = ReverseDiagDir(north_dir);
	TileIndex south_neighbour = TILE_ADD(tile, 2 * TileOffsByDiagDir(south_dir));

	TrackdirBits north_trackdirs = GetAvailShipTrackdirs(north_neighbour, north_dir);
	TrackdirBits south_trackdirs = GetAvailShipTrackdirs(south_neighbour, south_dir);
	if (north_trackdirs && south_trackdirs) {
		/* Ask pathfinder for best direction */
		bool reverse = false;
		bool path_found;
		switch (_settings_game.pf.pathfinder_for_ships) {
			case VPF_OPF: reverse = OPFShipChooseTrack(v, north_neighbour, north_dir, north_trackdirs, path_found) == INVALID_TRACKDIR; break; // OPF always allows reversing
			case VPF_YAPF: reverse = YapfShipCheckReverse(v); break;
			default: NOT_REACHED();
		}
		if (reverse) north_trackdirs = TRACKDIR_BIT_NONE;
	}

	if (north_trackdirs) {
		/* Leave towards north */
		v->direction = DiagDirToDir(north_dir);
		v->trackdir = DiagDirToDiagTrackdir(north_dir);
	} else if (south_trackdirs) {
		/* Leave towards south */
		v->direction = DiagDirToDir(south_dir);
		v->trackdir = DiagDirToDiagTrackdir(south_dir);
	} else {
		/* Both ways blocked */
		return false;
	}

	v->vehstatus &= ~VS_HIDDEN;

	v->cur_speed = 0;
	v->UpdateViewport(true, true);
	SetWindowDirty(WC_VEHICLE_DEPOT, v->tile);

	PlayShipSound(v);
	VehicleServiceInDepot(v);
	InvalidateWindowData(WC_VEHICLE_DEPOT, v->tile);
	SetWindowClassesDirty(WC_SHIPS_LIST);

	return false;
}

static bool ShipAccelerate(Vehicle *v)
{
	uint spd;
	byte t;

	spd = min(v->cur_speed + 1, v->vcache.cached_max_speed);
	spd = min(spd, v->current_order.GetMaxSpeed() * 2);

	/* updates statusbar only if speed have changed to save CPU time */
	if (spd != v->cur_speed) {
		v->cur_speed = spd;
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}

	/* Convert direction-independent speed into direction-dependent speed. (old movement method) */
	spd = v->GetOldAdvanceSpeed(spd);

	if (spd == 0) return false;
	if ((byte)++spd == 0) return true;

	v->progress = (t = v->progress) - (byte)spd;

	return (t < v->progress);
}

/**
 * Ship arrives at a dock. If it is the first time, send out a news item.
 * @param v  Ship that arrived.
 * @param st Station being visited.
 */
static void ShipArrivesAt(const Vehicle *v, Station *st)
{
	/* Check if station was ever visited before */
	if (!(st->had_vehicle_of_type & HVOT_SHIP)) {
		st->had_vehicle_of_type |= HVOT_SHIP;

		AddNewsItem<ArrivalNewsItem> (STR_NEWS_FIRST_SHIP_ARRIVAL, v, st);
		AI::NewEvent(v->owner, new ScriptEventStationFirstVehicle(st->index, v->index));
		Game::NewEvent(new ScriptEventStationFirstVehicle(st->index, v->index));
	}
}


/**
 * Runs the pathfinder to choose a trackdir to continue along.
 *
 * @param v Ship to navigate
 * @param tile Tile the ship is about to enter
 * @param enterdir Direction of entering
 * @param trackdirs Available trackdir choices on \a tile
 * @return Trackdir to choose, or INVALID_TRACKDIR when to reverse.
 */
static Trackdir ChooseShipTrack(Ship *v, TileIndex tile, DiagDirection enterdir, TrackdirBits trackdirs)
{
	assert(IsValidDiagDirection(enterdir));

	bool path_found = true;
	Trackdir trackdir;
	switch (_settings_game.pf.pathfinder_for_ships) {
		case VPF_OPF: trackdir = OPFShipChooseTrack(v, tile, enterdir, trackdirs, path_found); break;
		case VPF_YAPF: trackdir = YapfShipChooseTrack(v, tile, enterdir, trackdirs, path_found); break;
		default: NOT_REACHED();
	}

	v->HandlePathfindingResult(path_found);
	return trackdir;
}

static void ShipController(Ship *v)
{
	v->tick_counter++;
	v->current_order_time++;

	if (v->HandleBreakdown()) return;

	if (v->vehstatus & VS_STOPPED) return;

	ProcessOrders(v);
	v->HandleLoading();

	if (v->current_order.IsType(OT_LOADING)) return;

	if (CheckShipLeaveDepot(v)) return;

	v->ShowVisualEffect();

	if (!ShipAccelerate(v)) return;

	FullPosTile gp = GetNewVehiclePos(v);
	if (v->trackdir == TRACKDIR_WORMHOLE) {
		/* On a bridge */
		if (gp.tile != v->tile) {
			/* Still on the bridge */
			v->x_pos = gp.xx;
			v->y_pos = gp.yy;
			v->UpdatePositionAndViewport();
			return;
		} else {
			v->trackdir = DiagDirToDiagTrackdir(ReverseDiagDir(GetTunnelBridgeDirection(v->tile)));
		}
	} else if (v->trackdir == TRACKDIR_DEPOT) {
		/* Inside depot */
		assert(gp.tile == v->tile);
		gp.set (v->x_pos, v->y_pos, gp.tile);
	} else if (gp.tile == v->tile) {
		/* Not on a bridge or in a depot, staying in the old tile */

		/* A leave station order only needs one tick to get processed, so we can
		 * always skip ahead. */
		if (v->current_order.IsType(OT_LEAVESTATION)) {
			v->current_order.Clear();
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		} else if (v->dest_tile != 0) {
			/* We have a target, let's see if we reached it... */
			if (v->current_order.IsType(OT_GOTO_WAYPOINT) &&
					DistanceManhattan(v->dest_tile, gp.tile) <= 3) {
				/* We got within 3 tiles of our target buoy, so let's skip to our
				 * next order */
				UpdateVehicleTimetable(v, true);
				v->IncrementRealOrderIndex();
				v->current_order.MakeDummy();
			} else if (v->current_order.IsType(OT_GOTO_DEPOT)) {
				if (v->dest_tile == gp.tile && (gp.xx & 0xF) == 8 && (gp.yy & 0xF) == 8) {
					ShipEnterDepot (v);
					return;
				}
			} else if (v->current_order.IsType(OT_GOTO_STATION)) {
				StationID sid = v->current_order.GetDestination();
				Station *st = Station::Get(sid);
				if (st->IsDockingTile(gp.tile)) {
					assert(st->facilities & FACIL_DOCK);
					v->last_station_visited = sid;
					/* Process station in the orderlist. */
					ShipArrivesAt(v, st);
					v->BeginLoading();
				}
			}
		}
	} else {
		/* Not on a bridge or in a depot, about to enter a new tile */

		if (!IsValidTile(gp.tile)) goto reverse_direction;

		DiagDirection diagdir = DiagdirBetweenTiles(v->tile, gp.tile);
		assert(diagdir != INVALID_DIAGDIR);

		if (IsAqueductTile(v->tile) && GetTunnelBridgeDirection(v->tile) == diagdir) {
			TileIndex end_tile = GetOtherBridgeEnd(v->tile);
			if (end_tile != gp.tile) {
				/* Entering an aqueduct */
				v->tile = end_tile;
				v->trackdir = TRACKDIR_WORMHOLE;
				v->x_pos = gp.xx;
				v->y_pos = gp.yy;
				v->UpdatePositionAndViewport();
				return;
			}
		}

		TrackdirBits trackdirs = GetAvailShipTrackdirs(gp.tile, diagdir);
		if (trackdirs == TRACKDIR_BIT_NONE) goto reverse_direction;

		/* Choose a direction, and continue if we find one */
		Trackdir trackdir = ChooseShipTrack(v, gp.tile, diagdir, trackdirs);
		if (trackdir == INVALID_TRACKDIR) goto reverse_direction;

		const InitialSubcoords *b = get_initial_subcoords (trackdir);
		gp.adjust_subcoords (b);

		WaterClass old_wc = GetEffectiveWaterClass(v->tile);

		v->tile = gp.tile;
		v->trackdir = trackdir;

		/* Update ship cache when the water class changes. Aqueducts are always canals. */
		WaterClass new_wc = GetEffectiveWaterClass(gp.tile);
		if (old_wc != new_wc) v->UpdateCache();

		v->direction = b->dir;
	}

	/* update image of ship, as well as delta XY */
	v->x_pos = gp.xx;
	v->y_pos = gp.yy;
	v->z_pos = GetSlopePixelZ(gp.xx, gp.yy);

getout:
	v->UpdatePosition();
	v->UpdateViewport(true, true);
	return;

reverse_direction:
	v->direction = ReverseDir(v->direction);
	v->trackdir = ReverseTrackdir(v->trackdir);
	goto getout;
}

bool Ship::Tick()
{
	if (!(this->vehstatus & VS_STOPPED)) this->running_ticks++;

	ShipController(this);

	return true;
}

/**
 * Build a ship.
 * @param tile     tile of the depot where ship is built.
 * @param flags    type of operation.
 * @param e        the engine to build.
 * @param data     unused.
 * @param ret[out] the vehicle that has been built.
 * @return the cost of this operation or an error.
 */
CommandCost CmdBuildShip(TileIndex tile, DoCommandFlag flags, const Engine *e, uint16 data, Vehicle **ret)
{
	tile = GetShipDepotNorthTile(tile);
	if (flags & DC_EXEC) {
		int x;
		int y;

		const ShipVehicleInfo *svi = &e->u.ship;

		Ship *v = new Ship();
		*ret = v;

		v->owner = _current_company;
		v->tile = tile;
		x = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
		y = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
		v->x_pos = x;
		v->y_pos = y;
		v->z_pos = GetSlopePixelZ(x, y);

		v->UpdateDeltaXY(v->direction);
		v->vehstatus = VS_HIDDEN | VS_STOPPED | VS_DEFPAL;

		v->spritenum = svi->image_index;
		v->cargo_type = e->GetDefaultCargoType();
		v->cargo_cap = svi->capacity;
		v->refit_cap = 0;

		v->last_station_visited = INVALID_STATION;
		v->last_loading_station = INVALID_STATION;
		v->engine_type = e->index;

		v->reliability = e->reliability;
		v->reliability_spd_dec = e->reliability_spd_dec;
		v->max_age = e->GetLifeLengthInDays();
		_new_vehicle_id = v->index;

		v->trackdir = TRACKDIR_DEPOT;

		v->SetServiceInterval(Company::Get(_current_company)->settings.vehicle.servint_ships);
		v->date_of_last_service = _date;
		v->build_year = _cur_year;
		v->sprite_seq.Set(SPR_IMG_QUERY);
		v->random_bits = VehicleRandomBits();

		v->UpdateCache();

		if (e->flags & ENGINE_EXCLUSIVE_PREVIEW) SetBit(v->vehicle_flags, VF_BUILT_AS_PROTOTYPE);
		v->SetServiceIntervalIsPercent(Company::Get(_current_company)->settings.vehicle.servint_ispercent);

		v->InvalidateNewGRFCacheOfChain();

		v->cargo_cap = e->DetermineCapacity(v);

		v->InvalidateNewGRFCacheOfChain();

		v->UpdatePosition();
	}

	return CommandCost();
}

bool Ship::FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse)
{
	const Depot *depot = FindClosestShipDepot(this);

	if (depot == NULL) return false;

	if (location    != NULL) *location    = depot->xy;
	if (destination != NULL) *destination = depot->index;

	return true;
}
