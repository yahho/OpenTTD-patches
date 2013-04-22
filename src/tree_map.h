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

#include "tile/ground.h"
#include "tile_map.h"
#include "clear_map.h"

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
	return tile_get_tree_type(&_mc[t]);
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
	return tile_get_tree_count(&_mc[t]);
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
	tile_add_tree_count(&_mc[t], c);
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
	return tile_get_tree_growth(&_mc[t]);
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
	tile_add_tree_growth(&_mc[t], a);
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
	tile_set_tree_growth(&_mc[t], g);
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
	tile_make_trees(&_mc[t], type, count, growth, ground, density);
}

#endif /* TREE_MAP_H */
