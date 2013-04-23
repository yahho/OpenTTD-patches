/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/object.h Tile functions for object tiles. */

#ifndef TILE_OBJECT_H
#define TILE_OBJECT_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "water.h"
#include "../company_type.h"
#include "../object_type.h"

/**
 * Get the type of object at a tile
 * @param t The tile whose object type to get
 * @pre tile_is_object(t)
 * @return The type of the object at the tile
 */
static inline ObjectType tile_get_object_type(const Tile *t)
{
	assert(tile_is_object(t));
	return (ObjectType)t->m5;
}

/**
 * Get the index of the object at a tile
 * @param t The tile whose object index to get
 * @pre tile_is_object(t)
 * @return The index of the object at the tile
 */
static inline ObjectID tile_get_object_index(const Tile *t)
{
	assert(tile_is_object(t));
	return (ObjectID)t->m2;
}


/**
 * Check if a tile has a transmitter
 * @param t The tile to check
 * @return Whether there is a transmitter at the tile
 */
static inline bool tile_is_transmitter(const Tile *t)
{
	return tile_is_object(t) && tile_get_object_type(t) == OBJECT_TRANSMITTER;
}

/**
 * Check if an object tile is owned land
 * @param t The tile to check
 * @pre tile_is_object(t)
 * @return Whether the tile is owned land
 */
static inline bool tile_object_is_owned_land(const Tile *t)
{
	assert(tile_is_object(t));
	return tile_get_object_type(t) == OBJECT_OWNED_LAND;
}

/**
 * Check if a tile is owned land
 * @param t The tile to check
 * @return Whether the tile is owned land
 */
static inline bool tile_is_owned_land(const Tile *t)
{
	return tile_is_object(t) && tile_object_is_owned_land(t);
}

/**
 * Check if an object tile is a headquarters tile
 * @param t The tile to check
 * @pre tile_is_object(t)
 * @return Whether the tile is a headquarters tile
 */
static inline bool tile_object_is_hq(const Tile *t)
{
	assert(tile_is_object(t));
	return tile_get_object_type(t) == OBJECT_HQ;
}

/**
 * Check if an object tile is a statue
 * @param t The tile to check
 * @pre tile_is_object(t)
 * @return Whether the tile is a statue
 */
static inline bool tile_object_is_statue(const Tile *t)
{
	assert(tile_is_object(t));
	return tile_get_object_type(t) == OBJECT_STATUE;
}

/**
 * Check if a tile is a statue
 * @param t The tile to check
 * @return Whether the tile is a statue
 */
static inline bool tile_is_statue(const Tile *t)
{
	return tile_is_object(t) && tile_object_is_statue(t);
}


/**
 * Make an object tile
 * @param t The tile to make an object
 * @param type The object type to make
 * @param o The owner of the object
 * @param id The index of the object
 * @param wc The water class of the object
 * @param random The random bits of the object
 */
static inline void tile_make_object(Tile *t, ObjectType type, Owner o, ObjectID id, WaterClass wc, byte random)
{
	tile_set_type(t, TT_OBJECT);
	t->m1 = (wc << 5) | o;
	t->m2 = id;
	t->m3 = random;
	t->m4 = 0;
	t->m5 = type;
	t->m7 = 0;
}

#endif /* TILE_OBJECT_H */
