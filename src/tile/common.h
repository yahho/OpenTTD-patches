/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/common.h Miscellaneous tile content functions. */

#ifndef TILE_COMMON_H
#define TILE_COMMON_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "../company_type.h"
#include "../direction_type.h"

/**
 * Get the owner of a tile
 * @param t The tile whose owner to get
 * @return The owner of the tile
 */
static inline Owner tile_get_owner(const Tile *t)
{
	assert(!tile_is_void(t));
	assert(!tile_is_industry(t));
	assert(!tile_is_house(t));

	return (Owner)GB(t->m1, 0, 5);
}

/**
 * Set the owner of a tile
 * @param t The tile whose owner to set
 * @param owner The owner to set
 */
static inline void tile_set_owner(Tile *t, Owner owner)
{
	assert(!tile_is_void(t));
	assert(!tile_is_industry(t));
	assert(!tile_is_house(t));

	SB(t->m1, 0, 5, owner);
}

/**
 * Check if a tile belongs to a given owner
 * @param t The tile to check
 * @param owner The owner to check against
 * @return Whether the tile belongs to the given owner
 */
static inline bool tile_is_owner(const Tile *t, Owner owner)
{
	return tile_get_owner(t) == owner;
}


/**
 * Check if a tile is on snow/desert (for certain tile types)
 * @param t The tile to check
 * @return Whether the tile is on snow/desert
 */
static inline bool tile_get_snow(const Tile *t)
{
	assert((tile_is_railway(t) && !tile_is_subtype(t, TT_TRACK)) ||
		tile_is_road(t) || tile_is_type(t, TT_MISC));
	return HasBit(t->m3, 4);
}

#define tile_get_desert tile_get_snow

/**
 * Set if a tile is on snow/desert (for certain tile types)
 * @param t The tile to set
 * @param set Whether to set snow/desert
 */
static inline void tile_set_snow(Tile *t, bool set)
{
	assert((tile_is_railway(t) && !tile_is_subtype(t, TT_TRACK)) ||
		tile_is_road(t) || tile_is_type(t, TT_MISC));
	if (set) {
		SetBit(t->m3, 4);
	} else {
		ClrBit(t->m3, 4);
	}
}

#define tile_set_desert tile_set_snow

/**
 * Toggle snow/desert for a tile (for certain tile types)
 * @param t The tile to toggle
 */
static inline void tile_toggle_snow(Tile *t)
{
	assert((tile_is_railway(t) && !tile_is_subtype(t, TT_TRACK)) ||
		tile_is_road(t) || tile_is_type(t, TT_MISC));
	ToggleBit(t->m3, 4);
}

#define tile_toggle_desert tile_toggle_snow


/**
 * Get the the direction a bridge ramp or tunnel entrance heads to
 * @param t The bridge or tunnel tile
 * @return The direction into the bridge or tunnel
 */
static inline DiagDirection tile_get_tunnelbridge_direction(const Tile *t)
{
	assert(tile_is_bridge(t) || tile_is_tunnel(t));
	return (DiagDirection)GB(t->m3, 6, 2);
}


/**
 * Get the random bits of a tile
 * @param t The tile whose random bits to get
 * @pre tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_water(t)
 * @return The random bits of the tile
 */
static inline byte tile_get_random_bits(const Tile *t)
{
	assert(tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_water(t));
	return t->m3;
}

/**
 * Set the random bits of a tile
 * @param t The tile whose random bits to set
 * @param random The random bits to set
 * @pre tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_water(t)
 */
static inline void tile_set_random_bits(Tile *t, byte random)
{
	assert(tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_water(t));
	t->m3 = random;
}


/**
 * Get the current animation frame of a tile
 * @param t The tile whose frame to get
 * @pre tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_station(t)
 * @return The animation frame of the tile
 */
static inline byte tile_get_frame(const Tile *t)
{
	assert(tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_station(t));
	return t->m7;
}

/**
 * Set the animation frame of a tile
 * @param t The tile whose frame to set
 * @param frame The frame to set
 * @pre tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_station(t)
 */
static inline void tile_set_frame(Tile *t, byte frame)
{
	assert(tile_is_house(t) || tile_is_object(t) || tile_is_industry(t) || tile_is_station(t));
	t->m7 = frame;
}


/**
 * Check if this tile can have a bridge above
 * @param t The tile to check
 * @return Whether a bridge can be built over this tile
 */
static inline bool tile_is_bridgeable(const Tile *t)
{
	switch (tile_get_type(t)) {
		case TT_GROUND: return tile_is_subtype(t, TT_GROUND_FIELDS) || tile_is_subtype(t, TT_GROUND_CLEAR);
		case TT_OBJECT:
		case TT_WATER:
		case TT_RAILWAY:
		case TT_ROAD:   return true;
		case TT_MISC:   return !tile_is_subtype(t, TT_MISC_DEPOT);
		default:        return false;
	}
}

/**
 * Check if this bridgeable tile has a bridge above
 * @param t The tile to check
 * @pre tile_is_bridgeable(t)
 * @return Whether there is a bridge above the tile
 */
static inline bool tile_bridgeable_has_bridge(const Tile *t)
{
	assert(tile_is_bridgeable(t));
	return GB(t->m0, 0, 2) != 0;
}

/**
 * Check if this tile has a bridge
 * @param t The tile to check
 * @return Whether there is a bridge above the tile
 */
static inline bool tile_has_bridge_above(const Tile *t)
{
	return tile_is_bridgeable(t) && tile_bridgeable_has_bridge(t);
}

/**
 * Get the axis of the bridge over a tile
 * @param t The tile
 * @pre tile_has_bridge_above(t)
 * @return The axis of the bridge
 */
static inline Axis tile_get_bridge_axis(const Tile *t)
{
	assert(tile_has_bridge_above(t));
	return (Axis)(GB(t->m0, 0, 2) - 1);
}

/**
 * Remove the bridge over a tile
 * @param t The tile
 * @pre tile_is_bridgeable(t)
 */
static inline void tile_clear_bridge_above(Tile *t)
{
	assert(tile_is_bridgeable(t));
	SB(t->m0, 0, 2, 0);
}

/**
 * Set a bridge over a tile
 * @param t The tile
 * @param a Axis of the bridge
 * @pre tile_is_bridgeable(t)
 * @pre !tile_bridgeable_has_bridge(t)
 */
static inline void tile_set_bridge_above(Tile *t, Axis a)
{
	assert(tile_is_bridgeable(t));
	assert(!tile_bridgeable_has_bridge(t));
	SB(t->m0, 0, 2, 1 << a);
}

#endif /* TILE_COMMON_H */
