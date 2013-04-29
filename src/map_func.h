/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map_func.h Functions related to maps. */

#ifndef MAP_FUNC_H
#define MAP_FUNC_H

#include "core/math_func.hpp"
#include "map/map.h"
#include "map/coord.h"
#include "tile_type.h"
#include "map_type.h"
#include "direction_func.h"

/**
 * Get a tile from the virtual XY-coordinate.
 * @param x The virtual x coordinate of the tile.
 * @param y The virtual y coordinate of the tile.
 * @return The TileIndex calculated by the coordinate.
 */
static inline TileIndex TileVirtXY(uint x, uint y)
{
	return (y >> 4 << MapLogX()) + (x >> 4);
}

uint GetClosestWaterDistance(TileIndex tile, bool water);

#endif /* MAP_FUNC_H */
