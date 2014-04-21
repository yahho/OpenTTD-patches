/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/tilearea.cpp Handling of tile areas. */

#include "../stdafx.h"

#include "tilearea.h"

/**
 * Set this tile area based on initial and final coordinates.
 * @param x0 initial x of the area
 * @param y0 initial y of the area
 * @param x1 final x of the area
 * @param y1 final y of the area
 */
inline void OrthogonalTileArea::Set(uint x0, uint y0, uint x1, uint y1)
{
	this->tile = TileXY(x0, y0);
	this->w = x1 - x0 + 1;
	this->h = y1 - y0 + 1;
}

/**
 * Construct this tile area based on two points.
 * @param start the start of the area
 * @param end   the end of the area
 */
OrthogonalTileArea::OrthogonalTileArea(TileIndex start, TileIndex end)
{
	uint sx = TileX(start);
	uint sy = TileY(start);
	uint ex = TileX(end);
	uint ey = TileY(end);

	if (sx > ex) Swap(sx, ex);
	if (sy > ey) Swap(sy, ey);

	this->Set(sx, sy, ex, ey);
}

/**
 * Add a single tile to a tile area; enlarge if needed.
 * @param to_add The tile to add
 */
void OrthogonalTileArea::Add(TileIndex to_add)
{
	assert (to_add != INVALID_TILE);

	if (this->empty()) {
		this->tile = to_add;
		this->w = 1;
		this->h = 1;
		return;
	}

	uint sx = TileX(this->tile);
	uint sy = TileY(this->tile);
	uint ex = sx + this->w - 1;
	uint ey = sy + this->h - 1;

	uint ax = TileX(to_add);
	uint ay = TileY(to_add);

	this->Set(min(ax, sx), min(ay, sy), max(ax, ex), max(ay, ey));
}

/**
 * Add another tile area to a tile area; enlarge if needed.
 * @param to_add The tile area to add
 */
void OrthogonalTileArea::Add(const OrthogonalTileArea &to_add)
{
	if (to_add.empty()) return;

	if (this->empty()) {
		this->tile = to_add.tile;
		this->w = to_add.w;
		this->h = to_add.h;
		return;
	}

	uint sx = TileX(this->tile);
	uint sy = TileY(this->tile);
	uint ex = sx + this->w - 1;
	uint ey = sy + this->h - 1;

	uint ax = TileX(to_add.tile);
	uint ay = TileY(to_add.tile);
	uint zx = ax + to_add.w - 1;
	uint zy = ay + to_add.h - 1;

	this->Set(min(ax, sx), min(ay, sy), max(zx, ex), max(zy, ey));
}

/**
 * Does this tile area intersect with another?
 * @param ta the other tile area to check against.
 * @return true if they intersect.
 */
bool OrthogonalTileArea::Intersects(const OrthogonalTileArea &ta) const
{
	if (ta.w == 0 || this->w == 0) return false;

	assert(ta.w != 0 && ta.h != 0 && this->w != 0 && this->h != 0);

	uint left1   = TileX(this->tile);
	uint top1    = TileY(this->tile);
	uint right1  = left1 + this->w - 1;
	uint bottom1 = top1  + this->h - 1;

	uint left2   = TileX(ta.tile);
	uint top2    = TileY(ta.tile);
	uint right2  = left2 + ta.w - 1;
	uint bottom2 = top2  + ta.h - 1;

	return !(
			left2   > right1  ||
			right2  < left1   ||
			top2    > bottom1 ||
			bottom2 < top1
		);
}

/**
 * Does this tile area contain a tile?
 * @param tile Tile to test for.
 * @return True if the tile is inside the area.
 */
bool OrthogonalTileArea::Contains(TileIndex tile) const
{
	if (this->w == 0) return false;

	assert(this->w != 0 && this->h != 0);

	uint left   = TileX(this->tile);
	uint top    = TileY(this->tile);
	uint tile_x = TileX(tile);
	uint tile_y = TileY(tile);

	return IsInsideBS(tile_x, left, this->w) && IsInsideBS(tile_y, top, this->h);
}

/**
 * Clamp the tile area to map borders.
 */
void OrthogonalTileArea::ClampToMap()
{
	assert(this->tile < MapSize());
	this->w = min(this->w, MapSizeX() - TileX(this->tile));
	this->h = min(this->h, MapSizeY() - TileY(this->tile));
}

/** Expand the area by a given amount. */
void OrthogonalTileArea::expand (uint radius)
{
	uint x = TileX(this->tile);
	if (x < radius) {
		this->w = min (this->w + x + radius, MapSizeX());
		x = 0;
	} else {
		x -= radius;
		this->w = min (this->w + 2 * radius, MapSizeX() - x);
	}

	uint y = TileY(this->tile);
	if (y < radius) {
		this->h = min (this->h + y + radius, MapSizeY());
		y = 0;
	} else {
		y -= radius;
		this->h = min (this->h + 2 * radius, MapSizeY() - y);
	}

	this->tile = TileXY (x, y);
}


/**
 * Construct this tile area based on two points.
 * @param start the start of the area
 * @param end   the end of the area
 */
DiagonalTileArea::DiagonalTileArea (TileIndex start, TileIndex end)
{
	uint sx = TileX(start);
	uint sy = TileY(start);
	int sa = sx + sy;
	int sb = sx - sy;

	uint ex = TileX(end);
	uint ey = TileY(end);
	int ea = ex + ey;
	int eb = ex - ey;

	if (sa > ea) Swap(sa, ea);
	if (sb > eb) Swap(sb, eb);

	a0 = sa;
	a1 = ea;
	b0 = sb;
	b1 = eb;
}

/**
 * Does this tile area contain a tile?
 * @param tile Tile to test for.
 * @return True if the tile is inside the area.
 */
bool DiagonalTileArea::Contains (TileIndex tile) const
{
	uint x = TileX(tile);
	uint y = TileY(tile);
	int a = x + y;
	int b = x - y;

	return a >= a0 && a <= a1 && b >= b0 && b <= b1;
}


/*
 * There are two possibilities for a diagonal iterator: an "even" area or
 * an "odd" area, where even/odd is determined by the parity of the sum of
 * differences between the coordinates of the two endpoints.
 *
 *         x               x x
 *       x x x           x x x x
 *     x x x x x       x x x x
 *   x x x x x       x x x x
 *     x x x           x x
 *       x
 *      even            odd
 *
 * In our iterator, a "row" will run diagonally (up,right)-wards (so the
 * first tile will be (0,0), then (1,1), etc.; when we wrap around, we
 * start the second row at (1,0), then (2,1). As you can see, there are
 * some differences between an even area and an odd area:
 *   * In an even area, rows have two different lengths, while rows in an
 * odd area are all of the same length.
 *   * The difference between the last tile in a row and the first one in
 * the next row is constant in an even area, but not so in an odd area.
 *   * As a particular case, even areas can have no tiles in odd-numbered
 * rows, if their side has only one tile.
 *
 * This all leads to the following implementation.
 */

/**
 * Construct the iterator.
 * @param corner1 Tile from where to begin iterating.
 * @param corner2 Tile where to end the iterating.
 */
DiagonalTileIterator::DiagonalTileIterator(TileIndex corner1, TileIndex corner2)
	: TileIterator(corner2), x(TileX(corner2)), y(TileY(corner2))
{
	assert(corner1 < MapSize());
	assert(corner2 < MapSize());

	int dist_x = TileX(corner1) - TileX(corner2);
	int dist_y = TileY(corner1) - TileY(corner2);
	int w = dist_x + dist_y;
	int h = dist_y - dist_x;

	this->odd = (w % 2) != 0;

	/* See the ASCII art above to understand these. */
	if (w > 0) {
		this->s1 = 1;
		if (h >= 0) {
			this->s2x = 0;
			this->s2y = 1;
		} else {
			this->s2x = 1;
			this->s2y = 0;
		}
		w = w / 2;
		this->s2x -= w;
		this->s2y -= w;
	} else if (w < 0) {
		this->s1 = -1;
		if (h >= 0) {
			this->s2x = -1;
			this->s2y = 0;
		} else {
			this->s2x = 0;
			this->s2y = -1;
		}
		w = (-w) / 2;
		this->s2x += w;
		this->s2y += w;
	} else { /* w == 0 */
		/* Hack in some values that make things work without a special case in Next. */
		this->odd = true; /* true intended */
		this->s1  = 0;
		this->s2y = (h >= 0) ? 1 : -1;
		this->s2x = -this->s2y;
		h /= 2;
	}

	this->w = w;
	this->n = w;
	this->m = abs(h);
}

/**
 * Move ourselves to the next tile in the rectangle on the map.
 */
void DiagonalTileIterator::Next()
{
	assert(this->tile != INVALID_TILE);

	/* Determine the next tile, while clipping at map borders */
	do {
		if (this->n > 0) {
			/* next tile in the currrent row */
			this->n--;
			this->x += this->s1;
			this->y += this->s1;
		} else if (this->m > 0) {
			/* begin next row */
			this->m--;
			this->x += this->s2x;
			this->y += this->s2y;
			this->n = this->w;
			if ((this->m % 2) != 0) {
				/* adjust odd-numbered rows */
				if (this->odd ) {
					/* odd area, correct initial tile */
					this->x -= this->s1;
					this->y -= this->s1;
				} else {
					/* even area, correct row length */
					assert(this->n > 0);
					this->n--;
				}
			}
		} else {
			/* all done */
			this->tile = INVALID_TILE;
			return;
		}
	} while (this->x >= MapSizeX() || this->y >= MapSizeY());

	this->tile = TileXY(this->x, this->y);
}
