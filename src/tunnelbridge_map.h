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
 * Get the reservation state of the rail bridge
 * @pre IsRailBridgeTile(t)
 * @param t the tile
 * @return reservation state
 */
static inline bool HasBridgeReservation(TileIndex t)
{
	assert(IsRailBridgeTile(t));
	return HasBit(_mc[t].m5, 4);
}

/**
 * Set the reservation state of the rail bridge
 * @pre IsRailBridgeTile(t)
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetBridgeReservation(TileIndex t, bool b)
{
	assert(IsRailBridgeTile(t));
	SB(_mc[t].m5, 4, 1, b ? 1 : 0);
}

/**
 * Get the reserved track bits for a rail bridge
 * @pre IsRailBridgeTile(t)
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetBridgeReservationTrackBits(TileIndex t)
{
	return HasBridgeReservation(t) ? DiagDirToDiagTrackBits(GetTunnelBridgeDirection(t)) : TRACK_BIT_NONE;
}

/**
 * Get the reservation state of the rail tunnel
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @return reservation state
 */
static inline bool HasTunnelReservation(TileIndex t)
{
	assert(IsTunnelTile(t));
	assert(GetTunnelTransportType(t) == TRANSPORT_RAIL);
	return HasBit(_mc[t].m5, 4);
}

/**
 * Set the reservation state of the rail tunnel
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetTunnelReservation(TileIndex t, bool b)
{
	assert(IsTunnelTile(t));
	assert(GetTunnelTransportType(t) == TRANSPORT_RAIL);
	SB(_mc[t].m5, 4, 1, b ? 1 : 0);
}

/**
 * Get the reserved track bits for a rail tunnel
 * @pre IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetTunnelReservationTrackBits(TileIndex t)
{
	return HasTunnelReservation(t) ? DiagDirToDiagTrackBits(GetTunnelBridgeDirection(t)) : TRACK_BIT_NONE;
}

#endif /* TUNNELBRIDGE_MAP_H */
