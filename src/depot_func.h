/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file depot_func.h Functions related to depots. */

#ifndef DEPOT_FUNC_H
#define DEPOT_FUNC_H

#include "transport_type.h"
#include "vehicle_type.h"
#include "slope_func.h"
#include "map/depot.h"

void ShowDepotWindow(TileIndex tile, VehicleType type);

void DeleteDepotHighlightOfVehicle(const Vehicle *v);

/**
 * Find out if the slope of the tile is suitable to build a depot of given direction
 * @param direction The direction in which the depot's exit points
 * @param tileh The slope of the tile in question
 * @return true if the construction is possible
 */
static inline bool CanBuildDepotByTileh(DiagDirection direction, Slope tileh)
{
	assert(tileh != SLOPE_FLAT);
	Slope entrance_corners = InclinedSlope(direction);
	/* For steep slopes both entrance corners must be raised (i.e. neither of them is the lowest corner),
	 * For non-steep slopes at least one corner must be raised. */
	return IsSteepSlope(tileh) ? (tileh & entrance_corners) == entrance_corners : (tileh & entrance_corners) != 0;
}

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

#endif /* DEPOT_FUNC_H */
