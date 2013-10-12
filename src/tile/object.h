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
 * Get the index of the object at a tile
 * @param t The tile whose object index to get
 * @pre tile_is_object(t)
 * @return The index of the object at the tile
 */
static inline ObjectID tile_get_object_index(const Tile *t)
{
	assert(tile_is_object(t));
	return t->m2 | t->m5 << 16;
}


/**
 * Make an object tile
 * @param t The tile to make an object
 * @param o The owner of the object
 * @param id The index of the object
 * @param wc The water class of the object
 * @param random The random bits of the object
 */
static inline void tile_make_object(Tile *t, Owner o, ObjectID id, WaterClass wc, byte random)
{
	tile_set_type(t, TT_OBJECT);
	t->m1 = (wc << 5) | o;
	t->m2 = GB(id, 0, 16);
	t->m3 = random;
	t->m4 = 0;
	t->m5 = GB(id, 16, 8);
	t->m7 = 0;
}

#endif /* TILE_OBJECT_H */
