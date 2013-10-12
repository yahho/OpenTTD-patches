/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/object.h Map tile accessors for object tiles. */

#ifndef MAP_OBJECT_H
#define MAP_OBJECT_H

#include "../stdafx.h"
#include "../tile/common.h"
#include "../tile/object.h"
#include "map.h"
#include "class.h"
#include "coord.h"
#include "../company_type.h"
#include "../object_type.h"

/**
 * Get the index of which object this tile is attached to.
 * @param t the tile
 * @pre IsObjectTile(t)
 * @return The ObjectID of the object.
 */
static inline ObjectID GetObjectIndex(TileIndex t)
{
	return tile_get_object_index(&_mc[t]);
}

/**
 * Get the random bits of this tile.
 * @param t The tile to get the bits for.
 * @pre IsObjectTile(t)
 * @return The random bits.
 */
static inline byte GetObjectRandomBits(TileIndex t)
{
	assert(IsObjectTile(t));
	return tile_get_random_bits(&_mc[t]);
}


/**
 * Make an Object tile.
 * @param t      The tile to make and object tile.
 * @param o      The new owner of the tile.
 * @param index  Index to the object.
 * @param wc     Water class for this object.
 * @param random Random data to store on the tile
 */
static inline void MakeObject(TileIndex t, Owner o, ObjectID index, WaterClass wc, byte random)
{
	tile_make_object(&_mc[t], o, index, wc, random);
}

#endif /* MAP_OBJECT_H */
