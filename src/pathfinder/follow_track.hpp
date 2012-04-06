/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file follow_track.hpp Template function for track followers */

#ifndef  FOLLOW_TRACK_HPP
#define  FOLLOW_TRACK_HPP

#include "../pbs.h"
#include "../roadveh.h"
#include "../station_base.h"
#include "../train.h"
#include "../tunnelbridge.h"
#include "../tunnelbridge_map.h"
#include "../depot_map.h"
#include "pathfinder_type.h"
#include "pf_performance_timer.hpp"

/**
 * Track follower helper template class (can serve pathfinders and vehicle
 *  controllers). See 6 different typedefs below for 3 different transport
 *  types w/ or w/o 90-deg turns allowed
 */
template <TransportType Ttr_type_, typename VehicleType>
struct CFollowTrack
{
	enum TileFlag {
		TF_NONE,
		TF_STATION,
		TF_TUNNEL,
		TF_BRIDGE,
	};

	enum ErrorCode {
		EC_NONE,
		EC_OWNER,
		EC_RAIL_TYPE,
		EC_90DEG,
		EC_NO_WAY,
		EC_RESERVED,
	};

	const VehicleType  *m_veh;           ///< moving vehicle
	Owner               m_veh_owner;     ///< owner of the vehicle
	PFPos               m_old;           ///< the origin (vehicle moved from) before move
	PFNewPos            m_new;           ///< the new tile (the vehicle has entered)
	DiagDirection       m_exitdir;       ///< exit direction (leaving the old tile)
	TileFlag            m_flag;          ///< last turn passed station, tunnel or bridge
	int                 m_tiles_skipped; ///< number of skipped tunnel or station tiles
	ErrorCode           m_err;
	CPerformanceTimer  *m_pPerf;
	RailTypes           m_railtypes;
	bool                m_allow_90deg;
	bool                m_mask_reserved;

	inline CFollowTrack(const VehicleType *v = NULL, bool allow_90deg = true, bool mask_reserved = false, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
	{
		assert(!IsRailTT() || (v != NULL && v->type == VEH_TRAIN));
		m_veh = v;
		Init(v != NULL ? v->owner : INVALID_OWNER, allow_90deg, mask_reserved, IsRailTT() && railtype_override == INVALID_RAILTYPES ? Train::From(v)->compatible_railtypes : railtype_override, pPerf);
	}

	inline CFollowTrack(Owner o, bool allow_90deg = true, bool mask_reserved = false, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
	{
		m_veh = NULL;
		Init(o, allow_90deg, mask_reserved, railtype_override, pPerf);
	}

	inline void Init(Owner o, bool allow_90deg, bool mask_reserved, RailTypes railtype_override, CPerformanceTimer *pPerf)
	{
		assert((!IsRoadTT() || m_veh != NULL) && (!IsRailTT() || railtype_override != INVALID_RAILTYPES));
		m_veh_owner = o;
		m_pPerf = pPerf;
		m_railtypes = railtype_override;
		m_allow_90deg = allow_90deg;
		m_mask_reserved = mask_reserved;
	}

	inline static TransportType TT() { return Ttr_type_; }
	inline static bool IsWaterTT() { return TT() == TRANSPORT_WATER; }
	inline static bool IsRailTT() { return TT() == TRANSPORT_RAIL; }
	inline bool IsTram() { return IsRoadTT() && HasBit(RoadVehicle::From(m_veh)->compatible_roadtypes, ROADTYPE_TRAM); }
	inline static bool IsRoadTT() { return TT() == TRANSPORT_ROAD; }

protected:
	inline TrackdirBits GetTrackStatusTrackdirBits(TileIndex tile) const
	{
		return TrackStatusToTrackdirBits(GetTileTrackStatus(tile, TT(), IsRoadTT() && m_veh != NULL ? RoadVehicle::From(m_veh)->compatible_roadtypes : 0));
	}

public:
	/** Tests if a tile is a road tile with a single tramtrack (tram can reverse) */
	inline DiagDirection GetSingleTramBit(TileIndex tile)
	{
		assert(IsTram()); // this function shouldn't be called in other cases

		if (IsRoadTile(tile)) {
			RoadBits rb = GetRoadBits(tile, ROADTYPE_TRAM);
			switch (rb) {
				case ROAD_NW: return DIAGDIR_NW;
				case ROAD_SW: return DIAGDIR_SW;
				case ROAD_SE: return DIAGDIR_SE;
				case ROAD_NE: return DIAGDIR_NE;
				default: break;
			}
		}
		return INVALID_DIAGDIR;
	}

	/**
	 * main follower routine. Fills all members and return true on success.
	 *  Otherwise returns false if track can't be followed.
	 */
	inline bool Follow(const PFPos &pos)
	{
		m_old = pos;
		m_err = EC_NONE;
		m_exitdir = TrackdirToExitdir(m_old.td);

		if (m_old.InWormhole()) {
			FollowWormhole();
		} else {
			assert(((GetTrackStatusTrackdirBits(m_old.tile) & TrackdirToTrackdirBits(m_old.td)) != 0) ||
			       (IsTram() && GetSingleTramBit(m_old.tile) != INVALID_DIAGDIR)); // Disable the assertion for single tram bits
			if (ForcedReverse()) return true;
			if (!CanExitOldTile()) return false;
			FollowTileExit();
		}

		if (!QueryNewTileTrackStatus() || !CanEnterNewTile() ||
				(m_new.trackdirs &= DiagdirReachesTrackdirs(m_exitdir)) == TRACKDIR_BIT_NONE) {
			/* In case we can't enter the next tile, but are
			 * a normal road vehicle, then we can actually
			 * try to reverse as this is the end of the road.
			 * Trams can only turn on the appropriate bits in
			 * which case reaching this would mean a dead end
			 * near a building and in that case there would
			 * a "false" QueryNewTileTrackStatus result and
			 * as such reversing is already tried. The fact
			 * that function failed can have to do with a
			 * missing road bit, or inability to connect the
			 * different bits due to slopes. */
			return TryReverse();
		}
		if (!m_allow_90deg) {
			m_new.trackdirs &= (TrackdirBits)~(int)TrackdirCrossesTrackdirs(m_old.td);
			if (m_new.trackdirs == TRACKDIR_BIT_NONE) {
				m_err = EC_90DEG;
				return false;
			}
		}
		/* Check if the resulting trackdirs is a single trackdir */
		m_new.SetTrackdir();
		return true;
	}

	inline bool FollowNext()
	{
		assert(m_new.tile != INVALID_TILE);
		assert(m_new.IsTrackdirSet());
		return Follow(m_new);
	}

	inline void SetPos(const PFPos &pos)
	{
		m_new.PFPos::operator = (pos);
		m_new.trackdirs = TrackdirToTrackdirBits(pos.td);
	}

	inline bool MaskReservedTracks()
	{
		if (!m_mask_reserved) return true;

		if (m_flag == TF_STATION) {
			/* Check skipped station tiles as well. */
			TileIndexDiff diff = TileOffsByDiagDir(m_exitdir);
			for (TileIndex tile = m_new.tile - diff * m_tiles_skipped; tile != m_new.tile; tile += diff) {
				if (HasStationReservation(tile)) {
					m_new.td = INVALID_TRACKDIR;
					m_new.trackdirs = TRACKDIR_BIT_NONE;
					m_err = EC_RESERVED;
					return false;
				}
			}
		}

		TrackBits reserved = GetReservedTrackbits(m_new.tile);
		/* Mask already reserved trackdirs. */
		m_new.trackdirs &= ~TrackBitsToTrackdirBits(reserved);
		/* Mask out all trackdirs that conflict with the reservation. */
		Track t;
		FOR_EACH_SET_TRACK(t, TrackdirBitsToTrackBits(m_new.trackdirs)) {
			if (TracksOverlap(reserved | TrackToTrackBits(t))) m_new.trackdirs &= ~TrackToTrackdirBits(t);
		}
		if (m_new.trackdirs == TRACKDIR_BIT_NONE) {
			m_new.td = INVALID_TRACKDIR;
			m_err = EC_RESERVED;
			return false;
		}
		/* Check if the resulting trackdirs is a single trackdir */
		m_new.SetTrackdir();
		return true;
	}

protected:
	/** Follow m_exitdir from m_old and fill m_new.tile and m_tiles_skipped */
	inline void FollowTileExit()
	{
		assert(!m_old.InWormhole());
		/* extra handling for bridges in our direction */
		if (IsBridgeHeadTile(m_old.tile)) {
			if (m_exitdir == GetTunnelBridgeDirection(m_old.tile)) {
				/* we are entering the bridge */
				m_flag = TF_BRIDGE;
				m_new.tile = GetOtherBridgeEnd(m_old.tile);
				m_tiles_skipped = GetTunnelBridgeLength(m_new.tile, m_old.tile);
				m_new.wormhole = INVALID_TILE;
				return;
			}
		/* extra handling for tunnels in our direction */
		} else if (IsTunnelTile(m_old.tile)) {
			DiagDirection enterdir = GetTunnelBridgeDirection(m_old.tile);
			if (enterdir == m_exitdir) {
				/* we are entering the tunnel */
				m_flag = TF_TUNNEL;
				m_new.tile = GetOtherTunnelEnd(m_old.tile);
				m_tiles_skipped = GetTunnelBridgeLength(m_new.tile, m_old.tile);
				m_new.wormhole = INVALID_TILE;
				return;
			}
			assert(ReverseDiagDir(enterdir) == m_exitdir);
		}

		/* normal or station tile, do one step */
		TileIndexDiff diff = TileOffsByDiagDir(m_exitdir);
		m_new.tile = TILE_ADD(m_old.tile, diff);
		m_new.wormhole = INVALID_TILE;

		/* special handling for stations */
		if (IsRailTT() && HasStationTileRail(m_new.tile)) {
			m_flag = TF_STATION;
		} else if (IsRoadTT() && IsRoadStopTile(m_new.tile)) {
			m_flag = TF_STATION;
		} else {
			m_flag = TF_NONE;
		}

		m_tiles_skipped = 0;
	}

	/** Follow m_old when in a wormhole */
	inline void FollowWormhole()
	{
		assert(m_old.InWormhole());
		assert(IsBridgeHeadTile(m_old.wormhole) || IsTunnelTile(m_old.wormhole));

		m_new.tile = m_old.wormhole;
		m_new.wormhole = INVALID_TILE;
		m_flag = IsTileSubtype(m_old.wormhole, TT_BRIDGE) ? TF_BRIDGE : TF_TUNNEL;
		m_tiles_skipped = GetTunnelBridgeLength(m_new.tile, m_old.tile);
	}

	/** stores track status (available trackdirs) for the new tile into m_new.trackdirs */
	inline bool QueryNewTileTrackStatus()
	{
		CPerfStart perf(*m_pPerf);
		if (IsRailTT() && IsNormalRailTile(m_new.tile)) {
			m_new.trackdirs = TrackBitsToTrackdirBits(GetTrackBits(m_new.tile));
		} else {
			m_new.trackdirs = GetTrackStatusTrackdirBits(m_new.tile);

			if (IsTram() && m_new.trackdirs == 0) {
				/* GetTileTrackStatus() returns 0 for single tram bits.
				 * As we cannot change it there (easily) without breaking something, change it here */
				switch (GetSingleTramBit(m_new.tile)) {
					case DIAGDIR_NE:
					case DIAGDIR_SW:
						m_new.trackdirs = TRACKDIR_BIT_X_NE | TRACKDIR_BIT_X_SW;
						break;

					case DIAGDIR_NW:
					case DIAGDIR_SE:
						m_new.trackdirs = TRACKDIR_BIT_Y_NW | TRACKDIR_BIT_Y_SE;
						break;

					default: break;
				}
			}
		}
		return (m_new.trackdirs != TRACKDIR_BIT_NONE);
	}

	/** return true if we can leave m_old in m_exitdir */
	inline bool CanExitOldTile()
	{
		/* road stop can be left at one direction only unless it's a drive-through stop */
		if (IsRoadTT() && IsStandardRoadStopTile(m_old.tile)) {
			DiagDirection exitdir = GetRoadStopDir(m_old.tile);
			if (exitdir != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* single tram bits can only be left in one direction */
		if (IsTram()) {
			DiagDirection single_tram = GetSingleTramBit(m_old.tile);
			if (single_tram != INVALID_DIAGDIR && single_tram != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* road depots can be also left in one direction only */
		if (IsRoadTT() && IsDepotTypeTile(m_old.tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_old.tile);
			if (exitdir != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}
		return true;
	}

	/** return true if we can enter m_new.tile from m_exitdir */
	inline bool CanEnterNewTile()
	{
		if (IsRoadTT() && IsStandardRoadStopTile(m_new.tile)) {
			/* road stop can be entered from one direction only unless it's a drive-through stop */
			DiagDirection exitdir = GetRoadStopDir(m_new.tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* single tram bits can only be entered from one direction */
		if (IsTram()) {
			DiagDirection single_tram = GetSingleTramBit(m_new.tile);
			if (single_tram != INVALID_DIAGDIR && single_tram != ReverseDiagDir(m_exitdir)) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* road and rail depots can also be entered from one direction only */
		if (IsRoadTT() && IsDepotTypeTile(m_new.tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_new.tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
			/* don't try to enter other company's depots */
			if (GetTileOwner(m_new.tile) != m_veh_owner) {
				m_err = EC_OWNER;
				return false;
			}
		}
		if (IsRailTT() && IsDepotTypeTile(m_new.tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_new.tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* rail transport is possible only on tiles with the same owner as vehicle */
		if (IsRailTT() && GetTileOwner(m_new.tile) != m_veh_owner) {
			/* different owner */
			m_err = EC_NO_WAY;
			return false;
		}

		/* rail transport is possible only on compatible rail types */
		if (IsRailTT()) {
			RailType rail_type;
			if (IsNormalRailTile(m_new.tile)) {
				TrackBits tracks = GetTrackBits(m_new.tile) & DiagdirReachesTracks(m_exitdir);
				if (tracks == TRACK_BIT_NONE) {
					m_err = EC_NO_WAY;
					return false;
				}
				rail_type = GetRailType(m_new.tile, FindFirstTrack(tracks));
			} else {
				rail_type = GetTileRailType(m_new.tile);
			}
			if (!HasBit(m_railtypes, rail_type)) {
				/* incompatible rail type */
				m_err = EC_RAIL_TYPE;
				return false;
			}
		}

		/* tunnel holes and bridge ramps can be entered only from proper direction */
		if (IsTunnelTile(m_new.tile)) {
			if (m_flag != TF_TUNNEL) {
				DiagDirection tunnel_enterdir = GetTunnelBridgeDirection(m_new.tile);
				if (tunnel_enterdir != m_exitdir) {
					m_err = EC_NO_WAY;
					return false;
				}
			}
		} else if (IsBridgeHeadTile(m_new.tile)) {
			if (m_flag != TF_BRIDGE) {
				DiagDirection ramp_enderdir = GetTunnelBridgeDirection(m_new.tile);
				if (ramp_enderdir == ReverseDiagDir(m_exitdir)) {
					m_err = EC_NO_WAY;
					return false;
				}
			}
		}

		/* special handling for rail stations - get to the end of platform */
		if (IsRailTT() && m_flag == TF_STATION) {
			/* entered railway station
			 * get platform length */
			uint length = BaseStation::GetByTile(m_new.tile)->GetPlatformLength(m_new.tile, TrackdirToExitdir(m_old.td));
			/* how big step we must do to get to the last platform tile; */
			m_tiles_skipped = length - 1;
			/* move to the platform end */
			TileIndexDiff diff = TileOffsByDiagDir(m_exitdir);
			diff *= m_tiles_skipped;
			m_new.tile = TILE_ADD(m_new.tile, diff);
			return true;
		}

		return true;
	}

	/** return true if we must reverse (in depots and single tram bits) */
	inline bool ForcedReverse()
	{
		/* rail and road depots cause reversing */
		if (!IsWaterTT() && IsDepotTypeTile(m_old.tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_old.tile);
			if (exitdir != m_exitdir) {
				/* reverse */
				m_new.tile = m_old.tile;
				m_new.wormhole = INVALID_TILE;
				m_new.td = ReverseTrackdir(m_old.td);
				m_new.trackdirs = TrackdirToTrackdirBits(m_new.td);
				m_exitdir = exitdir;
				m_tiles_skipped = 0;
				m_flag = TF_NONE;
				return true;
			}
		}

		/* single tram bits cause reversing */
		if (IsTram() && GetSingleTramBit(m_old.tile) == ReverseDiagDir(m_exitdir)) {
			/* reverse */
			m_new.tile = m_old.tile;
			m_new.wormhole = INVALID_TILE;
			m_new.td = ReverseTrackdir(m_old.td);
			m_new.trackdirs = TrackdirToTrackdirBits(m_new.td);
			m_exitdir = ReverseDiagDir(m_exitdir);
			m_tiles_skipped = 0;
			m_flag = TF_NONE;
			return true;
		}

		return false;
	}

	/** return true if we successfully reversed at end of road/track */
	inline bool TryReverse()
	{
		if (IsRoadTT() && !IsTram()) {
			/* if we reached the end of road, we can reverse the RV and continue moving */
			m_exitdir = ReverseDiagDir(m_exitdir);
			/* new tile will be the same as old one */
			m_new.tile = m_old.tile;
			m_new.wormhole = INVALID_TILE;
			/* set new trackdir bits to all reachable trackdirs */
			m_new.trackdirs = GetTrackStatusTrackdirBits(m_new.tile);
			m_new.trackdirs &= DiagdirReachesTrackdirs(m_exitdir);
			/* we always have some trackdirs reachable after reversal */
			assert(m_new.trackdirs != TRACKDIR_BIT_NONE);
			/* check if the resulting trackdirs is a single trackdir */
			m_new.SetTrackdir();
			return true;
		}
		m_new.td = INVALID_TRACKDIR;
		m_err = EC_NO_WAY;
		return false;
	}

public:
	/** Helper for pathfinders - get min/max speed on m_old */
	int GetSpeedLimit(int *pmin_speed = NULL) const
	{
		int min_speed = 0;
		int max_speed = INT_MAX; // no limit

		if (IsRailTT()) {
			/* Check for on-bridge and railtype speed limit */
			TileIndex bridge_tile;
			RailType rt;

			if (!m_old.InWormhole()) {
				bridge_tile = IsRailBridgeTile(m_old.tile) ? m_old.tile : INVALID_TILE;
				rt = GetRailType(m_old.tile, TrackdirToTrack(m_old.td));
			} else if (IsTileSubtype(m_old.wormhole, TT_BRIDGE)) {
				bridge_tile = m_old.wormhole;
				rt = GetRailType(bridge_tile);
			} else {
				bridge_tile = INVALID_TILE;
				rt = GetRailType(m_old.wormhole);
			}

			/* Check for on-bridge speed limit */
			if (bridge_tile != INVALID_TILE) {
				int spd = GetBridgeSpec(GetRailBridgeType(bridge_tile))->speed;
				if (max_speed > spd) max_speed = spd;
			}

			/* Check for speed limit imposed by railtype */
			uint16 rail_speed = GetRailTypeInfo(rt)->max_speed;
			if (rail_speed > 0) max_speed = min(max_speed, rail_speed);
		} else if (IsRoadTT()) {
			/* Check for on-bridge speed limit */
			if (IsRoadBridgeTile(m_old.tile)) {
				int spd = 2 * GetBridgeSpec(GetRoadBridgeType(m_old.tile))->speed;
				if (max_speed > spd) max_speed = spd;
			}
		}

		/* if min speed was requested, return it */
		if (pmin_speed != NULL) *pmin_speed = min_speed;
		return max_speed;
	}
};

template <TransportType Ttr_type_, typename VehicleType, bool T90deg_turns_allowed, bool Tmask_reserved_tracks = false>
struct CFollowTrackT : CFollowTrack<Ttr_type_, VehicleType>
{
	inline CFollowTrackT(const VehicleType *v = NULL, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
		: CFollowTrack<Ttr_type_, VehicleType>(v, T90deg_turns_allowed, Tmask_reserved_tracks, railtype_override, pPerf)
	{
	}

	inline static bool Allow90degTurns() { return T90deg_turns_allowed; }
	inline static bool DoTrackMasking() { return Tmask_reserved_tracks; }
};

typedef CFollowTrackT<TRANSPORT_WATER, Ship, true > CFollowTrackWater90;
typedef CFollowTrackT<TRANSPORT_WATER, Ship, false> CFollowTrackWaterNo90;

typedef CFollowTrackT<TRANSPORT_ROAD, RoadVehicle, true > CFollowTrackRoad;

typedef CFollowTrackT<TRANSPORT_RAIL, Train, true > CFollowTrackRail90;
typedef CFollowTrackT<TRANSPORT_RAIL, Train, false> CFollowTrackRailNo90;
typedef CFollowTrackT<TRANSPORT_RAIL, Train, true,  true > CFollowTrackFreeRail90;
typedef CFollowTrackT<TRANSPORT_RAIL, Train, false, true > CFollowTrackFreeRailNo90;

struct CFollowTrackRail : CFollowTrack<TRANSPORT_RAIL, Train>
{
	inline CFollowTrackRail(const Train *v = NULL, bool allow_90deg = true, bool railtype_override = false)
		: CFollowTrack<TRANSPORT_RAIL, Train>(v, allow_90deg, false, railtype_override ? GetRailTypeInfo(v->railtype)->compatible_railtypes : INVALID_RAILTYPES)
	{
	}

	inline CFollowTrackRail(Owner o, bool allow_90deg = true, RailTypes railtype_override = INVALID_RAILTYPES)
		: CFollowTrack<TRANSPORT_RAIL, Train>(o, allow_90deg, false, railtype_override)
	{
	}
};

#endif /* FOLLOW_TRACK_HPP */
