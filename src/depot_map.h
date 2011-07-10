/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file depot_map.h Map related accessors for depots. */

#ifndef DEPOT_MAP_H
#define DEPOT_MAP_H

#include "station_map.h"

/**
 * Check if a tile is a depot and it is a depot of the given type.
 */
static inline bool IsDepotTypeTile(TileIndex tile, TransportType type)
{
	switch (type) {
		default: NOT_REACHED();
		case TRANSPORT_RAIL:
			return IsRailDepotTile(tile);

		case TRANSPORT_ROAD:
			return IsRoadDepotTile(tile);

		case TRANSPORT_WATER:
			return IsShipDepotTile(tile);
	}
}

/**
 * Check if a tile has a ground (rail or road) depot.
 * @param t the tile to check
 * @return whether the tile has a ground depot
 */
static inline bool IsGroundDepotTile(TileIndex t)
{
	return IsTileTypeSubtype(t, TT_MISC, TT_MISC_DEPOT);
}

/**
 * Check if a ground depot is a rail depot.
 * @param t the tile to check
 * @pre IsGroundDepotTile(TileIndex t)
 * @return whether the ground depot is a rail depot
 */
static inline bool IsRailDepot(TileIndex t)
{
	assert(IsGroundDepotTile(t));
	return !HasBit(_mc[t].m1, 5);
}

/**
 * Check if a ground depot is a road depot.
 * @param t the tile to check
 * @pre IsGroundDepotTile(TileIndex t)
 * @return whether the ground depot is a road depot
 */
static inline bool IsRoadDepot(TileIndex t)
{
	assert(IsGroundDepotTile(t));
	return HasBit(_mc[t].m1, 5);
}

/**
 * Is the given tile a tile with a depot on it?
 * @param tile the tile to check
 * @return true if and only if there is a depot on the tile.
 */
static inline bool IsDepotTile(TileIndex tile)
{
	return IsGroundDepotTile(tile) || IsShipDepotTile(tile) || IsHangarTile(tile);
}

/**
 * Get the index of which depot is attached to the tile.
 * @param t the tile
 * @pre IsGroundDepotTile(t) || IsShipDepotTile(t)
 * @return DepotID
 */
static inline DepotID GetDepotIndex(TileIndex t)
{
	/* Hangars don't have a Depot class, thus store no DepotID. */
	assert(IsGroundDepotTile(t) || IsShipDepotTile(t));
	return _mc[t].m2;
}

/**
 * Get the type of vehicles that can use a depot
 * @param t The tile
 * @pre IsGroundDepotTile(t) || IsShipDepotTile(t) || IsStationTile(t)
 * @return The type of vehicles that can use the depot
 */
static inline VehicleType GetDepotVehicleType(TileIndex t)
{
	switch (GetTileType(t)) {
		default: NOT_REACHED();
		case TT_WATER:   return VEH_SHIP;
		case TT_STATION: return VEH_AIRCRAFT;
		case TT_MISC:
			assert(IsGroundDepotTile(t));
			return IsRailDepot(t) ? VEH_TRAIN : VEH_ROAD;
	}
}

#endif /* DEPOT_MAP_H */
