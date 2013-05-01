/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/class.h Map tile classes. */

#ifndef MAP_CLASS_H
#define MAP_CLASS_H

#include "../stdafx.h"
#include "../tile/class.h"
#include "map.h"
#include "coord.h"
#include "subcoord.h"

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
	return tile_get_type(&_mc[tile]);
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
	return tile_get_subtype(&_mc[tile]);
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
	return tile_is_type(&_mc[tile], type);
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
	return tile_is_subtype(&_mc[tile], subtype);
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
	return tile_is_type_subtype(&_mc[tile], type, subtype);
}

/**
 * Checks if a tile is void.
 *
 * @param tile The tile to check
 * @return true If the tile is void
 */
static inline bool IsVoidTile(TileIndex tile)
{
	return tile_is_void(&_mc[tile]);
}

/**
 * Check if a tile is ground (but not void).
 * @param t the tile to check
 * @return whether the tile is ground (fields, clear or trees)
 */
static inline bool IsGroundTile(TileIndex t)
{
	return tile_is_ground(&_mc[t]);
}

/**
 * Checks if a tile has fields.
 *
 * @param tile The tile to check
 * @return true If the tile has fields
 */
static inline bool IsFieldsTile(TileIndex t)
{
	return tile_is_fields(&_mc[t]);
}

/**
 * Check if a tile is empty ground.
 * @param t the tile to check
 * @return whether the tile is empty ground
 */
static inline bool IsClearTile(TileIndex t)
{
	return tile_is_clear(&_mc[t]);
}

/**
 * Checks if a tile has trees.
 * @param t the tile to check
 * @return whether the tile has trees
 */
static inline bool IsTreeTile(TileIndex t)
{
	return tile_is_trees(&_mc[t]);
}

/**
 * Checks if a tile has an object.
 *
 * @param tile The tile to check
 * @return true If the tile has an object
 */
static inline bool IsObjectTile(TileIndex tile)
{
	return tile_is_object(&_mc[tile]);
}

/**
 * Checks if a tile has water.
 *
 * @param tile The tile to check
 * @return true If the tile has water
 */
static inline bool IsWaterTile(TileIndex tile)
{
	return tile_is_water(&_mc[tile]);
}

/**
 * Checks if a tile is railway.
 *
 * @param tile The tile to check
 * @return true If the tile is railway
 */
static inline bool IsRailwayTile(TileIndex tile)
{
	return tile_is_railway(&_mc[tile]);
}

/*
 * Check if a tile is normal rail.
 *
 * @param t the tile to check
 * @return whether the tile is normal rail
 */
static inline bool IsNormalRailTile(TileIndex t)
{
	return tile_is_rail_track(&_mc[t]);
}

/*
 * Check if a tile has a rail bridgehead.
 *
 * @param t the tile to check
 * @return whether the tile has a rail bridgehead
 */
static inline bool IsRailBridgeTile(TileIndex t)
{
	return tile_is_rail_bridge(&_mc[t]);
}

/**
 * Checks if a tile has a road.
 *
 * @param tile The tile to check
 * @return true If the tile has a road
 */
static inline bool IsRoadTile(TileIndex tile)
{
	return tile_is_road(&_mc[tile]);
}

/*
 * Check if a tile is normal road.
 *
 * @param t the tile to check
 * @return whether the tile is normal road
 */
static inline bool IsNormalRoadTile(TileIndex t)
{
	return tile_is_road_track(&_mc[t]);
}

/*
 * Check if a tile has a road bridgehead.
 *
 * @param t the tile to check
 * @return whether the tile has a road bridgehead
 */
static inline bool IsRoadBridgeTile(TileIndex t)
{
	return tile_is_road_bridge(&_mc[t]);
}

/**
 * Return whether a tile is a level crossing tile.
 * @param t Tile to query.
 * @return True if level crossing tile.
 */
static inline bool IsLevelCrossingTile(TileIndex t)
{
	return tile_is_crossing(&_mc[t]);
}

/**
 * Check if a tile has an aqueduct.
 *
 * @param t the tile to check
 * @return whether the tile has an aqueduct
 */
static inline bool IsAqueductTile(TileIndex t)
{
	return tile_is_aqueduct(&_mc[t]);
}

/**
 * Checks if a tile is a tunnel (entrance).
 *
 * @param tile The tile to check
 * @return true If the tile is a tunnel (entrance)
 */
static inline bool IsTunnelTile(TileIndex tile)
{
	return tile_is_tunnel(&_mc[tile]);
}

/**
 * Check if a tile has a ground (rail or road) depot.
 * @param t the tile to check
 * @return whether the tile has a ground depot
 */
static inline bool IsGroundDepotTile(TileIndex t)
{
	return tile_is_ground_depot(&_mc[t]);
}

/**
 * Checks if a tile is a station tile.
 *
 * @param tile The tile to check
 * @return true If the tile is a station tile
 */
static inline bool IsStationTile(TileIndex tile)
{
	return tile_is_station(&_mc[tile]);
}

/**
 * Checks if a tile is an industry.
 *
 * @param tile The tile to check
 * @return true If the tile is an industry
 */
static inline bool IsIndustryTile(TileIndex tile)
{
	return tile_is_industry(&_mc[tile]);
}

/**
 * Checks if a tile is a house.
 *
 * @param tile The tile to check
 * @return true If the tile is a house
 */
static inline bool IsHouseTile(TileIndex tile)
{
	return tile_is_house(&_mc[tile]);
}

/**
 * Check if a tile has a bridgehead.
 *
 * @param t the tile to check
 * @return whether the tile has a bridge head (rail, road or aqueduct)
 */
static inline bool IsBridgeHeadTile(TileIndex t)
{
	return tile_is_bridge(&_mc[t]);
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

#endif /* MAP_CLASS_H */
