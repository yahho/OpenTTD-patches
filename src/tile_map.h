/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile_map.h Map writing/reading functions for tiles. */

#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "tile/zoneheight.h"
#include "slope_type.h"
#include "map_func.h"
#include "core/bitmath_func.hpp"
#include "settings_type.h"

/**
 * Returns the height of a tile
 *
 * This function returns the height of the northern corner of a tile.
 * This is saved in the global map-array. It does not take affect by
 * any slope-data of the tile.
 *
 * @param tile The tile to get the height from
 * @return the height of the tile
 * @pre tile < MapSize()
 */
static inline uint TileHeight(TileIndex tile)
{
	assert(tile < MapSize());
	return tilezh_get_height(&_mth[tile]);
}

/**
 * Sets the height of a tile.
 *
 * This function sets the height of the northern corner of a tile.
 *
 * @param tile The tile to change the height
 * @param height The new height value of the tile
 * @pre tile < MapSize()
 * @pre heigth <= MAX_TILE_HEIGHT
 */
static inline void SetTileHeight(TileIndex tile, uint height)
{
	assert(tile < MapSize());
	tilezh_set_height(&_mth[tile], height);
}

/**
 * Returns the height of a tile in pixels.
 *
 * This function returns the height of the northern corner of a tile in pixels.
 *
 * @param tile The tile to get the height
 * @return The height of the tile in pixel
 */
static inline uint TilePixelHeight(TileIndex tile)
{
	return TileHeight(tile) * TILE_HEIGHT;
}

/**
 * Get the tiletype of a given tile.
 *
 * @param tile The tile to get the TileType
 * @return The tiletype of the tile
 * @pre tile < MapSize()
 */
static inline TileType GetTileType(TileIndex tile)
{
	assert(tile < MapSize());
	uint type = GB(_mc[tile].m0, 4, 4);
	return (TileType)type;
}

/**
 * Check if a tile is within the map (not a border)
 *
 * @param tile The tile to check
 * @return Whether the tile is in the interior of the map
 * @pre tile < MapSize()
 */
static inline bool IsInnerTile(TileIndex tile)
{
	assert(tile < MapSize());

	uint x = TileX(tile);
	uint y = TileY(tile);

	return x < MapMaxX() && y < MapMaxY() && ((x > 0 && y > 0) || !_settings_game.construction.freeform_edges);
}

/**
 * Set the type of a tile
 *
 * This functions sets the type of a tile. At the south-west or
 * south-east edges of the map, only void tiles are allowed.
 *
 * @param tile The tile to save the new type
 * @param type The type to save
 * @pre tile < MapSize()
 * @pre void type <=> tile is on the south-east or south-west edge.
 */
static inline void SetTileType(TileIndex tile, TileType type)
{
	assert(tile < MapSize());
	assert(type < 8);
	/* Only void tiles are allowed at the lower left and right
	 * edges of the map. If _settings_game.construction.freeform_edges is true,
	 * the upper edges of the map are also VOID tiles. */
	assert(IsInnerTile(tile) || (type == TT_GROUND));
	SB(_mc[tile].m0, 4, 4, type);
}

/**
 * Get the tile subtype of a given tile.
 *
 * @param tile The tile to get the TileSubtype
 * @return The TileSubtype of the tile
 * @pre tile < MapSize() and tile type has subtypes
 */
static inline TileSubtype GetTileSubtype(TileIndex tile)
{
	assert(tile < MapSize());
	assert(tiletype_has_subtypes(GetTileType(tile)));
	uint subtype = GB(_mc[tile].m1, 6, 2);
	return (TileSubtype)subtype;
}

/**
 * Set the type and subtype of a tile.
 * @param tile The tile to set
 * @param type The type to set
 * @param subtype The subtype to set
 * @pre tile < MapSize() and tile type has subtypes
 */
static inline void SetTileTypeSubtype(TileIndex tile, TileType type, TileSubtype subtype)
{
	assert(tile < MapSize());
	assert(type < 8);
	assert(tiletype_has_subtypes(type));
	SB(_mc[tile].m0, 4, 4, type);
	SB(_mc[tile].m1, 6, 2, subtype);
}

/**
 * Checks if a tile is a give tiletype.
 *
 * This function checks if a tile got the given tiletype.
 *
 * @param tile The tile to check
 * @param type The type to check against
 * @return true If the type matches against the type of the tile
 */
static inline bool IsTileType(TileIndex tile, TileType type)
{
	return GetTileType(tile) == type;
}

/**
 * Checks if a tile has a given subtype.
 *
 * @param tile The tile to check
 * @param subtype The subtype to check against
 * @return whether the tile has the given subtype
 * @note there is no check to ensure that the given subtype is allowed by the tile's type
 */
static inline bool IsTileSubtype(TileIndex tile, TileSubtype subtype)
{
	return GetTileSubtype(tile) == subtype;
}

/**
 * Checks if a tile has given type and subtype.
 *
 * @param tile The tile to check
 * @param type The type to check against
 * @param subtype The subtype to check against
 * @return whether the tile has the given type and subtype
 */
static inline bool IsTileTypeSubtype(TileIndex tile, TileType type, TileSubtype subtype)
{
	assert(tiletype_has_subtypes(type));
	return IsTileType(tile, type) && IsTileSubtype(tile, subtype);
}

/**
 * Checks if a tile is void.
 *
 * @param tile The tile to check
 * @return true If the tile is void
 */
static inline bool IsVoidTile(TileIndex tile)
{
	return IsTileTypeSubtype(tile, TT_GROUND, TT_GROUND_VOID);
}

/**
 * Check if a tile is ground (but not void).
 * @param t the tile to check
 * @return whether the tile is ground (fields, clear or trees)
 */
static inline bool IsGroundTile(TileIndex t)
{
	return IsTileType(t, TT_GROUND) && !IsTileSubtype(t, TT_GROUND_VOID);
}

/**
 * Checks if a tile has fields.
 *
 * @param tile The tile to check
 * @return true If the tile has fields
 */
static inline bool IsFieldsTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_GROUND, TT_GROUND_FIELDS);
}

/**
 * Check if a tile is empty ground.
 * @param t the tile to check
 * @return whether the tile is empty ground
 */
static inline bool IsClearTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_GROUND, TT_GROUND_CLEAR);
}

/**
 * Checks if a tile has trees.
 * @param t the tile to check
 * @return whether the tile has trees
 */
static inline bool IsTreeTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_GROUND, TT_GROUND_TREES);
}

/**
 * Checks if a tile has an object.
 *
 * @param tile The tile to check
 * @return true If the tile has an object
 */
static inline bool IsObjectTile(TileIndex tile)
{
	return IsTileType(tile, TT_OBJECT);
}

/**
 * Checks if a tile has water.
 *
 * @param tile The tile to check
 * @return true If the tile has water
 */
static inline bool IsWaterTile(TileIndex tile)
{
	return IsTileType(tile, TT_WATER);
}

/**
 * Checks if a tile is railway.
 *
 * @param tile The tile to check
 * @return true If the tile is railway
 */
static inline bool IsRailwayTile(TileIndex tile)
{
	return IsTileType(tile, TT_RAILWAY);
}

/*
 * Check if a tile is normal rail.
 *
 * @param t the tile to check
 * @return whether the tile is normal rail
 */
static inline bool IsNormalRailTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_RAILWAY, TT_TRACK);
}

/*
 * Check if a tile has a rail bridgehead.
 *
 * @param t the tile to check
 * @return whether the tile has a rail bridgehead
 */
static inline bool IsRailBridgeTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_RAILWAY, TT_BRIDGE);
}

/**
 * Checks if a tile has a road.
 *
 * @param tile The tile to check
 * @return true If the tile has a road
 */
static inline bool IsRoadTile(TileIndex tile)
{
	return IsTileType(tile, TT_ROAD);
}

/*
 * Check if a tile is normal road.
 *
 * @param t the tile to check
 * @return whether the tile is normal road
 */
static inline bool IsNormalRoadTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_ROAD, TT_TRACK);
}

/*
 * Check if a tile has a road bridgehead.
 *
 * @param t the tile to check
 * @return whether the tile has a road bridgehead
 */
static inline bool IsRoadBridgeTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_ROAD, TT_BRIDGE);
}

/**
 * Return whether a tile is a level crossing tile.
 * @param t Tile to query.
 * @return True if level crossing tile.
 */
static inline bool IsLevelCrossingTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_MISC, TT_MISC_CROSSING);
}

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
 * Checks if a tile is a tunnel (entrance).
 *
 * @param tile The tile to check
 * @return true If the tile is a tunnel (entrance)
 */
static inline bool IsTunnelTile(TileIndex tile)
{
	return IsTileTypeSubtype(tile, TT_MISC, TT_MISC_TUNNEL);
}

/**
 * Check if a tile has a ground (rail or road) depot.
 * @param t the tile to check
 * @return whether the tile has a ground depot
 */
static inline bool IsGroundDepotTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_MISC, TT_MISC_DEPOT);
}

/**
 * Checks if a tile is a station tile.
 *
 * @param tile The tile to check
 * @return true If the tile is a station tile
 */
static inline bool IsStationTile(TileIndex tile)
{
	return IsTileType(tile, TT_STATION);
}

/**
 * Checks if a tile is an industry.
 *
 * @param tile The tile to check
 * @return true If the tile is an industry
 */
static inline bool IsIndustryTile(TileIndex tile)
{
	return GB(_mc[tile].m0, 6, 2) == 2;
}

/**
 * Checks if a tile is a house.
 *
 * @param tile The tile to check
 * @return true If the tile is a house
 */
static inline bool IsHouseTile(TileIndex tile)
{
	return GB(_mc[tile].m0, 6, 2) == 3;
}

/**
 * Checks if a tile is valid
 *
 * @param tile The tile to check
 * @return True if the tile is on the map and not void.
 */
static inline bool IsValidTile(TileIndex tile)
{
	return tile < MapSize() && !IsVoidTile(tile);
}

/**
 * Returns the owner of a tile
 *
 * This function returns the owner of a tile. This cannot used
 * for tiles whose type is one of void, house or industry,
 * as no company owned any of these buildings.
 *
 * @param tile The tile to check
 * @return The owner of the tile
 * @pre IsValidTile(tile)
 * @pre The tile must not be a house, an industry or void
 */
static inline Owner GetTileOwner(TileIndex tile)
{
	assert(IsValidTile(tile));
	assert(!IsHouseTile(tile));
	assert(!IsIndustryTile(tile));

	return (Owner)GB(_mc[tile].m1, 0, 5);
}

/**
 * Sets the owner of a tile
 *
 * This function sets the owner status of a tile. Note that you cannot
 * set a owner for tiles of type house, void or industry.
 *
 * @param tile The tile to change the owner status.
 * @param owner The new owner.
 * @pre IsValidTile(tile)
 * @pre The tile must not be a house, an industry or void
 */
static inline void SetTileOwner(TileIndex tile, Owner owner)
{
	assert(IsValidTile(tile));
	assert(!IsHouseTile(tile));
	assert(!IsIndustryTile(tile));

	SB(_mc[tile].m1, 0, 5, owner);
}

/**
 * Checks if a tile belongs to the given owner
 *
 * @param tile The tile to check
 * @param owner The owner to check against
 * @return True if a tile belongs the the given owner
 */
static inline bool IsTileOwner(TileIndex tile, Owner owner)
{
	return GetTileOwner(tile) == owner;
}

/**
 * Set the tropic zone
 * @param tile the tile to set the zone of
 * @param type the new type
 * @pre tile < MapSize()
 */
static inline void SetTropicZone(TileIndex tile, TropicZone type)
{
	assert(tile < MapSize());
	assert(!IsVoidTile(tile) || type == TROPICZONE_NORMAL);
	tilezh_set_zone(&_mth[tile], type);
}

/**
 * Get the tropic zone
 * @param tile the tile to get the zone of
 * @pre tile < MapSize()
 * @return the zone type
 */
static inline TropicZone GetTropicZone(TileIndex tile)
{
	assert(tile < MapSize());
	return tilezh_get_zone(&_mth[tile]);
}

/** Check if a tile has snow/desert. */
#define IsOnDesert IsOnSnow
/**
 * Check if a tile has snow/desert.
 * @param t The tile to query.
 * @return True if the tile has snow/desert.
 */
static inline bool IsOnSnow(TileIndex t)
{
	assert((IsRailwayTile(t) && !IsTileSubtype(t, TT_TRACK)) ||
		IsRoadTile(t) || IsTileType(t, TT_MISC));
	return HasBit(_mc[t].m3, 4);
}

/** Set whether a tile has snow/desert. */
#define SetDesert SetSnow
/**
 * Set whether a tile has snow/desert.
 * @param t The tile to set.
 * @param set Whether to set snow/desert.
 */
static inline void SetSnow(TileIndex t, bool set)
{
	assert((IsRailwayTile(t) && !IsTileSubtype(t, TT_TRACK)) ||
		IsRoadTile(t) || IsTileType(t, TT_MISC));
	if (set) {
		SetBit(_mc[t].m3, 4);
	} else {
		ClrBit(_mc[t].m3, 4);
	}
}

/** Toggle the snow/desert state of a tile. */
#define ToggleDesert ToggleSnow
/**
 * Toggle the snow/desert state of a tile.
 * @param t The tile to change.
 */
static inline void ToggleSnow(TileIndex t)
{
	assert((IsRailwayTile(t) && !IsTileSubtype(t, TT_TRACK)) ||
		IsRoadTile(t) || IsTileType(t, TT_MISC));
	ToggleBit(_mc[t].m3, 4);
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
 * Get the direction pointing to the other end.
 *
 * Tunnel: Get the direction facing into the tunnel
 * Bridge: Get the direction pointing onto the bridge
 * @param t The tile to analyze
 * @pre IsTunnelTile(t) || IsBridgeHeadTile(t)
 * @return the above mentioned direction
 */
static inline DiagDirection GetTunnelBridgeDirection(TileIndex t)
{
	assert(IsTunnelTile(t) || IsBridgeHeadTile(t));
	return (DiagDirection)GB(_mc[t].m3, 6, 2);
}


/**
 * Returns the direction the depot is facing to
 * @param t the tile to get the depot facing from
 * @pre IsGroundDepotTile(t)
 * @return the direction the depot is facing
 */
static inline DiagDirection GetGroundDepotDirection(TileIndex t)
{
	assert(IsGroundDepotTile(t));
	return (DiagDirection)GB(_mc[t].m5, 0, 2);
}


/**
 * Get the current animation frame
 * @param t the tile
 * @pre IsHouseTile(t) || IsObjectTile(t) || IsIndustryTile(t) || IsStationTile(t)
 * @return frame number
 */
static inline byte GetAnimationFrame(TileIndex t)
{
	assert(IsHouseTile(t) || IsObjectTile(t) || IsIndustryTile(t) || IsStationTile(t));
	return _mc[t].m7;
}

/**
 * Set a new animation frame
 * @param t the tile
 * @param frame the new frame number
 * @pre IsHouseTile(t) || IsObjectTile(t) || IsIndustryTile(t) || IsStationTile(t)
 */
static inline void SetAnimationFrame(TileIndex t, byte frame)
{
	assert(IsHouseTile(t) || IsObjectTile(t) || IsIndustryTile(t) || IsStationTile(t));
	_mc[t].m7 = frame;
}

Slope GetTileSlope(TileIndex tile, int *h = NULL);
int GetTileZ(TileIndex tile);
int GetTileMaxZ(TileIndex tile);

bool IsTileFlat(TileIndex tile, int *h = NULL);

/**
 * Return the slope of a given tile
 * @param tile Tile to compute slope of
 * @param h    If not \c NULL, pointer to storage of z height
 * @return Slope of the tile, except for the HALFTILE part
 */
static inline Slope GetTilePixelSlope(TileIndex tile, int *h)
{
	Slope s = GetTileSlope(tile, h);
	if (h != NULL) *h *= TILE_HEIGHT;
	return s;
}

/**
 * Get bottom height of the tile
 * @param tile Tile to compute height of
 * @return Minimum height of the tile
 */
static inline int GetTilePixelZ(TileIndex tile)
{
	return GetTileZ(tile) * TILE_HEIGHT;
}

/**
 * Get top height of the tile
 * @param t Tile to compute height of
 * @return Maximum height of the tile
 */
static inline int GetTileMaxPixelZ(TileIndex tile)
{
	return GetTileMaxZ(tile) * TILE_HEIGHT;
}


/**
 * Compute the distance from a tile edge
 * @param side Tile edge
 * @param x x within the tile
 * @param y y within the tile
 * @return The distance from the edge
 */
static inline uint DistanceFromTileEdge(DiagDirection side, uint x, uint y)
{
	assert(x < TILE_SIZE);
	assert(y < TILE_SIZE);

	switch (side) {
		default: NOT_REACHED();
		case DIAGDIR_NE: return x;
		case DIAGDIR_SE: return TILE_SIZE - 1 - y;
		case DIAGDIR_SW: return TILE_SIZE - 1 - x;
		case DIAGDIR_NW: return y;
	}
}


/**
 * Calculate a hash value from a tile position
 *
 * @param x The X coordinate
 * @param y The Y coordinate
 * @return The hash of the tile
 */
static inline uint TileHash(uint x, uint y)
{
	uint hash = x >> 4;
	hash ^= x >> 6;
	hash ^= y >> 4;
	hash -= y >> 6;
	return hash;
}

/**
 * Get the last two bits of the TileHash
 *  from a tile position.
 *
 * @see TileHash()
 * @param x The X coordinate
 * @param y The Y coordinate
 * @return The last two bits from hash of the tile
 */
static inline uint TileHash2Bit(uint x, uint y)
{
	return GB(TileHash(x, y), 0, 2);
}

#endif /* TILE_MAP_H */
