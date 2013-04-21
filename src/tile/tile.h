/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/tile.h Types related to tiles. */

#ifndef TILE_TILE_H
#define TILE_TILE_H

#include "../stdafx.h"

/**
 * The different types a tile can have.
 *
 * Each tile belongs to one type, according to whatever is built on it.
 */
enum TileType {
	TT_GROUND       =  0,  ///< A tile without any structures, i.e. grass, rocks, farm fields, trees etc.; or void
	TT_OBJECT       =  1,  ///< Contains objects such as transmitters and owned land
	TT_WATER        =  2,  ///< Water tile
	TT_RAILWAY      =  4,  ///< A railway
	TT_ROAD         =  5,  ///< A tile with road (or tram tracks)
	TT_MISC         =  6,  ///< Level crossings, aqueducts, tunnels, depots
	TT_STATION      =  7,  ///< A tile of a station
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
static inline bool tiletype_has_subtypes(TileType tt)
{
	return (tt == TT_GROUND) || (tt == TT_RAILWAY) || (tt == TT_ROAD) || (tt == TT_MISC);
}

/**
 * Contents of a tile.
 * Look at docs/landscape.html for the exact meaning of the members.
 */
struct Tile {
	byte   m0;          ///< Primarily used for tile class and bridges
	byte   m1;          ///< Primarily used for tile class, water class and ownership information
	uint16 m2;          ///< Primarily used for indices to towns, industries and stations
	byte   m3;          ///< General purpose
	byte   m4;          ///< General purpose
	byte   m5;          ///< General purpose
	byte   m7;          ///< Primarily used for newgrf support
};

#endif /* TILE_TILE_H */
