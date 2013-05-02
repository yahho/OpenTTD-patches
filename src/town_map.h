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

#include "tile/common.h"
#include "tile/house.h"
#include "map/class.h"
#include "map/ground.h"
#include "map/road.h"
#include "house.h"

/**
 * Get the index of which town this house/street is attached to.
 * @param t the tile
 * @pre IsHouseTile(t) or IsRoadTile(t) or IsLevelCrossingTile(t)
 * @return TownID
 */
static inline TownID GetTownIndex(TileIndex t)
{
	return tile_get_town(&_mc[t]);
}

/**
 * Set the town index for a road or house tile.
 * @param t the tile
 * @param index the index of the town
 * @pre IsHouseTile(t) or IsRoadTile(t) or IsLevelCrossingTile(t)
 */
static inline void SetTownIndex(TileIndex t, TownID index)
{
	tile_set_town(&_mc[t], index);
}

/**
 * Get the type of this house, which is an index into the house spec array
 * without doing any NewGRF related translations.
 * @param t the tile
 * @pre IsHouseTile(t)
 * @return house type
 */
static inline HouseID GetCleanHouseType(TileIndex t)
{
	return tile_get_raw_house_type(&_mc[t]);
}

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
 * Set the house type.
 * @param t the tile
 * @param house_id the new house type
 * @pre IsHouseTile(t)
 */
static inline void SetHouseType(TileIndex t, HouseID house_id)
{
	tile_set_raw_house_type(&_mc[t], house_id);
}

/**
 * Check if the lift of this animated house has a destination
 * @param t the tile
 * @return has destination
 */
static inline bool LiftHasDestination(TileIndex t)
{
	return tile_has_lift_destination(&_mc[t]);
}

/**
 * Set the new destination of the lift for this animated house, and activate
 * the LiftHasDestination bit.
 * @param t the tile
 * @param dest new destination
 */
static inline void SetLiftDestination(TileIndex t, byte dest)
{
	tile_set_lift_destination(&_mc[t], dest);
}

/**
 * Get the current destination for this lift
 * @param t the tile
 * @return destination
 */
static inline byte GetLiftDestination(TileIndex t)
{
	return tile_get_lift_destination(&_mc[t]);
}

/**
 * Stop the lift of this animated house from moving.
 * Clears the first 4 bits of m7 at once, clearing the LiftHasDestination bit
 * and the destination.
 * @param t the tile
 */
static inline void HaltLift(TileIndex t)
{
	tile_halt_lift(&_mc[t]);
}

/**
 * Get the position of the lift on this animated house
 * @param t the tile
 * @return position, from 0 to 36
 */
static inline byte GetLiftPosition(TileIndex t)
{
	return tile_get_lift_position(&_mc[t]);
}

/**
 * Set the position of the lift on this animated house
 * @param t the tile
 * @param pos position, from 0 to 36
 */
static inline void SetLiftPosition(TileIndex t, byte pos)
{
	tile_set_lift_position(&_mc[t], pos);
}

/**
 * Get the completion of this house
 * @param t the tile
 * @return true if it is, false if it is not
 */
static inline bool IsHouseCompleted(TileIndex t)
{
	return tile_is_house_completed(&_mc[t]);
}

/**
 * Mark this house as been completed
 * @param t the tile
 * @param status
 */
static inline void SetHouseCompleted(TileIndex t, bool status)
{
	tile_set_house_completed(&_mc[t], status);
}

/**
 * Gets the building stage of a house
 * Since the stage is used for determining what sprite to use,
 * if the house is complete (and that stage no longer is available),
 * fool the system by returning the TOWN_HOUSE_COMPLETE (3),
 * thus showing a beautiful complete house.
 * @param t the tile of the house to get the building stage of
 * @pre IsHouseTile(t)
 * @return the building stage of the house
 */
static inline byte GetHouseBuildingStage(TileIndex t)
{
	return tile_get_building_stage(&_mc[t]);
}

/**
 * Gets the construction stage of a house
 * @param t the tile of the house to get the construction stage of
 * @pre IsHouseTile(t)
 * @return the construction stage of the house
 */
static inline byte GetHouseConstructionTick(TileIndex t)
{
	return tile_get_building_counter(&_mc[t]);
}

/**
 * Sets the increment stage of a house
 * It is working with the whole counter + stage 5 bits, making it
 * easier to work:  the wraparound is automatic.
 * @param t the tile of the house to increment the construction stage of
 * @pre IsHouseTile(t)
 */
static inline void IncHouseConstructionTick(TileIndex t)
{
	if (tile_inc_building_counter(&_mc[t])) {
		/* House is now completed.
		 * Store the year of construction as well, for newgrf house purpose */
		SetHouseCompleted(t, true);
	}
}

/**
 * Sets the age of the house to zero.
 * Needs to be called after the house is completed. During construction stages the map space is used otherwise.
 * @param t the tile of this house
 * @pre IsHouseTile(t) && IsHouseCompleted(t)
 */
static inline void ResetHouseAge(TileIndex t)
{
	tile_reset_house_age(&_mc[t]);
}

/**
 * Increments the age of the house.
 * @param t the tile of this house
 * @pre IsHouseTile(t)
 */
static inline void IncrementHouseAge(TileIndex t)
{
	tile_inc_house_age(&_mc[t]);
}

/**
 * Get the age of the house
 * @param t the tile of this house
 * @pre IsHouseTile(t)
 * @return year
 */
static inline Year GetHouseAge(TileIndex t)
{
	return tile_get_house_age(&_mc[t]);
}

/**
 * Set the random bits for this house.
 * This is required for newgrf house
 * @param t      the tile of this house
 * @param random the new random bits
 * @pre IsHouseTile(t)
 */
static inline void SetHouseRandomBits(TileIndex t, byte random)
{
	assert(IsHouseTile(t));
	tile_set_random_bits(&_mc[t], random);
}

/**
 * Get the random bits for this house.
 * This is required for newgrf house
 * @param t the tile of this house
 * @pre IsHouseTile(t)
 * @return random bits
 */
static inline byte GetHouseRandomBits(TileIndex t)
{
	assert(IsHouseTile(t));
	return tile_get_random_bits(&_mc[t]);
}

/**
 * Set the activated triggers bits for this house.
 * This is required for newgrf house
 * @param t        the tile of this house
 * @param triggers the activated triggers
 * @pre IsHouseTile(t)
 */
static inline void SetHouseTriggers(TileIndex t, byte triggers)
{
	tile_set_house_triggers(&_mc[t], triggers);
}

/**
 * Get the already activated triggers bits for this house.
 * This is required for newgrf house
 * @param t the tile of this house
 * @pre IsHouseTile(t)
 * @return triggers
 */
static inline byte GetHouseTriggers(TileIndex t)
{
	return tile_get_house_triggers(&_mc[t]);
}

/**
 * Get the amount of time remaining before the tile loop processes this tile.
 * @param t the house tile
 * @pre IsHouseTile(t)
 * @return time remaining
 */
static inline byte GetHouseProcessingTime(TileIndex t)
{
	return tile_get_house_processing_counter(&_mc[t]);
}

/**
 * Set the amount of time remaining before the tile loop processes this tile.
 * @param t the house tile
 * @param time the time to be set
 * @pre IsHouseTile(t)
 */
static inline void SetHouseProcessingTime(TileIndex t, byte time)
{
	tile_set_house_processing_counter(&_mc[t], time);
}

/**
 * Decrease the amount of time remaining before the tile loop processes this tile.
 * @param t the house tile
 * @pre IsHouseTile(t)
 */
static inline void DecHouseProcessingTime(TileIndex t)
{
	tile_dec_house_processing_counter(&_mc[t]);
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
	tile_make_house(&_mc[t], tid, type, stage, counter, random_bits, HouseSpec::Get(type)->processing_time);
}

#endif /* TOWN_MAP_H */
