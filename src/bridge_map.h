/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file bridge_map.h Map accessor functions for bridges. */

#ifndef BRIDGE_MAP_H
#define BRIDGE_MAP_H

#include "rail_map.h"
#include "road_map.h"
#include "bridge.h"

/**
 * Check if a tile has an aqueduct.
 *
 * @param t the tile to check
 * @return whether the tile has an aqueduct
 */
static inline bool IsAqueductTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_MISC, TT_MISC_AQUEDUCT);
}

/**
 * Check if a tile has a bridgehead.
 *
 * @param t the tile to check
 * @return whether the tile has a bridge head (rail, road or aqueduct)
 */
static inline bool IsBridgeHeadTile(TileIndex t)
{
	return IsRailBridgeTile(t) || IsRoadBridgeTile(t) || IsAqueductTile(t);
}


/**
 * Checks if this is a bridge, instead of a tunnel
 * @param t The tile to analyze
 * @pre IsTunnelBridgeTile(t)
 * @return true if the structure is a bridge one
 */
static inline bool IsBridge(TileIndex t)
{
	assert(IsTunnelBridgeTile(t));
	return HasBit(_mc[t].m5, 7);
}

/**
 * checks if there is a bridge on this tile
 * @param t The tile to analyze
 * @return true if a bridge is present
 */
static inline bool IsBridgeTile(TileIndex t)
{
	return IsTunnelBridgeTile(t) && IsBridge(t);
}

/**
 * checks for the possibility that a bridge may be on this tile
 * These are in fact all the tile types on which a bridge can be found
 * @param t The tile to analyze
 * @return true if a bridge might be present
 */
static inline bool MayHaveBridgeAbove(TileIndex t)
{
	return (IsTileType(t, TT_GROUND) && (IsTileSubtype(t, TT_GROUND_FIELDS) || IsTileSubtype(t, TT_GROUND_CLEAR))) ||
		(IsTileType(t, TT_MISC) && !IsTileSubtype(t, TT_MISC_DEPOT)) ||
			IsRailwayTile(t) || IsRoadTile(t) ||
			IsWaterTile(t) || IsTunnelBridgeTile(t) || IsObjectTile(t);
}

/**
 * checks if a bridge is set above the ground of this tile
 * @param t The tile to analyze
 * @pre MayHaveBridgeAbove(t)
 * @return true if a bridge is detected above
 */
static inline bool IsBridgeAbove(TileIndex t)
{
	assert(MayHaveBridgeAbove(t));
	return GB(_mc[t].m0, 0, 2) != 0;
}

/**
 * Checks if there is a bridge over this tile
 * @param t The tile to check
 * @return Whether there is a bridge over the tile
 */
static inline bool HasBridgeAbove(TileIndex t)
{
	return MayHaveBridgeAbove(t) && IsBridgeAbove(t);
}

/**
 * Determines the type of bridge on a tile
 * @param t The tile to analyze
 * @pre IsBridgeTile(t) || IsBridgeHeadTile(t)
 * @return The bridge type
 */
static inline BridgeType GetBridgeType(TileIndex t)
{
	assert(IsBridgeTile(t) || IsBridgeHeadTile(t));
	return _mc[t].m4;
}

/**
 * Get the axis of the bridge that goes over the tile. Not the axis or the ramp.
 * @param t The tile to analyze
 * @pre IsBridgeAbove(t)
 * @return the above mentioned axis
 */
static inline Axis GetBridgeAxis(TileIndex t)
{
	assert(IsBridgeAbove(t));
	return (Axis)(GB(_mc[t].m0, 0, 2) - 1);
}

TileIndex GetNorthernBridgeEnd(TileIndex t);
TileIndex GetSouthernBridgeEnd(TileIndex t);
TileIndex GetOtherBridgeEnd(TileIndex t);

int GetBridgeHeight(TileIndex tile);
/**
 * Get the height ('z') of a bridge in pixels.
 * @param tile the bridge ramp tile to get the bridge height from
 * @return the height of the bridge in pixels
 */
static inline int GetBridgePixelHeight(TileIndex tile)
{
	return GetBridgeHeight(tile) * TILE_HEIGHT;
}

/**
 * Remove the bridge over the given axis.
 * @param t the tile to remove the bridge from
 * @param a the axis of the bridge to remove
 * @pre MayHaveBridgeAbove(t)
 */
static inline void ClearSingleBridgeMiddle(TileIndex t, Axis a)
{
	assert(MayHaveBridgeAbove(t));
	ClrBit(_mc[t].m0, a);
}

/**
 * Removes bridges from the given, that is bridges along the X and Y axis.
 * @param t the tile to remove the bridge from
 * @pre MayHaveBridgeAbove(t)
 */
static inline void ClearBridgeMiddle(TileIndex t)
{
	ClearSingleBridgeMiddle(t, AXIS_X);
	ClearSingleBridgeMiddle(t, AXIS_Y);
}

/**
 * Set that there is a bridge over the given axis.
 * @param t the tile to add the bridge to
 * @param a the axis of the bridge to add
 * @pre MayHaveBridgeAbove(t)
 */
static inline void SetBridgeMiddle(TileIndex t, Axis a)
{
	assert(MayHaveBridgeAbove(t));
	SetBit(_mc[t].m0, a);
}

/**
 * Generic part to make a bridge ramp for both roads and rails.
 * @param t          the tile to make a bridge ramp
 * @param o          the new owner of the bridge ramp
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @param tt         the transport type of the bridge
 * @param rt         the road or rail type
 * @note this function should not be called directly.
 */
static inline void MakeBridgeRamp(TileIndex t, Owner o, BridgeType bridgetype, DiagDirection d, TransportType tt, uint rt)
{
	SetTileType(t, TT_TUNNELBRIDGE_TEMP);
	SetTileOwner(t, o);
	_mc[t].m2 = 0;
	_mc[t].m3 = rt;
	_mc[t].m4 = bridgetype;
	_mc[t].m5 = 1 << 7 | tt << 2 | d;
	SB(_mc[t].m0, 2, 2, 0);
	_mc[t].m7 = 0;
}

/**
 * Make a bridge ramp for roads.
 * @param t          the tile to make a bridge ramp
 * @param o          the new owner of the bridge ramp
 * @param owner_road the new owner of the road on the bridge
 * @param owner_tram the new owner of the tram on the bridge
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @param r          the road type of the bridge
 */
static inline void MakeRoadBridgeRamp(TileIndex t, Owner o, Owner owner_road, Owner owner_tram, BridgeType bridgetype, DiagDirection d, RoadTypes r)
{
	MakeBridgeRamp(t, o, bridgetype, d, TRANSPORT_ROAD, 0);
	SetRoadOwner(t, ROADTYPE_ROAD, owner_road);
	if (owner_tram != OWNER_TOWN) SetRoadOwner(t, ROADTYPE_TRAM, owner_tram);
	SetRoadTypes(t, r);
}

/**
 * Make a bridge ramp for rails.
 * @param t          the tile to make a bridge ramp
 * @param o          the new owner of the bridge ramp
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @param r          the rail type of the bridge
 */
static inline void MakeRailBridgeRamp(TileIndex t, Owner o, BridgeType bridgetype, DiagDirection d, RailType r)
{
	MakeBridgeRamp(t, o, bridgetype, d, TRANSPORT_RAIL, r);
}

/**
 * Make a bridge ramp for aqueducts.
 * @param t          the tile to make a bridge ramp
 * @param o          the new owner of the bridge ramp
 * @param d          the direction this ramp must be facing
 */
static inline void MakeAqueductBridgeRamp(TileIndex t, Owner o, DiagDirection d)
{
	MakeBridgeRamp(t, o, 0, d, TRANSPORT_WATER, 0);
}

#endif /* BRIDGE_MAP_H */
