/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/bridge.h Map tile accessors and other functions for bridge tiles. */

#ifndef MAP_BRIDGE_H
#define MAP_BRIDGE_H

#include "../stdafx.h"
#include "../tile/zoneheight.h"
#include "../tile/misc.h"
#include "map.h"
#include "coord.h"
#include "class.h"
#include "common.h"
#include "../direction_type.h"
#include "../company_type.h"

/**
 * Checks if there is a bridge over a tile
 * @param t The tile to check
 * @return Whether there is a bridge over the tile
 */
static inline bool HasBridgeAbove (TileIndex tile)
{
	return tile_has_bridge_above (&_mth[tile]);
}

/**
 * Get the axis of the bridge that goes over a tile
 * @param t The tile to check
 * @pre HasBridgeAbove(t)
 * @return The axis of the bridge over the tile
 */
static inline Axis GetBridgeAxis (TileIndex tile)
{
	return tile_get_bridge_axis (&_mth[tile]);
}

/**
 * Remove the bridge over a tile
 * @param t The tile to remove the bridge from
 */
static inline void ClearBridgeMiddle (TileIndex tile)
{
	tile_clear_bridge_above (&_mth[tile]);
}

/**
 * Set that there is a bridge over a tile along the given axis
 * @param t The tile to add the bridge to
 * @param a The axis of the bridge to add
 */
static inline void SetBridgeMiddle (TileIndex tile, Axis a)
{
	tile_set_bridge_above (&_mth[tile], a);
}


TileIndex GetBridgeEnd(TileIndex tile, DiagDirection dir);

/**
 * Finds the northern end of a bridge starting at a middle tile
 * @param t the bridge tile to find the bridge ramp for
 */
static inline TileIndex GetNorthernBridgeEnd(TileIndex t)
{
	return GetBridgeEnd(t, ReverseDiagDir(AxisToDiagDir(GetBridgeAxis(t))));
}

/**
 * Finds the southern end of a bridge starting at a middle tile
 * @param t the bridge tile to find the bridge ramp for
 */
static inline TileIndex GetSouthernBridgeEnd(TileIndex t)
{
	return GetBridgeEnd(t, AxisToDiagDir(GetBridgeAxis(t)));
}

/**
 * Starting at one bridge end finds the other bridge end
 * @param t the bridge ramp tile to find the other bridge ramp for
 */
static inline TileIndex GetOtherBridgeEnd(TileIndex tile)
{
	assert(IsBridgeHeadTile(tile));
	return GetBridgeEnd(tile, GetTunnelBridgeDirection(tile));
}

/**
 * Make a bridge ramp for aqueducts.
 * @param t          the tile to make a bridge ramp
 * @param o          the new owner of the bridge ramp
 * @param d          the direction this ramp must be facing
 */
static inline void MakeAqueductBridgeRamp(TileIndex t, Owner o, DiagDirection d)
{
	tile_make_aqueduct(&_mc[t], o, d);
}

#endif /* MAP_BRIDGE_H */
