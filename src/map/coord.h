/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/coord.h Map coordinate system. */

#ifndef MAP_COORD_H
#define MAP_COORD_H

#include "../stdafx.h"
#include "../core/math_func.hpp"
#include "map.h"
#include "../direction_type.h"
#include "../direction_func.h"
#include "../settings_type.h"

/**
 * The index/ID of a Tile.
 */
typedef uint32 TileIndex;

/**
 * The very nice invalid tile marker
 */
static const TileIndex INVALID_TILE = (TileIndex)-1;

/**
 * Get the X component of a tile
 * @param tile the tile to get the X component of
 * @return the X component
 */
static inline uint TileX(TileIndex tile)
{
	return tile & MapMaxX();
}

/**
 * Get the Y component of a tile
 * @param tile the tile to get the Y component of
 * @return the Y component
 */
static inline uint TileY(TileIndex tile)
{
	return tile >> MapLogX();
}

/**
 * Returns the TileIndex of a coordinate.
 *
 * @param x The x coordinate of the tile
 * @param y The y coordinate of the tile
 * @return The TileIndex calculated by the coordinate
 */
static inline TileIndex TileXY(uint x, uint y)
{
	return (y << MapLogX()) + x;
}

/**
 * Check if a tile is within the map (not a border)
 *
 * @param x The x coordinate of the tile
 * @param y The y coordinate of the tile
 * @return Whether the tile is in the interior of the map
 */
static inline bool IsInnerTileXY(uint x, uint y)
{
	return x < MapMaxX() && y < MapMaxY() && ((x > 0 && y > 0) || !_settings_game.construction.freeform_edges);
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

	return IsInnerTileXY(TileX(tile), TileY(tile));
}


/**
 * An offset value between two tiles.
 *
 * This value is used for the difference between
 * two tiles. It can be added to a tileindex to get
 * the resulting tileindex of the start tile applied
 * with this saved difference.
 *
 * @see TileDiffXY(int, int)
 */
typedef int32 TileIndexDiff;

/**
 * Calculates an offset for the given coordinate(-offset).
 *
 * This function calculate an offset value which can be added to an
 * #TileIndex. The coordinates can be negative.
 *
 * @param x The offset in x direction
 * @param y The offset in y direction
 * @return The resulting offset value of the given coordinate
 * @see ToTileIndexDiff(CoordDiff)
 */
static inline TileIndexDiff TileDiffXY(int x, int y)
{
	/* Multiplication gives much better optimization on MSVC than shifting.
	 * 0 << shift isn't optimized to 0 properly.
	 * Typically x and y are constants, and then this doesn't result
	 * in any actual multiplication in the assembly code.. */
	return (y * MapSizeX()) + x;
}

#ifndef _DEBUG
	/**
	 * Adds two tiles together.
	 *
	 * @param x One tile
	 * @param y Another tile to add
	 * @return The resulting tile(index)
	 */
	#define TILE_ADD(x, y) ((x) + (y))
#else
	extern TileIndex TileAdd(TileIndex tile, TileIndexDiff add,
		const char *exp, const char *file, int line);
	#define TILE_ADD(x, y) (TileAdd((x), (y), #x " + " #y, __FILE__, __LINE__))
#endif

/**
 * Adds a given offset to a tile.
 *
 * @param tile The tile to add an offset on it
 * @param x The x offset to add to the tile
 * @param y The y offset to add to the tile
 */
#define TILE_ADDXY(tile, x, y) TILE_ADD(tile, TileDiffXY(x, y))

TileIndex TileAddWrap(TileIndex tile, int addx, int addy);


/**
 * A pair-construct of a TileIndexDiff.
 *
 * This can be used to save the difference between to
 * tiles as a pair of x and y value.
 */
struct CoordDiff {
	int16 x;        ///< The x value of the coordinate
	int16 y;        ///< The y value of the coordinate
};

/**
 * Return the offset between to tiles from a CoordDiff struct.
 *
 * This function works like #TileDiffXY(int, int) and returns the
 * difference between two tiles.
 *
 * @param diff The coordinate of the offset as CoordDiff
 * @return The difference between two tiles.
 * @see TileDiffXY(int, int)
 */
static inline TileIndexDiff ToTileIndexDiff(CoordDiff diff)
{
	return (diff.y << MapLogX()) + diff.x;
}

/**
 * Add a CoordDiff to a TileIndex and returns the new one.
 *
 * Returns tile + the diff given in diff. If the result tile would end up
 * outside of the map, INVALID_TILE is returned instead.
 *
 * @param tile The base tile to add the offset on
 * @param diff The offset to add on the tile
 * @return The resulting TileIndex
 */
static inline TileIndex AddCoordDiffWrap(TileIndex tile, CoordDiff diff)
{
	int x = TileX(tile) + diff.x;
	int y = TileY(tile) + diff.y;
	/* Negative value will become big positive value after cast */
	if ((uint)x >= MapSizeX() || (uint)y >= MapSizeY()) return INVALID_TILE;
	return TileXY(x, y);
}

/**
 * Returns the diff between two tiles
 *
 * @param tile_a from tile
 * @param tile_b to tile
 * @return the difference between tila_a and tile_b
 */
static inline CoordDiff TileCoordDiff(TileIndex tile_a, TileIndex tile_b)
{
	CoordDiff difference;

	difference.x = TileX(tile_a) - TileX(tile_b);
	difference.y = TileY(tile_a) - TileY(tile_b);

	return difference;
}

/**
 * Returns the CoordDiff offset from a DiagDirection.
 *
 * @param dir The given direction
 * @return The offset as CoordDiff value
 */
static inline CoordDiff CoordDiffByDiagDir(DiagDirection dir)
{
	extern const CoordDiff _tileoffs_by_diagdir[DIAGDIR_END];

	assert(IsValidDiagDirection(dir));
	return _tileoffs_by_diagdir[dir];
}

/**
 * Returns the CoordDiff offset from a Direction.
 *
 * @param dir The given direction
 * @return The offset as CoordDiff value
 */
static inline CoordDiff CoordDiffByDir(Direction dir)
{
	extern const CoordDiff _tileoffs_by_dir[DIR_END];

	assert(IsValidDirection(dir));
	return _tileoffs_by_dir[dir];
}


/**
 * Convert a DiagDirection to a TileIndexDiff
 *
 * @param dir The DiagDirection
 * @return The resulting TileIndexDiff
 * @see CoordDiffByDiagDir
 */
static inline TileIndexDiff TileOffsByDiagDir(DiagDirection dir)
{
	extern MapSizeParams map_size;

	assert(IsValidDiagDirection(dir));
	return map_size.diffs[DiagDirToDir(dir)];
}

/**
 * Convert a Direction to a TileIndexDiff.
 *
 * @param dir The direction to convert from
 * @return The resulting TileIndexDiff
 */
static inline TileIndexDiff TileOffsByDir(Direction dir)
{
	extern MapSizeParams map_size;

	assert(IsValidDirection(dir));
	return map_size.diffs[dir];
}

/**
 * Adds a DiagDir to a tile.
 *
 * @param tile The current tile
 * @param dir The direction in which we want to step
 * @return the moved tile
 */
static inline TileIndex TileAddByDiagDir(TileIndex tile, DiagDirection dir)
{
	return TILE_ADD(tile, TileOffsByDiagDir(dir));
}


/**
 * Determines the DiagDirection to get from one tile to another.
 * The tiles do not necessarily have to be adjacent.
 * @param tile_from Origin tile
 * @param tile_to Destination tile
 * @return DiagDirection from tile_from towards tile_to, or INVALID_DIAGDIR if the tiles are not on an axis
 */
static inline DiagDirection DiagdirBetweenTiles(TileIndex tile_from, TileIndex tile_to)
{
	int dx = (int)TileX(tile_to) - (int)TileX(tile_from);
	int dy = (int)TileY(tile_to) - (int)TileY(tile_from);
	if (dx == 0) {
		if (dy == 0) return INVALID_DIAGDIR;
		return (dy < 0 ? DIAGDIR_NW : DIAGDIR_SE);
	} else {
		if (dy != 0) return INVALID_DIAGDIR;
		return (dx < 0 ? DIAGDIR_NE : DIAGDIR_SW);
	}
}


/* Functions to calculate distances */
uint DistanceManhattan(TileIndex, TileIndex); ///< also known as L1-Norm. Is the shortest distance one could go over diagonal tracks (or roads)
uint DistanceSquare(TileIndex, TileIndex); ///< euclidian- or L2-Norm squared
uint DistanceMax(TileIndex, TileIndex); ///< also known as L-Infinity-Norm
uint DistanceMaxPlusManhattan(TileIndex, TileIndex); ///< Max + Manhattan
uint DistanceFromEdge(TileIndex); ///< shortest distance from any edge of the map
uint DistanceFromEdgeDir(TileIndex, DiagDirection); ///< distance from the map edge in given direction

/**
 * Compute the distance between two tiles, when the difference between
 * the tiles is parallel to one of the axes.
 */
static inline uint DistanceAlongAxis(TileIndex t1, TileIndex t2)
{
	int x1 = TileX(t1);
	int y1 = TileY(t1);
	int x2 = TileX(t2);
	int y2 = TileY(t2);

	assert((x1 == x2) || (y1 == y2));

	return abs(x2 + y2 - x1 - y1);
}


/**
 * A callback function type for searching tiles.
 *
 * @param tile The tile to test
 * @param user_data additional data for the callback function to use
 * @return A boolean value, depend on the definition of the function.
 */
typedef bool TestTileOnSearchProc(TileIndex tile, void *user_data);


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

/**
 * Get a random tile out of a given seed.
 * @param r the random 'seed'
 * @return a valid tile
 */
static inline TileIndex RandomTileSeed(uint32 r)
{
	return TILE_MASK(r);
}

/**
 * Get a valid random tile.
 * @note a define so 'random' gets inserted in the place where it is actually
 *       called, thus making the random traces more explicit.
 * @return a valid tile
 */
#define RandomTile() RandomTileSeed(Random())

#endif /* MAP_COORD_H */
