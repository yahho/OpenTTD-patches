/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/tileset.h Sets of objects on the map. */

#ifndef MAP_TILESET_H
#define MAP_TILESET_H

#include <vector>
#include <set>

#include "coord.h"
#include "../core/forward_list.h"

/**
 * Base class for a set of objects on the map, arranged for fast searching
 * around a given tile.
 */
struct TileSetBase {
	/* Keep track of all defined tilesets. */
	static const uint TILESET_COUNT_MAX = 2; // town and industry tilesets
	static uint tileset_count;
	static TileSetBase *tilesets [TILESET_COUNT_MAX];

	/** Construct a tileset. */
	TileSetBase()
	{
		assert (tileset_count < TILESET_COUNT_MAX);
		tilesets[tileset_count++] = this;
	}

	/** Destruct a tileset. */
	virtual ~TileSetBase()
	{
		/* All tilesets currently used in the code are static, so
		 * we do nothing here (other than a basic sanity check). */
		tileset_count = UINT_MAX; // special flag value
	}

	/** Divide the map in squares of this size; must be a power of 2. */
	static const uint BLOCK_SIZE = 64;

	/** Compute the size to be used for the bucket vector. */
	static uint get_vector_size()
	{
		return MapSize() / (BLOCK_SIZE * BLOCK_SIZE);
	}

	/** Compute the bucket index to use for a tile. */
	static uint get_vector_index (uint x, uint y)
	{
		return (x / BLOCK_SIZE) + (y / BLOCK_SIZE) * (MapSizeX() / BLOCK_SIZE);
	}

	/** Compute the bucket index to use for a tile. */
	static uint get_vector_index (TileIndex tile)
	{
		return get_vector_index (TileX(tile), TileY(tile));
	}

	/** Iterator over a square area of tiles. */
	struct AreaIterator {
		uint width;   ///< the width along x of the iterated area
		uint m;       ///< number of rows left
		uint n;       ///< number of tiles left on the current row
		uint k;       ///< current bucket index

		AreaIterator (TileIndex tile, uint radius)
		{
			uint x = TileX(tile);
			uint x0 = (x > radius) ? (x - radius) / BLOCK_SIZE : 0;
			uint x1 = ((MapSizeX() - x > radius) ? x + radius : MapMaxX()) / BLOCK_SIZE;
			width = n = x1 - x0;

			uint y = TileY(tile);
			uint y0 = (y > radius) ? (y - radius) / BLOCK_SIZE : 0;
			uint y1 = ((MapSizeY() - y > radius) ? y + radius : MapMaxY()) / BLOCK_SIZE;
			m = y1 - y0;

                        k = x0 + y0 * (MapSizeX() / BLOCK_SIZE);
		}

		uint get_index (void) const
		{
			return k;
		}

		bool next (void)
		{
			if (n > 0) {
				k++;
				n--;
				return true;
			} else if (m > 0) {
				k += MapSizeX() / BLOCK_SIZE - width;
				m--;
				n = width;
				return true;
			} else {
				return false;
			}
		}
	};

	/** Reset the set after the map has changed. */
	virtual void reset (void) = 0;

	/** Reset all sets after the map has changed. */
	static void reset_all (void)
	{
		assert (tileset_count <= TILESET_COUNT_MAX);
		for (uint i = 0; i < tileset_count; i++) {
			tilesets[i]->reset();
		}
	}
};

/* Forward declarations. */
template <typename T, TileIndex (*get_tile) (const T*)> struct TileSetObject;
template <typename T, TileIndex (*get_tile) (const T*)> struct TileSet;

/** An object that becomes part of a tile set. */
template <typename T, TileIndex (*get_tile) (const T*)>
struct TileSetObject : ForwardListLink <T, TileSetObject<T, get_tile> > {
	typedef ::TileSet <T, get_tile> TileSet;

	static TileSet set; ///< set of objects

	/** Add this element to underlying set. */
	void add_to_tileset (void)
	{
		set.add (static_cast<T*>(this));
	}

	/** Remove this element from underlying set. */
	void remove_from_tileset (void)
	{
		set.remove (static_cast<T*>(this));
	}

	/** Test if there is any item in the set that is within a given distance of a given tile. */
	template <uint metric (TileIndex, TileIndex)>
	static bool find_any (TileIndex tile, uint threshold)
	{
		return set.find_any<metric> (tile, threshold);
	}

	/** Find the item in the set that is closest to a tile, within a threshold. */
	template <uint metric (TileIndex, TileIndex)>
	static T *find_closest (TileIndex tile, uint threshold)
	{
		return set.find_closest<metric> (tile, threshold);
	}
};

/**
 * A set of objects on the map, arranged for fast searching of objects that
 * are close to a given tile.
 */
template <typename T, TileIndex (*get_tile) (const T*)>
struct TileSet : TileSetBase {
	typedef ForwardList <T, false, TileSetObject <T, get_tile> > Bucket;

	std::vector <Bucket> buckets;

	/** Reset the set after the map size has changed. */
	void reset (void) FINAL_OVERRIDE
	{
		std::vector<Bucket> newv (get_vector_size());
		buckets.swap (newv);
	}

	/** Compute the bucket to use for an item. */
	Bucket *get_bucket (const T *item)
	{
		return &buckets[TileSetBase::get_vector_index (get_tile (item))];
	}

	/** Add an item to the set. */
	void add (T *item)
	{
		get_bucket(item)->prepend(item);
	}

	/** Remove an item from the set. */
	void remove (T *item)
	{
		get_bucket(item)->remove(item);
	}

	/** Iterator over the items in a square area of tiles. */
	struct Iterator {
		AreaIterator area_iter;
		TileSet *const set;
		typename Bucket::iterator item;

		Bucket *get_current_bucket (void)
		{
			return &set->buckets[area_iter.get_index()];
		}

		Iterator (TileSet *set, TileIndex tile, uint radius)
			: area_iter (tile, radius), set(set)
		{
			do {
				item = get_current_bucket()->begin();
			} while ((item == get_current_bucket()->end()) && area_iter.next());
		}

		T *get_item (void) const
		{
			return &*item;
		}

		void next (void)
		{
			item++;
			while ((item == get_current_bucket()->end()) && area_iter.next()) {
				item = get_current_bucket()->begin();
			}
		}
	};

	/** Test if there is any item in the set that is within a given distance of a given tile. */
	template <uint metric (TileIndex, TileIndex)>
	bool find_any (TileIndex tile, uint threshold)
	{
		for (Iterator iter (this, tile, threshold); iter.get_item() != NULL; iter.next()) {
			if (metric (tile, get_tile(iter.get_item())) <= threshold) return true;
		}
		return false;
	}

	/** Find the item in the set that is closest to a tile, within a threshold. */
	template <uint metric (TileIndex, TileIndex)>
	T *find_closest (TileIndex tile, uint threshold)
	{
		uint best_distance = threshold + 1;
		assert (best_distance != 0); // no wraparound please
		T *best_item = NULL;

		for (Iterator iter (this, tile, threshold); iter.get_item() != NULL; iter.next()) {
			uint dist = metric (tile, get_tile(iter.get_item()));
			if (dist < best_distance) {
				best_distance = dist;
				best_item = iter.get_item();
			}
		}
		return best_item;
	}
};

template <typename T, TileIndex (*get_tile) (const T*)>
TileSet<T, get_tile> TileSetObject<T, get_tile>::set;

#endif /* MAP_TILESET_H */
