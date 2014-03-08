/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pathfinder/railpos.h Railway path position type. */

#ifndef PATHFINDER_RAILPOS_H
#define PATHFINDER_RAILPOS_H

#include "pos.h"
#include "../rail_type.h"
#include "../track_func.h"
#include "../map/rail.h"

struct RailPathPos : PathPos<PathVTile> {
	typedef PathPos<PathVTile> Base;

	/** Create an empty RailPathPos */
	RailPathPos() : Base() { }

	/** Create a PathPos for a given tile and trackdir */
	RailPathPos (TileIndex t, Trackdir d) : Base (t, d) { }

	/** Create a PathPos in a wormhole */
	RailPathPos (TileIndex t, Trackdir d, TileIndex w) : Base (t, d, w) { }

	/** Get the rail type for this position. */
	RailType get_railtype() const
	{
		assert (is_valid());
		return !in_wormhole() ? GetRailType (tile, TrackdirToTrack(td)) :
			IsRailwayTile(wormhole) ? GetBridgeRailType(wormhole) : GetRailType(wormhole);
	}
};

#endif /* PATHFINDER_RAILPOS_H */
