/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/subcoord.h Tile coordinate system. */

#ifndef MAP_SUBCOORD_H
#define MAP_SUBCOORD_H

#include "../stdafx.h"
#include "map.h"
#include "coord.h"
#include "../direction_type.h"

static const uint TILE_SIZE_BITS = 4;                   ///< Log of tile size
static const uint TILE_SIZE      = 1 << TILE_SIZE_BITS; ///< Tiles are 16x16 "units" in size
static const uint TILE_UNIT_MASK = TILE_SIZE - 1;       ///< For masking in/out the inner-tile units.
static const uint TILE_PIXELS    = 32;                  ///< a tile is 32x32 pixels
static const uint TILE_HEIGHT    =  8;                  ///< The standard height-difference between tiles on two levels is 8 (z-diff 8)

/**
 * Get a tile from the virtual XY-coordinate.
 * @param x The virtual x coordinate of the tile.
 * @param y The virtual y coordinate of the tile.
 * @return The TileIndex calculated by the coordinate.
 */
static inline TileIndex TileVirtXY(uint xx, uint yy)
{
	return (yy >> TILE_SIZE_BITS << MapLogX()) + (xx >> TILE_SIZE_BITS);
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

#endif /* MAP_SUBCOORD_H */
