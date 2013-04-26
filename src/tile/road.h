/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/road.h Tile functions for road tiles. */

#ifndef TILE_ROAD_H
#define TILE_ROAD_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "common.h"
#include "misc.h"
#include "../direction_type.h"
#include "../direction_func.h"
#include "../road_type.h"
#include "../company_type.h"

/**
 * Get the road bits of a tile for a roadtype
 * @param t The tile whose road bits to get
 * @param rt The road type whose bits to get
 * @pre tile_is_road(t)
 * @return The road bits of the tile for the given roadtype
 */
static inline RoadBits tile_get_roadbits(const Tile *t, RoadType rt)
{
	assert(tile_is_road(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: return (RoadBits)GB(t->m4, 0, 4);
		case ROADTYPE_TRAM: return (RoadBits)GB(t->m4, 4, 4);
	}
}

/**
 * Get the road bits of a tile for all roadtypes
 * @param t The tile whose road bits to get
 * @pre tile_is_road(t)
 * @return The combined road bits of the tile
 */
static inline RoadBits tile_get_all_roadbits(const Tile *t)
{
	assert(tile_is_road(t));
	return (RoadBits)(GB(t->m4, 0, 4) | GB(t->m4, 4, 4));
}

/**
 * Set the road bits of a tile for a roadtype
 * @param t The tile whose road bits to set
 * @param rt The road type whose bits to set
 * @param roadbits The road bits to set
 * @pre tile_is_road(t)
 */
static inline void tile_set_roadbits(Tile *t, RoadType rt, RoadBits roadbits)
{
	assert(tile_is_road(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: SB(t->m4, 0, 4, roadbits); break;
		case ROADTYPE_TRAM: SB(t->m4, 4, 4, roadbits); break;
	}
}


/**
 * Get the road types present at a tile
 * @param t The tile whose road types to get
 * @pre tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_road_depot(t) || tile_is_station(t)
 * @return The road types present at the tile
 */
static inline RoadTypes tile_get_roadtypes(const Tile *t)
{
	assert(tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_road_depot(t) || tile_is_station(t));
	return (RoadTypes)GB(t->m7, 6, 2);
}

/**
 * Set the road types present at a tile
 * @param t The tile whose road types to set
 * @param rts The road types to set
 * @pre tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_station(t)
 */
static inline void tile_set_roadtypes(Tile *t, RoadTypes rts)
{
	assert(tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_station(t));
	SB(t->m7, 6, 2, rts);
}

/**
 * Check if a tile has a given roadtype
 * @param t The tile to check
 * @param rt The road type to check against
 * @pre tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_road_depot(t) || tile_is_station(t)
 * @return Whether the tile has the given roadtype
 */
static inline bool tile_has_roadtype(const Tile *t, RoadType rt)
{
	return HasBit(tile_get_roadtypes(t), rt);
}


/**
 * Get the owner of a road type on a tile
 * @param t The tile whose road to get the owner of
 * @param rt The roadtype whose owner to get
 * @pre tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_station(t)
 * @return The owner of the given road type
 */
static inline Owner tile_get_road_owner(const Tile *t, RoadType rt)
{
	assert(tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_station(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: return (Owner)GB(tile_is_road(t) ? t->m1 : t->m7, 0, 5);
		case ROADTYPE_TRAM: {
			/* OWNER_NONE is stored as OWNER_TOWN for trams */
			Owner o = (Owner)(tile_is_station(t) ? GB(t->m3, 4, 4) : GB(t->m5, 0, 4));
			return o == OWNER_TOWN ? OWNER_NONE : o;
		}
	}
}

/**
 * Set the owner of a road type on a tile
 * @param t The tile whose road to set the owner of
 * @param rt The roadtype whose owner to set
 * @param o The owner to set
 * @pre tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_station(t)
 */
static inline void tile_set_road_owner(Tile *t, RoadType rt, Owner o)
{
	assert(tile_is_road(t) || tile_is_crossing(t) || tile_is_road_tunnel(t) || tile_is_station(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: SB(tile_is_road(t) ? t->m1 : t->m7, 0, 5, o); break;
		case ROADTYPE_TRAM:
			/* OWNER_NONE is stored as OWNER_TOWN for trams */
			if (o == OWNER_NONE) o = OWNER_TOWN;
			if (tile_is_station(t)) {
				SB(t->m3, 4, 4, o);
			} else {
				SB(t->m5, 0, 4, o);
			}
			break;
	}
}


/** The possible road side decorations. */
enum Roadside {
	ROADSIDE_BARREN           = 0, ///< Road on barren land
	ROADSIDE_GRASS            = 1, ///< Road on grass
	ROADSIDE_PAVED            = 2, ///< Road with paved sidewalks
	ROADSIDE_STREET_LIGHTS    = 3, ///< Road with street lights on paved sidewalks
	ROADSIDE_TREES            = 5, ///< Road with trees on paved sidewalks
	ROADSIDE_GRASS_ROAD_WORKS = 6, ///< Road on grass with road works
	ROADSIDE_PAVED_ROAD_WORKS = 7, ///< Road with sidewalks and road works
};

/**
 * Get the road decorations of a tile
 * @param t The tile whose decorations to get
 * @pre tile_is_road_track(t) || tile_is_crossing(t)
 * @return The decorations of the tile
 */
static inline Roadside tile_get_roadside(const Tile *t)
{
	assert(tile_is_road_track(t) || tile_is_crossing(t));
	return (Roadside)GB(t->m5, 4, 3);
}

/**
 * Set the road decorations of a tile
 * @param t The tile whose decorations to set
 * @param s The decorations to set
 * @pre tile_is_road_track(t) || tile_is_crossing(t)
 */
static inline void tile_set_roadside(Tile *t, Roadside s)
{
	assert(tile_is_road_track(t) || tile_is_crossing(t));
	SB(t->m5, 4, 3, s);
}

/**
 * Check if a road tile has road works in progress
 * @param t The tile to check
 * @pre tile_is_road_track(t)
 * @return Whether there are road works currently on the tile
 */
static inline bool tile_has_roadworks(const Tile *t)
{
	return tile_get_roadside(t) >= ROADSIDE_GRASS_ROAD_WORKS;
}

/**
 * Reset the road works counter of a tile
 * @param t The tile whose road works counter to reset
 * @pre tile_is_road_track(t)
 */
static inline void tile_reset_roadworks(Tile *t)
{
	assert(tile_is_road_track(t));
	SB(t->m7, 0, 4, 0);
}

/**
 * Increment the road works counter of a tile
 * @param t The tile whose road works counter to increment
 * @pre tile_is_road_track(t)
 * @return Whether the counter has reached its maximum value
 */
static inline bool tile_inc_roadworks(Tile *t)
{
	AB(t->m7, 0, 4, 1);

	return GB(t->m7, 0, 4) == 15;
}


/** Which directions are disallowed ? */
enum DisallowedRoadDirections {
	DRD_NONE,       ///< None of the directions are disallowed
	DRD_SOUTHBOUND, ///< All southbound traffic is disallowed
	DRD_NORTHBOUND, ///< All northbound traffic is disallowed
	DRD_BOTH,       ///< All directions are disallowed
	DRD_END,        ///< Sentinel
};
DECLARE_ENUM_AS_BIT_SET(DisallowedRoadDirections)
/** Helper information for extract tool. */
template <> struct EnumPropsT<DisallowedRoadDirections> : MakeEnumPropsT<DisallowedRoadDirections, byte, DRD_NONE, DRD_END, DRD_END, 2> {};

/**
 * Get the disallowed road directions for a road tile
 * @param t The tile whose disallowed directions to get
 * @pre tile_is_road_track(t)
 * @return The disallowed directions for the tile
 */
static inline DisallowedRoadDirections tile_get_disallowed_directions(const Tile *t)
{
	assert(tile_is_road_track(t));
	return (DisallowedRoadDirections)GB(t->m3, 6, 2);
}

/**
 * Set the disallowed road directions for a road tile
 * @param t The tile whose disallowed directions to set
 * @param drd The disallowed directions to set
 * @pre tile_is_road_track(t)
 */
static inline void tile_set_disallowed_directions(Tile *t, DisallowedRoadDirections drd)
{
	assert(tile_is_road_track(t));
	assert(drd < DRD_END);
	SB(t->m3, 6, 2, drd);
}


/**
 * Get the bridge type of a road bridge
 * @param t The tile whose bridge type to get
 * @pre tile_is_road_bridge(t)
 * @return The bridge type of the tile
 */
static inline uint tile_get_road_bridge_type(const Tile *t)
{
	assert(tile_is_road_bridge(t));
	return GB(t->m7, 0, 4);
}

/**
 * Set the bridge type of a road bridge
 * @param t The tile whose bridge type to set
 * @param type The bridge type to set
 * @pre tile_is_road_bridge(t)
 */
static inline void tile_set_road_bridge_type(Tile *t, uint type)
{
	assert(tile_is_road_bridge(t));
	SB(t->m7, 0, 4, type);
}

/**
 * Check if a road bridge head is a custom bridge head
 * @param t The tile to check
 * @pre tile_is_road_bridge(t)
 * @return Whether the tile is a custom bridge head
 */
static inline bool tile_is_road_custom_bridgehead(const Tile *t)
{
	assert(tile_is_road_bridge(t));

	RoadBits axis = AxisToRoadBits(DiagDirToAxis(tile_get_tunnelbridge_direction(t)));
	RoadBits roadbits;

	roadbits = tile_get_roadbits(t, ROADTYPE_ROAD);
	if (roadbits != ROAD_NONE && roadbits != axis) return true;

	roadbits = tile_get_roadbits(t, ROADTYPE_TRAM);
	if (roadbits != ROAD_NONE && roadbits != axis) return true;

	return false;
}


/**
 * Make a road
 * @param t The tile to make a road
 * @param rts The road types of the tile
 * @param roadbits The roadbits for all roadtypes in the tile
 * @param town The id of the town that owns the road or is closest to the tile
 * @param road The owner of the road
 * @param tram The owner of the tram tracks
 */
static inline void tile_make_road(Tile *t, RoadTypes rts, RoadBits roadbits, uint town, Owner road, Owner tram)
{
	tile_set_type(t, TT_ROAD);
	t->m1 = (TT_TRACK << 6) | road;
	t->m2 = town;
	t->m3 = 0;
	t->m4 = (HasBit(rts, ROADTYPE_ROAD) ? roadbits : 0) | ((HasBit(rts, ROADTYPE_TRAM) ? roadbits : 0) << 4);
	t->m5 = (tram == OWNER_NONE) ? OWNER_TOWN : tram;
	t->m7 = rts << 6;
}

/**
 * Make a road bridge ramp
 * @param t The tile to make a road bridge ramp
 * @param type The bridge type
 * @param dir The direction the ramp is facing
 * @param rts The road types of the bridge
 * @param town The id of the town that owns the road or is closest to the tile
 * @param road The owner of the road
 * @param tram The owner of the tram tracks
 */
static inline void tile_make_road_bridge(Tile *t, uint type, DiagDirection dir, RoadTypes rts, uint town, Owner road, Owner tram)
{
	tile_set_type(t, TT_ROAD);
	t->m1 = (TT_BRIDGE << 6) | road;
	t->m2 = town;
	t->m3 = dir << 6;
	RoadBits roadbits = AxisToRoadBits(DiagDirToAxis(dir));
	t->m4 = (HasBit(rts, ROADTYPE_ROAD) ? roadbits : 0) | ((HasBit(rts, ROADTYPE_TRAM) ? roadbits : 0) << 4);
	t->m5 = (tram == OWNER_NONE) ? OWNER_TOWN : tram;
	t->m7 = (rts << 6) | type;
}

/**
 * Turn a road bridge ramp into normal road
 * @param t The tile to convert
 * @pre tile_is_road_bridge(t)
 * @note roadbits will have to be adjusted after this function is called
 */
static inline void tile_make_road_from_bridge(Tile *t)
{
	assert(tile_is_road_bridge(t));
	tile_set_subtype(t, TT_TRACK);
	SB(t->m3, 6, 2, 0);
	SB(t->m7, 0, 4, 0);
}

/**
 * Turn a road tile into a road bridge ramp
 * @param t The tile to convert
 * @param type The bridge type
 * @param dir The direction the ramp is facing
 * @pre tile_is_road_track(t)
 * @note roadbits will have to be adjusted after this function is called
 */
static inline void tile_make_bridge_from_road(Tile *t, uint type, DiagDirection dir)
{
	assert(tile_is_road_track(t));
	tile_set_subtype(t, TT_BRIDGE);
	SB(t->m3, 6, 2, dir);
	SB(t->m5, 4, 3, 0);
	SB(t->m7, 0, 4, type);
}

#endif /* TILE_ROAD_H */
