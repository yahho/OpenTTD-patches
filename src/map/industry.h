/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/industry.h Map tile accessors for industry tiles. */

#ifndef MAP_INDUSTRY_H
#define MAP_INDUSTRY_H

#include "../stdafx.h"
#include "../tile/common.h"
#include "../tile/industry.h"
#include "map.h"
#include "coord.h"
#include "class.h"


/**
 * Get the industry ID of the given tile
 * @param t the tile to get the industry ID from
 * @pre IsIndustryTile(t)
 * @return the industry ID
 */
static inline uint GetIndustryIndex(TileIndex t)
{
	return tile_get_industry_index(&_mc[t]);
}


/**
 * Is this industry tile fully built?
 * @param t the tile to analyze
 * @pre IsIndustryTile(t)
 * @return true if and only if the industry tile is fully built
 */
static inline bool IsIndustryCompleted(TileIndex t)
{
	return tile_is_industry_completed(&_mc[t]);
}

/**
 * Set if the industry that owns the tile as under construction or not
 * @param tile the tile to query
 * @pre IsIndustryTile(tile)
 */
static inline void SetIndustryCompleted(TileIndex tile)
{
	tile_set_industry_completed(&_mc[tile]);
}

/**
 * Returns the industry construction stage of the specified tile
 * @param tile the tile to query
 * @pre IsIndustryTile(tile)
 * @return the construction stage
 */
static inline byte GetIndustryConstructionStage(TileIndex tile)
{
	return tile_get_construction_stage(&_mc[tile]);
}

/**
 * Sets the industry construction stage of the specified tile
 * @param tile the tile to query
 * @param value the new construction stage
 * @pre IsIndustryTile(tile)
 */
static inline void SetIndustryConstructionStage(TileIndex tile, byte value)
{
	return tile_set_construction_stage(&_mc[tile], value);
}

/**
 * Returns this industry tile's construction counter value
 * @param tile the tile to query
 * @pre IsIndustryTile(tile)
 * @return the construction counter
 */
static inline byte GetIndustryConstructionCounter(TileIndex tile)
{
	return tile_get_construction_counter(&_mc[tile]);
}

/**
 * Sets this industry tile's construction counter value
 * @param tile the tile to query
 * @param value the new value for the construction counter
 * @pre IsIndustryTile(tile)
 */
static inline void SetIndustryConstructionCounter(TileIndex tile, byte value)
{
	tile_set_construction_counter(&_mc[tile], value);
}

/**
 * Reset the construction stage counter of the industry,
 * as well as the completion bit.
 * In fact, it is the same as restarting construction frmo ground up
 * @param tile the tile to query
 * @pre IsIndustryTile(tile)
 */
static inline void ResetIndustryConstructionStage(TileIndex tile)
{
	tile_reset_construction(&_mc[tile]);
}


/**
 * Get the random bits for this tile.
 * Used for grf callbacks
 * @param tile TileIndex of the tile to query
 * @pre IsIndustryTile(tile)
 * @return requested bits
 */
static inline byte GetIndustryRandomBits(TileIndex tile)
{
	assert(IsIndustryTile(tile));
	return tile_get_random_bits(&_mc[tile]);
}

/**
 * Set the random bits for this tile.
 * Used for grf callbacks
 * @param tile TileIndex of the tile to query
 * @param bits the random bits
 * @pre IsIndustryTile(tile)
 */
static inline void SetIndustryRandomBits(TileIndex tile, byte bits)
{
	assert(IsIndustryTile(tile));
	tile_set_random_bits(&_mc[tile], bits);
}


/**
 * Get the animation loop number
 * @param tile the tile to get the animation loop number of
 * @pre IsIndustryTile(tile)
 */
static inline byte GetIndustryAnimationLoop(TileIndex tile)
{
	return tile_get_industry_animation(&_mc[tile]);
}

/**
 * Set the animation loop number
 * @param tile the tile to set the animation loop number of
 * @param count the new animation frame number
 * @pre IsIndustryTile(tile)
 */
static inline void SetIndustryAnimationLoop(TileIndex tile, byte count)
{
	tile_set_industry_animation(&_mc[tile], count);
}


/**
 * Get the industry graphics ID for the given industry tile as
 * stored in the without translation.
 * @param t the tile to get the gfx for
 * @pre IsIndustryTile(t)
 * @return the gfx ID
 */
static inline uint GetCleanIndustryGfx(TileIndex t)
{
	return tile_get_raw_industry_gfx(&_mc[t]);
}

/**
 * Set the industry graphics ID for the given industry tile
 * @param t   the tile to set the gfx for
 * @pre IsIndustryTile(t)
 * @param gfx the graphics ID
 */
static inline void SetIndustryGfx(TileIndex t, uint gfx)
{
	tile_set_raw_industry_gfx(&_mc[t], gfx);
}


/**
 * Get the activated triggers bits for this industry tile
 * Used for grf callbacks
 * @param tile TileIndex of the tile to query
 * @pre IsIndustryTile(tile)
 * @return requested triggers
 */
static inline byte GetIndustryTriggers(TileIndex tile)
{
	return tile_get_industry_triggers(&_mc[tile]);
}

/**
 * Set the activated triggers bits for this industry tile
 * Used for grf callbacks
 * @param tile TileIndex of the tile to query
 * @param triggers the triggers to set
 * @pre IsIndustryTile(tile)
 */
static inline void SetIndustryTriggers(TileIndex tile, byte triggers)
{
	tile_set_industry_triggers(&_mc[tile], triggers);
}


/**
 * Make the given tile an industry tile
 * @param t      the tile to make an industry tile
 * @param index  the industry this tile belongs to
 * @param gfx    the graphics to use for the tile
 * @param random the random value
 * @param wc     the water class for this industry; only useful when build on water
 */
static inline void MakeIndustry(TileIndex t, uint index, uint gfx, uint8 random, WaterClass wc)
{
	tile_make_industry(&_mc[t], index, gfx, random, wc);
}

#endif /* MAP_INDUSTRY_H */
