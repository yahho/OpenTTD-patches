/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file signal_map.h Slightly cooked access to signals on the map */

#ifndef SIGNAL_MAP_H
#define SIGNAL_MAP_H

#include "stdafx.h"
#include "signal_type.h"
#include "map/rail.h"
#include "pathfinder/pathfinder_type.h"

/**
 * Checks for the presence of signals along the given trackdir.
 */
static inline bool HasSignalAlongPos(const PFPos &pos)
{
	return !pos.InWormhole() && IsRailwayTile(pos.tile) && HasSignalOnTrackdir(pos.tile, pos.td);
}

/**
 * Checks for the presence of signals against the given trackdir.
 */
static inline bool HasSignalAgainstPos(const PFPos &pos)
{
	return !pos.InWormhole() && IsRailwayTile(pos.tile) && HasSignalOnTrackdir(pos.tile, ReverseTrackdir(pos.td));
}

/**
 * Checks for the presence of signals along or against the given trackdir.
 */
static inline bool HasSignalOnPos(const PFPos &pos)
{
	return !pos.InWormhole() && IsRailwayTile(pos.tile) && HasSignalOnTrack(pos.tile, TrackdirToTrack(pos.td));
}

static inline SignalType GetSignalType(const PFPos &pos)
{
	assert(HasSignalOnPos(pos));
	return GetSignalType(pos.tile, TrackdirToTrack(pos.td));
}

/**
 * Gets the state of the signal along the given trackdir.
 */
static inline SignalState GetSignalStateByPos(const PFPos &pos)
{
	return GetSignalStateByTrackdir(pos.tile, pos.td);
}


/**
 * Is a pbs signal present along the trackdir?
 * @param tile the tile to check
 * @param td the trackdir to check
 */
static inline bool HasPbsSignalOnTrackdir(TileIndex tile, Trackdir td)
{
	return IsRailwayTile(tile) && HasSignalOnTrackdir(tile, td) &&
			IsPbsSignal(GetSignalType(tile, TrackdirToTrack(td)));
}

/**
 * Is a pbs signal present along the trackdir?
 * @param pos the position to check
 */
static inline bool HasPbsSignalAlongPos(const PFPos &pos)
{
	return !pos.InWormhole() && HasPbsSignalOnTrackdir(pos.tile, pos.td);
}

/**
 * Is a pbs signal present against the trackdir?
 * @param pos the position to check
 */
static inline bool HasPbsSignalAgainstPos(const PFPos &pos)
{
	return !pos.InWormhole() && HasPbsSignalOnTrackdir(pos.tile, ReverseTrackdir(pos.td));
}


/**
 * Is a one-way signal blocking the trackdir? A one-way signal on the
 * trackdir against will block, but signals on both trackdirs won't.
 * @param tile the tile to check
 * @param td the trackdir to check
 */
static inline bool HasOnewaySignalBlockingTrackdir(TileIndex tile, Trackdir td)
{
	return IsRailwayTile(tile) && HasSignalOnTrackdir(tile, ReverseTrackdir(td)) &&
			!HasSignalOnTrackdir(tile, td) && IsOnewaySignal(GetSignalType(tile, TrackdirToTrack(td)));
}

/**
 * Is a one-way signal blocking the trackdir? A one-way signal on the
 * trackdir against will block, but signals on both trackdirs won't.
 * @param pos the position to check
 */
static inline bool HasOnewaySignalBlockingPos(const PFPos &pos)
{
	return !pos.InWormhole() && HasOnewaySignalBlockingTrackdir(pos.tile, pos.td);
}

#endif /* SIGNAL_MAP_H */
