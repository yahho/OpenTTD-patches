/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/industry.h Tile functions for industry tiles. */

#ifndef TILE_INDUSTRY_H
#define TILE_INDUSTRY_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "water.h"

static const int INDUSTRY_COMPLETED = 3; ///< final stage of industry construction.


/**
 * Get the index of the industry at a tile
 * @param t The tile whose industry to get the index of
 * @pre tile_is_industry(t)
 * @return The index of the industry at the tile
 */
static inline uint tile_get_industry_index(const Tile *t)
{
	assert(tile_is_industry(t));
	return t->m2;
}


/**
 * Check if an industry tile is completed
 * @param t The tile to check
 * @pre tile_is_industry(t)
 * @return Whether the tile is completed
 */
static inline bool tile_is_industry_completed(const Tile *t)
{
	assert(tile_is_industry(t));
	return HasBit(t->m1, 7);
}

/**
 * Set the completion state of an industry tile
 * @param t The tile whose completion state to set
 * @pre tile_is_industry(t)
 */
static inline void tile_set_industry_completed(Tile *t)
{
	assert(tile_is_industry(t));
	SetBit(t->m1, 7);
}

/**
 * Get the construction stage of an industry tile
 * @param t The tile whose construction stage to get
 * @pre tile_is_industry(t)
 * @return The construction stage of the tile
 */
static inline uint tile_get_construction_stage(const Tile *t)
{
	assert(tile_is_industry(t));
	return tile_is_industry_completed(t) ? INDUSTRY_COMPLETED : GB(t->m1, 0, 2);
}

/**
 * Set the construction stage of an industry tile
 * @param t The tile whose construction stage to set
 * @param stage The construction stage to set
 * @pre tile_is_industry(t)
 */
static inline void tile_set_construction_stage(Tile *t, uint stage)
{
	assert(tile_is_industry(t));
	assert(stage < 4);
	SB(t->m1, 0, 2, stage);
}

/**
 * Get the construction counter of an industry tile
 * @param t The tile whose construction counter to get
 * @pre tile_is_industry(t)
 * @return The construction counter of the tile
 */
static inline uint tile_get_construction_counter(const Tile *t)
{
	assert(tile_is_industry(t));
	return GB(t->m1, 2, 2);
}

/**
 * Set the construction counter of an industry tile
 * @param t The tile whose construction counter to set
 * @param counter The construction counter to set
 * @pre tile_is_industry(t)
 */
static inline void tile_set_construction_counter(Tile *t, uint counter)
{
	assert(tile_is_industry(t));
	assert(counter < 4);
	SB(t->m1, 2, 2, counter);
}

/**
 * Reset industry construction state
 * @param t The tile to reset
 * @pre tile_is_industry(t)
 */
static inline void tile_reset_construction(Tile *t)
{
	assert(tile_is_industry(t));
	SB(t->m1, 0, 4, 0);
	SB(t->m1, 7, 1, 0);
}


/**
 * Get the animation of an industry tile
 * @param t The tile whose animation to get
 * @pre tile_is_industry(t)
 * @return The animation loop of the tile
 */
static inline uint tile_get_industry_animation(const Tile *t)
{
	assert(tile_is_industry(t));
	return t->m4;
}

/**
 * Set the animation of an industry tile
 * @param t The tile whose animation to set
 * @param count The animation loop to set
 * @pre tile_is_industry(t)
 */
static inline void tile_set_industry_animation(Tile *t, uint count)
{
	assert(tile_is_industry(t));
	t->m4 = count;
}


/**
 * Get the raw graphics index of an industry tile
 * @param t The tile whose graphics index to get
 * @pre tile_is_industry(t)
 * @return The raw graphics index of the tile
 */
static inline uint tile_get_raw_industry_gfx(const Tile *t)
{
	assert(tile_is_industry(t));
	return t->m5 | (GB(t->m0, 3, 1) << 8);
}

/**
 * Set the raw graphics index of an industry tile
 * @param t The tile whose graphics index to set
 * @param gfx The graphics index to set
 * @pre tile_is_industry(t)
 */
static inline void tile_set_raw_industry_gfx(Tile *t, uint gfx)
{
	assert(tile_is_industry(t));
	assert(gfx < 0x200);
	t->m5 = GB(gfx, 0, 8);
	SB(t->m0, 3, 1, GB(gfx, 8, 1));
}


/**
 * Get the triggers of an industry tile
 * @param t The tile whose triggers to get
 * @pre tile_is_industry(t)
 * @return The triggers of the tile
 */
static inline uint tile_get_industry_triggers(const Tile *t)
{
	assert(tile_is_industry(t));
	return GB(t->m0, 0, 3);
}

/**
 * Set the triggers of an industry tile
 * @param t The tile whose triggers to set
 * @param triggers The triggers to set
 * @pre tile_is_industry(t)
 */
static inline void tile_set_industry_triggers(Tile *t, uint triggers)
{
	assert(tile_is_industry(t));
	assert(triggers < 8);
	SB(t->m0, 0, 3, triggers);
}


/**
 * Make an industry tile
 * @param t The tile to make an industry
 * @param id The index of the industry
 * @param gfx The graphics index of the tile
 * @param random The random bits of the tile
 * @param wc The water class of the tile
 */
static inline void tile_make_industry(Tile *t, uint id, uint gfx, uint random, WaterClass wc = WATER_CLASS_INVALID)
{
	t->m0 = 0x80 | (GB(gfx, 8, 1) << 3);
	t->m1 = wc << 5;
	t->m2 = id;
	t->m3 = random;
	t->m4 = 0;
	t->m5 = GB(gfx, 0, 8);
	t->m7 = 0;
}

#endif /* TILE_INDUSTRY_H */
