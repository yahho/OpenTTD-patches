/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tunnelbridge_map.h Functions that have tunnels and bridges in common */

#ifndef TUNNELBRIDGE_MAP_H
#define TUNNELBRIDGE_MAP_H

#include "bridge_map.h"
#include "tunnel_map.h"


/**
 * Determines type of the wormhole and returns its other end
 * @param t one end
 * @pre IsTunnelTile(t) || IsBridgeHeadTile(t)
 * @return other end
 */
static inline TileIndex GetOtherTunnelBridgeEnd(TileIndex t)
{
	assert(IsTunnelTile(t) || IsBridgeHeadTile(t));
	return IsTunnelTile(t) ? GetOtherTunnelEnd(t) : GetOtherBridgeEnd(t);
}


/**
 * Get the reservation state of the rail bridge middle part
 * @pre IsRailBridgeTile(t)
 * @param t the tile
 * @return reservation state
 */
static inline bool HasBridgeMiddleReservation(TileIndex t)
{
	assert(IsRailBridgeTile(t));
	return HasBit(_mc[t].m2, 6);
}

/**
 * Set the reservation state of the rail bridge middle part
 * @pre IsRailBridgeTile(t)
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetBridgeMiddleReservation(TileIndex t, bool b)
{
	assert(IsRailBridgeTile(t));
	SB(_mc[t].m2, 6, 1, b ? 1 : 0);
}

/**
 * Get the reservation state of the rail tunnel head
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @return reservation state
 */
static inline bool HasTunnelHeadReservation(TileIndex t)
{
	return tile_is_tunnel_head_reserved(&_mc[t]);
}

/**
 * Set the reservation state of the rail tunnel head
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetTunnelHeadReservation(TileIndex t, bool b)
{
	tile_set_tunnel_head_reserved(&_mc[t], b);
}

/**
 * Get the reservation state of the rail tunnel middle part
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @return reservation state
 */
static inline bool HasTunnelMiddleReservation(TileIndex t)
{
	return tile_is_tunnel_middle_reserved(&_mc[t]);
}

/**
 * Set the reservation state of the rail tunnel middle part
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetTunnelMiddleReservation(TileIndex t, bool b)
{
	tile_set_tunnel_middle_reserved(&_mc[t], b);
}

/**
 * Get the reserved track bits for a rail tunnel
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetTunnelReservationTrackBits(TileIndex t)
{
	return tile_get_tunnel_reserved_trackbits(&_mc[t]);
}

#endif /* TUNNELBRIDGE_MAP_H */
