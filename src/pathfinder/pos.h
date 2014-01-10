/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pathfinder/pos.h Path position types. */

#ifndef PATHFINDER_POS_H
#define PATHFINDER_POS_H

#include "../map/coord.h"
#include "../track_type.h"
#include "../track_func.h"

/**
 * Path position (tile and trackdir)
 */
struct PathPos {
	TileIndex tile;
	Trackdir td;
	TileIndex wormhole;

	/** Create an empty PathPos */
	PathPos() : tile(INVALID_TILE), td(INVALID_TRACKDIR), wormhole(INVALID_TILE) { }

	/** Create a PathPos for a given tile and trackdir */
	PathPos(TileIndex t, Trackdir d) : tile(t), td(d), wormhole(INVALID_TILE) { }

	/** Create a PathPos in a wormhole */
	PathPos(TileIndex t, Trackdir d, TileIndex w) : tile(t), td(d), wormhole(w) { }

	/** Check if the PathPos is in a wormhole */
	bool in_wormhole() const { return wormhole != INVALID_TILE; }

	/** Compare with another PathPos */
	bool operator == (const PathPos &other) const
	{
		return (tile == other.tile) && (td == other.td) && (wormhole == other.wormhole);
	}

	/** Compare with another PathPos */
	bool operator != (const PathPos &other) const
	{
		return (tile != other.tile) || (td != other.td) || (wormhole != other.wormhole);
	}
};

/**
 * Pathfinder new position; td will be INVALID_POSITION unless trackdirs has exactly one trackdir set
 */
struct PathMPos : PathPos {
	TrackdirBits trackdirs;

	inline void SetTrackdir()
	{
		td = (KillFirstBit(trackdirs) == TRACKDIR_BIT_NONE) ? FindFirstTrackdir(trackdirs) : INVALID_TRACKDIR;
	}

	inline bool IsTrackdirSet() const { return td != INVALID_TRACKDIR; }
};

#endif /* PATHFINDER_POS_H */
