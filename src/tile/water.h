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

/**
 * Bit field layout of m5 for water tiles.
 */
enum WaterTileTypeBitLayout {
	WBL_TYPE_BEGIN        = 4,   ///< Start of the 'type' bitfield.
	WBL_TYPE_COUNT        = 4,   ///< Length of the 'type' bitfield.

	WBL_TYPE_NORMAL       = 0x0, ///< Clear water or coast ('type' bitfield).
	WBL_TYPE_LOCK         = 0x1, ///< Lock ('type' bitfield).
	WBL_TYPE_DEPOT        = 0x8, ///< Depot ('type' bitfield).

	WBL_COAST_FLAG        = 0,   ///< Flag for coast.

	WBL_LOCK_ORIENT_BEGIN = 0,   ///< Start of lock orientiation bitfield.
	WBL_LOCK_ORIENT_COUNT = 2,   ///< Length of lock orientiation bitfield.
	WBL_LOCK_PART_BEGIN   = 2,   ///< Start of lock part bitfield.
	WBL_LOCK_PART_COUNT   = 2,   ///< Length of lock part bitfield.

	WBL_DEPOT_PART        = 0,   ///< Depot part flag.
	WBL_DEPOT_AXIS        = 1,   ///< Depot axis flag.
};

/** Available water tile types. */
enum WaterTileType {
	WATER_TILE_CLEAR, ///< Plain water.
	WATER_TILE_COAST, ///< Coast.
	WATER_TILE_LOCK,  ///< Water lock.
	WATER_TILE_DEPOT, ///< Water Depot.
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

/** Sections of the water depot. */
enum DepotPart {
	DEPOT_PART_NORTH = 0, ///< Northern part of a depot.
	DEPOT_PART_SOUTH = 1, ///< Southern part of a depot.
	DEPOT_PART_END
};

/** Sections of the water lock. */
enum LockPart {
	LOCK_PART_MIDDLE = 0, ///< Middle part of a lock.
	LOCK_PART_LOWER  = 1, ///< Lower part of a lock.
	LOCK_PART_UPPER  = 2, ///< Upper part of a lock.
};


/**
 * Get the water type of a tile
 * @param t The tile whose water type to get
 * @pre tile_is_water(t)
 * @return The water type of the tile
 */
static inline WaterTileType tile_get_water_type(const Tile *t)
{
	assert(tile_is_water(t));

	switch (GB(t->m5, WBL_TYPE_BEGIN, WBL_TYPE_COUNT)) {
		case WBL_TYPE_NORMAL: return HasBit(t->m5, WBL_COAST_FLAG) ? WATER_TILE_COAST : WATER_TILE_CLEAR;
		case WBL_TYPE_LOCK:   return WATER_TILE_LOCK;
		case WBL_TYPE_DEPOT:  return WATER_TILE_DEPOT;
		default: NOT_REACHED();
	}
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
	return tile_get_water_type(t) == WATER_TILE_LOCK;
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
 * Get the axis of a ship depot
 * @param t The tile whose depot to get the axis of
 * @pre tile_is_ship_depot(t)
 * @return The axis of the depot
 */
static inline Axis tile_get_ship_depot_axis(const Tile *t)
{
	assert(tile_is_ship_depot(t));
	return (Axis)GB(t->m5, WBL_DEPOT_AXIS, 1);
}

/**
 * Get the part of a ship depot
 * @param t The tile whose depot to get the part of
 * @pre tile_is_ship_depot(t)
 * @return The part of the depot
 */
static inline DepotPart tile_get_ship_depot_part(const Tile *t)
{
	assert(tile_is_ship_depot(t));
	return (DepotPart)GB(t->m5, WBL_DEPOT_PART, 1);
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
	return XYNSToDiagDir(tile_get_ship_depot_axis(t), tile_get_ship_depot_part(t));
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
	return (DiagDirection)GB(t->m5, WBL_LOCK_ORIENT_BEGIN, WBL_LOCK_ORIENT_COUNT);
}

/**
 * Get the part of a lock
 * @param t The tile whose lock to get the part of
 * @pre tile_is_lock(t)
 * @return The part of the lock
 */
static inline byte tile_get_lock_part(const Tile *t)
{
	assert(tile_is_lock(t));
	return GB(t->m5, WBL_LOCK_PART_BEGIN, WBL_LOCK_PART_COUNT);
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
	t->m4 = 0;
	t->m5 = WBL_TYPE_NORMAL << WBL_TYPE_BEGIN;
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
	t->m4 = 0;
	t->m5 = WBL_TYPE_NORMAL << WBL_TYPE_BEGIN | 1 << WBL_COAST_FLAG;
	t->m7 = 0;
}

/**
 * Make a ship depot tile
 * @param t The tile to make a ship depot
 * @param o The owner of the depot
 * @param id The depot id
 * @param part The depot part (either #DEPOT_PART_NORTH or #DEPOT_PART_SOUTH).
 * @param a The axis of the depot
 * @param wc The original water class
 */
static inline void tile_make_ship_depot(Tile *t, Owner o, uint id, DepotPart part, Axis a, WaterClass wc)
{
	t->m0 = TT_WATER << 4;
	t->m1 = (wc << 5) | o;
	t->m2 = id;
	t->m3 = 0;
	t->m4 = 0;
	t->m5 = WBL_TYPE_DEPOT << WBL_TYPE_BEGIN | part << WBL_DEPOT_PART | a << WBL_DEPOT_AXIS;
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
static inline void tile_make_lock(Tile *t, Owner o, LockPart part, DiagDirection dir, WaterClass wc)
{
	t->m0 = TT_WATER << 4;
	t->m1 = (wc << 5) | o;
	t->m2 = 0;
	t->m3 = 0;
	t->m4 = 0;
	t->m5 = WBL_TYPE_LOCK << WBL_TYPE_BEGIN | part << WBL_LOCK_PART_BEGIN | dir << WBL_LOCK_ORIENT_BEGIN;
	t->m7 = 0;
}

#endif /* TILE_WATER_H */
