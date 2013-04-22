/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/ground.h Tile functions for ground tiles. */

#ifndef TILE_GROUND_H
#define TILE_GROUND_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "common.h"
#include "../direction_type.h"
#include "../company_type.h"

/**
 * Ground types. Valid densities in comments after the enum.
 */
enum Ground {
	GROUND_GRASS  = 0, ///< 0-3
	GROUND_SHORE  = 1, ///< 3
	GROUND_ROUGH  = 2, ///< 3
	GROUND_ROCKS  = 3, ///< 3
	GROUND_DESERT = 4, ///< 1,3
	GROUND_SNOW        =  8, ///< 0-3
	GROUND_SNOW_ROUGH  = 10, ///< 0-3
	GROUND_SNOW_ROCKS  = 11, ///< 0-3
};


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
 * Get the update counter of a ground tile
 * @param t The tile whose counter to get
 * @pre tile_is_ground(t)
 * @return The update counter of the tile
 */
static inline uint tile_get_clear_counter(const Tile *t)
{
	assert(tile_is_ground(t));
	return GB(t->m3, 0, 4);
}

/**
 * Set the update counter of a ground tile
 * @param t The tile whose counter to set
 * @param c The value to set the counter to
 * @pre tile_is_ground(t)
 */
static inline void tile_set_clear_counter(Tile *t, uint c)
{
	assert(tile_is_ground(t));
	SB(t->m3, 0, 4, c);
}

/**
 * Increment the update counter of a ground tile
 * @param t The tile whose counter to change
 * @param c The value to add to the counter
 * @pre tile_is_ground(t)
 */
static inline void tile_add_clear_counter(Tile *t, uint c)
{
	assert(tile_is_ground(t));
	t->m3 += c;
}


/**
 * Get the full ground type of a tile
 * @param t The tile whose ground type to get
 * @pre tile_is_clear(t) || tile_is_trees(t)
 * @return The full ground type of the tile
 */
static inline Ground tile_get_full_ground(const Tile *t)
{
	assert(tile_is_clear(t) || tile_is_trees(t));
	return (Ground)GB(t->m3, 4, 4);
}

/**
 * Get the raw ground type of a tile, ignoring snow
 * @param t The tile whose ground type to get
 * @pre tile_is_clear(t) || tile_is_trees(t)
 * @return The raw ground type of the tile
 */
static inline Ground tile_get_raw_ground(const Tile *t)
{
	assert(tile_is_clear(t) || tile_is_trees(t));
	return (Ground)GB(t->m3, 4, 3);
}

/**
 * Get the ground type of a tile, treating all snow types as equal
 * @param t The tile whose ground type to get
 * @pre tile_is_clear(t) || tile_is_trees(t)
 * @return The ground type of the tile
 */
static inline Ground tile_get_ground(const Tile *t)
{
	Ground g = tile_get_full_ground(t);
	return (g >= GROUND_SNOW) ? GROUND_SNOW : g;
}

/**
 * Check if a ground tile is covered with snow
 * @param t The tile to check
 * @pre tile_is_ground(t)
 * @return Whether the tile is covered with snow
 */
static inline bool tile_ground_has_snow(const Tile *t)
{
	assert(tile_is_ground(t));
	return !tile_is_subtype(t, TT_GROUND_FIELDS) && tile_get_full_ground(t) >= GROUND_SNOW;
}

/**
 * Check if a tile has a given ground type
 * @param t The tile to check
 * @param g The ground type to check against
 * @pre tile_is_clear(t) || tile_is_trees(t)
 * @return Whether the tile has the given ground type
 */
static inline bool tile_is_ground(const Tile *t, Ground g)
{
	return tile_get_ground(t) == g;
}


/**
 * Get the density of a non-field ground tile
 * @param t The tile whose density to get
 * @pre tile_is_clear(t) || tile_is_trees(t)
 * @return The density of the tile
 */
static inline uint tile_get_density(const Tile *t)
{
	assert(tile_is_clear(t) || tile_is_trees(t));
	return t->m4;
}

/**
 * Change the density of a non-field ground tile
 * @param t The tile whose density to change
 * @param d Increment to apply to the density
 * @pre tile_is_clear(t) || tile_is_trees(t)
 */
static inline void tile_add_density(Tile *t, int d)
{
	assert(tile_is_clear(t) || tile_is_trees(t));
	t->m4 += d;
}


/**
 * Set the ground type and density of a tile in one go, optionally resetting the counter.
 * @param t The tile whose type and density to set
 * @param g Ground to set
 * @param d density to set
 * @param keep_counter Whether to keep the update counter
 * @pre tile_is_clear(t) || tile_is_trees(t)
 */
static inline void tile_set_ground_density(Tile *t, Ground g, uint d, bool keep_counter = false)
{
	assert(tile_is_clear(t) || tile_is_trees(t));
	if (keep_counter) {
		SB(t->m3, 4, 4, g);
	} else {
		t->m3 = g << 4;
	}
	t->m4 = d;
}


/**
 * Get the tree type of a tile
 * @param t The tile whose tree type to get
 * @return The tree type of the tile
 */
static inline TreeType tile_get_tree_type(const Tile *t)
{
	assert(tile_is_trees(t));
	return (TreeType)t->m7;
}

/**
 * Get the number of trees on a tile
 * @param t The tile whose tree count to get
 * @return The number of trees on the tile (1--4)
 */
static inline uint tile_get_tree_count(const Tile *t)
{
	assert(tile_is_trees(t));
	return GB(t->m5, 6, 2) + 1;
}

/**
 * Increment/decrement the number of trees on a tile
 * @param t The tile whose tree count to change
 * @param c The amount to add to the count of trees
 * @note This function cannot be used to remove all trees from a tile
 */
static inline void tile_add_tree_count(Tile *t, int c)
{
	assert(tile_is_trees(t));
	t->m5 += c << 6;
}

/**
 * Get the tree growth status of a tile
 * @param t The tile whose tree growth status to get
 * @return The tree growth status of the tile
 */
static inline uint tile_get_tree_growth(const Tile *t)
{
	assert(tile_is_trees(t));
	return GB(t->m5, 0, 3);
}

/**
 * Set the tree growth status of a tile
 * @param t The tile whose tree growth status to set
 * @param g The tree growth status to set
 */
static inline void tile_set_tree_growth(Tile *t, uint g)
{
	assert(tile_is_trees(t));
	SB(t->m5, 0, 3, g);
}

/**
 * Increment/decrement the tree growth status of a tile
 * @param t The tile whose tree growth status to set
 * @param c The amount to add to the tree growth status
 */
static inline void tile_add_tree_growth(Tile *t, int c)
{
	assert(tile_is_trees(t));
	t->m5 += c;
}


/**
 * Get the field type (production stage) of a tile
 * @param t The tile whose field type to get
 * @pre tile_is_fields(t)
 * @return The field type of the tile
 */
static inline uint tile_get_field_type(const Tile *t)
{
	assert(tile_is_fields(t));
	return GB(t->m3, 4, 4);
}

/**
 * Set the field type (production stage) of a tile
 * @param t The tile whose field type to set
 * @param f The field type to set
 * @pre tile_is_fields(t)
 */
static inline void tile_set_field_type(Tile *t, uint f)
{
	assert(tile_is_fields(t));
	SB(t->m3, 4, 4, f);
}

/**
 * Get the industry index of a field tile
 * @param t The tile whose industry index to get
 * @pre tile_is_fields(t)
 * @return The index of the industry that made the field
 */
static inline uint tile_get_field_industry(const Tile *t)
{
	assert(tile_is_fields(t));
	return t->m2;
}

/**
 * Set the industry index of a field tile
 * @param t The tile whose industry index to set
 * @param i The index of the industry to set
 * @pre tile_is_fields(t)
 */
static inline void tile_set_field_industry(Tile *t, uint i)
{
	assert(tile_is_fields(t));
	t->m2 = i;
}

/**
 * Get the fence of a field tile at a given border
 * @param t The tile whose fence to get
 * @param side The border of the tile
 * @pre tile_is_fields(t)
 * @return The fence at the given side, or 0 for none
 */
static inline uint tile_get_field_fence(const Tile *t, DiagDirection side)
{
	assert(tile_is_fields(t));
	switch (side) {
		default: NOT_REACHED();
		case DIAGDIR_SE: return GB(t->m4, 2, 3);
		case DIAGDIR_SW: return GB(t->m4, 5, 3);
		case DIAGDIR_NE: return GB(t->m5, 5, 3);
		case DIAGDIR_NW: return GB(t->m5, 2, 3);
	}
}

/**
 * Set the fence of a field tile at a given border
 * @param t The tile whose fence to set
 * @param side The border of the tile
 * @param h The fence to set, or 0 for none
 * @pre tile_is_field(t)
 */
static inline void tile_set_field_fence(Tile *t, DiagDirection side, uint h)
{
	assert(tile_is_fields(t));
	switch (side) {
		default: NOT_REACHED();
		case DIAGDIR_SE: SB(t->m4, 2, 3, h); break;
		case DIAGDIR_SW: SB(t->m4, 5, 3, h); break;
		case DIAGDIR_NE: SB(t->m5, 5, 3, h); break;
		case DIAGDIR_NW: SB(t->m5, 2, 3, h); break;
	}
}


/**
 * Make a void tile
 * @param t The tile to make void
 */
static inline void tile_make_void(Tile *t)
{
	assert_compile(TT_GROUND == 0);
	assert_compile(TT_GROUND_VOID == 0);
	memset(t, 0, sizeof(Tile));
}

/**
 * Make a clear tile
 * @param t The tile to make clear
 * @param g The ground type
 * @param d The density
 */
static inline void tile_make_clear(Tile *t, Ground g, uint d = 0)
{
	/* It this was a non-bridgeable tile, also clear the bridge bits. */
	if (tile_is_bridgeable(t)) {
		tile_set_type(t, TT_GROUND);
	} else {
		t->m0 = TT_GROUND << 4;
	}

	t->m1 = (TT_GROUND_CLEAR << 6) | OWNER_NONE;
	t->m2 = 0;
	t->m3 = g << 4;
	t->m4 = d;
	t->m5 = 0;
	t->m7 = 0;
}

/**
 * Make a tree tile
 * @param t The tile to make a tree tile
 * @param tt The type of the trees
 * @param count The number of trees
 * @param growth The growth status of the trees
 * @param ground The ground type of the tile
 * @param density The density of the ground
 */
static inline void tile_make_trees(Tile *t, TreeType tt, uint count, uint growth, Ground ground, uint density)
{
	t->m0 = TT_GROUND << 4;
	t->m1 = (TT_GROUND_TREES << 6) | OWNER_NONE;
	t->m2 = 0;
	t->m3 = ground << 4;
	t->m4 = density;
	t->m5 = count << 6 | growth;
	t->m7 = tt;
}

/**
 * Make a (farm) field tile
 * @param t The tile to make a field
 * @param field_type The 'growth' level of the field
 * @param i The industry that is planting this field
 */
static inline void tile_make_field(Tile *t, uint field_type, uint i)
{
	tile_set_type(t, TT_GROUND);
	t->m1 = (TT_GROUND_FIELDS << 6) | OWNER_NONE;
	t->m2 = i;
	t->m3 = field_type << 4;
	t->m4 = 0;
	t->m5 = 0;
	t->m7 = 0;
}

/**
 * Make a ground tile snowy
 * @param t The tile to add snow to
 * @param d Density of snow
 */
static inline void tile_make_snow(Tile *t, uint d = 0)
{
	assert(tile_is_ground(t));

	if (tile_is_subtype(t, TT_GROUND_FIELDS)) {
		tile_make_clear(t, GROUND_SNOW, d);
	} else {
		tile_set_ground_density(t, (Ground)(tile_get_full_ground(t) | GROUND_SNOW), d, true);
	}
}

/**
 * Clear snow from a tile
 * @param t The tile to remove the snow from
 */
static inline void tile_clear_snow(Tile *t)
{
	assert(tile_ground_has_snow(t));
	tile_set_ground_density(t, (Ground)(tile_get_full_ground(t) & ~GROUND_SNOW), 3, true);
}

#endif /* TILE_GROUND_H */
