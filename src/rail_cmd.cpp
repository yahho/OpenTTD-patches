/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rail_cmd.cpp Handling of rail tiles. */

#include "stdafx.h"
#include "cmd_helper.h"
#include "viewport_func.h"
#include "command_func.h"
#include "depot_base.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "newgrf_debug.h"
#include "newgrf_railtype.h"
#include "train.h"
#include "autoslope.h"
#include "water.h"
#include "tunnelbridge_map.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "tunnelbridge.h"
#include "elrail_func.h"
#include "town.h"
#include "pbs.h"
#include "company_base.h"
#include "core/backup_type.hpp"
#include "date_func.h"
#include "strings_func.h"
#include "company_gui.h"
#include "map/object.h"

#include "table/strings.h"
#include "table/railtypes.h"

/** Helper type for lists/vectors of trains */
typedef SmallVector<Train *, 16> TrainList;

RailtypeInfo _railtypes[RAILTYPE_END];

assert_compile(sizeof(_original_railtypes) <= sizeof(_railtypes));

/** Enum holding the signal offset in the sprite sheet according to the side it is representing. */
enum SignalOffsets {
	SIGNAL_TO_NORTHEAST,
	SIGNAL_TO_SOUTHWEST,
	SIGNAL_TO_SOUTHEAST,
	SIGNAL_TO_NORTHWEST,
	SIGNAL_TO_WEST,
	SIGNAL_TO_EAST,
	SIGNAL_TO_NORTH,
	SIGNAL_TO_SOUTH,
};

/**
 * Reset all rail type information to its default values.
 */
void ResetRailTypes()
{
	memset(_railtypes, 0, sizeof(_railtypes));
	memcpy(_railtypes, _original_railtypes, sizeof(_original_railtypes));
}

void ResolveRailTypeGUISprites(RailtypeInfo *rti)
{
	SpriteID cursors_base = GetCustomRailSprite(rti, INVALID_TILE, RTSG_CURSORS);
	if (cursors_base != 0) {
		rti->gui_sprites.build_ns_rail = cursors_base +  0;
		rti->gui_sprites.build_x_rail  = cursors_base +  1;
		rti->gui_sprites.build_ew_rail = cursors_base +  2;
		rti->gui_sprites.build_y_rail  = cursors_base +  3;
		rti->gui_sprites.auto_rail     = cursors_base +  4;
		rti->gui_sprites.build_depot   = cursors_base +  5;
		rti->gui_sprites.build_tunnel  = cursors_base +  6;
		rti->gui_sprites.convert_rail  = cursors_base +  7;
		rti->cursor.rail_ns   = cursors_base +  8;
		rti->cursor.rail_swne = cursors_base +  9;
		rti->cursor.rail_ew   = cursors_base + 10;
		rti->cursor.rail_nwse = cursors_base + 11;
		rti->cursor.autorail  = cursors_base + 12;
		rti->cursor.depot     = cursors_base + 13;
		rti->cursor.tunnel    = cursors_base + 14;
		rti->cursor.convert   = cursors_base + 15;
	}

	/* Array of default GUI signal sprite numbers. */
	const SpriteID _signal_lookup[2][SIGTYPE_END] = {
		{SPR_IMG_SIGNAL_ELECTRIC_NORM,  SPR_IMG_SIGNAL_ELECTRIC_ENTRY, SPR_IMG_SIGNAL_ELECTRIC_EXIT,
		 SPR_IMG_SIGNAL_ELECTRIC_COMBO, SPR_IMG_SIGNAL_ELECTRIC_PBS,   SPR_IMG_SIGNAL_ELECTRIC_PBS_OWAY},

		{SPR_IMG_SIGNAL_SEMAPHORE_NORM,  SPR_IMG_SIGNAL_SEMAPHORE_ENTRY, SPR_IMG_SIGNAL_SEMAPHORE_EXIT,
		 SPR_IMG_SIGNAL_SEMAPHORE_COMBO, SPR_IMG_SIGNAL_SEMAPHORE_PBS,   SPR_IMG_SIGNAL_SEMAPHORE_PBS_OWAY},
	};

	for (SignalType type = SIGTYPE_NORMAL; type < SIGTYPE_END; type = (SignalType)(type + 1)) {
		for (SignalVariant var = SIG_ELECTRIC; var <= SIG_SEMAPHORE; var = (SignalVariant)(var + 1)) {
			SpriteID red   = GetCustomSignalSprite(rti, INVALID_TILE, type, var, SIGNAL_STATE_RED, true);
			SpriteID green = GetCustomSignalSprite(rti, INVALID_TILE, type, var, SIGNAL_STATE_GREEN, true);
			rti->gui_sprites.signals[type][var][0] = (red != 0)   ? red + SIGNAL_TO_NORTH   : _signal_lookup[var][type];
			rti->gui_sprites.signals[type][var][1] = (green != 0) ? green + SIGNAL_TO_NORTH : _signal_lookup[var][type] + 1;
		}
	}
}

/**
 * Resolve sprites of custom rail types
 */
void InitRailTypes()
{
	for (RailType rt = RAILTYPE_BEGIN; rt != RAILTYPE_END; rt++) {
		RailtypeInfo *rti = &_railtypes[rt];
		ResolveRailTypeGUISprites(rti);
	}
}

/**
 * Allocate a new rail type label
 */
RailType AllocateRailType(RailTypeLabel label)
{
	for (RailType rt = RAILTYPE_BEGIN; rt != RAILTYPE_END; rt++) {
		RailtypeInfo *rti = &_railtypes[rt];

		if (rti->label == 0) {
			/* Set up new rail type */
			memcpy(rti, &_railtypes[RAILTYPE_RAIL], sizeof(*rti));
			rti->label = label;
			/* Clear alternate label list. Can't use Reset() here as that would free
			 * the data pointer of RAILTYPE_RAIL and not our new rail type. */
			new (&rti->alternate_labels) RailTypeLabelList;

			/* Make us compatible with ourself. */
			rti->powered_railtypes    = (RailTypes)(1 << rt);
			rti->compatible_railtypes = (RailTypes)(1 << rt);

			/* We also introduce ourself. */
			rti->introduces_railtypes = (RailTypes)(1 << rt);

			/* Default sort order; order of allocation, but with some
			 * offsets so it's easier for NewGRF to pick a spot without
			 * changing the order of other (original) rail types.
			 * The << is so you can place other railtypes in between the
			 * other railtypes, the 7 is to be able to place something
			 * before the first (default) rail type. */
			rti->sorting_order = rt << 4 | 7;
			return rt;
		}
	}

	return INVALID_RAILTYPE;
}

static const byte _track_sloped_sprites[14] = {
	14, 15, 22, 13,
	 0, 21, 17, 12,
	23,  0, 18, 20,
	19, 16
};


/*         4
 *     ---------
 *    |\       /|
 *    | \    1/ |
 *    |  \   /  |
 *    |   \ /   |
 *  16|    \    |32
 *    |   / \2  |
 *    |  /   \  |
 *    | /     \ |
 *    |/       \|
 *     ---------
 *         8
 */


/**
 * Tests if a vehicle interacts with the specified track.
 * All track bits interact except parallel #TRACK_BIT_HORZ or #TRACK_BIT_VERT.
 *
 * @param tile The tile.
 * @param track The track.
 * @return Succeeded command (no train found), or a failed command (a train was found).
 */
static CommandCost EnsureNoTrainOnTrack(TileIndex tile, Track track)
{
	TrackBits rail_bits = TrackToTrackBits(track);
	return EnsureNoTrainOnTrackBits(tile, rail_bits);
}

/**
 * Check that the new track bits may be built.
 * @param tile %Tile to build on.
 * @param to_build New track bits.
 * @param flags    Flags of the operation.
 * @return Succeeded or failed command.
 */
static CommandCost CheckTrackCombination(TileIndex tile, Track to_build, RailType railtype, DoCommandFlag flags)
{
	assert(IsRailwayTile(tile));

	TrackBits current = GetTrackBits(tile); // The current track layout.
	assert(current != TRACK_BIT_NONE);

	TrackBits future = current | TrackToTrackBits(to_build); // The track layout we want to build.

	/* Are we really building something new? */
	if (current == future) {
		/* Nothing new is being built */
		if (IsCompatibleRail(GetRailType(tile, to_build), railtype)) {
			return_cmd_error(STR_ERROR_ALREADY_BUILT);
		} else {
			return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);
		}
	}

	/* These combinations are always allowed */
	if (future == TRACK_BIT_HORZ || future == TRACK_BIT_VERT) {
		if (flags & DC_EXEC) {
			SetRailType(tile, railtype, to_build);
		}
		return CommandCost();
	}

	if (flags & DC_NO_RAIL_OVERLAP) {
		/* If we are not allowed to overlap (flag is on for ai companies), check that */
		return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);
	}

	RailType rt; // RailType to convert to, or INVALID_RAILTYPE if no conversion is necessary

	if (current == TRACK_BIT_HORZ || current == TRACK_BIT_VERT) {
		RailType rt1 = GetRailType(tile, TRACK_UPPER);
		if (!IsCompatibleRail(rt1, railtype)) return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);

		RailType rt2 = GetRailType(tile, TRACK_LOWER);
		if (!IsCompatibleRail(rt2, railtype)) return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);

		if (rt1 != rt2) {
			/* Two different railtypes present */
			if ((railtype == rt1 || HasPowerOnRail(rt1, railtype)) && (railtype == rt2 || HasPowerOnRail(rt2, railtype))) {
				rt = railtype;
			} else if ((railtype == rt1 || HasPowerOnRail(railtype, rt1)) && HasPowerOnRail(rt2, rt1)) {
				rt = railtype = rt1;
			} else if ((railtype == rt2 || HasPowerOnRail(railtype, rt2)) && HasPowerOnRail(rt1, rt2)) {
				rt = railtype = rt2;
			} else {
				return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);
			}
		} else if (railtype == rt1) {
			/* Nothing to do */
			rt = INVALID_RAILTYPE;
		} else if (HasPowerOnRail(railtype, rt1)) {
			/* Try to keep existing railtype */
			railtype = rt1;
			rt = INVALID_RAILTYPE;
		} else if (HasPowerOnRail(rt1, railtype)) {
			rt = railtype;
		} else {
			return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);
		}
	} else {
		rt = GetRailType(tile, FindFirstTrack(current));

		if (railtype == rt) {
			/* Nothing to do */
			rt = INVALID_RAILTYPE;
		} else if (!IsCompatibleRail(rt, railtype)) {
			return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);
		} else if (HasPowerOnRail(railtype, rt)) {
			/* Try to keep existing railtype */
			railtype = rt;
			rt = INVALID_RAILTYPE;
		} else if (HasPowerOnRail(rt, railtype)) {
			rt = railtype;
		} else {
			return_cmd_error(STR_ERROR_IMPOSSIBLE_TRACK_COMBINATION);
		}
	}

	CommandCost ret;
	if (rt != INVALID_RAILTYPE) {
		ret = DoCommand(tile, tile, rt, flags, CMD_CONVERT_RAIL);
		if (ret.Failed()) return ret;
	}

	if (HasSignalOnTrack(tile, TRACK_UPPER) || HasSignalOnTrack(tile, TRACK_LOWER)) {
		return_cmd_error(STR_ERROR_MUST_REMOVE_SIGNALS_FIRST);
	}

	if (flags & DC_EXEC) {
		SetRailType(tile, railtype, to_build);
	}

	return ret;
}


/** Valid TrackBits on a specific (non-steep)-slope without foundation */
static const TrackBits _valid_tracks_without_foundation[15] = {
	TRACK_BIT_ALL,
	TRACK_BIT_RIGHT,
	TRACK_BIT_UPPER,
	TRACK_BIT_X,

	TRACK_BIT_LEFT,
	TRACK_BIT_NONE,
	TRACK_BIT_Y,
	TRACK_BIT_LOWER,

	TRACK_BIT_LOWER,
	TRACK_BIT_Y,
	TRACK_BIT_NONE,
	TRACK_BIT_LEFT,

	TRACK_BIT_X,
	TRACK_BIT_UPPER,
	TRACK_BIT_RIGHT,
};

/** Valid TrackBits on a specific (non-steep)-slope with leveled foundation */
static const TrackBits _valid_tracks_on_leveled_foundation[15] = {
	TRACK_BIT_NONE,
	TRACK_BIT_LEFT,
	TRACK_BIT_LOWER,
	TRACK_BIT_Y | TRACK_BIT_LOWER | TRACK_BIT_LEFT,

	TRACK_BIT_RIGHT,
	TRACK_BIT_ALL,
	TRACK_BIT_X | TRACK_BIT_LOWER | TRACK_BIT_RIGHT,
	TRACK_BIT_ALL,

	TRACK_BIT_UPPER,
	TRACK_BIT_X | TRACK_BIT_UPPER | TRACK_BIT_LEFT,
	TRACK_BIT_ALL,
	TRACK_BIT_ALL,

	TRACK_BIT_Y | TRACK_BIT_UPPER | TRACK_BIT_RIGHT,
	TRACK_BIT_ALL,
	TRACK_BIT_ALL
};

/**
 * Checks if a track combination is valid on a specific slope and returns the needed foundation.
 *
 * @param tileh Tile slope.
 * @param bits  Trackbits.
 * @return Needed foundation or FOUNDATION_INVALID if track/slope combination is not allowed.
 */
Foundation GetRailFoundation(Slope tileh, TrackBits bits)
{
	if (bits == TRACK_BIT_NONE) return FOUNDATION_NONE;

	if (IsSteepSlope(tileh)) {
		/* Test for inclined foundations */
		if (bits == TRACK_BIT_X) return FOUNDATION_INCLINED_X;
		if (bits == TRACK_BIT_Y) return FOUNDATION_INCLINED_Y;

		/* Get higher track */
		Corner highest_corner = GetHighestSlopeCorner(tileh);
		TrackBits higher_track = CornerToTrackBits(highest_corner);

		/* Only higher track? */
		if (bits == higher_track) return HalftileFoundation(highest_corner);

		/* Overlap with higher track? */
		if (TracksOverlap(bits | higher_track)) return FOUNDATION_INVALID;

		/* either lower track or both higher and lower track */
		return ((bits & higher_track) != 0 ? FOUNDATION_STEEP_BOTH : FOUNDATION_STEEP_LOWER);
	} else {
		if ((~_valid_tracks_without_foundation[tileh] & bits) == 0) return FOUNDATION_NONE;

		bool valid_on_leveled = ((~_valid_tracks_on_leveled_foundation[tileh] & bits) == 0);

		Corner track_corner;
		switch (bits) {
			case TRACK_BIT_LEFT:  track_corner = CORNER_W; break;
			case TRACK_BIT_LOWER: track_corner = CORNER_S; break;
			case TRACK_BIT_RIGHT: track_corner = CORNER_E; break;
			case TRACK_BIT_UPPER: track_corner = CORNER_N; break;

			case TRACK_BIT_HORZ:
				if (tileh == SLOPE_N) return HalftileFoundation(CORNER_N);
				if (tileh == SLOPE_S) return HalftileFoundation(CORNER_S);
				return (valid_on_leveled ? FOUNDATION_LEVELED : FOUNDATION_INVALID);

			case TRACK_BIT_VERT:
				if (tileh == SLOPE_W) return HalftileFoundation(CORNER_W);
				if (tileh == SLOPE_E) return HalftileFoundation(CORNER_E);
				return (valid_on_leveled ? FOUNDATION_LEVELED : FOUNDATION_INVALID);

			case TRACK_BIT_X:
				if (IsSlopeWithOneCornerRaised(tileh)) return FOUNDATION_INCLINED_X;
				return (valid_on_leveled ? FOUNDATION_LEVELED : FOUNDATION_INVALID);

			case TRACK_BIT_Y:
				if (IsSlopeWithOneCornerRaised(tileh)) return FOUNDATION_INCLINED_Y;
				return (valid_on_leveled ? FOUNDATION_LEVELED : FOUNDATION_INVALID);

			default:
				return (valid_on_leveled ? FOUNDATION_LEVELED : FOUNDATION_INVALID);
		}
		/* Single diagonal track */

		/* Track must be at least valid on leveled foundation */
		if (!valid_on_leveled) return FOUNDATION_INVALID;

		/* If slope has three raised corners, build leveled foundation */
		if (IsSlopeWithThreeCornersRaised(tileh)) return FOUNDATION_LEVELED;

		/* If neighboured corners of track_corner are lowered, build halftile foundation */
		if ((tileh & SlopeWithThreeCornersRaised(OppositeCorner(track_corner))) == SlopeWithOneCornerRaised(track_corner)) return HalftileFoundation(track_corner);

		/* else special anti-zig-zag foundation */
		return SpecialRailFoundation(track_corner);
	}
}


/**
 * Tests if a track can be build on a tile.
 *
 * @param tileh Tile slope.
 * @param rail_bits Tracks to build.
 * @param existing Tracks already built.
 * @param tile Tile (used for water test)
 * @return Error message or cost for foundation building.
 */
static CommandCost CheckRailSlope(Slope tileh, TrackBits rail_bits, TrackBits existing, TileIndex tile)
{
	/* don't allow building on the lower side of a coast */
	if (GetFloodingBehaviour(tile) != FLOOD_NONE) {
		if (!IsSteepSlope(tileh) && ((~_valid_tracks_on_leveled_foundation[tileh] & (rail_bits | existing)) != 0)) return_cmd_error(STR_ERROR_CAN_T_BUILD_ON_WATER);
	}

	Foundation f_new = GetRailFoundation(tileh, rail_bits | existing);

	/* check track/slope combination */
	if ((f_new == FOUNDATION_INVALID) ||
			((f_new != FOUNDATION_NONE) && (!_settings_game.construction.build_on_slopes))) {
		return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
	}

	Foundation f_old = GetRailFoundation(tileh, existing);
	return CommandCost(EXPENSES_CONSTRUCTION, f_new != f_old ? _price[PR_BUILD_FOUNDATION] : (Money)0);
}

/* Validate functions for rail building */
static inline bool ValParamTrackOrientation(Track track)
{
	return IsValidTrack(track);
}


/**
 * Check if a given trackbits set is valid for a rail bridge head
 * @param tileh The slope
 * @param dir The bridge direction
 * @param bits The trackbits
 * @return Whether the given combination is valid
 */
bool IsValidRailBridgeBits(Slope tileh, DiagDirection dir, TrackBits bits)
{
	DiagDirDiff diff = CheckExtendedBridgeHead(tileh, dir);

	switch (diff) {
		case DIAGDIRDIFF_SAME: return true;
		case DIAGDIRDIFF_REVERSE: return false;
		default: return (bits & DiagdirReachesTracks(ReverseDiagDir(ChangeDiagDir(dir, diff)))) == 0;
	}
}


/**
 * Build a single piece of rail
 * @param tile tile  to build on
 * @param flags operation to perform
 * @param p1 railtype of being built piece (normal, mono, maglev)
 * @param p2 rail track to build
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdBuildSingleRail(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	RailType railtype = Extract<RailType, 0, 4>(p1);
	Track track = Extract<Track, 0, 3>(p2);
	CommandCost cost(EXPENSES_CONSTRUCTION);

	if (!ValParamRailtype(railtype) || !ValParamTrackOrientation(track)) return CMD_ERROR;

	Slope tileh = GetTileSlope(tile);
	TrackBits trackbit = TrackToTrackBits(track);

	switch (GetTileType(tile)) {
		case TT_RAILWAY: {
			CommandCost ret = CheckTileOwnership(tile);
			if (ret.Failed()) return ret;

			ret = CheckTrackCombination(tile, track, railtype, flags);
			if (ret.Failed()) return ret;
			cost.AddCost(ret);

			if (IsTileSubtype(tile, TT_TRACK)) {
				ret = CheckRailSlope(tileh, trackbit, GetTrackBits(tile), tile);
				if (ret.Failed()) return ret;
				cost.AddCost(ret);
			} else {
				if (!IsValidRailBridgeBits(tileh, GetTunnelBridgeDirection(tile), GetTrackBits(tile) | trackbit)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
			}

			ret = EnsureNoTrainOnTrack(tile, track);
			if (ret.Failed()) return ret;

			if (flags & DC_EXEC) {
				if (IsTileSubtype(tile, TT_TRACK)) SetRailGroundType(tile, RAIL_GROUND_BARREN);
				TrackBits bits = GetTrackBits(tile);
				TrackBits newbits = bits | trackbit;
				SetTrackBits(tile, newbits);

				/* Update infrastructure count. */
				Owner owner = GetTileOwner(tile);
				if (newbits == TRACK_BIT_HORZ || newbits == TRACK_BIT_VERT) {
					Company::Get(owner)->infrastructure.rail[railtype]++;
				} else {
					RailType rt = GetRailType(tile, track);
					if (bits == TRACK_BIT_HORZ || bits == TRACK_BIT_VERT) {
						Company::Get(owner)->infrastructure.rail[rt] -= IsTileSubtype(tile, TT_BRIDGE) ? TUNNELBRIDGE_TRACKBIT_FACTOR + 1 : 2;
					} else {
						uint pieces = CountBits(bits);
						pieces *= pieces;
						if (IsTileSubtype(tile, TT_BRIDGE)) pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
						Company::Get(owner)->infrastructure.rail[rt] -= pieces;
					}
					uint pieces = CountBits(newbits);
					assert(TracksOverlap(newbits));
					pieces *= pieces;
					if (IsTileSubtype(tile, TT_BRIDGE)) pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
					Company::Get(owner)->infrastructure.rail[rt] += pieces;
				}
				DirtyCompanyInfrastructureWindows(owner);
			}
			break;
		}

		case TT_ROAD: {
			if (!IsTileSubtype(tile, TT_TRACK)) goto try_clear;

			/* Level crossings may only be built on these slopes */
			if (!HasBit(VALID_LEVEL_CROSSING_SLOPES, tileh)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);

			CommandCost ret = EnsureNoVehicleOnGround(tile);
			if (ret.Failed()) return ret;

			if (HasRoadWorks(tile)) return_cmd_error(STR_ERROR_ROAD_WORKS_IN_PROGRESS);

			if (GetDisallowedRoadDirections(tile) != DRD_NONE) return_cmd_error(STR_ERROR_CROSSING_ON_ONEWAY_ROAD);

			if (RailNoLevelCrossings(railtype)) return_cmd_error(STR_ERROR_CROSSING_DISALLOWED);

			RoadTypes roadtypes = GetRoadTypes(tile);
			RoadBits road = GetRoadBits(tile, ROADTYPE_ROAD);
			RoadBits tram = GetRoadBits(tile, ROADTYPE_TRAM);
			switch (roadtypes) {
				default: break;
				case ROADTYPES_TRAM:
					/* Tram crossings must always have road. */
					if (flags & DC_EXEC) {
						SetRoadOwner(tile, ROADTYPE_ROAD, _current_company);
						Company *c = Company::GetIfValid(_current_company);
						if (c != NULL) {
							/* A full diagonal tile has two road bits. */
							c->infrastructure.road[ROADTYPE_ROAD] += 2;
							DirtyCompanyInfrastructureWindows(c->index);
						}
					}
					roadtypes |= ROADTYPES_ROAD;
					break;

				case ROADTYPES_ALL:
					if (road != tram) return CMD_ERROR;
					break;
			}

			road |= tram;

			if ((track == TRACK_X && road == ROAD_Y) ||
					(track == TRACK_Y && road == ROAD_X)) {
				if (flags & DC_EXEC) {
					MakeRoadCrossing(tile, GetRoadOwner(tile, ROADTYPE_ROAD), GetRoadOwner(tile, ROADTYPE_TRAM), _current_company, (track == TRACK_X ? AXIS_Y : AXIS_X), railtype, roadtypes, GetTownIndex(tile));
					UpdateLevelCrossing(tile, false);
					Company::Get(_current_company)->infrastructure.rail[railtype] += LEVELCROSSING_TRACKBIT_FACTOR;
					DirtyCompanyInfrastructureWindows(_current_company);
				}
				break;
			}

			goto try_clear;
		}

		case TT_MISC:
			if (IsLevelCrossingTile(tile) && GetCrossingRailBits(tile) == trackbit) {
				return_cmd_error(STR_ERROR_ALREADY_BUILT);
			}
			/* FALL THROUGH */

		try_clear:
		default: {
			/* Will there be flat water on the lower halftile? */
			bool water_ground = IsWaterTile(tile) && IsSlopeWithOneCornerRaised(tileh);

			CommandCost ret = CheckRailSlope(tileh, trackbit, TRACK_BIT_NONE, tile);
			if (ret.Failed()) return ret;
			cost.AddCost(ret);

			ret = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
			if (ret.Failed()) return ret;
			cost.AddCost(ret);

			if (water_ground) {
				cost.AddCost(-_price[PR_CLEAR_WATER]);
				cost.AddCost(_price[PR_CLEAR_ROUGH]);
			}

			if (flags & DC_EXEC) {
				MakeRailNormal(tile, _current_company, trackbit, railtype);
				if (water_ground) SetRailGroundType(tile, RAIL_GROUND_WATER);
				Company::Get(_current_company)->infrastructure.rail[railtype]++;
				DirtyCompanyInfrastructureWindows(_current_company);
			}
			break;
		}
	}

	if (flags & DC_EXEC) {
		MarkTileDirtyByTile(tile);
		AddTrackToSignalBuffer(tile, track, _current_company);
		YapfNotifyTrackLayoutChange(tile, track);
	}

	cost.AddCost(RailBuildCost(railtype));
	return cost;
}

static void NotifyTrackRemoval(TileIndex tile, Track track, bool was_crossing, Owner owner)
{
	if (was_crossing) {
		/* crossing is set when only TRACK_BIT_X and TRACK_BIT_Y are set. As we
		 * are removing one of these pieces, we'll need to update signals for
		 * both directions explicitly, as after the track is removed it won't
		 * 'connect' with the other piece. */
		AddTrackToSignalBuffer(tile, TRACK_X, owner);
		AddTrackToSignalBuffer(tile, TRACK_Y, owner);
		YapfNotifyTrackLayoutChange(tile, TRACK_X);
		YapfNotifyTrackLayoutChange(tile, TRACK_Y);
	} else {
		AddTrackToSignalBuffer(tile, track, owner);
		YapfNotifyTrackLayoutChange(tile, track);
	}
}

/**
 * Remove a single piece of track from a railway tile
 * @param tile tile to remove track from
 * @param track the track to remove
 * @param flags operation to perform
 * @return the cost of this operation or an error
 */
static CommandCost RemoveRailTrack(TileIndex tile, Track track, DoCommandFlag flags)
{
	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckTileOwnership(tile);
		if (ret.Failed()) return ret;
	}

	CommandCost ret = EnsureNoTrainOnTrack(tile, track);
	if (ret.Failed()) return ret;

	TrackBits present = GetTrackBits(tile);
	TrackBits trackbit = TrackToTrackBits(track);
	bool crossing = false;

	if ((present & trackbit) == 0) return_cmd_error(STR_ERROR_THERE_IS_NO_RAILROAD_TRACK);
	if (present == (TRACK_BIT_X | TRACK_BIT_Y)) crossing = true;

	RailType rt = GetRailType(tile, track);
	CommandCost cost(EXPENSES_CONSTRUCTION, RailClearCost(rt));

	/* Charge extra to remove signals on the track, if they are there */
	if (HasSignalOnTrack(tile, track)) {
		cost.AddCost(DoCommand(tile, track, 0, flags, CMD_REMOVE_SIGNALS));
	}

	if (flags & DC_EXEC) {
		Train *v = NULL;

		if (HasReservedTrack(tile, track)) {
			v = GetTrainForReservation(tile, track);
			if (v != NULL) FreeTrainTrackReservation(v);
		}

		Owner owner = GetTileOwner(tile);

		if (TracksOverlap(present)) {
			/* Subtract old infrastructure count. */
			uint pieces = CountBits(present);
			pieces *= pieces;
			if (IsTileSubtype(tile, TT_BRIDGE)) pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
			Company::Get(owner)->infrastructure.rail[rt] -= pieces;
			/* Add new infrastructure count. */
			present ^= trackbit;
			if (present == TRACK_BIT_HORZ || present == TRACK_BIT_VERT) {
				pieces = IsTileSubtype(tile, TT_BRIDGE) ? TUNNELBRIDGE_TRACKBIT_FACTOR + 1 : 2;
			} else {
				pieces = CountBits(present);
				pieces *= pieces;
				if (IsTileSubtype(tile, TT_BRIDGE)) pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
			}
			Company::Get(owner)->infrastructure.rail[rt] += pieces;
		} else {
			Company::Get(owner)->infrastructure.rail[rt]--;
			present ^= trackbit;
		}
		DirtyCompanyInfrastructureWindows(owner);

		if (present == 0) {
			Slope tileh = GetTileSlope(tile);
			/* If there is flat water on the lower halftile, convert the tile to shore so the water remains */
			if (GetRailGroundType(tile) == RAIL_GROUND_WATER && IsSlopeWithOneCornerRaised(tileh)) {
				MakeShore(tile);
			} else {
				DoClearSquare(tile);
			}
			DeleteNewGRFInspectWindow(GSF_RAILTYPES, tile);
		} else {
			SetTrackBits(tile, present);
			SetTrackReservation(tile, GetRailReservationTrackBits(tile) & present);
		}

		MarkTileDirtyByTile(tile);
		NotifyTrackRemoval(tile, track, crossing, owner);

		if (v != NULL) TryPathReserve(v, true);
	}

	return cost;
}

static void RemoveRailBridgeHead(TileIndex tile, TrackBits remove, RailType rt)
{
	Owner owner = GetTileOwner(tile);

	TrackBits bits = GetTrackBits(tile);
	bool crossing = (bits == (TRACK_BIT_X | TRACK_BIT_Y));

	/* Update infrastructure count. */
	if (HasExactlyOneBit(bits)) {
		assert((bits & ~remove) == TRACK_BIT_NONE);
		bits = TRACK_BIT_NONE;
		Company::Get(owner)->infrastructure.rail[rt] -= TUNNELBRIDGE_TRACKBIT_FACTOR;
	} else if (bits != TRACK_BIT_HORZ && bits != TRACK_BIT_VERT) {
		assert(TracksOverlap(bits));
		uint pieces = CountBits(bits);
		Company::Get(owner)->infrastructure.rail[rt] -= pieces * pieces * TUNNELBRIDGE_TRACKBIT_FACTOR;
		bits &= ~remove;
		pieces = CountBits(bits);
		Company::Get(owner)->infrastructure.rail[rt] += pieces * pieces;
	} else if (remove == bits) {
		bits = TRACK_BIT_NONE;
		Company::Get(owner)->infrastructure.rail[rt] -= TUNNELBRIDGE_TRACKBIT_FACTOR;
		Company::Get(owner)->infrastructure.rail[GetSideRailType(tile, ReverseDiagDir(GetTunnelBridgeDirection(tile)))]--;
	} else {
		bits &= ~remove;
		Company::Get(owner)->infrastructure.rail[rt] -= TUNNELBRIDGE_TRACKBIT_FACTOR;
	}

	if (bits == TRACK_BIT_NONE) {
		DoClearSquare(tile);
		DeleteNewGRFInspectWindow(GSF_RAILTYPES, tile);
	} else {
		assert((DiagdirReachesTracks(ReverseDiagDir(GetTunnelBridgeDirection(tile))) & bits) == TRACK_BIT_NONE);
		MakeNormalRailFromBridge(tile);
		SetTrackBits(tile, bits);
		SetTrackReservation(tile, GetRailReservationTrackBits(tile) & bits);
	}

	MarkTileDirtyByTile(tile);

	while (remove != TRACK_BIT_NONE) {
		Track track = RemoveFirstTrack(&remove);
		NotifyTrackRemoval(tile, track, crossing, owner);
	}
}

static void RemoveRailBridge(TileIndex tile, TrackBits remove, TileIndex other_tile, TrackBits other_remove)
{
	SmallVector<Train*, 4> affected;

	TrackBits bits = GetReservedTrackbits(tile);
	while (bits != TRACK_BIT_NONE) {
		Track track = RemoveFirstTrack(&bits);
		if ((TrackToTrackBits(track) & remove) != TRACK_BIT_NONE) {
			Train *v = GetTrainForReservation(tile, track);
			FreeTrainTrackReservation(v);
			*affected.Append() = v;
		}
	}

	bits = GetReservedTrackbits(other_tile);
	while (bits != TRACK_BIT_NONE) {
		Track track = RemoveFirstTrack(&bits);
		if ((TrackToTrackBits(track) & other_remove) != TRACK_BIT_NONE) {
			Train *v = GetTrainForReservation(other_tile, track);
			FreeTrainTrackReservation(v);
			*affected.Append() = v;
		}
	}

	RailType rt = GetBridgeRailType(tile);
	Owner owner = GetTileOwner(tile);
	assert(GetTileOwner(other_tile) == owner);

	RemoveBridgeMiddleTiles(tile, other_tile);
	Company::Get(owner)->infrastructure.rail[rt] -= GetTunnelBridgeLength(tile, other_tile) * TUNNELBRIDGE_TRACKBIT_FACTOR;

	RemoveRailBridgeHead(tile, remove, rt);
	RemoveRailBridgeHead(other_tile, other_remove, rt);

	DirtyCompanyInfrastructureWindows(owner);

	for (uint i = 0; i < affected.Length(); ++i) {
		TryPathReserve(affected[i], true);
	}
}

/**
 * Remove a single piece of track from a rail bridge tile
 * @param tile tile to remove track from
 * @param track the track to remove
 * @param flags operation to perform
 * @return the cost of this operation or an error
 */
static CommandCost RemoveBridgeTrack(TileIndex tile, Track track, DoCommandFlag flags)
{
	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckTileOwnership(tile);
		if (ret.Failed()) return ret;
	}

	DiagDirection dir = GetTunnelBridgeDirection(tile);
	TrackBits present = GetTrackBits(tile);
	TrackBits trackbit = TrackToTrackBits(track);

	if ((present & trackbit) == TRACK_BIT_NONE) return_cmd_error(STR_ERROR_THERE_IS_NO_RAILROAD_TRACK);

	if ((present & DiagdirReachesTracks(ReverseDiagDir(dir)) & ~trackbit) != TRACK_BIT_NONE) {
		return RemoveRailTrack(tile, track, flags);
	}

	/* bridge must be torn down */

	TileIndex other_tile = GetOtherBridgeEnd(tile);
	TrackBits other_present = GetTrackBits(other_tile);
	TrackBits other_remove = other_present & DiagdirReachesTracks(dir);

	assert(other_remove != TRACK_BIT_NONE);

	CommandCost ret = EnsureNoTrainOnBridgeTrackBits(tile, trackbit, other_tile, other_remove);
	if (ret.Failed()) return ret;

	CommandCost cost(EXPENSES_CONSTRUCTION, (GetTunnelBridgeLength(tile, other_tile) + 2) * _price[PR_CLEAR_BRIDGE]);

	/* Charge extra to remove signals on the track, if they are there */
	if (HasSignalOnTrack(tile, track)) {
		cost.AddCost(DoCommand(tile, track, 0, flags, CMD_REMOVE_SIGNALS));
	}

	int n = CountBits(other_remove);
	if (n == 1) {
		Track other_track = FindFirstTrack(other_remove);
		if (HasSignalOnTrack(other_tile, other_track)) {
			cost.AddCost(DoCommand(other_tile, other_track, 0, flags, CMD_REMOVE_SIGNALS));
		}
	} else {
		assert(GetRailType(tile, track) == GetBridgeRailType(other_tile));
		cost.AddCost((n - 1) * RailClearCost(GetRailType(tile, track)));
	}

	if (flags & DC_EXEC) {
		RemoveRailBridge(tile, trackbit, other_tile, other_remove);
	}

	return cost;
}

/**
 * Remove the rail track from a crossing
 * @param tile tile to remove track from
 * @param flags operation to perform
 * @return the cost of this operation or an error
 */
static CommandCost RemoveCrossingTrack(TileIndex tile, DoCommandFlag flags)
{
	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckTileOwnership(tile);
		if (ret.Failed()) return ret;
	}

	if (!(flags & DC_BANKRUPT)) {
		CommandCost ret = EnsureNoVehicleOnGround(tile);
		if (ret.Failed()) return ret;
	}

	CommandCost cost(EXPENSES_CONSTRUCTION, RailClearCost(GetRailType(tile)));

	if (flags & DC_EXEC) {
		Track track = GetCrossingRailTrack(tile);
		Train *v = NULL;

		if (HasCrossingReservation(tile)) {
			v = GetTrainForReservation(tile, track);
			if (v != NULL) FreeTrainTrackReservation(v);
		}

		Owner owner = GetTileOwner(tile);
		Company::Get(owner)->infrastructure.rail[GetRailType(tile)] -= LEVELCROSSING_TRACKBIT_FACTOR;
		DirtyCompanyInfrastructureWindows(owner);
		MakeRoadNormal(tile, GetCrossingRoadBits(tile), GetRoadTypes(tile), GetTownIndex(tile), GetRoadOwner(tile, ROADTYPE_ROAD), GetRoadOwner(tile, ROADTYPE_TRAM));
		DeleteNewGRFInspectWindow(GSF_RAILTYPES, tile);

		MarkTileDirtyByTile(tile);

		AddTrackToSignalBuffer(tile, track, owner);
		YapfNotifyTrackLayoutChange(tile, track);

		if (v != NULL) TryPathReserve(v, true);
	}

	return cost;
}

/**
 * Remove a single piece of track
 * @param tile tile to remove track from
 * @param flags operation to perform
 * @param p1 unused
 * @param p2 rail orientation
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdRemoveSingleRail(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Track track = Extract<Track, 0, 3>(p2);

	if (!ValParamTrackOrientation(track)) return CMD_ERROR;

	switch (GetTileType(tile)) {
		case TT_MISC:
			if (!IsLevelCrossingTile(tile) || GetCrossingRailTrack(tile) != track) break;
			return RemoveCrossingTrack(tile, flags);

		case TT_RAILWAY:
			if (IsTileSubtype(tile, TT_BRIDGE)) {
				return RemoveBridgeTrack(tile, track, flags);
			} else {
				return RemoveRailTrack(tile, track, flags);
			}

		default: break;
	}

	return_cmd_error(STR_ERROR_THERE_IS_NO_RAILROAD_TRACK);
}


/**
 * Called from water_cmd if a non-flat rail-tile gets flooded and should be converted to shore.
 * The function floods the lower halftile, if the tile has a halftile foundation.
 *
 * @param t The tile to flood.
 * @return true if something was flooded.
 */
bool FloodHalftile(TileIndex t)
{
	assert(IsNormalRailTile(t));

	bool flooded = false;
	if (GetRailGroundType(t) == RAIL_GROUND_WATER) return flooded;

	Slope tileh = GetTileSlope(t);
	TrackBits rail_bits = GetTrackBits(t);

	if (IsSlopeWithOneCornerRaised(tileh)) {
		TrackBits lower_track = CornerToTrackBits(OppositeCorner(GetHighestSlopeCorner(tileh)));

		TrackBits to_remove = lower_track & rail_bits;
		if (to_remove != 0) {
			Backup<CompanyByte> cur_company(_current_company, OWNER_WATER, FILE_LINE);
			flooded = DoCommand(t, 0, FIND_FIRST_BIT(to_remove), DC_EXEC, CMD_REMOVE_SINGLE_RAIL).Succeeded();
			cur_company.Restore();
			if (!flooded) return flooded; // not yet floodable
			rail_bits = rail_bits & ~to_remove;
			if (rail_bits == 0) {
				MakeShore(t);
				MarkTileDirtyByTile(t);
				return flooded;
			}
		}

		if (IsNonContinuousFoundation(GetRailFoundation(tileh, rail_bits))) {
			flooded = true;
			SetRailGroundType(t, RAIL_GROUND_WATER);
			MarkTileDirtyByTile(t);
		}
	} else {
		/* Make shore on steep slopes and 'three-corners-raised'-slopes. */
		if (ApplyFoundationToSlope(GetRailFoundation(tileh, rail_bits), &tileh) == 0) {
			if (IsSteepSlope(tileh) || IsSlopeWithThreeCornersRaised(tileh)) {
				flooded = true;
				SetRailGroundType(t, RAIL_GROUND_WATER);
				MarkTileDirtyByTile(t);
			}
		}
	}
	return flooded;
}

static const TileIndexDiffC _trackdelta[] = {
	{ -1,  0 }, {  0,  1 }, { -1,  0 }, {  0,  1 }, {  1,  0 }, {  0,  1 },
	{  0,  0 },
	{  0,  0 },
	{  1,  0 }, {  0, -1 }, {  0, -1 }, {  1,  0 }, {  0, -1 }, { -1,  0 },
	{  0,  0 },
	{  0,  0 }
};


static CommandCost ValidateAutoDrag(Trackdir *trackdir, TileIndex start, TileIndex end)
{
	int x = TileX(start);
	int y = TileY(start);
	int ex = TileX(end);
	int ey = TileY(end);

	if (!ValParamTrackOrientation(TrackdirToTrack(*trackdir))) return CMD_ERROR;

	/* calculate delta x,y from start to end tile */
	int dx = ex - x;
	int dy = ey - y;

	/* calculate delta x,y for the first direction */
	int trdx = _trackdelta[*trackdir].x;
	int trdy = _trackdelta[*trackdir].y;

	if (!IsDiagonalTrackdir(*trackdir)) {
		trdx += _trackdelta[*trackdir ^ 1].x;
		trdy += _trackdelta[*trackdir ^ 1].y;
	}

	/* validate the direction */
	while ((trdx <= 0 && dx > 0) ||
			(trdx >= 0 && dx < 0) ||
			(trdy <= 0 && dy > 0) ||
			(trdy >= 0 && dy < 0)) {
		if (!HasBit(*trackdir, 3)) { // first direction is invalid, try the other
			SetBit(*trackdir, 3); // reverse the direction
			trdx = -trdx;
			trdy = -trdy;
		} else { // other direction is invalid too, invalid drag
			return CMD_ERROR;
		}
	}

	/* (for diagonal tracks, this is already made sure of by above test), but:
	 * for non-diagonal tracks, check if the start and end tile are on 1 line */
	if (!IsDiagonalTrackdir(*trackdir)) {
		trdx = _trackdelta[*trackdir].x;
		trdy = _trackdelta[*trackdir].y;
		if (abs(dx) != abs(dy) && abs(dx) + abs(trdy) != abs(dy) + abs(trdx)) return CMD_ERROR;
	}

	return CommandCost();
}

/**
 * Build or remove a stretch of railroad tracks.
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit 0-3) - railroad type normal/maglev (0 = normal, 1 = mono, 2 = maglev)
 * - p2 = (bit 4-6) - track-orientation, valid values: 0-5 (Track enum)
 * - p2 = (bit 7)   - 0 = build, 1 = remove tracks
 * - p2 = (bit 8)   - 0 = build up to an obstacle, 1 = fail if an obstacle is found (used for AIs).
 * @param text unused
 * @return the cost of this operation or an error
 */
static CommandCost CmdRailTrackHelper(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	CommandCost total_cost(EXPENSES_CONSTRUCTION);
	Track track = Extract<Track, 4, 3>(p2);
	bool remove = HasBit(p2, 7);
	RailType railtype = Extract<RailType, 0, 4>(p2);

	if (!ValParamRailtype(railtype) || !ValParamTrackOrientation(track)) return CMD_ERROR;
	if (p1 >= MapSize()) return CMD_ERROR;
	TileIndex end_tile = p1;
	Trackdir trackdir = TrackToTrackdir(track);

	CommandCost ret = ValidateAutoDrag(&trackdir, tile, end_tile);
	if (ret.Failed()) return ret;

	if ((flags & DC_EXEC) && _settings_client.sound.confirm) SndPlayTileFx(SND_20_SPLAT_2, tile);

	bool had_success = false;
	CommandCost last_error = CMD_ERROR;
	bool seen_bridgehead = false;
	for (;;) {
		if (seen_bridgehead && IsRailBridgeTile(tile) && DiagDirToDiagTrackdir(ReverseDiagDir(GetTunnelBridgeDirection(tile))) == trackdir) {
			seen_bridgehead = false;
		} else {
			CommandCost ret = DoCommand(tile, railtype, TrackdirToTrack(trackdir), flags, remove ? CMD_REMOVE_SINGLE_RAIL : CMD_BUILD_SINGLE_RAIL);

			if (ret.Failed()) {
				last_error = ret;
				if (last_error.GetErrorMessage() != STR_ERROR_ALREADY_BUILT && !remove) {
					if (HasBit(p2, 8)) return last_error;
					break;
				}

				/* Ownership errors are more important. */
				if (last_error.GetErrorMessage() == STR_ERROR_OWNED_BY && remove) break;
			} else {
				had_success = true;
				total_cost.AddCost(ret);
			}
		}

		if (IsRailBridgeTile(tile) && DiagDirToDiagTrackdir(GetTunnelBridgeDirection(tile)) == trackdir) {
			seen_bridgehead = true;
		}

		if (tile == end_tile) break;

		tile += ToTileIndexDiff(_trackdelta[trackdir]);

		/* toggle railbit for the non-diagonal tracks */
		if (!IsDiagonalTrackdir(trackdir)) ToggleBit(trackdir, 0);
	}

	if (had_success) return total_cost;
	return last_error;
}

/**
 * Build rail on a stretch of track.
 * Stub for the unified rail builder/remover
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit 0-3) - railroad type normal/maglev (0 = normal, 1 = mono, 2 = maglev)
 * - p2 = (bit 4-6) - track-orientation, valid values: 0-5 (Track enum)
 * - p2 = (bit 7)   - 0 = build, 1 = remove tracks
 * @param text unused
 * @return the cost of this operation or an error
 * @see CmdRailTrackHelper
 */
CommandCost CmdBuildRailroadTrack(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	return CmdRailTrackHelper(tile, flags, p1, ClrBit(p2, 7), text);
}

/**
 * Build rail on a stretch of track.
 * Stub for the unified rail builder/remover
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1 end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit 0-3) - railroad type normal/maglev (0 = normal, 1 = mono, 2 = maglev)
 * - p2 = (bit 4-6) - track-orientation, valid values: 0-5 (Track enum)
 * - p2 = (bit 7)   - 0 = build, 1 = remove tracks
 * @param text unused
 * @return the cost of this operation or an error
 * @see CmdRailTrackHelper
 */
CommandCost CmdRemoveRailroadTrack(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	return CmdRailTrackHelper(tile, flags, p1, SetBit(p2, 7), text);
}

/**
 * Build a train depot
 * @param tile position of the train depot
 * @param flags operation to perform
 * @param p1 rail type
 * @param p2 bit 0..1 entrance direction (DiagDirection)
 * @param text unused
 * @return the cost of this operation or an error
 *
 * @todo When checking for the tile slope,
 * distinguish between "Flat land required" and "land sloped in wrong direction"
 */
CommandCost CmdBuildTrainDepot(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	/* check railtype and valid direction for depot (0 through 3), 4 in total */
	RailType railtype = Extract<RailType, 0, 4>(p1);
	if (!ValParamRailtype(railtype)) return CMD_ERROR;

	Slope tileh = GetTileSlope(tile);

	DiagDirection dir = Extract<DiagDirection, 0, 2>(p2);

	/* Prohibit construction if
	 * The tile is non-flat AND
	 * 1) build-on-slopes is disabled
	 * 2) the tile is steep i.e. spans two height levels
	 * 3) the exit points in the wrong direction
	 */

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
		Depot *d = new Depot(tile);
		d->build_date = _date;

		MakeRailDepot(tile, _current_company, d->index, dir, railtype);
		MarkTileDirtyByTile(tile);
		MakeDefaultName(d);

		Company::Get(_current_company)->infrastructure.rail[railtype]++;
		DirtyCompanyInfrastructureWindows(_current_company);

		AddDepotToSignalBuffer(tile, _current_company);
		YapfNotifyTrackLayoutChange(tile, DiagDirToDiagTrack(dir));
	}

	cost.AddCost(_price[PR_BUILD_DEPOT_TRAIN]);
	cost.AddCost(RailBuildCost(railtype));
	return cost;
}

/**
 * Build signals, alternate between double/single, signal/semaphore,
 * pre/exit/combo-signals, and what-else not. If the rail piece does not
 * have any signals, bit 4 (cycle signal-type) is ignored
 * @param tile tile where to build the signals
 * @param flags operation to perform
 * @param p1 various bitstuffed elements
 * - p1 = (bit 0-2) - track-orientation, valid values: 0-5 (Track enum)
 * - p1 = (bit 3)   - 1 = override signal/semaphore, or pre/exit/combo signal or (for bit 7) toggle variant (CTRL-toggle)
 * - p1 = (bit 4)   - 0 = signals, 1 = semaphores
 * - p1 = (bit 5-7) - type of the signal, for valid values see enum SignalType in rail_map.h
 * - p1 = (bit 8)   - convert the present signal type and variant
 * - p1 = (bit 9-11)- start cycle from this signal type
 * - p1 = (bit 12-14)-wrap around after this signal type
 * - p1 = (bit 15-16)-cycle the signal direction this many times
 * - p1 = (bit 17)  - 1 = don't modify an existing signal but don't fail either, 0 = always set new signal type
 * @param p2 used for CmdBuildManySignals() to copy direction of first signal
 * @param text unused
 * @return the cost of this operation or an error
 * @todo p2 should be replaced by two bits for "along" and "against" the track.
 */
CommandCost CmdBuildSingleSignal(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Track track = Extract<Track, 0, 3>(p1);
	bool ctrl_pressed = HasBit(p1, 3); // was the CTRL button pressed
	SignalVariant sigvar = (ctrl_pressed ^ HasBit(p1, 4)) ? SIG_SEMAPHORE : SIG_ELECTRIC; // the signal variant of the new signal
	SignalType sigtype = Extract<SignalType, 5, 3>(p1); // the signal type of the new signal
	bool convert_signal = HasBit(p1, 8); // convert button pressed
	SignalType cycle_start = Extract<SignalType, 9, 3>(p1);
	SignalType cycle_stop = Extract<SignalType, 12, 3>(p1);
	uint num_dir_cycle = GB(p1, 15, 2);

	if (sigtype > SIGTYPE_LAST) return CMD_ERROR;
	if (cycle_start > cycle_stop || cycle_stop > SIGTYPE_LAST) return CMD_ERROR;

	/* You can only build signals on rail tiles, and the selected track must exist */
	if (!ValParamTrackOrientation(track) || !IsRailwayTile(tile) ||
			!HasTrack(tile, track)) {
		return_cmd_error(STR_ERROR_THERE_IS_NO_RAILROAD_TRACK);
	}
	/* Protect against invalid signal copying */
	if (p2 > 3) return CMD_ERROR;

	CommandCost ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;

	/* See if this is a valid track combination for signals (no overlap) */
	if (TracksOverlap(GetTrackBits(tile))) return_cmd_error(STR_ERROR_NO_SUITABLE_RAILROAD_TRACK);

	/* In case we don't want to change an existing signal, return without error. */
	if (HasBit(p1, 17) && HasSignalOnTrack(tile, track)) return CommandCost();

	/* you can not convert a signal if no signal is on track */
	if (convert_signal && !HasSignalOnTrack(tile, track)) return_cmd_error(STR_ERROR_THERE_ARE_NO_SIGNALS);

	CommandCost cost;
	if (!HasSignalOnTrack(tile, track)) {
		/* build new signals */
		cost = CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_SIGNALS]);
	} else {
		if (p2 != 0 && sigvar != GetSignalVariant(tile, track)) {
			/* convert signals <-> semaphores */
			cost = CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_SIGNALS] + _price[PR_CLEAR_SIGNALS]);

		} else if (convert_signal) {
			/* convert button pressed */
			if (ctrl_pressed || GetSignalVariant(tile, track) != sigvar) {
				/* convert electric <-> semaphore */
				cost = CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_SIGNALS] + _price[PR_CLEAR_SIGNALS]);
			} else {
				/* it is free to change signal type: normal-pre-exit-combo */
				cost = CommandCost();
			}

		} else {
			/* it is free to change orientation/pre-exit-combo signals */
			cost = CommandCost();
		}
	}

	if (flags & DC_EXEC) {
		Train *v = NULL;
		/* The new/changed signal could block our path. As this can lead to
		 * stale reservations, we clear the path reservation here and try
		 * to redo it later on. */
		if (HasReservedTrack(tile, track)) {
			v = GetTrainForReservation(tile, track);
			if (v != NULL) FreeTrainTrackReservation(v);
		}

		/* Subtract old signal infrastructure count. */
		Company::Get(GetTileOwner(tile))->infrastructure.signal -= CountBits(GetPresentSignals(tile, track));

		if (p2 == 0) {
			if (!HasSignalOnTrack(tile, track)) {
				/* build new signals */
				SetPresentSignals(tile, track, IsPbsSignal(sigtype) ? 1 : 3);
				SetSignalType(tile, track, sigtype);
				SetSignalStates(tile, track, 3);
				SetSignalVariant(tile, track, sigvar);
				while (num_dir_cycle-- > 0) CycleSignalSide(tile, track);
			} else {
				if (convert_signal) {
					/* convert signal button pressed */
					if (ctrl_pressed) {
						/* toggle the present signal variant: SIG_ELECTRIC <-> SIG_SEMAPHORE */
						SetSignalVariant(tile, track, (GetSignalVariant(tile, track) == SIG_ELECTRIC) ? SIG_SEMAPHORE : SIG_ELECTRIC);
						/* Query current signal type so the check for PBS signals below works. */
						sigtype = GetSignalType(tile, track);
					} else {
						/* convert the present signal to the chosen type and variant */
						SetSignalType(tile, track, sigtype);
						SetSignalVariant(tile, track, sigvar);
						if (IsPbsSignal(sigtype) && (GetPresentSignals(tile, track) == 3)) {
							SetPresentSignals(tile, track, 1);
						}
					}

				} else if (ctrl_pressed) {
					/* cycle between cycle_start and cycle_end */
					sigtype = (SignalType)(GetSignalType(tile, track) + 1);

					if (sigtype < cycle_start || sigtype > cycle_stop) sigtype = cycle_start;

					SetSignalType(tile, track, sigtype);
					if (IsPbsSignal(sigtype) && GetPresentSignals(tile, track) == 3) {
						SetPresentSignals(tile, track, 1);
					}
				} else {
					/* cycle the signal side: both -> left -> right -> both -> ... */
					CycleSignalSide(tile, track);
					/* Query current signal type so the check for PBS signals below works. */
					sigtype = GetSignalType(tile, track);
				}
			}
		} else {
			if (!HasSignalOnTrack(tile, track)) {
				/* there are no signals at all on this track yet */
				SetSignalStates(tile, track, 3);
			}

			/* If CmdBuildManySignals is called with copying signals, just copy the
			 * direction of the first signal given as parameter by CmdBuildManySignals */
			SetPresentSignals(tile, track, p2);
			SetSignalVariant(tile, track, sigvar);
			SetSignalType(tile, track, sigtype);
		}

		/* Add new signal infrastructure count. */
		Company::Get(GetTileOwner(tile))->infrastructure.signal += CountBits(GetPresentSignals(tile, track));
		DirtyCompanyInfrastructureWindows(GetTileOwner(tile));

		if (IsPbsSignal(sigtype)) {
			/* PBS signals should show red unless they are on reserved tiles without a train. */
			uint mask = GetPresentSignals(tile, track);
			uint state = GetSignalStates(tile, track);
			SetSignalStates(tile, track, HasBit(GetRailReservationTrackBits(tile), track) && EnsureNoTrainOnTrack(tile, track).Succeeded() ? (state | mask) : (state & ~mask));
		}
		MarkTileDirtyByTile(tile);
		AddTrackToSignalBuffer(tile, track, _current_company);
		YapfNotifyTrackLayoutChange(tile, track);
		if (v != NULL) {
			/* Extend the train's path if it's not stopped or loading, or not at a safe position. */
			if (!(((v->vehstatus & VS_STOPPED) && v->cur_speed == 0) || v->current_order.IsType(OT_LOADING)) ||
					!IsSafeWaitingPosition(v, v->GetPos(), _settings_game.pf.forbid_90_deg)) {
				TryPathReserve(v, true);
			}
		}
	}

	return cost;
}

static bool CheckSignalAutoFill(TileIndex &tile, Trackdir &trackdir, int &signal_ctr, bool remove)
{
	tile = AddTileIndexDiffCWrap(tile, _trackdelta[trackdir]);
	if (tile == INVALID_TILE) return false;

	/* Check for track bits on the new tile */
	TrackdirBits trackdirbits = TrackStatusToTrackdirBits(GetTileTrackStatus(tile, TRANSPORT_RAIL, 0));

	if (TracksOverlap(TrackdirBitsToTrackBits(trackdirbits))) return false;
	trackdirbits &= TrackdirReachesTrackdirs(trackdir);

	/* No track bits, must stop */
	if (trackdirbits == TRACKDIR_BIT_NONE) return false;

	/* Get the first track dir */
	trackdir = RemoveFirstTrackdir(&trackdirbits);

	/* Any left? It's a junction so we stop */
	if (trackdirbits != TRACKDIR_BIT_NONE) return false;

	switch (GetTileType(tile)) {
		case TT_RAILWAY:
			if (!IsTileSubtype(tile, TT_TRACK)) goto bridge;
			if (!remove && HasSignalOnTrack(tile, TrackdirToTrack(trackdir))) return false;
			signal_ctr++;
			if (IsDiagonalTrackdir(trackdir)) {
				signal_ctr++;
				/* Ensure signal_ctr even so X and Y pieces get signals */
				ClrBit(signal_ctr, 0);
			}
			return true;

		case TT_MISC:
			if (IsLevelCrossingTile(tile)) {
				signal_ctr += 2;
				return true;
			} else if (!IsTunnelTile(tile)) return false;

			if (GetTunnelTransportType(tile) != TRANSPORT_RAIL) return false;
		bridge:;{
			TileIndex orig_tile = tile; // backup old value

			if (GetTunnelBridgeDirection(tile) != TrackdirToExitdir(trackdir)) return false;

			/* Skip to end of tunnel or bridge
			 * note that tile is a parameter by reference, so it must be updated */
			tile = GetOtherTunnelBridgeEnd(tile);

			signal_ctr += (GetTunnelBridgeLength(orig_tile, tile) + 2) * 2;
			return true;
		}

		default: return false;
	}
}

/**
 * Build many signals by dragging; AutoSignals
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1  end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit  0- 2) - track-orientation, valid values: 0-5 (Track enum)
 * - p2 = (bit  3)    - 1 = override signal/semaphore, or pre/exit/combo signal (CTRL-toggle)
 * - p2 = (bit  4)    - 0 = signals, 1 = semaphores
 * - p2 = (bit  5)    - 0 = build, 1 = remove signals
 * - p2 = (bit  6)    - 0 = selected stretch, 1 = auto fill
 * - p2 = (bit  7- 9) - default signal type
 * - p2 = (bit 10)    - 0 = keep fixed distance, 1 = minimise gaps between signals
 * - p2 = (bit 24-31) - user defined signals_density
 * @param text unused
 * @return the cost of this operation or an error
 */
static CommandCost CmdSignalTrackHelper(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	CommandCost total_cost(EXPENSES_CONSTRUCTION);
	TileIndex start_tile = tile;

	Track track = Extract<Track, 0, 3>(p2);
	bool mode = HasBit(p2, 3);
	bool semaphores = HasBit(p2, 4);
	bool remove = HasBit(p2, 5);
	bool autofill = HasBit(p2, 6);
	bool minimise_gaps = HasBit(p2, 10);
	byte signal_density = GB(p2, 24, 8);

	if (p1 >= MapSize() || !ValParamTrackOrientation(track)) return CMD_ERROR;
	TileIndex end_tile = p1;
	if (signal_density == 0 || signal_density > 20) return CMD_ERROR;

	if (!IsRailwayTile(tile)) return_cmd_error(STR_ERROR_THERE_IS_NO_RAILROAD_TRACK);

	/* for vertical/horizontal tracks, double the given signals density
	 * since the original amount will be too dense (shorter tracks) */
	signal_density *= 2;

	Trackdir trackdir = TrackToTrackdir(track);
	CommandCost ret = ValidateAutoDrag(&trackdir, tile, end_tile);
	if (ret.Failed()) return ret;

	track = TrackdirToTrack(trackdir); // trackdir might have changed, keep track in sync
	Trackdir start_trackdir = trackdir;

	/* Must start on a valid track to be able to avoid loops */
	if (!HasTrack(tile, track)) return CMD_ERROR;

	SignalType sigtype = (SignalType)GB(p2, 7, 3);
	if (sigtype > SIGTYPE_LAST) return CMD_ERROR;

	byte signals;
	/* copy the signal-style of the first rail-piece if existing */
	if (HasSignalOnTrack(tile, track)) {
		signals = GetPresentSignals(tile, track);
		assert(signals != 0);

		/* copy signal/semaphores style (independent of CTRL) */
		semaphores = GetSignalVariant(tile, track) != SIG_ELECTRIC;

		sigtype = GetSignalType(tile, track);
		/* Don't but copy entry or exit-signal type */
		if (sigtype == SIGTYPE_ENTRY || sigtype == SIGTYPE_EXIT) sigtype = SIGTYPE_NORMAL;
	} else { // no signals exist, drag a two-way signal stretch
		signals = IsPbsSignal(sigtype) ? SignalBit(trackdir) : 3;
	}

	bool along   = (signals & SignalBit(trackdir)) != 0;
	bool against = (signals & SignalBit(ReverseTrackdir(trackdir))) != 0;

	/* signal_ctr         - amount of tiles already processed
	 * last_used_ctr      - amount of tiles before previously placed signal
	 * signals_density    - setting to put signal on every Nth tile (double space on |, -- tracks)
	 * last_suitable_ctr  - amount of tiles before last possible signal place
	 * last_suitable_tile - last tile where it is possible to place a signal
	 * last_suitable_trackdir - trackdir of the last tile
	 **********
	 * trackdir   - trackdir to build with autorail
	 * semaphores - semaphores or signals
	 * signals    - is there a signal/semaphore on the first tile, copy its style (two-way/single-way)
	 *              and convert all others to semaphore/signal
	 * remove     - 1 remove signals, 0 build signals */
	int signal_ctr = 0;
	int last_used_ctr = INT_MIN; // initially INT_MIN to force building/removing at the first tile
	int last_suitable_ctr = 0;
	TileIndex last_suitable_tile = INVALID_TILE;
	Trackdir last_suitable_trackdir = INVALID_TRACKDIR;
	CommandCost last_error = CMD_ERROR;
	bool had_success = false;
	for (;;) {
		/* only build/remove signals with the specified density */
		if (remove || minimise_gaps || signal_ctr % signal_density == 0) {
			uint32 p1 = GB(TrackdirToTrack(trackdir), 0, 3);
			SB(p1, 3, 1, mode);
			SB(p1, 4, 1, semaphores);
			SB(p1, 5, 3, sigtype);
			if (!remove && signal_ctr == 0) SetBit(p1, 17);

			/* Pick the correct orientation for the track direction */
			signals = 0;
			if (along)   signals |= SignalBit(trackdir);
			if (against) signals |= SignalBit(ReverseTrackdir(trackdir));

			/* Test tiles in between for suitability as well if minimising gaps. */
			bool test_only = !remove && minimise_gaps && signal_ctr < (last_used_ctr + signal_density);
			CommandCost ret = DoCommand(tile, p1, signals, test_only ? flags & ~DC_EXEC : flags, remove ? CMD_REMOVE_SIGNALS : CMD_BUILD_SIGNALS);

			if (ret.Succeeded()) {
				/* Remember last track piece where we can place a signal. */
				last_suitable_ctr = signal_ctr;
				last_suitable_tile = tile;
				last_suitable_trackdir = trackdir;
			} else if (!test_only && last_suitable_tile != INVALID_TILE) {
				/* If a signal can't be placed, place it at the last possible position. */
				SB(p1, 0, 3, TrackdirToTrack(last_suitable_trackdir));
				ClrBit(p1, 17);

				/* Pick the correct orientation for the track direction. */
				signals = 0;
				if (along)   signals |= SignalBit(last_suitable_trackdir);
				if (against) signals |= SignalBit(ReverseTrackdir(last_suitable_trackdir));

				ret = DoCommand(last_suitable_tile, p1, signals, flags, remove ? CMD_REMOVE_SIGNALS : CMD_BUILD_SIGNALS);
			}

			/* Collect cost. */
			if (!test_only) {
				/* Be user-friendly and try placing signals as much as possible */
				if (ret.Succeeded()) {
					had_success = true;
					total_cost.AddCost(ret);
					last_used_ctr = last_suitable_ctr;
					last_suitable_tile = INVALID_TILE;
				} else {
					/* The "No railway" error is the least important one. */
					if (ret.GetErrorMessage() != STR_ERROR_THERE_IS_NO_RAILROAD_TRACK ||
							last_error.GetErrorMessage() == INVALID_STRING_ID) {
						last_error = ret;
					}
				}
			}
		}

		if (autofill) {
			if (!CheckSignalAutoFill(tile, trackdir, signal_ctr, remove)) break;

			/* Prevent possible loops */
			if (tile == start_tile && trackdir == start_trackdir) break;
		} else {
			if (tile == end_tile) break;

			tile += ToTileIndexDiff(_trackdelta[trackdir]);
			signal_ctr++;

			/* toggle railbit for the non-diagonal tracks (|, -- tracks) */
			if (IsDiagonalTrackdir(trackdir)) {
				signal_ctr++;
			} else {
				ToggleBit(trackdir, 0);
			}
		}
	}

	return had_success ? total_cost : last_error;
}

/**
 * Build signals on a stretch of track.
 * Stub for the unified signal builder/remover
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1  end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit  0- 2) - track-orientation, valid values: 0-5 (Track enum)
 * - p2 = (bit  3)    - 1 = override signal/semaphore, or pre/exit/combo signal (CTRL-toggle)
 * - p2 = (bit  4)    - 0 = signals, 1 = semaphores
 * - p2 = (bit  5)    - 0 = build, 1 = remove signals
 * - p2 = (bit  6)    - 0 = selected stretch, 1 = auto fill
 * - p2 = (bit  7- 9) - default signal type
 * - p2 = (bit 24-31) - user defined signals_density
 * @param text unused
 * @return the cost of this operation or an error
 * @see CmdSignalTrackHelper
 */
CommandCost CmdBuildSignalTrack(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	return CmdSignalTrackHelper(tile, flags, p1, p2, text);
}

/**
 * Remove signals
 * @param tile coordinates where signal is being deleted from
 * @param flags operation to perform
 * @param p1 various bitstuffed elements, only track information is used
 *           - (bit  0- 2) - track-orientation, valid values: 0-5 (Track enum)
 *           - (bit  3)    - override signal/semaphore, or pre/exit/combo signal (CTRL-toggle)
 *           - (bit  4)    - 0 = signals, 1 = semaphores
 * @param p2 unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdRemoveSingleSignal(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Track track = Extract<Track, 0, 3>(p1);

	if (!ValParamTrackOrientation(track) || !IsRailwayTile(tile) || !HasTrack(tile, track)) {
		return_cmd_error(STR_ERROR_THERE_IS_NO_RAILROAD_TRACK);
	}
	if (!HasSignalOnTrack(tile, track)) {
		return_cmd_error(STR_ERROR_THERE_ARE_NO_SIGNALS);
	}

	/* Only water can remove signals from anyone */
	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckTileOwnership(tile);
		if (ret.Failed()) return ret;
	}

	/* Do it? */
	if (flags & DC_EXEC) {
		Train *v = NULL;
		if (HasReservedTrack(tile, track)) {
			v = GetTrainForReservation(tile, track);
		} else if (IsPbsSignal(GetSignalType(tile, track))) {
			/* PBS signal, might be the end of a path reservation. */
			Trackdir td = TrackToTrackdir(track);
			for (int i = 0; v == NULL && i < 2; i++, td = ReverseTrackdir(td)) {
				/* Only test the active signal side. */
				if (!HasSignalOnTrackdir(tile, ReverseTrackdir(td))) continue;
				TileIndex next = TileAddByDiagDir(tile, TrackdirToExitdir(td));
				TrackBits tracks = TrackdirBitsToTrackBits(TrackdirReachesTrackdirs(td));
				if (HasReservedTracks(next, tracks)) {
					v = GetTrainForReservation(next, TrackBitsToTrack(GetReservedTrackbits(next) & tracks));
				}
			}
		}
		Company::Get(GetTileOwner(tile))->infrastructure.signal -= CountBits(GetPresentSignals(tile, track));
		ClearSignals(tile, track);
		DirtyCompanyInfrastructureWindows(GetTileOwner(tile));

		AddTrackToSignalBuffer(tile, track, GetTileOwner(tile));
		YapfNotifyTrackLayoutChange(tile, track);
		if (v != NULL) TryPathReserve(v, false);

		MarkTileDirtyByTile(tile);
	}

	return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_CLEAR_SIGNALS]);
}

/**
 * Remove signals on a stretch of track.
 * Stub for the unified signal builder/remover
 * @param tile start tile of drag
 * @param flags operation to perform
 * @param p1  end tile of drag
 * @param p2 various bitstuffed elements
 * - p2 = (bit  0- 2) - track-orientation, valid values: 0-5 (Track enum)
 * - p2 = (bit  3)    - 1 = override signal/semaphore, or pre/exit/combo signal (CTRL-toggle)
 * - p2 = (bit  4)    - 0 = signals, 1 = semaphores
 * - p2 = (bit  5)    - 0 = build, 1 = remove signals
 * - p2 = (bit  6)    - 0 = selected stretch, 1 = auto fill
 * - p2 = (bit  7- 9) - default signal type
 * - p2 = (bit 24-31) - user defined signals_density
 * @param text unused
 * @return the cost of this operation or an error
 * @see CmdSignalTrackHelper
 */
CommandCost CmdRemoveSignalTrack(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	return CmdSignalTrackHelper(tile, flags, p1, SetBit(p2, 5), text); // bit 5 is remove bit
}

/** Update power of train under which is the railtype being converted */
static Vehicle *UpdateTrainPowerProc(Vehicle *v, void *data)
{
	if (v->type != VEH_TRAIN) return NULL;

	TrainList *affected_trains = static_cast<TrainList*>(data);
	affected_trains->Include(Train::From(v)->First());

	return NULL;
}

/** Check if the given tile track is reserved by a train which will be unpowered on the given railtype.
 *  If it is, remove its reservation and return it. Otherwise, return NULL. */
static inline Train *FindUnpoweredReservationTrain(TileIndex tile, Track track, RailType rt)
{
	Train *v = GetTrainForReservation(tile, track);
	if (v == NULL || HasPowerOnRail(v->railtype, rt)) return NULL;
	/* No power on new rail type, reroute. */
	FreeTrainTrackReservation(v);
	return v;
}

template <typename T>
static inline void FindUnpoweredReservationTrains(T *vector, TileIndex tile, RailType rt)
{
	TrackBits reserved = GetReservedTrackbits(tile);
	Track track;
	while ((track = RemoveFirstTrack(&reserved)) != INVALID_TRACK) {
		Train *v = FindUnpoweredReservationTrain(tile, track, rt);
		if (v != NULL) *vector->Append() = v;
	}
}

/** Check rail tile conversion */
static CommandCost CheckRailConversion(TileIndex tile, RailType totype)
{
	assert(IsRailwayTile(tile));

	/* Trying to convert other's rail */
	CommandCost ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;

	bool ignore_electric = _settings_game.vehicle.disable_elrails && totype == RAILTYPE_RAIL;

	TrackBits trackbits = GetTrackBits(tile);
	CommandCost cost(EXPENSES_CONSTRUCTION);

	RailType type = GetRailType(tile, TRACK_UPPER);

	switch (trackbits) {
		case TRACK_BIT_HORZ:
		case TRACK_BIT_VERT: {
			RailType type2 = GetRailType(tile, TRACK_LOWER);
			if (type != type2) {
				bool ignore1 = type == totype || (ignore_electric && type == RAILTYPE_ELECTRIC);
				bool ignore2 = type2 == totype || (ignore_electric && type2 == RAILTYPE_ELECTRIC);
				if (ignore1 && ignore2) return CommandCost();

				Track track = (trackbits == TRACK_BIT_HORZ) ? TRACK_UPPER : TRACK_LEFT;
				if (!ignore1 && !IsCompatibleRail(type, totype)) {
					CommandCost ret = EnsureNoTrainOnTrack(tile, track);
					if (ret.Failed()) return ret;
				}

				if (!ignore2 && !IsCompatibleRail(type2, totype)) {
					CommandCost ret = EnsureNoTrainOnTrack(tile, TrackToOppositeTrack(track));
					if (ret.Failed()) return ret;
				}

				cost.AddCost(RailConvertCost(type, totype));
				cost.AddCost(RailConvertCost(type2, totype));
				break;
			}
		}
			/* fall through */
		case TRACK_BIT_RIGHT:
		case TRACK_BIT_LOWER:
		case TRACK_BIT_LOWER_RIGHT:
			type = GetRailType(tile, TRACK_LOWER);
			/* fall through */
		default:
			/* Converting to the same type or converting 'hidden' elrail -> rail */
			if (type == totype || (ignore_electric && type == RAILTYPE_ELECTRIC)) return CommandCost();

			if (!IsCompatibleRail(type, totype)) {
				CommandCost ret = EnsureNoVehicleOnGround(tile);
				if (ret.Failed()) return ret;
			}

			cost.AddCost(RailConvertCost(type, totype) * CountBits(trackbits));
			break;
	}

	return cost;
}

/**
 * Convert one rail type to another, for normal rail tiles
 * @param tile tile to convert
 * @param totype new railtype to convert to
 * @param affected list of affected trains
 * @param flags operation to perform
 * @return the cost of this operation or an error
 */
static CommandCost ConvertTrack(TileIndex tile, RailType totype, TrainList *affected, DoCommandFlag flags)
{
	CommandCost ret = CheckRailConversion(tile, totype);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) { // we can safely convert, too
		SmallVector<Train *, 2> vehicles_affected;
		FindUnpoweredReservationTrains(&vehicles_affected, tile, totype);

		/* Update the company infrastructure counters. */
		Company *c = Company::Get(GetTileOwner(tile));
		uint num_pieces;

		TrackBits bits = GetTrackBits(tile);
		switch (bits) {
			case TRACK_BIT_HORZ:
			case TRACK_BIT_VERT:
				num_pieces = 2;
				c->infrastructure.rail[GetRailType(tile, TRACK_UPPER)]--;
				c->infrastructure.rail[GetRailType(tile, TRACK_LOWER)]--;
				break;

			case TRACK_BIT_RIGHT:
			case TRACK_BIT_LOWER:
				num_pieces = 1;
				c->infrastructure.rail[GetRailType(tile, TRACK_LOWER)]--;
				break;

			case TRACK_BIT_LOWER_RIGHT:
				num_pieces = 2 * 2;
				c->infrastructure.rail[GetRailType(tile, TRACK_LOWER)] -= 2 * 2;
				break;

			default:
				num_pieces = CountBits(bits);
				if (TracksOverlap(bits)) num_pieces *= num_pieces;
				c->infrastructure.rail[GetRailType(tile, TRACK_UPPER)] -= num_pieces;
				break;
		}

		c->infrastructure.rail[totype] += num_pieces;
		DirtyCompanyInfrastructureWindows(c->index);

		SetRailType(tile, totype);
		MarkTileDirtyByTile(tile);
		/* update power of train on this tile */
		FindVehicleOnPos(tile, affected, &UpdateTrainPowerProc);

		/* notify YAPF about the track layout change */
		TrackBits trackbits = GetTrackBits(tile);
		while (trackbits != TRACK_BIT_NONE) {
			YapfNotifyTrackLayoutChange(tile, RemoveFirstTrack(&trackbits));
		}

		for (uint i = 0; i < vehicles_affected.Length(); ++i) {
			TryPathReserve(vehicles_affected[i], true);
		}
	}

	return ret;
}

/**
 * Convert one rail type to another, for bridge tiles
 * @param tile tile to convert
 * @param endtile bridge end
 * @param totype new railtype to convert to
 * @param affected list of affected trains
 * @param flags operation to perform
 * @return the cost of this operation or an error
 */
static CommandCost ConvertBridge(TileIndex tile, TileIndex endtile, RailType totype, TrainList *affected, DoCommandFlag flags)
{
	CommandCost cost = CheckRailConversion(tile, totype);
	if (cost.Failed()) return cost;

	CommandCost ret = CheckRailConversion(endtile, totype);
	if (ret.Failed()) return ret;
	cost.AddCost(ret);

	/* Original railtype we are converting from */
	RailType type = GetBridgeRailType(tile);

	/* Converting to the same type or converting 'hidden' elrail -> rail */
	if (type == totype) return cost;
	if (_settings_game.vehicle.disable_elrails && totype == RAILTYPE_RAIL && type == RAILTYPE_ELECTRIC) return cost;

	/* When not converting rail <-> el. rail, no vehicle can be in the bridge */
	if (!IsCompatibleRail(type, totype)) {
		CommandCost ret = EnsureNoTrainOnTunnelBridgeMiddle(tile, endtile);
		if (ret.Failed()) return ret;
	}

	uint len = GetTunnelBridgeLength(tile, endtile);
	cost.AddCost(len * RailConvertCost(type, totype));

	if (flags & DC_EXEC) {
		SmallVector<Train *, 4> vehicles_affected;
		FindUnpoweredReservationTrains(&vehicles_affected, tile, totype);
		FindUnpoweredReservationTrains(&vehicles_affected, endtile, totype);

		/* Update the company infrastructure counters. */
		Company *c = Company::Get(GetTileOwner(tile));
		uint num_pieces = len;
		DiagDirection dir = GetTunnelBridgeDirection(tile);

		TrackBits bits = GetTrackBits(tile);
		if (bits == TRACK_BIT_HORZ || bits == TRACK_BIT_VERT) {
			c->infrastructure.rail[GetSideRailType(tile, ReverseDiagDir(dir))]--;
			c->infrastructure.rail[totype]++;
			num_pieces++;
		} else {
			uint n = CountBits(bits);
			num_pieces += n * n;
		}

		bits = GetTrackBits(endtile);
		if (bits == TRACK_BIT_HORZ || bits == TRACK_BIT_VERT) {
			c->infrastructure.rail[GetSideRailType(tile, dir)]--;
			c->infrastructure.rail[totype]++;
			num_pieces++;
		} else {
			uint n = CountBits(bits);
			num_pieces += n * n;
		}

		num_pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
		c->infrastructure.rail[type] -= num_pieces;
		c->infrastructure.rail[totype] += num_pieces;
		DirtyCompanyInfrastructureWindows(c->index);

		SetRailType(tile, totype);
		SetRailType(endtile, totype);

		FindVehicleOnPos(tile, affected, &UpdateTrainPowerProc);
		FindVehicleOnPos(endtile, affected, &UpdateTrainPowerProc);

		/* notify YAPF about the track layout change */
		TrackBits trackbits = GetTrackBits(tile);
		while (trackbits != TRACK_BIT_NONE) {
			YapfNotifyTrackLayoutChange(tile, RemoveFirstTrack(&trackbits));
		}
		trackbits = GetTrackBits(endtile);
		while (trackbits != TRACK_BIT_NONE) {
			YapfNotifyTrackLayoutChange(tile, RemoveFirstTrack(&trackbits));
		}

		MarkBridgeTilesDirty(tile, endtile, dir);

		for (uint i = 0; i < vehicles_affected.Length(); ++i) {
			TryPathReserve(vehicles_affected[i], true);
		}
	}

	return cost;
}

/**
 * Convert one rail type to another, for tunnel tiles
 * @param tile tile to convert
 * @param endtile tunnel end
 * @param totype new railtype to convert to
 * @param affected list of affected trains
 * @param flags operation to perform
 * @return the cost of this operation or an error
 */
static CommandCost ConvertTunnel(TileIndex tile, TileIndex endtile, RailType totype, TrainList *affected, DoCommandFlag flags)
{
	/* Trying to convert other's rail */
	CommandCost ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;

	/* Original railtype we are converting from */
	RailType type = GetRailType(tile);

	/* Converting to the same type or converting 'hidden' elrail -> rail */
	if (type == totype) return CommandCost();
	if (_settings_game.vehicle.disable_elrails && totype == RAILTYPE_RAIL && type == RAILTYPE_ELECTRIC) return CommandCost();

	/* When not converting rail <-> el. rail, no vehicle can be in the tunnel */
	if (!IsCompatibleRail(type, totype)) {
		CommandCost ret = TunnelBridgeIsFree(tile, endtile);
		if (ret.Failed()) return ret;
	}

	uint len = GetTunnelBridgeLength(tile, endtile) + 2;

	if (flags & DC_EXEC) {
		Track track = DiagDirToDiagTrack(GetTunnelBridgeDirection(tile));

		Train *v = NULL;
		if (HasTunnelHeadReservation(tile)) {
			v = FindUnpoweredReservationTrain(tile, track, totype);
		}

		Train *w = NULL;
		if (HasTunnelHeadReservation(endtile)) {
			w = FindUnpoweredReservationTrain(endtile, track, totype);
		}

		/* Update the company infrastructure counters. */
		uint num_pieces = len * TUNNELBRIDGE_TRACKBIT_FACTOR;
		Company *c = Company::Get(GetTileOwner(tile));
		c->infrastructure.rail[type] -= num_pieces;
		c->infrastructure.rail[totype] += num_pieces;
		DirtyCompanyInfrastructureWindows(c->index);

		SetRailType(tile, totype);
		SetRailType(endtile, totype);

		FindVehicleOnPos(tile, affected, &UpdateTrainPowerProc);
		FindVehicleOnPos(endtile, affected, &UpdateTrainPowerProc);

		YapfNotifyTrackLayoutChange(tile, track);
		YapfNotifyTrackLayoutChange(endtile, track);

		MarkTileDirtyByTile(tile);
		MarkTileDirtyByTile(endtile);

		if (v != NULL) TryPathReserve(v, true);
		if (w != NULL) TryPathReserve(w, true);
	}

	return CommandCost(EXPENSES_CONSTRUCTION, len * RailConvertCost(type, totype));
}

/**
 * Convert one rail type to another, generic version
 * @param tile tile to convert
 * @param totype new railtype to convert to
 * @param track present track in the tile
 * @param reserved whether the track is reserved
 * @param affected list of affected trains
 * @param flags operation to perform
 * @return the cost of this operation or an error
 */
static CommandCost ConvertGeneric(TileIndex tile, RailType totype, Track track, bool reserved, TrainList *affected, DoCommandFlag flags)
{
	/* Trying to convert other's rail */
	CommandCost ret = CheckTileOwnership(tile);
	if (ret.Failed()) return ret;

	/* Original railtype we are converting from */
	RailType type = GetRailType(tile);

	/* Converting to the same type or converting 'hidden' elrail -> rail */
	if (type == totype) return CommandCost();
	if (_settings_game.vehicle.disable_elrails && totype == RAILTYPE_RAIL && type == RAILTYPE_ELECTRIC) return CommandCost();

	if (!IsCompatibleRail(type, totype)) {
		CommandCost ret = EnsureNoVehicleOnGround(tile);
		if (ret.Failed()) return ret;
	}

	if (flags & DC_EXEC) { // we can safely convert, too
		Train *v = NULL;
		if (reserved) v = FindUnpoweredReservationTrain(tile, track, totype);

		/* Update the company infrastructure counters. */
		if (!IsRailStationTile(tile) || !IsStationTileBlocked(tile)) {
			Company *c = Company::Get(GetTileOwner(tile));
			uint num_pieces = IsLevelCrossingTile(tile) ? LEVELCROSSING_TRACKBIT_FACTOR : 1;
			c->infrastructure.rail[type] -= num_pieces;
			c->infrastructure.rail[totype] += num_pieces;
			DirtyCompanyInfrastructureWindows(c->index);
		}

		SetRailType(tile, totype);
		MarkTileDirtyByTile(tile);
		/* update power of train on this tile */
		FindVehicleOnPos(tile, affected, &UpdateTrainPowerProc);

		/* notify YAPF about the track layout change */
		YapfNotifyTrackLayoutChange(tile, track);

		if (v != NULL) TryPathReserve(v, true);
	}

	return CommandCost(EXPENSES_CONSTRUCTION, RailConvertCost(type, totype));
}

/**
 * Convert one rail type to the other. You can convert normal rail to
 * monorail/maglev easily or vice-versa.
 * @param tile end tile of rail conversion drag
 * @param flags operation to perform
 * @param p1 start tile of drag
 * @param p2 various bitstuffed elements:
 * - p2 = (bit  0- 3) new railtype to convert to.
 * - p2 = (bit  4)    build diagonally or not.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdConvertRail(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	RailType totype = Extract<RailType, 0, 4>(p2);

	if (!ValParamRailtype(totype)) return CMD_ERROR;
	if (p1 >= MapSize()) return CMD_ERROR;

	TrainList affected_trains;

	CommandCost cost(EXPENSES_CONSTRUCTION);
	CommandCost err = CommandCost(STR_ERROR_NO_SUITABLE_RAILROAD_TRACK); // by default, there is no track to convert.
	TileArea ta(tile, p1);
	TileIterator *iter = HasBit(p2, 4) ? (TileIterator *)new DiagonalTileIterator(tile, p1) : new OrthogonalTileIterator(ta);
	for (; (tile = *iter) != INVALID_TILE; ++(*iter)) {
		CommandCost ret;

		Track track = INVALID_TRACK;
		bool reserved;

		/* Check if there is any track on tile */
		switch (GetTileType(tile)) {
			case TT_RAILWAY:
				if (IsTileSubtype(tile, TT_TRACK)) {
					ret = ConvertTrack(tile, totype, &affected_trains, flags);
				} else {
					/* If both ends of bridge are in the range, do not try to convert twice -
					 * it would cause assert because of different test and exec runs */
					TileIndex endtile = GetOtherBridgeEnd(tile);
					if (endtile < tile && TileX(endtile) >= TileX(ta.tile) && TileX(endtile) < TileX(ta.tile) + ta.w &&
							TileY(endtile) >= TileY(ta.tile) && TileY(endtile) < TileY(ta.tile) + ta.h) continue;

					ret = ConvertBridge(tile, endtile, totype, &affected_trains, flags);
				}
				break;

			case TT_MISC:
				switch (GetTileSubtype(tile)) {
					default: continue;

					case TT_MISC_CROSSING:
						if (RailNoLevelCrossings(totype)) {
							err.MakeError(STR_ERROR_CROSSING_DISALLOWED);
							continue;
						}
						track = GetCrossingRailTrack(tile);
						reserved = HasCrossingReservation(tile);
						break;

					case TT_MISC_TUNNEL: {
						if (GetTunnelTransportType(tile) != TRANSPORT_RAIL) continue;

						/* If both ends of tunnel are in the range, do not try to convert twice -
						 * it would cause assert because of different test and exec runs */
						TileIndex endtile = GetOtherTunnelEnd(tile);
						if (endtile < tile && TileX(endtile) >= TileX(ta.tile) && TileX(endtile) < TileX(ta.tile) + ta.w &&
								TileY(endtile) >= TileY(ta.tile) && TileY(endtile) < TileY(ta.tile) + ta.h) continue;

						ret = ConvertTunnel(tile, endtile, totype, &affected_trains, flags);
						break;
					}

					case TT_MISC_DEPOT:
						if (!IsRailDepot(tile)) continue;
						track = GetRailDepotTrack(tile);
						reserved = HasDepotReservation(tile);
						break;
				}
				break;

			case TT_STATION:
				if (!HasStationRail(tile)) continue;
				track = GetRailStationTrack(tile);
				reserved = HasStationReservation(tile);
				break;

			default: continue;
		}

		if (track != INVALID_TRACK) {
			ret = ConvertGeneric(tile, totype, track, reserved, &affected_trains, flags);
		}

		if (ret.Failed()) {
			err = ret;
		} else {
			cost.AddCost(ret);

			if (IsRailDepotTile(tile) && (flags & DC_EXEC)) {
				/* Update build vehicle window related to this depot */
				InvalidateWindowData(WC_VEHICLE_DEPOT, tile);
				InvalidateWindowData(WC_BUILD_VEHICLE, tile);
			}
		}
	}

	if (flags & DC_EXEC) {
		/* Railtype changed, update trains as when entering different track */
		for (Train **v = affected_trains.Begin(); v != affected_trains.End(); v++) {
			(*v)->ConsistChanged(true);
		}
	}

	delete iter;
	return (cost.GetCost() == 0) ? err : cost;
}

static CommandCost ClearTile_Track(TileIndex tile, DoCommandFlag flags)
{
	if (flags & DC_AUTO) {
		if (!IsTileOwner(tile, _current_company)) {
			return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
		} else if (IsTileSubtype(tile, TT_BRIDGE)) {
			return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
		} else {
			return_cmd_error(STR_ERROR_MUST_REMOVE_RAILROAD_TRACK);
		}
	}

	if (IsTileSubtype(tile, TT_TRACK)) {
		CommandCost cost(EXPENSES_CONSTRUCTION);

		Slope tileh = GetTileSlope(tile);
		/* Is there flat water on the lower halftile that gets cleared expensively? */
		bool water_ground = (GetRailGroundType(tile) == RAIL_GROUND_WATER && IsSlopeWithOneCornerRaised(tileh));

		TrackBits tracks = GetTrackBits(tile);
		while (tracks != TRACK_BIT_NONE) {
			Track track = RemoveFirstTrack(&tracks);
			CommandCost ret = DoCommand(tile, 0, track, flags, CMD_REMOVE_SINGLE_RAIL);
			if (ret.Failed()) return ret;
			cost.AddCost(ret);
		}

		/* When bankrupting, don't make water dirty, there could be a ship on lower halftile.
		 * Same holds for non-companies clearing the tile, e.g. disasters. */
		if (water_ground && !(flags & DC_BANKRUPT) && Company::IsValidID(_current_company)) {
			CommandCost ret = EnsureNoVehicleOnGround(tile);
			if (ret.Failed()) return ret;

			/* The track was removed, and left a coast tile. Now also clear the water. */
			if (flags & DC_EXEC) DoClearSquare(tile);
			cost.AddCost(_price[PR_CLEAR_WATER]);
		}

		return cost;
	} else {
		if (_current_company != OWNER_WATER && _game_mode != GM_EDITOR) {
			CommandCost ret = CheckOwnership(GetTileOwner(tile));
			if (ret.Failed()) return ret;
		}

		TrackBits present = GetTrackBits(tile);

		if ((present == TRACK_BIT_HORZ) || (present == TRACK_BIT_VERT)) {
			Track track = FindFirstTrack(DiagdirReachesTracks(GetTunnelBridgeDirection(tile)) & present);

			CommandCost cost = DoCommand(tile, 0, track, flags, CMD_REMOVE_SINGLE_RAIL);
			if (cost.Failed()) return cost;

			CommandCost ret = RemoveBridgeTrack(tile, TrackToOppositeTrack(track), flags);
			if (ret.Failed()) return ret;

			cost.AddCost(ret);
			return cost;
		}

		TileIndex other_tile = GetOtherBridgeEnd(tile);
		TrackBits other_remove = GetTrackBits(other_tile) & DiagdirReachesTracks(GetTunnelBridgeDirection(tile));

		assert(other_remove != TRACK_BIT_NONE);

		CommandCost ret = EnsureNoTrainOnBridgeTrackBits(tile, present, other_tile, other_remove);
		if (ret.Failed()) return ret;

		uint len = GetTunnelBridgeLength(tile, other_tile) + 2; // Don't forget the end tiles.

		CommandCost cost(EXPENSES_CONSTRUCTION, len * _price[PR_CLEAR_BRIDGE]);
		cost.AddCost((CountBits(present) - 1) * RailClearCost(GetBridgeRailType(tile)));

		/* Charge extra to remove signals on the track, if any */
		if (HasSignalOnTrack(tile, FindFirstTrack(present))) {
			cost.AddCost(DoCommand(tile, FindFirstTrack(present), 0, flags, CMD_REMOVE_SIGNALS));
		}

		int n = CountBits(other_remove);
		if (n == 1) {
			Track other_track = FindFirstTrack(other_remove);
			if (HasSignalOnTrack(other_tile, other_track)) {
				cost.AddCost(DoCommand(other_tile, other_track, 0, flags, CMD_REMOVE_SIGNALS));
			}
		} else {
			cost.AddCost((n - 1) * RailClearCost(GetBridgeRailType(other_tile)));
		}

		if (flags & DC_EXEC) {
			RemoveRailBridge(tile, present, other_tile, other_remove);
		}

		return cost;
	}
}


static int GetSlopePixelZ_Track(TileIndex tile, uint x, uint y)
{
	int z;
	Slope tileh = GetTilePixelSlope(tile, &z);

	if (IsTileSubtype(tile, TT_TRACK)) {
		if (tileh == SLOPE_FLAT) return z;
		z += ApplyPixelFoundationToSlope(GetRailFoundation(tileh, GetTrackBits(tile)), &tileh);
		return z + GetPartialPixelZ(x & 0xF, y & 0xF, tileh);
	} else if (IsExtendedRailBridge(tile)) {
		return z + TILE_HEIGHT;
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


static uint32 _drawtile_track_palette;

static void DrawTrackFence_NW(const TileInfo *ti, SpriteID base_image)
{
	RailFenceOffset rfo = RFO_FLAT_X;
	if (ti->tileh & SLOPE_NW) rfo = (ti->tileh & SLOPE_W) ? RFO_SLOPE_SW : RFO_SLOPE_NE;
	AddSortableSpriteToDraw(base_image + rfo, _drawtile_track_palette,
		ti->x, ti->y + 1, 16, 1, 4, ti->z);
}

static void DrawTrackFence_SE(const TileInfo *ti, SpriteID base_image)
{
	RailFenceOffset rfo = RFO_FLAT_X;
	if (ti->tileh & SLOPE_SE) rfo = (ti->tileh & SLOPE_S) ? RFO_SLOPE_SW : RFO_SLOPE_NE;
	AddSortableSpriteToDraw(base_image + rfo, _drawtile_track_palette,
		ti->x, ti->y + TILE_SIZE - 1, 16, 1, 4, ti->z);
}

static void DrawTrackFence_NW_SE(const TileInfo *ti, SpriteID base_image)
{
	DrawTrackFence_NW(ti, base_image);
	DrawTrackFence_SE(ti, base_image);
}

static void DrawTrackFence_NE(const TileInfo *ti, SpriteID base_image)
{
	RailFenceOffset rfo = RFO_FLAT_Y;
	if (ti->tileh & SLOPE_NE) rfo = (ti->tileh & SLOPE_E) ? RFO_SLOPE_SE : RFO_SLOPE_NW;
	AddSortableSpriteToDraw(base_image + rfo, _drawtile_track_palette,
		ti->x + 1, ti->y, 1, 16, 4, ti->z);
}

static void DrawTrackFence_SW(const TileInfo *ti, SpriteID base_image)
{
	RailFenceOffset rfo = RFO_FLAT_Y;
	if (ti->tileh & SLOPE_SW) rfo = (ti->tileh & SLOPE_S) ? RFO_SLOPE_SE : RFO_SLOPE_NW;
	AddSortableSpriteToDraw(base_image + rfo, _drawtile_track_palette,
		ti->x + TILE_SIZE - 1, ti->y, 1, 16, 4, ti->z);
}

static void DrawTrackFence_NE_SW(const TileInfo *ti, SpriteID base_image)
{
	DrawTrackFence_NE(ti, base_image);
	DrawTrackFence_SW(ti, base_image);
}

/**
 * Draw fence at eastern side of track.
 */
static void DrawTrackFence_NS_1(const TileInfo *ti, SpriteID base_image)
{
	int z = ti->z + GetSlopePixelZInCorner(RemoveHalftileSlope(ti->tileh), CORNER_W);
	AddSortableSpriteToDraw(base_image + RFO_FLAT_VERT, _drawtile_track_palette,
		ti->x + TILE_SIZE / 2, ti->y + TILE_SIZE / 2, 1, 1, 4, z);
}

/**
 * Draw fence at western side of track.
 */
static void DrawTrackFence_NS_2(const TileInfo *ti, SpriteID base_image)
{
	int z = ti->z + GetSlopePixelZInCorner(RemoveHalftileSlope(ti->tileh), CORNER_E);
	AddSortableSpriteToDraw(base_image + RFO_FLAT_VERT, _drawtile_track_palette,
		ti->x + TILE_SIZE / 2, ti->y + TILE_SIZE / 2, 1, 1, 4, z);
}

/**
 * Draw fence at southern side of track.
 */
static void DrawTrackFence_WE_1(const TileInfo *ti, SpriteID base_image)
{
	int z = ti->z + GetSlopePixelZInCorner(RemoveHalftileSlope(ti->tileh), CORNER_N);
	AddSortableSpriteToDraw(base_image + RFO_FLAT_HORZ, _drawtile_track_palette,
		ti->x + TILE_SIZE / 2, ti->y + TILE_SIZE / 2, 1, 1, 4, z);
}

/**
 * Draw fence at northern side of track.
 */
static void DrawTrackFence_WE_2(const TileInfo *ti, SpriteID base_image)
{
	int z = ti->z + GetSlopePixelZInCorner(RemoveHalftileSlope(ti->tileh), CORNER_S);
	AddSortableSpriteToDraw(base_image + RFO_FLAT_HORZ, _drawtile_track_palette,
		ti->x + TILE_SIZE / 2, ti->y + TILE_SIZE / 2, 1, 1, 4, z);
}


static void DrawTrackDetails(const TileInfo *ti, TrackBits tracks)
{
	const RailtypeInfo *rti;

	switch (tracks) {
		case TRACK_BIT_HORZ:
		case TRACK_BIT_VERT:
			return; /* these never have fences */

		case TRACK_BIT_LOWER:
		case TRACK_BIT_RIGHT:
		case TRACK_BIT_LOWER_RIGHT:
			rti = GetRailTypeInfo(GetRailType(ti->tile, TRACK_LOWER));
			break;

		default:
			rti = GetRailTypeInfo(GetRailType(ti->tile, TRACK_UPPER));
			break;
	}

	/* Base sprite for track fences.
	 * Note: Halftile slopes only have fences on the upper part. */
	SpriteID base_image = GetCustomRailSprite(rti, ti->tile, RTSG_FENCES, IsHalftileSlope(ti->tileh) ? TCX_UPPER_HALFTILE : TCX_NORMAL);
	if (base_image == 0) base_image = SPR_TRACK_FENCE_FLAT_X;

	switch (GetRailGroundType(ti->tile)) {
		case RAIL_GROUND_FENCE_NW:     DrawTrackFence_NW(ti, base_image);    break;
		case RAIL_GROUND_FENCE_SE:     DrawTrackFence_SE(ti, base_image);    break;
		case RAIL_GROUND_FENCE_SENW:   DrawTrackFence_NW_SE(ti, base_image); break;
		case RAIL_GROUND_FENCE_NE:     DrawTrackFence_NE(ti, base_image);    break;
		case RAIL_GROUND_FENCE_SW:     DrawTrackFence_SW(ti, base_image);    break;
		case RAIL_GROUND_FENCE_NESW:   DrawTrackFence_NE_SW(ti, base_image); break;
		case RAIL_GROUND_FENCE_VERT1:  DrawTrackFence_NS_1(ti, base_image);  break;
		case RAIL_GROUND_FENCE_VERT2:  DrawTrackFence_NS_2(ti, base_image);  break;
		case RAIL_GROUND_FENCE_HORIZ1: DrawTrackFence_WE_1(ti, base_image);  break;
		case RAIL_GROUND_FENCE_HORIZ2: DrawTrackFence_WE_2(ti, base_image);  break;
		case RAIL_GROUND_WATER: {
			Corner track_corner;
			if (IsHalftileSlope(ti->tileh)) {
				/* Steep slope or one-corner-raised slope with halftile foundation */
				track_corner = GetHalftileSlopeCorner(ti->tileh);
			} else {
				/* Three-corner-raised slope */
				track_corner = OppositeCorner(GetHighestSlopeCorner(ComplementSlope(ti->tileh)));
			}
			switch (track_corner) {
				case CORNER_W: DrawTrackFence_NS_1(ti, base_image); break;
				case CORNER_S: DrawTrackFence_WE_2(ti, base_image); break;
				case CORNER_E: DrawTrackFence_NS_2(ti, base_image); break;
				case CORNER_N: DrawTrackFence_WE_1(ti, base_image); break;
				default: NOT_REACHED();
			}
			break;
		}
		default: break;
	}
}

/* SubSprite for drawing track halftiles. */
static const int INF = 1000; // big number compared to tilesprite size
static const SubSprite _halftile_sub_sprite[4] = {
	{ -INF    , -INF  , 32 - 33, INF     }, // CORNER_W, clip 33 pixels from right
	{ -INF    , 0 + 15, INF    , INF     }, // CORNER_S, clip 15 pixels from top
	{ -31 + 33, -INF  , INF    , INF     }, // CORNER_E, clip 33 pixels from left
	{ -INF    , -INF  , INF    , 30 - 15 }  // CORNER_N, clip 15 pixels from bottom
};
static const SubSprite _halftile_sub_sprite_upper[4] = {
	{ -INF    , -INF  , 32 - 33, INF     }, // CORNER_W, clip 33 pixels from right
	{ -INF    ,  0 + 7, INF    , INF     }, // CORNER_S, clip 7 pixels from top
	{ -31 + 33, -INF  , INF    , INF     }, // CORNER_E, clip 33 pixels from left
	{ -INF    , -INF  , INF    , 30 - 23 }  // CORNER_N, clip 23 pixels from bottom
};
static const byte _corner_to_track_sprite[] = {3, 1, 2, 0};

static inline void DrawTrackSprite(SpriteID sprite, PaletteID pal, const TileInfo *ti, Slope s)
{
	DrawGroundSprite(sprite, pal, NULL, 0, (ti->tileh & s) ? -8 : 0);
}

static void DrawTrackGround(TileInfo *ti, RailGroundType rgt, bool has_track)
{
	if (rgt == RAIL_GROUND_WATER) {
		if (has_track || IsSteepSlope(ti->tileh)) {
			/* three-corner-raised slope or steep slope with track on upper part */
			DrawShoreTile(ti->tileh);
		} else {
			/* single-corner-raised slope with track on upper part */
			DrawGroundSprite(SPR_FLAT_WATER_TILE, PAL_NONE);
		}
	} else {
		SpriteID image;

		switch (rgt) {
			case RAIL_GROUND_BARREN:     image = SPR_FLAT_BARE_LAND;  break;
			case RAIL_GROUND_ICE_DESERT: image = SPR_FLAT_SNOW_DESERT_TILE; break;
			default:                     image = SPR_FLAT_GRASS_TILE; break;
		}

		image += SlopeToSpriteOffset(ti->tileh);

		DrawGroundSprite(image, PAL_NONE);
	}
}

static void DrawTrackBitsOverlay(TileInfo *ti, TrackBits track, const RailtypeInfo *rti)
{
	SpriteID overlay = GetCustomRailSprite(rti, ti->tile, RTSG_OVERLAY);
	SpriteID ground = GetCustomRailSprite(rti, ti->tile, RTSG_GROUND);
	TrackBits pbs = _settings_client.gui.show_track_reservation ? GetRailReservationTrackBits(ti->tile) : TRACK_BIT_NONE;

	if (track == TRACK_BIT_NONE) {
		/* Half-tile foundation, no track here? */
	} else if (ti->tileh == SLOPE_NW && track == TRACK_BIT_Y) {
		DrawGroundSprite(ground + RTO_SLOPE_NW, PAL_NONE);
		if (pbs != TRACK_BIT_NONE) DrawGroundSprite(overlay + 9, PALETTE_CRASH);
	} else if (ti->tileh == SLOPE_NE && track == TRACK_BIT_X) {
		DrawGroundSprite(ground + RTO_SLOPE_NE, PAL_NONE);
		if (pbs != TRACK_BIT_NONE) DrawGroundSprite(overlay + 6, PALETTE_CRASH);
	} else if (ti->tileh == SLOPE_SE && track == TRACK_BIT_Y) {
		DrawGroundSprite(ground + RTO_SLOPE_SE, PAL_NONE);
		if (pbs != TRACK_BIT_NONE) DrawGroundSprite(overlay + 7, PALETTE_CRASH);
	} else if (ti->tileh == SLOPE_SW && track == TRACK_BIT_X) {
		DrawGroundSprite(ground + RTO_SLOPE_SW, PAL_NONE);
		if (pbs != TRACK_BIT_NONE) DrawGroundSprite(overlay + 8, PALETTE_CRASH);
	} else {
		switch (track) {
			/* Draw single ground sprite when not overlapping. No track overlay
			 * is necessary for these sprites. */
			case TRACK_BIT_X:     DrawGroundSprite(ground + RTO_X, PAL_NONE); break;
			case TRACK_BIT_Y:     DrawGroundSprite(ground + RTO_Y, PAL_NONE); break;
			case TRACK_BIT_UPPER: DrawTrackSprite(ground + RTO_N, PAL_NONE, ti, SLOPE_N); break;
			case TRACK_BIT_LOWER: DrawTrackSprite(ground + RTO_S, PAL_NONE, ti, SLOPE_S); break;
			case TRACK_BIT_RIGHT: DrawTrackSprite(ground + RTO_E, PAL_NONE, ti, SLOPE_E); break;
			case TRACK_BIT_LEFT:  DrawTrackSprite(ground + RTO_W, PAL_NONE, ti, SLOPE_W); break;
			case TRACK_BIT_CROSS: DrawGroundSprite(ground + RTO_CROSSING_XY, PAL_NONE); break;
			case TRACK_BIT_HORZ:  DrawTrackSprite(ground + RTO_N, PAL_NONE, ti, SLOPE_N);
			                      DrawTrackSprite(ground + RTO_S, PAL_NONE, ti, SLOPE_S); break;
			case TRACK_BIT_VERT:  DrawTrackSprite(ground + RTO_E, PAL_NONE, ti, SLOPE_E);
			                      DrawTrackSprite(ground + RTO_W, PAL_NONE, ti, SLOPE_W); break;

			default:
				/* We're drawing a junction tile */
				if ((track & TRACK_BIT_3WAY_NE) == 0) {
					DrawGroundSprite(ground + RTO_JUNCTION_SW, PAL_NONE);
				} else if ((track & TRACK_BIT_3WAY_SW) == 0) {
					DrawGroundSprite(ground + RTO_JUNCTION_NE, PAL_NONE);
				} else if ((track & TRACK_BIT_3WAY_NW) == 0) {
					DrawGroundSprite(ground + RTO_JUNCTION_SE, PAL_NONE);
				} else if ((track & TRACK_BIT_3WAY_SE) == 0) {
					DrawGroundSprite(ground + RTO_JUNCTION_NW, PAL_NONE);
				} else {
					DrawGroundSprite(ground + RTO_JUNCTION_NSEW, PAL_NONE);
				}

				/* Mask out PBS bits as we shall draw them afterwards anyway. */
				track &= ~pbs;

				/* Draw regular track bits */
				if (track & TRACK_BIT_X)     DrawGroundSprite(overlay + RTO_X, PAL_NONE);
				if (track & TRACK_BIT_Y)     DrawGroundSprite(overlay + RTO_Y, PAL_NONE);
				if (track & TRACK_BIT_UPPER) DrawGroundSprite(overlay + RTO_N, PAL_NONE);
				if (track & TRACK_BIT_LOWER) DrawGroundSprite(overlay + RTO_S, PAL_NONE);
				if (track & TRACK_BIT_RIGHT) DrawGroundSprite(overlay + RTO_E, PAL_NONE);
				if (track & TRACK_BIT_LEFT)  DrawGroundSprite(overlay + RTO_W, PAL_NONE);
		}

		/* Draw reserved track bits */
		if (pbs & TRACK_BIT_X)     DrawGroundSprite(overlay + RTO_X, PALETTE_CRASH);
		if (pbs & TRACK_BIT_Y)     DrawGroundSprite(overlay + RTO_Y, PALETTE_CRASH);
		if (pbs & TRACK_BIT_UPPER) DrawTrackSprite(overlay + RTO_N, PALETTE_CRASH, ti, SLOPE_N);
		if (pbs & TRACK_BIT_LOWER) DrawTrackSprite(overlay + RTO_S, PALETTE_CRASH, ti, SLOPE_S);
		if (pbs & TRACK_BIT_RIGHT) DrawTrackSprite(overlay + RTO_E, PALETTE_CRASH, ti, SLOPE_E);
		if (pbs & TRACK_BIT_LEFT)  DrawTrackSprite(overlay + RTO_W, PALETTE_CRASH, ti, SLOPE_W);
	}
}

static void DrawTrackBitsNonOverlay(TileInfo *ti, TrackBits track, const RailtypeInfo *rti, RailGroundType rgt)
{
	SpriteID image;
	PaletteID pal = PAL_NONE;
	const SubSprite *sub = NULL;
	bool junction = false;

	if (track == TRACK_BIT_NONE) return;

	if (ti->tileh != SLOPE_FLAT) {
		/* track on non-flat ground */
		image = _track_sloped_sprites[ti->tileh - 1] + rti->base_sprites.track_y;
	} else {
		/* track on flat ground */
		(image = rti->base_sprites.track_y, track == TRACK_BIT_Y) ||
		(image++,                           track == TRACK_BIT_X) ||
		(image++,                           track == TRACK_BIT_UPPER) ||
		(image++,                           track == TRACK_BIT_LOWER) ||
		(image++,                           track == TRACK_BIT_RIGHT) ||
		(image++,                           track == TRACK_BIT_LEFT) ||
		(image++,                           track == TRACK_BIT_CROSS) ||

		(image = rti->base_sprites.track_ns, track == TRACK_BIT_HORZ) ||
		(image++,                            track == TRACK_BIT_VERT) ||

		(junction = true, false) ||
		(image = rti->base_sprites.ground, (track & TRACK_BIT_3WAY_NE) == 0) ||
		(image++,                          (track & TRACK_BIT_3WAY_SW) == 0) ||
		(image++,                          (track & TRACK_BIT_3WAY_NW) == 0) ||
		(image++,                          (track & TRACK_BIT_3WAY_SE) == 0) ||
		(image++, true);
	}

	switch (rgt) {
		case RAIL_GROUND_BARREN:     pal = PALETTE_TO_BARE_LAND; break;
		case RAIL_GROUND_ICE_DESERT: image += rti->snow_offset;  break;
		case RAIL_GROUND_WATER: {
			/* three-corner-raised slope */
			DrawShoreTile(ti->tileh);
			Corner track_corner = OppositeCorner(GetHighestSlopeCorner(ComplementSlope(ti->tileh)));
			sub = &_halftile_sub_sprite_upper[track_corner];
			break;
		}
		default: break;
	}

	DrawGroundSprite(image, pal, sub);

	/* Draw track pieces individually for junction tiles */
	if (junction) {
		if (track & TRACK_BIT_X)     DrawGroundSprite(rti->base_sprites.single_x, PAL_NONE);
		if (track & TRACK_BIT_Y)     DrawGroundSprite(rti->base_sprites.single_y, PAL_NONE);
		if (track & TRACK_BIT_UPPER) DrawGroundSprite(rti->base_sprites.single_n, PAL_NONE);
		if (track & TRACK_BIT_LOWER) DrawGroundSprite(rti->base_sprites.single_s, PAL_NONE);
		if (track & TRACK_BIT_LEFT)  DrawGroundSprite(rti->base_sprites.single_w, PAL_NONE);
		if (track & TRACK_BIT_RIGHT) DrawGroundSprite(rti->base_sprites.single_e, PAL_NONE);
	}

	/* PBS debugging, draw reserved tracks darker */
	if (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation) {
		/* Get reservation, but mask track on halftile slope */
		TrackBits pbs = GetRailReservationTrackBits(ti->tile) & track;
		if (pbs & TRACK_BIT_X) {
			if (ti->tileh == SLOPE_FLAT || ti->tileh == SLOPE_ELEVATED) {
				DrawGroundSprite(rti->base_sprites.single_x, PALETTE_CRASH);
			} else {
				DrawGroundSprite(_track_sloped_sprites[ti->tileh - 1] + rti->base_sprites.single_sloped - 20, PALETTE_CRASH);
			}
		}
		if (pbs & TRACK_BIT_Y) {
			if (ti->tileh == SLOPE_FLAT || ti->tileh == SLOPE_ELEVATED) {
				DrawGroundSprite(rti->base_sprites.single_y, PALETTE_CRASH);
			} else {
				DrawGroundSprite(_track_sloped_sprites[ti->tileh - 1] + rti->base_sprites.single_sloped - 20, PALETTE_CRASH);
			}
		}
		if (pbs & TRACK_BIT_UPPER) DrawGroundSprite(rti->base_sprites.single_n, PALETTE_CRASH, NULL, 0, ti->tileh & SLOPE_N ? -(int)TILE_HEIGHT : 0);
		if (pbs & TRACK_BIT_LOWER) DrawGroundSprite(rti->base_sprites.single_s, PALETTE_CRASH, NULL, 0, ti->tileh & SLOPE_S ? -(int)TILE_HEIGHT : 0);
		if (pbs & TRACK_BIT_LEFT)  DrawGroundSprite(rti->base_sprites.single_w, PALETTE_CRASH, NULL, 0, ti->tileh & SLOPE_W ? -(int)TILE_HEIGHT : 0);
		if (pbs & TRACK_BIT_RIGHT) DrawGroundSprite(rti->base_sprites.single_e, PALETTE_CRASH, NULL, 0, ti->tileh & SLOPE_E ? -(int)TILE_HEIGHT : 0);
	}
}

static void DrawTrackBits(TileInfo *ti, TrackBits track, const RailtypeInfo *rti, RailGroundType rgt)
{
	if (rti->UsesOverlay()) {
		DrawTrackBitsOverlay(ti, track, rti);
	} else {
		DrawTrackBitsNonOverlay(ti, track, rti, rgt);
	}

}

static void DrawHalftileOverlay(TileInfo *ti, Corner corner, const RailtypeInfo *rti, RailGroundType rgt)
{
	SpriteID offset;
	switch (corner) {
		default: NOT_REACHED();
		case CORNER_N: offset = RTO_N; break;
		case CORNER_S: offset = RTO_S; break;
		case CORNER_E: offset = RTO_E; break;
		case CORNER_W: offset = RTO_W; break;
	}

	DrawGroundSprite(offset + GetCustomRailSprite(rti, ti->tile, RTSG_GROUND), PAL_NONE, &_halftile_sub_sprite[corner]);

	if (_settings_client.gui.show_track_reservation && HasReservedTracks(ti->tile, CornerToTrackBits(corner))) {
		DrawGroundSprite(offset + GetCustomRailSprite(rti, ti->tile, RTSG_OVERLAY), PALETTE_CRASH, &_halftile_sub_sprite[corner]);
	}
}

static void DrawHalftileNonOverlay(TileInfo *ti, Corner corner, const RailtypeInfo *rti, RailGroundType rgt)
{
	SpriteID image = rti->base_sprites.track_y + 2;
	PaletteID pal;

	switch (corner) {
		default: NOT_REACHED();
		case CORNER_W: image++; /* fall through */
		case CORNER_E: image++; /* fall through */
		case CORNER_S: image++; /* fall through */
		case CORNER_N: break;
	}

	switch (rgt) {
		case RAIL_GROUND_BARREN:     pal = PALETTE_TO_BARE_LAND; break;
		case RAIL_GROUND_ICE_DESERT: image += rti->snow_offset; /* fall through */
		default: pal = PAL_NONE; break;
	}

	DrawGroundSprite(image, pal, &_halftile_sub_sprite[corner]);

	/* PBS debugging, draw reserved tracks darker */
	if (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation && HasReservedTracks(ti->tile, CornerToTrackBits(corner))) {
		DrawGroundSprite(_corner_to_track_sprite[corner] + rti->base_sprites.single_n, PALETTE_CRASH, NULL, 0, 0);
	}
}

static void DrawHalftile(TileInfo *ti, Corner corner, const RailtypeInfo *rti, RailGroundType rgt)
{
	if (rti->UsesOverlay()) {
		DrawHalftileOverlay(ti, corner, rti, rgt);
	} else {
		DrawHalftileNonOverlay(ti, corner, rti, rgt);
	}
}

static void DrawUpperHalftileOverlay(TileInfo *ti, Corner corner, const RailtypeInfo *rti, RailGroundType rgt)
{
	SpriteID image;
	switch (rgt) {
		case RAIL_GROUND_BARREN:     image = SPR_FLAT_BARE_LAND;  break;
		case RAIL_GROUND_ICE_DESERT:
		case RAIL_GROUND_HALF_SNOW:  image = SPR_FLAT_SNOW_DESERT_TILE; break;
		default:                     image = SPR_FLAT_GRASS_TILE; break;
	}

	/* Draw higher halftile-overlay: Use the sloped sprites with three corners raised. They probably best fit the lighting. */
	Slope fake_slope = SlopeWithThreeCornersRaised(OppositeCorner(corner));

	image += SlopeToSpriteOffset(fake_slope);

	DrawGroundSprite(image, PAL_NONE, &_halftile_sub_sprite_upper[corner]);

	TrackBits track = CornerToTrackBits(corner);

	SpriteID overlay = GetCustomRailSprite(rti, ti->tile, RTSG_OVERLAY, TCX_UPPER_HALFTILE);
	SpriteID ground = GetCustomRailSprite(rti, ti->tile, RTSG_GROUND, TCX_UPPER_HALFTILE);

	int offset;
	switch (track) {
		default: NOT_REACHED();
		case TRACK_BIT_UPPER: offset = RTO_N; break;
		case TRACK_BIT_LOWER: offset = RTO_S; break;
		case TRACK_BIT_RIGHT: offset = RTO_E; break;
		case TRACK_BIT_LEFT:  offset = RTO_W; break;
	}

	DrawTrackSprite(ground + offset, PAL_NONE, ti, fake_slope);
	if (_settings_client.gui.show_track_reservation && HasReservedTracks(ti->tile, track)) {
		DrawTrackSprite(overlay + offset, PALETTE_CRASH, ti, fake_slope);
	}
}

static void DrawUpperHalftileNonOverlay(TileInfo *ti, Corner corner, const RailtypeInfo *rti, RailGroundType rgt)
{
	/* Draw higher halftile-overlay: Use the sloped sprites with three corners raised. They probably best fit the lighting. */
	Slope fake_slope = SlopeWithThreeCornersRaised(OppositeCorner(corner));
	SpriteID image = _track_sloped_sprites[fake_slope - 1] + rti->base_sprites.track_y;
	PaletteID pal = PAL_NONE;

	switch (rgt) {
		case RAIL_GROUND_BARREN:     pal = PALETTE_TO_BARE_LAND; break;
		case RAIL_GROUND_ICE_DESERT:
		case RAIL_GROUND_HALF_SNOW:  image += rti->snow_offset;  break; // higher part has snow in this case too
		default: break;
	}

	DrawGroundSprite(image, pal, &_halftile_sub_sprite_upper[corner]);

	if (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation && HasReservedTracks(ti->tile, CornerToTrackBits(corner))) {
		DrawGroundSprite(_corner_to_track_sprite[corner] + rti->base_sprites.single_n, PALETTE_CRASH, NULL, 0, -(int)TILE_HEIGHT);
	}
}

static void DrawUpperHalftile(TileInfo *ti, Corner corner, const RailtypeInfo *rti, RailGroundType rgt)
{
	DrawFoundation(ti, HalftileFoundation(corner));

	if (rti->UsesOverlay()) {
		DrawUpperHalftileOverlay(ti, corner, rti, rgt);
	} else {
		DrawUpperHalftileNonOverlay(ti, corner, rti, rgt);
	}
}

/**
 * Draw ground sprite and track bits
 * @param ti TileInfo
 * @param track TrackBits to draw
 */
static void DrawTrack(TileInfo *ti, TrackBits track)
{
	RailGroundType rgt = IsTileSubtype(ti->tile, TT_TRACK) ? GetRailGroundType(ti->tile) :
		IsOnSnow(ti->tile) ? RAIL_GROUND_ICE_DESERT : RAIL_GROUND_GRASS;
	Foundation f = IsTileSubtype(ti->tile, TT_TRACK) ? GetRailFoundation(ti->tileh, track) : FOUNDATION_LEVELED;
	Corner halftile_corner = CORNER_INVALID;
	bool draw_ground;
	const RailtypeInfo *rti, *halftile_rti;

	if (IsNonContinuousFoundation(f)) {
		/* Save halftile corner */
		if (f == FOUNDATION_STEEP_BOTH) {
			halftile_corner = GetHighestSlopeCorner(ti->tileh);
			f = FOUNDATION_STEEP_LOWER;
		} else {
			halftile_corner = GetHalftileFoundationCorner(f);
			f = FOUNDATION_NONE;
		}
		Track halftile_track = TrackBitsToTrack(CornerToTrackBits(halftile_corner));
		halftile_rti = GetRailTypeInfo(GetRailType(ti->tile, halftile_track));
		rti = GetRailTypeInfo(GetRailType(ti->tile, TrackToOppositeTrack(halftile_track)));
		/* Draw lower part first */
		track &= ~CornerToTrackBits(halftile_corner);
		/* Non-overlay railtypes need ground to be drawn if there is no lower halftile track */
		draw_ground = rti->UsesOverlay() || track == TRACK_BIT_NONE;
	} else {
		switch (track) {
			case TRACK_BIT_LOWER:
			case TRACK_BIT_RIGHT:
			case TRACK_BIT_LOWER_RIGHT:
				halftile_rti = NULL;
				rti = GetRailTypeInfo(GetRailType(ti->tile, TRACK_LOWER));
				draw_ground = rti->UsesOverlay();
				break;

			case TRACK_BIT_HORZ:
			case TRACK_BIT_VERT: {
				RailType halftile_rt = GetRailType(ti->tile, TRACK_LOWER);
				RailType rt = GetRailType(ti->tile, TRACK_UPPER);
				if (halftile_rt != rt) {
					halftile_rti = GetRailTypeInfo(halftile_rt);
					rti = GetRailTypeInfo(rt);
					draw_ground = true;
					break;
				}
			}
				/* fall through */
			default:
				halftile_rti = NULL;
				rti = GetRailTypeInfo(GetRailType(ti->tile, TRACK_UPPER));
				draw_ground = rti->UsesOverlay();
				break;
		}
	}

	DrawFoundation(ti, f, IsTileSubtype(ti->tile, TT_BRIDGE) ? GetTunnelBridgeDirection(ti->tile) : INVALID_DIAGDIR);
	/* DrawFoundation modifies ti */

	if (draw_ground) {
		/* Draw ground */
		DrawTrackGround(ti, rgt, track != TRACK_BIT_NONE);
	}

	if (IsValidCorner(halftile_corner) || halftile_rti == NULL) {
		DrawTrackBits(ti, track, rti, rgt);

		if (IsValidCorner(halftile_corner)) {
			DrawUpperHalftile(ti, halftile_corner, halftile_rti, rgt);
		}
	} else if (track == TRACK_BIT_HORZ) {
		DrawHalftile(ti, CORNER_S, halftile_rti, rgt);
		DrawHalftile(ti, CORNER_N, rti, rgt);
	} else {
		DrawHalftile(ti, CORNER_W, rti, rgt);
		DrawHalftile(ti, CORNER_E, halftile_rti, rgt);
	}
}

/**
 * Get surface height in point (x,y)
 * On tiles with halftile foundations move (x,y) to a safe point wrt. track
 */
static uint GetSafeSlopePixelZ(TileIndex tile, uint x, uint y, Track track)
{
	switch (track) {
		case TRACK_UPPER: x &= ~0xF; y &= ~0xF; break;
		case TRACK_LOWER: x |=  0xF; y |=  0xF; break;
		case TRACK_LEFT:  x |=  0xF; y &= ~0xF; break;
		case TRACK_RIGHT: x &= ~0xF; y |=  0xF; break;
		default: break;
	}

	uint z = GetSlopePixelZ_Track(tile, x, y);

	if (IsTileSubtype(tile, TT_BRIDGE) && !IsExtendedRailBridge(tile)) {
		assert(IsDiagonalTrack(track));
		z += GetBridgePartialPixelZ(GetTunnelBridgeDirection(tile), x & 0xF, y & 0xF);
	}

	return z;
}

static void DrawSingleSignal(TileIndex tile, Trackdir trackdir)
{
	static const struct {
		Point pos[2];        // signal position (left side, right side)
		SignalOffsets image; // offset from base signal sprite
	} SignalData[] = {
		{ { {11,  3}, {11, 13} }, SIGNAL_TO_NORTHEAST }, // TRACKDIR_X_NE
		{ { { 3,  4}, {13,  4} }, SIGNAL_TO_SOUTHEAST }, // TRACKDIR_Y_SE
		{ { { 1,  0}, {10,  4} }, SIGNAL_TO_EAST      }, // TRACKDIR_UPPER_E
		{ { {11,  4}, {14, 14} }, SIGNAL_TO_EAST      }, // TRACKDIR_LOWER_E
		{ { { 8,  5}, {14,  1} }, SIGNAL_TO_SOUTH     }, // TRACKDIR_LEFT_S
		{ { { 1, 14}, { 4,  6} }, SIGNAL_TO_SOUTH     }, // TRACKDIR_RIGHT_S
		{ { { 0,  0}, { 0,  0} }, SIGNAL_TO_NORTHEAST }, // TRACKDIR_RVREV_NE
		{ { { 0,  0}, { 0,  0} }, SIGNAL_TO_NORTHEAST }, // TRACKDIR_RVREV_SE
		{ { { 4, 13}, { 4,  3} }, SIGNAL_TO_SOUTHWEST }, // TRACKDIR_X_SW
		{ { {11, 13}, { 3, 11} }, SIGNAL_TO_NORTHWEST }, // TRACKDIR_Y_NW
		{ { { 3, 10}, { 0,  1} }, SIGNAL_TO_WEST      }, // TRACKDIR_UPPER_W
		{ { {14, 14}, { 5, 12} }, SIGNAL_TO_WEST      }, // TRACKDIR_LOWER_W
		{ { {14,  1}, {12, 10} }, SIGNAL_TO_NORTH     }, // TRACKDIR_LEFT_N
		{ { { 9, 11}, { 1, 14} }, SIGNAL_TO_NORTH     }, // TRACKDIR_RIGHT_N
	//	{ { { 0,  0}, { 0,  0} }, SIGNAL_TO_NORTHEAST }, // TRACKDIR_RVREV_SW
	//	{ { { 0,  0}, { 0,  0} }, SIGNAL_TO_NORTHEAST }, // TRACKDIR_RVREV_NW
	};

	if (!HasSignalOnTrackdir(tile, trackdir)) return;

	Track track = TrackdirToTrack(trackdir);
	SignalType type       = GetSignalType(tile, track);
	SignalVariant variant = GetSignalVariant(tile, track);
	SignalState condition = GetSignalStateByTrackdir(tile, trackdir);

	SpriteID sprite = GetCustomSignalSprite(GetRailTypeInfo(GetRailType(tile, track)), tile, type, variant, condition);
	SignalOffsets image = SignalData[trackdir].image;
	if (sprite != 0) {
		sprite += image;
	} else {
		/* Normal electric signals are stored in a different sprite block than all other signals. */
		sprite = (type == SIGTYPE_NORMAL && variant == SIG_ELECTRIC) ? SPR_ORIGINAL_SIGNALS_BASE : SPR_SIGNALS_BASE - 16;
		sprite += type * 16 + variant * 64 + image * 2 + condition + (type > SIGTYPE_LAST_NOPBS ? 64 : 0);
	}

	bool side;
	switch (_settings_game.construction.train_signal_side) {
		case 0:  side = false;                                 break; // left
		case 2:  side = true;                                  break; // right
		default: side = _settings_game.vehicle.road_side != 0; break; // driving side
	}
	uint x = TileX(tile) * TILE_SIZE + SignalData[trackdir].pos[side].x;
	uint y = TileY(tile) * TILE_SIZE + SignalData[trackdir].pos[side].y;

	AddSortableSpriteToDraw(sprite, PAL_NONE, x, y, 1, 1, BB_HEIGHT_UNDER_BRIDGE, GetSafeSlopePixelZ(tile, x, y, track));
}

static void DrawSignals(TileIndex tile, TrackBits rails)
{
	if (rails & TRACK_BIT_Y) {
		DrawSingleSignal(tile, TRACKDIR_Y_SE);
		DrawSingleSignal(tile, TRACKDIR_Y_NW);
	} else if (rails & TRACK_BIT_X) {
		DrawSingleSignal(tile, TRACKDIR_X_NE);
		DrawSingleSignal(tile, TRACKDIR_X_SW);
	} else {
		if (rails & TRACK_BIT_LEFT) {
			DrawSingleSignal(tile, TRACKDIR_LEFT_S);
			DrawSingleSignal(tile, TRACKDIR_LEFT_N);
		}
		if (rails & TRACK_BIT_RIGHT) {
			DrawSingleSignal(tile, TRACKDIR_RIGHT_S);
			DrawSingleSignal(tile, TRACKDIR_RIGHT_N);
		}
		if (rails & TRACK_BIT_UPPER) {
			DrawSingleSignal(tile, TRACKDIR_UPPER_E);
			DrawSingleSignal(tile, TRACKDIR_UPPER_W);
		}
		if (rails & TRACK_BIT_LOWER) {
			DrawSingleSignal(tile, TRACKDIR_LOWER_E);
			DrawSingleSignal(tile, TRACKDIR_LOWER_W);
		}
	}
}

static void DrawTile_Track(TileInfo *ti)
{
	if (IsTileSubtype(ti->tile, TT_TRACK) || IsExtendedRailBridge(ti->tile)) {
		_drawtile_track_palette = COMPANY_SPRITE_COLOUR(GetTileOwner(ti->tile));

		TrackBits rails = GetTrackBits(ti->tile);

		DrawTrack(ti, rails);

		if (HasBit(_display_opt, DO_FULL_DETAIL) && IsTileSubtype(ti->tile, TT_TRACK)) DrawTrackDetails(ti, rails);

		if (IsCatenaryDrawn()) DrawCatenary(ti);

		DrawSignals(ti->tile, rails);
	} else {
		DrawBridgeGround(ti);

		/* draw ramp */

		const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));

		DiagDirection dir = GetTunnelBridgeDirection(ti->tile);

		assert(rti->bridge_offset != 8); // This one is used for roads
		const PalSpriteID *psid = GetBridgeRampSprite(GetRailBridgeType(ti->tile), rti->bridge_offset, ti->tileh, dir);

		/* Draw PBS Reservation as SpriteCombine */
		StartSpriteCombine();

		/* HACK set the height of the BB of a sloped ramp to 1 so a vehicle on
		 * it doesn't disappear behind it
		 */
		/* Bridge heads are drawn solid no matter how invisibility/transparency is set */
		AddSortableSpriteToDraw(psid->sprite, psid->pal, ti->x, ti->y, 16, 16, ti->tileh == SLOPE_FLAT ? 0 : 8, ti->z);

		if (rti->UsesOverlay()) {
			SpriteID surface = GetCustomRailSprite(rti, ti->tile, RTSG_BRIDGE);
			if (surface != 0) {
				if (HasBridgeFlatRamp(ti->tileh, DiagDirToAxis(dir))) {
					AddSortableSpriteToDraw(surface + ((DiagDirToAxis(dir) == AXIS_X) ? RTBO_X : RTBO_Y), PAL_NONE, ti->x, ti->y, 16, 16, 0, ti->z + 8);
				} else {
					AddSortableSpriteToDraw(surface + RTBO_SLOPE + dir, PAL_NONE, ti->x, ti->y, 16, 16, 8, ti->z);
				}
			}
			/* Don't fallback to non-overlay sprite -- the spec states that
			 * if an overlay is present then the bridge surface must be
			 * present. */
		}

		/* PBS debugging, draw reserved tracks darker */
		if (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation && GetRailReservationTrackBits(ti->tile) != TRACK_BIT_NONE) {
			if (HasBridgeFlatRamp(ti->tileh, DiagDirToAxis(dir))) {
				AddSortableSpriteToDraw(DiagDirToAxis(dir) == AXIS_X ? rti->base_sprites.single_x : rti->base_sprites.single_y, PALETTE_CRASH, ti->x, ti->y, 16, 16, 0, ti->z + 8);
			} else {
				AddSortableSpriteToDraw(rti->base_sprites.single_sloped + dir, PALETTE_CRASH, ti->x, ti->y, 16, 16, 8, ti->z);
			}
		}

		EndSpriteCombine();

		if (HasCatenaryDrawn(GetRailType(ti->tile))) {
			DrawCatenary(ti);
		}

		if (DiagDirToAxis(dir) == AXIS_Y) {
			DrawSingleSignal(ti->tile, TRACKDIR_Y_SE);
			DrawSingleSignal(ti->tile, TRACKDIR_Y_NW);
		} else {
			DrawSingleSignal(ti->tile, TRACKDIR_X_NE);
			DrawSingleSignal(ti->tile, TRACKDIR_X_SW);
		}
	}

	DrawBridgeMiddle(ti);
}

static Foundation GetFoundation_Track(TileIndex tile, Slope tileh)
{
	return IsTileSubtype(tile, TT_TRACK) ? GetRailFoundation(tileh, GetTrackBits(tile)) :
		IsExtendedRailBridge(tile) ? FOUNDATION_LEVELED :
		GetBridgeFoundation(tileh, DiagDirToAxis(GetTunnelBridgeDirection(tile)));
}

static void TileLoop_Track(TileIndex tile)
{
	if (IsTileSubtype(tile, TT_BRIDGE)) {
		bool snow_or_desert = IsOnSnow(tile);
		switch (_settings_game.game_creation.landscape) {
			default: return;

			case LT_ARCTIC:
				/* As long as we do not have a snow density, we want to use the density
				 * from the entry edge. For bridges this is the highest point.
				 * (Independent of foundations) */
				if (snow_or_desert == (GetTileMaxZ(tile) > GetSnowLine())) return;
				break;

			case LT_TROPIC:
				if (GetTropicZone(tile) != TROPICZONE_DESERT || snow_or_desert) return;
				break;
		}
		ToggleSnow(tile);
		MarkTileDirtyByTile(tile);
		return;
	}

	RailGroundType old_ground = GetRailGroundType(tile);
	RailGroundType new_ground;

	if (old_ground == RAIL_GROUND_WATER) {
		TileLoop_Water(tile);
		return;
	}

	switch (_settings_game.game_creation.landscape) {
		case LT_ARCTIC: {
			int z;
			Slope slope = GetTileSlope(tile, &z);
			bool half = false;

			/* for non-flat track, use lower part of track
			 * in other cases, use the highest part with track */
			TrackBits track = GetTrackBits(tile);
			Foundation f = GetRailFoundation(slope, track);

			switch (f) {
				case FOUNDATION_NONE:
					/* no foundation - is the track on the upper side of three corners raised tile? */
					if (IsSlopeWithThreeCornersRaised(slope)) z++;
					break;

				case FOUNDATION_INCLINED_X:
				case FOUNDATION_INCLINED_Y:
					/* sloped track - is it on a steep slope? */
					if (IsSteepSlope(slope)) z++;
					break;

				case FOUNDATION_STEEP_LOWER:
					/* only lower part of steep slope */
					z++;
					break;

				default:
					/* if it is a steep slope, then there is a track on higher part */
					if (IsSteepSlope(slope)) z++;
					z++;
					break;
			}

			half = IsInsideMM(f, FOUNDATION_STEEP_BOTH, FOUNDATION_HALFTILE_N + 1);

			/* 'z' is now the lowest part of the highest track bit -
			 * for sloped track, it is 'z' of lower part
			 * for two track bits, it is 'z' of higher track bit
			 * For non-continuous foundations (and STEEP_BOTH), 'half' is set */
			if (z > GetSnowLine()) {
				if (half && z - GetSnowLine() == 1) {
					/* track on non-continuous foundation, lower part is not under snow */
					new_ground = RAIL_GROUND_HALF_SNOW;
				} else {
					new_ground = RAIL_GROUND_ICE_DESERT;
				}
				goto set_ground;
			}
			break;
			}

		case LT_TROPIC:
			if (GetTropicZone(tile) == TROPICZONE_DESERT) {
				new_ground = RAIL_GROUND_ICE_DESERT;
				goto set_ground;
			}
			break;
	}

	new_ground = RAIL_GROUND_GRASS;

	if (old_ground != RAIL_GROUND_BARREN) { // wait until bottom is green
		/* determine direction of fence */
		TrackBits rail = GetTrackBits(tile);

		Owner owner = GetTileOwner(tile);
		byte fences = 0;

		for (DiagDirection d = DIAGDIR_BEGIN; d < DIAGDIR_END; d++) {
			static const TrackBits dir_to_trackbits[DIAGDIR_END] = {TRACK_BIT_3WAY_NE, TRACK_BIT_3WAY_SE, TRACK_BIT_3WAY_SW, TRACK_BIT_3WAY_NW};

			/* Track bit on this edge => no fence. */
			if ((rail & dir_to_trackbits[d]) != TRACK_BIT_NONE) continue;

			TileIndex tile2 = tile + TileOffsByDiagDir(d);

			/* Show fences if it's a house, industry, object, road, tunnelbridge or not owned by us. */
			if (!IsValidTile(tile2) || IsHouseTile(tile2) || IsIndustryTile(tile2) ||
					(IsTileType(tile2, TT_MISC) && !IsRailDepotTile(tile2)) ||
					IsRoadTile(tile2) || (IsRailBridgeTile(tile2) && !IsExtendedRailBridge(tile2)) ||
					(IsObjectTile(tile2) && !IsOwnedLand(tile2)) || !IsTileOwner(tile2, owner)) {
				fences |= 1 << d;
			}
		}

		switch (fences) {
			case 0: break;
			case (1 << DIAGDIR_NE): new_ground = RAIL_GROUND_FENCE_NE; break;
			case (1 << DIAGDIR_SE): new_ground = RAIL_GROUND_FENCE_SE; break;
			case (1 << DIAGDIR_SW): new_ground = RAIL_GROUND_FENCE_SW; break;
			case (1 << DIAGDIR_NW): new_ground = RAIL_GROUND_FENCE_NW; break;
			case (1 << DIAGDIR_NE) | (1 << DIAGDIR_SW): new_ground = RAIL_GROUND_FENCE_NESW; break;
			case (1 << DIAGDIR_SE) | (1 << DIAGDIR_NW): new_ground = RAIL_GROUND_FENCE_SENW; break;
			case (1 << DIAGDIR_NE) | (1 << DIAGDIR_SE): new_ground = RAIL_GROUND_FENCE_VERT1; break;
			case (1 << DIAGDIR_NE) | (1 << DIAGDIR_NW): new_ground = RAIL_GROUND_FENCE_HORIZ2; break;
			case (1 << DIAGDIR_SE) | (1 << DIAGDIR_SW): new_ground = RAIL_GROUND_FENCE_HORIZ1; break;
			case (1 << DIAGDIR_SW) | (1 << DIAGDIR_NW): new_ground = RAIL_GROUND_FENCE_VERT2; break;
			default: NOT_REACHED();
		}
	}

set_ground:
	if (old_ground != new_ground) {
		SetRailGroundType(tile, new_ground);
		MarkTileDirtyByTile(tile);
	}
}


static TrackStatus GetTileTrackStatus_Track(TileIndex tile, TransportType mode, uint sub_mode, DiagDirection side)
{
	/* Case of half tile slope with water. */
	if (mode == TRANSPORT_WATER && IsTileSubtype(tile, TT_TRACK) && GetRailGroundType(tile) == RAIL_GROUND_WATER && IsSlopeWithOneCornerRaised(GetTileSlope(tile))) {
		TrackBits tb = GetTrackBits(tile);
		switch (tb) {
			default: NOT_REACHED();
			case TRACK_BIT_UPPER: tb = TRACK_BIT_LOWER; break;
			case TRACK_BIT_LOWER: tb = TRACK_BIT_UPPER; break;
			case TRACK_BIT_LEFT:  tb = TRACK_BIT_RIGHT; break;
			case TRACK_BIT_RIGHT: tb = TRACK_BIT_LEFT;  break;
		}
		return CombineTrackStatus(TrackBitsToTrackdirBits(tb), TRACKDIR_BIT_NONE);
	}

	if (mode != TRANSPORT_RAIL) return 0;

	if (IsTileSubtype(tile, TT_BRIDGE)) {
		if (side == GetTunnelBridgeDirection(tile)) return 0;
	}

	TrackBits trackbits = GetTrackBits(tile);
	TrackdirBits red_signals = TRACKDIR_BIT_NONE;

	uint a, b;

	a = GetPresentSignals(tile, TRACK_UPPER);
	/* When signals are not present (in neither direction),
	 * we pretend them to be green. Otherwise, it depends on
	 * the signal type. For signals that are only active from
	 * one side, we set the missing signals explicitly to
	 * `green'. Otherwise, they implicitly become `red'. */
	if (a == 0) {
		b = 3;
	} else {
		b = GetSignalStates(tile, TRACK_UPPER) & a;
		if (!IsOnewaySignal(tile, TRACK_UPPER)) b |= ~a;
	}

	if ((b & 0x2) == 0) red_signals |= (TRACKDIR_BIT_LEFT_N | TRACKDIR_BIT_X_NE | TRACKDIR_BIT_Y_SE | TRACKDIR_BIT_UPPER_E);
	if ((b & 0x1) == 0) red_signals |= (TRACKDIR_BIT_LEFT_S | TRACKDIR_BIT_X_SW | TRACKDIR_BIT_Y_NW | TRACKDIR_BIT_UPPER_W);

	a = GetPresentSignals(tile, TRACK_LOWER);
	if (a == 0) {
		b = 3;
	} else {
		b = GetSignalStates(tile, TRACK_LOWER) & a;
		if (!IsOnewaySignal(tile, TRACK_LOWER)) b |= ~a;
	}

	if ((b & 0x2) == 0) red_signals |= (TRACKDIR_BIT_RIGHT_N | TRACKDIR_BIT_LOWER_E);
	if ((b & 0x1) == 0) red_signals |= (TRACKDIR_BIT_RIGHT_S | TRACKDIR_BIT_LOWER_W);

	return CombineTrackStatus(TrackBitsToTrackdirBits(trackbits), red_signals);
}

static bool ClickTile_Track(TileIndex tile)
{
	return false;
}

static void GetTileDesc_Track(TileIndex tile, TileDesc *td)
{
	static const StringID signal_type[6][6] = {
		{
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_SIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_PRESIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_EXITSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_COMBOSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_NOENTRYSIGNALS
		},
		{
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_PRESIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRESIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_EXITSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_COMBOSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_NOENTRYSIGNALS
		},
		{
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_EXITSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_EXITSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_EXITSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_EXIT_COMBOSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_EXIT_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_EXIT_NOENTRYSIGNALS
		},
		{
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_COMBOSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_COMBOSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_EXIT_COMBOSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_COMBOSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_COMBO_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_COMBO_NOENTRYSIGNALS
		},
		{
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_EXIT_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_COMBO_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PBSSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PBS_NOENTRYSIGNALS
		},
		{
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NORMAL_NOENTRYSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PRE_NOENTRYSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_EXIT_NOENTRYSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_COMBO_NOENTRYSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_PBS_NOENTRYSIGNALS,
			STR_LAI_RAIL_DESCRIPTION_TRACK_WITH_NOENTRYSIGNALS
		}
	};

	const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(tile));
	td->rail_speed = rti->max_speed;
	td->owner[0] = GetTileOwner(tile);

	if (IsTileSubtype(tile, TT_TRACK)) {
		SetDParamX(td->dparam, 0, rti->strings.name);

		if (HasSignalOnTrack(tile, TRACK_UPPER)) {
			SignalType primary = GetSignalType(tile, TRACK_UPPER);
			SignalType secondary = HasSignalOnTrack(tile, TRACK_LOWER) ? GetSignalType(tile, TRACK_LOWER) : primary;
			td->str = signal_type[secondary][primary];
		} else if (HasSignalOnTrack(tile, TRACK_LOWER)) {
			SignalType signal = GetSignalType(tile, TRACK_LOWER);
			td->str = signal_type[signal][signal];
		} else {
			td->str = STR_LAI_RAIL_DESCRIPTION_TRACK;
		}
	} else {
		const BridgeSpec *spec = GetBridgeSpec(GetRailBridgeType(tile));
		td->str = spec->transport_name[TRANSPORT_RAIL];

		uint16 spd = spec->speed;
		if (td->rail_speed == 0 || spd < td->rail_speed) {
			td->rail_speed = spd;
		}
	}
}

static void ChangeTileOwner_Track(TileIndex tile, Owner old_owner, Owner new_owner)
{
	if (!IsTileOwner(tile, old_owner)) return;

	if (new_owner != INVALID_OWNER) {
		/* Update company infrastructure counts. No need to dirty windows here, we'll redraw the whole screen anyway. */
		TrackBits bits = GetTrackBits(tile);
		uint factor = IsTileSubtype(tile, TT_BRIDGE) ? TUNNELBRIDGE_TRACKBIT_FACTOR : 1;
		RailType rt;
		uint num_sigs;

		switch (bits) {
			case TRACK_BIT_HORZ:
			case TRACK_BIT_VERT:
				if (IsTileSubtype(tile, TT_BRIDGE)) {
					DiagDirection dir = GetTunnelBridgeDirection(tile);
					rt = GetSideRailType(tile, dir);
					Company::Get(old_owner)->infrastructure.rail[rt] -= TUNNELBRIDGE_TRACKBIT_FACTOR;
					Company::Get(new_owner)->infrastructure.rail[rt] += TUNNELBRIDGE_TRACKBIT_FACTOR;
					rt = GetSideRailType(tile, ReverseDiagDir(dir));
				} else {
					rt = GetRailType(tile, TRACK_UPPER);
					Company::Get(old_owner)->infrastructure.rail[rt]--;
					Company::Get(new_owner)->infrastructure.rail[rt]++;
					rt = GetRailType(tile, TRACK_LOWER);
				}
				Company::Get(old_owner)->infrastructure.rail[rt]--;
				Company::Get(new_owner)->infrastructure.rail[rt]++;
				num_sigs = CountBits(GetPresentSignals(tile, TRACK_UPPER)) + CountBits(GetPresentSignals(tile, TRACK_LOWER));
				break;

			case TRACK_BIT_RIGHT:
			case TRACK_BIT_LOWER:
				rt = GetRailType(tile, TRACK_LOWER);
				Company::Get(old_owner)->infrastructure.rail[rt] -= factor;
				Company::Get(new_owner)->infrastructure.rail[rt] += factor;
				num_sigs = CountBits(GetPresentSignals(tile, TRACK_LOWER));
				break;

			case TRACK_BIT_LOWER_RIGHT:
				rt = GetRailType(tile, TRACK_LOWER);
				Company::Get(old_owner)->infrastructure.rail[rt] -= 2 * 2 * factor;
				Company::Get(new_owner)->infrastructure.rail[rt] += 2 * 2 * factor;
				num_sigs = 0;
				break;

			default: {
				rt = GetRailType(tile, TRACK_UPPER);
				uint num_pieces = CountBits(bits);
				if (TracksOverlap(bits)) {
					num_pieces *= num_pieces;
					num_sigs = 0;
				} else {
					num_sigs = CountBits(GetPresentSignals(tile, TRACK_UPPER));
				}
				num_pieces *= factor;
				Company::Get(old_owner)->infrastructure.rail[rt] -= num_pieces;
				Company::Get(new_owner)->infrastructure.rail[rt] += num_pieces;
				break;
			}
		}

		Company::Get(old_owner)->infrastructure.signal -= num_sigs;
		Company::Get(new_owner)->infrastructure.signal += num_sigs;

		if (IsTileSubtype(tile, TT_BRIDGE)) {
			TileIndex other_end = GetOtherBridgeEnd(tile);
			if (tile < other_end) {
				uint num_pieces = GetTunnelBridgeLength(tile, other_end) * TUNNELBRIDGE_TRACKBIT_FACTOR;
				RailType rt = GetBridgeRailType(tile);
				Company::Get(old_owner)->infrastructure.rail[rt] -= num_pieces;
				Company::Get(new_owner)->infrastructure.rail[rt] += num_pieces;
			}
		}

		SetTileOwner(tile, new_owner);
	} else {
		DoCommand(tile, 0, 0, DC_EXEC | DC_BANKRUPT, CMD_LANDSCAPE_CLEAR);
	}
}

/**
 * Tests if autoslope is allowed.
 *
 * @param tile The tile.
 * @param flags Terraform command flags.
 * @param z_old Old TileZ.
 * @param tileh_old Old TileSlope.
 * @param z_new New TileZ.
 * @param tileh_new New TileSlope.
 * @param rail_bits Trackbits.
 */
static CommandCost TestAutoslopeOnRailTile(TileIndex tile, uint flags, int z_old, Slope tileh_old, int z_new, Slope tileh_new, TrackBits rail_bits)
{
	if (!_settings_game.construction.build_on_slopes || !AutoslopeEnabled()) return_cmd_error(STR_ERROR_MUST_REMOVE_RAILROAD_TRACK);

	/* Is the slope-rail_bits combination valid in general? I.e. is it safe to call GetRailFoundation() ? */
	if (CheckRailSlope(tileh_new, rail_bits, TRACK_BIT_NONE, tile).Failed()) return_cmd_error(STR_ERROR_MUST_REMOVE_RAILROAD_TRACK);

	/* Get the slopes on top of the foundations */
	z_old += ApplyFoundationToSlope(GetRailFoundation(tileh_old, rail_bits), &tileh_old);
	z_new += ApplyFoundationToSlope(GetRailFoundation(tileh_new, rail_bits), &tileh_new);

	Corner track_corner;
	switch (rail_bits) {
		case TRACK_BIT_LEFT:  track_corner = CORNER_W; break;
		case TRACK_BIT_LOWER: track_corner = CORNER_S; break;
		case TRACK_BIT_RIGHT: track_corner = CORNER_E; break;
		case TRACK_BIT_UPPER: track_corner = CORNER_N; break;

		/* Surface slope must not be changed */
		default:
			if (z_old != z_new || tileh_old != tileh_new) return_cmd_error(STR_ERROR_MUST_REMOVE_RAILROAD_TRACK);
			return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
	}

	/* The height of the track_corner must not be changed. The rest ensures GetRailFoundation() already. */
	z_old += GetSlopeZInCorner(RemoveHalftileSlope(tileh_old), track_corner);
	z_new += GetSlopeZInCorner(RemoveHalftileSlope(tileh_new), track_corner);
	if (z_old != z_new) return_cmd_error(STR_ERROR_MUST_REMOVE_RAILROAD_TRACK);

	CommandCost cost = CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
	/* Make the ground dirty, if surface slope has changed */
	if (tileh_old != tileh_new) {
		/* If there is flat water on the lower halftile add the cost for clearing it */
		if (GetRailGroundType(tile) == RAIL_GROUND_WATER && IsSlopeWithOneCornerRaised(tileh_old)) cost.AddCost(_price[PR_CLEAR_WATER]);
		if ((flags & DC_EXEC) != 0) SetRailGroundType(tile, RAIL_GROUND_BARREN);
	}
	return  cost;
}

/**
 * Test-procedure for HasVehicleOnPos to check for a ship.
 */
static Vehicle *EnsureNoShipProc(Vehicle *v, void *data)
{
	return v->type == VEH_SHIP ? v : NULL;
}

static CommandCost TerraformTile_Track(TileIndex tile, DoCommandFlag flags, int z_new, Slope tileh_new)
{
	int z_old;
	Slope tileh_old = GetTileSlope(tile, &z_old);

	if (IsTileSubtype(tile, TT_TRACK)) {
		TrackBits rail_bits = GetTrackBits(tile);
		/* Is there flat water on the lower halftile that must be cleared expensively? */
		bool was_water = (GetRailGroundType(tile) == RAIL_GROUND_WATER && IsSlopeWithOneCornerRaised(tileh_old));

		/* Allow clearing the water only if there is no ship */
		if (was_water && HasVehicleOnPos(tile, NULL, &EnsureNoShipProc)) return_cmd_error(STR_ERROR_SHIP_IN_THE_WAY);

		/* First test autoslope. However if it succeeds we still have to test the rest, because non-autoslope terraforming is cheaper. */
		CommandCost autoslope_result = TestAutoslopeOnRailTile(tile, flags, z_old, tileh_old, z_new, tileh_new, rail_bits);

		/* When there is only a single horizontal/vertical track, one corner can be terraformed. */
		Corner allowed_corner;
		switch (rail_bits) {
			case TRACK_BIT_RIGHT: allowed_corner = CORNER_W; break;
			case TRACK_BIT_UPPER: allowed_corner = CORNER_S; break;
			case TRACK_BIT_LEFT:  allowed_corner = CORNER_E; break;
			case TRACK_BIT_LOWER: allowed_corner = CORNER_N; break;
			default: return autoslope_result;
		}

		Foundation f_old = GetRailFoundation(tileh_old, rail_bits);

		/* Do not allow terraforming if allowed_corner is part of anti-zig-zag foundations */
		if (tileh_old != SLOPE_NS && tileh_old != SLOPE_EW && IsSpecialRailFoundation(f_old)) return autoslope_result;

		/* Everything is valid, which only changes allowed_corner */
		for (Corner corner = (Corner)0; corner < CORNER_END; corner = (Corner)(corner + 1)) {
			if (allowed_corner == corner) continue;
			if (z_old + GetSlopeZInCorner(tileh_old, corner) != z_new + GetSlopePixelZInCorner(tileh_new, corner)) return autoslope_result;
		}

		/* Make the ground dirty */
		if ((flags & DC_EXEC) != 0) SetRailGroundType(tile, RAIL_GROUND_BARREN);

		/* allow terraforming */
		return CommandCost(EXPENSES_CONSTRUCTION, was_water ? _price[PR_CLEAR_WATER] : (Money)0);
	} else {
		if (_settings_game.construction.build_on_slopes && AutoslopeEnabled()) {
			DiagDirection direction = GetTunnelBridgeDirection(tile);

			if (IsExtendedRailBridge(tile)) {
				if (IsValidRailBridgeBits(tileh_new, direction, GetTrackBits(tile))) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
			} else {
				/* Check if new slope is valid for bridges in general (so we can safely call GetBridgeFoundation()) */
				CheckBridgeSlope(direction, &tileh_old, &z_old);
				CommandCost res = CheckBridgeSlope(direction, &tileh_new, &z_new);

				/* Surface slope is valid and remains unchanged? */
				if (res.Succeeded() && (z_old == z_new) && (tileh_old == tileh_new)) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
			}
		}

		return DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
	}
}


extern const TileTypeProcs _tile_type_rail_procs = {
	DrawTile_Track,           // draw_tile_proc
	GetSlopePixelZ_Track,     // get_slope_z_proc
	ClearTile_Track,          // clear_tile_proc
	NULL,                     // add_accepted_cargo_proc
	GetTileDesc_Track,        // get_tile_desc_proc
	GetTileTrackStatus_Track, // get_tile_track_status_proc
	ClickTile_Track,          // click_tile_proc
	NULL,                     // animate_tile_proc
	TileLoop_Track,           // tile_loop_proc
	ChangeTileOwner_Track,    // change_tile_owner_proc
	NULL,                     // add_produced_cargo_proc
	GetFoundation_Track,      // get_foundation_proc
	TerraformTile_Track,      // terraform_tile_proc
};
