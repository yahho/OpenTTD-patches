/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/zoneheight.h Maptile zone and height accessors. */

#ifndef MAP_ZONEHEIGHT_H
#define MAP_ZONEHEIGHT_H

#include "../stdafx.h"
#include "../tile/zoneheight.h"
#include "map.h"
#include "coord.h"
#include "subcoord.h"

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

/**
 * Set the tropic zone
 * @param tile the tile to set the zone of
 * @param type the new type
 * @pre tile < MapSize()
 */
static inline void SetTropicZone(TileIndex tile, TropicZone type)
{
	assert(tile < MapSize());
	assert(IsInnerTile(tile) || type == TROPICZONE_NORMAL);
	tilezh_set_zone(&_mth[tile], type);
}

#endif /* MAP_ZONEHEIGHT_H */
