/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tree_map.h Map accessors for tree tiles. */

#ifndef TREE_MAP_H
#define TREE_MAP_H

#include "tile_map.h"
#include "clear_map.h"

/**
 * List of tree types along all landscape types.
 *
 * This enumeration contains a list of the different tree types along
 * all landscape types. The values for the enumerations may be used for
 * offsets from the grfs files. These points to the start of
 * the tree list for a landscape. See the TREE_COUNT_* enumerations
 * for the amount of different trees for a specific landscape.
 */
enum TreeType {
	TREE_TEMPERATE    = 0x00, ///< temperate tree
	TREE_SUB_ARCTIC   = 0x0C, ///< tree on a sub_arctic landscape
	TREE_RAINFOREST   = 0x14, ///< tree on the 'green part' on a sub-tropical map
	TREE_CACTUS       = 0x1B, ///< a cactus for the 'desert part' on a sub-tropical map
	TREE_SUB_TROPICAL = 0x1C, ///< tree on a sub-tropical map, non-rainforest, non-desert
	TREE_TOYLAND      = 0x20, ///< tree on a toyland map
	TREE_INVALID      = 0xFF, ///< An invalid tree
};

/* Counts the number of tree types for each landscape.
 *
 * This list contains the counts of different tree types for each landscape. This list contains
 * 5 entries instead of 4 (as there are only 4 landscape types) as the sub tropic landscape
 * has two types of area, one for normal trees and one only for cacti.
 */
static const uint TREE_COUNT_TEMPERATE    = TREE_SUB_ARCTIC - TREE_TEMPERATE;    ///< number of tree types on a temperate map.
static const uint TREE_COUNT_SUB_ARCTIC   = TREE_RAINFOREST - TREE_SUB_ARCTIC;   ///< number of tree types on a sub arctic map.
static const uint TREE_COUNT_RAINFOREST   = TREE_CACTUS     - TREE_RAINFOREST;   ///< number of tree types for the 'rainforest part' of a sub-tropic map.
static const uint TREE_COUNT_SUB_TROPICAL = TREE_TOYLAND    - TREE_SUB_TROPICAL; ///< number of tree types for the 'sub-tropic part' of a sub-tropic map.
static const uint TREE_COUNT_TOYLAND      = 9;                                   ///< number of tree types on a toyland map.


/**
 * Returns the treetype of a tile.
 *
 * This function returns the treetype of a given tile. As there are more
 * possible treetypes for a tile in a game as the enumeration #TreeType defines
 * this function may be return a value which isn't catch by an entry of the
 * enumeration #TreeType. But there is no problem known about it.
 *
 * @param t The tile to get the treetype from
 * @return The treetype of the given tile with trees
 * @pre IsTreeTile(t)
 */
static inline TreeType GetTreeType(TileIndex t)
{
	assert(IsTreeTile(t));
	return (TreeType)_mc[t].m7;
}

/**
 * Set the density and ground type of a tile with trees.
 *
 * This functions saves the ground type and the density which belongs to it
 * for a given tile.
 *
 * @param t The tile to set the density and ground type
 * @param g The ground type to save
 * @param d The density to save with
 * @pre IsTreeTile(t)
 */
static inline void SetTreeGroundDensity(TileIndex t, Ground g, uint d)
{
	assert(IsTreeTile(t)); // XXX incomplete
	SB(_mc[t].m3, 4, 4, g);
	SB(_mc[t].m4, 0, 2, d);
}

/**
 * Returns the number of trees on a tile.
 *
 * This function returns the number of trees of a tile (1-4).
 * The tile must contain at least one tree.
 *
 * @param t The index to get the number of trees
 * @return The number of trees (1-4)
 * @pre IsTreeTile(t)
 */
static inline uint GetTreeCount(TileIndex t)
{
	assert(IsTreeTile(t));
	return GB(_mc[t].m5, 6, 2) + 1;
}

/**
 * Add a amount to the tree-count value of a tile with trees.
 *
 * This function add a value to the tree-count value of a tile. This
 * value may be negative to reduce the tree-counter. If the resulting
 * value reach 0 it doesn't get converted to a "normal" tile.
 *
 * @param t The tile to change the tree amount
 * @param c The value to add (or reduce) on the tree-count value
 * @pre IsTreeTile(t)
 */
static inline void AddTreeCount(TileIndex t, int c)
{
	assert(IsTreeTile(t)); // XXX incomplete
	_mc[t].m5 += c << 6;
}

/**
 * Returns the tree growth status.
 *
 * This function returns the tree growth status of a tile with trees.
 *
 * @param t The tile to get the tree growth status
 * @return The tree growth status
 * @pre IsTreeTile(t)
 */
static inline uint GetTreeGrowth(TileIndex t)
{
	assert(IsTreeTile(t));
	return GB(_mc[t].m5, 0, 3);
}

/**
 * Add a value to the tree growth status.
 *
 * This function adds a value to the tree grow status of a tile.
 *
 * @param t The tile to add the value on
 * @param a The value to add on the tree growth status
 * @pre IsTreeTile(t)
 */
static inline void AddTreeGrowth(TileIndex t, int a)
{
	assert(IsTreeTile(t)); // XXX incomplete
	_mc[t].m5 += a;
}

/**
 * Sets the tree growth status of a tile.
 *
 * This function sets the tree growth status of a tile directly with
 * the given value.
 *
 * @param t The tile to change the tree growth status
 * @param g The new value
 * @pre IsTreeTile(t)
 */
static inline void SetTreeGrowth(TileIndex t, uint g)
{
	assert(IsTreeTile(t)); // XXX incomplete
	SB(_mc[t].m5, 0, 3, g);
}

/**
 * Get the tick counter of a tree tile.
 *
 * Returns the saved tick counter of a given tile.
 *
 * @param t The tile to get the counter value from
 * @pre IsTreeTile(t)
 */
static inline uint GetTreeCounter(TileIndex t)
{
	assert(IsTreeTile(t));
	return GB(_mc[t].m3, 0, 4);
}

/**
 * Add a value on the tick counter of a tree-tile
 *
 * This function adds a value on the tick counter of a tree-tile.
 *
 * @param t The tile to add the value on
 * @param a The value to add on the tick counter
 * @pre IsTreeTile(t)
 */
static inline void AddTreeCounter(TileIndex t, int a)
{
	assert(IsTreeTile(t)); // XXX incomplete
	_mc[t].m3 += a;
}

/**
 * Set the tick counter for a tree-tile
 *
 * This function sets directly the tick counter for a tree-tile.
 *
 * @param t The tile to set the tick counter
 * @param c The new tick counter value
 * @pre IsTreeTile(t)
 */
static inline void SetTreeCounter(TileIndex t, uint c)
{
	assert(IsTreeTile(t)); // XXX incomplete
	SB(_mc[t].m3, 0, 4, c);
}

/**
 * Make a tree-tile.
 *
 * This functions change the tile to a tile with trees and all informations which belongs to it.
 *
 * @param t The tile to make a tree-tile from
 * @param type The type of the tree
 * @param count the number of trees
 * @param growth the growth status
 * @param ground the ground type
 * @param density the density (not the number of trees)
 */
static inline void MakeTree(TileIndex t, TreeType type, uint count, uint growth, Ground ground, uint density)
{
	SetTileTypeSubtype(t, TT_GROUND, TT_GROUND_TREES);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, OWNER_NONE);
	_mc[t].m2 = 0;
	_mc[t].m3 = ground << 4;
	_mc[t].m4 = density;
	_mc[t].m5 = count << 6 | growth;
	_mc[t].m7 = type;
}

#endif /* TREE_MAP_H */
