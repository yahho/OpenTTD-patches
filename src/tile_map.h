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
#include "tile/class.h"
#include "tile/common.h"
#include "tile/misc.h"
#include "map/coord.h"
#include "map/subcoord.h"
#include "map/zoneheight.h"
#include "map/class.h"
#include "map/common.h"
#include "slope_type.h"
#include "map_func.h"
#include "core/bitmath_func.hpp"
#include "settings_type.h"

/**
 * Returns the direction the depot is facing to
 * @param t the tile to get the depot facing from
 * @pre IsGroundDepotTile(t)
 * @return the direction the depot is facing
 */
static inline DiagDirection GetGroundDepotDirection(TileIndex t)
{
	return tile_get_ground_depot_direction(&_mc[t]);
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

#endif /* TILE_MAP_H */
