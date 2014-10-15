/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/zoneheight.h Types related to tile zone and height. */

#ifndef TILE_ZONEHEIGHT_H
#define TILE_ZONEHEIGHT_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "../direction_type.h"

static const uint MAX_TILE_HEIGHT     = 15;                    ///< Maximum allowed tile height

static const uint MIN_SNOWLINE_HEIGHT = 2;                     ///< Minimum snowline height
static const uint DEF_SNOWLINE_HEIGHT = 7;                     ///< Default snowline height
static const uint MAX_SNOWLINE_HEIGHT = (MAX_TILE_HEIGHT - 2); ///< Maximum allowed snowline height

/**
 * Tropic zone of a tile (subtropic climate only).
 *
 * The tropiczone is not modified during gameplay. It mainly affects tree growth (desert tiles are visible though).
 *
 * In randomly generated maps:
 *  TROPICZONE_DESERT: Generated everywhere, if there is neither water nor mountains (TileHeight >= 4) in a certain distance from the tile.
 *  TROPICZONE_RAINFOREST: Generated everywhere, if there is no desert in a certain distance from the tile.
 *  TROPICZONE_NORMAL: Everywhere else, i.e. between desert and rainforest and on sea (if you clear the water).
 *
 * In scenarios:
 *  TROPICZONE_NORMAL: Default value.
 *  TROPICZONE_DESERT: Placed manually.
 *  TROPICZONE_RAINFOREST: Placed if you plant certain rainforest-trees.
 */
enum TropicZone {
	TROPICZONE_NORMAL     = 0,      ///< Normal tropiczone
	TROPICZONE_DESERT     = 1,      ///< Tile is desert
	TROPICZONE_RAINFOREST = 2,      ///< Rainforest tile
};

/** Zone and height of a tile. */
struct TileZH {
	byte height; ///< height of the (northern corner of the) tile
	byte zb;     ///< tile zone (2 most significant bits), bridge above (2 least significant bits)
};

/**
 * Get the height of a tile
 *
 * This function returns the height of the northern corner of a tile.
 *
 * @param t The tile whose height to get
 * @return The height of the tile
 */
static inline uint tilezh_get_height(const TileZH *t)
{
	return t->height;
}

/**
 * Set the height of a tile
 *
 * This function sets the height of the northern corner of a tile.
 *
 * @param t The tile whose height to set
 * @param height The new height value of the tile
 * @pre height <= MAX_TILE_HEIGHT
 */
static inline void tilezh_set_height(TileZH *t, uint height)
{
	assert(height <= MAX_TILE_HEIGHT);
	t->height = height;
}

/**
 * Get the tropic zone of a tile
 * @param t The tile whose zone to get
 * @return The zone type of the tile
 */
static inline TropicZone tilezh_get_zone(const TileZH *t)
{
	return (TropicZone)GB(t->zb, 6, 2);
}

/**
 * Set the tropic zone of a tile
 * @param t The tile whose zone to set
 * @param z The new zone type
 * @pre tile < MapSize()
 */
static inline void tilezh_set_zone(TileZH *t, TropicZone z)
{
	SB(t->zb, 6, 2, z);
}


/**
 * Check if this tile has a bridge over it
 * @param t The tile to check
 * @return Whether there is a bridge above the tile
 */
static inline bool tile_has_bridge_above (const TileZH *t)
{
	return GB(t->zb, 0, 2) != 0;
}

/**
 * Get the axis of the bridge over a tile
 * @param t The tile
 * @pre tile_has_bridge_above(t)
 * @return The axis of the bridge
 */
static inline Axis tile_get_bridge_axis (const TileZH *t)
{
	assert (tile_has_bridge_above (t));
	return (Axis)(GB(t->zb, 0, 2) - 1);
}

/**
 * Remove the bridge over a tile
 * @param t The tile
 */
static inline void tile_clear_bridge_above (TileZH *t)
{
	SB(t->zb, 0, 2, 0);
}

/**
 * Set a bridge over a tile
 * @param t The tile
 * @param a Axis of the bridge
 * @pre !tile_has_bridge_above(t)
 */
static inline void tile_set_bridge_above (TileZH *t, Axis a)
{
	assert (!tile_has_bridge_above (t));
	SB(t->zb, 0, 2, 1 << a);
}

#endif /* TILE_ZONEHEIGHT_H */
