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
 * Gets the ObjectType of the given object tile
 * @param t the tile to get the type from.
 * @pre IsObjectTile(t)
 * @return the type.
 */
static inline ObjectType GetObjectType(TileIndex t)
{
	return tile_get_object_type(&_mc[t]);
}

/**
 * Check whether the object on a tile is of a specific type.
 * @param t Tile to test.
 * @param type Type to test.
 * @pre IsObjectTile(t)
 * @return True if type matches.
 */
static inline bool IsObjectType(TileIndex t, ObjectType type)
{
	return GetObjectType(t) == type;
}

/**
 * Check whether a tile is a object tile of a specific type.
 * @param t Tile to test.
 * @param type Type to test.
 * @return True if type matches.
 */
static inline bool IsObjectTypeTile(TileIndex t, ObjectType type)
{
	return IsObjectTile(t) && IsObjectType(t, type);
}

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
 * @note do not use this function directly. Use one of the other Make* functions.
 * @param t      The tile to make and object tile.
 * @param u      The object type of the tile.
 * @param o      The new owner of the tile.
 * @param index  Index to the object.
 * @param wc     Water class for this object.
 * @param random Random data to store on the tile
 */
static inline void MakeObject(TileIndex t, ObjectType u, Owner o, ObjectID index, WaterClass wc, byte random)
{
	tile_make_object(&_mc[t], u, o, index, wc, random);
}

#endif /* MAP_OBJECT_H */
