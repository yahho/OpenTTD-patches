/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/station.h Map tile accessors for station tiles. */

#ifndef MAP_STATION_H
#define MAP_STATION_H

#include "../stdafx.h"
#include "../tile/station.h"
#include "map.h"
#include "coord.h"
#include "class.h"
#include "common.h"
#include "../direction_type.h"
#include "../track_type.h"
#include "../track_func.h"
#include "../rail_type.h"
#include "../road_type.h"
#include "../station_type.h"
#include "../company_type.h"

/**
 * Get the station type of this tile
 * @param t the tile to query
 * @pre IsStationTile(t)
 * @return the station type
 */
static inline StationType GetStationType(TileIndex t)
{
	return tile_get_station_type(&_mc[t]);
}


/**
 * Is this station tile a rail station?
 * @param t the tile to get the information from
 * @pre IsStationTile(t)
 * @return true if and only if the tile is a rail station
 */
static inline bool IsRailStation(TileIndex t)
{
	return tile_station_is_rail(&_mc[t]);
}

/**
 * Is this tile a station tile and a rail station?
 * @param t the tile to get the information from
 * @return true if and only if the tile is a rail station
 */
static inline bool IsRailStationTile(TileIndex t)
{
	return tile_is_rail_station(&_mc[t]);
}

/**
 * Is this station tile a rail waypoint?
 * @param t the tile to get the information from
 * @pre IsStationTile(t)
 * @return true if and only if the tile is a rail waypoint
 */
static inline bool IsRailWaypoint(TileIndex t)
{
	return tile_station_is_waypoint(&_mc[t]);
}

/**
 * Is this tile a station tile and a rail waypoint?
 * @param t the tile to get the information from
 * @return true if and only if the tile is a rail waypoint
 */
static inline bool IsRailWaypointTile(TileIndex t)
{
	return tile_is_waypoint(&_mc[t]);
}

/**
 * Has this station tile a rail? In other words, is this station
 * tile a rail station or rail waypoint?
 * @param t the tile to check
 * @pre IsStationTile(t)
 * @return true if and only if the tile has rail
 */
static inline bool HasStationRail(TileIndex t)
{
	return tile_station_has_rail(&_mc[t]);
}

/**
 * Has this station tile a rail? In other words, is this station
 * tile a rail station or rail waypoint?
 * @param t the tile to check
 * @return true if and only if the tile is a station tile and has rail
 */
static inline bool HasStationTileRail(TileIndex t)
{
	return tile_has_rail_station(&_mc[t]);
}

/**
 * Is the station at \a t a truck stop?
 * @param t Tile to check
 * @pre IsStationTile(t)
 * @return \c true if station is a truck stop, \c false otherwise
 */
static inline bool IsTruckStop(TileIndex t)
{
	return tile_station_is_truck(&_mc[t]);
}

/**
 * Is the station at \a t a bus stop?
 * @param t Tile to check
 * @pre IsStationTile(t)
 * @return \c true if station is a bus stop, \c false otherwise
 */
static inline bool IsBusStop(TileIndex t)
{
	return tile_station_is_bus(&_mc[t]);
}

/**
 * Is the station at \a t a road station?
 * @param t Tile to check
 * @pre IsStationTile(t)
 * @return \c true if station at the tile is a bus top or a truck stop, \c false otherwise
 */
static inline bool IsRoadStop(TileIndex t)
{
	return tile_station_is_road(&_mc[t]);
}

/**
 * Is tile \a t a road stop station?
 * @param t Tile to check
 * @return \c true if the tile is a station tile and a road stop
 */
static inline bool IsRoadStopTile(TileIndex t)
{
	return tile_is_road_station(&_mc[t]);
}

/**
 * Is tile \a t part of an oilrig?
 * @param t Tile to check
 * @pre IsStationTile(t)
 * @return \c true if the tile is an oilrig tile
 */
static inline bool IsOilRig(TileIndex t)
{
	return tile_station_is_oilrig(&_mc[t]);
}

/**
 * Is tile \a t a dock tile?
 * @param t Tile to check
 * @pre IsStationTile(t)
 * @return \c true if the tile is a dock
 */
static inline bool IsDock(TileIndex t)
{
	return tile_station_is_dock(&_mc[t]);
}

/**
 * Is tile \a t a dock tile?
 * @param t Tile to check
 * @return \c true if the tile is a dock
 */
static inline bool IsDockTile(TileIndex t)
{
	return tile_is_dock(&_mc[t]);
}

/**
 * Is tile \a t a buoy tile?
 * @param t Tile to check
 * @pre IsStationTile(t)
 * @return \c true if the tile is a buoy
 */
static inline bool IsBuoy(TileIndex t)
{
	return tile_station_is_buoy(&_mc[t]);
}

/**
 * Is tile \a t a buoy tile?
 * @param t Tile to check
 * @return \c true if the tile is a buoy
 */
static inline bool IsBuoyTile(TileIndex t)
{
	return tile_is_buoy(&_mc[t]);
}

/**
 * Is this station tile an airport?
 * @param t the tile to get the information from
 * @pre IsStationTile(t)
 * @return true if and only if the tile is an airport
 */
static inline bool IsAirport(TileIndex t)
{
	return tile_station_is_airport(&_mc[t]);
}

/**
 * Is this tile a station tile and an airport tile?
 * @param t the tile to get the information from
 * @return true if and only if the tile is an airport
 */
static inline bool IsAirportTile(TileIndex t)
{
	return tile_is_airport(&_mc[t]);
}


/**
 * Get StationID from a tile
 * @param t Tile to query station ID from
 * @pre IsStationTile(t)
 * @return Station ID of the station at \a t
 */
static inline uint GetStationIndex(TileIndex t)
{
	return tile_get_station_index(&_mc[t]);
}

/**
 * Get the station graphics of this tile
 * @param t the tile to query
 * @pre IsStationTile(t)
 * @return the station graphics
 */
static inline StationGfx GetStationGfx(TileIndex t)
{
	return tile_get_station_gfx(&_mc[t]);
}

/**
 * Set the station graphics of this tile
 * @param t the tile to update
 * @param gfx the new graphics
 * @pre IsStationTile(t)
 */
static inline void SetStationGfx(TileIndex t, StationGfx gfx)
{
	tile_set_station_gfx(&_mc[t], gfx);
}


/**
 * Get the random bits of a station tile.
 * @param t Tile to query
 * @pre IsStationTile(t)
 * @return The random bits for this station tile.
 */
static inline byte GetStationTileRandomBits(TileIndex t)
{
	return tile_get_station_random_bits(&_mc[t]);
}

/**
 * Set the random bits for a station tile.
 * @param t Tile to set random bits for.
 * @param random_bits The random bits.
 * @pre IsStationTile(t)
 */
static inline void SetStationTileRandomBits(TileIndex t, byte random_bits)
{
	tile_set_station_random_bits(&_mc[t], random_bits);
}


/**
 * Get the rail direction of a rail station.
 * @param t Tile to query
 * @pre HasStationRail(t)
 * @return The direction of the rails on tile \a t.
 */
static inline Axis GetRailStationAxis(TileIndex t)
{
	return tile_get_station_axis(&_mc[t]);
}

/**
 * Get the rail track of a rail station tile.
 * @param t Tile to query
 * @pre HasStationRail(t)
 * @return The rail track of the rails on tile \a t.
 */
static inline Track GetRailStationTrack(TileIndex t)
{
	return tile_get_station_track(&_mc[t]);
}

/**
 * Get the trackbits of a rail station tile.
 * @param t Tile to query
 * @pre HasStationRail(t)
 * @return The trackbits of the rails on tile \a t.
 */
static inline TrackBits GetRailStationTrackBits(TileIndex t)
{
	return tile_get_station_trackbits(&_mc[t]);
}

/**
 * Get the reservation state of the rail station
 * @pre HasStationRail(t)
 * @param t the station tile
 * @return reservation state
 */
static inline bool HasStationReservation(TileIndex t)
{
	return tile_station_is_reserved(&_mc[t]);
}

/**
 * Set the reservation state of the rail station
 * @pre HasStationRail(t)
 * @param t the station tile
 * @param b the reservation state
 */
static inline void SetRailStationReservation(TileIndex t, bool b)
{
	tile_station_set_reserved(&_mc[t], b);
}

/**
 * Get the reserved track bits for a waypoint
 * @pre HasStationRail(t)
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetStationReservationTrackBits(TileIndex t)
{
	return tile_station_get_reserved_trackbits(&_mc[t]);
}

/**
 * Get the custom station spec for this tile.
 * @param t Tile to query
 * @pre HasStationTileRail(t)
 * @return The custom station spec of this tile.
 */
static inline uint GetCustomStationSpecIndex(TileIndex t)
{
	return tile_get_station_spec(&_mc[t]);
}

/**
 * Set the custom station spec for this tile.
 * @param t Tile to set the stationspec of.
 * @param specindex The new spec.
 * @pre HasStationTileRail(t)
 */
static inline void SetCustomStationSpecIndex(TileIndex t, byte specindex)
{
	tile_set_station_spec(&_mc[t], specindex);
}

/**
 * Is there a custom rail station spec on this tile?
 * @param t Tile to query
 * @pre HasStationTileRail(t)
 * @return True if this station is part of a newgrf station.
 */
static inline bool IsCustomStationSpecIndex(TileIndex t)
{
	return tile_has_custom_station_spec(&_mc[t]);
}


/**
 * Is tile \a t a standard (non-drive through) road stop station?
 * @param t Tile to check
 * @return \c true if the tile is a station tile and a standard road stop
 */
static inline bool IsStandardRoadStopTile(TileIndex t)
{
	return tile_is_standard_road_station(&_mc[t]);
}

/**
 * Is tile \a t a drive through road stop station?
 * @param t Tile to check
 * @return \c true if the tile is a station tile and a drive through road stop
 */
static inline bool IsDriveThroughStopTile(TileIndex t)
{
	return tile_is_drive_through_road_station(&_mc[t]);
}

/**
 * Gets the direction the road stop entrance points towards.
 * @param t the tile of the road stop
 * @pre IsRoadStopTile(t)
 * @return the direction of the entrance
 */
static inline DiagDirection GetRoadStopDir(TileIndex t)
{
	return tile_get_road_station_dir(&_mc[t]);
}

/**
 * Gets the axis of the road stop.
 * @param t the tile of the road stop
 * @pre IsDriveThroughRoadStopTile(t)
 * @return the axis of the road stop
 */
static inline Axis GetRoadStopAxis(TileIndex t)
{
	return tile_get_road_station_axis(&_mc[t]);
}

/**
 * Get the road stop type of this tile
 * @param t the tile to query
 * @pre IsRoadStop(t)
 * @return the road stop type
 */
static inline RoadStopType GetRoadStopType(TileIndex t)
{
	assert(IsRoadStop(t));
	return GetStationType(t) == STATION_TRUCK ? ROADSTOP_TRUCK : ROADSTOP_BUS;
}


/**
 * Get the direction of a dock.
 * @param t Tile to query
 * @pre IsDock(t)
 * @pre \a t is the land part of the dock
 * @return The direction of the dock on tile \a t.
 */
static inline DiagDirection GetDockDirection(TileIndex t)
{
	return tile_get_dock_direction(&_mc[t]);
}

/**
 * Get the tileoffset from this tile a ship should target to get to this dock.
 * @param t Tile to query
 * @pre IsStationTile(t)
 * @pre IsBuoy(t) || IsOilRig(t) || IsDock(t)
 * @return The offset from this tile that should be used as destination for ships.
 */
static inline TileIndexDiff GetDockOffset(TileIndex t)
{
	assert(IsStationTile(t));

	if (IsBuoy(t)) return TileDiffXY(0, 0);
	if (IsOilRig(t)) return TileDiffXY(2, 0);

	assert(IsDock(t));
	return 2 * TileOffsByDiagDir(GetDockDirection(t));
}


/**
 * Make the given tile a rail station tile.
 * @param t the tile to make a rail station tile
 * @param o the owner of the station
 * @param sid the station to which this tile belongs
 * @param a the axis of this tile
 * @param section the StationGfx to be used for this tile
 * @param rt the railtype of this tile
 */
static inline void MakeRailStation(TileIndex t, Owner o, StationID sid, Axis a, byte section, RailType rt)
{
	tile_make_rail_station(&_mc[t], o, sid, a, section, rt);
}

/**
 * Make the given tile a rail waypoint tile.
 * @param t the tile to make a rail waypoint
 * @param o the owner of the waypoint
 * @param sid the waypoint to which this tile belongs
 * @param a the axis of this tile
 * @param section the StationGfx to be used for this tile
 * @param rt the railtype of this tile
 */
static inline void MakeRailWaypoint(TileIndex t, Owner o, StationID sid, Axis a, byte section, RailType rt)
{
	tile_make_rail_station(&_mc[t], o, sid, a, section, rt, true);
}

/**
 * Make the given tile a roadstop tile.
 * @param t the tile to make a roadstop
 * @param o the owner of the roadstop
 * @param sid the station to which this tile belongs
 * @param rst the type of roadstop to make this tile
 * @param rt the roadtypes on this tile
 * @param d the direction of the roadstop
 */
static inline void MakeRoadStop(TileIndex t, Owner o, StationID sid, RoadStopType rst, RoadTypes rt, DiagDirection d)
{
	tile_make_road_stop(&_mc[t], o, sid, d, rt, rst == ROADSTOP_BUS, o, o);
}

/**
 * Make the given tile a drivethrough roadstop tile.
 * @param t the tile to make a roadstop
 * @param station the owner of the roadstop
 * @param road the owner of the road
 * @param tram the owner of the tram
 * @param sid the station to which this tile belongs
 * @param rst the type of roadstop to make this tile
 * @param rt the roadtypes on this tile
 * @param a the direction of the roadstop
 */
static inline void MakeDriveThroughRoadStop(TileIndex t, Owner station, Owner road, Owner tram, StationID sid, RoadStopType rst, RoadTypes rt, Axis a)
{
	tile_make_road_stop(&_mc[t], station, sid, GFX_ROAD_DT_OFFSET + a, rt, rst == ROADSTOP_BUS, road, tram);
}

/**
 * Make the given tile an oilrig tile.
 * @param t the tile to make an oilrig
 * @param sid the station to which this tile belongs
 * @param wc the type of water on this tile
 */
static inline void MakeOilrig(TileIndex t, StationID sid, WaterClass wc)
{
	tile_make_oilrig(&_mc[t], sid, wc);
}

/**
 * Make the given tile a dock tile.
 * @param t the tile to make a dock
 * @param o the owner of the dock
 * @param sid the station to which this tile belongs
 * @param d the direction of the dock
 * @param wc the type of water on this tile
 */
static inline void MakeDock(TileIndex t, Owner o, StationID sid, DiagDirection d, WaterClass wc)
{
	tile_make_dock(&_mc[t], o, sid, d);
	tile_make_dock(&_mc[t + TileOffsByDiagDir(d)], o, sid, GFX_DOCK_BASE_WATER_PART + DiagDirToAxis(d), wc);
}

/**
 * Make the given tile a buoy tile.
 * @param t the tile to make a buoy
 * @param sid the station to which this tile belongs
 * @param wc the type of water on this tile
 */
static inline void MakeBuoy(TileIndex t, StationID sid, WaterClass wc)
{
	/* Make the owner of the buoy tile the same as the current owner of the
	 * water tile. In this way, we can reset the owner of the water to its
	 * original state when the buoy gets removed. */
	tile_make_buoy(&_mc[t], GetTileOwner(t), sid, wc);
}

/**
 * Make the given tile an airport tile.
 * @param t the tile to make a airport
 * @param o the owner of the airport
 * @param sid the station to which this tile belongs
 * @param section the StationGfx to be used for this tile
 * @param wc the type of water on this tile
 */
static inline void MakeAirport(TileIndex t, Owner o, StationID sid, byte section, WaterClass wc)
{
	tile_make_airport(&_mc[t], o, sid, section, wc);
}

#endif /* MAP_STATION_H */
