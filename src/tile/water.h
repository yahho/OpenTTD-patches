/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/water.h Tile functions for water tiles. */

#ifndef TILE_WATER_H
#define TILE_WATER_H

#include "../stdafx.h"
#include "../core/enum_type.hpp"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "../direction_type.h"
#include "../direction_func.h"
#include "../company_type.h"

/** Available water tile types. */
enum WaterTileType {
	WATER_TILE_CLEAR,       ///< nothing (plain water)
	WATER_TILE_COAST,       ///< coast
	WATER_TILE_DEPOT,       ///< ship depot
	WATER_TILE_LOCK_MIDDLE, ///< water lock, middle part
	WATER_TILE_LOCK_LOWER,  ///< water lock, lower part
	WATER_TILE_LOCK_UPPER,  ///< water lock, upper part
};

/** classes of water (for #WATER_TILE_CLEAR water tile type). */
enum WaterClass {
	WATER_CLASS_SEA,     ///< Sea.
	WATER_CLASS_CANAL,   ///< Canal.
	WATER_CLASS_RIVER,   ///< River.
	WATER_CLASS_INVALID, ///< Used for industry tiles on land (also for oilrig if newgrf says so).
};
/** Helper information for extract tool. */
template <> struct EnumPropsT<WaterClass> : MakeEnumPropsT<WaterClass, byte, WATER_CLASS_SEA, WATER_CLASS_INVALID, WATER_CLASS_INVALID, 2> {};


/**
 * Get the water type of a tile
 * @param t The tile whose water type to get
 * @pre tile_is_water(t)
 * @return The water type of the tile
 */
static inline WaterTileType tile_get_water_type(const Tile *t)
{
	assert(tile_is_water(t));
	return (WaterTileType)t->m4;
}

/**
 * Check if a water tile is clear
 * @param t The tile to check
 * @pre tile_is_water(t)
 * @return Whether the tile is clear
 */
static inline bool tile_water_is_clear(const Tile *t)
{
	return tile_get_water_type(t) == WATER_TILE_CLEAR;
}

/**
 * Check if a water tile is a coast tile
 * @param t The tile to check
 * @pre tile_is_water(t)
 * @return Whether the tile is a coast tile
 */
static inline bool tile_water_is_coast(const Tile *t)
{
	return tile_get_water_type(t) == WATER_TILE_COAST;
}

/**
 * Check if a water tile is a ship depot tile
 * @param t The tile to check
 * @pre tile_is_water(t)
 * @return Whether the tile is a ship depot tile
 */
static inline bool tile_water_is_depot(const Tile *t)
{
	return tile_get_water_type(t) == WATER_TILE_DEPOT;
}

/**
 * Check if a water tile is (part of) a lock
 * @param t The tile to check
 * @pre tile_is_water(t)
 * @return Whether the tile is part of a lock
 */
static inline bool tile_water_is_lock(const Tile *t)
{
	return (uint)(tile_get_water_type(t) - WATER_TILE_LOCK_MIDDLE) < 3;
}

/**
 * Check if a tile is a clear water tile
 * @param t The tile to check
 * @return Whether the tile is a clear water tile
 */
static inline bool tile_is_clear_water(const Tile *t)
{
	return tile_is_water(t) && tile_water_is_clear(t);
}

/**
 * Check if a tile is a coast tile
 * @param t The tile to check
 * @return Whether the tile is a coast tile
 */
static inline bool tile_is_coast(const Tile *t)
{
	return tile_is_water(t) && tile_water_is_coast(t);
}

/**
 * Check if a tile is a ship depot tile
 * @param t The tile to check
 * @return Whether the tile is a ship depot tile
 */
static inline bool tile_is_ship_depot(const Tile *t)
{
	return tile_is_water(t) && tile_water_is_depot(t);
}

/**
 * Check if a tile is (part of) a lock
 * @param t The tile to check
 * @return Whether the tile is part of a lock
 */
static inline bool tile_is_lock(const Tile *t)
{
	return tile_is_water(t) && tile_water_is_lock(t);
}


/**
 * Check if a tile has an associated water class
 * @param t The tile to check
 * @return Whether the tile has an associated water class
 */
static inline bool tile_has_water_class(const Tile *t)
{
	return tile_is_object(t) || tile_is_water(t) || tile_is_station(t) || tile_is_industry(t);
}

/**
 * Get the water class of a tile
 * @param t The tile whose water class to get
 * @pre tile_has_water_class(t)
 * @return The water class of the tile
 */
static inline WaterClass tile_get_water_class(const Tile *t)
{
	assert(tile_has_water_class(t));
	return (WaterClass)GB(t->m1, 5, 2);
}

/**
 * Set the water class of a tile
 * @param t The tile whose water class to set
 * @param wc The water class to set
 * @pre tile_has_water_class(t)
 */
static inline void tile_set_water_class(Tile *t, WaterClass wc)
{
	assert(tile_has_water_class(t));
	SB(t->m1, 5, 2, wc);
}

/**
 * Check if a tile is built on water
 * @param t The tile to check
 * @pre tile_has_water_class(t)
 * @return Whether the tile is built on water
 */
static inline bool tile_is_on_water(const Tile *t)
{
	return tile_get_water_class(t) != WATER_CLASS_INVALID;
}


/**
 * Check if a tile is a sea tile
 * @param t The tile to check
 * @pre tile_is_water(t)
 * @return Whether the tile is a sea tile
 */
static inline bool tile_water_is_sea(const Tile *t)
{
	return tile_water_is_clear(t) && tile_get_water_class(t) == WATER_CLASS_SEA;
}

/**
 * Check if a tile is a canal tile
 * @param t The tile to check
 * @pre tile_is_water(t)
 * @return Whether the tile is a canal tile
 */
static inline bool tile_water_is_canal(const Tile *t)
{
	return tile_water_is_clear(t) && tile_get_water_class(t) == WATER_CLASS_CANAL;
}

/**
 * Check if a tile is a river tile
 * @param t The tile to check
 * @pre tile_is_water(t)
 * @return Whether the tile is a river tile
 */
static inline bool tile_water_is_river(const Tile *t)
{
	return tile_water_is_clear(t) && tile_get_water_class(t) == WATER_CLASS_RIVER;
}


/**
 * Get the direction of a ship depot
 * @param t The tile whose depot to get the direction of
 * @pre tile_is_ship_depot(t)
 * @return The direction of the depot
 */
static inline DiagDirection tile_get_ship_depot_direction(const Tile *t)
{
	assert(tile_is_ship_depot(t));
	return (DiagDirection)t->m5;
}


/**
 * Get the direction of a lock
 * @param t The tile whose lock to get the direction of
 * @pre tile_is_lock(t)
 * @return The direction of the lock
 */
static inline DiagDirection tile_get_lock_direction(const Tile *t)
{
	assert(tile_is_lock(t));
	return (DiagDirection)t->m5;
}


/**
 * Make a water tile
 * @param t The tile to make water
 * @param o The owner of the tile
 * @param wc The water class of the tile
 * @param random_bits The random bits for the tile
 */
static inline void tile_make_water(Tile *t, Owner o, WaterClass wc, byte random_bits = 0)
{
	tile_set_type(t, TT_WATER);
	t->m1 = (wc << 5) | o;
	t->m2 = 0;
	t->m3 = random_bits;
	t->m4 = WATER_TILE_CLEAR;
	t->m5 = 0;
	t->m7 = 0;
}

/**
 * Make a sea tile
 * @param t The tile to make sea
 */
static inline void tile_make_sea(Tile *t)
{
	tile_make_water(t, OWNER_WATER, WATER_CLASS_SEA);
}

/**
 * Make a canal tile
 * @param t The tile to make a canal
 * @param o The owner of the canal
 * @param random_bits The random bits for the tile
 */
static inline void tile_make_canal(Tile *t, Owner o, byte random_bits)
{
	assert(o != OWNER_WATER);
	tile_make_water(t, o, WATER_CLASS_CANAL, random_bits);
}

/**
 * Make a river tile
 * @param t The tile to make a river
 * @param random_bits The random bits for the tile
 */
static inline void tile_make_river(Tile *t, byte random_bits)
{
	tile_make_water(t, OWNER_WATER, WATER_CLASS_RIVER, random_bits);
}

/**
 * Make a shore tile
 * @param t The tile to make shore
 */
static inline void tile_make_shore(Tile *t)
{
	tile_set_type(t, TT_WATER);
	t->m1 = (WATER_CLASS_SEA << 5) | OWNER_WATER;
	t->m2 = 0;
	t->m3 = 0;
	t->m4 = WATER_TILE_COAST;
	t->m5 = 0;
	t->m7 = 0;
}

/**
 * Make a ship depot tile
 * @param t The tile to make a ship depot
 * @param o The owner of the depot
 * @param id The depot id
 * @param dir The direction this depot part will face
 * @param wc The original water class
 */
static inline void tile_make_ship_depot(Tile *t, Owner o, uint id, DiagDirection dir, WaterClass wc)
{
	t->m0 = TT_WATER << 4;
	t->m1 = (wc << 5) | o;
	t->m2 = id;
	t->m3 = 0;
	t->m4 = WATER_TILE_DEPOT;
	t->m5 = dir;
	t->m7 = 0;
}

/**
 * Make a lock tile
 * @param t The tile to make a lock
 * @param o The owner of the lock
 * @param part The lock part
 * @param dir The orientation of this part of the lock
 * @param wc The original water class
 */
static inline void tile_make_lock(Tile *t, Owner o, WaterTileType part, DiagDirection dir, WaterClass wc)
{
	t->m0 = TT_WATER << 4;
	t->m1 = (wc << 5) | o;
	t->m2 = 0;
	t->m3 = 0;
	t->m4 = part;
	t->m5 = dir;
	t->m7 = 0;
}

#endif /* TILE_WATER_H */
