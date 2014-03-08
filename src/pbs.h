/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs.h PBS support routines */

#ifndef PBS_H
#define PBS_H

#include "map/coord.h"
#include "direction_type.h"
#include "track_type.h"
#include "track_func.h"
#include "vehicle_type.h"
#include "pathfinder/railpos.h"
#include "map/bridge.h"
#include "map/tunnelbridge.h"
#include "signal_map.h"

TrackBits GetReservedTrackbits(TileIndex t);

void SetRailStationPlatformReservation(TileIndex start, DiagDirection dir, bool b);
void SetRailStationPlatformReservation(const RailPathPos &pos, bool b);

bool TryReserveRailTrack(TileIndex tile, Track t, bool trigger_stations = true);
void UnreserveRailTrack(TileIndex tile, Track t);

static inline bool TryReserveRailTrack(const RailPathPos &pos)
{
	if (!pos.in_wormhole()) {
		return TryReserveRailTrack(pos.tile, TrackdirToTrack(pos.td));
	} else if (IsRailwayTile(pos.wormhole)) {
		if (HasBridgeMiddleReservation(pos.wormhole)) return false;
		SetBridgeMiddleReservation(pos.wormhole, true);
		SetBridgeMiddleReservation(GetOtherBridgeEnd(pos.wormhole), true);
		return true;
	} else {
		if (HasTunnelMiddleReservation(pos.wormhole)) return false;
		SetTunnelMiddleReservation(pos.wormhole, true);
		SetTunnelMiddleReservation(GetOtherTunnelEnd(pos.wormhole), true);
		return true;
	}
}

static inline void UnreserveRailTrack(const RailPathPos &pos)
{
	if (!pos.in_wormhole()) {
		UnreserveRailTrack(pos.tile, TrackdirToTrack(pos.td));
	} else if (IsRailwayTile(pos.wormhole)) {
		SetBridgeMiddleReservation(pos.wormhole, false);
		SetBridgeMiddleReservation(GetOtherBridgeEnd(pos.wormhole), false);
	} else {
		SetTunnelMiddleReservation(pos.wormhole, false);
		SetTunnelMiddleReservation(GetOtherTunnelEnd(pos.wormhole), false);
	}
}

bool FollowTrainReservation(const Train *v, RailPathPos *pos, Vehicle **train_on_res = NULL);

/** State of a waiting position wrt PBS. */
enum PBSPositionState {
	PBS_UNSAFE, ///< Not a safe waiting position
	PBS_BUSY,   ///< Waiting position safe but busy
	PBS_FREE,   ///< Waiting position safe and free
};

/** Checking behaviour for CheckWaitingPosition. */
enum PBSCheckingBehaviour {
	PBS_CHECK_FULL,      ///< Do a full check of the waiting position
	PBS_CHECK_SAFE,      ///< Only check if the waiting position is safe
	PBS_CHECK_FREE,      ///< Assume that the waiting position is safe, and check if it is free
	PBS_CHECK_SAFE_FREE, ///< Check if the waiting position is both safe and free
};

PBSPositionState CheckWaitingPosition(const Train *v, const RailPathPos &pos, bool forbid_90deg = false, PBSCheckingBehaviour cb = PBS_CHECK_FULL);

static inline bool IsSafeWaitingPosition(const Train *v, const RailPathPos &pos, bool forbid_90deg = false)
{
	return CheckWaitingPosition(v, pos, forbid_90deg, PBS_CHECK_SAFE) != PBS_UNSAFE;
}

static inline bool IsWaitingPositionFree(const Train *v, const RailPathPos &pos, bool forbid_90deg = false)
{
	return CheckWaitingPosition(v, pos, forbid_90deg, PBS_CHECK_FREE) == PBS_FREE;
}

static inline bool IsFreeSafeWaitingPosition(const Train *v, const RailPathPos &pos, bool forbid_90deg = false)
{
	return CheckWaitingPosition(v, pos, forbid_90deg, PBS_CHECK_SAFE_FREE) == PBS_FREE;
}

Train *GetTrainForReservation(TileIndex tile, Track track);

/**
 * Check whether some of tracks is reserved on a tile.
 *
 * @param tile the tile
 * @param tracks the tracks to test
 * @return true if at least on of tracks is reserved
 */
static inline bool HasReservedTracks(TileIndex tile, TrackBits tracks)
{
	return (GetReservedTrackbits(tile) & tracks) != TRACK_BIT_NONE;
}

/**
 * Check whether a track is reserved on a tile.
 *
 * @param tile the tile
 * @param track the track to test
 * @return true if the track is reserved
 */
static inline bool HasReservedTrack(TileIndex tile, Track track)
{
	return HasReservedTracks(tile, TrackToTrackBits(track));
}

/**
 * Check whether a position is reserved.
 *
 * @param pos the position
 * @return true if the track is reserved
 */
static inline bool HasReservedPos(const RailPathPos &pos)
{
	return !pos.in_wormhole() ? HasReservedTrack(pos.tile, TrackdirToTrack(pos.td)) :
		IsRailwayTile(pos.wormhole) ? HasBridgeMiddleReservation(pos.wormhole) : HasTunnelMiddleReservation(pos.wormhole);
}

#endif /* PBS_H */
