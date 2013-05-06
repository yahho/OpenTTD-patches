/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/misc.h Tile functions for misc tiles. */

#ifndef TILE_MISC_H
#define TILE_MISC_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "common.h"
#include "water.h"
#include "signal.h"
#include "../direction_type.h"
#include "../direction_func.h"
#include "../track_type.h"
#include "../track_func.h"
#include "../transport_type.h"
#include "../road_type.h"
#include "../road_func.h"
#include "../rail_type.h"
#include "../company_type.h"

/**
 * Get the road axis of a level crossing
 * @param t The tile whose crossing road axis to get
 * @pre tile_is_crossing(t)
 * @return The axis of the road
 */
static inline Axis tile_get_crossing_road_axis(const Tile *t)
{
	assert(tile_is_crossing(t));
	return (Axis)GB(t->m4, 5, 1);
}

/**
 * Get the road bits of a level crossing
 * @param t The tile whose road bits to get
 * @pre tile_is_crossing(t)
 * @return The road bits of the tile
 */
static inline RoadBits tile_get_crossing_roadbits(const Tile *t)
{
	assert(tile_is_crossing(t));
	return tile_get_crossing_road_axis(t) == AXIS_Y ? ROAD_Y : ROAD_X;
}

/**
 * Get the rail axis of a level crossing
 * @param t The tile whose crossing rail axis to get
 * @pre tile_is_crossing(t)
 * @return The axis of the rail track
 */
static inline Axis tile_get_crossing_rail_axis(const Tile *t)
{
	assert(tile_is_crossing(t));
	return OtherAxis(tile_get_crossing_road_axis(t));
}

/**
 * Get the rail track of a level crossing
 * @param t The tile whose track to get
 * @pre tile_is_crossing(t)
 * @return The rail track of the crossing
 */
static inline Track tile_get_crossing_rail_track(const Tile *t)
{
	return AxisToTrack(tile_get_crossing_rail_axis(t));
}

/**
 * Get the rail track bits of a level crossing
 * @param t The tile whose track bits to get
 * @pre tile_is_crossing(t)
 * @return The rail track bits of the crossing
 */
static inline TrackBits tile_get_crossing_rail_trackbits(const Tile *t)
{
	return AxisToTrackBits(tile_get_crossing_rail_axis(t));
}

/**
 * Get the track reservation state of a level crossing
 * @param t The tile whose reservation state to get
 * @pre tile_is_crossing(t)
 * @return Whether the tile is reserved
 */
static inline bool tile_crossing_is_reserved(const Tile *t)
{
	assert(tile_is_crossing(t));
	return HasBit(t->m4, 7);
}

/**
 * Set the track reservation state of a level crossing
 * @param t The tile whose reservation state to set
 * @param b The reservation state to set
 * @pre tile_is_crossing(t)
 */
static inline void tile_crossing_set_reserved(Tile *t, bool b)
{
	assert(tile_is_crossing(t));
	if (b) {
		SetBit(t->m4, 7);
	} else {
		ClrBit(t->m4, 7);
	}
}

/**
 * Get the track reservation track bits of a level crossing
 * @param t The tile whose reserved track bits to get
 * @pre tile_is_crossing(t)
 * @return The reserved track bits of the crossing
 */
static inline TrackBits tile_crossing_get_reserved_trackbits(const Tile *t)
{
	return tile_crossing_is_reserved(t) ? tile_get_crossing_rail_trackbits(t) : TRACK_BIT_NONE;
}

/**
 * Get the bar state of a level crossing
 * @param t The tile whose bar to get the state of
 * @pre tile_is_crossing(t)
 * @return Whether the crossing is barred
 */
static inline bool tile_crossing_is_barred(const Tile *t)
{
	assert(tile_is_crossing(t));
	return HasBit(t->m4, 6);
}

/**
 * Set the bar state of a level crossing
 * @param t The tile whose bar to set the state of
 * @param b Whether to bar or unbar the crossing
 * @pre tile_is_crossing(t)
 */
static inline void tile_crossing_set_barred(Tile *t, bool b)
{
	assert(tile_is_crossing(t));
	if (b) {
		SetBit(t->m4, 6);
	} else {
		ClrBit(t->m4, 6);
	}
}

/**
 * Make a level crossing
 * @param t The tile to make a level crossing
 * @param rail The owner of the rail track
 * @param road The owner of the road
 * @param tram The owner of the tram track
 * @param axis The axis of the road
 * @param rt The type of rail
 * @param roadtypes The present roadtypes
 * @param town The id of the town that owns the road or is closest to the tile
 */
static inline void tile_make_crossing(Tile *t, Owner rail, Owner road, Owner tram, Axis axis, RailType rt, RoadTypes roadtypes, uint town)
{
	tile_set_type(t, TT_MISC);
	t->m1 = (TT_MISC_CROSSING << 6) | rail;
	t->m2 = town;
	t->m3 = rt;
	t->m4 = axis << 5;
	t->m5 = (tram == OWNER_NONE) ? OWNER_TOWN : tram;
	t->m7 = (roadtypes << 6) | road;
}


/**
 * Make an aqueduct
 * @param t The tile to make an aqueduct
 * @param o The owner of the tile
 * @param dir The direction the ramp is facing
 */
static inline void tile_make_aqueduct(Tile *t, Owner o, DiagDirection dir)
{
	tile_set_type(t, TT_MISC);
	t->m1 = (TT_MISC_AQUEDUCT << 6) | o;
	t->m2 = 0;
	t->m3 = dir << 6;
	t->m4 = 0;
	t->m5 = 0;
	t->m7 = 0;
}


/**
 * Get the transport type of a tunnel
 * @param t The tile whose transport type to get
 * @pre tile_is_tunnel(t)
 * @return The transport type of the tunnel
 */
static inline TransportType tile_get_tunnel_transport_type(const Tile *t)
{
	assert(tile_is_tunnel(t));
	return (TransportType)GB(t->m5, 6, 2);
}

/**
 * Check if a tile is a rail tunnel tile
 * @param t The tile to check
 * @return Whether the tile is a rail tunnel tile
 */
static inline bool tile_is_rail_tunnel(const Tile *t)
{
	return tile_is_tunnel(t) && tile_get_tunnel_transport_type(t) == TRANSPORT_RAIL;
}

/**
 * Check if a tile is a road tunnel tile
 * @param t The tile to check
 * @return Whether the tile is a road tunnel tile
 */
static inline bool tile_is_road_tunnel(const Tile *t)
{
	return tile_is_tunnel(t) && tile_get_tunnel_transport_type(t) == TRANSPORT_ROAD;
}

/**
 * Get the reservation state of a rail tunnel head
 * @param t The tile whose reservation state to get
 * @pre tile_is_rail_tunnel(t)
 * @return Whether the tile is reserved
 */
static inline bool tile_is_tunnel_head_reserved(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return HasBit(t->m5, 4);
}

/**
 * Set the reservation state of a rail tunnel head
 * @param t The tile whose reservation state to set
 * @param b The reservation state to set
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_set_tunnel_head_reserved(Tile *t, bool b)
{
	assert(tile_is_rail_tunnel(t));
	if (b) {
		SetBit(t->m5, 4);
	} else {
		ClrBit(t->m5, 4);
	}
}

/**
 * Get the reserved track bits of a tunnel head
 * @param t The tile whose reserved track bits to get
 * @pre tile_is_rail_tunnel(t)
 * @return The reserved track bits of the tunnel head
 */
static inline TrackBits tile_get_tunnel_reserved_trackbits(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return tile_is_tunnel_head_reserved(t) ? DiagDirToDiagTrackBits(tile_get_tunnelbridge_direction(t)) : TRACK_BIT_NONE;
}

/**
 * Get the reservation state of the middle part of a tunnel
 * @param t The tunnel tile whose reservation state to get
 * @pre tile_is_rail_tunnel(t)
 * @return Whether the tunnel middle part is reserved
 */
static inline bool tile_is_tunnel_middle_reserved(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return HasBit(t->m5, 5);
}

/**
 * Set the reservation state of the middle part of a tunnel
 * @param t The tunnel whose reservation state to set
 * @param b The reservation state to set
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_set_tunnel_middle_reserved(Tile *t, bool b)
{
	assert(tile_is_rail_tunnel(t));
	if (b) {
		SetBit(t->m5, 5);
	} else {
		ClrBit(t->m5, 5);
	}
}

/**
 * Get the signal byte for the tunnel signals
 * @param t The tile whose signal byte to get
 * @pre tile_is_rail_tunnel(t)
 */
static inline SignalPair *tile_tunnel_signalpair(Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return &t->m7;
}

static inline const SignalPair *tile_tunnel_signalpair(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return &t->m7;
}

/**
 * Clear the signals on a tunnel head
 * @param t The tunnel head whose signals to clear
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_clear_tunnel_signals(Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	signalpair_clear(&t->m7);
}

/**
 * Get present signals on a tunnel head
 * @param t The tunnel head whose present signals to get
 * @pre tile_is_rail_tunnel(t)
 * @return A bitmask of present signals (bit 0 is outwards, bit 1 is inwards)
 */
static inline uint tile_get_tunnel_present_signals(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return signalpair_get_present(&t->m7);
}

/**
 * Set present signals on a tunnel head
 * @param t The tunnel head whose present signals to set
 * @param mask A bitmask of present signals (bit 0 is outwards, bit 1 is inwards)
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_set_tunnel_present_signals(Tile *t, uint mask)
{
	assert(tile_is_rail_tunnel(t));
	signalpair_set_present(&t->m7, mask);
}

/**
 * Check if a tunnel head has signals at all
 * @param t The tile to check
 * @pre tile_is_rail_tunnel(t)
 * @return Whether the tunnel head has any signals
 */
static inline bool tile_has_tunnel_signals(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return signalpair_has_signals(&t->m7);
}

/**
 * Check if a tunnel head has a signal on a particular direction
 * @param t The tile to check
 * @param inwards Whether to check the direction into the tunnel, else out of it
 * @pre tile_is_rail_tunnel(t)
 * @return Whether there is a signal in the given direction
 */
static inline bool tile_has_tunnel_signal(const Tile *t, bool inwards)
{
	assert(tile_is_rail_tunnel(t));
	return signalpair_has_signal(&t->m7, inwards);
}

/**
 * Get signal states on a tunnel head
 * @param t The tunnel head whose signal states to get
 * @pre tile_is_rail_tunnel(t)
 * @return A bitmask of signal states (bit 0 is outwards, bit 1 is inwards)
 */
static inline uint tile_get_tunnel_signal_states(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return signalpair_get_states(&t->m7);
}

/**
 * Set signal states on a tunnel head
 * @param t The tunnel head whose signal states to set
 * @param mask A bitmask of signal states (bit 0 is outwards, bit 1 is inwards)
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_set_tunnel_signal_states(Tile *t, uint mask)
{
	assert(tile_is_rail_tunnel(t));
	signalpair_set_states(&t->m7, mask);
}

/**
 * Get the signal state on a direction of a tunnel head
 * @param t The tunnel tile whose signal to check
 * @param inwards Whether to check the direction into the tunnel, else out of it
 * @pre tile_is_rail_tunnel(t)
 * @return The signal state on the given direction
 */
static inline SignalState tile_get_tunnel_signal_state(const Tile *t, bool inwards)
{
	assert(tile_is_rail_tunnel(t));
	return signalpair_get_state(&t->m7, inwards);
}

/**
 * Set the signal state on a direction of a tunnel head
 * @param t The tunnel tile whose signal to set
 * @param inwards Whether to set the direction into the tunnel, else out of it
 * @param state The state to set
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_set_tunnel_signal_state(Tile *t, bool inwards, SignalState state)
{
	assert(tile_is_rail_tunnel(t));
	signalpair_set_state(&t->m7, inwards, state);
}

/**
 * Get the type of the signals on a tunnel head
 * @param t The tunnel tile whose signals to get the type of
 * @pre tile_is_rail_tunnel(t)
 * @return The type of the signals on the tunnel head
 */
static inline SignalType tile_get_tunnel_signal_type(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return signalpair_get_type(&t->m7);
}

/**
 * Set the type of the signals on a tunnel head
 * @param t The tunnel tile whose signals to set the type of
 * @param type The type of signals to set
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_set_tunnel_signal_type(Tile *t, SignalType type)
{
	assert(tile_is_rail_tunnel(t));
	signalpair_set_type(&t->m7, type);
}

/**
 * Get the variant of the signals on a tunnel head
 * @param t The tunnel tile whose signals to get the variant of
 * @pre tile_is_rail_tunnel(t)
 * @return The variant of the signals on the tunnel head
 */
static inline SignalVariant tile_get_tunnel_signal_variant(const Tile *t)
{
	assert(tile_is_rail_tunnel(t));
	return signalpair_get_variant(&t->m7);
}

/**
 * Set the variant of the signals on a tunnel head
 * @param t The tunnel tile whose signals to set the variant of
 * @param v The variant of signals to set
 * @pre tile_is_rail_tunnel(t)
 */
static inline void tile_set_tunnel_signal_variant(Tile *t, SignalVariant v)
{
	assert(tile_is_rail_tunnel(t));
	signalpair_set_variant(&t->m7, v);
}

/**
 * Make a rail tunnel
 * @param t The tile to make a rail tunnel
 * @param o The owner of the tile
 * @param d The direction into the tunnel
 * @param rt The rail type of the tunnel
 */
static inline void tile_make_rail_tunnel(Tile *t, Owner o, DiagDirection d, RailType rt)
{
	tile_set_type(t, TT_MISC);
	t->m1 = (TT_MISC_TUNNEL << 6) | o;
	t->m2 = 0;
	t->m3 = (d << 6) | rt;
	t->m4 = 0;
	t->m5 = TRANSPORT_RAIL << 6;
	t->m7 = 0;
}

/**
 * Make a road tunnel
 * @param t The tile to make a road tunnel
 * @param o The owner of the tile
 * @param d The direction into the tunnel
 * @param r The road type of the tunnel
 */
static inline void tile_make_road_tunnel(Tile *t, Owner o, DiagDirection d, RoadTypes r)
{
	tile_set_type(t, TT_MISC);
	t->m1 = (TT_MISC_TUNNEL << 6) | o;
	t->m2 = 0;
	t->m3 = d << 6;
	t->m4 = 0;
	t->m5 = (TRANSPORT_ROAD << 6) | (o == OWNER_TOWN ? 0 : o == OWNER_NONE ? OWNER_TOWN : o);
	t->m7 = (r << 6) | o;
}


/**
 * Check if a ground depot is a rail depot
 * @param t The tile to check
 * @pre tile_is_ground_depot(t)
 * @return Whether the tile is a rail depot
 */
static inline bool tile_depot_is_rail(const Tile *t)
{
	assert(tile_is_ground_depot(t));
	return !HasBit(t->m1, 5);
}

/**
 * Check if a ground depot is a road depot
 * @param t The tile to check
 * @pre tile_is_ground_depot(t)
 * @return Whether the tile is a road depot
 */
static inline bool tile_depot_is_road(const Tile *t)
{
	assert(tile_is_ground_depot(t));
	return HasBit(t->m1, 5);
}

/**
 * Check if a tile is a rail depot tile
 * @param t The tile to check
 * @return Whether the tile is a rail depot tile
 */
static inline bool tile_is_rail_depot(const Tile *t)
{
	return tile_is_ground_depot(t) && tile_depot_is_rail(t);
}

/**
 * Check if a tile is a road depot tile
 * @param t The tile to check
 * @return Whether the tile is a road depot tile
 */
static inline bool tile_is_road_depot(const Tile *t)
{
	return tile_is_ground_depot(t) && tile_depot_is_road(t);
}

/**
 * Get the index of the depot at a tile
 * @param t The tile whose depot index to get
 * @pre tile_is_ground_depot(t) || tile_is_ship_depot(t)
 * @return The index of the depot at the tile
 */
static inline uint tile_get_depot_index(const Tile *t)
{
	assert(tile_is_ground_depot(t) || tile_is_ship_depot(t));
	return t->m2;
}

/**
 * Get the direction a ground depot is facing
 * @param t The tile whose depot direction to get
 * @pre tile_is_ground_depot(t)
 * @return The direction the depot is facing
 */
static inline DiagDirection tile_get_ground_depot_direction(const Tile *t)
{
	assert(tile_is_ground_depot(t));
	return (DiagDirection)GB(t->m5, 0, 2);
}

/**
 * Get the track of a ground depot
 * @param t The tile whose track to get
 * @pre tile_is_ground_depot(t)
 * @return The track of the depot
 */
static inline Track tile_get_depot_track(const Tile *t)
{
	assert(tile_is_ground_depot(t));
	return DiagDirToDiagTrack(tile_get_ground_depot_direction(t));
}

/**
 * Get the reservation state of a depot
 * @param t The tile whose reservation state to get
 * @pre tile_is_rail_depot(t)
 * @return Whether the tile is reserved
 */
static inline bool tile_is_depot_reserved(const Tile *t)
{
	assert(tile_is_rail_depot(t));
	return HasBit(t->m5, 4);
}

/**
 * Set the reservation state of a depot
 * @param t The tile whose reservation state to set
 * @param b The reservation state to set
 * @pre tile_is_rail_depot(t)
 */
static inline void tile_set_depot_reserved(Tile *t, bool b)
{
	assert(tile_is_rail_depot(t));
	if (b) {
		SetBit(t->m5, 4);
	} else {
		ClrBit(t->m5, 4);
	}
}

/**
 * Get the reserved track bits of a depot
 * @param t The tile whose reserved track bits to get
 * @pre tile_is_rail_depot(t)
 * @return The reserved track bits of the tile
 */
static inline TrackBits tile_get_depot_reserved_trackbits(const Tile *t)
{
	return tile_is_depot_reserved(t) ? TrackToTrackBits(tile_get_depot_track(t)): TRACK_BIT_NONE;
}

/**
 * Make a rail depot
 * @param t The tile to make a rail depot
 * @param o The owner of the depot
 * @param id The index of the depot
 * @param dir The direction the depot is facing
 * @param rt The rail type of the depot
 */
static inline void tile_make_rail_depot(Tile *t, Owner o, uint id, DiagDirection dir, RailType rt)
{
	t->m0 = TT_MISC << 4;
	t->m1 = (TT_MISC_DEPOT << 6) | o;
	t->m2 = id;
	t->m3 = rt;
	t->m4 = 0;
	t->m5 = dir;
	t->m7 = 0;
}

/**
 * Make a road depot
 * @param t The tile to make a road depot
 * @param o The owner of the depot
 * @param id The index of the depot
 * @param dir The direction the depot is facing
 * @param rt The road type of the depot
 */
static inline void tile_make_road_depot(Tile *t, Owner o, uint id, DiagDirection dir, RoadType rt)
{
	t->m0 = TT_MISC << 4;
	t->m1 = (TT_MISC_DEPOT << 6) | (1 << 5) | o;
	t->m2 = id;
	t->m3 = 0;
	t->m4 = 0;
	t->m5 = dir;
	t->m7 = RoadTypeToRoadTypes(rt) << 6;
}

#endif /* TILE_MISC_H */
