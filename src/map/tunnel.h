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
#include "map.h"
#include "coord.h"
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
