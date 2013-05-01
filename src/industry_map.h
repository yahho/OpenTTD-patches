/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file industry_map.h Accessors for industries */

#ifndef INDUSTRY_MAP_H
#define INDUSTRY_MAP_H

#include "tile/common.h"
#include "tile/industry.h"
#include "map/water.h"
#include "industrytype.h"


/**
 * The following enums are indices used to know what to draw for this industry tile.
 * They all are pointing toward array _industry_draw_tile_data, in table/industry_land.h
 * How to calculate the correct position ? GFXid << 2 | IndustryStage (0 to 3)
 */
enum IndustryGraphics {
	GFX_COAL_MINE_TOWER_NOT_ANIMATED   =   0,
	GFX_COAL_MINE_TOWER_ANIMATED       =   1,
	GFX_POWERPLANT_CHIMNEY             =   8,
	GFX_POWERPLANT_SPARKS              =  10,
	GFX_OILRIG_1                       =  24,
	GFX_OILRIG_2                       =  25,
	GFX_OILRIG_3                       =  26,
	GFX_OILRIG_4                       =  27,
	GFX_OILRIG_5                       =  28,
	GFX_OILWELL_NOT_ANIMATED           =  29,
	GFX_OILWELL_ANIMATED_1             =  30,
	GFX_OILWELL_ANIMATED_2             =  31,
	GFX_OILWELL_ANIMATED_3             =  32,
	GFX_COPPER_MINE_TOWER_NOT_ANIMATED =  47,
	GFX_COPPER_MINE_TOWER_ANIMATED     =  48,
	GFX_COPPER_MINE_CHIMNEY            =  49,
	GFX_GOLD_MINE_TOWER_NOT_ANIMATED   =  79,
	GFX_GOLD_MINE_TOWER_ANIMATED       =  88,
	GFX_TOY_FACTORY                    = 143,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_1    = 148,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_2    = 149,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_3    = 150,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_4    = 151,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_5    = 152,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_6    = 153,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_7    = 154,
	GFX_PLASTIC_FOUNTAIN_ANIMATED_8    = 155,
	GFX_BUBBLE_GENERATOR               = 161,
	GFX_BUBBLE_CATCHER                 = 162,
	GFX_TOFFEE_QUARY                   = 165,
	GFX_SUGAR_MINE_SIEVE               = 174,
	GFX_WATERTILE_SPECIALCHECK         = 255,  ///< not really a tile, but rather a very special check
};

/**
 * Get the industry ID of the given tile
 * @param t the tile to get the industry ID from
 * @pre IsIndustryTile(t)
 * @return the industry ID
 */
static inline IndustryID GetIndustryIndex(TileIndex t)
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

IndustryType GetIndustryType(TileIndex tile);

/**
 * Set if the industry that owns the tile as under construction or not
 * @param tile the tile to query
 * @param isCompleted whether it is completed or not
 * @pre IsIndustryTile(tile)
 */
static inline void SetIndustryCompleted(TileIndex tile, bool isCompleted)
{
	tile_set_industry_completed(&_mc[tile], isCompleted);
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
 * Get the industry graphics ID for the given industry tile as
 * stored in the without translation.
 * @param t the tile to get the gfx for
 * @pre IsIndustryTile(t)
 * @return the gfx ID
 */
static inline IndustryGfx GetCleanIndustryGfx(TileIndex t)
{
	return tile_get_raw_industry_gfx(&_mc[t]);
}

/**
 * Get the industry graphics ID for the given industry tile
 * @param t the tile to get the gfx for
 * @pre IsIndustryTile(t)
 * @return the gfx ID
 */
static inline IndustryGfx GetIndustryGfx(TileIndex t)
{
	assert(IsIndustryTile(t));
	return GetTranslatedIndustryTileID(GetCleanIndustryGfx(t));
}

/**
 * Set the industry graphics ID for the given industry tile
 * @param t   the tile to set the gfx for
 * @pre IsIndustryTile(t)
 * @param gfx the graphics ID
 */
static inline void SetIndustryGfx(TileIndex t, IndustryGfx gfx)
{
	tile_set_raw_industry_gfx(&_mc[t], gfx);
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
static inline void MakeIndustry(TileIndex t, IndustryID index, IndustryGfx gfx, uint8 random, WaterClass wc)
{
	tile_make_industry(&_mc[t], index, gfx, random, wc);
}

#endif /* INDUSTRY_MAP_H */
