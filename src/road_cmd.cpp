/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road_cmd.cpp Commands related to road tiles. */

#include "stdafx.h"
#include "cmd_helper.h"
#include "road_internal.h"
#include "viewport_func.h"
#include "command_func.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "depot_base.h"
#include "newgrf.h"
#include "autoslope.h"
#include "tunnelbridge_map.h"
#include "strings_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "tunnelbridge.h"
#include "cheat_type.h"
#include "effectvehicle_func.h"
#include "effectvehicle_base.h"
#include "elrail_func.h"
#include "roadveh.h"
#include "town.h"
#include "company_base.h"
#include "core/random_func.hpp"
#include "newgrf_railtype.h"
#include "date_func.h"
#include "genworld.h"
#include "company_gui.h"

#include "table/strings.h"
#include "table/road_land.h"


/**
 * Verify whether a road vehicle is available.
 * @return \c true if at least one road vehicle is available, \c false if not
 */
bool RoadVehiclesAreBuilt()
{
	const RoadVehicle *rv;
	FOR_ALL_ROADVEHICLES(rv) return true;

	return false;
}

/** Invalid RoadBits on a leveled slope. */
static const RoadBits _invalid_leveled_roadbits[15] = {
	ROAD_NONE,         // SLOPE_FLAT
	ROAD_NE | ROAD_SE, // SLOPE_W
	ROAD_NE | ROAD_NW, // SLOPE_S
	ROAD_NE,           // SLOPE_SW
	ROAD_NW | ROAD_SW, // SLOPE_E
	ROAD_NONE,         // SLOPE_EW
	ROAD_NW,           // SLOPE_SE
	ROAD_NONE,         // SLOPE_WSE
	ROAD_SE | ROAD_SW, // SLOPE_N
	ROAD_SE,           // SLOPE_NW
	ROAD_NONE,         // SLOPE_NS
	ROAD_NONE,         // SLOPE_ENW
	ROAD_SW,           // SLOPE_NE
	ROAD_NONE,         // SLOPE_SEN
	ROAD_NONE,         // SLOPE_NWS
};

/** Invalid straight RoadBits on a slope (with and without foundation). */
static const RoadBits _invalid_straight_roadbits[15] = {
	ROAD_NONE, // SLOPE_FLAT
	ROAD_NONE, // SLOPE_W    Foundation
	ROAD_NONE, // SLOPE_S    Foundation
	ROAD_Y,    // SLOPE_SW
	ROAD_NONE, // SLOPE_E    Foundation
	ROAD_ALL,  // SLOPE_EW
	ROAD_X,    // SLOPE_SE
	ROAD_ALL,  // SLOPE_WSE
	ROAD_NONE, // SLOPE_N    Foundation
	ROAD_X,    // SLOPE_NW
	ROAD_ALL,  // SLOPE_NS
	ROAD_ALL,  // SLOPE_ENW
	ROAD_Y,    // SLOPE_NE
	ROAD_ALL,  // SLOPE_SEN
	ROAD_ALL,  // SLOPE_NWS
};

static Foundation GetRoadFoundation(Slope tileh, RoadBits bits);

/**
 * Is it allowed to remove the given road bits from the given tile?
 * @param tile      the tile to remove the road from
 * @param remove    the roadbits that are going to be removed
 * @param owner     the actual owner of the roadbits of the tile
 * @param rt        the road type to remove the bits from
 * @param flags     command flags
 * @param town_check Shall the town rating checked/affected
 * @return A succeeded command when it is allowed to remove the road bits, a failed command otherwise.
 */
CommandCost CheckAllowRemoveRoad(TileIndex tile, RoadBits remove, Owner owner, RoadType rt, DoCommandFlag flags, bool town_check)
{
	if (_game_mode == GM_EDITOR || remove == ROAD_NONE) return CommandCost();

	/* Water can always flood and towns can always remove "normal" road pieces.
	 * Towns are not be allowed to remove non "normal" road pieces, like tram
	 * tracks as that would result in trams that cannot turn. */
	if (_current_company == OWNER_WATER ||
			(rt == ROADTYPE_ROAD && !Company::IsValidID(_current_company))) return CommandCost();

	/* Only do the special processing if the road is owned
	 * by a town */
	if (owner != OWNER_TOWN) {
		if (owner == OWNER_NONE) return CommandCost();
		CommandCost ret = CheckOwnership(owner);
		return ret;
	}

	if (!town_check) return CommandCost();

	if (_cheats.magic_bulldozer.value) return CommandCost();

	Town *t = ClosestTownFromTile(tile, UINT_MAX);
	if (t == NULL) return CommandCost();

	/* check if you're allowed to remove the street owned by a town
	 * removal allowance depends on difficulty setting */
	CommandCost ret = CheckforTownRating(flags, t, ROAD_REMOVE);
	if (ret.Failed()) return ret;

	/* Get a bitmask of which neighbouring roads has a tile */
	RoadBits n = ROAD_NONE;
	RoadBits present = GetAnyRoadBits(tile, rt);
	if ((present & ROAD_NE) && (GetAnyRoadBits(TILE_ADDXY(tile, -1,  0), rt) & ROAD_SW)) n |= ROAD_NE;
	if ((present & ROAD_SE) && (GetAnyRoadBits(TILE_ADDXY(tile,  0,  1), rt) & ROAD_NW)) n |= ROAD_SE;
	if ((present & ROAD_SW) && (GetAnyRoadBits(TILE_ADDXY(tile,  1,  0), rt) & ROAD_NE)) n |= ROAD_SW;
	if ((present & ROAD_NW) && (GetAnyRoadBits(TILE_ADDXY(tile,  0, -1), rt) & ROAD_SE)) n |= ROAD_NW;

	int rating_decrease = RATING_ROAD_DOWN_STEP_EDGE;
	/* If 0 or 1 bits are set in n, or if no bits that match the bits to remove,
	 * then allow it */
	if (KillFirstBit(n) != ROAD_NONE && (n & remove) != ROAD_NONE) {
		/* you can remove all kind of roads with extra dynamite */
		if (!_settings_game.construction.extra_dynamite) {
			SetDParam(0, t->index);
			return_cmd_error(STR_ERROR_LOCAL_AUTHORITY_REFUSES_TO_ALLOW_THIS);
		}
		rating_decrease = RATING_ROAD_DOWN_STEP_INNER;
	}
	ChangeTownRating(t, rating_decrease, RATING_ROAD_MINIMUM, flags);

	return CommandCost();
}


/**
 * Delete a piece of road from a normal road tile
 * @param tile tile where to remove road from
 * @param flags operation to perform
 * @param pieces roadbits to remove
 * @param rt roadtype to remove
 * @param town_check should we check if the town allows removal?
 */
static CommandCost RemoveRoad_Road(TileIndex tile, DoCommandFlag flags, RoadBits pieces, RoadType rt, bool town_check)
{
	CommandCost ret = EnsureNoVehicleOnGround(tile);
	if (ret.Failed()) return ret;

	ret = CheckAllowRemoveRoad(tile, pieces, GetRoadOwner(tile, rt), rt, flags, town_check);
	if (ret.Failed()) return ret;

	if (HasRoadWorks(tile) && _current_company != OWNER_WATER) return_cmd_error(STR_ERROR_ROAD_WORKS_IN_PROGRESS);

	Slope tileh = GetTileSlope(tile);

	/* Steep slopes behave the same as slopes with one corner raised. */
	if (IsSteepSlope(tileh)) {
		tileh = SlopeWithOneCornerRaised(GetHighestSlopeCorner(tileh));
	}

	RoadBits present = GetRoadBits(tile, rt);
	const RoadBits other = GetOtherRoadBits(tile, rt);
	const Foundation f = GetRoadFoundation(tileh, present);

	/* Autocomplete to a straight road
	 * @li if the bits of the other roadtypes result in another foundation
	 * @li if build on slopes is disabled */
	if ((IsStraightRoad(other) && (other & _invalid_leveled_roadbits[tileh & SLOPE_ELEVATED]) != ROAD_NONE) ||
			(tileh != SLOPE_FLAT && !_settings_game.construction.build_on_slopes)) {
		pieces |= MirrorRoadBits(pieces);
	}

	/* limit the bits to delete to the existing bits. */
	pieces &= present;
	if (pieces == ROAD_NONE) return_cmd_error(rt == ROADTYPE_TRAM ? STR_ERROR_THERE_IS_NO_TRAMWAY : STR_ERROR_THERE_IS_NO_ROAD);

	/* Now set present what it will be after the remove */
	present ^= pieces;

	/* Check for invalid RoadBit combinations on slopes */
	if (tileh != SLOPE_FLAT && present != ROAD_NONE &&
			(present & _invalid_leveled_roadbits[tileh & SLOPE_ELEVATED]) == present) {
		return CMD_ERROR;
	}

	if (flags & DC_EXEC) {
		if (HasRoadWorks(tile)) {
			/* flooding tile with road works, don't forget to remove the effect vehicle too */
			assert(_current_company == OWNER_WATER);
			EffectVehicle *v;
			FOR_ALL_EFFECTVEHICLES(v) {
				if (TileVirtXY(v->x_pos, v->y_pos) == tile) {
					delete v;
				}
			}
		}

		Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
		if (c != NULL) {
			c->infrastructure.road[rt] -= CountBits(pieces);
			DirtyCompanyInfrastructureWindows(c->index);
		}

		if (present == ROAD_NONE) {
			RoadTypes rts = GetRoadTypes(tile) & ComplementRoadTypes(RoadTypeToRoadTypes(rt));
			if (rts == ROADTYPES_NONE) {
				/* Includes MarkTileDirtyByTile() */
				DoClearSquare(tile);
			} else {
				if (rt == ROADTYPE_ROAD && IsRoadOwner(tile, ROADTYPE_ROAD, OWNER_TOWN)) {
					/* Update nearest-town index */
					SetTownIndex(tile, CalcClosestTownIDFromTile(tile));
				}
				SetRoadBits(tile, ROAD_NONE, rt);
				SetRoadTypes(tile, rts);
				MarkTileDirtyByTile(tile);
			}
		} else {
			/* When bits are removed, you *always* end up with something that
			 * is not a complete straight road tile. However, trams do not have
			 * onewayness, so they cannot remove it either. */
			if (rt != ROADTYPE_TRAM) SetDisallowedRoadDirections(tile, DRD_NONE);
			SetRoadBits(tile, present, rt);
			MarkTileDirtyByTile(tile);
		}
	}

	CommandCost cost(EXPENSES_CONSTRUCTION, CountBits(pieces) * _price[PR_CLEAR_ROAD]);
	/* If we build a foundation we have to pay for it. */
	if (f == FOUNDATION_NONE && GetRoadFoundation(tileh, present) != FOUNDATION_NONE) cost.AddCost(_price[PR_BUILD_FOUNDATION]);
	return cost;
}

/**
 * Delete a piece of road from a bridge
 * @param tile tile where to remove road from
 * @param flags operation to perform
 * @param pieces roadbits to remove
 * @param rt roadtype to remove
 * @param town_check should we check if the town allows removal?
 */
static CommandCost RemoveRoad_Bridge(TileIndex tile, DoCommandFlag flags, RoadBits pieces, RoadType rt, bool town_check)
{
	TileIndex other_end = GetOtherBridgeEnd(tile);
	CommandCost ret = TunnelBridgeIsFree(tile, other_end);
	if (ret.Failed()) return ret;

	ret = CheckAllowRemoveRoad(tile, pieces, GetRoadOwner(tile, rt), rt, flags, town_check);
	if (ret.Failed()) return ret;

	/* If it's the last roadtype, just clear the whole tile */
	if (GetRoadTypes(tile) == RoadTypeToRoadTypes(rt)) return DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);

	/* Removing any roadbit in the bridge axis removes the roadtype (that's the behaviour remove-long-roads needs) */
	if ((AxisToRoadBits(DiagDirToAxis(GetTunnelBridgeDirection(tile))) & pieces) == ROAD_NONE) return_cmd_error(rt == ROADTYPE_TRAM ? STR_ERROR_THERE_IS_NO_TRAMWAY : STR_ERROR_THERE_IS_NO_ROAD);

	CommandCost cost(EXPENSES_CONSTRUCTION);
	/* Pay for *every* tile of the bridge */
	uint len = GetTunnelBridgeLength(other_end, tile) + 2;
	cost.AddCost(len * _price[PR_CLEAR_ROAD]);

	if (flags & DC_EXEC) {
		Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
		if (c != NULL) {
			/* A full diagonal road tile has two road bits. */
			c->infrastructure.road[rt] -= len * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
			DirtyCompanyInfrastructureWindows(c->index);
		}

		SetRoadTypes(other_end, GetRoadTypes(other_end) & ~RoadTypeToRoadTypes(rt));
		SetRoadTypes(tile, GetRoadTypes(tile) & ~RoadTypeToRoadTypes(rt));

		/* If the owner of the bridge sells all its road, also move the ownership
		 * to the owner of the other roadtype. */
		RoadType other_rt = (rt == ROADTYPE_ROAD) ? ROADTYPE_TRAM : ROADTYPE_ROAD;
		Owner other_owner = GetRoadOwner(tile, other_rt);
		if (other_owner != GetTileOwner(tile)) {
			SetTileOwner(tile, other_owner);
			SetTileOwner(other_end, other_owner);
		}

		/* Mark tiles dirty that have been repaved */
		MarkBridgeTilesDirty(tile, other_end, GetTunnelBridgeDirection(tile));
	}

	return cost;
}

/**
 * Delete a piece of road from a crossing
 * @param tile tile where to remove road from
 * @param flags operation to perform
 * @param pieces roadbits to remove
 * @param rt roadtype to remove
 * @param crossing_check should we check if there is a tram track when we are removing road from crossing?
 * @param town_check should we check if the town allows removal?
 */
static CommandCost RemoveRoad_Crossing(TileIndex tile, DoCommandFlag flags, RoadBits pieces, RoadType rt, bool crossing_check, bool town_check)
{
	CommandCost ret = EnsureNoVehicleOnGround(tile);
	if (ret.Failed()) return ret;

	ret = CheckAllowRemoveRoad(tile, pieces, GetRoadOwner(tile, rt), rt, flags, town_check);
	if (ret.Failed()) return ret;

	if (pieces & ComplementRoadBits(GetCrossingRoadBits(tile))) {
		return CMD_ERROR;
	}

	/* Don't allow road to be removed from the crossing when there is tram;
	 * we can't draw the crossing without roadbits ;) */
	if (rt == ROADTYPE_ROAD && HasTileRoadType(tile, ROADTYPE_TRAM) && (flags & DC_EXEC || crossing_check)) return CMD_ERROR;

	if (flags & DC_EXEC) {
		Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
		if (c != NULL) {
			/* A full diagonal road tile has two road bits. */
			c->infrastructure.road[rt] -= 2;
			DirtyCompanyInfrastructureWindows(c->index);
		}

		Track railtrack = GetCrossingRailTrack(tile);
		RoadTypes rts = GetRoadTypes(tile) & ComplementRoadTypes(RoadTypeToRoadTypes(rt));
		if (rts == ROADTYPES_NONE) {
			TrackBits tracks = GetCrossingRailBits(tile);
			bool reserved = HasCrossingReservation(tile);
			MakeRailNormal(tile, GetTileOwner(tile), tracks, GetRailType(tile));
			if (reserved) SetTrackReservation(tile, tracks);

			/* Update rail count for level crossings. The plain track should still be accounted
			 * for, so only subtract the difference to the level crossing cost. */
			c = Company::GetIfValid(GetTileOwner(tile));
			if (c != NULL) c->infrastructure.rail[GetRailType(tile)] -= LEVELCROSSING_TRACKBIT_FACTOR - 1;
		} else {
			SetRoadTypes(tile, rts);
		}
		MarkTileDirtyByTile(tile);
		YapfNotifyTrackLayoutChange(tile, railtrack);
	}

	return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_CLEAR_ROAD] * 2);
}

/**
 * Delete a piece of road from a tunnel
 * @param tile tile where to remove road from
 * @param flags operation to perform
 * @param pieces roadbits to remove
 * @param rt roadtype to remove
 * @param town_check should we check if the town allows removal?
 */
static CommandCost RemoveRoad_Tunnel(TileIndex tile, DoCommandFlag flags, RoadBits pieces, RoadType rt, bool town_check)
{
	if (GetTunnelTransportType(tile) != TRANSPORT_ROAD) return CMD_ERROR;

	TileIndex other_end = GetOtherTunnelBridgeEnd(tile);
	CommandCost ret = TunnelBridgeIsFree(tile, other_end);
	if (ret.Failed()) return ret;

	ret = CheckAllowRemoveRoad(tile, pieces, GetRoadOwner(tile, rt), rt, flags, town_check);
	if (ret.Failed()) return ret;

	/* If it's the last roadtype, just clear the whole tile */
	if (GetRoadTypes(tile) == RoadTypeToRoadTypes(rt)) return DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);

	/* Removing any roadbit in the tunnel axis removes the roadtype (that's the behaviour remove-long-roads needs) */
	if ((AxisToRoadBits(DiagDirToAxis(GetTunnelBridgeDirection(tile))) & pieces) == ROAD_NONE) return_cmd_error(rt == ROADTYPE_TRAM ? STR_ERROR_THERE_IS_NO_TRAMWAY : STR_ERROR_THERE_IS_NO_ROAD);

	CommandCost cost(EXPENSES_CONSTRUCTION);
	/* Pay for *every* tile of the tunnel */
	uint len = GetTunnelBridgeLength(other_end, tile) + 2;
	cost.AddCost(len * _price[PR_CLEAR_ROAD]);

	if (flags & DC_EXEC) {
		Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
		if (c != NULL) {
			/* A full diagonal road tile has two road bits. */
			c->infrastructure.road[rt] -= len * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
			DirtyCompanyInfrastructureWindows(c->index);
		}

		SetRoadTypes(other_end, GetRoadTypes(other_end) & ~RoadTypeToRoadTypes(rt));
		SetRoadTypes(tile, GetRoadTypes(tile) & ~RoadTypeToRoadTypes(rt));

		/* If the owner of the bridge sells all its road, also move the ownership
		 * to the owner of the other roadtype. */
		RoadType other_rt = (rt == ROADTYPE_ROAD) ? ROADTYPE_TRAM : ROADTYPE_ROAD;
		Owner other_owner = GetRoadOwner(tile, other_rt);
		if (other_owner != GetTileOwner(tile)) {
			SetTileOwner(tile, other_owner);
			SetTileOwner(other_end, other_owner);
		}

		/* Mark tiles dirty that have been repaved */
		MarkTileDirtyByTile(tile);
		MarkTileDirtyByTile(other_end);
	}

	return cost;
}

/**
 * Delete a piece of road from a station
 * @param tile tile where to remove road from
 * @param flags operation to perform
 * @param pieces roadbits to remove
 * @param rt roadtype to remove
 * @param town_check should we check if the town allows removal?
 */
static CommandCost RemoveRoad_Station(TileIndex tile, DoCommandFlag flags, RoadBits pieces, RoadType rt, bool town_check)
{
	if (!IsDriveThroughStopTile(tile)) return CMD_ERROR;

	CommandCost ret = EnsureNoVehicleOnGround(tile);
	if (ret.Failed()) return ret;

	ret = CheckAllowRemoveRoad(tile, pieces, GetRoadOwner(tile, rt), rt, flags, town_check);
	if (ret.Failed()) return ret;

	/* If it's the last roadtype, just clear the whole tile */
	if (GetRoadTypes(tile) == RoadTypeToRoadTypes(rt)) return DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);

	if (flags & DC_EXEC) {
		Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
		if (c != NULL) {
			/* A full diagonal road tile has two road bits. */
			c->infrastructure.road[rt] -= 2;
			DirtyCompanyInfrastructureWindows(c->index);
		}
		SetRoadTypes(tile, GetRoadTypes(tile) & ~RoadTypeToRoadTypes(rt));
		MarkTileDirtyByTile(tile);
	}

	return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_CLEAR_ROAD] * 2);
}

/**
 * Delete a piece of road.
 * @param tile tile where to remove road from
 * @param flags operation to perform
 * @param pieces roadbits to remove
 * @param rt roadtype to remove
 * @param crossing_check should we check if there is a tram track when we are removing road from crossing?
 * @param town_check should we check if the town allows removal?
 */
CommandCost RemoveRoad(TileIndex tile, DoCommandFlag flags, RoadBits pieces, RoadType rt, bool crossing_check, bool town_check = true)
{
	/* The tile doesn't have the given road type */
	if (!HasTileRoadType(tile, rt)) return_cmd_error(rt == ROADTYPE_TRAM ? STR_ERROR_THERE_IS_NO_TRAMWAY : STR_ERROR_THERE_IS_NO_ROAD);

	switch (GetTileType(tile)) {
		case TT_ROAD: {
			if (IsTileSubtype(tile, TT_TRACK)) {
				return RemoveRoad_Road(tile, flags, pieces, rt, town_check);
			} else {
				return RemoveRoad_Bridge(tile, flags, pieces, rt, town_check);
			}
		}

		case TT_MISC:
			switch (GetTileSubtype(tile)) {
				case TT_MISC_CROSSING: return RemoveRoad_Crossing(tile, flags, pieces, rt, crossing_check, town_check);
				case TT_MISC_TUNNEL:   return RemoveRoad_Tunnel(tile, flags, pieces, rt, town_check);
				default: return CMD_ERROR;
			}

		case TT_STATION:
			return RemoveRoad_Station(tile, flags, pieces, rt, town_check);

		default:
			return CMD_ERROR;
	}
}


/**
 * Calculate the costs for roads on slopes
 * Also compute the road bits that have to be built to fit the slope
 *
 * @param tileh The current slope
 * @param pieces The RoadBits we want to add
 * @param existing The existent RoadBits of the current type
 * @param other The existent RoadBits for the other type
 * @param build The actual RoadBits to build
 * @return The costs for these RoadBits on this slope
 */
static CommandCost CheckRoadSlope(Slope tileh, RoadBits pieces, RoadBits existing, RoadBits other, RoadBits *build)
{
	/* Remove already build pieces */
	CLRBITS(pieces, existing);

	/* If we can't build anything stop here */
	if (pieces == ROAD_NONE) return CMD_ERROR;

	/* All RoadBit combos are valid on flat land */
	if (tileh == SLOPE_FLAT) {
		if (build != NULL) *build = pieces;
		return CommandCost();
	}

	/* Steep slopes behave the same as slopes with one corner raised. */
	if (IsSteepSlope(tileh)) {
		tileh = SlopeWithOneCornerRaised(GetHighestSlopeCorner(tileh));
	}

	/* Roads on slopes */
	if (_settings_game.construction.build_on_slopes && (_invalid_leveled_roadbits[tileh] & (other | existing | pieces)) == ROAD_NONE) {
		if (build != NULL) *build = pieces;

		/* If we add leveling we've got to pay for it */
		if ((other | existing) == ROAD_NONE) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);

		return CommandCost();
	}

	/* Autocomplete uphill roads */
	pieces |= MirrorRoadBits(pieces);
	RoadBits type_bits = existing | pieces;

	/* Uphill roads */
	if (IsStraightRoad(type_bits) && (other == type_bits || other == ROAD_NONE) &&
			(_invalid_straight_roadbits[tileh] & type_bits) == ROAD_NONE) {
		/* Slopes without foundation */
		if (!IsSlopeWithOneCornerRaised(tileh)) {
			if (build != NULL) *build = pieces;
			if (HasExactlyOneBit(existing) && GetRoadFoundation(tileh, existing) == FOUNDATION_NONE) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
			return CommandCost();
		}

		/* Prevent build on slopes if it isn't allowed */
		if (_settings_game.construction.build_on_slopes) {
			if (build != NULL) *build = pieces;

			/* If we add foundation we've got to pay for it */
			if ((other | existing) == ROAD_NONE) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);

			return CommandCost();
		}
	}
	return CMD_ERROR;
}

/**
 * Build a piece of road.
 * @param tile tile where to build road
 * @param flags operation to perform
 * @param p1 bit 0..3 road pieces to build (RoadBits)
 *           bit 4..5 road type
 *           bit 6..7 disallowed directions to toggle
 * @param p2 the town that is building the road (0 if not applicable)
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildRoad(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	CompanyID company = _current_company;
	CommandCost cost(EXPENSES_CONSTRUCTION);

	RoadBits existing = ROAD_NONE;
	RoadBits other_bits = ROAD_NONE;

	/* Road pieces are max 4 bitset values (NE, NW, SE, SW) and town can only be non-zero
	 * if a non-company is building the road */
	if ((Company::IsValidID(company) && p2 != 0) || (company == OWNER_TOWN && !Town::IsValidID(p2)) || (company == OWNER_DEITY && p2 != 0)) return CMD_ERROR;
	if (company != OWNER_TOWN) {
		const Town *town = CalcClosestTownFromTile(tile);
		p2 = (town != NULL) ? town->index : (TownID)INVALID_TOWN;

		if (company == OWNER_DEITY) {
			company = OWNER_TOWN;

			/* If we are not within a town, we are not owned by the town */
			if (town == NULL || DistanceSquare(tile, town->xy) > town->cache.squared_town_zone_radius[HZB_TOWN_EDGE]) {
				company = OWNER_NONE;
			}
		}
	}

	RoadBits pieces = Extract<RoadBits, 0, 4>(p1);

	/* do not allow building 'zero' road bits, code wouldn't handle it */
	if (pieces == ROAD_NONE) return CMD_ERROR;

	RoadType rt = Extract<RoadType, 4, 2>(p1);
	if (!IsValidRoadType(rt) || !ValParamRoadType(rt)) return CMD_ERROR;

	DisallowedRoadDirections toggle_drd = Extract<DisallowedRoadDirections, 6, 2>(p1);

	Slope tileh = GetTileSlope(tile);

	bool need_to_clear = false;
	switch (GetTileType(tile)) {
		case TT_ROAD:
			if (IsTileSubtype(tile, TT_TRACK)) {
				if (HasRoadWorks(tile)) return_cmd_error(STR_ERROR_ROAD_WORKS_IN_PROGRESS);

				other_bits = GetOtherRoadBits(tile, rt);
				if (!HasTileRoadType(tile, rt)) break;

				existing = GetRoadBits(tile, rt);
				bool crossing = !IsStraightRoad(existing | pieces);
				if (rt != ROADTYPE_TRAM && (GetDisallowedRoadDirections(tile) != DRD_NONE || toggle_drd != DRD_NONE) && crossing) {
					/* Junctions cannot be one-way */
					return_cmd_error(STR_ERROR_ONEWAY_ROADS_CAN_T_HAVE_JUNCTION);
				}
				if ((existing & pieces) == pieces) {
					/* We only want to set the (dis)allowed road directions */
					if (toggle_drd != DRD_NONE && rt != ROADTYPE_TRAM) {
						if (crossing) return_cmd_error(STR_ERROR_ONEWAY_ROADS_CAN_T_HAVE_JUNCTION);

						Owner owner = GetRoadOwner(tile, ROADTYPE_ROAD);
						if (owner != OWNER_NONE) {
							CommandCost ret = CheckOwnership(owner, tile);
							if (ret.Failed()) return ret;
						}

						DisallowedRoadDirections dis_existing = GetDisallowedRoadDirections(tile);
						DisallowedRoadDirections dis_new      = dis_existing ^ toggle_drd;

						/* We allow removing disallowed directions to break up
						 * deadlocks, but adding them can break articulated
						 * vehicles. As such, only when less is disallowed,
						 * i.e. bits are removed, we skip the vehicle check. */
						if (CountBits(dis_existing) <= CountBits(dis_new)) {
							CommandCost ret = EnsureNoVehicleOnGround(tile);
							if (ret.Failed()) return ret;
						}

						/* Ignore half built tiles */
						if ((flags & DC_EXEC) && rt != ROADTYPE_TRAM && IsStraightRoad(existing)) {
							SetDisallowedRoadDirections(tile, dis_new);
							MarkTileDirtyByTile(tile);
						}
						return CommandCost();
					}
					return_cmd_error(STR_ERROR_ALREADY_BUILT);
				}
			} else {
				/* Only allow building the outern roadbit, so building long roads stops at existing bridges */
				if (MirrorRoadBits(DiagDirToRoadBits(GetTunnelBridgeDirection(tile))) != pieces) goto do_clear;
				if (HasTileRoadType(tile, rt)) return_cmd_error(STR_ERROR_ALREADY_BUILT);
				/* Don't allow adding roadtype to the bridge when vehicles are already driving on it */
				CommandCost ret = TunnelBridgeIsFree(tile, GetOtherBridgeEnd(tile));
				if (ret.Failed()) return ret;
			}
			break;

		case TT_MISC:
			switch (GetTileSubtype(tile)) {
				case TT_MISC_CROSSING: {
					other_bits = GetCrossingRoadBits(tile);
					if (pieces & ComplementRoadBits(other_bits)) goto do_clear;
					pieces = other_bits; // we need to pay for both roadbits

					if (HasTileRoadType(tile, rt)) return_cmd_error(STR_ERROR_ALREADY_BUILT);
					break;
				}

				case TT_MISC_TUNNEL: {
					if (GetTunnelTransportType(tile) != TRANSPORT_ROAD) goto do_clear;
					/* Only allow building the outern roadbit, so building long roads stops at existing bridges */
					if (MirrorRoadBits(DiagDirToRoadBits(GetTunnelBridgeDirection(tile))) != pieces) goto do_clear;
					if (HasTileRoadType(tile, rt)) return_cmd_error(STR_ERROR_ALREADY_BUILT);
					/* Don't allow adding roadtype to the bridge/tunnel when vehicles are already driving on it */
					CommandCost ret = TunnelBridgeIsFree(tile, GetOtherTunnelBridgeEnd(tile));
					if (ret.Failed()) return ret;
					break;
				}

				case TT_MISC_DEPOT:
					if (IsRoadDepot(tile) && (GetAnyRoadBits(tile, rt) & pieces) == pieces) return_cmd_error(STR_ERROR_ALREADY_BUILT);
					/* fall through */
				default:
					goto do_clear;
			}
			break;

		case TT_RAILWAY: {
			if (!IsTileSubtype(tile, TT_TRACK)) goto do_clear;

			if (IsSteepSlope(tileh)) {
				return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
			}

			/* Level crossings may only be built on these slopes */
			if (!HasBit(VALID_LEVEL_CROSSING_SLOPES, tileh)) {
				return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
			}

			if (HasSignalOnTrack(tile, TRACK_UPPER)) goto do_clear;

			if (RailNoLevelCrossings(GetRailType(tile))) {
				return_cmd_error(STR_ERROR_CROSSING_DISALLOWED);
			}

			Axis roaddir;
			switch (GetTrackBits(tile)) {
				case TRACK_BIT_X:
					if (pieces & ROAD_X) goto do_clear;
					roaddir = AXIS_Y;
					break;

				case TRACK_BIT_Y:
					if (pieces & ROAD_Y) goto do_clear;
					roaddir = AXIS_X;
					break;

				default: goto do_clear;
			}

			CommandCost ret = EnsureNoVehicleOnGround(tile);
			if (ret.Failed()) return ret;

			if (flags & DC_EXEC) {
				Track railtrack = AxisToTrack(OtherAxis(roaddir));
				YapfNotifyTrackLayoutChange(tile, railtrack);
				/* Update company infrastructure counts. A level crossing has two road bits. */
				Company *c = Company::GetIfValid(company);
				if (c != NULL) {
					c->infrastructure.road[rt] += 2;
					if (rt != ROADTYPE_ROAD) c->infrastructure.road[ROADTYPE_ROAD] += 2;
					DirtyCompanyInfrastructureWindows(company);
				}
				/* Update rail count for level crossings. The plain track is already
				 * counted, so only add the difference to the level crossing cost. */
				c = Company::GetIfValid(GetTileOwner(tile));
				if (c != NULL) c->infrastructure.rail[GetRailType(tile)] += LEVELCROSSING_TRACKBIT_FACTOR - 1;

				/* Always add road to the roadtypes (can't draw without it) */
				bool reserved = HasBit(GetRailReservationTrackBits(tile), railtrack);
				MakeRoadCrossing(tile, company, company, GetTileOwner(tile), roaddir, GetRailType(tile), RoadTypeToRoadTypes(rt) | ROADTYPES_ROAD, p2);
				SetCrossingReservation(tile, reserved);
				UpdateLevelCrossing(tile, false);
				MarkTileDirtyByTile(tile);
			}
			return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_ROAD] * (rt == ROADTYPE_ROAD ? 2 : 4));
		}

		case TT_STATION: {
			if ((GetAnyRoadBits(tile, rt) & pieces) == pieces) return_cmd_error(STR_ERROR_ALREADY_BUILT);
			if (!IsDriveThroughStopTile(tile)) goto do_clear;

			RoadBits curbits = AxisToRoadBits(DiagDirToAxis(GetRoadStopDir(tile)));
			if (pieces & ~curbits) goto do_clear;
			pieces = curbits; // we need to pay for both roadbits

			if (HasTileRoadType(tile, rt)) return_cmd_error(STR_ERROR_ALREADY_BUILT);
			break;
		}

		default: {
do_clear:;
			need_to_clear = true;
			break;
		}
	}

	if (need_to_clear) {
		CommandCost ret = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);
	}

	if (other_bits != pieces) {
		/* Check the foundation/slopes when adding road/tram bits */
		CommandCost ret = CheckRoadSlope(tileh, pieces, existing, other_bits, &pieces);
		/* Return an error if we need to build a foundation (ret != 0) but the
		 * current setting is turned off */
		if (ret.Failed() || (ret.GetCost() != 0 && !_settings_game.construction.build_on_slopes)) {
			return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
		}
		cost.AddCost(ret);
	}

	if (!need_to_clear) {
		if (IsNormalRoadTile(tile) || IsLevelCrossingTile(tile)) {
			/* Don't put the pieces that already exist */
			pieces &= ComplementRoadBits(existing);

			/* Check if new road bits will have the same foundation as other existing road types */
			if (IsNormalRoadTile(tile)) {
				Slope slope = GetTileSlope(tile);
				Foundation found_new = GetRoadFoundation(slope, pieces | existing);

				/* Test if all other roadtypes can be built at that foundation */
				for (RoadType rtest = ROADTYPE_ROAD; rtest < ROADTYPE_END; rtest++) {
					if (rtest != rt) { // check only other road types
						RoadBits bits = GetRoadBits(tile, rtest);
						/* do not check if there are not road bits of given type */
						if (bits != ROAD_NONE && GetRoadFoundation(slope, bits) != found_new) {
							return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
						}
					}
				}
			}
		}

		CommandCost ret = EnsureNoVehicleOnGround(tile);
		if (ret.Failed()) return ret;

	}

	uint num_pieces = (!need_to_clear && (IsTunnelTile(tile) || IsRoadBridgeTile(tile))) ?
			/* There are 2 pieces on *every* tile of the bridge or tunnel */
			2 * (GetTunnelBridgeLength(GetOtherTunnelBridgeEnd(tile), tile) + 2) :
			/* Count pieces */
			CountBits(pieces);

	cost.AddCost(num_pieces * _price[PR_BUILD_ROAD]);

	if (flags & DC_EXEC) {
		switch (GetTileType(tile)) {
			case TT_ROAD:
				if (IsTileSubtype(tile, TT_TRACK)) {
					if (existing == ROAD_NONE) {
						SetRoadTypes(tile, GetRoadTypes(tile) | RoadTypeToRoadTypes(rt));
						SetRoadOwner(tile, rt, company);
						if (rt == ROADTYPE_ROAD) SetTownIndex(tile, p2);
					}
					SetRoadBits(tile, existing | pieces, rt);
				} else {
					TileIndex other_end = GetOtherBridgeEnd(tile);

					SetRoadTypes(other_end, GetRoadTypes(other_end) | RoadTypeToRoadTypes(rt));
					SetRoadTypes(tile, GetRoadTypes(tile) | RoadTypeToRoadTypes(rt));
					SetRoadOwner(other_end, rt, company);
					SetRoadOwner(tile, rt, company);

					/* Mark tiles dirty that have been repaved */
					MarkBridgeTilesDirty(tile, other_end, GetTunnelBridgeDirection(tile));
				}
				break;

			case TT_STATION:
				assert(IsDriveThroughStopTile(tile));
				SetRoadTypes(tile, GetRoadTypes(tile) | RoadTypeToRoadTypes(rt));
				SetRoadOwner(tile, rt, company);
				break;

			case TT_MISC:
				if (IsLevelCrossingTile(tile)) {
					SetRoadTypes(tile, GetRoadTypes(tile) | RoadTypeToRoadTypes(rt));
					SetRoadOwner(tile, rt, company);
					if (rt == ROADTYPE_ROAD) SetTownIndex(tile, p2);
					break;
				} else if (IsTunnelTile(tile)) {
					TileIndex other_end = GetOtherTunnelBridgeEnd(tile);

					SetRoadTypes(other_end, GetRoadTypes(other_end) | RoadTypeToRoadTypes(rt));
					SetRoadTypes(tile, GetRoadTypes(tile) | RoadTypeToRoadTypes(rt));
					SetRoadOwner(other_end, rt, company);
					SetRoadOwner(tile, rt, company);

					/* Mark tiles dirty that have been repaved */
					MarkTileDirtyByTile(other_end);
					MarkTileDirtyByTile(tile);
					break;
				}
				/* fall through */
			default:
				MakeRoadNormal(tile, pieces, RoadTypeToRoadTypes(rt), p2, company, company);
				break;
		}

		/* Update company infrastructure count. */
		Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
		if (c != NULL) {
			if (IsTunnelTile(tile) || IsRoadBridgeTile(tile)) num_pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
			c->infrastructure.road[rt] += num_pieces;
			DirtyCompanyInfrastructureWindows(c->index);
		}

		if (rt != ROADTYPE_TRAM && IsNormalRoadTile(tile)) {
			existing |= pieces;
			SetDisallowedRoadDirections(tile, IsStraightRoad(existing) ?
					GetDisallowedRoadDirections(tile) ^ toggle_drd : DRD_NONE);
		}

		MarkTileDirtyByTile(tile);
	}
	return cost;
}

/**
 * Checks whether a road or tram connection can be found when building a new road or tram.
 * @param tile Tile at which the road being built will end.
 * @param rt Roadtype of the road being built.
 * @param dir Direction that the road is following.
 * @return True if the next tile at dir direction is suitable for being connected directly by a second roadbit at the end of the road being built.
 */
static bool CanConnectToRoad(TileIndex tile, RoadType rt, DiagDirection dir)
{
	RoadBits bits = GetAnyRoadBits(tile + TileOffsByDiagDir(dir), rt, false);
	return (bits & DiagDirToRoadBits(ReverseDiagDir(dir))) != 0;
}

/**
 * Build a long piece of road.
 * @param start_tile start tile of drag (the building cost will appear over this tile)
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit 0) - start tile starts in the 2nd half of tile (p2 & 1). Only used if bit 6 is set or if we are building a single tile
 * - p2 = (bit 1) - end tile starts in the 2nd half of tile (p2 & 2). Only used if bit 6 is set or if we are building a single tile
 * - p2 = (bit 2) - direction: 0 = along x-axis, 1 = along y-axis (p2 & 4)
 * - p2 = (bit 3 + 4) - road type
 * - p2 = (bit 5) - set road direction
 * - p2 = (bit 6) - defines two different behaviors for this command:
 *      - 0 = Build up to an obstacle. Do not build the first and last roadbits unless they can be connected to something, or if we are building a single tile
 *      - 1 = Fail if an obstacle is found. Always take into account bit 0 and 1. This behavior is used for scripts
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildLongRoad(TileIndex start_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	DisallowedRoadDirections drd = DRD_NORTHBOUND;

	if (p1 >= MapSize()) return CMD_ERROR;

	TileIndex end_tile = p1;
	RoadType rt = Extract<RoadType, 3, 2>(p2);
	if (!IsValidRoadType(rt) || !ValParamRoadType(rt)) return CMD_ERROR;

	Axis axis = Extract<Axis, 2, 1>(p2);
	/* Only drag in X or Y direction dictated by the direction variable */
	if (axis == AXIS_X && TileY(start_tile) != TileY(end_tile)) return CMD_ERROR; // x-axis
	if (axis == AXIS_Y && TileX(start_tile) != TileX(end_tile)) return CMD_ERROR; // y-axis

	DiagDirection dir = AxisToDiagDir(axis);

	/* Swap direction, also the half-tile drag var (bit 0 and 1) */
	if (start_tile > end_tile || (start_tile == end_tile && HasBit(p2, 0))) {
		dir = ReverseDiagDir(dir);
		p2 ^= 3;
		drd = DRD_SOUTHBOUND;
	}

	/* On the X-axis, we have to swap the initial bits, so they
	 * will be interpreted correctly in the GTTS. Furthermore
	 * when you just 'click' on one tile to build them. */
	if ((axis == AXIS_Y) == (start_tile == end_tile && HasBit(p2, 0) == HasBit(p2, 1))) drd ^= DRD_BOTH;
	/* No disallowed direction bits have to be toggled */
	if (!HasBit(p2, 5)) drd = DRD_NONE;

	CommandCost cost(EXPENSES_CONSTRUCTION);
	CommandCost last_error = CMD_ERROR;
	TileIndex tile = start_tile;
	bool had_bridge = false;
	bool had_tunnel = false;
	bool had_success = false;
	bool is_ai = HasBit(p2, 6);

	/* Start tile is the first tile clicked by the user. */
	for (;;) {
		RoadBits bits = AxisToRoadBits(axis);

		/* Determine which road parts should be built. */
		if (!is_ai && start_tile != end_tile) {
			/* Only build the first and last roadbit if they can connect to something. */
			if (tile == end_tile && !CanConnectToRoad(tile, rt, dir)) {
				bits = DiagDirToRoadBits(ReverseDiagDir(dir));
			} else if (tile == start_tile && !CanConnectToRoad(tile, rt, ReverseDiagDir(dir))) {
				bits = DiagDirToRoadBits(dir);
			}
		} else {
			/* Road parts only have to be built at the start tile or at the end tile. */
			if (tile == end_tile && !HasBit(p2, 1)) bits &= DiagDirToRoadBits(ReverseDiagDir(dir));
			if (tile == start_tile && HasBit(p2, 0)) bits &= DiagDirToRoadBits(dir);
		}

		CommandCost ret = DoCommand(tile, drd << 6 | rt << 4 | bits, 0, flags, CMD_BUILD_ROAD);
		if (ret.Failed()) {
			last_error = ret;
			if (last_error.GetErrorMessage() != STR_ERROR_ALREADY_BUILT) {
				if (is_ai) return last_error;
				break;
			}
		} else {
			had_success = true;
			/* Only pay for the upgrade on one side of the bridges and tunnels */
			if (IsTunnelTile(tile)) {
				if (!had_tunnel || GetTunnelBridgeDirection(tile) == dir) {
					cost.AddCost(ret);
				}
				had_tunnel = true;
			} else if (IsRoadBridgeTile(tile)) {
				if (!had_bridge || GetTunnelBridgeDirection(tile) == dir) {
					cost.AddCost(ret);
				}
				had_bridge = true;
			} else {
				cost.AddCost(ret);
			}
		}

		if (tile == end_tile) break;

		tile += TileOffsByDiagDir(dir);
	}

	return had_success ? cost : last_error;
}

/**
 * Remove a long piece of road.
 * @param start_tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit 0) - start tile starts in the 2nd half of tile (p2 & 1)
 * - p2 = (bit 1) - end tile starts in the 2nd half of tile (p2 & 2)
 * - p2 = (bit 2) - direction: 0 = along x-axis, 1 = along y-axis (p2 & 4)
 * - p2 = (bit 3 + 4) - road type
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdRemoveLongRoad(TileIndex start_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	CommandCost cost(EXPENSES_CONSTRUCTION);

	if (p1 >= MapSize()) return CMD_ERROR;

	TileIndex end_tile = p1;
	RoadType rt = Extract<RoadType, 3, 2>(p2);
	if (!IsValidRoadType(rt)) return CMD_ERROR;

	Axis axis = Extract<Axis, 2, 1>(p2);
	/* Only drag in X or Y direction dictated by the direction variable */
	if (axis == AXIS_X && TileY(start_tile) != TileY(end_tile)) return CMD_ERROR; // x-axis
	if (axis == AXIS_Y && TileX(start_tile) != TileX(end_tile)) return CMD_ERROR; // y-axis

	/* Swap start and ending tile, also the half-tile drag var (bit 0 and 1) */
	if (start_tile > end_tile || (start_tile == end_tile && HasBit(p2, 0))) {
		TileIndex t = start_tile;
		start_tile = end_tile;
		end_tile = t;
		p2 ^= IsInsideMM(p2 & 3, 1, 3) ? 3 : 0;
	}

	Money money = GetAvailableMoneyForCommand();
	TileIndex tile = start_tile;
	CommandCost last_error = CMD_ERROR;
	bool had_success = false;
	/* Start tile is the small number. */
	for (;;) {
		RoadBits bits = AxisToRoadBits(axis);

		if (tile == end_tile && !HasBit(p2, 1)) bits &= ROAD_NW | ROAD_NE;
		if (tile == start_tile && HasBit(p2, 0)) bits &= ROAD_SE | ROAD_SW;

		/* try to remove the halves. */
		if (bits != 0) {
			CommandCost ret = RemoveRoad(tile, flags & ~DC_EXEC, bits, rt, true);
			if (ret.Succeeded()) {
				if (flags & DC_EXEC) {
					money -= ret.GetCost();
					if (money < 0) {
						_additional_cash_required = DoCommand(start_tile, end_tile, p2, flags & ~DC_EXEC, CMD_REMOVE_LONG_ROAD).GetCost();
						return cost;
					}
					RemoveRoad(tile, flags, bits, rt, true, false);
				}
				cost.AddCost(ret);
				had_success = true;
			} else {
				/* Ownership errors are more important. */
				if (last_error.GetErrorMessage() != STR_ERROR_OWNED_BY) last_error = ret;
			}
		}

		if (tile == end_tile) break;

		tile += (axis == AXIS_Y) ? TileDiffXY(0, 1) : TileDiffXY(1, 0);
	}

	return had_success ? cost : last_error;
}

/**
 * Build a road depot.
 * @param tile tile where to build the depot
 * @param flags operation to perform
 * @param p1 bit 0..1 entrance direction (DiagDirection)
 *           bit 2..3 road type
 * @param p2 unused
 * @param text unused
 * @return the cost of this operation or an error
 *
 * @todo When checking for the tile slope,
 * distinguish between "Flat land required" and "land sloped in wrong direction"
 */
CommandCost CmdBuildRoadDepot(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	DiagDirection dir = Extract<DiagDirection, 0, 2>(p1);
	RoadType rt = Extract<RoadType, 2, 2>(p1);

	if (!IsValidRoadType(rt) || !ValParamRoadType(rt)) return CMD_ERROR;

	Slope tileh = GetTileSlope(tile);
	if (tileh != SLOPE_FLAT && (
				!_settings_game.construction.build_on_slopes ||
				!CanBuildDepotByTileh(dir, tileh)
			)) {
		return_cmd_error(STR_ERROR_FLAT_LAND_REQUIRED);
	}

	CommandCost cost = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (cost.Failed()) return cost;

	if (HasBridgeAbove(tile)) return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);

	if (!Depot::CanAllocateItem()) return CMD_ERROR;

	if (flags & DC_EXEC) {
		Depot *dep = new Depot(tile);
		dep->build_date = _date;

		/* A road depot has two road bits. */
		Company::Get(_current_company)->infrastructure.road[rt] += 2;
		DirtyCompanyInfrastructureWindows(_current_company);

		MakeRoadDepot(tile, _current_company, dep->index, dir, rt);
		MarkTileDirtyByTile(tile);
		MakeDefaultName(dep);
	}
	cost.AddCost(_price[PR_BUILD_DEPOT_ROAD]);
	return cost;
}

static CommandCost ClearTile_Road(TileIndex tile, DoCommandFlag flags)
{
	if (IsTileSubtype(tile, TT_TRACK)) {
		RoadBits b = GetAllRoadBits(tile);

		/* Clear the road if only one piece is on the tile OR we are not using the DC_AUTO flag */
		if ((HasExactlyOneBit(b) && GetRoadBits(tile, ROADTYPE_TRAM) == ROAD_NONE) || !(flags & DC_AUTO)) {
			CommandCost ret(EXPENSES_CONSTRUCTION);
			RoadType rt;
			FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
				CommandCost tmp_ret = RemoveRoad(tile, flags, GetRoadBits(tile, rt), rt, true);
				if (tmp_ret.Failed()) return tmp_ret;
				ret.AddCost(tmp_ret);
			}
			return ret;
		}

		return_cmd_error(STR_ERROR_MUST_REMOVE_ROAD_FIRST);
	} else {
		if (flags & DC_AUTO) return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);

		/* Floods can remove anything as well as the scenario editor */

		if (_current_company != OWNER_WATER && _game_mode != GM_EDITOR) {
			RoadTypes rts = GetRoadTypes(tile);
			Owner road_owner = _current_company;
			if (HasBit(rts, ROADTYPE_ROAD)) road_owner = GetRoadOwner(tile, ROADTYPE_ROAD);

			/* We can remove unowned road and if the town allows it */
			if (road_owner == OWNER_TOWN && _current_company != OWNER_TOWN && !(_settings_game.construction.extra_dynamite || _cheats.magic_bulldozer.value)) {
				/* Town does not allow */
				CommandCost ret = CheckTileOwnership(tile);
				if (ret.Failed()) return ret;
			} else {
				if (road_owner != OWNER_NONE && road_owner != OWNER_TOWN) {
					CommandCost ret = CheckOwnership(road_owner, tile);
					if (ret.Failed()) return ret;
				}

				if (HasBit(rts, ROADTYPE_TRAM)) {
					Owner tram_owner = GetRoadOwner(tile, ROADTYPE_TRAM);
					if (tram_owner != OWNER_NONE) {
						CommandCost ret = CheckOwnership(tram_owner, tile);
						if (ret.Failed()) return ret;
					}
				}
			}
		}

		TileIndex endtile = GetOtherBridgeEnd(tile);

		CommandCost ret = TunnelBridgeIsFree(tile, endtile);
		if (ret.Failed()) return ret;

		if (IsTileOwner(tile, OWNER_TOWN) && _game_mode != GM_EDITOR) {
			Town *t = ClosestTownFromTile(tile, UINT_MAX); // town penalty rating

			/* Check if you are allowed to remove the bridge owned by a town
			 * Removal depends on difficulty settings */
			CommandCost ret = CheckforTownRating(flags, t, TUNNELBRIDGE_REMOVE);
			if (ret.Failed()) return ret;

			/* checks if the owner is town then decrease town rating by RATING_TUNNEL_BRIDGE_DOWN_STEP until
			 * you have a "Poor" (0) town rating */
			ChangeTownRating(t, RATING_TUNNEL_BRIDGE_DOWN_STEP, RATING_TUNNEL_BRIDGE_MINIMUM, flags);
		}

		uint len = GetTunnelBridgeLength(tile, endtile) + 2; // Don't forget the end tiles.

		if (flags & DC_EXEC) {
			/* Update company infrastructure counts. */
			RoadType rt;
			FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
				Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
				if (c != NULL) {
					/* A full diagonal road tile has two road bits. */
					c->infrastructure.road[rt] -= len * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
					DirtyCompanyInfrastructureWindows(c->index);
				}
			}

			RemoveBridgeMiddleTiles(tile, endtile);
			DoClearSquare(tile);
			DoClearSquare(endtile);
		}

		return CommandCost(EXPENSES_CONSTRUCTION, len * _price[PR_CLEAR_BRIDGE]);
	}
}


/**
 * Get the foundationtype of a RoadBits Slope combination
 *
 * @param tileh The Slope part
 * @param bits The RoadBits part
 * @return The resulting Foundation
 */
static Foundation GetRoadFoundation(Slope tileh, RoadBits bits)
{
	/* Flat land and land without a road doesn't require a foundation */
	if (tileh == SLOPE_FLAT || bits == ROAD_NONE) return FOUNDATION_NONE;

	/* Steep slopes behave the same as slopes with one corner raised. */
	if (IsSteepSlope(tileh)) {
		tileh = SlopeWithOneCornerRaised(GetHighestSlopeCorner(tileh));
	}

	/* Leveled RoadBits on a slope */
	if ((_invalid_leveled_roadbits[tileh] & bits) == ROAD_NONE) return FOUNDATION_LEVELED;

	/* Straight roads without foundation on a slope */
	if (!IsSlopeWithOneCornerRaised(tileh) &&
			(_invalid_straight_roadbits[tileh] & bits) == ROAD_NONE)
		return FOUNDATION_NONE;

	/* Roads on steep Slopes or on Slopes with one corner raised */
	return (bits == ROAD_X ? FOUNDATION_INCLINED_X : FOUNDATION_INCLINED_Y);
}

const byte _road_sloped_sprites[14] = {
	0,  0,  2,  0,
	0,  1,  0,  0,
	3,  0,  0,  0,
	0,  0
};

/**
 * Whether to draw unpaved roads regardless of the town zone.
 * By default, OpenTTD always draws roads as unpaved if they are on a desert
 * tile or above the snowline. Newgrf files, however, can set a bit that allows
 * paved roads to be built on desert tiles as they would be on grassy tiles.
 *
 * @param tile The tile the road is on
 * @param roadside What sort of road this is
 * @return True if the road should be drawn unpaved regardless of the roadside.
 */
static bool AlwaysDrawUnpavedRoads(TileIndex tile, Roadside roadside)
{
	return (IsOnSnow(tile) &&
			!(_settings_game.game_creation.landscape == LT_TROPIC && HasGrfMiscBit(GMB_DESERT_PAVED_ROADS) &&
				roadside != ROADSIDE_BARREN && roadside != ROADSIDE_GRASS && roadside != ROADSIDE_GRASS_ROAD_WORKS));
}

/**
 * Draws the catenary for the given tile
 * @param ti   information about the tile (slopes, height etc)
 * @param tram the roadbits for the tram
 */
void DrawTramCatenary(const TileInfo *ti, RoadBits tram)
{
	/* Do not draw catenary if it is invisible */
	if (IsInvisibilitySet(TO_CATENARY)) return;

	/* Don't draw the catenary under a low bridge */
	if (HasBridgeAbove(ti->tile) && !IsTransparencySet(TO_CATENARY)) {
		int height = GetBridgeHeight(GetNorthernBridgeEnd(ti->tile));

		if (height <= GetTileMaxZ(ti->tile) + 1) return;
	}

	SpriteID front;
	SpriteID back;

	if (ti->tileh != SLOPE_FLAT) {
		back  = SPR_TRAMWAY_BACK_WIRES_SLOPED  + _road_sloped_sprites[ti->tileh - 1];
		front = SPR_TRAMWAY_FRONT_WIRES_SLOPED + _road_sloped_sprites[ti->tileh - 1];
	} else {
		back  = SPR_TRAMWAY_BASE + _road_backpole_sprites_1[tram];
		front = SPR_TRAMWAY_BASE + _road_frontwire_sprites_1[tram];
	}

	AddSortableSpriteToDraw(back,  PAL_NONE, ti->x, ti->y, 16, 16, TILE_HEIGHT + BB_HEIGHT_UNDER_BRIDGE, ti->z, IsTransparencySet(TO_CATENARY));
	AddSortableSpriteToDraw(front, PAL_NONE, ti->x, ti->y, 16, 16, TILE_HEIGHT + BB_HEIGHT_UNDER_BRIDGE, ti->z, IsTransparencySet(TO_CATENARY));
}

/**
 * Draws details on/around the road
 * @param img the sprite to draw
 * @param ti  the tile to draw on
 * @param dx  the offset from the top of the BB of the tile
 * @param dy  the offset from the top of the BB of the tile
 * @param h   the height of the sprite to draw
 */
static void DrawRoadDetail(SpriteID img, const TileInfo *ti, int dx, int dy, int h)
{
	int x = ti->x | dx;
	int y = ti->y | dy;
	int z = ti->z;
	if (ti->tileh != SLOPE_FLAT) z = GetSlopePixelZ(x, y);
	AddSortableSpriteToDraw(img, PAL_NONE, x, y, 2, 2, h, z);
}

/**
 * Draw ground sprite and road pieces
 * @param ti TileInfo
 */
static void DrawRoadBits(TileInfo *ti)
{
	RoadBits road = GetRoadBits(ti->tile, ROADTYPE_ROAD);
	RoadBits tram = GetRoadBits(ti->tile, ROADTYPE_TRAM);

	SpriteID image = 0;
	PaletteID pal = PAL_NONE;

	if (ti->tileh != SLOPE_FLAT) {
		DrawFoundation(ti, GetRoadFoundation(ti->tileh, road | tram));

		/* DrawFoundation() modifies ti.
		 * Default sloped sprites.. */
		if (ti->tileh != SLOPE_FLAT) image = _road_sloped_sprites[ti->tileh - 1] + SPR_ROAD_SLOPE_START;
	}

	if (image == 0) image = _road_tile_sprites_1[road != ROAD_NONE ? road : tram];

	Roadside roadside = GetRoadside(ti->tile);

	if (AlwaysDrawUnpavedRoads(ti->tile, roadside)) {
		image += 19;
	} else {
		switch (roadside) {
			case ROADSIDE_BARREN:           pal = PALETTE_TO_BARE_LAND; break;
			case ROADSIDE_GRASS:            break;
			case ROADSIDE_GRASS_ROAD_WORKS: break;
			default:                        image -= 19; break; // Paved
		}
	}

	DrawGroundSprite(image, pal);

	/* For tram we overlay the road graphics with either tram tracks only
	 * (when there is actual road beneath the trams) or with tram tracks
	 * and some dirts which hides the road graphics */
	if (tram != ROAD_NONE) {
		if (ti->tileh != SLOPE_FLAT) {
			image = _road_sloped_sprites[ti->tileh - 1] + SPR_TRAMWAY_SLOPED_OFFSET;
		} else {
			image = _road_tile_sprites_1[tram] - SPR_ROAD_Y;
		}
		image += (road == ROAD_NONE) ? SPR_TRAMWAY_TRAM : SPR_TRAMWAY_OVERLAY;
		DrawGroundSprite(image, pal);
	}

	if (road != ROAD_NONE) {
		DisallowedRoadDirections drd = GetDisallowedRoadDirections(ti->tile);
		if (drd != DRD_NONE) {
			DrawGroundSpriteAt(SPR_ONEWAY_BASE + drd - 1 + ((road == ROAD_X) ? 0 : 3), PAL_NONE, 8, 8, GetPartialPixelZ(8, 8, ti->tileh));
		}
	}

	if (HasRoadWorks(ti->tile)) {
		/* Road works */
		DrawGroundSprite((road | tram) & ROAD_X ? SPR_EXCAVATION_X : SPR_EXCAVATION_Y, PAL_NONE);
		return;
	}

	if (tram != ROAD_NONE) DrawTramCatenary(ti, tram);

	/* Return if full detail is disabled, or we are zoomed fully out. */
	if (!HasBit(_display_opt, DO_FULL_DETAIL) || _cur_dpi->zoom > ZOOM_LVL_DETAIL) return;

	/* Do not draw details (street lights, trees) under low bridge */
	if (HasBridgeAbove(ti->tile) && (roadside == ROADSIDE_TREES || roadside == ROADSIDE_STREET_LIGHTS)) {
		int height = GetBridgeHeight(GetNorthernBridgeEnd(ti->tile));
		int minz = GetTileMaxZ(ti->tile) + 2;

		if (roadside == ROADSIDE_TREES) minz++;

		if (height < minz) return;
	}

	/* If there are no road bits, return, as there is nothing left to do */
	if (HasAtMostOneBit(road)) return;

	/* Draw extra details. */
	for (const DrawRoadTileStruct *drts = _road_display_table[roadside][road | tram]; drts->image != 0; drts++) {
		DrawRoadDetail(drts->image, ti, drts->subcoord_x, drts->subcoord_y, 0x10);
	}
}

/** Tile callback function for rendering a road tile to the screen */
static void DrawTile_Road(TileInfo *ti)
{
	if (IsTileSubtype(ti->tile, TT_TRACK)) {
		DrawRoadBits(ti);
	} else {
		DrawBridgeGround(ti);

		/* draw ramp */

		DiagDirection dir = GetTunnelBridgeDirection(ti->tile);

		const PalSpriteID *psid = GetBridgeRampSprite(GetRoadBridgeType(ti->tile), 8, ti->tileh, dir);

		/* Draw Trambits as SpriteCombine */
		StartSpriteCombine();

		/* HACK set the height of the BB of a sloped ramp to 1 so a vehicle on
		 * it doesn't disappear behind it
		 */
		/* Bridge heads are drawn solid no matter how invisibility/transparency is set */
		AddSortableSpriteToDraw(psid->sprite, psid->pal, ti->x, ti->y, 16, 16, ti->tileh == SLOPE_FLAT ? 0 : 8, ti->z);

		RoadTypes rts = GetRoadTypes(ti->tile);

		if (HasBit(rts, ROADTYPE_TRAM)) {
			uint offset = dir;
			int z = ti->z;
			if (ti->tileh != SLOPE_FLAT) {
				offset = (offset + 1) & 1;
				z += TILE_HEIGHT;
			} else {
				offset += 2;
			}
			/* DrawBridgeTramBits() calls EndSpriteCombine() and StartSpriteCombine() */
			DrawBridgeTramBits(ti->x, ti->y, z, offset, HasBit(rts, ROADTYPE_ROAD), true);
		}

		EndSpriteCombine();
	}

	DrawBridgeMiddle(ti);
}

void DrawLevelCrossing(TileInfo *ti)
{
	if (ti->tileh != SLOPE_FLAT) DrawFoundation(ti, FOUNDATION_LEVELED);

	PaletteID pal = PAL_NONE;
	const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));

	if (rti->UsesOverlay()) {
		Axis axis = GetCrossingRailAxis(ti->tile);
		SpriteID road = SPR_ROAD_Y + axis;

		Roadside roadside = GetRoadside(ti->tile);

		if (AlwaysDrawUnpavedRoads(ti->tile, roadside)) {
			road += 19;
		} else {
			switch (roadside) {
				case ROADSIDE_BARREN: pal = PALETTE_TO_BARE_LAND; break;
				case ROADSIDE_GRASS:  break;
				default:              road -= 19; break; // Paved
			}
		}

		DrawGroundSprite(road, pal);

		SpriteID rail = GetCustomRailSprite(rti, ti->tile, RTSG_CROSSING) + axis;
		/* Draw tracks, but draw PBS reserved tracks darker. */
		pal = (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation && HasCrossingReservation(ti->tile)) ? PALETTE_CRASH : PAL_NONE;
		DrawGroundSprite(rail, pal);

		DrawRailTileSeq(ti, &_crossing_layout, TO_CATENARY, rail, 0, PAL_NONE);
	} else {
		SpriteID image = rti->base_sprites.crossing;

		if (GetCrossingRoadAxis(ti->tile) == AXIS_X) image++;
		if (IsCrossingBarred(ti->tile)) image += 2;

		Roadside roadside = GetRoadside(ti->tile);

		if (AlwaysDrawUnpavedRoads(ti->tile, roadside)) {
			image += 8;
		} else {
			switch (roadside) {
				case ROADSIDE_BARREN: pal = PALETTE_TO_BARE_LAND; break;
				case ROADSIDE_GRASS:  break;
				default:              image += 4; break; // Paved
			}
		}

		DrawGroundSprite(image, pal);

		/* PBS debugging, draw reserved tracks darker */
		if (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation && HasCrossingReservation(ti->tile)) {
			DrawGroundSprite(GetCrossingRoadAxis(ti->tile) == AXIS_Y ? GetRailTypeInfo(GetRailType(ti->tile))->base_sprites.single_x : GetRailTypeInfo(GetRailType(ti->tile))->base_sprites.single_y, PALETTE_CRASH);
		}
	}

	if (HasTileRoadType(ti->tile, ROADTYPE_TRAM)) {
		DrawGroundSprite(SPR_TRAMWAY_OVERLAY + (GetCrossingRoadAxis(ti->tile) ^ 1), pal);
		DrawTramCatenary(ti, GetCrossingRoadBits(ti->tile));
	}

	if (HasCatenaryDrawn(GetRailType(ti->tile))) DrawCatenary(ti);

	DrawBridgeMiddle(ti);
}

/**
 * Updates cached nearest town for all road tiles
 * @param invalidate are we just invalidating cached data?
 * @pre invalidate == true implies _generating_world == true
 */
void UpdateNearestTownForRoadTiles(bool invalidate)
{
	assert(!invalidate || _generating_world);

	for (TileIndex t = 0; t < MapSize(); t++) {
		if ((IsRoadTile(t) || IsLevelCrossingTile(t)) && !HasTownOwnedRoad(t)) {
			TownID tid = (TownID)INVALID_TOWN;
			if (!invalidate) {
				const Town *town = CalcClosestTownFromTile(t);
				if (town != NULL) tid = town->index;
			}
			SetTownIndex(t, tid);
		}
	}
}

static int GetSlopePixelZ_Road(TileIndex tile, uint x, uint y)
{
	int z;
	Slope tileh = GetTilePixelSlope(tile, &z);

	if (IsTileSubtype(tile, TT_TRACK)) {
		if (tileh == SLOPE_FLAT) return z;
		z += ApplyPixelFoundationToSlope(GetRoadFoundation(tileh, GetAllRoadBits(tile)), &tileh);
		return z + GetPartialPixelZ(x & 0xF, y & 0xF, tileh);
	} else {
		x &= 0xF;
		y &= 0xF;

		DiagDirection dir = GetTunnelBridgeDirection(tile);

		z += ApplyPixelFoundationToSlope(GetBridgeFoundation(tileh, DiagDirToAxis(dir)), &tileh);

		/* On the bridge ramp? */
		uint pos = (DiagDirToAxis(dir) == AXIS_X ? y : x);
		if (5 <= pos && pos <= 10) {
			return z + ((tileh == SLOPE_FLAT) ? GetBridgePartialPixelZ(dir, x, y) : TILE_HEIGHT);
		}

		return z + GetPartialPixelZ(x, y, tileh);
	}
}

static Foundation GetFoundation_Road(TileIndex tile, Slope tileh)
{
	return IsTileSubtype(tile, TT_TRACK) ? GetRoadFoundation(tileh, GetAllRoadBits(tile)) : GetBridgeFoundation(tileh, DiagDirToAxis(GetTunnelBridgeDirection(tile)));
}

static const Roadside _town_road_types[][2] = {
	{ ROADSIDE_GRASS,         ROADSIDE_GRASS },
	{ ROADSIDE_PAVED,         ROADSIDE_PAVED },
	{ ROADSIDE_PAVED,         ROADSIDE_PAVED },
	{ ROADSIDE_TREES,         ROADSIDE_TREES },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED }
};

static const Roadside _town_road_types_2[][2] = {
	{ ROADSIDE_GRASS,         ROADSIDE_GRASS },
	{ ROADSIDE_PAVED,         ROADSIDE_PAVED },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED },
	{ ROADSIDE_STREET_LIGHTS, ROADSIDE_PAVED }
};

void UpdateRoadSide(TileIndex tile, HouseZonesBits grp)
{
	/* Adjust road ground type depending on 'grp' (grp is the distance to the center) */
	const Roadside *new_rs = (_settings_game.game_creation.landscape == LT_TOYLAND) ? _town_road_types_2[grp] : _town_road_types[grp];
	Roadside cur_rs = GetRoadside(tile);

	/* We have our desired type, do nothing */
	if (cur_rs == new_rs[0]) return;

	/* We have the pre-type of the desired type, switch to the desired type */
	if (cur_rs == new_rs[1]) {
		cur_rs = new_rs[0];
	/* We have barren land, install the pre-type */
	} else if (cur_rs == ROADSIDE_BARREN) {
		cur_rs = new_rs[1];
	/* We're totally off limits, remove any installation and make barren land */
	} else {
		cur_rs = ROADSIDE_BARREN;
	}

	SetRoadside(tile, cur_rs);
	MarkTileDirtyByTile(tile);
}

static void TileLoop_Road(TileIndex tile)
{
	switch (_settings_game.game_creation.landscape) {
		case LT_ARCTIC: {
			int z = IsTileSubtype(tile, TT_TRACK) ? GetTileZ(tile) : GetTileMaxZ(tile);
			if (IsOnSnow(tile) != (z > GetSnowLine())) {
				ToggleSnow(tile);
				MarkTileDirtyByTile(tile);
			}
			break;
		}

		case LT_TROPIC:
			if (GetTropicZone(tile) == TROPICZONE_DESERT && !IsOnDesert(tile)) {
				ToggleDesert(tile);
				MarkTileDirtyByTile(tile);
			}
			break;
	}

	if (!IsTileSubtype(tile, TT_TRACK)) return;

	const Town *t = ClosestTownFromTile(tile, UINT_MAX);
	if (!HasRoadWorks(tile)) {
		HouseZonesBits grp = HZB_TOWN_EDGE;

		if (t != NULL) {
			grp = GetTownRadiusGroup(t, tile);

			/* Show an animation to indicate road work */
			if (t->road_build_months != 0 &&
					(DistanceManhattan(t->xy, tile) < 8 || grp != HZB_TOWN_EDGE) &&
					!HasAtMostOneBit(GetAllRoadBits(tile))) {
				if (GetFoundationSlope(tile) == SLOPE_FLAT && EnsureNoVehicleOnGround(tile).Succeeded() && Chance16(1, 40)) {
					StartRoadWorks(tile);

					if (_settings_client.sound.ambient) SndPlayTileFx(SND_21_JACKHAMMER, tile);
					CreateEffectVehicleAbove(
						TileX(tile) * TILE_SIZE + 7,
						TileY(tile) * TILE_SIZE + 7,
						0,
						EV_BULLDOZER);
					MarkTileDirtyByTile(tile);
					return;
				}
			}
		}

		UpdateRoadSide(tile, grp);
	} else if (IncreaseRoadWorksCounter(tile)) {
		TerminateRoadWorks(tile);

		if (_settings_game.economy.mod_road_rebuild) {
			/* Generate a nicer town surface */
			const RoadBits old_rb = GetAnyRoadBits(tile, ROADTYPE_ROAD);
			const RoadBits new_rb = CleanUpRoadBits(tile, old_rb);

			if (old_rb != new_rb) {
				RemoveRoad(tile, DC_EXEC | DC_AUTO | DC_NO_WATER, (old_rb ^ new_rb), ROADTYPE_ROAD, true);
			}
		}

		MarkTileDirtyByTile(tile);
	}
}

static bool ClickTile_Road(TileIndex tile)
{
	return false;
}

static TrackStatus GetTileTrackStatus_Road(TileIndex tile, TransportType mode, uint sub_mode, DiagDirection side)
{
	/* Converts RoadBits to TrackdirBits */
	static const TrackdirBits road_trackdirbits[16] = {
		TRACKDIR_BIT_NONE,                           // ROAD_NONE
		TRACKDIR_BIT_NONE,                           // ROAD_NW
		TRACKDIR_BIT_NONE,                           // ROAD_SW
		TRACKDIR_BIT_LEFT_S | TRACKDIR_BIT_LEFT_N,   // ROAD_W
		TRACKDIR_BIT_NONE,                           // ROAD_SE
		TRACKDIR_BIT_Y_SE | TRACKDIR_BIT_Y_NW,       // ROAD_Y
		TRACKDIR_BIT_LOWER_E | TRACKDIR_BIT_LOWER_W, // ROAD_S
		TRACKDIR_BIT_LEFT_S | TRACKDIR_BIT_LOWER_E | TRACKDIR_BIT_Y_SE
			| TRACKDIR_BIT_LEFT_N | TRACKDIR_BIT_LOWER_W | TRACKDIR_BIT_Y_NW,  // ROAD_Y | ROAD_SW
		TRACKDIR_BIT_NONE,                           // ROAD_NE
		TRACKDIR_BIT_UPPER_E | TRACKDIR_BIT_UPPER_W, // ROAD_N
		TRACKDIR_BIT_X_NE | TRACKDIR_BIT_X_SW,       // ROAD_X
		TRACKDIR_BIT_LEFT_S | TRACKDIR_BIT_UPPER_E | TRACKDIR_BIT_X_NE
			| TRACKDIR_BIT_LEFT_N | TRACKDIR_BIT_UPPER_W | TRACKDIR_BIT_X_SW,  // ROAD_X | ROAD_NW
		TRACKDIR_BIT_RIGHT_S | TRACKDIR_BIT_RIGHT_N, // ROAD_E
		TRACKDIR_BIT_RIGHT_S | TRACKDIR_BIT_UPPER_E | TRACKDIR_BIT_Y_SE
			| TRACKDIR_BIT_RIGHT_N | TRACKDIR_BIT_UPPER_W | TRACKDIR_BIT_Y_NW, // ROAD_Y | ROAD_NE
		TRACKDIR_BIT_RIGHT_S | TRACKDIR_BIT_LOWER_E | TRACKDIR_BIT_X_NE
			| TRACKDIR_BIT_RIGHT_N | TRACKDIR_BIT_LOWER_W | TRACKDIR_BIT_X_SW, // ROAD_X | ROAD_SE
		TRACKDIR_BIT_MASK,                           // ROAD_ALL
	};

	TrackdirBits trackdirbits;

	if (IsTileSubtype(tile, TT_TRACK)) {
		static const uint drd_mask[DRD_END] = { 0xFFFF, 0xFF00, 0xFF, 0x0 };

		if ((GetRoadTypes(tile) & sub_mode) == 0) return 0;

		RoadType rt = (RoadType)FindFirstBit(sub_mode);
		RoadBits bits = GetRoadBits(tile, rt);

		/* no roadbit at this side of tile, return 0 */
		if (side != INVALID_DIAGDIR && (DiagDirToRoadBits(side) & bits) == 0) return 0;

		if (HasRoadWorks(tile)) {
			trackdirbits = TRACKDIR_BIT_NONE;
		} else {
			trackdirbits = road_trackdirbits[bits];
			if (rt == ROADTYPE_ROAD) trackdirbits &= (TrackdirBits)drd_mask[GetDisallowedRoadDirections(tile)];
		}
	} else {
		if (mode != TRANSPORT_ROAD || ((GetRoadTypes(tile) & sub_mode) == 0)) return 0;

		DiagDirection dir = GetTunnelBridgeDirection(tile);
		if (side != INVALID_DIAGDIR && side != ReverseDiagDir(dir)) return 0;
		trackdirbits = TrackBitsToTrackdirBits(DiagDirToDiagTrackBits(dir));
	}

	return CombineTrackStatus(trackdirbits, TRACKDIR_BIT_NONE);
}

static const StringID _road_tile_strings[] = {
	STR_LAI_ROAD_DESCRIPTION_ROAD,
	STR_LAI_ROAD_DESCRIPTION_ROAD,
	STR_LAI_ROAD_DESCRIPTION_ROAD,
	STR_LAI_ROAD_DESCRIPTION_ROAD_WITH_STREETLIGHTS,
	STR_LAI_ROAD_DESCRIPTION_ROAD,
	STR_LAI_ROAD_DESCRIPTION_TREE_LINED_ROAD,
	STR_LAI_ROAD_DESCRIPTION_ROAD,
	STR_LAI_ROAD_DESCRIPTION_ROAD,
};

static void GetTileDesc_Road(TileIndex tile, TileDesc *td)
{
	RoadTypes rts = GetRoadTypes(tile);

	Owner tram_owner = INVALID_OWNER;
	if (HasBit(rts, ROADTYPE_TRAM)) tram_owner = GetRoadOwner(tile, ROADTYPE_TRAM);

	if (IsTileSubtype(tile, TT_TRACK)) {
		if (!HasBit(rts, ROADTYPE_ROAD)) {
			td->str = STR_LAI_ROAD_DESCRIPTION_TRAMWAY;
			td->owner[0] = tram_owner;
			return;
		}
		td->str = _road_tile_strings[GetRoadside(tile)];
	} else {
		td->str = GetBridgeSpec(GetRoadBridgeType(tile))->transport_name[TRANSPORT_ROAD];
		if (!HasBit(rts, ROADTYPE_ROAD)) {
			td->owner[0] = tram_owner;
			return;
		}
	}

	/* So the tile at least has a road; check if it has both road and tram */
	Owner road_owner = GetRoadOwner(tile, ROADTYPE_ROAD);

	if (HasBit(rts, ROADTYPE_TRAM)) {
		td->owner_type[0] = STR_LAND_AREA_INFORMATION_ROAD_OWNER;
		td->owner[0] = road_owner;
		td->owner_type[1] = STR_LAND_AREA_INFORMATION_TRAM_OWNER;
		td->owner[1] = tram_owner;
	} else {
		/* One to rule them all */
		td->owner[0] = road_owner;
	}
}


static void ChangeTileOwner_Road(TileIndex tile, Owner old_owner, Owner new_owner)
{
	Company *oldc = Company::Get(old_owner);

	Company *newc;
	if (new_owner != INVALID_OWNER) {
		newc = Company::Get(new_owner);
	} else {
		new_owner = OWNER_NONE;
		newc = NULL;
	}

	if (IsTileSubtype(tile, TT_TRACK)) {
		for (RoadType rt = ROADTYPE_ROAD; rt < ROADTYPE_END; rt++) {
			/* Update all roadtypes, no matter if they are present */
			if (GetRoadOwner(tile, rt) == old_owner) {
				if (HasTileRoadType(tile, rt)) {
					/* No need to dirty windows here, we'll redraw the whole screen anyway. */
					uint num_bits = CountBits(GetRoadBits(tile, rt));
					oldc->infrastructure.road[rt] -= num_bits;
					if (newc != NULL) newc->infrastructure.road[rt] += num_bits;
				}

				SetRoadOwner(tile, rt, new_owner);
			}
		}
	} else {
		TileIndex other_end = GetOtherBridgeEnd(tile);
		/* Set number of pieces to zero if it's the southern tile as we
		 * don't want to update the infrastructure counts twice. */
		uint num_pieces = tile < other_end ? (GetTunnelBridgeLength(tile, other_end) + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR * 2 : 0;

		for (RoadType rt = ROADTYPE_ROAD; rt < ROADTYPE_END; rt++) {
			/* Update all roadtypes, no matter if they are present */
			if (GetRoadOwner(tile, rt) == old_owner) {
				if (HasBit(GetRoadTypes(tile), rt)) {
					/* Update company infrastructure counts. A full diagonal road tile has two road bits.
					 * No need to dirty windows here, we'll redraw the whole screen anyway. */
					oldc->infrastructure.road[rt] -= num_pieces;
					if (newc != NULL) newc->infrastructure.road[rt] += num_pieces;
				}

				SetRoadOwner(tile, rt, new_owner);
			}
		}

		if (IsTileOwner(tile, old_owner)) SetTileOwner(tile, new_owner);
	}
}

static CommandCost TerraformTile_Road(TileIndex tile, DoCommandFlag flags, int z_new, Slope tileh_new)
{
	if (_settings_game.construction.build_on_slopes && AutoslopeEnabled()) {
		if (IsTileSubtype(tile, TT_TRACK)) {
			RoadBits bits = GetAllRoadBits(tile);
			RoadBits bits_new;
			/* Check if the slope-road_bits combination is valid at all, i.e. it is safe to call GetRoadFoundation(). */
			if (CheckRoadSlope(tileh_new, bits, ROAD_NONE, ROAD_NONE, &bits_new).Succeeded()) {
				if (bits == bits_new) {
					int z_old;
					Slope tileh_old = GetTileSlope(tile, &z_old);

					/* Get the slope on top of the foundation */
					z_old += ApplyFoundationToSlope(GetRoadFoundation(tileh_old, bits), &tileh_old);
					z_new += ApplyFoundationToSlope(GetRoadFoundation(tileh_new, bits), &tileh_new);

					/* The surface slope must not be changed */
					if ((z_old == z_new) && (tileh_old == tileh_new)) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
				}
			}
		} else {
			int z_old;
			Slope tileh_old = GetTileSlope(tile, &z_old);

			DiagDirection direction = GetTunnelBridgeDirection(tile);

			/* Check if new slope is valid for bridges in general (so we can safely call GetBridgeFoundation()) */
			CheckBridgeSlope(direction, &tileh_old, &z_old);
			CommandCost res = CheckBridgeSlope(direction, &tileh_new, &z_new);

			/* Surface slope is valid and remains unchanged? */
			if (res.Succeeded() && (z_old == z_new) && (tileh_old == tileh_new)) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
		}
	}

	return DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
}

/** Tile callback functions for road tiles */
extern const TileTypeProcs _tile_type_road_procs = {
	DrawTile_Road,           // draw_tile_proc
	GetSlopePixelZ_Road,     // get_slope_z_proc
	ClearTile_Road,          // clear_tile_proc
	NULL,                    // add_accepted_cargo_proc
	GetTileDesc_Road,        // get_tile_desc_proc
	GetTileTrackStatus_Road, // get_tile_track_status_proc
	ClickTile_Road,          // click_tile_proc
	NULL,                    // animate_tile_proc
	TileLoop_Road,           // tile_loop_proc
	ChangeTileOwner_Road,    // change_tile_owner_proc
	NULL,                    // add_produced_cargo_proc
	GetFoundation_Road,      // get_foundation_proc
	TerraformTile_Road,      // terraform_tile_proc
};
