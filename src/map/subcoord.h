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
#include "../track_type.h"

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
	return TileXY (xx >> TILE_SIZE_BITS, yy >> TILE_SIZE_BITS);
}


/** Subcoord difference pair and direction. */
struct InitialSubcoords {
	byte x, y;
	DirectionByte dir;
};

/** Get the inital subcoords and direction for trains and ships on a tile. */
static inline const InitialSubcoords *get_initial_subcoords (Trackdir td)
{
	extern const InitialSubcoords initial_subcoords [TRACKDIR_END];

	return &initial_subcoords[td];
}


/** Full position and tile to which it belongs. */
struct FullPosTile {
	int xx, yy;     ///< full subtile coordinates
	TileIndex tile; ///< tile to which the coordinates belong

	/** Set this position to the given parameters. */
	void set (int xx, int yy, TileIndex tile)
	{
		this->xx = xx;
		this->yy = yy;
		this->tile = tile;
	}

	/** Compute the tile to which the current coordinates belong. */
	void calc_tile (void)
	{
		this->tile = TileVirtXY (this->xx, this->yy);
	}

	/** Set this position to the given coordinates, and compute the tile. */
	void set (int xx, int yy)
	{
		this->xx = xx;
		this->yy = yy;
		this->calc_tile();
	}

	/** Get the value next to z0 that is closest to z1. */
	static int get_towards (int z0, int z1)
	{
		return (z1 == z0) ? z0 : (z1 > z0) ? (z0 + 1) : (z0 - 1);
	}

	/**
	 * Set this position to the point next to (xx0, yy0) that is
	 * closest to (xx1, yy1).
	 */
	void set_towards (int xx0, int yy0, int xx1, int yy1)
	{
		this->xx = get_towards (xx0, xx1);
		this->yy = get_towards (yy0, yy1);
		this->calc_tile();
	}

	struct DeltaCoord { int8 dx, dy; };

	static const DeltaCoord delta_coord [DIR_END];

	/** Set this position to the point next to (xx, yy) in direction dir. */
	void set_towards (int xx, int yy, Direction dir)
	{
		this->xx = xx + delta_coord[dir].dx;
		this->yy = yy + delta_coord[dir].dy;
		this->calc_tile();
	}

	/** Adjust subcoords after a vehicle enters a new tile. */
	void adjust_subcoords (const InitialSubcoords *subcoords)
	{
		this->xx = (this->xx & ~0xF) | subcoords->x;
		this->yy = (this->yy & ~0xF) | subcoords->y;
	}
};


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
