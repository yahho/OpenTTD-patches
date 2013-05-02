/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/station.h Tile functions for station tiles. */

#ifndef TILE_STATION_H
#define TILE_STATION_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "tile.h"
#include "class.h"
#include "water.h"
#include "../direction_type.h"
#include "../track_type.h"
#include "../track_func.h"
#include "../rail_type.h"
#include "../road_type.h"
#include "../station_type.h"
#include "../company_type.h"

typedef byte StationGfx; ///< Index of station graphics. @see _station_display_datas

/* These enums are not really needed, but they help understand the gfx scheme. */

enum GfxRoad {
	GFX_ROAD_ST_NE = DIAGDIR_NE, ///< Standard road stop, heading NE
	GFX_ROAD_ST_SE = DIAGDIR_SE, ///< Standard road stop, heading SE
	GFX_ROAD_ST_SW = DIAGDIR_SW, ///< Standard road stop, heading SW
	GFX_ROAD_ST_NW = DIAGDIR_NW, ///< Standard road stop, heading NW

	GFX_ROAD_DT_X  = DIAGDIR_END + AXIS_X, ///< Drive-through road stop, along X
	GFX_ROAD_DT_Y  = DIAGDIR_END + AXIS_Y, ///< Drive-through road stop, along Y

	GFX_ROAD_DT_OFFSET = DIAGDIR_END,
};

enum GfxDock {
	GFX_DOCK_COAST_NE = DIAGDIR_NE, ///< Coast part, heading NE
	GFX_DOCK_COAST_SE = DIAGDIR_SE, ///< Coast part, heading SE
	GFX_DOCK_COAST_SW = DIAGDIR_SW, ///< Coast part, heading SW
	GFX_DOCK_COAST_NW = DIAGDIR_NW, ///< Coast part, heading NW

	GFX_DOCK_WATER_X  = DIAGDIR_END + AXIS_X, ///< Water part, along X
	GFX_DOCK_WATER_Y  = DIAGDIR_END + AXIS_Y, ///< Water part, along Y

	GFX_DOCK_BASE_WATER_PART = DIAGDIR_END,
};


/**
 * Get the type of station at a tile
 * @param t The tile whose station type to get
 * @pre tile_is_station(t)
 * @return The type of station at the tile
 */
static inline StationType tile_get_station_type(const Tile *t)
{
	assert(tile_is_station(t));
	return (StationType)GB(t->m0, 1, 3);
}

/**
 * Check if a station tile is of a given type
 * @param t The tile to check
 * @param type The station type to check against
 * @pre tile_is_station(t)
 * @return Whether the station tile is of the given type
 */
static inline bool tile_station_is_type(const Tile *t, StationType type)
{
	return tile_get_station_type(t) == type;
}

#define tile_station_is_rail(t)     tile_station_is_type(t, STATION_RAIL)
#define tile_station_is_waypoint(t) tile_station_is_type(t, STATION_WAYPOINT)
#define tile_station_is_truck(t)    tile_station_is_type(t, STATION_TRUCK)
#define tile_station_is_bus(t)      tile_station_is_type(t, STATION_BUS)
#define tile_station_is_oilrig(t)   tile_station_is_type(t, STATION_OILRIG)
#define tile_station_is_dock(t)     tile_station_is_type(t, STATION_DOCK)
#define tile_station_is_buoy(t)     tile_station_is_type(t, STATION_BUOY)
#define tile_station_is_airport(t)  tile_station_is_type(t, STATION_AIRPORT)

/**
 * Check if a tile is a station of a given type
 * @param t The tile to check
 * @param type The station type to check against
 * @return Whether the tile is a station of the given type
 */
static inline bool tile_is_type_station(const Tile *t, StationType type)
{
	return tile_is_station(t) && tile_station_is_type(t, type);
}

#define tile_is_rail_station(t)  tile_is_type_station(t, STATION_RAIL)
#define tile_is_waypoint(t)      tile_is_type_station(t, STATION_WAYPOINT)
#define tile_is_truck_station(t) tile_is_type_station(t, STATION_TRUCK)
#define tile_is_bus_station(t)   tile_is_type_station(t, STATION_BUS)
#define tile_is_oilrig(t)        tile_is_type_station(t, STATION_OILRIG)
#define tile_is_dock(t)          tile_is_type_station(t, STATION_DOCK)
#define tile_is_buoy(t)          tile_is_type_station(t, STATION_BUOY)
#define tile_is_airport(t)       tile_is_type_station(t, STATION_AIRPORT)

/**
 * Check if a station tile is a rail station or a rail waypoint
 * @param t The tile to check
 * @pre tile_is_station(t)
 * @return Whether the station tile is a rail station or a rail waypoint
 */
static inline bool tile_station_has_rail(const Tile *t)
{
	StationType type = tile_get_station_type(t);
	return type == STATION_RAIL || type == STATION_WAYPOINT;
}

/**
 * Check if a tile is a rail station or a rail waypoint
 * @param t The tile to check
 * @return Whether the tile is a rail station or a rail waypoint
 */
static inline bool tile_has_rail_station(const Tile *t)
{
	return tile_is_station(t) && tile_station_has_rail(t);
}

/**
 * Check if a station tile is a road station
 * @param t The tile to check
 * @pre tile_is_station(t)
 * @return Whether the station tile is a road station
 */
static inline bool tile_station_is_road(const Tile *t)
{
	StationType type = tile_get_station_type(t);
	return type == STATION_TRUCK || type == STATION_BUS;
}

/**
 * Check if a tile is a road station
 * @param t The tile to check
 * @return Whether the tile is a road station
 */
static inline bool tile_is_road_station(const Tile *t)
{
	return tile_is_station(t) && tile_station_is_road(t);
}


/**
 * Get the index of the station at a tile
 * @param t The tile whose station index to get
 * @pre tile_is_station(t)
 * @return The index of the station at the tile
 */
static inline uint tile_get_station_index(const Tile *t)
{
	assert(tile_is_station(t));
	return t->m2;
}

/**
 * Get the graphics index of a station tile
 * @param t The tile whose graphics index to get
 * @pre tile_is_station(t)
 * @return The graphics index of the tile
 */
static inline StationGfx tile_get_station_gfx(const Tile *t)
{
	assert(tile_is_station(t));
	return t->m5;
}

/**
 * Set the graphics index of a station tile
 * @param t The tile whose graphics index to set
 * @param gfx The graphics index to set
 * @pre tile_is_station(t)
 */
static inline void tile_set_station_gfx(Tile *t, StationGfx gfx)
{
	assert(tile_is_station(t));
	t->m5 = gfx;
}

/**
 * Get the random bits of a station tile
 * @param t The tile whose random bits to get
 * @pre tile_is_station(t)
 * @return The random bits for the tile
 */
static inline uint tile_get_station_random_bits(const Tile *t)
{
	assert(tile_is_station(t));
	return GB(t->m3, 4, 4);
}

/**
 * Set the random bits of a station tile
 * @param t The tile whose random bits to set
 * @param random The random bits to set
 * @pre tile_is_station(t)
 */
static inline void tile_set_station_random_bits(Tile *t, uint random)
{
	assert(tile_is_station(t));
	SB(t->m3, 4, 4, random);
}


/**
 * Get the axis of a rail station tile
 * @param t The tile whose axis to get
 * @pre tile_has_rail_station(t)
 * @return The axis of the station
 */
static inline Axis tile_get_station_axis(const Tile *t)
{
	assert(tile_has_rail_station(t));
	return HasBit(tile_get_station_gfx(t), 0) ? AXIS_Y : AXIS_X;
}

/**
 * Get the track of a rail station tile
 * @param t The tile whose track to get
 * @pre tile_has_rail_station(t)
 * @return The track of the station
 */
static inline Track tile_get_station_track(const Tile *t)
{
	return AxisToTrack(tile_get_station_axis(t));
}

/**
 * Get the track bits of a rail station tile
 * @param t The tile whose track bits to get
 * @pre tile_has_rail_station(t)
 * @return The track bits of the station
 */
static inline TrackBits tile_get_station_trackbits(const Tile *t)
{
	return TrackToTrackBits(tile_get_station_track(t));
}

/**
 * Get the reservation state of a rail station tile
 * @param t The tile whose reservation state to get
 * @pre tile_has_rail_station(t)
 * @return Whether the tile is reserved
 */
static inline bool tile_station_is_reserved(const Tile *t)
{
	assert(tile_has_rail_station(t));
	return HasBit(t->m0, 0);
}

/**
 * Set the reservation state of a rail station tile
 * @param t The tile whose reservation state to set
 * @param b The reservation state to set
 * @pre tile_has_rail_station(t)
 */
static inline void tile_station_set_reserved(Tile *t, bool b)
{
	assert(tile_has_rail_station(t));
	SB(t->m0, 0, 1, b ? 1 : 0);
}

/**
 * Get the reservation track bits of a rail station tile
 * @param t The tile whose reserved track bits to get
 * @pre tile_has_rail_station(t)
 * @return The reserved track bits of the tile
 */
static inline TrackBits tile_station_get_reserved_trackbits(const Tile *t)
{
	return tile_station_is_reserved(t) ? tile_get_station_trackbits(t) : TRACK_BIT_NONE;
}

/**
 * Get the custom spec of a rail station tile
 * @param t The tile whose custom spec to get
 * @pre tile_has_rail_station(t)
 * @return The custom spec index of the tile
 */
static inline uint tile_get_station_spec(const Tile *t)
{
	assert(tile_has_rail_station(t));
	return t->m4;
}

/**
 * Set the custom spec of a rail station tile
 * @param t The tile whose custom spec to set
 * @param spec The custom spec index to set
 * @pre tile_has_rail_station(t)
 */
static inline void tile_set_station_spec(Tile *t, uint spec)
{
	assert(tile_has_rail_station(t));
	t->m4 = spec;
}

/**
 * Check if a rail station tile has a custom spec
 * @param t The tile to check
 * @pre tile_has_rail_station(t)
 * @return Whether the tile as a custom spec
 */
static inline bool tile_has_custom_station_spec(const Tile *t)
{
	return tile_get_station_spec(t) != 0;
}


/**
 * Check if a road stop is a standard stop (not drive-through)
 * @param t The tile to check
 * @pre tile_is_road_station(t)
 * @return Whether the road stop is a standard stop
 */
static inline bool tile_road_station_is_standard(const Tile *t)
{
	assert(tile_is_road_station(t));
	return tile_get_station_gfx(t) < GFX_ROAD_DT_OFFSET;
}

/**
 * Check if a tile is a standard road stop (not drive-through)
 * @param t The tile to check
 * @return Whether there is a standard road stop on the tile
 */
static inline bool tile_is_standard_road_station(const Tile *t)
{
	return tile_is_road_station(t) && tile_road_station_is_standard(t);
}

/**
 * Check if a tile is a drive-through road stop
 * @param t The tile to check
 * @return Whether there is a drive-through road stop on the tile
 */
static inline bool tile_is_drive_through_road_station(const Tile *t)
{
	return tile_is_road_station(t) && !tile_road_station_is_standard(t);
}

/**
 * Get the direction of a road stop.
 * For standard stops, return the tile side of the entrance.
 * For drive-through stops, return the east-bound direction of the axis.
 * @param t The tile whose road stop to get the direction of
 * @pre tile_is_road_station(t)
 * @return The direction of the road stop, as described above.
 */
static inline DiagDirection tile_get_road_station_dir(const Tile *t)
{
	assert(tile_is_road_station(t));
	StationGfx gfx = tile_get_station_gfx(t);
	return (DiagDirection)(gfx % GFX_ROAD_DT_OFFSET);
}

/**
 * Get the axis of a drive-through road stop.
 * @param t The tile whose road stop to get the axis of
 * @pre tile_is_drive_through_road_station(t)
 * @return The axis of the road stop
 */
static inline Axis tile_get_road_station_axis(const Tile *t)
{
	assert(tile_is_drive_through_road_station(t));
	StationGfx gfx = tile_get_station_gfx(t);
	return (Axis)(gfx - GFX_ROAD_DT_OFFSET);
}


/**
 * Get the direction of a dock
 * @param t The tile whose dock direction to get
 * @pre tile_is_dock(t)
 * @return The direction of the dock
 */
static inline DiagDirection tile_get_dock_direction(const Tile *t)
{
	assert(tile_is_dock(t));
	StationGfx gfx = tile_get_station_gfx(t);
	assert(gfx < GFX_DOCK_BASE_WATER_PART);
	return (DiagDirection)gfx;
}


/**
 * Make a station tile
 * @param t The tile to make a station
 * @param type The station type to make
 * @param o The owner of the tile
 * @param id The index of the station
 * @param gfx The graphics index of the tile
 * @param wc The water class of the tile
 * @note Do not use this function directly; use the helpers below
 */
static inline void tile_make_station(Tile *t, StationType type, Owner o, uint id, StationGfx gfx, WaterClass wc = WATER_CLASS_INVALID)
{
	t->m0 = (TT_STATION << 4) | (type << 1);
	t->m1 = (wc << 5) | o;
	t->m2 = id;
	t->m3 = 0;
	t->m4 = 0;
	t->m5 = gfx;
	t->m7 = 0;
}

/**
 * Make a rail station tile
 * @param t The tile to make a rail station
 * @param o The owner of the tile
 * @param id The index of the station
 * @param axis The axis of the station
 * @param gfx The graphics index of the tile
 * @param rt The rail type of the tile
 * @param waypoint Make a rail waypoint, instead of a rail station
 */
static inline void tile_make_rail_station(Tile *t, Owner o, uint id, Axis axis, StationGfx gfx, RailType rt, bool waypoint = false)
{
	assert(gfx % 2 == 0);
	tile_make_station(t, waypoint ? STATION_WAYPOINT : STATION_RAIL, o, id, gfx + axis);
	t->m3 = rt;
}

/**
 * Make a road stop tile
 * @param t The tile to make a road stop
 * @param o The owner of the tile
 * @param id The index of the station
 * @param gfx The graphics index of the tile
 * @param rts The roadtypes on the tile
 * @param bus Make a bus stop, else a truck stop
 * @param road The owner of the road
 * @param tram The owner of the tram track
 */
static inline void tile_make_road_stop(Tile *t, Owner o, uint id, StationGfx gfx, RoadTypes rts, bool bus, Owner road, Owner tram)
{
	tile_make_station(t, bus ? STATION_BUS : STATION_TRUCK, o, id, gfx);
	t->m3 = tram << 4;
	t->m7 = (rts << 6) | road;
}

/**
 * Make an oilrig tile
 * @param t The tile to make an oilrig
 * @param id The index of the station
 * @param wc The water class of the tile
 */
static inline void tile_make_oilrig(Tile *t, uint id, WaterClass wc)
{
	tile_make_station(t, STATION_OILRIG, OWNER_NONE, id, 0, wc);
}

/**
 * Make a dock tile
 * @param t The tile to make a dock
 * @param o The owner of the tile
 * @param id The index of the station
 * @param gfx The graphics index of the tile
 * @param wc The water class of the tile
 */
static inline void tile_make_dock(Tile *t, Owner o, uint id, StationGfx gfx, WaterClass wc = WATER_CLASS_INVALID)
{
	tile_make_station(t, STATION_DOCK, o, id, gfx, wc);
}

/**
 * Make a buoy tile
 * @param t The tile to make a buoy
 * @param o The owner of the tile
 * @param id The index of the station
 * @param wc The water class of the tile
 */
static inline void tile_make_buoy(Tile *t, Owner o, uint id, WaterClass wc)
{
	tile_make_station(t, STATION_BUOY, o, id, 0, wc);
}

/**
 * Make an airport tile
 * @param t The tile to make an airport
 * @param o The owner of the tile
 * @param id The index of the station
 * @param gfx The graphics index of the tile
 * @param wc The water class of the tile
 */
static inline void tile_make_airport(Tile *t, Owner o, uint id, StationGfx gfx, WaterClass wc)
{
	tile_make_station(t, STATION_AIRPORT, o, id, gfx, wc);
}

#endif /* TILE_STATION_H */
