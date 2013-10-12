/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file object.h Functions related to objects. */

#ifndef OBJECT_H
#define OBJECT_H

#include "map/coord.h"
#include "map/object.h"
#include "company_type.h"
#include "object_type.h"

ObjectType GetObjectType(TileIndex t);

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

void UpdateCompanyHQ(TileIndex tile, uint score);

void BuildObject(ObjectType type, TileIndex tile, CompanyID owner = OWNER_NONE, struct Town *town = NULL, uint8 view = 0);

void PlaceProc_Object(TileIndex tile);
void ShowBuildObjectPicker(struct Window *w);

#endif /* OBJECT_H */
