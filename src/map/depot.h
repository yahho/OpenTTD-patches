/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/depot.h Map tile accessors for depot tiles. */

#ifndef MAP_DEPOT_H
#define MAP_DEPOT_H

#include "../stdafx.h"
#include "../tile/misc.h"
#include "map.h"
#include "coord.h"
#include "class.h"
#include "water.h"
#include "../direction_type.h"
#include "../track_type.h"
#include "../company_type.h"

/**
 * Check if a ground depot is a rail depot.
 * @param t the tile to check
 * @pre IsGroundDepotTile(TileIndex t)
 * @return whether the ground depot is a rail depot
 */
static inline bool IsRailDepot(TileIndex t)
{
	return tile_depot_is_rail(&_mc[t]);
}

/**
 * Check if a ground depot is a road depot.
 * @param t the tile to check
 * @pre IsGroundDepotTile(TileIndex t)
 * @return whether the ground depot is a road depot
 */
static inline bool IsRoadDepot(TileIndex t)
{
	return tile_depot_is_road(&_mc[t]);
}

/**
 * Is this tile rail tile and a rail depot?
 * @param t the tile to get the information from
 * @return true if and only if the tile is a rail depot
 */
static inline bool IsRailDepotTile(TileIndex t)
{
	return tile_is_rail_depot(&_mc[t]);
}

/**
 * Return whether a tile is a road depot tile.
 * @param t Tile to query.
 * @return True if road depot tile.
 */
static inline bool IsRoadDepotTile(TileIndex t)
{
	return tile_is_road_depot(&_mc[t]);
}


/**
 * Get the index of which depot is attached to the tile.
 * @param t the tile
 * @pre IsGroundDepotTile(t) || IsShipDepotTile(t)
 * @return DepotID
 */
static inline uint GetDepotIndex(TileIndex t)
{
	return tile_get_depot_index(&_mc[t]);
}


/**
 * Returns the direction the depot is facing to
 * @param t the tile to get the depot facing from
 * @pre IsGroundDepotTile(t)
 * @return the direction the depot is facing
 */
static inline DiagDirection GetGroundDepotDirection(TileIndex t)
{
	return tile_get_ground_depot_direction(&_mc[t]);
}


/**
 * Returns the track of a depot, ignoring direction
 * @pre IsRailDepotTile(t)
 * @param t the tile to get the depot track from
 * @return the track of the depot
 */
static inline Track GetRailDepotTrack(TileIndex t)
{
	return tile_get_depot_track(&_mc[t]);
}

/**
 * Get the reservation state of the depot
 * @pre IsRailDepotTile(t)
 * @param t the depot tile
 * @return reservation state
 */
static inline bool HasDepotReservation(TileIndex t)
{
	return tile_is_depot_reserved(&_mc[t]);
}

/**
 * Set the reservation state of the depot
 * @pre IsRailDepotTile(t)
 * @param t the depot tile
 * @param b the reservation state
 */
static inline void SetDepotReservation(TileIndex t, bool b)
{
	tile_set_depot_reserved(&_mc[t], b);
}

/**
 * Get the reserved track bits for a depot
 * @pre IsRailDepotTile(t)
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetDepotReservationTrackBits(TileIndex t)
{
	return tile_get_depot_reserved_trackbits(&_mc[t]);
}


static inline void MakeRailDepot(TileIndex t, Owner o, uint did, DiagDirection d, RailType r)
{
	tile_make_rail_depot(&_mc[t], o, did, d, r);
}

/**
 * Make a road depot.
 * @param t     Tile to make a level crossing.
 * @param owner New owner of the depot.
 * @param did   New depot ID.
 * @param dir   Direction of the depot exit.
 * @param rt    Road type of the depot.
 */
static inline void MakeRoadDepot(TileIndex t, Owner owner, uint did, DiagDirection dir, RoadType rt)
{
	tile_make_road_depot(&_mc[t], owner, did, dir, rt);
}

#endif /* MAP_DEPOT_H */
