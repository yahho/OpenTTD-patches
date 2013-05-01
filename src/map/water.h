/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/water.h Map tile accessors for water tiles. */

#ifndef MAP_WATER_H
#define MAP_WATER_H

#include "../stdafx.h"
#include "../tile/common.h"
#include "../tile/water.h"
#include "map.h"
#include "coord.h"
#include "common.h"
#include "../direction_type.h"
#include "../direction_func.h"
#include "../company_type.h"

/**
 * Get the water tile type at a tile.
 * @param t Water tile to query.
 * @return Water tile type at the tile.
 */
static inline WaterTileType GetWaterTileType(TileIndex t)
{
	return tile_get_water_type(&_mc[t]);
}

/**
 * Is it a plain water tile?
 * @param t Water tile to query.
 * @return \c true if any type of clear water like ocean, river, or canal.
 * @pre IsWaterTile(t)
 */
static inline bool IsPlainWater(TileIndex t)
{
	return tile_water_is_clear(&_mc[t]);
}

/**
 * Is it a coast tile?
 * @param t Water tile to query.
 * @return \c true if it is a sea water tile.
 * @pre IsWaterTile(t)
 */
static inline bool IsCoast(TileIndex t)
{
	return tile_water_is_coast(&_mc[t]);
}

/**
 * Is it a water tile with a ship depot on it?
 * @param t Water tile to query.
 * @return \c true if it is a ship depot tile.
 * @pre IsWaterTile(t)
 */
static inline bool IsShipDepot(TileIndex t)
{
	return tile_water_is_depot(&_mc[t]);
}

/**
 * Is there a lock on a given water tile?
 * @param t Water tile to query.
 * @return \c true if it is a water lock tile.
 * @pre IsWaterTile(t)
 */
static inline bool IsLock(TileIndex t)
{
	return tile_water_is_lock(&_mc[t]);
}

/**
 * Is it a water tile with plain water?
 * @param t Tile to query.
 * @return \c true if it is a plain water tile.
 */
static inline bool IsPlainWaterTile(TileIndex t)
{
	return tile_is_clear_water(&_mc[t]);
}

/**
 * Is it a coast tile
 * @param t Tile to query.
 * @return \c true if it is a coast.
 */
static inline bool IsCoastTile(TileIndex t)
{
	return tile_is_coast(&_mc[t]);
}

/**
 * Is it a ship depot tile?
 * @param t Tile to query.
 * @return \c true if it is a ship depot tile.
 */
static inline bool IsShipDepotTile(TileIndex t)
{
	return tile_is_ship_depot(&_mc[t]);
}


/**
 * Checks whether the tile has an waterclass associated.
 * You can then subsequently call GetWaterClass().
 * @param t Tile to query.
 * @return True if the tiletype has a waterclass.
 */
static inline bool HasTileWaterClass(TileIndex t)
{
	return tile_has_water_class(&_mc[t]);
}

/**
 * Get the water class at a tile.
 * @param t Water tile to query.
 * @pre IsWaterTile(t) || IsStationTile(t) || IsIndustryTile(t) || IsObjectTile(t)
 * @return Water class at the tile.
 */
static inline WaterClass GetWaterClass(TileIndex t)
{
	return tile_get_water_class(&_mc[t]);
}

/**
 * Set the water class at a tile.
 * @param t  Water tile to change.
 * @param wc New water class.
 * @pre IsWaterTile(t) || IsStationTile(t) || IsIndustryTile(t) || IsObjectTile(t)
 */
static inline void SetWaterClass(TileIndex t, WaterClass wc)
{
	tile_set_water_class(&_mc[t], wc);
}

/**
 * Tests if the tile was built on water.
 * @param t the tile to check
 * @pre IsWaterTile(t) || IsStationTile(t) || IsIndustryTile(t) || IsObjectTile(t)
 * @return true iff on water
 */
static inline bool IsTileOnWater(TileIndex t)
{
	return tile_is_on_water(&_mc[t]);
}


/**
 * Is it a sea water tile?
 * @param t Water tile to query.
 * @return \c true if it is a sea water tile.
 * @pre IsWaterTile(t)
 */
static inline bool IsSea(TileIndex t)
{
	return tile_water_is_sea(&_mc[t]);
}

/**
 * Is it a canal tile?
 * @param t Water tile to query.
 * @return \c true if it is a canal tile.
 * @pre IsWaterTile(t)
 */
static inline bool IsCanal(TileIndex t)
{
	return tile_water_is_canal(&_mc[t]);
}

/**
 * Is it a river water tile?
 * @param t Water tile to query.
 * @return \c true if it is a river water tile.
 * @pre IsWaterTile(t)
 */
static inline bool IsRiver(TileIndex t)
{
	return tile_water_is_river(&_mc[t]);
}


/**
 * Get the axis of the ship depot.
 * @param t Water tile to query.
 * @return Axis of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline Axis GetShipDepotAxis(TileIndex t)
{
	return tile_get_ship_depot_axis(&_mc[t]);
}

/**
 * Get the part of a ship depot.
 * @param t Water tile to query.
 * @return Part of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline DepotPart GetShipDepotPart(TileIndex t)
{
	return tile_get_ship_depot_part(&_mc[t]);
}

/**
 * Get the direction of the ship depot.
 * @param t Water tile to query.
 * @return Direction of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline DiagDirection GetShipDepotDirection(TileIndex t)
{
	return tile_get_ship_depot_direction(&_mc[t]);
}

/**
 * Get the other tile of the ship depot.
 * @param t Tile to query, containing one section of a ship depot.
 * @return Tile containing the other section of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline TileIndex GetOtherShipDepotTile(TileIndex t)
{
	return t + (GetShipDepotPart(t) != DEPOT_PART_NORTH ? -1 : 1) * (GetShipDepotAxis(t) != AXIS_X ? TileDiffXY(0, 1) : TileDiffXY(1, 0));
}

/**
 * Get the most northern tile of a ship depot.
 * @param t One of the tiles of the ship depot.
 * @return The northern tile of the depot.
 * @pre IsShipDepotTile(t)
 */
static inline TileIndex GetShipDepotNorthTile(TileIndex t)
{
	assert(IsShipDepot(t));
	TileIndex tile2 = GetOtherShipDepotTile(t);

	return t < tile2 ? t : tile2;
}


/**
 * Get the direction of the water lock.
 * @param t Water tile to query.
 * @return Direction of the lock.
 * @pre IsWaterTile(t) && IsLock(t)
 */
static inline DiagDirection GetLockDirection(TileIndex t)
{
	return tile_get_lock_direction(&_mc[t]);
}

/**
 * Get the part of a lock.
 * @param t Water tile to query.
 * @return The part.
 * @pre IsWaterTile(t) && IsLock(t)
 */
static inline byte GetLockPart(TileIndex t)
{
	return tile_get_lock_part(&_mc[t]);
}


/**
 * Get the random bits of the water tile.
 * @param t Water tile to query.
 * @return Random bits of the tile.
 * @pre IsWaterTile(t)
 */
static inline byte GetWaterTileRandomBits(TileIndex t)
{
	assert(IsWaterTile(t));
	return tile_get_random_bits(&_mc[t]);
}


/**
 * Checks whether the tile has water at the ground.
 * That is, it is either some plain water tile, or a object/industry/station/... with water under it.
 * @return true iff the tile has water at the ground.
 * @note Coast tiles are not considered waterish, even if there is water on a halftile.
 */
static inline bool HasTileWaterGround(TileIndex t)
{
	return HasTileWaterClass(t) && IsTileOnWater(t) && !IsCoastTile(t);
}


/**
 * Helper function for making a watery tile.
 * @param t The tile to change into water
 * @param o The owner of the water
 * @param wc The class of water the tile has to be
 * @param random_bits Eventual random bits to be set for this tile
 */
static inline void MakeWater(TileIndex t, Owner o, WaterClass wc, uint8 random_bits)
{
	tile_make_water(&_mc[t], o, wc, random_bits);
}

/**
 * Make a sea tile.
 * @param t The tile to change into sea
 */
static inline void MakeSea(TileIndex t)
{
	tile_make_sea(&_mc[t]);
}

/**
 * Make a canal tile
 * @param t The tile to change into canal
 * @param o The owner of the canal
 * @param random_bits Random bits to be set for this tile
 */
static inline void MakeCanal(TileIndex t, Owner o, uint8 random_bits)
{
	tile_make_canal(&_mc[t], o, random_bits);
}

/**
 * Make a river tile
 * @param t The tile to change into river
 * @param random_bits Random bits to be set for this tile
 */
static inline void MakeRiver(TileIndex t, uint8 random_bits)
{
	tile_make_river(&_mc[t], random_bits);
}

/**
 * Helper function to make a coast tile.
 * @param t The tile to change into water
 */
static inline void MakeShore(TileIndex t)
{
	tile_make_shore(&_mc[t]);
}

/**
 * Make a ship depot section.
 * @param t    Tile to place the ship depot section.
 * @param o    Owner of the depot.
 * @param did  Depot ID.
 * @param part Depot part (either #DEPOT_PART_NORTH or #DEPOT_PART_SOUTH).
 * @param a    Axis of the depot.
 * @param original_water_class Original water class.
 */
static inline void MakeShipDepot(TileIndex t, Owner o, uint did, DepotPart part, Axis a, WaterClass original_water_class)
{
	tile_make_ship_depot(&_mc[t], o, did, part, a, original_water_class);
}

/**
 * Make a lock section.
 * @param t Tile to place the water lock section.
 * @param o Owner of the lock.
 * @param part Part to place.
 * @param dir Lock orientation
 * @param original_water_class Original water class.
 * @see MakeLock
 */
static inline void MakeLockTile(TileIndex t, Owner o, LockPart part, DiagDirection dir, WaterClass original_water_class)
{
	tile_make_lock(&_mc[t], o, part, dir, original_water_class);
}

/**
 * Make a water lock.
 * @param t Tile to place the water lock section.
 * @param o Owner of the lock.
 * @param d Direction of the water lock.
 * @param wc_lower Original water class of the lower part.
 * @param wc_upper Original water class of the upper part.
 * @param wc_middle Original water class of the middle part.
 */
static inline void MakeLock(TileIndex t, Owner o, DiagDirection d, WaterClass wc_lower, WaterClass wc_upper, WaterClass wc_middle)
{
	TileIndexDiff delta = TileOffsByDiagDir(d);

	/* Keep the current waterclass and owner for the tiles.
	 * It allows to restore them after the lock is deleted */
	MakeLockTile(t, o, LOCK_PART_MIDDLE, d, wc_middle);
	MakeLockTile(t - delta, IsPlainWaterTile(t - delta) ? GetTileOwner(t - delta) : o, LOCK_PART_LOWER, d, wc_lower);
	MakeLockTile(t + delta, IsPlainWaterTile(t + delta) ? GetTileOwner(t + delta) : o, LOCK_PART_UPPER, d, wc_upper);
}

#endif /* MAP_WATER_H */
