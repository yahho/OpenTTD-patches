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
#include "tunnelbridge_map.h"
#include "water_map.h"
#include "strings_func.h"
#include "tunnelbridge.h"
#include "cheat_type.h"
#include "company_base.h"
#include "object_base.h"
#include "company_gui.h"

#include "table/strings.h"

TileIndex _build_tunnel_endtile; ///< The end of a tunnel; as hidden return from the tunnel build command for GUI purposes.


/**
 * Build a Bridge
 * @param end_tile end tile
 * @param flags type of operation
 * @param p1 packed start tile coords (~ dx)
 * @param p2 various bitstuffed elements
 * - p2 = (bit  0- 7) - bridge type (hi bh)
 * - p2 = (bit  8-11) - rail type or road types.
 * - p2 = (bit 15-16) - transport type.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildBridge(TileIndex end_tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	CompanyID company = _current_company;

	RailType railtype = INVALID_RAILTYPE;
	RoadTypes roadtypes = ROADTYPES_NONE;

	/* unpack parameters */
	BridgeType bridge_type = GB(p2, 0, 8);

	if (!IsValidTile(p1)) return_cmd_error(STR_ERROR_BRIDGE_THROUGH_MAP_BORDER);

	TransportType transport_type = Extract<TransportType, 15, 2>(p2);

	/* type of bridge */
	switch (transport_type) {
		case TRANSPORT_ROAD:
			roadtypes = Extract<RoadTypes, 8, 2>(p2);
			if (!HasExactlyOneBit(roadtypes) || !HasRoadTypesAvail(company, roadtypes)) return CMD_ERROR;
			break;

		case TRANSPORT_RAIL:
			railtype = Extract<RailType, 8, 4>(p2);
			if (!ValParamRailtype(railtype)) return CMD_ERROR;
			break;

		case TRANSPORT_WATER:
			break;

		default:
			/* Airports don't have bridges. */
			return CMD_ERROR;
	}
	TileIndex tile_start = p1;
	TileIndex tile_end = end_tile;

	if (company == OWNER_DEITY) {
		if (transport_type != TRANSPORT_ROAD) return CMD_ERROR;
		const Town *town = CalcClosestTownFromTile(tile_start);

		company = OWNER_TOWN;

		/* If we are not within a town, we are not owned by the town */
		if (town == NULL || DistanceSquare(tile_start, town->xy) > town->cache.squared_town_zone_radius[HZB_TOWN_EDGE]) {
			company = OWNER_NONE;
		}
	}

	if (tile_start == tile_end) {
		return_cmd_error(STR_ERROR_CAN_T_START_AND_END_ON);
	}

	Axis direction;
	if (TileX(tile_start) == TileX(tile_end)) {
		direction = AXIS_Y;
	} else if (TileY(tile_start) == TileY(tile_end)) {
		direction = AXIS_X;
	} else {
		return_cmd_error(STR_ERROR_START_AND_END_MUST_BE_IN);
	}

	if (tile_end < tile_start) Swap(tile_start, tile_end);

	uint bridge_len = GetTunnelBridgeLength(tile_start, tile_end);
	if (transport_type != TRANSPORT_WATER) {
		/* set and test bridge length, availability */
		CommandCost ret = CheckBridgeAvailability(bridge_type, bridge_len, flags);
		if (ret.Failed()) return ret;
	} else {
		if (bridge_len > _settings_game.construction.max_bridge_length) return_cmd_error(STR_ERROR_BRIDGE_TOO_LONG);
	}

	int z_start;
	int z_end;
	Slope tileh_start = GetTileSlope(tile_start, &z_start);
	Slope tileh_end = GetTileSlope(tile_end, &z_end);

	CommandCost terraform_cost_north = CheckBridgeSlope(direction == AXIS_X ? DIAGDIR_SW : DIAGDIR_SE, &tileh_start, &z_start);
	CommandCost terraform_cost_south = CheckBridgeSlope(direction == AXIS_X ? DIAGDIR_NE : DIAGDIR_NW, &tileh_end,   &z_end);

	/* Aqueducts can't be built of flat land. */
	if (transport_type == TRANSPORT_WATER && (tileh_start == SLOPE_FLAT || tileh_end == SLOPE_FLAT)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
	if (z_start != z_end) return_cmd_error(STR_ERROR_BRIDGEHEADS_NOT_SAME_HEIGHT);

	CommandCost cost(EXPENSES_CONSTRUCTION);

	if (IsBridgeHeadTile(tile_start) && IsBridgeHeadTile(tile_end) &&
			GetOtherBridgeEnd(tile_start) == tile_end &&
			GetTunnelBridgeTransportType(tile_start) == transport_type) {
		/* Replace a current bridge. */

		switch (transport_type) {
			default: NOT_REACHED();

			case TRANSPORT_RAIL:
				/* Make sure the railtypes match. */
				if (GetRailType(tile_start) != railtype) {
					return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
				}

				/* Do not allow replacing another company's bridges. */
				if (!IsTileOwner(tile_start, company)) {
					return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
				}

				break;

			case TRANSPORT_ROAD: {
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
				roadtypes |= GetRoadTypes(tile_start);

				break;
			}

			case TRANSPORT_WATER:
				return_cmd_error(STR_ERROR_ALREADY_BUILT);
		}

		/* Do not replace the bridge with the same bridge type. */
		if (!(flags & DC_QUERY_COST) && (bridge_type == GetBridgeType(tile_start)) && (transport_type != TRANSPORT_ROAD || (roadtypes & ~GetRoadTypes(tile_start)) == 0)) {
			return_cmd_error(STR_ERROR_ALREADY_BUILT);
		}

		cost.AddCost((bridge_len + 1) * _price[PR_CLEAR_BRIDGE]); // The cost of clearing the current bridge.

		/* do the drill? */
		if (flags & DC_EXEC) {
			switch (transport_type) {
				case TRANSPORT_RAIL:
					SetRailBridgeType(tile_start, bridge_type);
					SetRailBridgeType(tile_end,   bridge_type);
					break;

				case TRANSPORT_ROAD: {
					RoadTypes prev_roadtypes = GetRoadTypes(tile_start);
					/* Also give unowned present roadtypes to new owner */
					if (HasBit(prev_roadtypes, ROADTYPE_ROAD) && GetRoadOwner(tile_start, ROADTYPE_ROAD) == OWNER_NONE) ClrBit(prev_roadtypes, ROADTYPE_ROAD);
					if (HasBit(prev_roadtypes, ROADTYPE_TRAM) && GetRoadOwner(tile_start, ROADTYPE_TRAM) == OWNER_NONE) ClrBit(prev_roadtypes, ROADTYPE_TRAM);
					Company *c = Company::GetIfValid(company);
					if (c != NULL) {
						/* Add all new road types to the company infrastructure counter. */
						RoadType new_rt;
						FOR_EACH_SET_ROADTYPE(new_rt, roadtypes ^ prev_roadtypes) {
							/* A full diagonal road tile has two road bits. */
							c->infrastructure.road[new_rt] += (bridge_len + 2) * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
						}
					}
					DirtyCompanyInfrastructureWindows(company);

					SetRoadBridgeType(tile_start, bridge_type);
					SetRoadTypes(tile_start, roadtypes);
					SetRoadBridgeType(tile_end,   bridge_type);
					SetRoadTypes(tile_end,   roadtypes);

					for (RoadType rt = ROADTYPE_BEGIN; rt < ROADTYPE_END; rt++) {
						if (!HasBit(prev_roadtypes, rt)) {
							SetRoadOwner(tile_start, rt, company);
							SetRoadOwner(tile_end,   rt, company);
						}
					}

					break;
				}

				default:
					NOT_REACHED();
			}

			MarkBridgeTilesDirty(tile_start, tile_end, AxisToDiagDir(direction));
		}
	} else {
		/* Build a new bridge. */

		bool allow_on_slopes = (_settings_game.construction.build_on_slopes && transport_type != TRANSPORT_WATER);

		/* Try and clear the start landscape */
		CommandCost ret = DoCommand(tile_start, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret.Failed()) return ret;
		cost = ret;

		if (terraform_cost_north.Failed() || (terraform_cost_north.GetCost() != 0 && !allow_on_slopes)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
		cost.AddCost(terraform_cost_north);

		/* Try and clear the end landscape */
		ret = DoCommand(tile_end, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);

		/* false - end tile slope check */
		if (terraform_cost_south.Failed() || (terraform_cost_south.GetCost() != 0 && !allow_on_slopes)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
		cost.AddCost(terraform_cost_south);

		const TileIndex heads[] = {tile_start, tile_end};
		for (int i = 0; i < 2; i++) {
			if (HasBridgeAbove(heads[i])) {
				TileIndex north_head = GetNorthernBridgeEnd(heads[i]);

				if (direction == GetBridgeAxis(heads[i])) return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);

				if (z_start + 1 == GetBridgeHeight(north_head)) {
					return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
				}
			}
		}

		TileIndexDiff delta = (direction == AXIS_X ? TileDiffXY(1, 0) : TileDiffXY(0, 1));
		for (TileIndex tile = tile_start + delta; tile != tile_end; tile += delta) {
			if (GetTileMaxZ(tile) > z_start) return_cmd_error(STR_ERROR_BRIDGE_TOO_LOW_FOR_TERRAIN);

			if (HasBridgeAbove(tile)) {
				/* Disallow crossing bridges for the time being */
				return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
			}

			switch (GetTileType(tile)) {
				case TT_WATER:
					if (!IsPlainWater(tile) && !IsCoast(tile)) goto not_valid_below;
					break;

				case TT_MISC:
					if (IsTileSubtype(tile, TT_MISC_TUNNEL)) break;
					if (IsTileSubtype(tile, TT_MISC_DEPOT)) goto not_valid_below;
					assert(TT_BRIDGE == TT_MISC_AQUEDUCT);
					/* fall through */
				case TT_RAILWAY:
				case TT_ROAD:
					if (!IsTileSubtype(tile, TT_BRIDGE)) break;
					if (direction == DiagDirToAxis(GetTunnelBridgeDirection(tile))) goto not_valid_below;
					if (z_start < GetBridgeHeight(tile)) goto not_valid_below;
					break;

				case TT_OBJECT: {
					const ObjectSpec *spec = ObjectSpec::GetByTile(tile);
					if ((spec->flags & OBJECT_FLAG_ALLOW_UNDER_BRIDGE) == 0) goto not_valid_below;
					if (GetTileMaxZ(tile) + spec->height > z_start) goto not_valid_below;
					break;
				}

				case TT_GROUND:
					assert(IsGroundTile(tile));
					if (!IsTileSubtype(tile, TT_GROUND_TREES)) break;
					/* fall through */
				default:
	not_valid_below:;
					/* try and clear the middle landscape */
					ret = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
					if (ret.Failed()) return ret;
					cost.AddCost(ret);
					break;
			}
		}

		/* do the drill? */
		if (flags & DC_EXEC) {
			DiagDirection dir = AxisToDiagDir(direction);

			Company *c = Company::GetIfValid(company);
			switch (transport_type) {
				case TRANSPORT_RAIL:
					/* Add to company infrastructure count. */
					if (c != NULL) c->infrastructure.rail[railtype] += (bridge_len + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
					MakeRailBridgeRamp(tile_start, company, bridge_type, dir,                 railtype);
					MakeRailBridgeRamp(tile_end,   company, bridge_type, ReverseDiagDir(dir), railtype);
					break;

				case TRANSPORT_ROAD:
					if (c != NULL) {
						/* Add all new road types to the company infrastructure counter. */
						RoadType new_rt;
						FOR_EACH_SET_ROADTYPE(new_rt, roadtypes) {
							/* A full diagonal road tile has two road bits. */
							c->infrastructure.road[new_rt] += (bridge_len + 2) * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
						}
					}
					MakeRoadBridgeRamp(tile_start, company, company, bridge_type, dir,                 roadtypes);
					MakeRoadBridgeRamp(tile_end,   company, company, bridge_type, ReverseDiagDir(dir), roadtypes);
					break;

				case TRANSPORT_WATER:
					if (c != NULL) c->infrastructure.water += (bridge_len + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
					MakeAqueductBridgeRamp(tile_start, company, dir);
					MakeAqueductBridgeRamp(tile_end,   company, ReverseDiagDir(dir));
					break;

				default:
					NOT_REACHED();
			}

			MarkTileDirtyByTile(tile_start);
			MarkTileDirtyByTile(tile_end);

			/* Mark all tiles dirty */
			for (TileIndex tile = tile_start + delta; tile < tile_end; tile += delta) {
				SetBridgeMiddle(tile, direction);
				MarkTileDirtyByTile(tile);
			}

			DirtyCompanyInfrastructureWindows(company);
		}
	}

	if ((flags & DC_EXEC) && transport_type == TRANSPORT_RAIL) {
		Track track = AxisToTrack(direction);
		AddSideToSignalBuffer(tile_start, INVALID_DIAGDIR, company);
		YapfNotifyTrackLayoutChange(tile_start, track);
	}

	/* for human player that builds the bridge he gets a selection to choose from bridges (DC_QUERY_COST)
	 * It's unnecessary to execute this command every time for every bridge. So it is done only
	 * and cost is computed in "bridge_gui.c". For AI, Towns this has to be of course calculated
	 */
	Company *c = Company::GetIfValid(company);
	if (!(flags & DC_QUERY_COST) || (c != NULL && c->is_ai)) {
		bridge_len += 2; // begin and end tiles/ramps

		switch (transport_type) {
			case TRANSPORT_ROAD: cost.AddCost(bridge_len * _price[PR_BUILD_ROAD] * 2 * CountBits(roadtypes)); break;
			case TRANSPORT_RAIL: cost.AddCost(bridge_len * RailBuildCost(railtype)); break;
			default: break;
		}

		if (c != NULL) bridge_len = CalcBridgeLenCostFactor(bridge_len);

		if (transport_type != TRANSPORT_WATER) {
			cost.AddCost((int64)bridge_len * _price[PR_BUILD_BRIDGE] * GetBridgeSpec(bridge_type)->price >> 8);
		} else {
			/* Aqueducts use a separate base cost. */
			cost.AddCost((int64)bridge_len * _price[PR_BUILD_AQUEDUCT]);
		}

	}

	return cost;
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
		default: break;
	}

	if (flags & DC_EXEC) {
		Company *c = Company::GetIfValid(company);
		uint num_pieces = (tiles + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
		if (transport_type == TRANSPORT_RAIL) {
			if (!IsTunnelTile(start_tile) && c != NULL) c->infrastructure.rail[railtype] += num_pieces;
			MakeRailTunnel(start_tile, company, direction,                 railtype);
			MakeRailTunnel(end_tile,   company, ReverseDiagDir(direction), railtype);
			AddSideToSignalBuffer(start_tile, INVALID_DIAGDIR, company);
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
