/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/tunnel.h Map tile accessors and other functions for tunnel tiles. */

#ifndef MAP_TUNNEL_H
#define MAP_TUNNEL_H

#include "../stdafx.h"
#include "../tile/misc.h"
#include "../tile/signal.h"
#include "map.h"
#include "coord.h"
#include "class.h"
#include "../direction_type.h"
#include "../track_type.h"
#include "../transport_type.h"
#include "../road_type.h"
#include "../rail_type.h"
#include "../company_type.h"

/**
 * Get the transport type of the tunnel (road or rail)
 * @param t The tile to analyze
 * @pre IsTunnelTile(t)
 * @return the transport type in the tunnel
 */
static inline TransportType GetTunnelTransportType(TileIndex t)
{
	return tile_get_tunnel_transport_type(&_mc[t]);
}

/**
 * Check if a map tile is a rail tunnel tile
 * @param t The index of the map tile to check
 * @return Whether the tile is a rail tunnel tile
 */
static inline bool maptile_is_rail_tunnel(TileIndex t)
{
	return tile_is_rail_tunnel(&_mc[t]);
}

/**
 * Check if a map tile is a road tunnel tile
 * @param t The index of the map tile to check
 * @return Whether the tile is a road tunnel tile
 */
static inline bool maptile_is_road_tunnel(TileIndex t)
{
	return tile_is_road_tunnel(&_mc[t]);
}


/**
 * Get the reservation state of the rail tunnel head
 * @pre maptile_is_rail_tunnel(t)
 * @param t the tile
 * @return reservation state
 */
static inline bool HasTunnelHeadReservation(TileIndex t)
{
	return tile_is_tunnel_head_reserved(&_mc[t]);
}

/**
 * Set the reservation state of the rail tunnel head
 * @pre maptile_is_rail_tunnel(t)
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetTunnelHeadReservation(TileIndex t, bool b)
{
	tile_set_tunnel_head_reserved(&_mc[t], b);
}

/**
 * Get the reserved track bits for a rail tunnel
 * @pre maptile_is_rail_tunnel(t)
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetTunnelReservationTrackBits(TileIndex t)
{
	return tile_get_tunnel_reserved_trackbits(&_mc[t]);
}

/**
 * Get the reservation state of the rail tunnel middle part
 * @pre maptile_is_rail_tunnel(t)
 * @param t the tile
 * @return reservation state
 */
static inline bool HasTunnelMiddleReservation(TileIndex t)
{
	return tile_is_tunnel_middle_reserved(&_mc[t]);
}

/**
 * Set the reservation state of the rail tunnel middle part
 * @pre maptile_is_rail_tunnel(t)
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetTunnelMiddleReservation(TileIndex t, bool b)
{
	tile_set_tunnel_middle_reserved(&_mc[t], b);
}


/**
 * Get the signal byte for the tunnel signals
 * @param t The tile whose signal byte to get
 * @pre maptile_is_rail_tunnel(t)
 */
static inline SignalPair *maptile_tunnel_signalpair(TileIndex t)
{
	return tile_tunnel_signalpair(&_mc[t]);
}

/**
 * Clear the signals on a tunnel head
 * @param t The index of the tunnel head whose signals to clear
 * @pre maptile_is_rail_tunnel(t)
 */
static inline void maptile_clear_tunnel_signals(TileIndex t)
{
	tile_clear_tunnel_signals(&_mc[t]);
}

/**
 * Get present signals on a tunnel head
 * @param t The index of the tunnel head whose present signals to get
 * @pre maptile_is_rail_tunnel(t)
 * @return A bitmask of present signals (bit 0 is outwards, bit 1 is inwards)
 */
static inline uint maptile_get_tunnel_present_signals(TileIndex t)
{
	return tile_get_tunnel_present_signals(&_mc[t]);
}

/**
 * Set present signals on a tunnel head
 * @param t The index of the tunnel head whose present signals to set
 * @param mask A bitmask of present signals (bit 0 is outwards, bit 1 is inwards)
 * @pre maptile_is_rail_tunnel(t)
 */
static inline void maptile_set_tunnel_present_signals(TileIndex t, uint mask)
{
	tile_set_tunnel_present_signals(&_mc[t], mask);
}

/**
 * Check if a tunnel head has signals at all
 * @param t The index of the map tile to check
 * @pre maptile_is_rail_tunnel(t)
 * @return Whether the tunnel head has any signals
 */
static inline bool maptile_has_tunnel_signals(TileIndex t)
{
	return tile_has_tunnel_signals(&_mc[t]);
}

/**
 * Check if a tunnel head has a signal on a particular direction
 * @param t The index of the map tile to check
 * @param inwards Whether to check the direction into the tunnel, else out of it
 * @pre maptile_is_rail_tunnel(t)
 * @return Whether there is a signal in the given direction
 */
static inline bool maptile_has_tunnel_signal(TileIndex t, bool inwards)
{
	return tile_has_tunnel_signal(&_mc[t], inwards);
}

/**
 * Get signal states on a tunnel head
 * @param t The index of the tunnel head whose signal states to get
 * @pre maptile_is_rail_tunnel(t)
 * @return A bitmask of signal states (bit 0 is outwards, bit 1 is inwards)
 */
static inline uint maptile_get_tunnel_signal_states(TileIndex t)
{
	return tile_get_tunnel_signal_states(&_mc[t]);
}

/**
 * Set signal states on a tunnel head
 * @param t The index of the tunnel head whose signal states to set
 * @param mask A bitmask of signal states (bit 0 is outwards, bit 1 is inwards)
 * @pre maptile_is_rail_tunnel(t)
 */
static inline void maptile_set_tunnel_signal_states(TileIndex t, uint mask)
{
	tile_set_tunnel_signal_states(&_mc[t], mask);
}

/**
 * Get the signal state on a direction of a tunnel head
 * @param t The index of the tunnel tile whose signal to check
 * @param inwards Whether to check the direction into the tunnel, else out of it
 * @pre maptile_is_rail_tunnel(t)
 * @return The signal state on the given direction
 */
static inline SignalState maptile_get_tunnel_signal_state(TileIndex t, bool inwards)
{
	return tile_get_tunnel_signal_state(&_mc[t], inwards);
}

/**
 * Set the signal state on a direction of a tunnel head
 * @param t The index of the tunnel tile whose signal to set
 * @param inwards Whether to set the direction into the tunnel, else out of it
 * @param state The state to set
 * @pre maptile_is_rail_tunnel(t)
 */
static inline void maptile_set_tunnel_signal_state(TileIndex t, bool inwards, SignalState state)
{
	tile_set_tunnel_signal_state(&_mc[t], inwards, state);
}

/**
 * Get the type of the signals on a tunnel head
 * @param t The index of the tunnel tile whose signals to get the type of
 * @pre maptile_is_rail_tunnel(t)
 * @return The type of the signals on the tunnel head
 */
static inline SignalType maptile_get_tunnel_signal_type(TileIndex t)
{
	return tile_get_tunnel_signal_type(&_mc[t]);
}

/**
 * Set the type of the signals on a tunnel head
 * @param t The index of the tunnel tile whose signals to set the type of
 * @param type The type of signals to set
 * @pre maptile_is_rail_tunnel(t)
 */
static inline void maptile_set_tunnel_signal_type(TileIndex t, SignalType type)
{
	tile_set_tunnel_signal_type(&_mc[t], type);
}

/**
 * Get the variant of the signals on a tunnel head
 * @param t The index of the tunnel tile whose signals to get the variant of
 * @pre maptile_is_rail_tunnel(t)
 * @return The variant of the signals on the tunnel head
 */
static inline SignalVariant maptile_get_tunnel_signal_variant(TileIndex t)
{
	return tile_get_tunnel_signal_variant(&_mc[t]);
}

/**
 * Set the variant of the signals on a tunnel head
 * @param t The index of the tunnel tile whose signals to set the variant of
 * @param v The variant of signals to set
 * @pre maptile_is_rail_tunnel(t)
 */
static inline void maptile_set_tunnel_signal_variant(TileIndex t, SignalVariant v)
{
	tile_set_tunnel_signal_variant(&_mc[t], v);
}


/**
 * Makes a rail tunnel entrance
 * @param t the entrance of the tunnel
 * @param o the owner of the entrance
 * @param d the direction facing out of the tunnel
 * @param r the rail type used in the tunnel
 */
static inline void MakeRailTunnel(TileIndex t, Owner o, DiagDirection d, RailType r)
{
	tile_make_rail_tunnel(&_mc[t], o, d, r);
}

/**
 * Makes a road tunnel entrance
 * @param t the entrance of the tunnel
 * @param o the owner of the entrance
 * @param d the direction facing out of the tunnel
 * @param r the road type used in the tunnel
 */
static inline void MakeRoadTunnel(TileIndex t, Owner o, DiagDirection d, RoadTypes r)
{
	tile_make_road_tunnel(&_mc[t], o, d, r);
}


TileIndex GetOtherTunnelEnd(TileIndex);

bool IsTunnelInWay(TileIndex, int z);
bool IsTunnelInWayDir(TileIndex tile, int z, DiagDirection dir);

#endif /* MAP_TUNNEL_H */
