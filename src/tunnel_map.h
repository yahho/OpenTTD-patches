/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tunnel_map.h Map accessors for tunnels. */

#ifndef TUNNEL_MAP_H
#define TUNNEL_MAP_H

#include "tile/misc.h"
#include "tile_map.h"
#include "road_map.h"


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

TileIndex GetOtherTunnelEnd(TileIndex);
bool IsTunnelInWay(TileIndex, int z);
bool IsTunnelInWayDir(TileIndex tile, int z, DiagDirection dir);

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

#endif /* TUNNEL_MAP_H */
