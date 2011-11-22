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
	TileIndex           m_old_tile;      ///< the origin (vehicle moved from) before move
	Trackdir            m_old_td;        ///< the trackdir (the vehicle was on) before move
	TileIndex           m_new_tile;      ///< the new tile (the vehicle has entered)
	TrackdirBits        m_new_td_bits;   ///< the new set of available trackdirs
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
	inline bool Follow(TileIndex old_tile, Trackdir old_td)
	{
		m_old_tile = old_tile;
		m_old_td = old_td;
		m_err = EC_NONE;
		assert(((GetTrackStatusTrackdirBits(m_old_tile) & TrackdirToTrackdirBits(m_old_td)) != 0) ||
		       (IsTram() && GetSingleTramBit(m_old_tile) != INVALID_DIAGDIR)); // Disable the assertion for single tram bits
		m_exitdir = TrackdirToExitdir(m_old_td);
		if (ForcedReverse()) return true;
		if (!CanExitOldTile()) return false;
		FollowTileExit();
		if (!QueryNewTileTrackStatus() || !CanEnterNewTile() ||
				(m_new_td_bits &= DiagdirReachesTrackdirs(m_exitdir)) == TRACKDIR_BIT_NONE) {
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
			m_new_td_bits &= (TrackdirBits)~(int)TrackdirCrossesTrackdirs(m_old_td);
			if (m_new_td_bits == TRACKDIR_BIT_NONE) {
				m_err = EC_90DEG;
				return false;
			}
		}
		return true;
	}

	inline bool MaskReservedTracks()
	{
		if (!m_mask_reserved) return true;

		if (m_flag == TF_STATION) {
			/* Check skipped station tiles as well. */
			TileIndexDiff diff = TileOffsByDiagDir(m_exitdir);
			for (TileIndex tile = m_new_tile - diff * m_tiles_skipped; tile != m_new_tile; tile += diff) {
				if (HasStationReservation(tile)) {
					m_new_td_bits = TRACKDIR_BIT_NONE;
					m_err = EC_RESERVED;
					return false;
				}
			}
		}

		TrackBits reserved = GetReservedTrackbits(m_new_tile);
		/* Mask already reserved trackdirs. */
		m_new_td_bits &= ~TrackBitsToTrackdirBits(reserved);
		/* Mask out all trackdirs that conflict with the reservation. */
		Track t;
		FOR_EACH_SET_TRACK(t, TrackdirBitsToTrackBits(m_new_td_bits)) {
			if (TracksOverlap(reserved | TrackToTrackBits(t))) m_new_td_bits &= ~TrackToTrackdirBits(t);
		}
		if (m_new_td_bits == TRACKDIR_BIT_NONE) {
			m_err = EC_RESERVED;
			return false;
		}
		return true;
	}

protected:
	/** Follow the m_exitdir from m_old_tile and fill m_new_tile and m_tiles_skipped */
	inline void FollowTileExit()
	{
		/* extra handling for tunnels and bridges in our direction */
		if (IsTunnelTile(m_old_tile) || IsBridgeHeadTile(m_old_tile)) {
			DiagDirection enterdir = GetTunnelBridgeDirection(m_old_tile);
			if (enterdir == m_exitdir) {
				/* we are entering the tunnel / bridge */
				if (IsTunnelTile(m_old_tile)) {
					m_flag = TF_TUNNEL;
					m_new_tile = GetOtherTunnelEnd(m_old_tile);
				} else {
					m_flag = TF_BRIDGE;
					m_new_tile = GetOtherBridgeEnd(m_old_tile);
				}
				m_tiles_skipped = GetTunnelBridgeLength(m_new_tile, m_old_tile);
				return;
			}
			assert(ReverseDiagDir(enterdir) == m_exitdir);
		}

		/* normal or station tile, do one step */
		TileIndexDiff diff = TileOffsByDiagDir(m_exitdir);
		m_new_tile = TILE_ADD(m_old_tile, diff);

		/* special handling for stations */
		if (IsRailTT() && HasStationTileRail(m_new_tile)) {
			m_flag = TF_STATION;
		} else if (IsRoadTT() && IsRoadStopTile(m_new_tile)) {
			m_flag = TF_STATION;
		} else {
			m_flag = TF_NONE;
		}

		m_tiles_skipped = 0;
	}

	/** stores track status (available trackdirs) for the new tile into m_new_td_bits */
	inline bool QueryNewTileTrackStatus()
	{
		CPerfStart perf(*m_pPerf);
		if (IsRailTT() && IsNormalRailTile(m_new_tile)) {
			m_new_td_bits = (TrackdirBits)(GetTrackBits(m_new_tile) * 0x101);
		} else {
			m_new_td_bits = GetTrackStatusTrackdirBits(m_new_tile);

			if (IsTram() && m_new_td_bits == 0) {
				/* GetTileTrackStatus() returns 0 for single tram bits.
				 * As we cannot change it there (easily) without breaking something, change it here */
				switch (GetSingleTramBit(m_new_tile)) {
					case DIAGDIR_NE:
					case DIAGDIR_SW:
						m_new_td_bits = TRACKDIR_BIT_X_NE | TRACKDIR_BIT_X_SW;
						break;

					case DIAGDIR_NW:
					case DIAGDIR_SE:
						m_new_td_bits = TRACKDIR_BIT_Y_NW | TRACKDIR_BIT_Y_SE;
						break;

					default: break;
				}
			}
		}
		return (m_new_td_bits != TRACKDIR_BIT_NONE);
	}

	/** return true if we can leave m_old_tile in m_exitdir */
	inline bool CanExitOldTile()
	{
		/* road stop can be left at one direction only unless it's a drive-through stop */
		if (IsRoadTT() && IsStandardRoadStopTile(m_old_tile)) {
			DiagDirection exitdir = GetRoadStopDir(m_old_tile);
			if (exitdir != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* single tram bits can only be left in one direction */
		if (IsTram()) {
			DiagDirection single_tram = GetSingleTramBit(m_old_tile);
			if (single_tram != INVALID_DIAGDIR && single_tram != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* road depots can be also left in one direction only */
		if (IsRoadTT() && IsDepotTypeTile(m_old_tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_old_tile);
			if (exitdir != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}
		return true;
	}

	/** return true if we can enter m_new_tile from m_exitdir */
	inline bool CanEnterNewTile()
	{
		if (IsRoadTT() && IsStandardRoadStopTile(m_new_tile)) {
			/* road stop can be entered from one direction only unless it's a drive-through stop */
			DiagDirection exitdir = GetRoadStopDir(m_new_tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* single tram bits can only be entered from one direction */
		if (IsTram()) {
			DiagDirection single_tram = GetSingleTramBit(m_new_tile);
			if (single_tram != INVALID_DIAGDIR && single_tram != ReverseDiagDir(m_exitdir)) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* road and rail depots can also be entered from one direction only */
		if (IsRoadTT() && IsDepotTypeTile(m_new_tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_new_tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
			/* don't try to enter other company's depots */
			if (GetTileOwner(m_new_tile) != m_veh_owner) {
				m_err = EC_OWNER;
				return false;
			}
		}
		if (IsRailTT() && IsDepotTypeTile(m_new_tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_new_tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* rail transport is possible only on tiles with the same owner as vehicle */
		if (IsRailTT() && GetTileOwner(m_new_tile) != m_veh_owner) {
			/* different owner */
			m_err = EC_NO_WAY;
			return false;
		}

		/* rail transport is possible only on compatible rail types */
		if (IsRailTT()) {
			RailType rail_type;
			if (IsNormalRailTile(m_new_tile)) {
				TrackBits tracks = GetTrackBits(m_new_tile) & DiagdirReachesTracks(m_exitdir);
				if (tracks == TRACK_BIT_NONE) {
					m_err = EC_NO_WAY;
					return false;
				}
				rail_type = GetRailType(m_new_tile, FindFirstTrack(tracks));
			} else {
				rail_type = GetTileRailType(m_new_tile);
			}
			if (!HasBit(m_railtypes, rail_type)) {
				/* incompatible rail type */
				m_err = EC_RAIL_TYPE;
				return false;
			}
		}

		/* tunnel holes and bridge ramps can be entered only from proper direction */
		if (IsTunnelTile(m_new_tile)) {
			if (m_flag != TF_TUNNEL) {
				DiagDirection tunnel_enterdir = GetTunnelBridgeDirection(m_new_tile);
				if (tunnel_enterdir != m_exitdir) {
					m_err = EC_NO_WAY;
					return false;
				}
			}
		} else if (IsBridgeHeadTile(m_new_tile)) {
			if (m_flag != TF_BRIDGE) {
				DiagDirection ramp_enderdir = GetTunnelBridgeDirection(m_new_tile);
				if (ramp_enderdir != m_exitdir) {
					m_err = EC_NO_WAY;
					return false;
				}
			}
		}

		/* special handling for rail stations - get to the end of platform */
		if (IsRailTT() && m_flag == TF_STATION) {
			/* entered railway station
			 * get platform length */
			uint length = BaseStation::GetByTile(m_new_tile)->GetPlatformLength(m_new_tile, TrackdirToExitdir(m_old_td));
			/* how big step we must do to get to the last platform tile; */
			m_tiles_skipped = length - 1;
			/* move to the platform end */
			TileIndexDiff diff = TileOffsByDiagDir(m_exitdir);
			diff *= m_tiles_skipped;
			m_new_tile = TILE_ADD(m_new_tile, diff);
			return true;
		}

		return true;
	}

	/** return true if we must reverse (in depots and single tram bits) */
	inline bool ForcedReverse()
	{
		/* rail and road depots cause reversing */
		if (!IsWaterTT() && IsDepotTypeTile(m_old_tile, TT())) {
			DiagDirection exitdir = GetGroundDepotDirection(m_old_tile);
			if (exitdir != m_exitdir) {
				/* reverse */
				m_new_tile = m_old_tile;
				m_new_td_bits = TrackdirToTrackdirBits(ReverseTrackdir(m_old_td));
				m_exitdir = exitdir;
				m_tiles_skipped = 0;
				m_flag = TF_NONE;
				return true;
			}
		}

		/* single tram bits cause reversing */
		if (IsTram() && GetSingleTramBit(m_old_tile) == ReverseDiagDir(m_exitdir)) {
			/* reverse */
			m_new_tile = m_old_tile;
			m_new_td_bits = TrackdirToTrackdirBits(ReverseTrackdir(m_old_td));
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
			m_new_tile = m_old_tile;
			/* set new trackdir bits to all reachable trackdirs */
			m_new_td_bits = GetTrackStatusTrackdirBits(m_new_tile);
			m_new_td_bits &= DiagdirReachesTrackdirs(m_exitdir);
			/* we always have some trackdirs reachable after reversal */
			assert(m_new_td_bits != TRACKDIR_BIT_NONE);
			return true;
		}
		m_err = EC_NO_WAY;
		return false;
	}

public:
	/** Helper for pathfinders - get min/max speed on the m_old_tile/m_old_td */
	int GetSpeedLimit(int *pmin_speed = NULL) const
	{
		int min_speed = 0;
		int max_speed = INT_MAX; // no limit

		if (IsRailTT()) {
			/* Check for on-bridge speed limit */
			if (IsRailBridgeTile(m_old_tile)) {
				int spd = GetBridgeSpec(GetRailBridgeType(m_old_tile))->speed;
				if (max_speed > spd) max_speed = spd;
			}
			/* Check for speed limit imposed by railtype */
			uint16 rail_speed = GetRailTypeInfo(GetRailType(m_old_tile, TrackdirToTrack(m_old_td)))->max_speed;
			if (rail_speed > 0) max_speed = min(max_speed, rail_speed);
		} else if (IsRoadTT()) {
			/* Check for on-bridge speed limit */
			if (IsRoadBridgeTile(m_old_tile)) {
				int spd = 2 * GetBridgeSpec(GetRoadBridgeType(m_old_tile))->speed;
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
