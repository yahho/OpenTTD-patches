/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile_type.h Types related to tiles. */

#ifndef TILE_TYPE_H
#define TILE_TYPE_H

#include "tile/tile.h"

static const uint TILE_SIZE      = 16;            ///< Tiles are 16x16 "units" in size
static const uint TILE_UNIT_MASK = TILE_SIZE - 1; ///< For masking in/out the inner-tile units.
static const uint TILE_PIXELS    = 32;            ///< a tile is 32x32 pixels
static const uint TILE_HEIGHT    =  8;            ///< The standard height-difference between tiles on two levels is 8 (z-diff 8)

static const uint MAX_TILE_HEIGHT     = 15;                    ///< Maximum allowed tile height

static const uint MIN_SNOWLINE_HEIGHT = 2;                     ///< Minimum snowline height
static const uint DEF_SNOWLINE_HEIGHT = 7;                     ///< Default snowline height
static const uint MAX_SNOWLINE_HEIGHT = (MAX_TILE_HEIGHT - 2); ///< Maximum allowed snowline height


/**
 * Additional infos of a tile on a tropic game.
 *
 * The tropiczone is not modified during gameplay. It mainly affects tree growth. (desert tiles are visible though)
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

/**
 * The index/ID of a Tile.
 */
typedef uint32 TileIndex;

/**
 * The very nice invalid tile marker
 */
static const TileIndex INVALID_TILE = (TileIndex)-1;

#endif /* TILE_TYPE_H */
