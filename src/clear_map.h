/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file clear_map.h Map accessors for 'clear' tiles */

#ifndef CLEAR_MAP_H
#define CLEAR_MAP_H

#include "tile/ground.h"
#include "bridge_map.h"
#include "industry_type.h"

/**
 * Get the full ground type of a clear tile.
 * @param t the tile to get the ground type of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the ground type
 */
static inline Ground GetFullClearGround(TileIndex t)
{
	return tile_get_full_ground(&_mc[t]);
}

/**
 * Test if a tile is covered with snow.
 * @param t the tile to check
 * @pre IsGroundTile(t)
 * @return whether the tile is covered with snow.
 */
static inline bool IsSnowTile(TileIndex t)
{
	return tile_ground_has_snow(&_mc[t]);
}

/**
 * Get the tile ground ignoring snow.
 * @param t the tile to get the clear ground type of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the ground type
 */
static inline Ground GetRawClearGround(TileIndex t)
{
	return tile_get_raw_ground(&_mc[t]);
}

/**
 * Get the tile ground, treating all snow types as equal.
 * @param t the tile to get the clear ground type of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the ground type
 */
static inline Ground GetClearGround(TileIndex t)
{
	return tile_get_ground(&_mc[t]);
}

/**
 * Set the tile ground.
 * @param t  the tile to set the clear ground type of
 * @param ct the ground type
 * @pre IsClearTile(t) || IsTreeTile(t)
 */
static inline bool IsClearGround(TileIndex t, Ground g)
{
	return tile_is_ground(&_mc[t], g);
}


/**
 * Get the density of a non-field clear tile.
 * @param t the tile to get the density of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the density
 */
static inline uint GetClearDensity(TileIndex t)
{
	return tile_get_density(&_mc[t]);
}

/**
 * Increment the density of a non-field clear tile.
 * @param t the tile to increment the density of
 * @param d the amount to increment the density with
 * @pre IsClearTile(t) || IsTreeTile(t)
 */
static inline void AddClearDensity(TileIndex t, int d)
{
	tile_add_density(&_mc[t], d);
}

/**
 * Set the density of a non-field clear tile.
 * @param t the tile to set the density of
 * @param d the new density
 * @pre IsClearTile(t) || IsTreeTile(t)
 */
static inline void SetClearDensity(TileIndex t, uint d)
{
	assert(IsClearTile(t) || IsTreeTile(t));
	SB(_mc[t].m4, 0, 2, d);
}

/**
 * Sets ground type and density in one go, optionally resetting the counter.
 * @param t       the tile to set the ground type and density for
 * @param type    the new ground type of the tile
 * @param density the density of the ground tile
 * @param keep_counter whether to keep the update counter
 * @pre IsClearTile(t) || IsTreeTile(t)
 */
static inline void SetClearGroundDensity(TileIndex t, Ground g, uint density, bool keep_counter = false)
{
	tile_set_ground_density(&_mc[t], g, density, keep_counter);
}


/**
 * Get the field type (production stage) of the field
 * @param t the field to get the type of
 * @pre IsFieldsTile(t)
 * @return the field type
 */
static inline uint GetFieldType(TileIndex t)
{
	return tile_get_field_type(&_mc[t]);
}

/**
 * Set the field type (production stage) of the field
 * @param t the field to get the type of
 * @param f the field type
 * @pre IsFieldsTile(t)
 */
static inline void SetFieldType(TileIndex t, uint f)
{
	tile_set_field_type(&_mc[t], f);
}

/**
 * Get the industry (farm) that made the field
 * @param t the field to get creating industry of
 * @pre IsFieldsTile(t)
 * @return the industry that made the field
 */
static inline IndustryID GetIndustryIndexOfField(TileIndex t)
{
	return tile_get_field_industry(&_mc[t]);
}

/**
 * Set the industry (farm) that made the field
 * @param t the field to get creating industry of
 * @param i the industry that made the field
 * @pre IsFieldsTile(t)
 */
static inline void SetIndustryIndexOfField(TileIndex t, IndustryID i)
{
	tile_set_field_industry(&_mc[t], i);
}


/**
 * Get the counter used to advance to the next clear density/field type.
 * @param t the tile to get the counter of
 * @pre IsGroundTile(t)
 * @return the value of the counter
 */
static inline uint GetClearCounter(TileIndex t)
{
	return tile_get_clear_counter(&_mc[t]);
}

/**
 * Increments the counter used to advance to the next clear density/field type.
 * @param t the tile to increment the counter of
 * @param c the amount to increment the counter with
 * @pre IsGroundTile(t)
 */
static inline void AddClearCounter(TileIndex t, int c)
{
	tile_add_clear_counter(&_mc[t], c);
}

/**
 * Sets the counter used to advance to the next clear density/field type.
 * @param t the tile to set the counter of
 * @param c the amount to set the counter to
 * @pre IsClearTile(t) || IsFieldsTile(t)
 */
static inline void SetClearCounter(TileIndex t, uint c)
{
	tile_set_clear_counter(&_mc[t], c);
}


/**
 * Is there a fence at the given border?
 * @param t the tile to check for fences
 * @param side the border to check
 * @pre IsFieldsTile(t)
 * @return 0 if there is no fence, otherwise the fence type
 */
static inline uint GetFence(TileIndex t, DiagDirection side)
{
	return tile_get_field_fence(&_mc[t], side);
}

/**
 * Sets the type of fence (and whether there is one) for the given border.
 * @param t the tile to check for fences
 * @param side the border to check
 * @param h 0 if there is no fence, otherwise the fence type
 * @pre IsFieldsTile(t)
 */
static inline void SetFence(TileIndex t, DiagDirection side, uint h)
{
	tile_set_field_fence(&_mc[t], side, h);
}


/**
 * Make a nice void tile ;)
 * @param t the tile to make void
 */
static inline void MakeVoid(TileIndex t)
{
	SetTileHeight(t, 0);
	tile_make_void(&_mc[t]);
}


/**
 * Make a clear tile.
 * @param t       the tile to make a clear tile
 * @param g       the type of ground
 * @param density the density of the grass/snow/desert etc
 */
static inline void MakeClear(TileIndex t, Ground g, uint density)
{
	tile_make_clear(&_mc[t], g, density);
}


/**
 * Make a (farm) field tile.
 * @param t          the tile to make a farm field
 * @param field_type the 'growth' level of the field
 * @param industry   the industry this tile belongs to
 */
static inline void MakeField(TileIndex t, uint field_type, IndustryID industry)
{
	tile_make_field(&_mc[t], field_type, industry);
}

/**
 * Make a snow tile.
 * @param t the tile to make snowy
 * @param density The density of snowiness.
 */
static inline void MakeSnow(TileIndex t, uint density = 0)
{
	tile_make_snow(&_mc[t], density);
}

/**
 * Clear the snow from a tile and return it to its previous type.
 * @param t the tile to clear of snow
 * @pre IsSnowTile(t)
 */
static inline void ClearSnow(TileIndex t)
{
	tile_clear_snow(&_mc[t]);
}

#endif /* CLEAR_MAP_H */
