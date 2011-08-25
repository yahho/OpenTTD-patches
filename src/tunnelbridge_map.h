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
 * Get the direction pointing to the other end.
 *
 * Tunnel: Get the direction facing into the tunnel
 * Bridge: Get the direction pointing onto the bridge
 * @param t The tile to analyze
 * @pre IsTunnelBridgeTile(t) || IsBridgeHeadTile(t)
 * @return the above mentioned direction
 */
static inline DiagDirection GetTunnelBridgeDirection(TileIndex t)
{
	assert(IsTunnelBridgeTile(t) || IsBridgeHeadTile(t));
	return (DiagDirection)GB(_mc[t].m5, 0, 2);
}

/**
 * Tunnel: Get the transport type of the tunnel (road or rail)
 * Bridge: Get the transport type of the bridge's ramp
 * @param t The tile to analyze
 * @pre IsTunnelBridgeTile(t) || IsBridgeHeadTile(t)
 * @return the transport type in the tunnel/bridge
 */
static inline TransportType GetTunnelBridgeTransportType(TileIndex t)
{
	if (IsTunnelTile(t)) {
		return GetTunnelTransportType(t);
	} else if (IsRailBridgeTile(t)) {
		return TRANSPORT_RAIL;
	} else if (IsRoadBridgeTile(t)) {
		return TRANSPORT_ROAD;
	} else {
		assert(IsAqueductTile(t));
		return TRANSPORT_WATER;
	}
}

/**
 * Tunnel: Is this tunnel entrance in a snowy or desert area?
 * Bridge: Does the bridge ramp lie in a snow or desert area?
 * @param t The tile to analyze
 * @pre IsTunnelBridgeTile(t) || IsBridgeHeadTile(t)
 * @return true if and only if the tile is in a snowy/desert area
 */
static inline bool HasTunnelBridgeSnowOrDesert(TileIndex t)
{
	assert(IsTunnelBridgeTile(t) || IsBridgeHeadTile(t));
	return HasBit(_mc[t].m7, 5);
}

/**
 * Tunnel: Places this tunnel entrance in a snowy or desert area, or takes it out of there.
 * Bridge: Sets whether the bridge ramp lies in a snow or desert area.
 * @param t the tunnel entrance / bridge ramp tile
 * @param snow_or_desert is the entrance/ramp in snow or desert (true), when
 *                       not in snow and not in desert false
 * @pre IsTunnelBridgeTile(t) || IsBridgeHeadTile(t)
 */
static inline void SetTunnelBridgeSnowOrDesert(TileIndex t, bool snow_or_desert)
{
	assert(IsTunnelBridgeTile(t) || IsBridgeHeadTile(t));
	SB(_mc[t].m7, 5, 1, snow_or_desert);
}

/**
 * Determines type of the wormhole and returns its other end
 * @param t one end
 * @pre IsTunnelBridgeTile(t) || IsBridgeHeadTile(t)
 * @return other end
 */
static inline TileIndex GetOtherTunnelBridgeEnd(TileIndex t)
{
	assert(IsTunnelBridgeTile(t) || IsBridgeHeadTile(t));
	return IsTunnelTile(t) ? GetOtherTunnelEnd(t) : GetOtherBridgeEnd(t);
}


/**
 * Get the reservation state of the rail tunnel/bridge
 * @pre (IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL) || IsRailBridgeTile(t)
 * @param t the tile
 * @return reservation state
 */
static inline bool HasTunnelBridgeReservation(TileIndex t)
{
	if (!IsRailBridgeTile(t)) {
		assert(IsTunnelTile(t));
		assert(GetTunnelTransportType(t) == TRANSPORT_RAIL);
	}
	return HasBit(_mc[t].m5, 4);
}

/**
 * Set the reservation state of the rail tunnel/bridge
 * @pre (IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL) || IsRailBridgeTile(t)
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetTunnelBridgeReservation(TileIndex t, bool b)
{
	if (!IsRailBridgeTile(t)) {
		assert(IsTunnelTile(t));
		assert(GetTunnelTransportType(t) == TRANSPORT_RAIL);
	}
	SB(_mc[t].m5, 4, 1, b ? 1 : 0);
}

/**
 * Get the reserved track bits for a rail tunnel/bridge
 * @pre (IsTunnelTile(t) && GetTunnelTransportType(t) == TRANSPORT_RAIL) || IsRailBridgeTile(t)
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetTunnelBridgeReservationTrackBits(TileIndex t)
{
	return HasTunnelBridgeReservation(t) ? DiagDirToDiagTrackBits(GetTunnelBridgeDirection(t)) : TRACK_BIT_NONE;
}

#endif /* TUNNELBRIDGE_MAP_H */
