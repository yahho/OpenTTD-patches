/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/house.h Tile functions for house tiles. */

#ifndef TILE_HOUSE_H
#define TILE_HOUSE_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"

/**
 * Simple value that indicates the house has reached the final stage of
 * construction.
 */
static const byte TOWN_HOUSE_COMPLETED = 3;


/**
 * Get the town index of a tile (house town for houses, owner or closest town for roads)
 * @param t The tile whose town index to get
 * @pre tile_is_house(t) || tile_is_road(t) || tile_is_crossing(t)
 * @return The town index of the tile
 */
static inline uint tile_get_town(const Tile *t)
{
	assert(tile_is_house(t) || tile_is_road(t) || tile_is_crossing(t));
	return t->m2;
}

/**
 * Set the town index of a tile (house town for houses, owner or closest town for roads)
 * @param t The tile whose town index to set
 * @param town The town index to set
 * @pre tile_is_house(t) || tile_is_road(t) || tile_is_crossing(t)
 */
static inline void tile_set_town(Tile *t, uint town)
{
	assert(tile_is_house(t) || tile_is_road(t) || tile_is_crossing(t));
	t->m2 = town;
}


/**
 * Get the raw house type of a tile
 * @param t The tile whose house type to get
 * @pre tile_is_house(t)
 * @return The raw house type of a tile
 */
static inline uint tile_get_raw_house_type(const Tile *t)
{
	assert(tile_is_house(t));
	return t->m4 | (GB(t->m1, 6, 1) << 8);
}

/**
 * Set the raw house type of a tile
 * @param t The tile whose house type to set
 * @param id The house type to set
 * @pre tile_is_house(t)
 */
static inline void tile_set_raw_house_type(Tile *t, uint id)
{
	assert(tile_is_house(t));
	t->m4 = GB(id, 0, 8);
	SB(t->m1, 6, 1, GB(id, 8, 1));
}


/**
 * House Construction Scheme.
 *  Construction counter, for buildings under construction. Incremented on every
 *  periodic tile processing.
 *  On wraparound, the stage of building in is increased.
 *  tile_get_building_stage is taking care of the real stages,
 *  (as the sprite for the next phase of house building)
 *  tile_{get,inc}_building_counter is simply a tick counter between the
 *  different stages
 */

/**
 * Check if a house tile is completed
 * @param t The tile to check
 * @pre tile_is_house(t)
 * @return Whether the tile is completed
 */
static inline bool tile_is_house_completed(const Tile *t)
{
	assert(tile_is_house(t));
	return HasBit(t->m1, 7);
}

/**
 * Set the completion state of a house tile
 * @param t The tile whose completion state to set
 * @param b The completion state to set
 * @pre tile_is_house(t)
 */
static inline void tile_set_house_completed(Tile *t, bool b)
{
	assert(tile_is_house(t));
	SB(t->m1, 7, 1, b ? 1 : 0);
}

/**
 * Get the building stage of a house tile
 * @param t The tile whose building stage to get
 * @pre tile_is_house(t)
 * @return The building stage of the tile
 */
static inline unsigned tile_get_building_stage(const Tile *t)
{
	assert(tile_is_house(t));
	return tile_is_house_completed(t) ? TOWN_HOUSE_COMPLETED : GB(t->m5, 3, 2);
}

/**
 * Get the building counter of a house tile
 * @param t The tile whose building counter to get
 * @pre tile_is_house(t)
 * @return The building counter of the tile
 */
static inline unsigned tile_get_building_counter(const Tile *t)
{
	assert(tile_is_house(t));
	return tile_is_house_completed(t) ? 0 : GB(t->m5, 0, 3);
}

/**
 * Increment the building counter of a house tile
 * @param t The tile whose building counter to increment
 * @pre tile_is_house(t)
 * @return Whether the house has reached its maximum building stage
 */
static inline bool tile_inc_building_counter(Tile *t)
{
	assert(tile_is_house(t));
	AB(t->m5, 0, 5, 1);
	return GB(t->m5, 3, 2) == TOWN_HOUSE_COMPLETED;
}


/**
 * Get the age of a house
 * @param t The tile whose age to get
 * @pre tile_is_house(t)
 * @return The age of the house
 */
static inline uint tile_get_house_age(const Tile *t)
{
	assert(tile_is_house(t));
	return tile_is_house_completed(t) ? t->m5 : 0;
}

/**
 * Reset the age of a house
 * @param t The tile whose age to reset
 * @pre tile_is_house(t) && tile_is_house_completed(t)
 */
static inline void tile_reset_house_age(Tile *t)
{
	assert(tile_is_house(t));
	assert(tile_is_house_completed(t));
	t->m5 = 0;
}

/**
 * Increment the age of a house, if it is completed
 * @param t The tile whose age to increment
 * @pre tile_is_house(t)
 */
static inline void tile_inc_house_age(Tile *t)
{
	assert(tile_is_house(t));
	if (tile_is_house_completed(t) && t->m5 < 0xFF) t->m5++;
}


/**
 * Get the triggers of a house tile
 * @param t The tile whose triggers to get
 * @pre tile_is_house(t)
 * @return The triggers of the tile
 */
static inline uint tile_get_house_triggers(const Tile *t)
{
	assert(tile_is_house(t));
	return GB(t->m0, 0, 5);
}

/**
 * Set the triggers of a house tile
 * @param t The tile whose triggers to set
 * @param triggers The triggers to set
 * @pre tile_is_house(t)
 */
static inline void tile_set_house_triggers(Tile *t, uint triggers)
{
	assert(tile_is_house(t));
	assert(triggers < 32);
	SB(t->m0, 0, 5, triggers);
}


/**
 * Get the processing counter of a house tile
 * @param t The tile whose processing counter to get
 * @pre tile_is_house(t)
 * @return The processing counter of the tile
 */
static inline uint tile_get_house_processing_counter(const Tile *t)
{
	assert(tile_is_house(t));
	return GB(t->m1, 0, 6);
}

/**
 * Set the processing counter of a house tile
 * @param t The tile whose processing counter to set
 * @param time The counter to set
 * @pre tile_is_house(t)
 */
static inline void tile_set_house_processing_counter(Tile *t, uint time)
{
	assert(tile_is_house(t));
	assert(time < 64);
	SB(t->m1, 0, 6, time);
}

/**
 * Decrement the processing counter of a house tile
 * @param t The tile whose processing counter to decrement
 * @pre tile_is_house(t) && tile_get_house_processing_counter(t) > 0
 */
static inline void tile_dec_house_processing_counter(Tile *t)
{
	assert(tile_is_house(t));
	assert(tile_get_house_processing_counter(t) > 0);
	t->m1--;
}


/**
 * Check if the lift of a house is currently animated (has a destination)
 * @param t The tile to check
 * @pre tile_is_house(t)
 * @return Whether the house lift has a destination
 */
static inline bool tile_has_lift_destination(const Tile *t)
{
	assert(tile_is_house(t));
	return HasBit(t->m7, 3);
}

/**
 * Get the lift destination of a house tile
 * @param t The tile whose lift destination to get
 * @pre tile_is_house(t)
 * @return The lift destination of the house
 */
static inline uint tile_get_lift_destination(const Tile *t)
{
	assert(tile_is_house(t));
	return GB(t->m7, 0, 3);
}

/**
 * Set the lift destination of a house tile, and activate its animation bit
 * @param t The tile whose lift destination to set
 * @param uint dest The destination to set
 * @pre tile_is_house(t)
 */
static inline void tile_set_lift_destination(Tile *t, uint dest)
{
	assert(tile_is_house(t));
	SetBit(t->m7, 3);
	SB(t->m7, 0, 3, dest);
}

/**
 * Stop the lift of a house tile, and clear its destination
 * @param t The tile whose lift to stop
 * @pre tile_is_house(t)
 */
static inline void tile_halt_lift(Tile *t)
{
	assert(tile_is_house(t));
	SB(t->m7, 0, 4, 0);
}

/**
 * Get the lift position of a house tile
 * @param t The tile whose lift position to get
 * @pre tile_is_house(t)
 * @return The lift position of the tile, between 0 and 36
 */
static inline uint tile_get_lift_position(const Tile *t)
{
	assert(tile_is_house(t));
	return GB(t->m1, 0, 6);
}

/**
 * Set the lift position of a house tile
 * @param t The tile whose lift position to set
 * @param pos The lift position to set
 * @pre tile_is_house(t)
 */
static inline void tile_set_lift_position(Tile *t, uint pos)
{
	assert(tile_is_house(t));
	SB(t->m1, 0, 6, pos);
}


/**
 * Make a house tile
 * @param t The tile to make a house
 * @param town The index of the town to which this house belongs
 * @param type The type of the house
 * @param stage The building stage of the house
 * @param counter The building counter of the house
 * @param random The random bits of the house
 * @param processing The initial processing counter of the house
 */
static inline void tile_make_house(Tile *t, uint town, uint type, uint stage, uint counter, uint random, uint processing)
{
	t->m0 = 0xC0;
	bool completed = stage == TOWN_HOUSE_COMPLETED;
	t->m1 = (completed ? 0x80 : 0) | (GB(type, 8, 1) << 6) | processing;
	t->m2 = town;
	t->m3 = random;
	t->m4 = GB(type, 0, 8);
	t->m5 = completed ? 0 : ((stage << 3) | counter);
	t->m7 = 0;
}

#endif /* TILE_HOUSE_H */
