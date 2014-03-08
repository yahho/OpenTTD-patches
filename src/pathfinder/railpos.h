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

	/** Check if there are signals at a position */
	bool has_signals() const
	{
		if (in_wormhole()) {
			return false;
		} else if (IsRailwayTile(tile)) {
			return HasSignalOnTrack (tile, TrackdirToTrack(td));
		} else if (maptile_is_rail_tunnel(tile)) {
			return maptile_has_tunnel_signals (tile);
		} else {
			return false;
		}
	}

	/** Get the type of signals at a position */
	SignalType get_signal_type() const
	{
		assert(has_signals());
		return IsRailwayTile(tile) ?
			GetSignalType (tile, TrackdirToTrack(td)) :
			maptile_get_tunnel_signal_type(tile);
	}

	/** Check if there is a signal along/against a position */
	bool has_signal_along (bool along = true) const
	{
		if (in_wormhole()) {
			return false;
		} else if (IsRailwayTile(tile)) {
			return HasSignalOnTrackdir (tile, along ? td : ReverseTrackdir(td));
		} else if (maptile_is_rail_tunnel(tile)) {
			bool inwards = TrackdirToExitdir(td) == GetTunnelBridgeDirection(tile);
			return maptile_has_tunnel_signal (tile, along ^ inwards);
		} else {
			return false;
		}
	}

	/** Check if there is a signal against a position */
	bool has_signal_against() const { return has_signal_along(false); }

	/** Get the state of the signal along a position */
	SignalState get_signal_state() const
	{
		assert(has_signal_along());
		return IsRailwayTile(tile) ?
			GetSignalStateByTrackdir (tile, td) :
			maptile_get_tunnel_signal_state (tile, TrackdirToExitdir(td) == GetTunnelBridgeDirection(tile));
	}
};

#endif /* PATHFINDER_RAILPOS_H */
