/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/rail.h Tile functions for railway tiles. */

#ifndef TILE_RAIL_H
#define TILE_RAIL_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "common.h"
#include "misc.h"
#include "signal.h"
#include "../direction_type.h"
#include "../direction_func.h"
#include "../track_type.h"
#include "../track_func.h"
#include "../rail_type.h"
#include "../company_type.h"

/**
 * Get the rail track bits of a tile
 * @param t The tile whose rail track bits to get
 * @pre tile_is_railway(t)
 * @return The rail track bits of the tile
 */
static inline TrackBits tile_get_trackbits(const Tile *t)
{
	assert(tile_is_railway(t));
	return (TrackBits)GB(t->m2, 0, 6);
}

/**
 * Set the rail track bits of a tile
 * @param t The tile whose rail track bits to set
 * @param trackbits The track bits to set
 * @pre tile_is_railway(t)
 */
static inline void tile_set_trackbits(Tile *t, TrackBits trackbits)
{
	assert(tile_is_railway(t));
	SB(t->m2, 0, 6, trackbits);
}

/**
 * Check if a railway tile has a given track
 * @param t The tile to check
 * @param track The track to check for
 * @pre tile_is_railway(t)
 * @return Whether the railway tile has the given track
 */
static inline bool tile_has_track(const Tile *t, Track track)
{
	return HasBit(tile_get_trackbits(t), track);
}


/**
 * Get the rail type of a track
 * @param t The tile whose track to get the rail of
 * @param track The track whose rail type to get, if applicable
 * @return The rail type of the given track
 */
static inline RailType tile_get_rail_type(const Tile *t, Track track = INVALID_TRACK)
{
	assert(tile_is_railway(t) || tile_is_crossing(t) || tile_is_rail_tunnel(t) || tile_is_rail_depot(t) || tile_is_station(t));

	if (tile_is_railway(t) && (track == TRACK_LOWER || track == TRACK_RIGHT)) {
		return (RailType)GB(t->m5, 0, 4);
	} else {
		return (RailType)GB(t->m3, 0, 4);
	}
}

/**
 * Set the rail type of a track, or of a whole tile
 * @param t The tile whose rail type to set
 * @param rt The rail type to set
 * @param track The track whose rail type to set, if applicable
 */
static inline void tile_set_rail_type(Tile *t, RailType rt, Track track = INVALID_TRACK)
{
	if (!tile_is_railway(t)) {
		assert(tile_is_crossing(t) || tile_is_rail_tunnel(t) || tile_is_rail_depot(t) || tile_is_station(t));
		assert(track == INVALID_TRACK);
		SB(t->m3, 0, 4, rt);
	} else {
		switch (track) {
			case INVALID_TRACK:
				SB(t->m3, 0, 4, rt);
				SB(t->m5, 0, 4, rt);
				break;
			case TRACK_LOWER:
			case TRACK_RIGHT:
				SB(t->m5, 0, 4, rt);
				break;
			default:
				SB(t->m3, 0, 4, rt);
				break;
		}
	}
}

/**
 * Get the rail type of the track that incides on a given tile side
 * @param t The track whose rail type to get
 * @param side The side whose track to get the rail type of
 * @pre tile_is_railway(t)
 * @return The rail type of the track that incides on the given side, or INVALID_RAILTYPE if there is none
 */
static inline RailType tile_get_side_rail_type(const Tile *t, DiagDirection side)
{
	TrackBits trackbits = tile_get_trackbits(t) & DiagdirReachesTracks(ReverseDiagDir(side));
	if (trackbits == TRACK_BIT_NONE) return INVALID_RAILTYPE;
	return tile_get_rail_type(t, FindFirstTrack(trackbits));
}

/**
 * Get the rail type of the track that heads into a bridge
 * @param t The tile whose bridge rail type to get
 * @pre tile_is_rail_bridge(t)
 * @return The rail type of the bridge
 */
static inline RailType tile_get_bridge_rail_type(const Tile *t)
{
	assert(tile_is_rail_bridge(t));
	return tile_get_side_rail_type(t, tile_get_tunnelbridge_direction(t));
}


/**
 * Get the rail reservation track bits for a tile
 * @param t The tile whose reservation track bits to get
 * @pre tile_is_railway(t)
 * @return The reservation track bits of the tile
 */
static inline TrackBits tile_get_reservation_trackbits(const Tile *t)
{
	assert(tile_is_railway(t));
	byte track_b = GB(t->m2, 8, 3);
	if (track_b == 0) return TRACK_BIT_NONE;
	Track track = (Track)(track_b - 1);    // map array saves Track+1
	return (TrackBits)(TrackToTrackBits(track) | (HasBit(t->m2, 11) ? TrackToTrackBits(TrackToOppositeTrack(track)) : 0));
}

/**
 * Set the rail reservation track bits for a tile
 * @param t The tile whose reservation track bits to set
 * @param b The reserved track bits
 * @pre tile_is_railway(t)
 */
static inline void tile_set_reservation_trackbits(Tile *t, TrackBits b)
{
	assert(tile_is_railway(t));
	assert(b != INVALID_TRACK_BIT);
	assert(!TracksOverlap(b));
	Track track = RemoveFirstTrack(&b);
	SB(t->m2, 8, 3, track == INVALID_TRACK ? 0 : track + 1);
	SB(t->m2, 11, 1, (byte)(b != TRACK_BIT_NONE));
}


/**
 * Get the signal byte for a signal
 *
 * A railway tile can have up to two signal pairs. The first one is stored
 * in m4, the second one is stored in m7. This function gets a pointer to
 * the right byte in a tile for a given track.
 *
 * @param t The tile whose signal byte to get
 * @param track The track whose signal byte to get
 * @pre tile_is_railway(t)
 */
static inline SignalPair *tile_signalpair(Tile *t, Track track)
{
	assert(track < TRACK_END); // do not use this for INVALID_TRACK
	return (track == TRACK_LOWER || track == TRACK_RIGHT) ? &t->m7 : &t->m4;
}

static inline const SignalPair *tile_signalpair(const Tile *t, Track track)
{
	assert(track < TRACK_END); // do not use this for INVALID_TRACK
	return (track == TRACK_LOWER || track == TRACK_RIGHT) ? &t->m7 : &t->m4;
}

/**
 * Check if a trackdir is the along trackdir when encoding signals for its track
 *
 * A track can have signals on either or both of its trackdirs. In its
 * encoding byte, one of the trackdirs is encoded as 'along', and the
 * other is encoded as 'against'. This function determines if a given
 * trackdir is the along or against trackdir for its track.
 *
 * @param trackdir The trackdir to check
 * @return Whether signals for this trackdir are encoded in the along bit
 */
static inline bool trackdir_is_signal_along(Trackdir trackdir)
{
	/* return 1 for trackdirs 0 1 2 3 12 13 */
	/* return 0 for trackdirs 4 5 8 9 10 11 */
	return HasBit(trackdir + 0xC, 3);
}


/**
 * Clear signals on a track
 * @param t The tile to clear signals from
 * @param track The track to clear signals from
 */
static inline void tile_clear_signals(Tile *t, Track track)
{
	signalpair_clear(tile_signalpair(t, track));
}

/**
 * Get present signals on a track
 * @param t The tile whose present signals to get
 * @param track The track whose present signals to get
 * @return A bitmask of present signals (bit 0 is against, bit 1 is along)
 */
static inline uint tile_get_present_signals(const Tile *t, Track track)
{
	return signalpair_get_present(tile_signalpair(t, track));
}

/**
 * Set present signals on a track
 * @param t The tile whose present signals to set
 * @param track The track whose present signals to set
 * @param mask A bitmask of present signals (bit 0 is against, bit 1 is along)
 */
static inline void tile_set_present_signals(Tile *t, Track track, uint mask)
{
	signalpair_set_present(tile_signalpair(t, track), mask);
}

/**
 * Check if a track has signals at all
 * @param t The tile to check
 * @param track The track to check
 * @return Whether the track has any signals
 */
static inline bool tile_has_track_signals(const Tile *t, Track track)
{
	return signalpair_has_signals(tile_signalpair(t, track));
}

/**
 * Check if a track has a signal on a trackdir
 * @param t The tile to check
 * @param trackdir The trackdir to check
 * @return Whether there is a signal on the given trackdir
 */
static inline bool tile_has_trackdir_signal(const Tile *t, Trackdir trackdir)
{
	assert (IsValidTrackdir(trackdir));
	return signalpair_has_signal(tile_signalpair(t, TrackdirToTrack(trackdir)),
		trackdir_is_signal_along(trackdir));
}


/**
 * Get signal states on a track
 * @param t The tile to check
 * @param track The track to check
 * @return A bitmask of signal states (bit 0 is against, bit 1 is along)
 */
static inline uint tile_get_signal_states(const Tile *t, Track track)
{
	return signalpair_get_states(tile_signalpair(t, track));
}

/**
 * Set signal states on a track
 * @param t The tile to set
 * @param track The track to set
 * @param mask A bitmask of signal states (bit 0 is against, bit 1 is along)
 */
static inline void tile_set_signal_states(Tile *t, Track track, uint mask)
{
	signalpair_set_states(tile_signalpair(t, track), mask);
}

/**
 * Get the signal state on a trackdir
 * @param t The tile to check
 * @param trackdir The trackdir to check
 * @return The signal state on the given trackdir
 */
static inline SignalState tile_get_signal_state(const Tile *t, Trackdir trackdir)
{
	assert(IsValidTrackdir(trackdir));
	return signalpair_get_state(tile_signalpair(t, TrackdirToTrack(trackdir)),
		trackdir_is_signal_along(trackdir));
}

/**
 * Set the signal state on a trackdir
 * @param t The tile to set
 * @param trackdir The trackdir to set
 * @param state The state to set
 */
static inline void tile_set_signal_state(Tile *t, Trackdir trackdir, SignalState state)
{
	assert(IsValidTrackdir(trackdir));
	signalpair_set_state(tile_signalpair(t, TrackdirToTrack(trackdir)),
		trackdir_is_signal_along(trackdir), state);
}


/**
 * Get the type of the signals on a track
 * @param t The tile whose signals to get the type of
 * @param track The track whose signals to get the type of
 * @return The type of the signals on the track
 */
static inline SignalType tile_get_signal_type(const Tile *t, Track track)
{
	return signalpair_get_type(tile_signalpair(t, track));
}

/**
 * Set the type of the signals on a track
 * @param t The tile whose signals to set the type of
 * @param track The track whose signals to set the type of
 * @param type The type of signals to set
 */
static inline void tile_set_signal_type(Tile *t, Track track, SignalType type)
{
	signalpair_set_type(tile_signalpair(t, track), type);
}

/**
 * Get the variant of the signals on a track
 * @param t The tile whose signals to get the variant of
 * @param track The track whose signals to get the variant of
 * @return The variant of the signals on the track
 */
static inline SignalVariant tile_get_signal_variant(const Tile *t, Track track)
{
	return signalpair_get_variant(tile_signalpair(t, track));
}

/**
 * Set the variant of the signals on a track
 * @param t The tile whose signals to set the variant of
 * @param track The track whose signals to set the variant of
 * @param v The variant of signals to set
 */
static inline void tile_set_signal_variant(Tile *t, Track track, SignalVariant v)
{
	signalpair_set_variant(tile_signalpair(t, track), v);
}


/** The ground 'under' the rail */
enum RailGroundType {
	RAIL_GROUND_BARREN       =  0, ///< Nothing (dirt)
	RAIL_GROUND_GRASS        =  1, ///< Grassy
	RAIL_GROUND_FENCE_NW     =  2, ///< Grass with a fence at the NW edge
	RAIL_GROUND_FENCE_SE     =  3, ///< Grass with a fence at the SE edge
	RAIL_GROUND_FENCE_SENW   =  4, ///< Grass with a fence at the NW and SE edges
	RAIL_GROUND_FENCE_NE     =  5, ///< Grass with a fence at the NE edge
	RAIL_GROUND_FENCE_SW     =  6, ///< Grass with a fence at the SW edge
	RAIL_GROUND_FENCE_NESW   =  7, ///< Grass with a fence at the NE and SW edges
	RAIL_GROUND_FENCE_VERT1  =  8, ///< Grass with a fence at the eastern side
	RAIL_GROUND_FENCE_VERT2  =  9, ///< Grass with a fence at the western side
	RAIL_GROUND_FENCE_HORIZ1 = 10, ///< Grass with a fence at the southern side
	RAIL_GROUND_FENCE_HORIZ2 = 11, ///< Grass with a fence at the northern side
	RAIL_GROUND_ICE_DESERT   = 12, ///< Icy or sandy
	RAIL_GROUND_WATER        = 13, ///< Grass with a fence and shore or water on the free halftile
	RAIL_GROUND_HALF_SNOW    = 14, ///< Snow only on higher part of slope (steep or one corner raised)
};

/**
 * Get the ground type of a railway tile
 * @param t The tile whose ground type to get
 * @pre tile_is_rail_track(t)
 * @return The ground type of the tile
 */
static inline RailGroundType tile_get_rail_ground(const Tile *t)
{
	assert(tile_is_rail_track(t));
	return (RailGroundType)GB(t->m3, 4, 4);
}

/**
 * Set the ground type of a railway tile
 * @param t The tile whose ground type to set
 * @param g The ground type to set
 * @pre tile_is_rail_track(t)
 */
static inline void tile_set_rail_ground(Tile *t, RailGroundType rgt)
{
	assert(tile_is_rail_track(t));
	SB(t->m3, 4, 4, rgt);
}


/**
 * Get the bridge type of a rail bridge
 * @param t The tile whose bridge type to get
 * @pre tile_is_rail_bridge(t)
 * @return The bridge type of the tile
 */
static inline uint tile_get_rail_bridge_type(const Tile *t)
{
	assert(tile_is_rail_bridge(t));
	return GB(t->m2, 12, 4);
}

/**
 * Set the bridge type of a rail bridge
 * @param t The tile whose bridge type to set
 * @param type The bridge type to set
 * @pre tile_is_rail_bridge(t)
 */
static inline void tile_set_rail_bridge_type(Tile *t, uint type)
{
	assert(tile_is_rail_bridge(t));
	SB(t->m2, 12, 4, type);
}

/**
 * Check if a rail bridge head is a custom bridge head
 * @param t The tile to check
 * @pre tile_is_rail_bridge(t)
 * @return Whether the tile is a custom bridge head
 */
static inline bool tile_is_rail_custom_bridgehead(const Tile *t)
{
	assert(tile_is_rail_bridge(t));
	return tile_get_trackbits(t) != DiagDirToDiagTrackBits(tile_get_tunnelbridge_direction(t));
}


/**
 * Get the reservation state of the middle part of a bridge
 * @param t The bridge tile whose reservation state to get
 * @pre tile_is_rail_bridge(t)
 * @return Whether the bridge middle part is reserved
 */
static inline bool tile_is_bridge_middle_reserved(const Tile *t)
{
	assert(tile_is_rail_bridge(t));
	return HasBit(t->m2, 6);
}

/**
 * Set the reservation state of the middle part of a bridge
 * @param t The bridge whose reservation state to set
 * @param b The reservation state to set
 * @pre tile_is_rail_bridge(t)
 */
static inline void tile_set_bridge_middle_reserved(Tile *t, bool b)
{
	assert(tile_is_rail_bridge(t));
	if (b) {
		SetBit(t->m2, 6);
	} else {
		ClrBit(t->m2, 6);
	}
}


/**
 * Make a railway tile
 * @param t The tile to make a railway tile
 * @param o The owner of the tile
 * @param trackbits The track bits of the tile
 * @param rt The rail type of the tile
 */
static inline void tile_make_railway(Tile *t, Owner o, TrackBits trackbits, RailType rt)
{
	tile_set_type(t, TT_RAILWAY);
	t->m1 = (TT_TRACK << 6) | o;
	t->m2 = trackbits;
	t->m3 = rt;
	t->m4 = 0;
	t->m5 = rt;
	t->m7 = 0;
}

/**
 * Make a rail bridge ramp
 * @param t The tile to make a rail bridge ramp
 * @param o The owner of the tile
 * @param type The bridge type
 * @param dir The direction the ramp is facing
 * @param rt The rail type of the tile
 */
static inline void tile_make_rail_bridge(Tile *t, Owner o, uint type, DiagDirection dir, RailType rt)
{
	tile_set_type(t, TT_RAILWAY);
	t->m1 = (TT_BRIDGE << 6) | o;
	t->m2 = (type << 12) | DiagDirToDiagTrackBits(dir);
	t->m3 = (dir << 6) | rt;
	t->m4 = 0;
	t->m5 = 0;
	t->m7 = 0;
}

/**
 * Turn a rail bridge ramp into normal railway
 * @param t The tile to convert
 * @pre tile_is_rail_bridge(t)
 * @note trackbits will have to be adjusted after this function is called
 */
static inline void tile_make_railway_from_bridge(Tile *t)
{
	assert(tile_is_rail_bridge(t));
	tile_set_subtype(t, TT_TRACK);
	ClrBit(t->m2, 6);
	SB(t->m2, 12, 4, 0);
	SB(t->m3, 4, 4, 0);
	SB(t->m5, 4, 4, 0);
}

/**
 * Turn a railway tile into a rail bridge ramp
 * @param t The tile to convert
 * @param type The bridge type
 * @param dir The direction the ramp is facing
 * @pre tile_is_rail_track(t)
 * @note trackbits will have to be adjusted after this function is called
 */
static inline void tile_make_rail_bridge_from_track(Tile *t, uint type, DiagDirection dir)
{
	assert(tile_is_rail_track(t));
	tile_set_subtype(t, TT_BRIDGE);
	SB(t->m2, 12, 4, type);
	SB(t->m3, 4, 2, 0);
	SB(t->m3, 6, 2, dir);
}

#endif /* TILE_RAIL_H */
