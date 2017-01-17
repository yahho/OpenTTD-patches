/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file elrail.cpp
 * This file deals with displaying wires and pylons for electric railways.
 * <h2>Basics</h2>
 *
 * <h3>Tile Types</h3>
 *
 * We have two different types of tiles in the drawing code:
 * Normal Railway Tiles (NRTs) which can have more than one track on it, and
 * Special Railways tiles (SRTs) which have only one track (like crossings, depots
 * stations, etc).
 *
 * <h3>Location Categories</h3>
 *
 * All tiles are categorized into three location groups (TLG):
 * Group 0: Tiles with both an even X coordinate and an even Y coordinate
 * Group 1: Tiles with an even X and an odd Y coordinate
 * Group 2: Tiles with an odd X and an even Y coordinate
 * Group 3: Tiles with both an odd X and Y coordinate.
 *
 * <h3>Pylon Points</h3>
 * <h4>Control Points</h4>
 * A Pylon Control Point (PCP) is a position where a wire (or rather two)
 * is mounted onto a pylon.
 * Each NRT does contain 4 PCPs which are bitmapped to a byte
 * variable and are represented by the DiagDirection enum
 *
 * Each track ends on two PCPs and thus requires one pylon on each end. However,
 * there is one exception: Straight-and-level tracks only have one pylon every
 * other tile.
 *
 * Now on each edge there are two PCPs: One from each adjacent tile. Both PCPs
 * are merged using an OR operation (i. e. if one tile needs a PCP at the position
 * in question, both tiles get it).
 *
 * <h4>Position Points</h4>
 * A Pylon Position Point (PPP) is a position where a pylon is located on the
 * ground.  Each PCP owns 8 in (45 degree steps) PPPs that are located around
 * it. PPPs are represented using the Direction enum. Each track bit has PPPs
 * that are impossible (because the pylon would be situated on the track) and
 * some that are preferred (because the pylon would be rectangular to the track).
 *
 * @image html elrail_tile.png
 * @image html elrail_track.png
 *
 */

#include "stdafx.h"

#include <utility>

#include "map/rail.h"
#include "map/road.h"
#include "map/slope.h"
#include "map/bridge.h"
#include "map/tunnelbridge.h"
#include "viewport_func.h"
#include "train.h"
#include "rail_gui.h"
#include "bridge.h"
#include "elrail_func.h"
#include "company_base.h"
#include "newgrf_railtype.h"
#include "station_func.h"

#include "table/elrail_data.h"

/**
 * Check if a tile is on an odd X coordinate.
 * @param t The tile to check
 * @return Whether the tile is on an odd X coordinate
 */
static inline bool IsOddX (TileIndex t)
{
	return HasBit (TileX(t), 0);
}

/**
 * Check if a tile is on an odd Y coordinate.
 * @param t The tile to check
 * @return Whether the tile is on an odd Y coordinate
 */
static inline bool IsOddY (TileIndex t)
{
	return HasBit (TileY(t), 0);
}

/** Get the electrified track bits on a railway tile. */
static TrackBits GetElectrifiedTrackBits (TileIndex t)
{
	TrackBits present = GetTrackBits(t);
	TrackBits result = TRACK_BIT_NONE;
	if (HasCatenary (GetRailType (t, TRACK_UPPER))) result |= present & (TRACK_BIT_CROSS | TRACK_BIT_UPPER | TRACK_BIT_LEFT);
	if (HasCatenary (GetRailType (t, TRACK_LOWER))) result |= present & (TRACK_BIT_LOWER | TRACK_BIT_RIGHT);
	return result;
}

/**
 * Finds which Electrified Rail Bits are present on a given tile.
 * @param t tile to check
 * @param dir direction this tile is from the home tile, or INVALID_TILE for the home tile itself
 * @param override pointer to PCP override, can be NULL
 * @return trackbits of tile if it is electrified
 */
static TrackBits GetRailTrackBitsUniversal(TileIndex t, DiagDirection dir, DiagDirection *override = NULL)
{
	switch (GetTileType(t)) {
		case TT_RAILWAY:
			if (IsTileSubtype(t, TT_BRIDGE) && (override != NULL)) {
				*override = GetTunnelBridgeDirection(t);
			}
			return GetElectrifiedTrackBits (t);

		case TT_MISC:
			if (GetTileSubtype(t) != TT_MISC_CROSSING) return TRACK_BIT_NONE;
			if (!HasCatenary(GetRailType(t))) return TRACK_BIT_NONE;
			return GetCrossingRailBits(t);

		case TT_STATION:
			if (!HasStationRail(t)) return TRACK_BIT_NONE;
			if (!HasCatenary(GetRailType(t))) return TRACK_BIT_NONE;
			/* Ignore neighbouring station tiles that allow neither wires nor pylons. */
			if (dir != INVALID_DIAGDIR && !CanStationTileHavePylons(t) && !CanStationTileHaveWires(t)) return TRACK_BIT_NONE;
			return TrackToTrackBits(GetRailStationTrack(t));

		default:
			return TRACK_BIT_NONE;
	}
}

/**
 * Masks out track bits when neighbouring tiles are unelectrified.
 */
static TrackBits MaskWireBits(TileIndex t, TrackBits tracks)
{
	if (!IsNormalRailTile(t)) return tracks;

	TrackdirBits neighbour_tdb = TRACKDIR_BIT_NONE;
	for (DiagDirection d = DIAGDIR_BEGIN; d < DIAGDIR_END; d++) {
		/* If the neighbour tile is either not electrified or has no tracks that can be reached
		 * from this tile, mark all trackdirs that can be reached from the neighbour tile
		 * as needing no catenary. We make an exception for blocked station tiles with a matching
		 * axis that still display wires to preserve visual continuity. */
		TileIndex next_tile = TileAddByDiagDir(t, d);
		TrackBits reachable = TrackStatusToTrackBits(GetTileRailwayStatus(next_tile)) & DiagdirReachesTracks(d);
		RailType rt;
		if ((reachable != TRACK_BIT_NONE) ?
				((rt = GetRailType(next_tile, FindFirstTrack(reachable))) == INVALID_RAILTYPE || !HasCatenary(rt)) :
				(!HasStationTileRail(next_tile) || GetRailStationAxis(next_tile) != DiagDirToAxis(d) || !CanStationTileHaveWires(next_tile))) {
			neighbour_tdb |= DiagdirReachesTrackdirs(ReverseDiagDir(d));
		}
	}

	/* If the tracks from either a diagonal crossing or don't overlap, both
	 * trackdirs have to be marked to mask the corresponding track bit. Else
	 * one marked trackdir is enough the mask the track bit. */
	TrackBits mask;
	if (tracks == TRACK_BIT_CROSS || !TracksOverlap(tracks)) {
		/* If the tracks form either a diagonal crossing or don't overlap, both
		 * trackdirs have to be marked to mask the corresponding track bit. */
		mask = ~(TrackBits)((neighbour_tdb & (neighbour_tdb >> 8)) & TRACK_BIT_MASK);
		/* If that results in no masked tracks and it is not a diagonal crossing,
		 * require only one marked trackdir to mask. */
		if (tracks != TRACK_BIT_CROSS && (mask & TRACK_BIT_MASK) == TRACK_BIT_MASK) mask = ~TrackdirBitsToTrackBits(neighbour_tdb);
	} else {
		/* Require only one marked trackdir to mask the track. */
		mask = ~TrackdirBitsToTrackBits(neighbour_tdb);
		/* If that results in an empty set, require both trackdirs for diagonal track. */
		if ((tracks & mask) == TRACK_BIT_NONE) {
			if ((neighbour_tdb & TRACKDIR_BIT_X_NE) == 0 || (neighbour_tdb & TRACKDIR_BIT_X_SW) == 0) mask |= TRACK_BIT_X;
			if ((neighbour_tdb & TRACKDIR_BIT_Y_NW) == 0 || (neighbour_tdb & TRACKDIR_BIT_Y_SE) == 0) mask |= TRACK_BIT_Y;
			/* If that still is not enough, require both trackdirs for any track. */
			if ((tracks & mask) == TRACK_BIT_NONE) mask = ~(TrackBits)((neighbour_tdb & (neighbour_tdb >> 8)) & TRACK_BIT_MASK);
		}
	}

	/* Mask the tracks only if at least one track bit would remain. */
	return (tracks & mask) != TRACK_BIT_NONE ? tracks & mask : tracks;
}

/**
 * Get the base wire sprite to use.
 */
static inline SpriteID GetWireBase(TileIndex tile, TileContext context = TCX_NORMAL, Track track = INVALID_TRACK)
{
	const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(tile, track));
	SpriteID wires = GetCustomRailSprite(rti, tile, RTSG_WIRES, context);
	return wires == 0 ? SPR_WIRE_BASE : wires;
}

/**
 * Get the base pylon sprite to use.
 */
static inline SpriteID GetPylonBase(TileIndex tile, TileContext context = TCX_NORMAL, Track track = INVALID_TRACK)
{
	const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(tile, track));
	SpriteID pylons = GetCustomRailSprite(rti, tile, RTSG_PYLONS, context);
	return pylons == 0 ? SPR_PYLON_BASE : pylons;
}

/**
 * Draws wires on a rail tunnel or depot tile.
 * @param ti The TileInfo to draw the tile for.
 * @param depot The tile is a depot, else a tunnel.
 * @param dir The direction of the tunnel or depot.
 */
void DrawRailTunnelDepotCatenary (const TileInfo *ti, bool depot,
	DiagDirection dir)
{
	struct SortableSpriteStruct {
		struct { int8 x, y, w, h; } bb[2];
		int8 x_offset;
		int8 y_offset;
	};

	static const SortableSpriteStruct data[2] = {
		{ { {  0, -6, 16,  8 }, { 0, 0, 15, 1 } }, 0, 7 }, //! Wire along X axis
		{ { { -6,  0,  8, 16 }, { 0, 0, 1, 15 } }, 7, 0 }, //! Wire along Y axis
	};

	assert_compile (WSO_ENTRANCE_NE == WSO_ENTRANCE_NE + DIAGDIR_NE);
	assert_compile (WSO_ENTRANCE_SE == WSO_ENTRANCE_NE + DIAGDIR_SE);
	assert_compile (WSO_ENTRANCE_SW == WSO_ENTRANCE_NE + DIAGDIR_SW);
	assert_compile (WSO_ENTRANCE_NW == WSO_ENTRANCE_NE + DIAGDIR_NW);

	const SortableSpriteStruct *sss = &data[DiagDirToAxis(dir)];
	int dz = depot ? 0 : BB_Z_SEPARATOR - ELRAIL_ELEVATION;
	int z = depot ? GetTileMaxPixelZ (ti->tile) : GetTilePixelZ (ti->tile);
	/* This wire is not visible with the default depot sprites. */
	AddSortableSpriteToDraw (ti->vd,
		GetWireBase (ti->tile) + WSO_ENTRANCE_NE + dir, PAL_NONE,
		ti->x + sss->x_offset, ti->y + sss->y_offset,
		sss->bb[depot].w, sss->bb[depot].h, dz + 1,
		z + ELRAIL_ELEVATION, IsTransparencySet (TO_CATENARY),
		sss->bb[depot].x, sss->bb[depot].y, dz);
}


struct SideTrackData {
	byte track;     ///< a track that incides at this side
	byte preferred; ///< preferred pylon position points for it
};

static const uint NUM_TRACKS_PER_SIDE = 3;

/* Side track data, 3 tracks per side. */
static const SideTrackData side_tracks[DIAGDIR_END][NUM_TRACKS_PER_SIDE] = {
	{    // NE
		{ TRACK_X,     1 << DIR_NE | 1 << DIR_SE | 1 << DIR_NW },
		{ TRACK_UPPER, 1 << DIR_E  | 1 << DIR_N  | 1 << DIR_S  },
		{ TRACK_RIGHT, 1 << DIR_N  | 1 << DIR_E  | 1 << DIR_W  },
	}, { // SE
		{ TRACK_Y,     1 << DIR_NE | 1 << DIR_SE | 1 << DIR_SW },
		{ TRACK_LOWER, 1 << DIR_E  | 1 << DIR_N  | 1 << DIR_S  },
		{ TRACK_RIGHT, 1 << DIR_S  | 1 << DIR_E  | 1 << DIR_W  },
	}, { // SW
		{ TRACK_X,     1 << DIR_SE | 1 << DIR_SW | 1 << DIR_NW },
		{ TRACK_LOWER, 1 << DIR_W  | 1 << DIR_N  | 1 << DIR_S  },
		{ TRACK_LEFT,  1 << DIR_S  | 1 << DIR_E  | 1 << DIR_W  },
	}, { // NW
		{ TRACK_Y,     1 << DIR_SW | 1 << DIR_NW | 1 << DIR_NE },
		{ TRACK_UPPER, 1 << DIR_W  | 1 << DIR_N  | 1 << DIR_S  },
		{ TRACK_LEFT,  1 << DIR_N  | 1 << DIR_E  | 1 << DIR_W  },
	},
};

/* Mask of positions at which pylons can be built per track. */
static const byte allowed_ppp[TRACK_END] = {
	1 << DIR_N  | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_S  | 1 << DIR_W  | 1 << DIR_NW, // X
	1 << DIR_N  | 1 << DIR_NE | 1 << DIR_E  | 1 << DIR_S  | 1 << DIR_SW | 1 << DIR_W,  // Y
	1 << DIR_N  | 1 << DIR_NE | 1 << DIR_SE | 1 << DIR_S  | 1 << DIR_SW | 1 << DIR_NW, // UPPER
	1 << DIR_N  | 1 << DIR_NE | 1 << DIR_SE | 1 << DIR_S  | 1 << DIR_SW | 1 << DIR_NW, // LOWER
	1 << DIR_NE | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_SW | 1 << DIR_W  | 1 << DIR_NW, // LEFT
	1 << DIR_NE | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_SW | 1 << DIR_W  | 1 << DIR_NW, // RIGHT
};

/**
 * Mask preferred and allowed pylon position points on a tile side.
 * @param tracks Tracks present on the tile.
 * @param wires Electrified tracks present on the tile.
 * @param side Tile side to check.
 * @param preferred Pointer to preferred positions to mask.
 * @param allowed Pointer to allowed positions to mask.
 * @return Whether the pylon control point is in use from this tile.
 */
static bool CheckCatenarySide (TrackBits tracks, TrackBits wires,
	DiagDirection side, byte *preferred, byte *allowed)
{
	bool pcp_in_use = false;
	byte pmask = 0xFF;
	byte amask = 0xFF;

	for (uint k = 0; k < NUM_TRACKS_PER_SIDE; k++) {
		/* We check whether the track in question is present. */
		const SideTrackData *data = &side_tracks[side][k];
		byte track = data->track;
		if (HasBit(wires, track)) {
			/* track found */
			pcp_in_use = true;
			pmask &= data->preferred;
		}
		if (HasBit(tracks, track)) {
			amask &= allowed_ppp[track];
		}
	}

	*preferred &= pmask;
	*allowed &= amask;
	return pcp_in_use;
}

/**
 * Mask preferred and allowed pylon position points on a tile side,
 * when there is a single track along an axis on the tile.
 * @param axis Axis of the track.
 * @param side Tile side to check.
 * @param preferred Pointer to preferred positions to mask.
 * @return Whether the pylon control point is in use from this tile.
 */
static inline bool CheckCatenarySideAxis (Axis axis, DiagDirection side,
	byte *preferred)
{
	/* We check whether the track in question is present. */
	if (DiagDirToAxis (side) != axis) return false;

	/* track found */
	*preferred &= side_tracks[side][0].preferred;
	return true;
}

/**
 * Check if the pylon on a tile side should be elided on long track runs.
 * @param side Tile side to check.
 * @param preferred Preferred pylon positions.
 * @param odd Array of tile coordinate parity per axis.
 * @param level Whether the land is level (for tracks running along an axis).
 * @return Whether the pylon should be elided.
 */
static bool CheckPylonElision (DiagDirection side, byte preferred,
	const bool *odd, bool level)
{
	Axis axis = DiagDirToAxis (side);
	bool ignore;
	switch (preferred) {
		case 1 << DIR_NW | 1 << DIR_SE:
			if (!level) return false;
			ignore = false; // must be X axis
			break;

		case 1 << DIR_NE | 1 << DIR_SW:
			if (!level) return false;
			ignore = true;  // must be Y axis
			break;

		case 1 << DIR_E | 1 << DIR_W:
			/* Non-orthogonal tracks must always be level. */
			ignore = (axis == AXIS_X) ? !odd[AXIS_Y] : odd[AXIS_X];
			break;

		case 1 << DIR_N | 1 << DIR_S:
			/* Non-orthogonal tracks must always be level. */
			ignore = !odd[OtherAxis(axis)];
			break;

		default:
			return false;
	}

	/* This configuration may be subject to pylon elision. */
	/* Toggle ignore if we are in an odd row, or heading the other way. */
	return (ignore ^ odd[axis] ^ HasBit(side, 1));
}

/** Possible return values for CheckNeighbourPCP below. */
enum {
	PCP_NB_NONE,      ///< PCP not in use from the neighbour tile
	PCP_NB_TUNNEL,    ///< PCP in use by a tunnel from the neighbour tile
	PCP_NB_TRY_ELIDE, ///< PCP is in use and may be subject to elision
};

/**
 * Check whether a pylon is also in use from a railway tile at the other side.
 * @param tile The neighbour railway tile to check.
 * @param side The tile side to check from this tile.
 * @param preferred Pointer to preferred positions to mask.
 * @param allowed Pointer to allowed positions to mask.
 * @param slope Pointer to store the tile slope if pylon elision is possible.
 * @return A value representing the PCP state at the given side.
 */
static uint CheckRailNeighbourPCP (TileIndex tile, DiagDirection side,
	byte *preferred, byte *allowed, Slope *slope)
{
	assert (IsRailwayTile (tile));

	bool is_bridge = IsTileSubtype (tile, TT_BRIDGE);
	if (is_bridge && GetTunnelBridgeDirection (tile) == side) {
		return PCP_NB_NONE;
	}

	TrackBits nb_tracks = GetElectrifiedTrackBits (tile);
	if (nb_tracks == TRACK_BIT_NONE) return PCP_NB_NONE;
	TrackBits nb_wires = MaskWireBits (tile, nb_tracks);

	/* Tracks inciding from the neighbour tile */
	if (!CheckCatenarySide (nb_tracks, nb_wires, side, preferred, allowed)) {
		return PCP_NB_NONE;
	}

	/* Read the foundations if they are present, and adjust the tileh */
	assert_compile (TRACK_BIT_X == 1);
	assert_compile (TRACK_BIT_Y == 2);

	Slope nb_slope;
	if (nb_tracks > 2) {
		/* Anything having more than a single X or Y track must be
		 * flat (or a half tile slope, but we treat those as flat). */
		nb_slope = SLOPE_FLAT;
	} else if (!is_bridge) {
		nb_slope = GetTileSlope (tile);
		Foundation f = GetRailFoundation (nb_slope, nb_tracks);
		ApplyFoundationToSlope (f, &nb_slope);
	} else {
		nb_slope = GetTileSlope (tile);
		/* With a single X or Y track, bridge must
		 * head away from our side. */
		nb_slope = HasBridgeFlatRamp (nb_slope, DiagDirToAxis (side)) ?
				SLOPE_FLAT :
				InclinedSlope (ReverseDiagDir (side));
	}

	*slope = nb_slope;
	return PCP_NB_TRY_ELIDE;
}

/**
 * Check whether a pylon is also in use from the other side.
 * @param tile The neighbour tile to check.
 * @param side The tile side to check from this tile.
 * @param preferred Pointer to preferred positions to mask.
 * @param allowed Pointer to allowed positions to mask.
 * @param slope Pointer to store the tile slope if pylon elision is possible.
 * @return A value representing the PCP state at the given side.
 */
static uint CheckNeighbourPCP (TileIndex tile, DiagDirection side,
	byte *preferred, byte *allowed, Slope *slope)
{
	Axis axis;
	switch (GetTileType (tile)) {
		case TT_RAILWAY:
			return CheckRailNeighbourPCP (tile, side,
						preferred, allowed, slope);

		case TT_MISC:
			switch (GetTileSubtype (tile)) {
				default: return PCP_NB_NONE;

				case TT_MISC_CROSSING:
					if (!HasCatenary (GetRailType (tile))) return PCP_NB_NONE;
					axis = GetCrossingRailAxis (tile);
					break;

				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType (tile) != TRANSPORT_RAIL) return PCP_NB_NONE;
					if (!HasCatenary (GetRailType (tile))) return PCP_NB_NONE;
					/* ignore tunnels facing the wrong way for neighbouring tiles */
					if (GetTunnelBridgeDirection (tile) != ReverseDiagDir (side)) return PCP_NB_NONE;
					/* force tunnels to always have a pylon (no elision) */
					*preferred = 0;
					return PCP_NB_TUNNEL;
			}
			break;

		case TT_STATION:
			if (!HasStationRail (tile)) return PCP_NB_NONE;
			if (!HasCatenary (GetRailType (tile))) return PCP_NB_NONE;
			/* Ignore neighbouring station tiles that allow neither wires nor pylons. */
			if (!CanStationTileHavePylons (tile) && !CanStationTileHaveWires (tile)) return PCP_NB_NONE;
			axis = GetRailStationAxis (tile);
			break;

		default:
			return PCP_NB_NONE;
	}

	/* Crossing or station tile, so just one flat track along an axis. */
	if (!CheckCatenarySideAxis (axis, side, preferred)) {
		return PCP_NB_NONE;
	}

	*slope = SLOPE_FLAT;
	return PCP_NB_TRY_ELIDE;
}

/** Possible return values for CheckSidePCP below. */
enum {
	PCP_NONE,        ///< PCP is not in use
	PCP_IN_USE,      ///< PCP is in use from this tile
	PCP_IN_USE_BOTH, ///< PCP is in use also from the neighbour tile
};

/**
 * Check whether there should be a pylon at a tile side.
 * @param tile The tile to check.
 * @param home_tracks Tracks present on the home tile.
 * @param home_wires Electrified tracks present on the home tile.
 * @param home_slope Slope of the home tile, adjusted for foundations.
 * @param side The side to check.
 * @param odd Array of tile coordinate parity per axis.
 * @return A value representing the PCP state at the given side, plus
 *  a bitmask of allowed directions for the pylon, if any.
 */
static std::pair <uint, byte> CheckSidePCP (TileIndex tile,
	TrackBits home_tracks, TrackBits home_wires, Slope home_slope,
	DiagDirection side, const bool *odd)
{
	/* We cycle through all the existing tracks at a PCP and see what
	 * PPPs we want to have, or may not have at all */
	byte PPPpreferred = 0xFF; // We start with preferring everything (end-of-line in any direction)
	byte PPPallowed = AllowedPPPonPCP[side];

	/* Tracks inciding from the home tile */
	if (!CheckCatenarySide (home_tracks, home_wires, side, &PPPpreferred, &PPPallowed)) {
		/* PCP not used at all from this tile. */
		return std::make_pair (PCP_NONE, 0);
	}

	bool pcp_neighbour;
	Slope nb_slope;
	switch (CheckNeighbourPCP (tile + TileOffsByDiagDir (side),
				ReverseDiagDir (side),
				&PPPpreferred, &PPPallowed, &nb_slope)) {
		default: // PCP_NB_TRY_ELIDE
			if (CheckPylonElision (side, PPPpreferred, odd, home_slope == nb_slope)) {
				return std::make_pair (PCP_NONE, 0);
			}
			/* fall through */
		case PCP_NB_TUNNEL:
			pcp_neighbour = true;
			break;

		case PCP_NB_NONE:
			pcp_neighbour = false;
			break;
	}

	/* Now decide where we draw our pylons. First try the preferred PPPs,
	 * but they may not exist. In that case, we try the any of the allowed
	 * ones. if they don't exist either, don't draw anything. Note that
	 * the preferred PPPs still contain the end-of-line markers. Remove
	 * those (simply by ANDing with allowed, since these markers are never
	 * allowed) */
	if (PPPallowed == 0) return std::make_pair (PCP_NONE, 0);

	if ((PPPallowed & PPPpreferred) != 0) PPPallowed &= PPPpreferred;
	return std::make_pair (pcp_neighbour ? PCP_IN_USE_BOTH : PCP_IN_USE, PPPallowed);
}

/**
 * Choose the pylon position point to use for a pylon.
 * @param side Tile side where the pylon will be drawn.
 * @param allowed Mask of allowed pylon position points.
 * @param order Possible pylon positions arranged by preference.
 * @param nb Whether there is a neighbour tile that could draw this pylon.
 * @return The pylon position point to use, or -1 for none.
 * @note Use the overloaded variant below.
 */
static int ChoosePylonPosition (DiagDirection side, byte allowed,
	const Direction *order, bool nb)
{
	/* Which of the PPPs are inside the tile. For the two PPPs on the tile
	 * border the following system is used: if you rotate the PCP so that
	 * it is in the north, the eastern PPP belongs to the tile. */
	static const byte owned[DIAGDIR_END] = {
		1 << DIR_SE | 1 << DIR_S  | 1 << DIR_SW | 1 << DIR_W,
		1 << DIR_N  | 1 << DIR_SW | 1 << DIR_W  | 1 << DIR_NW,
		1 << DIR_N  | 1 << DIR_NE | 1 << DIR_E  | 1 << DIR_NW,
		1 << DIR_NE | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_S,
	};

	assert (allowed != 0);

	for (Direction k = DIR_BEGIN; k < DIR_END; k++) {
		byte pos = order[k];

		if (!HasBit(allowed, pos)) continue;

		/* Don't build the pylon if it would be outside the tile */
		if (HasBit(owned[side], pos)) return pos;

		/* We have a neighbour that will draw it, bail out */
		if (nb) return -1;
	}

	NOT_REACHED();
}

/**
 * Choose the pylon position point to use for a pylon.
 * @param side Tile side where the pylon will be drawn.
 * @param allowed Mask of allowed pylon position points.
 * @param odd_x Whether the tile is on an odd X coordinate.
 * @param odd_y Whether the tile is on an odd Y coordinate.
 * @param nb Whether there is a neighbour tile that could draw this pylon.
 * @return The pylon position point to use.
 */
static inline int ChoosePylonPosition (DiagDirection side, byte allowed,
	bool odd_x, bool odd_y, bool nb)
{
	/* Several PPPs maybe exist, here they are sorted in order of preference. */
	static const Direction order[2][2][DIAGDIR_END][DIR_END] = {
		{    // X even
			{    // Y even
				{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_N, DIR_E, DIR_S, DIR_W}, // NE
				{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_S, DIR_E, DIR_N, DIR_W}, // SE
				{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_S, DIR_W, DIR_N, DIR_E}, // SW
				{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_N, DIR_W, DIR_S, DIR_E}, // NW
			}, { // Y odd
				{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_S, DIR_W, DIR_N, DIR_E}, // NE
				{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_N, DIR_W, DIR_S, DIR_E}, // SE
				{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_N, DIR_E, DIR_S, DIR_W}, // SW
				{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_S, DIR_E, DIR_N, DIR_W}, // NW
			}
		}, { // X odd
			{    // Y even
				{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_S, DIR_W, DIR_N, DIR_E}, // NE
				{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_N, DIR_W, DIR_S, DIR_E}, // SE
				{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_N, DIR_E, DIR_S, DIR_W}, // SW
				{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_S, DIR_E, DIR_N, DIR_W}, // NW
			}, { // Y odd
				{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_N, DIR_E, DIR_S, DIR_W}, // NE
				{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_S, DIR_E, DIR_N, DIR_W}, // SE
				{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_S, DIR_W, DIR_N, DIR_E}, // SW
				{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_N, DIR_W, DIR_S, DIR_E}, // NW
			}
		}
	};

	return ChoosePylonPosition (side, allowed, order[odd_x][odd_y][side], nb);
}

/*
 * Add a pylon sprite for a tile.
 * @param ti The TileInfo struct of the tile being drawn.
 * @param pylon The sprite to draw.
 * @param x X position of the sprite.
 * @param y Y position of the sprite.
 * @param z Z position of the sprite.
 */
static void AddPylonSprite (const TileInfo *ti, SpriteID pylon,
	int x, int y, int z)
{
	AddSortableSpriteToDraw (ti->vd, pylon, PAL_NONE, x, y, 1, 1,
			BB_HEIGHT_UNDER_BRIDGE, z,
			IsTransparencySet (TO_CATENARY), -1, -1);
}

/**
 * Draw a pylon at a tile side.
 * @param ti The TileInfo struct of the tile being drawn.
 * @param side Side where to draw the pylon.
 * @param dir Pylon position point.
 * @param pylon_base Pylon sprite base.
 */
static void DrawPylon (const TileInfo *ti, DiagDirection side, Direction dir,
	SpriteID pylon_base)
{
	uint x = ti->x + x_pcp_offsets[side] + x_ppp_offsets[dir];
	uint y = ti->y + y_pcp_offsets[side] + y_ppp_offsets[dir];

	/* The elevation of the "pylon"-sprite should be the elevation
	 * at the PCP. PCPs are always on a tile edge.
	 *
	 * This position can be outside of the tile, i.e.
	 * ?_pcp_offset == TILE_SIZE > TILE_SIZE - 1.
	 * So we have to move it inside the tile, because if the neighboured
	 * tile has a foundation, that does not smoothly connect to the
	 * current tile, we will get a wrong elevation from GetSlopePixelZ().
	 *
	 * When we move the position inside the tile, we will get a wrong
	 * elevation if we have a slope. To catch all cases we round the Z
	 * position to the next (TILE_HEIGHT / 2). This will return the
	 * correct elevation for slopes and will also detect non-continuous
	 * elevation on edges.
	 *
	 * Also note that the result of GetSlopePixelZ() is very special on
	 * bridge-ramps.
	 */

	TileIndex tile = ti->tile;
	int z = GetSlopePixelZ (TileX(tile) * TILE_SIZE + min(x_pcp_offsets[side], TILE_SIZE - 1), TileY(tile) * TILE_SIZE + min(y_pcp_offsets[side], TILE_SIZE - 1));
	int elevation = (z + 2) & ~3; // this means z = (z + TILE_HEIGHT / 4) / (TILE_HEIGHT / 2) * (TILE_HEIGHT / 2);

	AddPylonSprite (ti, pylon_base + pylon_sprites[dir], x, y, elevation);
}

/**
 * Draws overhead wires and pylons for electric railways.
 * @param ti The TileInfo struct of the tile being drawn
 */
void DrawCatenary (const TileInfo *ti)
{
	/* Pylons are placed on a tile edge, so we need to take into account
	 * the track configuration of 2 adjacent tiles. home stores the
	 * current tile */
	TrackBits home_tracks, home_wires;
	/* Note that ti->tileh has already been adjusted for Foundations */
	Slope home_slope = ti->tileh;

	bool odd[AXIS_END];
	odd[AXIS_X] = IsOddX(ti->tile);
	odd[AXIS_Y] = IsOddY(ti->tile);
	byte PCPstatus = 0;
	DiagDirection overridePCP = INVALID_DIAGDIR;

	/* Find which rail bits are present, and select the override points.
	 * We don't draw a pylon:
	 * 1) INSIDE a tunnel (we wouldn't see it anyway)
	 * 2) on the "far" end of a bridge head (the one that connects to bridge middle),
	 *    because that one is drawn on the bridge. Exception is for length 0 bridges
	 *    which have no middle tiles */
	home_tracks = GetRailTrackBitsUniversal (ti->tile, INVALID_DIAGDIR, &overridePCP);
	home_wires = MaskWireBits (ti->tile, home_tracks);

	/* Half tile slopes coincide only with horizontal/vertical track.
	 * Faking a flat slope results in the correct sprites on positions. */
	Track halftile_track;
	TileContext halftile_context;
	if (IsHalftileSlope (home_slope)) {
		switch (GetHalftileSlopeCorner (home_slope)) {
			default: NOT_REACHED();
			case CORNER_W: halftile_track = TRACK_LEFT;  break;
			case CORNER_S: halftile_track = TRACK_LOWER; break;
			case CORNER_E: halftile_track = TRACK_RIGHT; break;
			case CORNER_N: halftile_track = TRACK_UPPER; break;
		}
		halftile_context = TCX_UPPER_HALFTILE;
		home_slope = SLOPE_FLAT;
	} else {
		switch (home_tracks) {
			case TRACK_BIT_LOWER:
			case TRACK_BIT_HORZ:
				halftile_track = GetRailType(ti->tile, TRACK_UPPER) == GetRailType(ti->tile, TRACK_LOWER) ? INVALID_TRACK : TRACK_LOWER;
				break;
			case TRACK_BIT_RIGHT:
			case TRACK_BIT_VERT:
				halftile_track = GetRailType(ti->tile, TRACK_LEFT) == GetRailType(ti->tile, TRACK_RIGHT) ? INVALID_TRACK : TRACK_RIGHT;
				break;
			default:
				halftile_track = INVALID_TRACK;
				break;
		}
		halftile_context = TCX_NORMAL;
	}

	if (IsRailBridgeTile (ti->tile) && !IsExtendedRailBridge (ti->tile)) {
		if (home_slope != SLOPE_FLAT) {
			home_slope = SLOPE_FLAT;
		} else {
			home_slope = InclinedSlope (GetTunnelBridgeDirection (ti->tile));
		}
	}

	SpriteID sprite_normal, sprite_halftile;

	if (halftile_track == INVALID_TRACK) {
		sprite_normal = GetPylonBase(ti->tile, TCX_NORMAL);
	} else {
		sprite_halftile = GetPylonBase(ti->tile, halftile_context, halftile_track);
		sprite_normal = GetPylonBase(ti->tile, TCX_NORMAL,
			HasBit(home_tracks, TrackToOppositeTrack(halftile_track)) ? TrackToOppositeTrack(halftile_track) : halftile_track);
	}

	for (DiagDirection i = DIAGDIR_BEGIN; i < DIAGDIR_END; i++) {
		static const TrackBits edge_tracks[] = {
			TRACK_BIT_UPPER | TRACK_BIT_RIGHT, // DIAGDIR_NE
			TRACK_BIT_LOWER | TRACK_BIT_RIGHT, // DIAGDIR_SE
			TRACK_BIT_LOWER | TRACK_BIT_LEFT,  // DIAGDIR_SW
			TRACK_BIT_UPPER | TRACK_BIT_LEFT,  // DIAGDIR_NW
		};
		SpriteID pylon_base = (halftile_track != INVALID_TRACK && HasBit(edge_tracks[i], halftile_track)) ? sprite_halftile : sprite_normal;

		bool pcp_neighbour;
		byte PPPallowed;
		if (overridePCP != i) {
			std::pair <uint, byte> pcp_state = CheckSidePCP (ti->tile,
					home_tracks, home_wires, home_slope,
					i, odd);
			if (pcp_state.first == PCP_NONE) continue;
			pcp_neighbour = (pcp_state.first == PCP_IN_USE_BOTH);
			PPPallowed = pcp_state.second;
			SetBit(PCPstatus, i);
		} else {
			/* Bridge tile. */
			TrackBits bridge_tracks = DiagdirReachesTracks (ReverseDiagDir (i));
			if ((home_tracks & bridge_tracks) == TRACK_BIT_NONE) continue;
			SetBit(PCPstatus, i);
			/* Pylon is drawn by the middle part if there is any. */
			if (GetTunnelBridgeLength (ti->tile, GetOtherBridgeEnd (ti->tile)) > 0) continue;
			pcp_neighbour = true;
			PPPallowed = AllowedPPPonPCP[i];
		}

		if (IsRailStationTile(ti->tile) && !CanStationTileHavePylons(ti->tile)) continue;

		if (HasBridgeAbove(ti->tile)) {
			if (GetBridgeAxis (ti->tile) == DiagDirToAxis (i)) {
				int height = GetBridgeHeight (GetNorthernBridgeEnd (ti->tile));
				if (height <= GetTileMaxZ (ti->tile) + 1) {
					continue;
				}
			}
		}

		int pos = ChoosePylonPosition (i, PPPallowed, odd[AXIS_X], odd[AXIS_Y], pcp_neighbour);
		if (pos >= 0) {
			DrawPylon (ti, i, (Direction)pos, pylon_base);
		}
	}

	/* Don't draw a wire under a low bridge */
	if (HasBridgeAbove(ti->tile) && !IsTransparencySet(TO_BRIDGES)) {
		int height = GetBridgeHeight(GetNorthernBridgeEnd(ti->tile));

		if (height <= GetTileMaxZ(ti->tile) + 1) return;
	}

	/* Don't draw a wire if the station tile does not want any */
	if (IsRailStationTile(ti->tile) && !CanStationTileHaveWires(ti->tile)) return;

	if (halftile_track == INVALID_TRACK) {
		sprite_normal = GetWireBase(ti->tile, TCX_NORMAL);
	} else {
		sprite_halftile = GetWireBase(ti->tile, halftile_context, halftile_track);
		sprite_normal = GetWireBase(ti->tile, TCX_NORMAL,
			HasBit(home_tracks, TrackToOppositeTrack(halftile_track)) ? TrackToOppositeTrack(halftile_track) : halftile_track);
	}

	/* Drawing of pylons is finished, now draw the wires */
	Track t;
	FOR_EACH_SET_TRACK(t, home_wires) {
		byte PCPconfig = HasBit(PCPstatus, PCPpositions[t][0]) +
			(HasBit(PCPstatus, PCPpositions[t][1]) << 1);

		assert(PCPconfig != 0); // We have a pylon on neither end of the wire, that doesn't work (since we have no sprites for that)
		assert(!IsSteepSlope(home_slope));

		const SortableSpriteStructM *sss;
		switch (home_slope) {
			case SLOPE_SW: sss = &CatenarySpriteDataSW;  break;
			case SLOPE_SE: sss = &CatenarySpriteDataSE;  break;
			case SLOPE_NW: sss = &CatenarySpriteDataNW;  break;
			case SLOPE_NE: sss = &CatenarySpriteDataNE;  break;
			default:       sss = &CatenarySpriteData[t]; break;
		}

		/*
		 * The "wire"-sprite position is inside the tile, i.e. 0 <= sss->?_offset < TILE_SIZE.
		 * Therefore it is safe to use GetSlopePixelZ() for the elevation.
		 * Also note that the result of GetSlopePixelZ() is very special for bridge-ramps.
		 */
		SpriteID wire_base = (t == halftile_track) ? sprite_halftile : sprite_normal;
		AddSortableSpriteToDraw (ti->vd, wire_base + sss->image_offset[PCPconfig], PAL_NONE, ti->x + sss->x_offset, ti->y + sss->y_offset,
			sss->x_size, sss->y_size, sss->z_size, GetSlopePixelZ(ti->x + sss->x_offset, ti->y + sss->y_offset) + sss->z_offset,
			IsTransparencySet(TO_CATENARY));
	}
}

/**
 * Draws overhead wires and pylons at a tunnel entrance.
 * @param ti The TileInfo struct of the tile being drawn.
 * @param dir The direction of the tunnel.
 */
void DrawRailTunnelCatenary (const TileInfo *ti, DiagDirection dir)
{
	/* Draw pylon. */
	TileIndex tile = ti->tile;
	DiagDirection rev = ReverseDiagDir (dir);

	byte dummy_preferred, dummy_allowed;
	Slope dummy_slope;
	bool pcp_neighbour = CheckNeighbourPCP (tile + TileOffsByDiagDir (rev),
				dir, &dummy_preferred, &dummy_allowed, &dummy_slope);

	int pos = ChoosePylonPosition (rev, AllowedPPPonPCP[rev],
			IsOddX(tile), IsOddY(tile), pcp_neighbour);

	if (pos >= 0) {
		DrawPylon (ti, rev, (Direction)pos,
				GetPylonBase (tile, TCX_NORMAL));
	}

	/* Draw wire. */
	StartSpriteCombine (ti->vd);
	DrawRailTunnelDepotCatenary (ti, false, dir);
}

/**
 * Draws wires on a tunnel tile
 *
 * DrawTile_TunnelBridge() calls this function to draw the wires on the bridge.
 *
 * @param ti The Tileinfo to draw the tile for
 */
void DrawCatenaryOnBridge(const TileInfo *ti)
{
	TileIndex start = GetNorthernBridgeEnd(ti->tile);
	TileIndex end = GetSouthernBridgeEnd(ti->tile);

	uint length = GetTunnelBridgeLength(start, end);
	uint num = GetTunnelBridgeLength(ti->tile, start) + 1;

	Axis axis = GetBridgeAxis(ti->tile);

	const SortableSpriteStructM *sss = &CatenarySpriteData[AxisToTrack(axis)];

	uint config;
	if ((length % 2) && num == length) {
		/* Draw the "short" wire on the southern end of the bridge
		 * only needed if the length of the bridge is odd */
		config = 3;
	} else {
		/* Draw "long" wires on all other tiles of the bridge (one pylon every two tiles) */
		config = 2 - (num % 2);
	}

	uint height = GetBridgePixelHeight(end);

	SpriteID wire_base = GetWireBase(end, TCX_ON_BRIDGE);

	AddSortableSpriteToDraw (ti->vd, wire_base + sss->image_offset[config], PAL_NONE, ti->x + sss->x_offset, ti->y + sss->y_offset,
		sss->x_size, sss->y_size, sss->z_size, height + sss->z_offset,
		IsTransparencySet(TO_CATENARY)
	);

	/* Finished with wires, draw pylons */
	if ((num % 2) == 0 && num != length) return; /* no pylons to draw */

	DiagDirection PCPpos;
	Direction PPPpos;
	if (axis == AXIS_X) {
		PCPpos = DIAGDIR_NE;
		PPPpos = IsOddY(ti->tile) ? DIR_SE : DIR_NW;
	} else {
		PCPpos = DIAGDIR_NW;
		PPPpos = IsOddX(ti->tile) ? DIR_SW : DIR_NE;
	}

	SpriteID pylon = GetPylonBase(end, TCX_ON_BRIDGE) + pylon_sprites[PPPpos];
	uint x = ti->x + x_ppp_offsets[PPPpos];
	uint y = ti->y + y_ppp_offsets[PPPpos];

	/* every other tile needs a pylon on the northern end */
	if (num % 2) {
		AddPylonSprite (ti, pylon, x + x_pcp_offsets[PCPpos],
				y + y_pcp_offsets[PCPpos], height);
	}

	/* need a pylon on the southern end of the bridge */
	if (num == length) {
		PCPpos = ReverseDiagDir(PCPpos);
		AddPylonSprite (ti, pylon, x + x_pcp_offsets[PCPpos],
				y + y_pcp_offsets[PCPpos], height);
	}
}

bool SettingsDisableElrail(int32 p1)
{
	Company *c;
	Train *t;
	bool disable = (p1 != 0);

	/* we will now walk through all electric train engines and change their railtypes if it is the wrong one*/
	const RailType old_railtype = disable ? RAILTYPE_ELECTRIC : RAILTYPE_RAIL;
	const RailType new_railtype = disable ? RAILTYPE_RAIL : RAILTYPE_ELECTRIC;

	/* walk through all train engines */
	Engine *e;
	FOR_ALL_ENGINES_OF_TYPE(e, VEH_TRAIN) {
		RailVehicleInfo *rv_info = &e->u.rail;
		/* if it is an electric rail engine and its railtype is the wrong one */
		if (rv_info->engclass == 2 && rv_info->railtype == old_railtype) {
			/* change it to the proper one */
			rv_info->railtype = new_railtype;
		}
	}

	/* when disabling elrails, make sure that all existing trains can run on
	 *  normal rail too */
	if (disable) {
		FOR_ALL_TRAINS(t) {
			if (t->railtype == RAILTYPE_ELECTRIC) {
				/* this railroad vehicle is now compatible only with elrail,
				 *  so add there also normal rail compatibility */
				t->compatible_railtypes |= RAILTYPES_RAIL;
				t->railtype = RAILTYPE_RAIL;
				SetBit(t->flags, VRF_EL_ENGINE_ALLOWED_NORMAL_RAIL);
			}
		}
	}

	/* Fix the total power and acceleration for trains */
	FOR_ALL_TRAINS(t) {
		/* power and acceleration is cached only for front engines */
		if (t->IsFrontEngine()) {
			t->ConsistChanged(CCF_TRACK);
		}
	}

	FOR_ALL_COMPANIES(c) c->avail_railtypes = GetCompanyRailtypes(c->index);

	/* This resets the _last_built_railtype, which will be invalid for electric
	 * rails. It may have unintended consequences if that function is ever
	 * extended, though. */
	ReinitGuiAfterToggleElrail(disable);
	return true;
}
