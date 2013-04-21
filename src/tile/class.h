/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/class.h Tile classes. */

#ifndef TILE_CLASS_H
#define TILE_CLASS_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"

/**
 * Get the tile type of a tile
 * @param t The tile whose type to get
 * @return The type of the tile
 */
static inline TileType tile_get_type(const Tile *t)
{
	return (TileType)GB(t->m0, 4, 4);
}

/**
 * Set the tile type of a tile
 * @param t The tile whose type to set
 * @param tt The type to set
 */
static inline void tile_set_type(Tile *t, TileType tt)
{
	assert(tt < 8); // only some tile types make sense
	SB(t->m0, 4, 4, tt);
}

/**
 * Get the tile subtype of a tile
 * @param t The tile whose subtype to get
 * @return The subtype of the tile
 */
static inline TileSubtype tile_get_subtype(const Tile *t)
{
	assert(tiletype_has_subtypes(tile_get_type(t)));
	return (TileSubtype)GB(t->m1, 6, 2);
}

/**
 * Set the tile subtype of a tile
 * @param t The tile whose subtype to set
 * @param ts The subtype to set
 */
static inline void tile_set_subtype(Tile *t, TileSubtype ts)
{
	assert(tiletype_has_subtypes(tile_get_type(t)));
	SB(t->m1, 6, 2, ts);
}

/**
 * Set the tile type and subtype of a tile
 * @param t The tile whose type and subtype to set
 * @param tt The type to set
 * @param ts The subtype to set
 */
static inline void tile_set_type_subtype(Tile *t, TileType tt, TileSubtype ts)
{
	assert(tiletype_has_subtypes(tt));
	SB(t->m0, 4, 4, tt);
	SB(t->m1, 6, 2, ts);
}

/**
 * Check if a tile is of a given type
 * @param t The tile to check
 * @param tt The type to check against
 * @return Whether the tile is of the given type
 */
static inline bool tile_is_type(const Tile *t, TileType tt)
{
	return tile_get_type(t) == tt;
}

/**
 * Check if a tile is of a given subtype
 * @param t The tile to check
 * @param ts The subtype to check against
 * @return Whether the tile is of the given subtype
 */
static inline bool tile_is_subtype(const Tile *t, TileSubtype ts)
{
	return tile_get_subtype(t) == ts;
}

/**
 * Check if a tile is of given type and subtype
 * @param t The tile to check
 * @param tt The type to check against
 * @param ts The subtype to checck against
 * @return Whether the tile is of given type and subtype
 */
static inline bool tile_is_type_subtype(const Tile *t, TileType tt, TileSubtype ts)
{
	return tile_is_type(t, tt) && tile_is_subtype(t, ts);
}

/**
 * Check if a tile is a void tile
 * @param t The tile to check
 * @return Whether the tile is a void tile
 */
static inline bool tile_is_void(const Tile *t)
{
	return tile_is_type_subtype(t, TT_GROUND, TT_GROUND_VOID);
}

/**
 * Check if a tile is a ground tile (but not void)
 * @param t The tile to check
 * @return Whether the tile is a (non-void) ground tile
 */
static inline bool tile_is_ground(const Tile *t)
{
	return tile_is_type(t, TT_GROUND) && !tile_is_subtype(t, TT_GROUND_VOID);
}

/**
 * Check if a tile is a fields tile
 * @param t The tile to check
 * @return Whether the tile has fields
 */
static inline bool tile_is_fields(const Tile *t)
{
	return tile_is_type_subtype(t, TT_GROUND, TT_GROUND_FIELDS);
}

/**
 * Check if a tile is clear
 * @param t The tile to check
 * @return Whether the tile is clear
 */
static inline bool tile_is_clear(const Tile *t)
{
	return tile_is_type_subtype(t, TT_GROUND, TT_GROUND_CLEAR);
}

/**
 * Check if a tile is a tree tile
 * @param t The tile to check
 * @return Whether the tile has trees
 */
static inline bool tile_is_trees(const Tile *t)
{
	return tile_is_type_subtype(t, TT_GROUND, TT_GROUND_TREES);
}

/**
 * Check if a tile is an object tile
 * @param t The tile to check
 * @return Whether the tile is an object
 */
static inline bool tile_is_object(const Tile *t)
{
	return tile_is_type(t, TT_OBJECT);
}

/**
 * Check if a tile is a water tile
 * @param t The tile to check
 * @return Whether the tile is a water tile
 */
static inline bool tile_is_water(const Tile *t)
{
	return tile_is_type(t, TT_WATER);
}

/**
 * Check if a tile is a railway tile
 * @param t The tile to check
 * @return Whether the tile is a railway tile
 */
static inline bool tile_is_railway(const Tile *t)
{
	return tile_is_type(t, TT_RAILWAY);
}

/**
 * Check if a tile is a rail track tile
 * @param t The tile to check
 * @return Whether the tile is a rail track tile
 */
static inline bool tile_is_rail_track(const Tile *t)
{
	return tile_is_type_subtype(t, TT_RAILWAY, TT_TRACK);
}

/**
 * Check if a tile is a rail bridge tile
 * @param t The tile to check
 * @return Whether the tile is a rail bridge tile
 */
static inline bool tile_is_rail_bridge(const Tile *t)
{
	return tile_is_type_subtype(t, TT_RAILWAY, TT_BRIDGE);
}

/**
 * Check if a tile is a road tile
 * @param t The tile to check
 * @return Whether the tile is a road tile
 */
static inline bool tile_is_road(const Tile *t)
{
	return tile_is_type(t, TT_ROAD);
}

/**
 * Check if a tile is road track tile (normal road)
 * @param t The tile to check
 * @return Whether the tile is normal road tile
 */
static inline bool tile_is_road_track(const Tile *t)
{
	return tile_is_type_subtype(t, TT_ROAD, TT_TRACK);
}

/**
 * Check if a tile is road bridge tile
 * @param t The tile to check
 * @return Whether the tile is road bridge tile
 */
static inline bool tile_is_road_bridge(const Tile *t)
{
	return tile_is_type_subtype(t, TT_ROAD, TT_BRIDGE);
}

/**
 * Check if a tile is a level crossing tile
 * @param t The tile to check
 * @return Whether the tile is a level crossing
 */
static inline bool tile_is_crossing(const Tile *t)
{
	return tile_is_type_subtype(t, TT_MISC, TT_MISC_CROSSING);
}

/**
 * Check if a tile is an aqueduct tile
 * @param t The tile to check
 * @return Whether the tile is an aqueduct
 */
static inline bool tile_is_aqueduct(const Tile *t)
{
	return tile_is_type_subtype(t, TT_MISC, TT_MISC_AQUEDUCT);
}

/**
 * Check if a tile is a tunnel tile
 * @param t The tile to check
 * @return Whether the tile is a tunnel entrance
 */
static inline bool tile_is_tunnel(const Tile *t)
{
	return tile_is_type_subtype(t, TT_MISC, TT_MISC_TUNNEL);
}

/**
 * Check if a tile is a ground (rail or road) depot tile
 * @param t The tile to check
 * @return Whether the tile is ground depot
 */
static inline bool tile_is_ground_depot(const Tile *t)
{
	return tile_is_type_subtype(t, TT_MISC, TT_MISC_DEPOT);
}

/**
 * Check if a tile is a station tile
 * @param t The tile to check
 * @return Whether the tile is a station
 */
static inline bool tile_is_station(const Tile *t)
{
	return tile_is_type(t, TT_STATION);
}

/**
 * Check if a tile is an industry tile
 * @param t The tile to check
 * @return Whether the tile is an industry
 */
static inline bool tile_is_industry(const Tile *t)
{
	return GB(t->m0, 6, 2) == 2;
}

/**
 * Check if a tile is a house tile
 * @param t The tile to check
 * @return Whether the tile is a house
 */
static inline bool tile_is_house(const Tile *t)
{
	return GB(t->m0, 6, 2) == 3;
}

/**
 * Check if a tile is a bridge tile (rail tunnel, road tunnel, aqueduct)
 * @param t The tile to check
 * @return Whether the tile is a bridge ramp
 */
static inline bool tile_is_bridge(const Tile *t)
{
	assert_compile(TT_RAILWAY == 4);
	assert_compile(TT_ROAD == 5);
	assert_compile(TT_MISC == 6);
	assert_compile(TT_MISC_AQUEDUCT == TT_BRIDGE);

	return (((uint)tile_get_type(t)) - 4) < 3 && tile_get_subtype(t) == TT_BRIDGE;
}

#endif /* TILE_CLASS_H */
