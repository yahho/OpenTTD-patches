/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pathfinder/types.h General types related to pathfinders. */

#ifndef PATHFINDER_TYPES_H
#define PATHFINDER_TYPES_H

#include "../map/coord.h"
#include "pos.h"

/**
 * Helper container to find a depot
 */
struct FindDepotData {
	TileIndex tile;   ///< The tile of the depot
	bool reverse;     ///< True if reversing is necessary for the train to get to this depot

	/**
	 * Create an instance of this structure.
	 * @param tile        the tile of the depot
	 * @param reverse     whether we need to reverse first.
	 */
	FindDepotData(TileIndex tile = INVALID_TILE, bool reverse = false) :
		tile(tile), reverse(reverse)
	{
	}
};

/**
 * This struct contains information about the result of pathfinding.
 */
struct PFResult {
	RailPathPos pos;   ///< RailPathPos the reserved path ends, INVALID_TILE if no valid path was found.
	bool        okay;  ///< True if tile is a safe waiting position, false otherwise.
	bool        found; ///< True if a path was actually found, false if it was only an estimate.

	/**
	 * Create an empty PFResult.
	 */
	PFResult() : pos(), okay(false), found(false) {}
};

#endif /* PATHFINDER_TYPES_H */
