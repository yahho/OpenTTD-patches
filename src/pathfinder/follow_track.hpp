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

#include "../map/road.h"
#include "../pbs.h"
#include "../roadveh.h"
#include "../station_base.h"
#include "../train.h"
#include "../tunnelbridge.h"
#include "../depot_func.h"
#include "../bridge.h"
#include "../ship.h"
#include "pathfinder_type.h"
#include "pf_performance_timer.hpp"

/**
 * Track follower common base class
 */
struct CFollowTrackBase
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

	enum TileResult {
		TR_NORMAL,
		TR_NO_WAY,
		TR_REVERSE,
	};

	PFPos               m_old;           ///< the origin (vehicle moved from) before move
	PFNewPos            m_new;           ///< the new tile (the vehicle has entered)
	DiagDirection       m_exitdir;       ///< exit direction (leaving the old tile)
	TileFlag            m_flag;          ///< last turn passed station, tunnel or bridge
	int                 m_tiles_skipped; ///< number of skipped tunnel or station tiles
	ErrorCode           m_err;
	bool                m_allow_90deg;
};


/**
 * Track follower rail base class
 */
struct CFollowTrackRailBase : CFollowTrackBase
{
	typedef Train VehicleType;

	static const TransportType transport_type = TRANSPORT_RAIL;

	const Owner               m_veh_owner;     ///< owner of the vehicle
	const RailTypes           m_railtypes;
	CPerformanceTimer  *const m_pPerf;

	inline CFollowTrackRailBase(const Train *v, bool allow_90deg = true, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
		: m_veh_owner(v->owner), m_railtypes(railtype_override == INVALID_RAILTYPES ? v->compatible_railtypes : railtype_override), m_pPerf(pPerf)
	{
		assert(v != NULL);
		assert(m_railtypes != INVALID_RAILTYPES);
		m_allow_90deg = allow_90deg;
	}

	inline CFollowTrackRailBase(Owner o, bool allow_90deg = true, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
		: m_veh_owner(o), m_railtypes(railtype_override), m_pPerf(pPerf)
	{
		assert(railtype_override != INVALID_RAILTYPES);
		m_allow_90deg = allow_90deg;
	}

	inline TrackdirBits GetTrackStatusTrackdirBits(TileIndex tile) const
	{
		return TrackStatusToTrackdirBits(GetTileRailwayStatus(tile));
	}

	/** check old tile */
	inline TileResult CheckOldTile()
	{
		assert((GetTrackStatusTrackdirBits(m_old.tile) & TrackdirToTrackdirBits(m_old.td)) != 0);

		/* depots cause reversing */
		if (IsRailDepotTile(m_old.tile)) {
			DiagDirection exitdir = GetGroundDepotDirection(m_old.tile);
			if (exitdir != m_exitdir) {
				assert(exitdir == ReverseDiagDir(m_exitdir));
				return TR_REVERSE;
			}
		}

		return TR_NORMAL;
	}

	/** stores track status (available trackdirs) for the new tile into m_new.trackdirs */
	inline bool CheckNewTile()
	{
		CPerfStart perf(*m_pPerf);

		if (IsNormalRailTile(m_new.tile)) {
			m_new.trackdirs = TrackBitsToTrackdirBits(GetTrackBits(m_new.tile));
		} else {
			m_new.trackdirs = GetTrackStatusTrackdirBits(m_new.tile);
		}

		if (m_new.trackdirs == TRACKDIR_BIT_NONE) return false;

		perf.Stop();

		if (IsRailDepotTile(m_new.tile)) {
			DiagDirection exitdir = GetGroundDepotDirection(m_new.tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* rail transport is possible only on tiles with the same owner as vehicle */
		if (GetTileOwner(m_new.tile) != m_veh_owner) {
			/* different owner */
			m_err = EC_NO_WAY;
			return false;
		}

		/* rail transport is possible only on compatible rail types */
		RailType rail_type;
		if (IsRailwayTile(m_new.tile)) {
			rail_type = GetSideRailType(m_new.tile, ReverseDiagDir(m_exitdir));
			if (rail_type == INVALID_RAILTYPE) {
				m_err = EC_NO_WAY;
				return false;
			}
		} else {
			rail_type = GetRailType(m_new.tile);
		}

		if (!HasBit(m_railtypes, rail_type)) {
			/* incompatible rail type */
			m_err = EC_RAIL_TYPE;
			return false;
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
		} else if (IsRailBridgeTile(m_new.tile)) {
			if (m_flag != TF_BRIDGE) {
				DiagDirection ramp_enderdir = GetTunnelBridgeDirection(m_new.tile);
				if (ramp_enderdir == ReverseDiagDir(m_exitdir)) {
					m_err = EC_NO_WAY;
					return false;
				}
			}
		}

		/* special handling for rail stations - get to the end of platform */
		if (m_flag == TF_STATION) {
			/* entered railway station
			 * get platform length */
			uint length = BaseStation::GetByTile(m_new.tile)->GetPlatformLength(m_new.tile, TrackdirToExitdir(m_old.td));
			/* how big step we must do to get to the last platform tile; */
			m_tiles_skipped = length - 1;
			/* move to the platform end */
			TileIndexDiff diff = TileOffsByDiagDir(m_exitdir);
			diff *= m_tiles_skipped;
			m_new.tile = TILE_ADD(m_new.tile, diff);
		}

		return true;
	}

	/** return true if we successfully reversed at end of road/track */
	inline bool CheckEndOfLine()
	{
		m_new.td = INVALID_TRACKDIR;
		m_err = EC_NO_WAY;
		return false;
	}

	inline bool CheckStation()
	{
		return HasStationTileRail(m_new.tile);
	}

	/** Helper for pathfinders - get min/max speed on m_old */
	int GetSpeedLimit(int *pmin_speed = NULL) const
	{
		/* Check for on-bridge and railtype speed limit */
		TileIndex bridge_tile;
		RailType rt;

		if (!m_old.InWormhole()) {
			bridge_tile = IsRailBridgeTile(m_old.tile) ? m_old.tile : INVALID_TILE;
			rt = GetRailType(m_old.tile, TrackdirToTrack(m_old.td));
		} else if (IsTileSubtype(m_old.wormhole, TT_BRIDGE)) {
			bridge_tile = m_old.wormhole;
			rt = GetBridgeRailType(bridge_tile);
		} else {
			bridge_tile = INVALID_TILE;
			rt = GetRailType(m_old.wormhole);
		}

		int max_speed;

		/* Check for on-bridge speed limit */
		if (bridge_tile != INVALID_TILE) {
			max_speed = GetBridgeSpec(GetRailBridgeType(bridge_tile))->speed;
		} else {
			max_speed = INT_MAX; // no limit
		}

		/* Check for speed limit imposed by railtype */
		uint16 rail_speed = GetRailTypeInfo(rt)->max_speed;
		if (rail_speed > 0) max_speed = min(max_speed, rail_speed);

		/* if min speed was requested, return it */
		if (pmin_speed != NULL) *pmin_speed = 0;
		return max_speed;
	}
};

template <bool mask_reserved>
struct CFollowTrackRailBaseT : CFollowTrackRailBase
{
	inline CFollowTrackRailBaseT(const Train *v, bool allow_90deg = true, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
		: CFollowTrackRailBase(v, allow_90deg, railtype_override, pPerf)
	{
	}

	inline CFollowTrackRailBaseT(Owner o, bool allow_90deg = true, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
		: CFollowTrackRailBase(o, allow_90deg, railtype_override, pPerf)
	{
	}

	inline static bool DoTrackMasking() { return mask_reserved; }

	inline bool MaskReservedTracks()
	{
		if (!mask_reserved) return true;

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

		if (m_new.InWormhole()) {
			assert(m_new.IsTrackdirSet());
			if (HasReservedPos(m_new)) {
				m_new.td = INVALID_TRACKDIR;
				m_new.trackdirs = TRACKDIR_BIT_NONE;
				m_err = EC_RESERVED;
				return false;
			} else {
				return true;
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
};

/**
 * Track follower road base class
 */
struct CFollowTrackRoadBase : CFollowTrackBase
{
	typedef RoadVehicle VehicleType;

	static const TransportType transport_type = TRANSPORT_ROAD;

	const Vehicle *const m_veh;     ///< moving vehicle

	inline CFollowTrackRoadBase(const RoadVehicle *v, bool allow_90deg = true)
		: m_veh(v)
	{
		assert(v != NULL);
		m_allow_90deg = allow_90deg;
	}

	inline TrackdirBits GetTrackStatusTrackdirBits(TileIndex tile) const
	{
		return TrackStatusToTrackdirBits(GetTileRoadStatus(tile, m_veh != NULL ? RoadVehicle::From(m_veh)->compatible_roadtypes : 0));
	}

	inline bool IsTram() { return HasBit(RoadVehicle::From(m_veh)->compatible_roadtypes, ROADTYPE_TRAM); }

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

	/** check old tile */
	inline TileResult CheckOldTile()
	{
		assert(((GetTrackStatusTrackdirBits(m_old.tile) & TrackdirToTrackdirBits(m_old.td)) != 0) ||
		       (IsTram() && GetSingleTramBit(m_old.tile) != INVALID_DIAGDIR)); // Disable the assertion for single tram bits

		/* depots cause reversing */
		if (IsRoadDepotTile(m_old.tile)) {
			DiagDirection exitdir = GetGroundDepotDirection(m_old.tile);
			if (exitdir != m_exitdir) {
				assert(exitdir == ReverseDiagDir(m_exitdir));
				return TR_REVERSE;
			}
		}

		/* road stop can be left at one direction only unless it's a drive-through stop */
		if (IsStandardRoadStopTile(m_old.tile)) {
			DiagDirection exitdir = GetRoadStopDir(m_old.tile);
			if (exitdir != m_exitdir) {
				return TR_NO_WAY;
			}
		}

		if (IsTram()) {
			DiagDirection single_tram = GetSingleTramBit(m_old.tile);
			/* single tram bits cause reversing */
			if (single_tram == ReverseDiagDir(m_exitdir)) {
				return TR_REVERSE;
			}
			/* single tram bits can only be left in one direction */
			if (single_tram != INVALID_DIAGDIR && single_tram != m_exitdir) {
				return TR_NO_WAY;
			}
		}

		return TR_NORMAL;
	}

	/** stores track status (available trackdirs) for the new tile into m_new.trackdirs */
	inline bool CheckNewTile()
	{
		m_new.trackdirs = GetTrackStatusTrackdirBits(m_new.tile);

		if (m_new.trackdirs == TRACKDIR_BIT_NONE) {
			if (!IsTram()) return false;

			/* GetTileRoadStatus() returns 0 for single tram bits.
			 * As we cannot change it there (easily) without breaking something, change it here */
			DiagDirection single_tram = GetSingleTramBit(m_new.tile);
			if (single_tram == ReverseDiagDir(m_exitdir)) {
				m_new.trackdirs = DiagDirToAxis(single_tram) == AXIS_X ? TRACKDIR_BIT_X_NE | TRACKDIR_BIT_X_SW : TRACKDIR_BIT_Y_NW | TRACKDIR_BIT_Y_SE;
				return true;
			} else {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		if (IsStandardRoadStopTile(m_new.tile)) {
			/* road stop can be entered from one direction only unless it's a drive-through stop */
			DiagDirection exitdir = GetRoadStopDir(m_new.tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
		}

		/* depots can also be entered from one direction only */
		if (IsRoadDepotTile(m_new.tile)) {
			DiagDirection exitdir = GetGroundDepotDirection(m_new.tile);
			if (ReverseDiagDir(exitdir) != m_exitdir) {
				m_err = EC_NO_WAY;
				return false;
			}
			/* don't try to enter other company's depots */
			if (GetTileOwner(m_new.tile) != m_veh->owner) {
				m_err = EC_OWNER;
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
		} else if (IsRoadBridgeTile(m_new.tile)) {
			if (m_flag != TF_BRIDGE) {
				DiagDirection ramp_enderdir = GetTunnelBridgeDirection(m_new.tile);
				if (ramp_enderdir == ReverseDiagDir(m_exitdir)) {
					m_err = EC_NO_WAY;
					return false;
				}
			}
		}

		return true;
	}

	/** return true if we successfully reversed at end of road/track */
	inline bool CheckEndOfLine()
	{
		if (!IsTram()) {
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

	inline bool CheckStation()
	{
		return IsRoadStopTile(m_new.tile);
	}

	/** Helper for pathfinders - get min/max speed on m_old */
	int GetSpeedLimit(int *pmin_speed = NULL) const
	{
		int max_speed;

		/* Check for on-bridge speed limit */
		if (IsRoadBridgeTile(m_old.tile)) {
			max_speed = 2 * GetBridgeSpec(GetRoadBridgeType(m_old.tile))->speed;
		} else {
			max_speed = INT_MAX; // no limit
		}

		/* if min speed was requested, return it */
		if (pmin_speed != NULL) *pmin_speed = 0;
		return max_speed;
	}
};

/**
 * Track follower water base class
 */
struct CFollowTrackWaterBase : CFollowTrackBase
{
	typedef Ship VehicleType;

	static const TransportType transport_type = TRANSPORT_WATER;

	inline CFollowTrackWaterBase(const Ship *v = NULL, bool allow_90deg = true)
	{
		m_allow_90deg = allow_90deg;
	}

	inline TrackdirBits GetTrackStatusTrackdirBits(TileIndex tile) const
	{
		return TrackStatusToTrackdirBits(GetTileWaterwayStatus(tile));
	}

	/** check old tile */
	inline TileResult CheckOldTile()
	{
		assert((GetTrackStatusTrackdirBits(m_old.tile) & TrackdirToTrackdirBits(m_old.td)) != 0);

		return TR_NORMAL;
	}

	/** stores track status (available trackdirs) for the new tile into m_new.trackdirs */
	inline bool CheckNewTile()
	{
		m_new.trackdirs = GetTrackStatusTrackdirBits(m_new.tile);

		if (m_new.trackdirs == TRACKDIR_BIT_NONE) return false;

		/* tunnel holes and bridge ramps can be entered only from proper direction */
		if (IsAqueductTile(m_new.tile)) {
			if (m_flag != TF_BRIDGE) {
				DiagDirection ramp_enderdir = GetTunnelBridgeDirection(m_new.tile);
				if (ramp_enderdir == ReverseDiagDir(m_exitdir)) {
					m_err = EC_NO_WAY;
					return false;
				}
			}
		}

		return true;
	}

	/** return true if we successfully reversed at end of road/track */
	inline bool CheckEndOfLine()
	{
		m_new.td = INVALID_TRACKDIR;
		m_err = EC_NO_WAY;
		return false;
	}

	inline bool CheckStation()
	{
		return false;
	}
};


/**
 * Track follower helper template class (can serve pathfinders and vehicle
 *  controllers). See 6 different typedefs below for 3 different transport
 *  types w/ or w/o 90-deg turns allowed
 */
template <class Base, bool Twormhole = false>
struct CFollowTrack : Base
{
	typedef typename Base::VehicleType VehicleType;

	inline CFollowTrack(const VehicleType *v = NULL, bool allow_90deg = true)
		: Base(v, allow_90deg)
	{
	}

	inline CFollowTrack(const VehicleType *v, bool allow_90deg, RailTypes railtype_override, CPerformanceTimer *pPerf = NULL)
		: Base(v, allow_90deg, railtype_override, pPerf)
	{
	}

	inline CFollowTrack(Owner o, bool allow_90deg = true, RailTypes railtype_override = INVALID_RAILTYPES, CPerformanceTimer *pPerf = NULL)
		: Base(o, allow_90deg, railtype_override, pPerf)
	{
	}

	inline static bool StepWormhole() {return Twormhole;}
	/**
	 * main follower routine. Fills all members and return true on success.
	 *  Otherwise returns false if track can't be followed.
	 */
	inline bool Follow(const PFPos &pos)
	{
		Base::m_old = pos;
		Base::m_err = Base::EC_NONE;
		Base::m_exitdir = TrackdirToExitdir(Base::m_old.td);

		if (Base::m_old.InWormhole()) {
			FollowWormhole();
		} else {
			switch (Base::CheckOldTile()) {
				case Base::TR_NO_WAY:
					Base::m_err = Base::EC_NO_WAY;
					return false;
				case Base::TR_REVERSE:
					Base::m_new.tile = Base::m_old.tile;
					Base::m_new.wormhole = INVALID_TILE;
					Base::m_new.td = ReverseTrackdir(Base::m_old.td);
					Base::m_new.trackdirs = TrackdirToTrackdirBits(Base::m_new.td);
					Base::m_exitdir = ReverseDiagDir(Base::m_exitdir);
					Base::m_tiles_skipped = 0;
					Base::m_flag = Base::TF_NONE;
					return true;
				default:
					break;
			}
			FollowTileExit();
		}

		if (Base::m_new.InWormhole()) {
			assert(StepWormhole());
			Base::m_new.td = DiagDirToDiagTrackdir(Base::m_exitdir);
			Base::m_new.trackdirs = TrackdirToTrackdirBits(Base::m_new.td);
			return true;
		}

		if (!Base::CheckNewTile() || (Base::m_new.trackdirs &= DiagdirReachesTrackdirs(Base::m_exitdir)) == TRACKDIR_BIT_NONE) {
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
			return Base::CheckEndOfLine();
		}
		if (!Base::m_allow_90deg) {
			Base::m_new.trackdirs &= (TrackdirBits)~(int)TrackdirCrossesTrackdirs(Base::m_old.td);
			if (Base::m_new.trackdirs == TRACKDIR_BIT_NONE) {
				Base::m_err = Base::EC_90DEG;
				return false;
			}
		}
		/* Check if the resulting trackdirs is a single trackdir */
		Base::m_new.SetTrackdir();
		return true;
	}

	inline bool FollowNext()
	{
		assert(Base::m_new.tile != INVALID_TILE);
		assert(Base::m_new.IsTrackdirSet());
		return Follow(Base::m_new);
	}

	inline void SetPos(const PFPos &pos)
	{
		Base::m_new.PFPos::operator = (pos);
		Base::m_new.trackdirs = TrackdirToTrackdirBits(pos.td);
	}

protected:
	/** Follow m_exitdir from m_old and fill m_new.tile and m_tiles_skipped */
	inline void FollowTileExit()
	{
		assert(!Base::m_old.InWormhole());
		/* extra handling for bridges in our direction */
		if (IsBridgeHeadTile(Base::m_old.tile)) {
			if (Base::m_exitdir == GetTunnelBridgeDirection(Base::m_old.tile)) {
				/* we are entering the bridge */
				Base::m_flag = Base::TF_BRIDGE;
				Base::m_new.tile = GetOtherBridgeEnd(Base::m_old.tile);
				Base::m_tiles_skipped = GetTunnelBridgeLength(Base::m_new.tile, Base::m_old.tile);
				if (StepWormhole() && Base::m_tiles_skipped > 0) {
					Base::m_tiles_skipped--;
					Base::m_new.wormhole = Base::m_new.tile;
					Base::m_new.tile = TILE_ADD(Base::m_new.tile, TileOffsByDiagDir(ReverseDiagDir(Base::m_exitdir)));
				} else {
					Base::m_new.wormhole = INVALID_TILE;
				}
				return;
			}
		/* extra handling for tunnels in our direction */
		} else if (IsTunnelTile(Base::m_old.tile)) {
			DiagDirection enterdir = GetTunnelBridgeDirection(Base::m_old.tile);
			if (enterdir == Base::m_exitdir) {
				/* we are entering the tunnel */
				Base::m_flag = Base::TF_TUNNEL;
				Base::m_new.tile = GetOtherTunnelEnd(Base::m_old.tile);
				Base::m_tiles_skipped = GetTunnelBridgeLength(Base::m_new.tile, Base::m_old.tile);
				if (StepWormhole() && Base::m_tiles_skipped > 0) {
					Base::m_tiles_skipped--;
					Base::m_new.wormhole = Base::m_new.tile;
					Base::m_new.tile = TILE_ADD(Base::m_new.tile, TileOffsByDiagDir(ReverseDiagDir(Base::m_exitdir)));
				} else {
					Base::m_new.wormhole = INVALID_TILE;
				}
				return;
			}
			assert(ReverseDiagDir(enterdir) == Base::m_exitdir);
		}

		/* normal or station tile, do one step */
		TileIndexDiff diff = TileOffsByDiagDir(Base::m_exitdir);
		Base::m_new.tile = TILE_ADD(Base::m_old.tile, diff);
		Base::m_new.wormhole = INVALID_TILE;

		/* special handling for stations */
		Base::m_flag = Base::CheckStation() ? Base::TF_STATION : Base::TF_NONE;

		Base::m_tiles_skipped = 0;
	}

	/** Follow m_old when in a wormhole */
	inline void FollowWormhole()
	{
		assert(Base::m_old.InWormhole());
		assert(IsBridgeHeadTile(Base::m_old.wormhole) || IsTunnelTile(Base::m_old.wormhole));

		Base::m_new.tile = Base::m_old.wormhole;
		Base::m_new.wormhole = INVALID_TILE;
		Base::m_flag = IsTileSubtype(Base::m_old.wormhole, TT_BRIDGE) ? Base::TF_BRIDGE : Base::TF_TUNNEL;
		Base::m_tiles_skipped = GetTunnelBridgeLength(Base::m_new.tile, Base::m_old.tile);
	}
};

template <class Base, bool T90deg_turns_allowed, bool Twormhole = false>
struct CFollowTrackT : CFollowTrack<Base, Twormhole>
{
	typedef typename Base::VehicleType VehicleType;

	inline CFollowTrackT(const VehicleType *v)
		: CFollowTrack<Base, Twormhole>(v, T90deg_turns_allowed)
	{
	}

	inline CFollowTrackT(const VehicleType *v, RailTypes railtype_override, CPerformanceTimer *pPerf = NULL)
		: CFollowTrack<Base, Twormhole>(v, T90deg_turns_allowed, railtype_override, pPerf)
	{
	}

	inline static bool Allow90degTurns() { return T90deg_turns_allowed; }
};

typedef CFollowTrackT<CFollowTrackWaterBase, true > CFollowTrackWater90;
typedef CFollowTrackT<CFollowTrackWaterBase, false> CFollowTrackWaterNo90;

typedef CFollowTrackT<CFollowTrackRoadBase, true> CFollowTrackRoad;

typedef CFollowTrackT<CFollowTrackRailBaseT<false>, true,  true> CFollowTrackRail90;
typedef CFollowTrackT<CFollowTrackRailBaseT<false>, false, true> CFollowTrackRailNo90;
typedef CFollowTrackT<CFollowTrackRailBaseT<true>,  true,  true> CFollowTrackFreeRail90;
typedef CFollowTrackT<CFollowTrackRailBaseT<true>,  false, true> CFollowTrackFreeRailNo90;

struct CFollowTrackRail : CFollowTrack<CFollowTrackRailBaseT<false>, true>
{
	inline CFollowTrackRail(const Train *v = NULL, bool allow_90deg = true, bool railtype_override = false)
		: CFollowTrack<CFollowTrackRailBaseT<false>, true>(v, allow_90deg, railtype_override ? GetRailTypeInfo(v->railtype)->compatible_railtypes : INVALID_RAILTYPES)
	{
	}

	inline CFollowTrackRail(Owner o, bool allow_90deg = true, RailTypes railtype_override = INVALID_RAILTYPES)
		: CFollowTrack<CFollowTrackRailBaseT<false>, true>(o, allow_90deg, railtype_override)
	{
	}
};

#endif /* FOLLOW_TRACK_HPP */
