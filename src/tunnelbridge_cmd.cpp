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
#include "ship.h"
#include "roadveh.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "newgrf_sound.h"
#include "autoslope.h"
#include "tunnelbridge_map.h"
#include "strings_func.h"
#include "date_func.h"
#include "clear_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "tunnelbridge.h"
#include "cheat_type.h"
#include "elrail_func.h"
#include "pbs.h"
#include "company_base.h"
#include "newgrf_railtype.h"
#include "object_base.h"
#include "water.h"
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
	bool pbs_reservation = false;

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

				/* Keep the reservation, the path stays valid. */
				pbs_reservation = HasTunnelBridgeReservation(tile_start);

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
						GetBridgeSpec(bridge_type)->speed < GetBridgeSpec(GetBridgeType(tile_start))->speed &&
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
					if (IsTileSubtype(tile, TT_MISC_DEPOT)) goto not_valid_below;
					assert(TT_BRIDGE == TT_MISC_AQUEDUCT);
					/* fall through */
				case TT_RAILWAY:
				case TT_ROAD:
					if (!IsTileSubtype(tile, TT_BRIDGE)) break;
					if (direction == DiagDirToAxis(GetTunnelBridgeDirection(tile))) goto not_valid_below;
					if (z_start < GetBridgeHeight(tile)) goto not_valid_below;
					break;

				case TT_TUNNELBRIDGE_TEMP:
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

			if (flags & DC_EXEC) {
				/* We do this here because when replacing a bridge with another
				 * type calling SetBridgeMiddle isn't needed. After all, the
				 * tile already has the has_bridge_above bits set. */
				SetBridgeMiddle(tile, direction);
			}
		}
	}

	/* do the drill? */
	if (flags & DC_EXEC) {
		DiagDirection dir = AxisToDiagDir(direction);

		Company *c = Company::GetIfValid(company);
		switch (transport_type) {
			case TRANSPORT_RAIL:
				/* Add to company infrastructure count if building a new bridge. */
				if (!IsBridgeHeadTile(tile_start) && c != NULL) c->infrastructure.rail[railtype] += (bridge_len + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
				MakeRailBridgeRamp(tile_start, company, bridge_type, dir,                 railtype);
				MakeRailBridgeRamp(tile_end,   company, bridge_type, ReverseDiagDir(dir), railtype);
				SetTunnelBridgeReservation(tile_start, pbs_reservation);
				SetTunnelBridgeReservation(tile_end,   pbs_reservation);
				break;

			case TRANSPORT_ROAD: {
				RoadTypes prev_roadtypes = IsRoadBridgeTile(tile_start) ? GetRoadTypes(tile_start) : ROADTYPES_NONE;
				/* Also give unowned present roadtypes to new owner */
				if (HasBit(prev_roadtypes, ROADTYPE_ROAD) && GetRoadOwner(tile_start, ROADTYPE_ROAD) == OWNER_NONE) ClrBit(prev_roadtypes, ROADTYPE_ROAD);
				if (HasBit(prev_roadtypes, ROADTYPE_TRAM) && GetRoadOwner(tile_start, ROADTYPE_TRAM) == OWNER_NONE) ClrBit(prev_roadtypes, ROADTYPE_TRAM);
				if (c != NULL) {
					/* Add all new road types to the company infrastructure counter. */
					RoadType new_rt;
					FOR_EACH_SET_ROADTYPE(new_rt, roadtypes ^ prev_roadtypes) {
						/* A full diagonal road tile has two road bits. */
						c->infrastructure.road[new_rt] += (bridge_len + 2) * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
					}
				}
				Owner owner_road = company;
				Owner owner_tram = company;
				if (HasBit(prev_roadtypes, ROADTYPE_ROAD)) owner_road = GetRoadOwner(tile_start, ROADTYPE_ROAD);
				if (HasBit(prev_roadtypes, ROADTYPE_TRAM)) owner_tram = GetRoadOwner(tile_start, ROADTYPE_TRAM);
				MakeRoadBridgeRamp(tile_start, owner_road, owner_tram, bridge_type, dir,                 roadtypes);
				MakeRoadBridgeRamp(tile_end,   owner_road, owner_tram, bridge_type, ReverseDiagDir(dir), roadtypes);
				break;
			}

			case TRANSPORT_WATER:
				assert(!IsBridgeHeadTile(tile_start));
				if (c != NULL) c->infrastructure.water += (bridge_len + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
				MakeAqueductBridgeRamp(tile_start, company, dir);
				MakeAqueductBridgeRamp(tile_end,   company, ReverseDiagDir(dir));
				break;

			default:
				NOT_REACHED();
		}

		MarkBridgeTilesDirty(tile_start, tile_end, AxisToDiagDir(direction));
		DirtyCompanyInfrastructureWindows(company);
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


/**
 * Remove a tunnel from the game.
 * @param tile Tile containing one of the endpoints.
 * @param flags Command flags.
 * @return Succeeded or failed command.
 */
static CommandCost ClearTile_TunnelBridge(TileIndex tile, DoCommandFlag flags)
{
	assert(IsTunnel(tile));

	if (flags & DC_AUTO) return_cmd_error(STR_ERROR_MUST_DEMOLISH_TUNNEL_FIRST);

	if (_current_company != OWNER_WATER && _game_mode != GM_EDITOR) {
		if (GetTunnelTransportType(tile) == TRANSPORT_RAIL) {
			CommandCost ret = CheckOwnership(GetTileOwner(tile));
			if (ret.Failed()) return ret;
		} else {
			RoadTypes rts = GetRoadTypes(tile);
			Owner road_owner = _current_company;
			Owner tram_owner = _current_company;

			if (HasBit(rts, ROADTYPE_ROAD)) road_owner = GetRoadOwner(tile, ROADTYPE_ROAD);
			if (HasBit(rts, ROADTYPE_TRAM)) tram_owner = GetRoadOwner(tile, ROADTYPE_TRAM);

			/* We can remove unowned road and if the town allows it */
			if (road_owner == OWNER_TOWN && !(_settings_game.construction.extra_dynamite || _cheats.magic_bulldozer.value)) {
				CommandCost ret = CheckTileOwnership(tile);
				if (ret.Failed()) return ret;
			} else {
				if (road_owner == OWNER_NONE || road_owner == OWNER_TOWN) road_owner = _current_company;
				if (tram_owner == OWNER_NONE) tram_owner = _current_company;

				CommandCost ret = CheckOwnership(road_owner, tile);
				if (ret.Failed()) return ret;
				ret = CheckOwnership(tram_owner, tile);
				if (ret.Failed()) return ret;
			}
		}
	}

	TileIndex endtile = GetOtherTunnelEnd(tile);

	CommandCost ret = TunnelBridgeIsFree(tile, endtile);
	if (ret.Failed()) return ret;

	_build_tunnel_endtile = endtile;

	Town *t = NULL;
	if (IsTileOwner(tile, OWNER_TOWN) && _game_mode != GM_EDITOR) {
		t = ClosestTownFromTile(tile, UINT_MAX); // town penalty rating

		/* Check if you are allowed to remove the tunnel owned by a town
		 * Removal depends on difficulty settings */
		CommandCost ret = CheckforTownRating(flags, t, TUNNELBRIDGE_REMOVE);
		if (ret.Failed()) return ret;
	}

	/* checks if the owner is town then decrease town rating by RATING_TUNNEL_BRIDGE_DOWN_STEP until
	 * you have a "Poor" (0) town rating */
	if (IsTileOwner(tile, OWNER_TOWN) && _game_mode != GM_EDITOR) {
		ChangeTownRating(t, RATING_TUNNEL_BRIDGE_DOWN_STEP, RATING_TUNNEL_BRIDGE_MINIMUM, flags);
	}

	uint len = GetTunnelBridgeLength(tile, endtile) + 2; // Don't forget the end tiles.

	if (flags & DC_EXEC) {
		if (GetTunnelTransportType(tile) == TRANSPORT_RAIL) {
			/* We first need to request values before calling DoClearSquare */
			DiagDirection dir = GetTunnelBridgeDirection(tile);
			Track track = DiagDirToDiagTrack(dir);
			Owner owner = GetTileOwner(tile);

			Train *v = NULL;
			if (HasTunnelBridgeReservation(tile)) {
				v = GetTrainForReservation(tile, track);
				if (v != NULL) FreeTrainTrackReservation(v);
			}

			if (Company::IsValidID(owner)) {
				Company::Get(owner)->infrastructure.rail[GetRailType(tile)] -= len * TUNNELBRIDGE_TRACKBIT_FACTOR;
				DirtyCompanyInfrastructureWindows(owner);
			}

			DoClearSquare(tile);
			DoClearSquare(endtile);

			/* cannot use INVALID_DIAGDIR for signal update because the tunnel doesn't exist anymore */
			AddSideToSignalBuffer(tile,    ReverseDiagDir(dir), owner);
			AddSideToSignalBuffer(endtile, dir,                 owner);

			YapfNotifyTrackLayoutChange(tile,    track);
			YapfNotifyTrackLayoutChange(endtile, track);

			if (v != NULL) TryPathReserve(v);
		} else {
			RoadType rt;
			FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
				/* A full diagonal road tile has two road bits. */
				Company *c = Company::GetIfValid(GetRoadOwner(tile, rt));
				if (c != NULL) {
					c->infrastructure.road[rt] -= len * 2 * TUNNELBRIDGE_TRACKBIT_FACTOR;
					DirtyCompanyInfrastructureWindows(c->index);
				}
			}

			DoClearSquare(tile);
			DoClearSquare(endtile);
		}
	}
	return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_CLEAR_TUNNEL] * len);
}

/**
 * Draws a tunnel tile.
 * @param ti TileInfo of the structure to draw
 * Please note that in this code, "roads" are treated as railtype 1, whilst the real railtypes are 0, 2 and 3
 */
static void DrawTile_TunnelBridge(TileInfo *ti)
{
	TransportType transport_type = GetTunnelTransportType(ti->tile);
	DiagDirection tunnelbridge_direction = GetTunnelBridgeDirection(ti->tile);

	assert(IsTunnel(ti->tile));

	/* Front view of tunnel bounding boxes:
	 *
	 *   122223  <- BB_Z_SEPARATOR
	 *   1    3
	 *   1    3                1,3 = empty helper BB
	 *   1    3                  2 = SpriteCombine of tunnel-roof and catenary (tram & elrail)
	 *
	 */

	static const int _tunnel_BB[4][12] = {
		/*  tunnnel-roof  |  Z-separator  | tram-catenary
		 * w  h  bb_x bb_y| x   y   w   h |bb_x bb_y w h */
		{  1,  0, -15, -14,  0, 15, 16,  1, 0, 1, 16, 15 }, // NE
		{  0,  1, -14, -15, 15,  0,  1, 16, 1, 0, 15, 16 }, // SE
		{  1,  0, -15, -14,  0, 15, 16,  1, 0, 1, 16, 15 }, // SW
		{  0,  1, -14, -15, 15,  0,  1, 16, 1, 0, 15, 16 }, // NW
	};
	const int *BB_data = _tunnel_BB[tunnelbridge_direction];

	bool catenary = false;

	SpriteID image;
	SpriteID railtype_overlay = 0;
	if (transport_type == TRANSPORT_RAIL) {
		const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));
		image = rti->base_sprites.tunnel;
		if (rti->UsesOverlay()) {
			/* Check if the railtype has custom tunnel portals. */
			railtype_overlay = GetCustomRailSprite(rti, ti->tile, RTSG_TUNNEL_PORTAL);
			if (railtype_overlay != 0) image = SPR_RAILTYPE_TUNNEL_BASE; // Draw blank grass tunnel base.
		}
	} else {
		image = SPR_TUNNEL_ENTRY_REAR_ROAD;
	}

	if (HasTunnelBridgeSnowOrDesert(ti->tile)) image += railtype_overlay != 0 ? 8 : 32;

	image += tunnelbridge_direction * 2;
	DrawGroundSprite(image, PAL_NONE);

	/* PBS debugging, draw reserved tracks darker */
	if (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation && (transport_type == TRANSPORT_RAIL && HasTunnelBridgeReservation(ti->tile))) {
		const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));
		DrawGroundSprite(DiagDirToAxis(tunnelbridge_direction) == AXIS_X ? rti->base_sprites.single_x : rti->base_sprites.single_y, PALETTE_CRASH);
	}

	if (transport_type == TRANSPORT_ROAD) {
		RoadTypes rts = GetRoadTypes(ti->tile);

		if (HasBit(rts, ROADTYPE_TRAM)) {
			static const SpriteID tunnel_sprites[2][4] = { { 28, 78, 79, 27 }, {  5, 76, 77,  4 } };

			DrawGroundSprite(SPR_TRAMWAY_BASE + tunnel_sprites[rts - ROADTYPES_TRAM][tunnelbridge_direction], PAL_NONE);

			/* Do not draw wires if they are invisible */
			if (!IsInvisibilitySet(TO_CATENARY)) {
				catenary = true;
				StartSpriteCombine();
				AddSortableSpriteToDraw(SPR_TRAMWAY_TUNNEL_WIRES + tunnelbridge_direction, PAL_NONE, ti->x, ti->y, BB_data[10], BB_data[11], TILE_HEIGHT, ti->z, IsTransparencySet(TO_CATENARY), BB_data[8], BB_data[9], BB_Z_SEPARATOR);
			}
		}
	} else {
		const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));
		if (rti->UsesOverlay()) {
			SpriteID surface = GetCustomRailSprite(rti, ti->tile, RTSG_TUNNEL);
			if (surface != 0) DrawGroundSprite(surface + tunnelbridge_direction, PAL_NONE);
		}

		if (HasCatenaryDrawn(GetRailType(ti->tile))) {
			/* Maybe draw pylons on the entry side */
			DrawCatenary(ti);

			catenary = true;
			StartSpriteCombine();
			/* Draw wire above the ramp */
			DrawCatenaryOnTunnel(ti);
		}
	}

	if (railtype_overlay != 0 && !catenary) StartSpriteCombine();

	AddSortableSpriteToDraw(image + 1, PAL_NONE, ti->x + TILE_SIZE - 1, ti->y + TILE_SIZE - 1, BB_data[0], BB_data[1], TILE_HEIGHT, ti->z, false, BB_data[2], BB_data[3], BB_Z_SEPARATOR);
	/* Draw railtype tunnel portal overlay if defined. */
	if (railtype_overlay != 0) AddSortableSpriteToDraw(railtype_overlay + tunnelbridge_direction, PAL_NONE, ti->x + TILE_SIZE - 1, ti->y + TILE_SIZE - 1, BB_data[0], BB_data[1], TILE_HEIGHT, ti->z, false, BB_data[2], BB_data[3], BB_Z_SEPARATOR);

	if (catenary || railtype_overlay != 0) EndSpriteCombine();

	/* Add helper BB for sprite sorting that separates the tunnel from things beside of it. */
	AddSortableSpriteToDraw(SPR_EMPTY_BOUNDING_BOX, PAL_NONE, ti->x,              ti->y,              BB_data[6], BB_data[7], TILE_HEIGHT, ti->z);
	AddSortableSpriteToDraw(SPR_EMPTY_BOUNDING_BOX, PAL_NONE, ti->x + BB_data[4], ti->y + BB_data[5], BB_data[6], BB_data[7], TILE_HEIGHT, ti->z);

	DrawBridgeMiddle(ti);
}


static int GetSlopePixelZ_TunnelBridge(TileIndex tile, uint x, uint y)
{
	int z;
	Slope tileh = GetTilePixelSlope(tile, &z);

	x &= 0xF;
	y &= 0xF;

	assert(IsTunnel(tile));

	/* In the tunnel entrance? */
	uint pos = (DiagDirToAxis(GetTunnelBridgeDirection(tile)) == AXIS_X ? y : x);
	if (5 <= pos && pos <= 10) return z;

	return z + GetPartialPixelZ(x, y, tileh);
}

static Foundation GetFoundation_TunnelBridge(TileIndex tile, Slope tileh)
{
	return FOUNDATION_NONE;
}

static void GetTileDesc_TunnelBridge(TileIndex tile, TileDesc *td)
{
	TransportType tt = GetTunnelTransportType(tile);

	assert(IsTunnel(tile));

	td->str = (tt == TRANSPORT_RAIL) ? STR_LAI_TUNNEL_DESCRIPTION_RAILROAD : STR_LAI_TUNNEL_DESCRIPTION_ROAD;
	td->owner[0] = GetTileOwner(tile);

	Owner road_owner = INVALID_OWNER;
	Owner tram_owner = INVALID_OWNER;
	RoadTypes rts = GetRoadTypes(tile);
	if (HasBit(rts, ROADTYPE_ROAD)) road_owner = GetRoadOwner(tile, ROADTYPE_ROAD);
	if (HasBit(rts, ROADTYPE_TRAM)) tram_owner = GetRoadOwner(tile, ROADTYPE_TRAM);

	/* Is there a mix of owners? */
	if ((tram_owner != INVALID_OWNER && tram_owner != td->owner[0]) ||
			(road_owner != INVALID_OWNER && road_owner != td->owner[0])) {
		uint i = 1;
		if (road_owner != INVALID_OWNER) {
			td->owner_type[i] = STR_LAND_AREA_INFORMATION_ROAD_OWNER;
			td->owner[i] = road_owner;
			i++;
		}
		if (tram_owner != INVALID_OWNER) {
			td->owner_type[i] = STR_LAND_AREA_INFORMATION_TRAM_OWNER;
			td->owner[i] = tram_owner;
		}
	}

	if (tt == TRANSPORT_RAIL) {
		const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(tile));
		td->rail_speed = rti->max_speed;
	}
}


static void TileLoop_TunnelBridge(TileIndex tile)
{
	bool snow_or_desert = HasTunnelBridgeSnowOrDesert(tile);
	switch (_settings_game.game_creation.landscape) {
		case LT_ARCTIC: {
			/* As long as we do not have a snow density, we want to use the density
			 * from the entry edge. For tunnels this is the lowest point.
			 * (Independent of foundations) */
			if (snow_or_desert != (GetTileZ(tile) > GetSnowLine())) {
				SetTunnelBridgeSnowOrDesert(tile, !snow_or_desert);
				MarkTileDirtyByTile(tile);
			}
			break;
		}

		case LT_TROPIC:
			if (GetTropicZone(tile) == TROPICZONE_DESERT && !snow_or_desert) {
				SetTunnelBridgeSnowOrDesert(tile, true);
				MarkTileDirtyByTile(tile);
			}
			break;

		default:
			break;
	}
}

static TrackStatus GetTileTrackStatus_TunnelBridge(TileIndex tile, TransportType mode, uint sub_mode, DiagDirection side)
{
	TransportType transport_type = GetTunnelTransportType(tile);
	if (transport_type != mode || (transport_type == TRANSPORT_ROAD && (GetRoadTypes(tile) & sub_mode) == 0)) return 0;

	DiagDirection dir = GetTunnelBridgeDirection(tile);
	if (side != INVALID_DIAGDIR && side != ReverseDiagDir(dir)) return 0;
	return CombineTrackStatus(TrackBitsToTrackdirBits(DiagDirToDiagTrackBits(dir)), TRACKDIR_BIT_NONE);
}

static void ChangeTileOwner_TunnelBridge(TileIndex tile, Owner old_owner, Owner new_owner)
{
	TileIndex other_end = GetOtherTunnelBridgeEnd(tile);
	/* Set number of pieces to zero if it's the southern tile as we
	 * don't want to update the infrastructure counts twice. */
	uint num_pieces = tile < other_end ? (GetTunnelBridgeLength(tile, other_end) + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR : 0;

	for (RoadType rt = ROADTYPE_ROAD; rt < ROADTYPE_END; rt++) {
		/* Update all roadtypes, no matter if they are present */
		if (GetRoadOwner(tile, rt) == old_owner) {
			if (HasBit(GetRoadTypes(tile), rt)) {
				/* Update company infrastructure counts. A full diagonal road tile has two road bits.
				 * No need to dirty windows here, we'll redraw the whole screen anyway. */
				Company::Get(old_owner)->infrastructure.road[rt] -= num_pieces * 2;
				if (new_owner != INVALID_OWNER) Company::Get(new_owner)->infrastructure.road[rt] += num_pieces * 2;
			}

			SetRoadOwner(tile, rt, new_owner == INVALID_OWNER ? OWNER_NONE : new_owner);
		}
	}

	if (!IsTileOwner(tile, old_owner)) return;

	/* Update company infrastructure counts for rail and water as well.
	 * No need to dirty windows here, we'll redraw the whole screen anyway. */
	TransportType tt = GetTunnelTransportType(tile);
	Company *old = Company::Get(old_owner);
	if (tt == TRANSPORT_RAIL) {
		old->infrastructure.rail[GetRailType(tile)] -= num_pieces;
		if (new_owner != INVALID_OWNER) Company::Get(new_owner)->infrastructure.rail[GetRailType(tile)] += num_pieces;
	} else if (tt == TRANSPORT_WATER) {
		old->infrastructure.water -= num_pieces;
		if (new_owner != INVALID_OWNER) Company::Get(new_owner)->infrastructure.water += num_pieces;
	}

	if (new_owner != INVALID_OWNER) {
		SetTileOwner(tile, new_owner);
	} else {
		if (tt == TRANSPORT_RAIL) {
			/* Since all of our vehicles have been removed, it is safe to remove the rail
			 * tunnel. */
			CommandCost ret = DoCommand(tile, 0, 0, DC_EXEC | DC_BANKRUPT, CMD_LANDSCAPE_CLEAR);
			assert(ret.Succeeded());
		} else {
			/* In any other case, we can safely reassign the ownership to OWNER_NONE. */
			SetTileOwner(tile, OWNER_NONE);
		}
	}
}

/**
 * Frame when the 'enter tunnel' sound should be played. This is the second
 * frame on a tile, so the sound is played shortly after entering the tunnel
 * tile, while the vehicle is still visible.
 */
static const byte TUNNEL_SOUND_FRAME = 1;

/**
 * Frame when a vehicle should be hidden in a tunnel with a certain direction.
 * This differs per direction, because of visibility / bounding box issues.
 * Note that direction, in this case, is the direction leading into the tunnel.
 * When entering a tunnel, hide the vehicle when it reaches the given frame.
 * When leaving a tunnel, show the vehicle when it is one frame further
 * to the 'outside', i.e. at (TILE_SIZE-1) - (frame) + 1
 */
extern const byte _tunnel_visibility_frame[DIAGDIR_END] = {12, 8, 8, 12};

static VehicleEnterTileStatus VehicleEnter_TunnelBridge(Vehicle *v, TileIndex tile, int x, int y)
{
	int z = GetSlopePixelZ(x, y) - v->z_pos;
	assert(abs(z) < 3);

	/* Direction into the wormhole */
	const DiagDirection dir = GetTunnelBridgeDirection(tile);
	/* Direction of the vehicle */
	const DiagDirection vdir = DirToDiagDir(v->direction);
	/* New position of the vehicle on the tile */
	byte pos = (DiagDirToAxis(vdir) == AXIS_X ? x : y) & TILE_UNIT_MASK;
	/* Number of units moved by the vehicle since entering the tile */
	byte frame = (vdir == DIAGDIR_NE || vdir == DIAGDIR_NW) ? TILE_SIZE - 1 - pos : pos;

	assert(IsTunnel(tile));

	if (v->type == VEH_TRAIN) {
		Train *t = Train::From(v);

		if (t->track != TRACK_BIT_WORMHOLE && dir == vdir) {
			if (t->IsFrontEngine() && frame == TUNNEL_SOUND_FRAME) {
				if (!PlayVehicleSound(t, VSE_TUNNEL) && RailVehInfo(t->engine_type)->engclass == 0) {
					SndPlayVehicleFx(SND_05_TRAIN_THROUGH_TUNNEL, v);
				}
				return VETSB_CONTINUE;
			}
			if (frame == _tunnel_visibility_frame[dir]) {
				t->tile = GetOtherTunnelEnd(tile);
				t->track = TRACK_BIT_WORMHOLE;
				t->vehstatus |= VS_HIDDEN;
				return VETSB_ENTERED_WORMHOLE;
			}
		}

		if (dir == ReverseDiagDir(vdir) && frame == TILE_SIZE - _tunnel_visibility_frame[dir] && z == 0) {
			/* We're at the tunnel exit ?? */
			t->tile = tile;
			t->track = DiagDirToDiagTrackBits(vdir);
			assert(t->track);
			t->vehstatus &= ~VS_HIDDEN;
			return VETSB_ENTERED_WORMHOLE;
		}
	} else if (v->type == VEH_ROAD) {
		RoadVehicle *rv = RoadVehicle::From(v);

		/* Enter tunnel? */
		if (rv->state != RVSB_WORMHOLE && dir == vdir) {
			if (frame == _tunnel_visibility_frame[dir]) {
				/* Frame should be equal to the next frame number in the RV's movement */
				assert(frame == rv->frame + 1);
				rv->tile = GetOtherTunnelEnd(tile);
				rv->state = RVSB_WORMHOLE;
				rv->vehstatus |= VS_HIDDEN;
				return VETSB_ENTERED_WORMHOLE;
			} else {
				return VETSB_CONTINUE;
			}
		}

		/* We're at the tunnel exit ?? */
		if (dir == ReverseDiagDir(vdir) && frame == TILE_SIZE - _tunnel_visibility_frame[dir] && z == 0) {
			rv->tile = tile;
			rv->state = DiagDirToDiagTrackdir(vdir);
			rv->frame = frame;
			rv->vehstatus &= ~VS_HIDDEN;
			return VETSB_ENTERED_WORMHOLE;
		}
	}

	return VETSB_CONTINUE;
}

static CommandCost TerraformTile_TunnelBridge(TileIndex tile, DoCommandFlag flags, int z_new, Slope tileh_new)
{
	return DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
}

extern const TileTypeProcs _tile_type_tunnelbridge_procs = {
	DrawTile_TunnelBridge,           // draw_tile_proc
	GetSlopePixelZ_TunnelBridge,     // get_slope_z_proc
	ClearTile_TunnelBridge,          // clear_tile_proc
	NULL,                            // add_accepted_cargo_proc
	GetTileDesc_TunnelBridge,        // get_tile_desc_proc
	GetTileTrackStatus_TunnelBridge, // get_tile_track_status_proc
	NULL,                            // click_tile_proc
	NULL,                            // animate_tile_proc
	TileLoop_TunnelBridge,           // tile_loop_proc
	ChangeTileOwner_TunnelBridge,    // change_tile_owner_proc
	NULL,                            // add_produced_cargo_proc
	VehicleEnter_TunnelBridge,       // vehicle_enter_tile_proc
	GetFoundation_TunnelBridge,      // get_foundation_proc
	TerraformTile_TunnelBridge,      // terraform_tile_proc
};
