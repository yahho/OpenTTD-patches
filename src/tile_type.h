/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile_type.h Types related to tiles. */

#ifndef TILE_TYPE_H
#define TILE_TYPE_H

static const uint TILE_SIZE      = 16;            ///< Tiles are 16x16 "units" in size
static const uint TILE_UNIT_MASK = TILE_SIZE - 1; ///< For masking in/out the inner-tile units.
static const uint TILE_PIXELS    = 32;            ///< a tile is 32x32 pixels
static const uint TILE_HEIGHT    =  8;            ///< The standard height-difference between tiles on two levels is 8 (z-diff 8)

static const uint MAX_TILE_HEIGHT     = 15;                    ///< Maximum allowed tile height

static const uint MIN_SNOWLINE_HEIGHT = 2;                     ///< Minimum snowline height
static const uint DEF_SNOWLINE_HEIGHT = 7;                     ///< Default snowline height
static const uint MAX_SNOWLINE_HEIGHT = (MAX_TILE_HEIGHT - 2); ///< Maximum allowed snowline height


/**
 * The different types of tiles.
 *
 * Each tile belongs to one type, according whatever is build on it.
 *
 * @note A railway with a crossing street is marked as road.
 */
enum TileType {
	TT_GROUND       =  0,  ///< A tile without any structures, i.e. grass, rocks, farm fields, trees etc.; or void
	TT_OBJECT       =  1,  ///< Contains objects such as transmitters and owned land
	TT_WATER        =  2,  ///< Water tile
	TT_RAILWAY      =  4,  ///< A railway
	TT_ROAD         =  5,  ///< A tile with road (or tram tracks)
	TT_MISC         =  6,  ///< Level crossings, aqueducts, tunnels, depots
	TT_STATION      =  7,  ///< A tile of a station
	TT_INDUSTRY_TEMP     =  8,
	TT_VOID_TEMP         =  9,
	TT_TUNNELBRIDGE_TEMP = 11,
	//TT_INDUSTRY   =  8,  ///< Part of an industry
	//TT_HOUSE      = 12,  ///< A house by a town
};

/**
 * Subtypes of certain tile types.
 *
 * Each subtype only makes sense for certain types, normally just one.
 */
enum TileSubtype {
	TT_GROUND_VOID   = 0,  ///< Void tile (TT_GROUND)
	TT_GROUND_FIELDS = 1,  ///< Fields (TT_GROUND)
	TT_GROUND_CLEAR  = 2,  ///< Clear (neither fields nor trees) (TT_GROUND)
	TT_GROUND_TREES  = 3,  ///< Trees (TT_GROUND)
	TT_TRACK         = 0,  ///< Railway track or normal road (TT_RAILWAY, TT_ROAD)
	TT_BRIDGE        = 1,  ///< Bridge ramp/bridgehead (TT_RAILWAY, TT_ROAD)
	TT_MISC_CROSSING = 0,  ///< Level crossing (TT_MISC)
	TT_MISC_AQUEDUCT = 1,  ///< Aqueduct (TT_MISC)
	TT_MISC_TUNNEL   = 2,  ///< Tunnel entry (TT_MISC)
	TT_MISC_DEPOT    = 3,  ///< Railway or road depot (TT_MISC)
};

/**
 * Check whether a given tile type has subtypes.
 *
 * @param tt The type to check
 * @return Whether the given type has subtypes
 */
static inline bool TileTypeHasSubtypes(TileType tt)
{
	return (tt == TT_GROUND) || (tt == TT_RAILWAY) || (tt == TT_ROAD) || (tt == TT_MISC);
}

/**
 * Additional infos of a tile on a tropic game.
 *
 * The tropiczone is not modified during gameplay. It mainly affects tree growth. (desert tiles are visible though)
 *
 * In randomly generated maps:
 *  TROPICZONE_DESERT: Generated everywhere, if there is neither water nor mountains (TileHeight >= 4) in a certain distance from the tile.
 *  TROPICZONE_RAINFOREST: Generated everywhere, if there is no desert in a certain distance from the tile.
 *  TROPICZONE_NORMAL: Everywhere else, i.e. between desert and rainforest and on sea (if you clear the water).
 *
 * In scenarios:
 *  TROPICZONE_NORMAL: Default value.
 *  TROPICZONE_DESERT: Placed manually.
 *  TROPICZONE_RAINFOREST: Placed if you plant certain rainforest-trees.
 */
enum TropicZone {
	TROPICZONE_NORMAL     = 0,      ///< Normal tropiczone
	TROPICZONE_DESERT     = 1,      ///< Tile is desert
	TROPICZONE_RAINFOREST = 2,      ///< Rainforest tile
};

/**
 * The index/ID of a Tile.
 */
typedef uint32 TileIndex;

/**
 * The very nice invalid tile marker
 */
static const TileIndex INVALID_TILE = (TileIndex)-1;

#endif /* TILE_TYPE_H */
