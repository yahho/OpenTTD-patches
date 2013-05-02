/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/road.cpp Map tile complex road accessors. */

#include "../stdafx.h"
#include "../road_type.h"
#include "map.h"
#include "coord.h"
#include "common.h"
#include "tunnel.h"
#include "depot.h"
#include "station.h"
#include "road.h"

/**
 * Returns the RoadBits on an arbitrary tile
 * Special behaviour:
 * - road depots: entrance is treated as road piece
 * - road tunnels: entrance is treated as road piece
 * - bridge ramps: start of the ramp is treated as road piece
 * - bridge middle parts: bridge itself is ignored
 *
 * If tunnel_bridge_entrance is set then the road bit that leads
 * into the tunnel/bridge is also returned
 * @param tile the tile to get the road bits for
 * @param rt   the road type to get the road bits form
 * @param tunnel_bridge_entrance whether to return the road bit that leads into a tunnel/bridge.
 * @return the road bits of the given tile
 */
RoadBits GetAnyRoadBits(TileIndex tile, RoadType rt, bool tunnel_bridge_entrance)
{
	switch (GetTileType(tile)) {
		case TT_ROAD: {
			if (!HasTileRoadType(tile, rt)) return ROAD_NONE;
			RoadBits bits = GetRoadBits(tile, rt);
			if (!tunnel_bridge_entrance && IsTileSubtype(tile, TT_BRIDGE)) {
				bits &= ~DiagDirToRoadBits(GetTunnelBridgeDirection(tile));
			}
			return bits;
		}

		case TT_MISC:
			switch (GetTileSubtype(tile)) {
				default: NOT_REACHED();
				case TT_MISC_CROSSING: return HasTileRoadType(tile, rt) ? GetCrossingRoadBits(tile) : ROAD_NONE;
				case TT_MISC_AQUEDUCT: return ROAD_NONE;
				case TT_MISC_DEPOT:    return IsRoadDepot(tile) && HasTileRoadType(tile, rt) ? DiagDirToRoadBits(GetGroundDepotDirection(tile)) : ROAD_NONE;
				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType(tile) != TRANSPORT_ROAD) return ROAD_NONE;
					if (!HasTileRoadType(tile, rt)) return ROAD_NONE;
					return tunnel_bridge_entrance ?
							AxisToRoadBits(DiagDirToAxis(GetTunnelBridgeDirection(tile))) :
							DiagDirToRoadBits(ReverseDiagDir(GetTunnelBridgeDirection(tile)));
			}

		case TT_STATION:
			if (!IsRoadStopTile(tile)) return ROAD_NONE;
			if (!HasTileRoadType(tile, rt)) return ROAD_NONE;
			if (IsDriveThroughStopTile(tile)) return (GetRoadStopDir(tile) == DIAGDIR_NE) ? ROAD_X : ROAD_Y;
			return DiagDirToRoadBits(GetRoadStopDir(tile));

		default: return ROAD_NONE;
	}
}
