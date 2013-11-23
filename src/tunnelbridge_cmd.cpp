/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file tunnelbridge_cmd.cpp
 * This file deals with tunnels and bridges (non-gui stuff)
 * @todo separate this file into two
 */

#include "stdafx.h"
#include "newgrf_object.h"
#include "viewport_func.h"
#include "cmd_helper.h"
#include "command_func.h"
#include "town.h"
#include "train.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "autoslope.h"
#include "map/tunnelbridge.h"
#include "map/water.h"
#include "map/road.h"
#include "strings_func.h"
#include "tunnelbridge.h"
#include "bridge.h"
#include "signal_func.h"
#include "cheat_type.h"
#include "company_base.h"
#include "object_base.h"
#include "company_gui.h"

#include "table/strings.h"

TileIndex _build_tunnel_endtile; ///< The end of a tunnel; as hidden return from the tunnel build command for GUI purposes.


extern bool IsValidRoadBridgeBits(Slope tileh, DiagDirection dir, RoadBits bits);

/**
 * Build a road bridge
 * @param tile_start Start tile
 * @param tile_end End tile
 * @param bridge_type Bridge type
 * @param rts Road types
 * @param town the town that is building the road (if applicable)
 * @param flags type of operation
 * @return the cost of this operation or an error
 */
static CommandCost BuildRoadBridge(TileIndex tile_start, TileIndex tile_end, BridgeType bridge_type, RoadTypes rts, TownID town, DoCommandFlag flags)
{
	CompanyID company = _current_company;

	if (!HasExactlyOneBit(rts) || !HasRoadTypesAvail(_current_company, rts)) return CMD_ERROR;

	if (company == OWNER_DEITY) {
		const Town *t = CalcClosestTownFromTile(tile_start);

		/* If we are not within a town, we are not owned by the town */
		if (t == NULL || DistanceSquare(tile_start, t->xy) > t->cache.squared_town_zone_radius[HZB_TOWN_EDGE]) {
			company = OWNER_NONE;
		} else {
			company = OWNER_TOWN;
			town = t->index;
		}
	} else if (company == OWNER_TOWN) {
		if (!Town::IsValidID(town)) return CMD_ERROR;
	} else {
		town = INVALID_TOWN;
	}

	Axis direction;
	CommandCost ret = CheckBridgeTiles(tile_start, tile_end, &direction);
	if (ret.Failed()) return ret;

	if (tile_end < tile_start) Swap(tile_start, tile_end);

	/* set and test bridge length, availability */
	uint bridge_len = GetTunnelBridgeLength(tile_start, tile_end);
	ret = CheckBridgeAvailability(bridge_type, bridge_len, flags);
	if (ret.Failed()) return ret;

	CommandCost cost(EXPENSES_CONSTRUCTION);

	if (IsRoadBridgeTile(tile_start) && IsRoadBridgeTile(tile_end) &&
			GetOtherBridgeEnd(tile_start) == tile_end) {
		/* Replace a current bridge. */

		/* Special owner check. */
		Owner owner_road = HasTileRoadType(tile_start, ROADTYPE_ROAD) ? GetRoadOwner(tile_start, ROADTYPE_ROAD) : INVALID_OWNER;
		Owner owner_tram = HasTileRoadType(tile_start, ROADTYPE_TRAM) ? GetRoadOwner(tile_start, ROADTYPE_TRAM) : INVALID_OWNER;

		/* You must own one of the roadtypes, or one of the roadtypes must be unowned, or a town must own the road. */
		if (owner_road != company && owner_road != OWNER_NONE && owner_road != OWNER_TOWN &&
				owner_tram != company && owner_tram != OWNER_NONE) {
			return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
		}

		/* You must own all of the roadtypes if downgrading. */
		if (!(flags & DC_QUERY_COST) &&
				GetBridgeSpec(bridge_type)->speed < GetBridgeSpec(GetRoadBridgeType(tile_start))->speed &&
				_game_mode != GM_EDITOR) {
			if (owner_road == OWNER_TOWN) {
				Town *t = ClosestTownFromTile(tile_start, UINT_MAX);

				if (t == NULL) return CMD_ERROR;

				SetDParam(0, t->index);
				return_cmd_error(STR_ERROR_LOCAL_AUTHORITY_REFUSES_TO_ALLOW_THIS);
			}
			if (owner_road != company && owner_road != OWNER_NONE && owner_road != INVALID_OWNER) {
				return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
			}
			if (owner_tram != company && owner_tram != OWNER_NONE && owner_tram != INVALID_OWNER) {
				return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
			}
		}

		/* Do not remove road types when upgrading a bridge */
		rts |= GetRoadTypes(tile_start);

		/* Do not replace the bridge with the same bridge type. */
		if (!(flags & DC_QUERY_COST) && (bridge_type == GetRoadBridgeType(tile_start)) && ((rts & ~GetRoadTypes(tile_start)) == 0)) {
			return_cmd_error(STR_ERROR_ALREADY_BUILT);
		}

		cost.AddCost((bridge_len + 1) * _price[PR_CLEAR_BRIDGE]); // The cost of clearing the current bridge.

		/* do the drill? */
		if (flags & DC_EXEC) {
			RoadTypes prev_roadtypes = GetRoadTypes(tile_start);
			/* Also give unowned present roadtypes to new owner */
			if (HasBit(prev_roadtypes, ROADTYPE_ROAD) && GetRoadOwner(tile_start, ROADTYPE_ROAD) == OWNER_NONE) ClrBit(prev_roadtypes, ROADTYPE_ROAD);
			if (HasBit(prev_roadtypes, ROADTYPE_TRAM) && GetRoadOwner(tile_start, ROADTYPE_TRAM) == OWNER_NONE) ClrBit(prev_roadtypes, ROADTYPE_TRAM);
			Company *c = Company::GetIfValid(company);
			if (c != NULL) {
				/* Add all new road types to the company infrastructure counter. */
				RoadType new_rt;
				FOR_EACH_SET_ROADTYPE(new_rt, rts ^ prev_roadtypes) {
					/* A full diagonal road tile has two road bits. */
					c->infrastructure.road[new_rt] += (bridge_len + 2) * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
				}
			}
			DirtyCompanyInfrastructureWindows(company);

			SetRoadBridgeType(tile_start, bridge_type);
			SetRoadTypes(tile_start, rts);
			SetRoadBridgeType(tile_end,   bridge_type);
			SetRoadTypes(tile_end,   rts);

			for (RoadType rt = ROADTYPE_BEGIN; rt < ROADTYPE_END; rt++) {
				if (!HasBit(prev_roadtypes, rt)) {
					SetRoadOwner(tile_start, rt, company);
					SetRoadOwner(tile_end,   rt, company);
				}
			}

			MarkBridgeTilesDirty(tile_start, tile_end, AxisToDiagDir(direction));
		}
	} else {
		DiagDirection dir = AxisToDiagDir(direction);

		bool clear_start = !IsNormalRoadTile(tile_start) ||
			GetDisallowedRoadDirections(tile_start) != DRD_NONE ||
			HasRoadWorks(tile_start) ||
			!IsValidRoadBridgeBits(GetTileSlope(tile_start), dir, GetAllRoadBits(tile_start));

		if (!clear_start) {
			RoadType rt;
			FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile_start) & rts) {
				if (GetRoadOwner(tile_start, rt) != company) clear_start = true;
			}
		}

		bool clear_end = !IsNormalRoadTile(tile_end) ||
			GetDisallowedRoadDirections(tile_end) != DRD_NONE ||
			HasRoadWorks(tile_end) ||
			!IsValidRoadBridgeBits(GetTileSlope(tile_end), ReverseDiagDir(dir), GetAllRoadBits(tile_end));

		if (!clear_end) {
			RoadType rt;
			FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile_end) & rts) {
				if (GetRoadOwner(tile_end, rt) != company) clear_end = true;
			}
		}

		/* Build a new bridge. */
		CommandCost ret = CheckBridgeBuildable(tile_start, tile_end, flags, clear_start, clear_end);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);

		/* do the drill? */
		if (flags & DC_EXEC) {
			Company *c = Company::GetIfValid(company);
			uint num_pieces = 2 * bridge_len;

			if (clear_start) {
				MakeRoadBridgeRamp(tile_start, company, company, bridge_type, dir, rts, town != INVALID_TOWN ? town : CalcClosestTownIDFromTile(tile_start));
				num_pieces += 2;
			} else {
				MakeRoadBridgeFromRoad(tile_start, bridge_type, dir);
				SetRoadTypes(tile_start, GetRoadTypes(tile_start) | rts);

				RoadBits bits = DiagDirToRoadBits(dir);
				RoadType new_rt;
				FOR_EACH_SET_ROADTYPE(new_rt, rts) {
					RoadBits pieces = GetRoadBits(tile_start, new_rt);
					uint n = CountBits(pieces);
					if (c != NULL) c->infrastructure.road[new_rt] += (n + 1) * TUNNELBRIDGE_TRACKBIT_FACTOR - n;
					SetRoadBits(tile_start, pieces | bits, new_rt);
				}
			}

			if (clear_end) {
				MakeRoadBridgeRamp(tile_end, company, company, bridge_type, ReverseDiagDir(dir), rts, town != INVALID_TOWN ? town : CalcClosestTownIDFromTile(tile_end));
				num_pieces += 2;
			} else {
				MakeRoadBridgeFromRoad(tile_end, bridge_type, ReverseDiagDir(dir));
				SetRoadTypes(tile_end, GetRoadTypes(tile_end) | rts);

				RoadBits bits = DiagDirToRoadBits(ReverseDiagDir(dir));
				RoadType new_rt;
				FOR_EACH_SET_ROADTYPE(new_rt, rts) {
					RoadBits pieces = GetRoadBits(tile_end, new_rt);
					uint n = CountBits(pieces);
					if (c != NULL) c->infrastructure.road[new_rt] += (n + 1) * TUNNELBRIDGE_TRACKBIT_FACTOR - n;
					SetRoadBits(tile_end, pieces | bits, new_rt);
				}
			}

			num_pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
			if (c != NULL) {
				/* Add all new road types to the company infrastructure counter. */
				RoadType new_rt;
				FOR_EACH_SET_ROADTYPE(new_rt, rts) {
					/* A full diagonal road tile has two road bits. */
					c->infrastructure.road[new_rt] += num_pieces;
				}
			}

			SetBridgeMiddleTiles(tile_start, tile_end, direction);
			DirtyCompanyInfrastructureWindows(company);
		}
	}

	/* for human player that builds the bridge he gets a selection to choose from bridges (DC_QUERY_COST)
	 * It's unnecessary to execute this command every time for every bridge. So it is done only
	 * and cost is computed in "bridge_gui.c". For AI, Towns this has to be of course calculated
	 */
	Company *c = Company::GetIfValid(company);
	if (!(flags & DC_QUERY_COST) || (c != NULL && c->is_ai)) {
		bridge_len += 2; // begin and end tiles/ramps

		cost.AddCost(bridge_len * _price[PR_BUILD_ROAD] * 2 * CountBits(rts));

		if (c != NULL) bridge_len = CalcBridgeLenCostFactor(bridge_len);

		cost.AddCost((int64)bridge_len * _price[PR_BUILD_BRIDGE] * GetBridgeSpec(bridge_type)->price >> 8);
	}

	return cost;
}


/**
 * Trackbits to add to a rail tile if a bridge is built on it, or TRACK_BIT_NONE if tile must be cleared
 * @param tile tile to check
 * @param dir direction for bridge
 * @param rt railtype for bridge
 * @return trackbits to add, or TRACK_BIT_NONE if tile must be cleared
 */
static TrackBits RailBridgeAddBits(TileIndex tile, DiagDirection dir, RailType rt)
{
	if (!IsNormalRailTile(tile) || !IsTileOwner(tile, _current_company)) return TRACK_BIT_NONE;

	Slope tileh = GetTileSlope(tile);
	DiagDirDiff diff = CheckExtendedBridgeHead(tileh, dir);
	TrackBits present = GetTrackBits(tile);

	switch (diff) {
		case DIAGDIRDIFF_REVERSE: return TRACK_BIT_NONE;

		case DIAGDIRDIFF_SAME: {
			TrackBits bridgebits = DiagdirReachesTracks(ReverseDiagDir(dir));
			if ((present & bridgebits) != TRACK_BIT_NONE) return TRACK_BIT_NONE;

			Track track = FindFirstTrack(present);

			if (KillFirstBit(present) != TRACK_BIT_NONE) {
				return (GetRailType(tile, track) == rt) ? bridgebits : TRACK_BIT_NONE;
			} else if (GetRailType(tile, track) == rt && !HasSignalOnTrack(tile, track)) {
				return bridgebits & ~TrackToTrackBits(TrackToOppositeTrack(track));
			} else if (IsDiagonalTrack(track)) {
				return TRACK_BIT_NONE;
			} else {
				return bridgebits & TrackToTrackBits(TrackToOppositeTrack(track));
			}
		}

		default: {
			TrackBits allowed = TRACK_BIT_ALL & ~DiagdirReachesTracks(ReverseDiagDir(ChangeDiagDir(dir, diff)));
			if (present != (allowed & ~DiagdirReachesTracks(ReverseDiagDir(dir)))) {
				return TRACK_BIT_NONE;
			}

			Track track = FindFirstTrack(present);
			assert(present == TrackToTrackBits(track)); // present should have exactly one trackbit

			if (HasSignalOnTrack(tile, track) || GetRailType(tile, track) != rt) return TRACK_BIT_NONE;

			return allowed ^ present;
		}
	}
}

/**
 * Build a rail bridge
 * @param tile_start Start tile
 * @param tile_end End tile
 * @param bridge_type Bridge type
 * @param railtype Rail type
 * @param flags type of operation
 * @return the cost of this operation or an error
 */
static CommandCost BuildRailBridge(TileIndex tile_start, TileIndex tile_end, BridgeType bridge_type, RailType railtype, DoCommandFlag flags)
{
	if (!ValParamRailtype(railtype)) return CMD_ERROR;

	if (_current_company == OWNER_DEITY) return CMD_ERROR;

	Axis direction;
	CommandCost ret = CheckBridgeTiles(tile_start, tile_end, &direction);
	if (ret.Failed()) return ret;

	if (tile_end < tile_start) Swap(tile_start, tile_end);

	/* set and test bridge length, availability */
	uint bridge_len = GetTunnelBridgeLength(tile_start, tile_end);
	ret = CheckBridgeAvailability(bridge_type, bridge_len, flags);
	if (ret.Failed()) return ret;

	CommandCost cost(EXPENSES_CONSTRUCTION);

	if (IsRailBridgeTile(tile_start) && IsRailBridgeTile(tile_end) &&
			GetOtherBridgeEnd(tile_start) == tile_end) {
		/* Replace a current bridge. */

		/* Make sure the railtypes match. */
		if (GetBridgeRailType(tile_start) != railtype) {
			return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
		}

		/* Do not replace the bridge with the same bridge type. */
		if (!(flags & DC_QUERY_COST) && bridge_type == GetRailBridgeType(tile_start)) {
			return_cmd_error(STR_ERROR_ALREADY_BUILT);
		}

		/* Do not allow replacing another company's bridges. */
		if (!IsTileOwner(tile_start, _current_company) && !IsTileOwner(tile_start, OWNER_TOWN)) {
			return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
		}

		cost.AddCost((bridge_len + 1) * _price[PR_CLEAR_BRIDGE]); // The cost of clearing the current bridge.

		/* do the drill? */
		if (flags & DC_EXEC) {
			SetRailBridgeType(tile_start, bridge_type);
			SetRailBridgeType(tile_end,   bridge_type);

			MarkBridgeTilesDirty(tile_start, tile_end, AxisToDiagDir(direction));
		}
	} else {
		DiagDirection dir = AxisToDiagDir(direction);

		TrackBits add_start = RailBridgeAddBits(tile_start, dir, railtype);
		TrackBits add_end   = RailBridgeAddBits(tile_end,   ReverseDiagDir(dir), railtype);

		/* Build a new bridge. */
		CommandCost ret = CheckBridgeBuildable(tile_start, tile_end, flags, add_start == TRACK_BIT_NONE, add_end == TRACK_BIT_NONE);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);

		/* do the drill? */
		if (flags & DC_EXEC) {
			/* Add to company infrastructure count if building a new bridge. */
			Company *c = Company::Get(_current_company);
			uint pieces = bridge_len;

			if (add_start == TRACK_BIT_NONE) {
				MakeRailBridgeRamp(tile_start, _current_company, bridge_type, dir, railtype);
				pieces++;
			} else {
				TrackBits bits = GetTrackBits(tile_start);
				MakeRailBridgeFromRail(tile_start, bridge_type, dir);
				SetTrackBits(tile_start, bits | add_start);
				SetRailType(tile_start, railtype, FindFirstTrack(add_start));

				if (HasExactlyOneBit(add_start)) {
					pieces++;
				} else {
					uint n = CountBits(bits);
					c->infrastructure.rail[railtype] -= n * n;
					n = CountBits(bits | add_start);
					pieces += n * n;
				}
			}

			if (add_end == TRACK_BIT_NONE) {
				MakeRailBridgeRamp(tile_end, _current_company, bridge_type, ReverseDiagDir(dir), railtype);
				pieces++;
			} else {
				TrackBits bits = GetTrackBits(tile_end);
				MakeRailBridgeFromRail(tile_end, bridge_type, ReverseDiagDir(dir));
				SetTrackBits(tile_end, bits | add_end);
				SetRailType(tile_end, railtype, FindFirstTrack(add_end));

				if (HasExactlyOneBit(add_end)) {
					pieces++;
				} else {
					uint n = CountBits(bits);
					c->infrastructure.rail[railtype] -= n * n;
					n = CountBits(bits | add_end);
					pieces += n * n;
				}
			}

			c->infrastructure.rail[railtype] += pieces * TUNNELBRIDGE_TRACKBIT_FACTOR;
			DirtyCompanyInfrastructureWindows(_current_company);

			SetBridgeMiddleTiles(tile_start, tile_end, direction);
			DirtyCompanyInfrastructureWindows(_current_company);
		}
	}

	if (flags & DC_EXEC) {
		Track track = AxisToTrack(direction);
		AddBridgeToSignalBuffer(tile_start, _current_company);
		YapfNotifyTrackLayoutChange(tile_start, track);
	}

	/* for human player that builds the bridge he gets a selection to choose from bridges (DC_QUERY_COST)
	 * It's unnecessary to execute this command every time for every bridge. So it is done only
	 * and cost is computed in "bridge_gui.c". For AI, Towns this has to be of course calculated
	 */
	Company *c = Company::Get(_current_company);
	if (!(flags & DC_QUERY_COST) || c->is_ai) {
		bridge_len += 2; // begin and end tiles/ramps

		cost.AddCost(bridge_len * RailBuildCost(railtype));

		bridge_len = CalcBridgeLenCostFactor(bridge_len);

		cost.AddCost((int64)bridge_len * _price[PR_BUILD_BRIDGE] * GetBridgeSpec(bridge_type)->price >> 8);
	}

	return cost;
}


/**
 * Build an aqueduct
 * @param tile_start Start tile
 * @param tile_end End tile
 * @param flags type of operation
 * @return the cost of this operation or an error
 */
static CommandCost BuildAqueduct(TileIndex tile_start, TileIndex tile_end, DoCommandFlag flags)
{
	if (_current_company == OWNER_DEITY) return CMD_ERROR;

	Axis direction;
	CommandCost ret = CheckBridgeTiles(tile_start, tile_end, &direction);
	if (ret.Failed()) return ret;

	if (tile_end < tile_start) Swap(tile_start, tile_end);

	/* set and test bridge length, availability */
	uint bridge_len = GetTunnelBridgeLength(tile_start, tile_end);
	if (bridge_len > _settings_game.construction.max_bridge_length) return_cmd_error(STR_ERROR_BRIDGE_TOO_LONG);

	CommandCost cost(EXPENSES_CONSTRUCTION);

	if (IsAqueductTile(tile_start) && IsAqueductTile(tile_end) &&
			GetOtherBridgeEnd(tile_start) == tile_end) {
		return_cmd_error(STR_ERROR_ALREADY_BUILT);
	}

	/* Build a new bridge. */
	ret = CheckBridgeBuildable(tile_start, tile_end, flags, true, true, true);
	if (ret.Failed()) return ret;
	cost.AddCost(ret);

	/* do the drill? */
	if (flags & DC_EXEC) {
		DiagDirection dir = AxisToDiagDir(direction);

		Company *c = Company::GetIfValid(_current_company);
		if (c != NULL) c->infrastructure.water += (bridge_len + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
		MakeAqueductBridgeRamp(tile_start, _current_company, dir);
		MakeAqueductBridgeRamp(tile_end,   _current_company, ReverseDiagDir(dir));

		SetBridgeMiddleTiles(tile_start, tile_end, direction);
		DirtyCompanyInfrastructureWindows(_current_company);
	}

	/* for human player that builds the bridge he gets a selection to choose from bridges (DC_QUERY_COST)
	 * It's unnecessary to execute this command every time for every bridge. So it is done only
	 * and cost is computed in "bridge_gui.c". For AI, Towns this has to be of course calculated
	 */
	Company *c = Company::GetIfValid(_current_company);
	if (!(flags & DC_QUERY_COST) || (c != NULL && c->is_ai)) {
		bridge_len += 2; // begin and end tiles/ramps

		if (c != NULL) bridge_len = CalcBridgeLenCostFactor(bridge_len);

		/* Aqueducts use a separate base cost. */
		cost.AddCost((int64)bridge_len * _price[PR_BUILD_AQUEDUCT]);
	}

	return cost;
}


/**
 * Build a Bridge
 * @param end_tile end tile
 * @param flags type of operation
 * @param p1 packed start tile coords (~ dx)
 * @param p2 various bitstuffed elements
 * - p2 = (bit  0- 7) - bridge type (hi bh)
 * - p2 = (bit  8-11) - rail type or road types.
 * - p2 = (bit 12-13) - transport type.
 * - p2 = (bit 16-31) - the town that is building the road (if applicable)
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildBridge(TileIndex end_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	/* unpack parameters */
	BridgeType bridge_type = GB(p2, 0, 8);

	if (!IsValidTile(p1)) return_cmd_error(STR_ERROR_BRIDGE_THROUGH_MAP_BORDER);

	/* type of bridge */
	switch (Extract<TransportType, 12, 2>(p2)) {
		case TRANSPORT_ROAD:
			return BuildRoadBridge(p1, end_tile, bridge_type, Extract<RoadTypes, 8, 2>(p2), GB(p2, 16, 16), flags);

		case TRANSPORT_RAIL:
			return BuildRailBridge(p1, end_tile, bridge_type, Extract<RailType, 8, 4>(p2), flags);

		case TRANSPORT_WATER:
			return BuildAqueduct(p1, end_tile, flags);

		default:
			/* Airports don't have bridges. */
			return CMD_ERROR;
	}
}


/**
 * Build Tunnel.
 * @param start_tile start tile of tunnel
 * @param flags type of operation
 * @param p1 bit 0-3 railtype or roadtypes
 *           bit 8-9 transport type
 * @param p2 unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildTunnel(TileIndex start_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	CompanyID company = _current_company;

	TransportType transport_type = Extract<TransportType, 8, 2>(p1);

	RailType railtype = INVALID_RAILTYPE;
	RoadTypes rts = ROADTYPES_NONE;
	_build_tunnel_endtile = 0;
	switch (transport_type) {
		case TRANSPORT_RAIL:
			railtype = Extract<RailType, 0, 4>(p1);
			if (!ValParamRailtype(railtype)) return CMD_ERROR;
			break;

		case TRANSPORT_ROAD:
			rts = Extract<RoadTypes, 0, 2>(p1);
			if (!HasExactlyOneBit(rts) || !HasRoadTypesAvail(company, rts)) return CMD_ERROR;
			break;

		default: return CMD_ERROR;
	}

	if (company == OWNER_DEITY) {
		if (transport_type != TRANSPORT_ROAD) return CMD_ERROR;
		const Town *town = CalcClosestTownFromTile(start_tile);

		company = OWNER_TOWN;

		/* If we are not within a town, we are not owned by the town */
		if (town == NULL || DistanceSquare(start_tile, town->xy) > town->cache.squared_town_zone_radius[HZB_TOWN_EDGE]) {
			company = OWNER_NONE;
		}
	}

	int start_z;
	int end_z;
	Slope start_tileh = GetTileSlope(start_tile, &start_z);
	DiagDirection direction = GetInclinedSlopeDirection(start_tileh);
	if (direction == INVALID_DIAGDIR) return_cmd_error(STR_ERROR_SITE_UNSUITABLE_FOR_TUNNEL);

	if (HasTileWaterGround(start_tile)) return_cmd_error(STR_ERROR_CAN_T_BUILD_ON_WATER);

	CommandCost ret = DoCommand(start_tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (ret.Failed()) return ret;

	/* XXX - do NOT change 'ret' in the loop, as it is used as the price
	 * for the clearing of the entrance of the tunnel. Assigning it to
	 * cost before the loop will yield different costs depending on start-
	 * position, because of increased-cost-by-length: 'cost += cost >> 3' */

	TileIndexDiff delta = TileOffsByDiagDir(direction);
	DiagDirection tunnel_in_way_dir;
	if (DiagDirToAxis(direction) == AXIS_Y) {
		tunnel_in_way_dir = (TileX(start_tile) < (MapMaxX() / 2)) ? DIAGDIR_SW : DIAGDIR_NE;
	} else {
		tunnel_in_way_dir = (TileY(start_tile) < (MapMaxX() / 2)) ? DIAGDIR_SE : DIAGDIR_NW;
	}

	TileIndex end_tile = start_tile;

	/* Tile shift coefficient. Will decrease for very long tunnels to avoid exponential growth of price*/
	int tiles_coef = 3;
	/* Number of tiles from start of tunnel */
	int tiles = 0;
	/* Number of tiles at which the cost increase coefficient per tile is halved */
	int tiles_bump = 25;

	CommandCost cost(EXPENSES_CONSTRUCTION);
	Slope end_tileh;
	for (;;) {
		end_tile += delta;
		if (!IsValidTile(end_tile)) return_cmd_error(STR_ERROR_TUNNEL_THROUGH_MAP_BORDER);
		end_tileh = GetTileSlope(end_tile, &end_z);

		if (start_z == end_z) break;

		if (!_cheats.crossing_tunnels.value && IsTunnelInWayDir(end_tile, start_z, tunnel_in_way_dir)) {
			return_cmd_error(STR_ERROR_ANOTHER_TUNNEL_IN_THE_WAY);
		}

		tiles++;
		if (tiles == tiles_bump) {
			tiles_coef++;
			tiles_bump *= 2;
		}

		cost.AddCost(_price[PR_BUILD_TUNNEL]);
		cost.AddCost(cost.GetCost() >> tiles_coef); // add a multiplier for longer tunnels
	}

	/* Add the cost of the entrance */
	cost.AddCost(_price[PR_BUILD_TUNNEL]);
	cost.AddCost(ret);

	/* if the command fails from here on we want the end tile to be highlighted */
	_build_tunnel_endtile = end_tile;

	if (tiles > _settings_game.construction.max_tunnel_length) return_cmd_error(STR_ERROR_TUNNEL_TOO_LONG);

	if (HasTileWaterGround(end_tile)) return_cmd_error(STR_ERROR_CAN_T_BUILD_ON_WATER);

	/* Clear the tile in any case */
	ret = DoCommand(end_tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	if (ret.Failed()) return_cmd_error(STR_ERROR_UNABLE_TO_EXCAVATE_LAND);
	cost.AddCost(ret);

	/* slope of end tile must be complementary to the slope of the start tile */
	if (end_tileh != ComplementSlope(start_tileh)) {
		/* Mark the tile as already cleared for the terraform command.
		 * Do this for all tiles (like trees), not only objects. */
		ClearedObjectArea *coa = FindClearedObject(end_tile);
		if (coa == NULL) {
			coa = _cleared_object_areas.Append();
			coa->first_tile = end_tile;
			coa->area = TileArea(end_tile, 1, 1);
		}

		/* Hide the tile from the terraforming command */
		TileIndex old_first_tile = coa->first_tile;
		coa->first_tile = INVALID_TILE;
		ret = DoCommand(end_tile, end_tileh & start_tileh, 0, flags, CMD_TERRAFORM_LAND);
		coa->first_tile = old_first_tile;
		if (ret.Failed()) return_cmd_error(STR_ERROR_UNABLE_TO_EXCAVATE_LAND);
		cost.AddCost(ret);
	}
	cost.AddCost(_price[PR_BUILD_TUNNEL]);

	/* Pay for the rail/road in the tunnel including entrances */
	switch (transport_type) {
		case TRANSPORT_ROAD: cost.AddCost((tiles + 2) * _price[PR_BUILD_ROAD] * 2); break;
		case TRANSPORT_RAIL: cost.AddCost((tiles + 2) * RailBuildCost(railtype)); break;
		default: NOT_REACHED();
	}

	if (flags & DC_EXEC) {
		Company *c = Company::GetIfValid(company);
		uint num_pieces = (tiles + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
		if (transport_type == TRANSPORT_RAIL) {
			if (!IsTunnelTile(start_tile) && c != NULL) c->infrastructure.rail[railtype] += num_pieces;
			MakeRailTunnel(start_tile, company, direction,                 railtype);
			MakeRailTunnel(end_tile,   company, ReverseDiagDir(direction), railtype);
			AddTunnelToSignalBuffer(start_tile, company);
			YapfNotifyTrackLayoutChange(start_tile, DiagDirToDiagTrack(direction));
		} else {
			if (c != NULL) {
				RoadType rt;
				FOR_EACH_SET_ROADTYPE(rt, rts ^ (IsTunnelTile(start_tile) ? GetRoadTypes(start_tile) : ROADTYPES_NONE)) {
					c->infrastructure.road[rt] += num_pieces * 2; // A full diagonal road has two road bits.
				}
			}
			MakeRoadTunnel(start_tile, company, direction,                 rts);
			MakeRoadTunnel(end_tile,   company, ReverseDiagDir(direction), rts);
		}
		DirtyCompanyInfrastructureWindows(company);
	}

	return cost;
}
