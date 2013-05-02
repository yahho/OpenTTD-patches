/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/rail.h Map tile accessors for railway tiles. */

#ifndef MAP_RAIL_H
#define MAP_RAIL_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "../tile/rail.h"
#include "map.h"
#include "coord.h"
#include "class.h"
#include "tunnel.h"
#include "depot.h"
#include "station.h"
#include "../direction_type.h"
#include "../direction_func.h"
#include "../track_type.h"
#include "../track_func.h"
#include "../rail_type.h"
#include "../company_type.h"

/**
 * Gets the track bits of the given tile
 * @param tile the tile to get the track bits from
 * @return the track bits of the tile
 */
static inline TrackBits GetTrackBits(TileIndex tile)
{
	return tile_get_trackbits(&_mc[tile]);
}

/**
 * Sets the track bits of the given tile
 * @param t the tile to set the track bits of
 * @param b the new track bits for the tile
 */
static inline void SetTrackBits(TileIndex t, TrackBits b)
{
	tile_set_trackbits(&_mc[t], b);
}

/**
 * Returns whether the given track is present on the given tile.
 * @param tile  the tile to check the track presence of
 * @param track the track to search for on the tile
 * @pre IsRailwayTile(tile)
 * @return true if and only if the given track exists on the tile
 */
static inline bool HasTrack(TileIndex tile, Track track)
{
	return tile_has_track(&_mc[tile], track);
}


/**
 * Gets the rail type of the given tile
 * @param t the tile to get the rail type from
 * @return the rail type of the tile
 */
static inline RailType GetRailType(TileIndex t, Track track = INVALID_TRACK)
{
	return tile_get_rail_type(&_mc[t], track);
}

/**
 * Sets the rail type of the given tile
 * @param t the tile to set the rail type of
 * @param r the new rail type for the tile
 */
static inline void SetRailType(TileIndex t, RailType r, Track track = INVALID_TRACK)
{
	tile_set_rail_type(&_mc[t], r, track);
}

/**
 * Gets the rail type of the rail inciding on a given tile side
 * @param t the tile to get the rail type from
 * @param dir the side to get the track to check
 * @return the rail type of the tracks in the tile inciding on the given side, or INVALID_RAILTYPE if there are none
 */
static inline RailType GetSideRailType(TileIndex t, DiagDirection dir)
{
	return tile_get_side_rail_type(&_mc[t], dir);
}

/**
 * Gets the rail type of a rail bridge
 * @param t the tile to get the rail type from
 * @return the rail type of the bridge
 */
static inline RailType GetBridgeRailType(TileIndex t)
{
	return tile_get_bridge_rail_type(&_mc[t]);
}


/**
 * Returns the reserved track bits of the tile
 * @pre IsRailwayTile(t)
 * @param t the tile to query
 * @return the track bits
 */
static inline TrackBits GetRailReservationTrackBits(TileIndex t)
{
	return tile_get_reservation_trackbits(&_mc[t]);
}

/**
 * Sets the reserved track bits of the tile
 * @pre IsRailwayTile(t) && !TracksOverlap(b)
 * @param t the tile to change
 * @param b the track bits
 */
static inline void SetTrackReservation(TileIndex t, TrackBits b)
{
	tile_set_reservation_trackbits(&_mc[t], b);
}

/**
 * Try to reserve a specific track on a tile
 * @pre IsRailwayTile(t) && HasTrack(tile, t)
 * @param tile the tile
 * @param t the rack to reserve
 * @return true if successful
 */
static inline bool TryReserveTrack(TileIndex tile, Track t)
{
	assert(IsRailwayTile(tile));
	assert(HasTrack(tile, t));
	TrackBits bits = TrackToTrackBits(t);
	TrackBits res = GetRailReservationTrackBits(tile);
	if ((res & bits) != TRACK_BIT_NONE) return false;  // already reserved
	res |= bits;
	if (TracksOverlap(res)) return false;  // crossing reservation present
	SetTrackReservation(tile, res);
	return true;
}

/**
 * Lift the reservation of a specific track on a tile
 * @pre IsRailwayTile(t) && HasTrack(tile, t)
 * @param tile the tile
 * @param t the track to free
 */
static inline void UnreserveTrack(TileIndex tile, Track t)
{
	assert(IsRailwayTile(tile));
	assert(HasTrack(tile, t));
	TrackBits res = GetRailReservationTrackBits(tile);
	res &= ~TrackToTrackBits(t);
	SetTrackReservation(tile, res);
}


/**
 * Clear signals on a track
 * @param tile  the tile to clear the signals from
 * @param track the track to clear the signals from
 */
static inline void ClearSignals(TileIndex tile, Track track)
{
	tile_clear_signals(&_mc[tile], track);
}

/**
 * Get whether the given signals are present (Along/AgainstTrackDir)
 * @param tile the tile to get the present signals for
 * @param track the track to get the present signals for
 * @return the signals that are present
 */
static inline uint GetPresentSignals(TileIndex tile, Track track)
{
	return tile_get_present_signals(&_mc[tile], track);
}

/**
 * Set whether the given signals are present (Along/AgainstTrackDir)
 * @param tile    the tile to set the present signals for
 * @param track   the track to set the present signals for
 * @param signals the signals that have to be present
 */
static inline void SetPresentSignals(TileIndex tile, Track track, uint signals)
{
	tile_set_present_signals(&_mc[tile], track, signals);
}

/**
 * Checks for the presence of signals (either way) on the given track on the
 * given rail tile.
 */
static inline bool HasSignalOnTrack(TileIndex tile, Track track)
{
	return tile_has_track_signals(&_mc[tile], track);
}

/**
 * Checks for the presence of signals along the given trackdir on the given
 * rail tile.
 *
 * Along meaning if you are currently driving on the given trackdir, this is
 * the signal that is facing us (for which we stop when it's red).
 */
static inline bool HasSignalOnTrackdir(TileIndex tile, Trackdir trackdir)
{
	return tile_has_trackdir_signal(&_mc[tile], trackdir);
}


/**
 * Get the states of the signals (Along/AgainstTrackDir)
 * @param tile  the tile to get the states for
 * @param track the tile to get the states for
 * @return the state of the signals
 */
static inline uint GetSignalStates(TileIndex tile, Track track)
{
	return tile_get_signal_states(&_mc[tile], track);
}

/**
 * Set the states of the signals (Along/AgainstTrackDir)
 * @param tile  the tile to set the states for
 * @param track the track to set the states for
 * @param state the new state
 */
static inline void SetSignalStates(TileIndex tile, Track track, uint state)
{
	tile_set_signal_states(&_mc[tile], track, state);
}

/**
 * Gets the state of the signal along the given trackdir.
 *
 * Along meaning if you are currently driving on the given trackdir, this is
 * the signal that is facing us (for which we stop when it's red).
 */
static inline SignalState GetSignalStateByTrackdir(TileIndex tile, Trackdir trackdir)
{
	return tile_get_signal_state(&_mc[tile], trackdir);
}

/**
 * Sets the state of the signal along the given trackdir.
 */
static inline void SetSignalStateByTrackdir(TileIndex tile, Trackdir trackdir, SignalState state)
{
	tile_set_signal_state(&_mc[tile], trackdir, state);
}


static inline SignalType GetSignalType(TileIndex t, Track track)
{
	return tile_get_signal_type(&_mc[t], track);
}

static inline void SetSignalType(TileIndex t, Track track, SignalType s)
{
	tile_set_signal_type(&_mc[t], track, s);
}

static inline SignalVariant GetSignalVariant(TileIndex t, Track track)
{
	return tile_get_signal_variant(&_mc[t], track);
}

static inline void SetSignalVariant(TileIndex t, Track track, SignalVariant v)
{
	tile_set_signal_variant(&_mc[t], track, v);
}


static inline RailGroundType GetRailGroundType(TileIndex t)
{
	return tile_get_rail_ground(&_mc[t]);
}

static inline void SetRailGroundType(TileIndex t, RailGroundType rgt)
{
	tile_set_rail_ground(&_mc[t], rgt);
}


/**
 * Determines the type of rail bridge on a tile
 * @param t The tile to analyze
 * @pre IsRailBridgeTile(t)
 * @return The bridge type
 */
static inline uint GetRailBridgeType(TileIndex t)
{
	return tile_get_rail_bridge_type(&_mc[t]);
}

/**
 * Set the type of rail bridge on a tile
 * @param t The tile to set
 * @param type The type to set
 */
static inline void SetRailBridgeType(TileIndex t, uint type)
{
	tile_set_rail_bridge_type(&_mc[t], type);
}

/**
 * Check if a rail bridge is an extended bridge head
 * @param t The tile to check
 * @return Whether there are track bits set other than the axis track bit
 */
static inline bool IsExtendedRailBridge(TileIndex t)
{
	return tile_is_rail_custom_bridgehead(&_mc[t]);
}


/**
 * Get the reservation state of the rail bridge middle part
 * @pre IsRailBridgeTile(t)
 * @param t the tile
 * @return reservation state
 */
static inline bool HasBridgeMiddleReservation(TileIndex t)
{
	return tile_is_bridge_middle_reserved(&_mc[t]);
}

/**
 * Set the reservation state of the rail bridge middle part
 * @pre IsRailBridgeTile(t)
 * @param t the tile
 * @param b the reservation state
 */
static inline void SetBridgeMiddleReservation(TileIndex t, bool b)
{
	tile_set_bridge_middle_reserved(&_mc[t], b);
}


static inline void MakeRailNormal(TileIndex t, Owner o, TrackBits b, RailType r)
{
	tile_make_railway(&_mc[t], o, b, r);
}

/**
 * Make a bridge ramp for rails.
 * @param t          the tile to make a bridge ramp
 * @param o          the new owner of the bridge ramp
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @param r          the rail type of the bridge
 */
static inline void MakeRailBridgeRamp(TileIndex t, Owner o, uint bridgetype, DiagDirection d, RailType r)
{
	tile_make_rail_bridge(&_mc[t], o, bridgetype, d, r);
}

/**
 * Make a normal rail tile from a rail bridge ramp.
 * @param t the tile to make a normal rail
 * @note trackbits will have to be adjusted when this function is called
 */
static inline void MakeNormalRailFromBridge(TileIndex t)
{
	tile_make_railway_from_bridge(&_mc[t]);
}

/**
 * Make a rail bridge tile from a normal rail track.
 * @param t          the tile to make a rail bridge
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @note trackbits will have to be adjusted when this function is called
 */
static inline void MakeRailBridgeFromRail(TileIndex t, uint bridgetype, DiagDirection d)
{
	tile_make_rail_bridge_from_track(&_mc[t], bridgetype, d);
}


/**
 * Return the rail type of tile, or INVALID_RAILTYPE if this is no rail tile.
 */
static inline RailType GetTileRailType(TileIndex tile, Track track = INVALID_TRACK)
{
	switch (GetTileType(tile)) {
		case TT_RAILWAY:
			return GetRailType(tile, track);

		case TT_MISC:
			if (IsLevelCrossingTile(tile) || maptile_is_rail_tunnel(tile) || IsRailDepotTile(tile)) return GetRailType(tile);
			break;

		case TT_STATION:
			if (HasStationRail(tile)) return GetRailType(tile);
			break;

		default:
			break;
	}
	return INVALID_RAILTYPE;
}

#endif /* MAP_RAIL_H */
