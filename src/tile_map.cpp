/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile_map.cpp Global tile accessors. */

#include "stdafx.h"
#include "tile_map.h"

static Slope GetTileSlopeGivenHeight(int hnorth, int hwest, int heast, int hsouth, int *h) {

	/* Due to the fact that tiles must connect with each other without leaving gaps, the
	 * biggest difference in height between any corner and 'min' is between 0, 1, or 2.
	 *
	 * Also, there is at most 1 corner with height difference of 2. */

	int hminnw = min(hnorth, hwest);
	int hmines = min(heast, hsouth);
	int hmin = min(hminnw, hmines);

	int hmaxnw = max(hnorth, hwest);
	int hmaxes = max(heast, hsouth);
	int hmax = max(hmaxnw, hmaxes);

	uint r = SLOPE_FLAT; ///< Computed slope of the tile.

	if (hnorth != hmin) {
		r += SLOPE_N;
	}
	if (hwest != hmin) {
		r += SLOPE_W;
	}
	if (heast != hmin) {
		r += SLOPE_E;
	}
	if (hsouth != hmin) {
		r += SLOPE_S;
	}
	if (hmax - hmin == 2) {
		r += SLOPE_STEEP;
	}

	if (h != NULL) *h = hmin;

	return (Slope)r;
}

/**
 * Return the slope of a given tile inside the map.
 * @param tile Tile to compute slope of
 * @param h    If not \c NULL, pointer to storage of z height
 * @return Slope of the tile, except for the HALFTILE part
 */
Slope GetTileSlope(TileIndex tile, int *h)
{
	assert(tile < MapSize());

	if (!IsInnerTile(tile)) {
		if (h != NULL) *h = TileHeight(tile);
		return SLOPE_FLAT;
	}

	int hnorth = TileHeight(tile);                    // Height of the North corner.
	int hwest = TileHeight(tile + TileDiffXY(1, 0));  // Height of the West corner.
	int heast = TileHeight(tile + TileDiffXY(0, 1));  // Height of the East corner.
	int hsouth = TileHeight(tile + TileDiffXY(1, 1)); // Height of the South corner.

	return GetTileSlopeGivenHeight(hnorth, hwest, heast, hsouth, h);
}

/**
 * Check if a given tile is flat
 * @param tile Tile to check
 * @param h If not \c NULL, pointer to storage of z height (only if tile is flat)
 * @return Whether the tile is flat
 */
bool IsTileFlat(TileIndex tile, int *h)
{
	assert(tile < MapSize());

	if (!IsInnerTile(tile)) {
		if (h != NULL) *h = TileHeight(tile);
		return true;
	}

	uint z = TileHeight(tile);
	if (TileHeight(tile + TileDiffXY(1, 0)) != z) return false;
	if (TileHeight(tile + TileDiffXY(0, 1)) != z) return false;
	if (TileHeight(tile + TileDiffXY(1, 1)) != z) return false;

	if (h != NULL) *h = z;
	return true;
}

/**
 * Return the slope of a given tile outside the map.
 *
 * @param tile Tile outside the map to compute slope of.
 * @param h    If not \c NULL, pointer to storage of z height.
 * @return Slope of the tile outside map, except for the HALFTILE part. */
Slope GetTilePixelSlopeOutsideMap(int x, int y, int *h)
{
	int hnorth = TileHeightOutsideMap(x, y);         // N corner.
	int hwest = TileHeightOutsideMap(x + 1, y);      // W corner.
	int heast = TileHeightOutsideMap(x, y + 1);      // E corner.
	int hsouth = TileHeightOutsideMap(x + 1, y + 1); // S corner.

	Slope s = GetTileSlopeGivenHeight(hnorth, hwest, heast, hsouth, h);
	if (h != NULL) *h *= TILE_HEIGHT;
	return s;
}

/**
 * Get bottom height of the tile inside the map.
 * @param tile Tile to compute height of
 * @return Minimum height of the tile
 */
int GetTileZ(TileIndex tile)
{
	if (TileX(tile) == MapMaxX() || TileY(tile) == MapMaxY()) return 0;

	int h = TileHeight(tile); // N corner
	h = min(h, TileHeight(tile + TileDiffXY(1, 0))); // W corner
	h = min(h, TileHeight(tile + TileDiffXY(0, 1))); // E corner
	h = min(h, TileHeight(tile + TileDiffXY(1, 1))); // S corner

	return h;
}

/**
 * Get bottom height of the tile outside map.
 *
 * @param tile Tile outside the map to compute height of.
 * @return Minimum height of the tile outside the map. */
int GetTilePixelZOutsideMap(int x, int y)
{
	uint h = TileHeightOutsideMap(x, y);            // N corner.
	h = min(h, TileHeightOutsideMap(x + 1, y));     // W corner.
	h = min(h, TileHeightOutsideMap(x, y + 1));     // E corner.
	h = min(h, TileHeightOutsideMap(x + 1, y + 1)); // S corner

	return h * TILE_HEIGHT;
}

/**
 * Get top height of the tile inside the map.
 * @param t Tile to compute height of
 * @return Maximum height of the tile
 */
int GetTileMaxZ(TileIndex t)
{
	if (TileX(t) == MapMaxX() || TileY(t) == MapMaxY()) return TileHeightOutsideMap(TileX(t), TileY(t));

	int h = TileHeight(t); // N corner
	h = max<int>(h, TileHeight(t + TileDiffXY(1, 0))); // W corner
	h = max<int>(h, TileHeight(t + TileDiffXY(0, 1))); // E corner
	h = max<int>(h, TileHeight(t + TileDiffXY(1, 1))); // S corner

	return h;
}

/**
 * Get top height of the tile outside the map.
 *
 * @see Detailed description in header.
 *
 * @param tile Tile outside to compute height of.
 * @return Maximum height of the tile.
 */
int GetTileMaxPixelZOutsideMap(int x, int y)
{
	uint h = TileHeightOutsideMap(x, y);
	h = max(h, TileHeightOutsideMap(x + 1, y));
	h = max(h, TileHeightOutsideMap(x, y + 1));
	h = max(h, TileHeightOutsideMap(x + 1, y + 1));

	return h * TILE_HEIGHT;
}
