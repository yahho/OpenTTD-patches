/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/map.h Basic map definitions. */

#ifndef MAP_MAP_H
#define MAP_MAP_H

#include "../stdafx.h"
#include "../core/math_func.hpp"
#include "../direction_type.h"
#include "../tile/tile.h"
#include "../tile/zoneheight.h"

/** Minimal and maximal map width and height */
static const uint MIN_MAP_SIZE_BITS = 6;                      ///< Minimal size of map is equal to 2 ^ MIN_MAP_SIZE_BITS
static const uint MAX_MAP_SIZE_BITS = 12;                     ///< Maximal size of map is equal to 2 ^ MAX_MAP_SIZE_BITS
static const uint MIN_MAP_SIZE      = 1 << MIN_MAP_SIZE_BITS; ///< Minimal map size = 64
static const uint MAX_MAP_SIZE      = 1 << MAX_MAP_SIZE_BITS; ///< Maximal map size = 4096


/**
 * Pointer to the tile zone-and-height array.
 *
 * This variable points to the tile zone-and-height array which contains
 * the tiles of the map.
 */
extern TileZH *_mth;

/**
 * Pointer to the tile contents array.
 *
 * This variable points to the tile contents array which contains the tiles
 * of the map.
 */
extern Tile *_mc;


void AllocateMap(uint size_x, uint size_y);

/** Map size parameters */
struct MapSizeParams {
	uint log_x;  ///< logarithm of the size (number of bits) along the X axis
	uint log_y;  ///< logarithm of the size (number of bits) along the Y axis
	uint size_x; ///< size of the map along the X axis
	uint size_y; ///< size of the map along the Y axis
	uint size;   ///< total number of tiles on the map
	int diffs[DIR_END]; ///< tile index differences per direction
};

/**
 * Logarithm of the map size along the X side.
 * @note try to avoid using this one
 * @return 2^"return value" == MapSizeX()
 */
static inline uint MapLogX()
{
	extern MapSizeParams map_size;
	return map_size.log_x;
}

/**
 * Logarithm of the map size along the y side.
 * @note try to avoid using this one
 * @return 2^"return value" == MapSizeY()
 */
static inline uint MapLogY()
{
	extern MapSizeParams map_size;
	return map_size.log_y;
}

/**
 * Get the size of the map along the X
 * @return the number of tiles along the X of the map
 */
static inline uint MapSizeX()
{
	extern MapSizeParams map_size;
	return map_size.size_x;
}

/**
 * Get the size of the map along the Y
 * @return the number of tiles along the Y of the map
 */
static inline uint MapSizeY()
{
	extern MapSizeParams map_size;
	return map_size.size_y;
}

/**
 * Get the size of the map
 * @return the number of tiles of the map
 */
static inline uint MapSize()
{
	extern MapSizeParams map_size;
	return map_size.size;
}

/**
 * Gets the maximum X coordinate within the map, including void tiles
 * @return the maximum X coordinate
 */
static inline uint MapMaxX()
{
	return MapSizeX() - 1;
}

/**
 * Gets the maximum Y coordinate within the map, including void tiles
 * @return the maximum Y coordinate
 */
static inline uint MapMaxY()
{
	return MapSizeY() - 1;
}

/**
 * 'Wraps' the given tile to it is within the map. It does
 * this by masking the 'high' bits of.
 * @param x the tile to 'wrap'
 */

#define TILE_MASK(x) ((x) & (MapSize() - 1))


/**
 * Scales the given value by the map size, where the given value is
 * for a 256 by 256 map.
 * @param n the value to scale
 * @return the scaled size
 */
static inline uint ScaleByMapSize(uint n)
{
	/* Subtract 12 from shift in order to prevent integer overflow
	 * for large values of n. It's safe since the min mapsize is 64x64. */
	return CeilDiv(n << (MapLogX() + MapLogY() - 12), 1 << 4);
}

/**
 * Scales the given value by the map perimeter, where the given
 * value is for a 256 by 256 map
 * @param n the value to scale
 * @return the scaled size
 */
static inline uint ScaleByMapPerimeter(uint n)
{
	/* Normal circumference for the X+Y is 256+256 = 1<<9
	 * Note, not actually taking the full circumference into account,
	 * just half of it. */
	return CeilDiv((n << MapLogX()) + (n << MapLogY()), 1 << 9);
}

#endif /* MAP_MAP_H */
