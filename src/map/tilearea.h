/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/tilearea.h Handling of tile areas. */

#ifndef MAP_TILEAREA_H
#define MAP_TILEAREA_H

#include "coord.h"

/** Represents the covered area of e.g. a rail station */
struct OrthogonalTileArea {
	TileIndex tile; ///< The base tile of the area
	uint16 w;       ///< The width of the area
	uint16 h;       ///< The height of the area

	/** Just construct this tile area */
	OrthogonalTileArea() : tile(INVALID_TILE), w(0), h(0) {}

	/** Construct this tile area with a single tile */
	OrthogonalTileArea (TileIndex tile) : tile(tile), w(1), h(1) { }

	/**
	 * Construct this tile area with some set values
	 * @param tile the base tile
	 * @param w the width
	 * @param h the height
	 */
	OrthogonalTileArea(TileIndex tile, uint8 w, uint8 h) : tile(tile), w(w), h(h) {}

	OrthogonalTileArea(TileIndex start, TileIndex end);

	void Set(uint x0, uint y0, uint x1, uint y1);

	void Add(TileIndex to_add);

	void Add(const OrthogonalTileArea &to_add);

	/** Check if this tile area is empty. */
	bool empty (void) const
	{
		assert ((this->tile == INVALID_TILE) == (this->w == 0));
		assert ((this->tile == INVALID_TILE) == (this->h == 0));
		return this->tile == INVALID_TILE;
	}

	/**
	 * Clears the 'tile area', i.e. make the tile invalid.
	 */
	void Clear()
	{
		this->tile = INVALID_TILE;
		this->w    = 0;
		this->h    = 0;
	}

	bool Intersects(const OrthogonalTileArea &ta) const;

	bool Contains(TileIndex tile) const;

	void ClampToMap();

	void expand (uint radius);

	void expand (uint xm, uint ym, uint xp, uint yp);

	/**
	 * Get the center tile.
	 * @return The tile at the center, or just north of it.
	 */
	TileIndex GetCenterTile() const
	{
		return TILE_ADDXY(this->tile, this->w / 2, this->h / 2);
	}

	/**
	 * Get the tile in the area closest to a given tile.
	 * @param t The reference tile.
	 * @return The closest tile, or INVALID_TILE if the area is empty.
	 */
	TileIndex get_closest_tile(TileIndex t) const
	{
		if (this->empty()) return INVALID_TILE;

		/* clamp x coordinate */
		uint x = TileX(this->tile);
		x = ClampU(TileX(t), x, x + this->w - 1);

		/* clamp y coordinate */
		uint y = TileY(this->tile);
		y = ClampU(TileY(t), y, y + this->h - 1);

		/* return the tile of our target coordinates */
		return TileXY(x, y);
	}

	/** Get the maximum distance from a tile in the area to any border. */
	uint get_radius_max (TileIndex t) const
	{
		assert (!this->empty());

		uint dx = TileX(t) - TileX(this->tile);
		assert (dx < this->w); // unsigned comparison
		if (dx < this->w / 2) dx = this->w - 1 - dx;

		uint dy = TileY(t) - TileY(this->tile);
		assert (dy < this->h); // unsigned comparison
		if (dy < this->h / 2) dy = this->h - 1 - dy;

		return max (dx, dy);
	}

	/** Scan for tiles in a given row or column (internal). */
	template <class Pred>
	static bool scan_row_column (TileIndex tile, uint diff, uint n, Pred pred)
	{
		while (n > 0) {
			if (pred(tile)) return true;
			tile += diff;
			n--;
		}
		return false;
	}

	/**
	 * Shrink the tile area spanned by a set of tiles when tiles are
	 * removed from the set (internal).
	 * @param pred   Set predicate.
	 * @param left   Area may shrink from the left.
	 * @param right  Area may shrink from the right.
	 * @param bottom Area may shrink from the bottom.
	 * @param top    Area may shrink from the top.
	 */
	template <class Pred>
	void shrink_span (Pred pred, bool left, bool right, bool bottom, bool top)
	{
		const TileIndexDiff diff_x = TileDiffXY (1, 0); // rightwards
		const TileIndexDiff diff_y = TileDiffXY (0, 1); // upwards

		if (left) {
			/* scan initial columns for tiles in the set */
			for (;;) {
				if (scan_row_column (this->tile, diff_y, this->h, pred)) break;
				this->tile += diff_x;
				this->w--;
				if (this->w == 0) {
					this->Clear();
					return;
				}
			}
		}

		if (right) {
			/* scan final columns for tiles in the set */
			TileIndex t = this->tile + (this->w - 1) * diff_x;
			for (;;) {
				if (scan_row_column (t, diff_y, this->h, pred)) break;
				t -= diff_x;
				this->w--;
				if (this->w == 0) {
					this->Clear();
					return;
				}
			}
		}

		if (bottom) {
			/* scan initial rows for tiles in the set */
			for (;;) {
				if (scan_row_column (this->tile, diff_x, this->w, pred)) break;
				this->tile += diff_y;
				this->h--;
				if (this->h == 0) {
					this->Clear();
					return;
				}
			}
		}

		if (top) {
			/* scan final rows for tiles in the set */
			TileIndex t = this->tile + (this->h - 1) * diff_y;
			for (;;) {
				if (scan_row_column (t, diff_x, this->w, pred)) break;
				t -= diff_y;
				this->h--;
				if (this->h == 0) {
					this->Clear();
					return;
				}
			}
		}
	}

	/**
	 * Shrink the tile area spanned by a set of tiles when tiles are
	 * removed from the set.
	 * @param pred Set predicate.
	 * @param removed Area containing all removed tiles.
	 */
	template <class Pred>
	void shrink_span (Pred pred, const OrthogonalTileArea &removed)
	{
		uint tx = TileX(this->tile);
		uint rx = TileX(removed.tile);
		bool left  = rx <= tx;
		bool right = rx + removed.w >= tx + this->w;

		uint ty = TileY(this->tile);
		uint ry = TileY(removed.tile);
		bool bottom = ry <= ty;
		bool top    = ry + removed.h >= ty + this->h;

		shrink_span (pred, left, right, bottom, top);
	}

	/**
	 * Shrink the tile area spanned by a set of tiles when tiles are
	 * removed from the set.
	 * @param pred Set predicate.
	 */
	template <class Pred>
	void shrink_span (Pred pred)
	{
		shrink_span (pred, true, true, true, true);
	}
};

/** Represents a diagonal tile area. */
struct DiagonalTileArea {
	int a0, b0; ///< Lower left corner in transformed coordinate space
	int a1, b1; ///< Upper right corner in transformed coordinate space

	DiagonalTileArea (TileIndex start, TileIndex end);

	bool Contains (TileIndex tile) const;
};

/** Shorthand for the much more common orthogonal tile area. */
typedef OrthogonalTileArea TileArea;


/** Base class for tile iterators. */
class TileIterator {
protected:
	TileIndex tile; ///< The current tile we are at.

	/**
	 * Initialise the iterator starting at this tile.
	 * @param tile The tile we start iterating from.
	 */
	TileIterator(TileIndex tile) : tile(tile)
	{
	}

	/**
	 * Compute the next tile.
	 */
	virtual void Next() = 0;

public:
	/** Some compilers really like this. */
	virtual ~TileIterator()
	{
	}

	/**
	 * Get the tile we are currently at.
	 * @return The tile we are at, or INVALID_TILE when we're done.
	 */
	inline operator TileIndex () const
	{
		return this->tile;
	}

	/**
	 * Move ourselves to the next tile in the rectangle on the map.
	 */
	TileIterator& operator ++()
	{
		assert(this->tile != INVALID_TILE);
		this->Next();
		return *this;
	}

	/**
	 * Allocate a new iterator that is a copy of this one.
	 */
	virtual TileIterator *Clone() const = 0;
};

/** Iterator to iterate over a tile area (rectangle) of the map. */
class OrthogonalTileIterator : public TileIterator {
private:
	const uint w;       ///< The width of the iterated area.
	const uint rowdiff; ///< The amount to add when switching rows
	uint x;             ///< The current 'x' position in the rectangle.
	uint y;             ///< The current 'y' position in the rectangle.

protected:
	/**
	 * Move ourselves to the next tile in the rectangle on the map.
	 */
	inline void Next() OVERRIDE
	{
		assert(this->tile != INVALID_TILE);

		if (--this->x > 0) {
			this->tile++;
		} else if (--this->y > 0) {
			this->x = this->w;
			this->tile += this->rowdiff;
		} else {
			this->tile = INVALID_TILE;
		}
	}

public:
	/**
	 * Construct the iterator.
	 * @param ta Area, i.e. begin point and width/height of to-be-iterated area.
	 */
	OrthogonalTileIterator(const OrthogonalTileArea &ta)
		: TileIterator(ta.w == 0 || ta.h == 0 ? INVALID_TILE : ta.tile),
			w(ta.w), rowdiff(TileDiffXY(1, 1) - ta.w), x(ta.w), y(ta.h)
	{
	}

	OrthogonalTileIterator *Clone() const OVERRIDE
	{
		return new OrthogonalTileIterator(*this);
	}
};

/** Iterator to iterate over a diagonal area of the map. */
class DiagonalTileIterator : public TileIterator {
private:
	uint x;   ///< x coordinate of the current tile
	uint y;   ///< y coordinate of the current tile
	bool odd; ///< Whether this is an "odd" area
	int8 s1;  ///< Advancing a tile adds (s1,s1)
	int  s2x; ///< Advancing a row adds (s2x,s2y)
	int  s2y; ///< Advancing a row adds (s2x,s2y)
	uint w;   ///< The width of the main rectangle side
	uint n;   ///< The number of tiles left on the current row
	uint m;   ///< The number of rows left

protected:
	void Next() OVERRIDE;

public:
	DiagonalTileIterator(TileIndex begin, TileIndex end);

	DiagonalTileIterator *Clone() const OVERRIDE
	{
		return new DiagonalTileIterator(*this);
	}
};

/**
 * Iterator to perform a circular (spiral) search over a square
 * or around a rectangle.
 */
class CircularTileIterator : public TileIterator {
private:
	uint x, y;
	uint extent[AXIS_END];
	uint j;
	DiagDirection d;
	uint r;

protected:
	void Next() OVERRIDE;

public:
	CircularTileIterator (TileIndex tile, uint size);

	CircularTileIterator (const TileArea &ta, uint radius);

	CircularTileIterator *Clone() const OVERRIDE
	{
		return new CircularTileIterator(*this);
	}
};


/** Base class for iterators that also contain the area they iterate over. */
class TileAreaIterator {
public:
	virtual ~TileAreaIterator() { }

protected:
	virtual TileIterator *GetIterator() = 0;

	const TileIterator *GetIterator() const
	{
		return const_cast<TileAreaIterator*>(this)->GetIterator();
	}

public:
	virtual bool Contains (TileIndex tile) const = 0;

	/**
	 * Get the tile we are currently at.
	 * @return The tile we are at, or INVALID_TILE when we're done.
	 */
	inline operator TileIndex () const
	{
		return *GetIterator();
	}

	/**
	 * Move ourselves to the next tile in the rectangle on the map.
	 */
	TileAreaIterator& operator++()
	{
		++*GetIterator();
		return *this;
	}
};

/** Orthogonal tile area iterator. */
class OrthogonalTileAreaIterator FINAL : public OrthogonalTileArea, public OrthogonalTileIterator, public TileAreaIterator {
public:
	OrthogonalTileAreaIterator (TileIndex begin, TileIndex end)
		: OrthogonalTileArea (begin, end), OrthogonalTileIterator(*static_cast<OrthogonalTileArea*>(this))
	{
	}

	OrthogonalTileIterator *GetIterator() FINAL_OVERRIDE
	{
		return this;
	}

	bool Contains (TileIndex tile) const FINAL_OVERRIDE
	{
		return OrthogonalTileArea::Contains(tile);
	}
};

/** Diagonal tile area iterator. */
class DiagonalTileAreaIterator FINAL : public DiagonalTileArea, public DiagonalTileIterator, public TileAreaIterator {
public:
	DiagonalTileAreaIterator (TileIndex begin, TileIndex end)
		: DiagonalTileArea (begin, end), DiagonalTileIterator (begin, end)
	{
	}

	DiagonalTileIterator *GetIterator() FINAL_OVERRIDE
	{
		return this;
	}

	bool Contains (TileIndex tile) const FINAL_OVERRIDE
	{
		return DiagonalTileArea::Contains(tile);
	}
};


/**
 * A loop which iterates over the tiles of a TileArea.
 * @param var The name of the variable which contains the current tile.
 *            This variable will be allocated in this \c for of this loop.
 * @param ta  The tile area to search over.
 */
#define TILE_AREA_LOOP(var, ta) for (OrthogonalTileIterator var(ta); var != INVALID_TILE; ++var)

#endif /* MAP_TILEAREA_H */
