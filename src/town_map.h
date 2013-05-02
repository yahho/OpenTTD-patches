/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file town_map.h Accessors for towns */

#ifndef TOWN_MAP_H
#define TOWN_MAP_H

#include "map/class.h"
#include "map/ground.h"
#include "map/road.h"
#include "map/house.h"
#include "house.h"

/**
 * Get the type of this house, which is an index into the house spec array
 * @param t the tile
 * @pre IsHouseTile(t)
 * @return house type
 */
static inline HouseID GetHouseType(TileIndex t)
{
	return GetTranslatedHouseID(GetCleanHouseType(t));
}

/**
 * Make the tile a house.
 * @param t tile index
 * @param tid Town index
 * @param counter of construction step
 * @param stage of construction (used for drawing)
 * @param type of house.  Index into house specs array
 * @param random_bits required for newgrf houses
 * @pre IsGroundTile(t)
 */
static inline void MakeHouseTile(TileIndex t, TownID tid, byte counter, byte stage, HouseID type, byte random_bits)
{
	assert(IsGroundTile(t));
	MakeHouseTile(t, tid, counter, stage, type, random_bits, HouseSpec::Get(type)->processing_time);
}

#endif /* TOWN_MAP_H */
