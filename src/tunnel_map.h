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
	assert(IsTunnelTile(t));
	return (TransportType)GB(_mc[t].m5, 6, 2);
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
	SetTileTypeSubtype(t, TT_MISC, TT_MISC_TUNNEL);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, o);
	_mc[t].m2 = 0;
	_mc[t].m3 = d << 6;
	_mc[t].m4 = 0;
	_mc[t].m5 = TRANSPORT_ROAD << 6;
	_mc[t].m7 = 0;
	SetRoadOwner(t, ROADTYPE_ROAD, o);
	if (o != OWNER_TOWN) SetRoadOwner(t, ROADTYPE_TRAM, o);
	SetRoadTypes(t, r);
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
	SetTileTypeSubtype(t, TT_MISC, TT_MISC_TUNNEL);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, o);
	_mc[t].m2 = 0;
	_mc[t].m3 = (d << 6) | r;
	_mc[t].m4 = 0;
	_mc[t].m5 = TRANSPORT_RAIL << 6;
	_mc[t].m7 = 0;
}

#endif /* TUNNEL_MAP_H */
