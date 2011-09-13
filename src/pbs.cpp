/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pbs.cpp PBS support routines */

#include "stdafx.h"
#include "viewport_func.h"
#include "vehicle_func.h"
#include "newgrf_station.h"
#include "pathfinder/follow_track.hpp"

/**
 * Get the reserved trackbits for any tile, regardless of type.
 * @param t the tile
 * @return the reserved trackbits. TRACK_BIT_NONE on nothing reserved or
 *     a tile without rail.
 */
TrackBits GetReservedTrackbits(TileIndex t)
{
	switch (GetTileType(t)) {
		case TT_RAILWAY:
			return IsTileSubtype(t, TT_BRIDGE) ? GetBridgeReservationTrackBits(t) : GetRailReservationTrackBits(t);

		case TT_MISC:
			switch (GetTileSubtype(t)) {
				case TT_MISC_CROSSING: return GetCrossingReservationTrackBits(t);
				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType(t) == TRANSPORT_RAIL) return GetTunnelReservationTrackBits(t);
					break;
				case TT_MISC_DEPOT:
					if (IsRailDepot(t)) return GetDepotReservationTrackBits(t);
					break;
				default:
					break;
			}
			break;

		case TT_STATION:
			if (HasStationRail(t)) return GetStationReservationTrackBits(t);
			break;

		default:
			break;
	}
	return TRACK_BIT_NONE;
}

/**
 * Set the reservation for a complete station platform.
 * @pre IsRailStationTile(start)
 * @param start starting tile of the platform
 * @param dir the direction in which to follow the platform
 * @param b the state the reservation should be set to
 */
void SetRailStationPlatformReservation(TileIndex start, DiagDirection dir, bool b)
{
	assert(IsRailStationTile(start));
	assert(GetRailStationAxis(start) == DiagDirToAxis(dir));

	TileIndex     tile = start;
	TileIndexDiff diff = TileOffsByDiagDir(dir);

	do {
		SetRailStationReservation(tile, b);
		MarkTileDirtyByTile(tile);
		tile = TILE_ADD(tile, diff);
	} while (IsCompatibleTrainStationTile(tile, start));
}

/**
 * Set the reservation for a complete station platform.
 * @pre IsRailStationTile(pos.tile)
 * @param pos starting tile and direction of the platform
 * @param b the state the reservation should be set to
 */
void SetRailStationPlatformReservation(const PFPos &pos, bool b)
{
	SetRailStationPlatformReservation(pos.tile, TrackdirToExitdir(pos.td), b);
}

/**
 * Try to reserve a specific track on a tile
 * @param tile the tile
 * @param t the track
 * @param trigger_stations whether to call station randomisation trigger
 * @return \c true if reservation was successful, i.e. the track was
 *     free and didn't cross any other reserved tracks.
 */
bool TryReserveRailTrack(TileIndex tile, Track t, bool trigger_stations)
{
	assert((GetTileTrackStatus(tile, TRANSPORT_RAIL, 0) & TrackToTrackBits(t)) != 0);

	if (_settings_client.gui.show_track_reservation) {
		/* show the reserved rail if needed */
		MarkTileDirtyByTile(tile);
	}

	switch (GetTileType(tile)) {
		case TT_RAILWAY:
			if (!IsTileSubtype(tile, TT_BRIDGE)) return TryReserveTrack(tile, t);
			if (!GetBridgeReservationTrackBits(tile)) {
				SetTunnelBridgeReservation(tile, true);
				return true;
			}
			break;

		case TT_MISC:
			switch (GetTileSubtype(tile)) {
				case TT_MISC_CROSSING:
					if (HasCrossingReservation(tile)) break;
					SetCrossingReservation(tile, true);
					BarCrossing(tile);
					MarkTileDirtyByTile(tile); // crossing barred, make tile dirty
					return true;

				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType(tile) == TRANSPORT_RAIL && !GetTunnelReservationTrackBits(tile)) {
						SetTunnelBridgeReservation(tile, true);
						return true;
					}
					break;

				case TT_MISC_DEPOT:
					if (IsRailDepotTile(tile) && !HasDepotReservation(tile)) {
						SetDepotReservation(tile, true);
						MarkTileDirtyByTile(tile); // some GRFs change their appearance when tile is reserved
						return true;
					}
					break;

				default: break;
			}
			break;

		case TT_STATION:
			if (HasStationRail(tile) && !HasStationReservation(tile)) {
				SetRailStationReservation(tile, true);
				if (trigger_stations && IsRailStation(tile)) TriggerStationRandomisation(NULL, tile, SRT_PATH_RESERVATION);
				MarkTileDirtyByTile(tile); // some GRFs need redraw after reserving track
				return true;
			}
			break;

		default:
			break;
	}
	return false;
}

/**
 * Lift the reservation of a specific track on a tile
 * @param tile the tile
 * @param t the track
 */
void UnreserveRailTrack(TileIndex tile, Track t)
{
	assert((GetTileTrackStatus(tile, TRANSPORT_RAIL, 0) & TrackToTrackBits(t)) != 0);

	if (_settings_client.gui.show_track_reservation) {
		MarkTileDirtyByTile(tile);
	}

	switch (GetTileType(tile)) {
		case TT_RAILWAY:
			IsTileSubtype(tile, TT_BRIDGE) ? SetTunnelBridgeReservation(tile, false) : UnreserveTrack(tile, t);
			break;

		case TT_MISC:
			switch (GetTileSubtype(tile)) {
				case TT_MISC_CROSSING:
					SetCrossingReservation(tile, false);
					UpdateLevelCrossing(tile);
					break;

				case TT_MISC_TUNNEL:
					if (GetTunnelTransportType(tile) == TRANSPORT_RAIL) SetTunnelBridgeReservation(tile, false);
					break;

				case TT_MISC_DEPOT:
					if (IsRailDepot(tile)) {
						SetDepotReservation(tile, false);
						MarkTileDirtyByTile(tile);
					}
					break;

				default:
					break;
			}
			break;

		case TT_STATION:
			if (HasStationRail(tile)) {
				SetRailStationReservation(tile, false);
				MarkTileDirtyByTile(tile);
			}
			break;

		default:
			break;
	}
}


/** Follow a reservation starting from a specific tile to the end. */
static PBSTileInfo FollowReservation(Owner o, RailTypes rts, const PFPos &pos, bool ignore_oneway = false)
{
	/* Start track not reserved? This can happen if two trains
	 * are on the same tile. The reservation on the next tile
	 * is not ours in this case, so exit. */
	if (!HasReservedPos(pos)) return PBSTileInfo(pos, false);

	/* Do not disallow 90 deg turns as the setting might have changed between reserving and now. */
	CFollowTrackRail ft(o, true, rts);
	ft.SetPos(pos);
	PFPos cur = pos;
	PFPos start;

	while (ft.FollowNext()) {
		ft.m_new.trackdirs &= TrackBitsToTrackdirBits(GetReservedTrackbits(ft.m_new.tile));

		/* No reservation --> path end found */
		if (ft.m_new.trackdirs == TRACKDIR_BIT_NONE) {
			if (ft.m_flag == ft.TF_STATION) {
				/* Check skipped station tiles as well, maybe our reservation ends inside the station. */
				TileIndexDiff diff = TileOffsByDiagDir(ft.m_exitdir);
				while (ft.m_tiles_skipped-- > 0) {
					ft.m_new.tile -= diff;
					if (HasStationReservation(ft.m_new.tile)) {
						cur = ft.m_new;
						break;
					}
				}
			}
			break;
		}

		/* Can't have more than one reserved trackdir */
		ft.m_new.td = FindFirstTrackdir(ft.m_new.trackdirs);

		/* One-way signal against us. The reservation can't be ours as it is not
		 * a safe position from our direction and we can never pass the signal. */
		if (!ignore_oneway && HasOnewaySignalBlockingPos(ft.m_new)) break;

		cur = ft.m_new;

		if (start.tile == INVALID_TILE) {
			/* Update the start tile after we followed the track the first
			 * time. This is necessary because the track follower can skip
			 * tiles (in stations for example) which means that we might
			 * never visit our original starting tile again. */
			start = cur;
		} else {
			/* Loop encountered? */
			if (cur == start) break;
		}
		/* Depot tile? Can't continue. */
		if (IsRailDepotTile(cur.tile)) break;
		/* Non-pbs signal? Reservation can't continue. */
		if (HasSignalAlongPos(cur) && !IsPbsSignal(GetSignalType(cur))) break;
	}

	return PBSTileInfo(cur, false);
}

/**
 * Helper struct for finding the best matching vehicle on a specific track.
 */
struct FindTrainOnTrackInfo {
	PBSTileInfo res; ///< Information about the track.
	Train *best;     ///< The currently "best" vehicle we have found.

	/** Init the best location to NULL always! */
	FindTrainOnTrackInfo() : best(NULL) {}
};

/** Callback for Has/FindVehicleOnPos to find a train on a specific track. */
static Vehicle *FindTrainOnTrackEnum(Vehicle *v, void *data)
{
	FindTrainOnTrackInfo *info = (FindTrainOnTrackInfo *)data;

	if (v->type != VEH_TRAIN || (v->vehstatus & VS_CRASHED)) return NULL;

	Train *t = Train::From(v);
	if (t->trackdir == TRACKDIR_WORMHOLE || TrackdirToTrack(t->trackdir) == TrackdirToTrack(info->res.pos.td)) {
		t = t->First();

		/* ALWAYS return the lowest ID (anti-desync!) */
		if (info->best == NULL || t->index < info->best->index) info->best = t;
		return t;
	}

	return NULL;
}

/** Find a train on a reserved path end */
static void FindTrainOnPathEnd(FindTrainOnTrackInfo *ftoti)
{
	FindVehicleOnPos(ftoti->res.pos.tile, ftoti, FindTrainOnTrackEnum);
	if (ftoti->best != NULL) return;

	/* Special case for stations: check the whole platform for a vehicle. */
	if (IsRailStationTile(ftoti->res.pos.tile)) {
		TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(ftoti->res.pos.td)));
		for (TileIndex tile = ftoti->res.pos.tile + diff; IsCompatibleTrainStationTile(tile, ftoti->res.pos.tile); tile += diff) {
			FindVehicleOnPos(tile, ftoti, FindTrainOnTrackEnum);
			if (ftoti->best != NULL) return;
		}
	}

	/* Special case for bridges/tunnels: check the other end as well. */
	if (IsTunnelTile(ftoti->res.pos.tile) || IsRailBridgeTile(ftoti->res.pos.tile)) {
		FindVehicleOnPos(GetOtherTunnelBridgeEnd(ftoti->res.pos.tile), ftoti, FindTrainOnTrackEnum);
		if (ftoti->best != NULL) return;
	}
}

/**
 * Follow a train reservation to the last tile.
 *
 * @param v the vehicle
 * @param train_on_res Is set to a train we might encounter
 * @returns The last tile of the reservation or the current train tile if no reservation present.
 */
PBSTileInfo FollowTrainReservation(const Train *v, Vehicle **train_on_res)
{
	assert(v->type == VEH_TRAIN);

	PFPos pos = v->GetPos();

	if (IsRailDepotTile(pos.tile) && !GetDepotReservationTrackBits(pos.tile)) return PBSTileInfo(pos, false);

	FindTrainOnTrackInfo ftoti;
	ftoti.res = FollowReservation(v->owner, GetRailTypeInfo(v->railtype)->compatible_railtypes, pos);
	ftoti.res.okay = IsSafeWaitingPosition(v, ftoti.res.pos, _settings_game.pf.forbid_90_deg);
	if (train_on_res != NULL) {
		FindTrainOnPathEnd(&ftoti);
		if (ftoti.best != NULL) *train_on_res = ftoti.best->First();
	}
	return ftoti.res;
}

/**
 * Find the train which has reserved a specific path.
 *
 * @param tile A tile on the path.
 * @param track A reserved track on the tile.
 * @return The vehicle holding the reservation or NULL if the path is stray.
 */
Train *GetTrainForReservation(TileIndex tile, Track track)
{
	assert(HasReservedTrack(tile, track));
	Trackdir  trackdir = TrackToTrackdir(track);

	RailTypes rts = GetRailTypeInfo(GetTileRailType(tile, track))->compatible_railtypes;

	/* Follow the path from tile to both ends, one of the end tiles should
	 * have a train on it. We need FollowReservation to ignore one-way signals
	 * here, as one of the two search directions will be the "wrong" way. */
	for (int i = 0; i < 2; ++i, trackdir = ReverseTrackdir(trackdir)) {
		/* If the tile has a one-way block signal in the current trackdir, skip the
		 * search in this direction as the reservation can't come from this side.*/
		if (HasOnewaySignalBlockingTrackdir(tile, ReverseTrackdir(trackdir)) && !HasPbsSignalOnTrackdir(tile, trackdir)) continue;

		FindTrainOnTrackInfo ftoti;
		ftoti.res = FollowReservation(GetTileOwner(tile), rts, PFPos(tile, trackdir), true);

		FindTrainOnPathEnd(&ftoti);
		if (ftoti.best != NULL) return ftoti.best;
	}

	return NULL;
}

/**
 * Analyse a waiting position, to check if it is safe and/or if it is free.
 *
 * @param v the vehicle to test for
 * @param pos The position
 * @param forbid_90deg Don't allow trains to make 90 degree turns
 * @param cb Checking behaviour
 * @return Depending on cb:
 *  * PBS_CHECK_FULL: Do a full check. Return PBS_UNSAFE, PBS_BUSY, PBS_FREE
 *    depending on the waiting position state.
 *  * PBS_CHECK_SAFE: Only check if the position is safe. Return PBS_UNSAFE
 *    iff it is not.
 *  * PBS_CHECK_FREE: Assume that the position is safe, and check if it is
 *    free. Return PBS_FREE iff it is. The behaviour is undefined if the
 *    position is actually not safe.
 *  * PBS_CHECK_SAFE_FREE: Check if the position is both safe and free.
 *    Return PBS_FREE iff it is.
 */
PBSPositionState CheckWaitingPosition(const Train *v, const PFPos &pos, bool forbid_90deg, PBSCheckingBehaviour cb)
{
	/* Depots are always safe, and free iff unreserved. */
	if (IsRailDepotTile(pos.tile)) return HasDepotReservation(pos.tile) ? PBS_BUSY : PBS_FREE;

	if (HasSignalAlongPos(pos) && !IsPbsSignal(GetSignalType(pos))) {
		/* For non-pbs signals, stop on the signal tile. */
		if (cb == PBS_CHECK_SAFE) return PBS_FREE;
		return HasReservedTrack(pos.tile, TrackdirToTrack(pos.td)) ? PBS_BUSY : PBS_FREE;
	}

	PBSPositionState state;
	if ((cb != PBS_CHECK_SAFE) && TrackOverlapsTracks(GetReservedTrackbits(pos.tile), TrackdirToTrack(pos.td))) {
		/* Track reserved? Can never be a free waiting position. */
		if (cb != PBS_CHECK_FULL) return PBS_BUSY;
		state = PBS_BUSY;
	} else {
		/* Track not reserved or we do not care (PBS_CHECK_SAFE). */
		state = PBS_FREE;
	}

	/* Check next tile. */
	CFollowTrackRail ft(v, !forbid_90deg, true);

	/* End of track? Safe position. */
	if (!ft.Follow(pos)) return state;

	assert(ft.m_new.trackdirs != TRACKDIR_BIT_NONE);
	assert((state == PBS_FREE) || (cb == PBS_CHECK_FULL));

	if (cb != PBS_CHECK_FREE) {
		if (!ft.m_new.IsTrackdirSet()) return PBS_UNSAFE;

		if (HasSignalAlongPos(ft.m_new)) {
			/* PBS signal on next trackdir? Safe position. */
			if (!IsPbsSignal(GetSignalType(ft.m_new))) return PBS_UNSAFE;
		} else if (HasSignalAgainstPos(ft.m_new)) {
			/* One-way PBS signal against us? Safe position. */
			if (GetSignalType(ft.m_new) != SIGTYPE_PBS_ONEWAY) return PBS_UNSAFE;
		} else {
			/* No signal at all? Unsafe position. */
			return PBS_UNSAFE;
		}

		if (cb == PBS_CHECK_SAFE) return PBS_FREE;
		if (state != PBS_FREE) return PBS_BUSY;
	} else if (!IsStationTile(pos.tile)) {
		/* With PBS_CHECK_FREE, all these should be true. */
		assert(ft.m_new.IsTrackdirSet());
		assert(HasSignalOnPos(ft.m_new));
		assert(IsPbsSignal(GetSignalType(ft.m_new)));
	}

	assert(state == PBS_FREE);

	return HasReservedPos(ft.m_new) ? PBS_BUSY : PBS_FREE;
}
