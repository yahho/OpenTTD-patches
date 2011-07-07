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

#include "bridge_map.h"
#include "industry_type.h"

/**
 * Ground types. Valid densities in comments after the enum.
 */
enum Ground {
	GROUND_GRASS  = 0, ///< 0-3
	GROUND_SHORE  = 1, ///< 3
	GROUND_ROUGH  = 2, ///< 3
	GROUND_ROCKS  = 3, ///< 3
	GROUND_DESERT = 4, ///< 1,3
	GROUND_SNOW        =  8, ///< 0-3
	GROUND_SNOW_ROUGH  = 10, ///< 0-3
	GROUND_SNOW_ROCKS  = 11, ///< 0-3
};


/**
 * Check if a tile is ground (but not void).
 * @param t the tile to check
 * @return whether the tile is ground (fields, clear or trees)
 */
static inline bool IsGroundTile(TileIndex t)
{
	return IsTileType(t, TT_GROUND) && !IsTileSubtype(t, TT_GROUND_VOID);
}


/**
 * Check if a tile is empty ground.
 * @param t the tile to check
 * @return whether the tile is empty ground
 */
static inline bool IsClearTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_GROUND, TT_GROUND_CLEAR);
}

/**
 * Checks if a tile has trees.
 * @param t the tile to check
 * @return whether the tile has trees
 */
static inline bool IsTreeTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_GROUND, TT_GROUND_TREES);
}


/**
 * Get the full ground type of a clear tile.
 * @param t the tile to get the ground type of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the ground type
 */
static inline Ground GetFullClearGround(TileIndex t)
{
	assert(IsClearTile(t) || IsTreeTile(t));
	return (Ground)GB(_mc[t].m3, 4, 4);
}

/**
 * Test if a tile is covered with snow.
 * @param t the tile to check
 * @pre IsGroundTile(t)
 * @return whether the tile is covered with snow.
 */
static inline bool IsSnowTile(TileIndex t)
{
	assert(IsGroundTile(t));
	return !IsTileSubtype(t, TT_GROUND_FIELDS) && (GetFullClearGround(t) >= GROUND_SNOW);
}

/**
 * Get the tile ground ignoring snow.
 * @param t the tile to get the clear ground type of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the ground type
 */
static inline Ground GetRawClearGround(TileIndex t)
{
	assert(IsClearTile(t) || IsTreeTile(t));
	return (Ground)(GetFullClearGround(t) & 0x7);
}

/**
 * Get the tile ground, treating all snow types as equal.
 * @param t the tile to get the clear ground type of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the ground type
 */
static inline Ground GetClearGround(TileIndex t)
{
	assert(IsClearTile(t) || IsTreeTile(t));
	Ground g = GetFullClearGround(t);
	return (g >= GROUND_SNOW) ? GROUND_SNOW : g;
}

/**
 * Set the tile ground.
 * @param t  the tile to set the clear ground type of
 * @param ct the ground type
 * @pre IsClearTile(t) || IsTreeTile(t)
 */
static inline bool IsClearGround(TileIndex t, Ground g)
{
	assert(IsClearTile(t) || IsTreeTile(t));
	return GetClearGround(t) == g;
}


/**
 * Get the density of a non-field clear tile.
 * @param t the tile to get the density of
 * @pre IsClearTile(t) || IsTreeTile(t)
 * @return the density
 */
static inline uint GetClearDensity(TileIndex t)
{
	assert(IsClearTile(t) || IsTreeTile(t));
	return GB(_mc[t].m4, 0, 2);
}

/**
 * Increment the density of a non-field clear tile.
 * @param t the tile to increment the density of
 * @param d the amount to increment the density with
 * @pre IsClearTile(t) || IsTreeTile(t)
 */
static inline void AddClearDensity(TileIndex t, int d)
{
	assert(IsClearTile(t) || IsTreeTile(t));
	_mc[t].m4 += d;
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
	assert(IsClearTile(t) || IsTreeTile(t));
	if (keep_counter) {
		SB(_mc[t].m3, 4, 4, g);
	} else {
		_mc[t].m3 = g << 4;
	}
	SB(_mc[t].m4, 0, 2, density);
}


/**
 * Checks if a tile has fields.
 *
 * @param tile The tile to check
 * @return true If the tile has fields
 */
static inline bool IsFieldsTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_GROUND, TT_GROUND_FIELDS);
}

/**
 * Get the field type (production stage) of the field
 * @param t the field to get the type of
 * @pre IsFieldsTile(t)
 * @return the field type
 */
static inline uint GetFieldType(TileIndex t)
{
	assert(IsFieldsTile(t));
	return GB(_mc[t].m3, 4, 4);
}

/**
 * Set the field type (production stage) of the field
 * @param t the field to get the type of
 * @param f the field type
 * @pre IsFieldsTile(t)
 */
static inline void SetFieldType(TileIndex t, uint f)
{
	assert(IsFieldsTile(t));
	SB(_mc[t].m3, 4, 4, f);
}

/**
 * Get the industry (farm) that made the field
 * @param t the field to get creating industry of
 * @pre IsFieldsTile(t)
 * @return the industry that made the field
 */
static inline IndustryID GetIndustryIndexOfField(TileIndex t)
{
	assert(IsFieldsTile(t));
	return(IndustryID) _mc[t].m2;
}

/**
 * Set the industry (farm) that made the field
 * @param t the field to get creating industry of
 * @param i the industry that made the field
 * @pre IsFieldsTile(t)
 */
static inline void SetIndustryIndexOfField(TileIndex t, IndustryID i)
{
	assert(IsFieldsTile(t));
	_mc[t].m2 = i;
}


/**
 * Get the counter used to advance to the next clear density/field type.
 * @param t the tile to get the counter of
 * @pre IsGroundTile(t)
 * @return the value of the counter
 */
static inline uint GetClearCounter(TileIndex t)
{
	assert(IsGroundTile(t));
	return GB(_mc[t].m3, 0, 4);
}

/**
 * Increments the counter used to advance to the next clear density/field type.
 * @param t the tile to increment the counter of
 * @param c the amount to increment the counter with
 * @pre IsGroundTile(t)
 */
static inline void AddClearCounter(TileIndex t, int c)
{
	assert(IsGroundTile(t));
	_mc[t].m3 += c;
}

/**
 * Sets the counter used to advance to the next clear density/field type.
 * @param t the tile to set the counter of
 * @param c the amount to set the counter to
 * @pre IsClearTile(t) || IsFieldsTile(t)
 */
static inline void SetClearCounter(TileIndex t, uint c)
{
	assert(IsGroundTile(t));
	SB(_mc[t].m3, 0, 4, c);
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
	assert(IsFieldsTile(t));
	switch (side) {
		default: NOT_REACHED();
		case DIAGDIR_SE: return GB(_mc[t].m4, 2, 3);
		case DIAGDIR_SW: return GB(_mc[t].m4, 5, 3);
		case DIAGDIR_NE: return GB(_mc[t].m5, 5, 3);
		case DIAGDIR_NW: return GB(_mc[t].m5, 2, 3);
	}
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
	assert(IsFieldsTile(t));
	switch (side) {
		default: NOT_REACHED();
		case DIAGDIR_SE: SB(_mc[t].m4, 2, 3, h); break;
		case DIAGDIR_SW: SB(_mc[t].m4, 5, 3, h); break;
		case DIAGDIR_NE: SB(_mc[t].m5, 5, 3, h); break;
		case DIAGDIR_NW: SB(_mc[t].m5, 2, 3, h); break;
	}
}


/**
 * Make a clear tile.
 * @param t       the tile to make a clear tile
 * @param g       the type of ground
 * @param density the density of the grass/snow/desert etc
 */
static inline void MakeClear(TileIndex t, Ground g, uint density)
{
	/* If this is a non-bridgeable tile, clear the bridge bits while the rest
	 * of the tile information is still here. */
	if (!MayHaveBridgeAbove(t)) SB(_mc[t].m0, 0, 2, 0);

	SetTileType(t, TT_GROUND);
	SB(_mc[t].m0, 2, 2, 0);
	_mc[t].m1 = TT_GROUND_CLEAR << 6;
	SetTileOwner(t, OWNER_NONE);
	_mc[t].m2 = 0;
	_mc[t].m3 = g << 4;
	_mc[t].m4 = density;
	_mc[t].m5 = 0;
	_mc[t].m7 = 0;
}


/**
 * Make a (farm) field tile.
 * @param t          the tile to make a farm field
 * @param field_type the 'growth' level of the field
 * @param industry   the industry this tile belongs to
 */
static inline void MakeField(TileIndex t, uint field_type, IndustryID industry)
{
	SetTileType(t, TT_GROUND);
	SB(_mc[t].m0, 2, 2, 0);
	_mc[t].m1 = TT_GROUND_FIELDS << 6;
	SetTileOwner(t, OWNER_NONE);
	_mc[t].m2 = industry;
	_mc[t].m3 = field_type << 4;
	_mc[t].m4 = 0;
	_mc[t].m5 = 0;
	_mc[t].m7 = 0;
}

/**
 * Make a snow tile.
 * @param t the tile to make snowy
 * @param density The density of snowiness.
 */
static inline void MakeSnow(TileIndex t, uint density = 0)
{
	if (IsTileSubtype(t, TT_GROUND_FIELDS)) {
		MakeClear(t, GROUND_SNOW, density);
	} else {
		SetClearGroundDensity(t, (Ground)(GetFullClearGround(t) | GROUND_SNOW), density, true);
	}
}

/**
 * Clear the snow from a tile and return it to its previous type.
 * @param t the tile to clear of snow
 * @pre IsSnowTile(t)
 */
static inline void ClearSnow(TileIndex t)
{
	assert(IsSnowTile(t));
	SetClearGroundDensity(t, (Ground)(GetFullClearGround(t) & ~GROUND_SNOW), 3, true);
}

#endif /* CLEAR_MAP_H */
