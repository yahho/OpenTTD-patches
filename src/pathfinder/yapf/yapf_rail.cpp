/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_rail.cpp The rail pathfinding. */

#include "../../stdafx.h"

#include "yapf.hpp"
#include "yapf_node_rail.hpp"
#include "../../viewport_func.h"
#include "../../newgrf_station.h"
#include "../../station_func.h"
#include "../../misc/blob.hpp"
#include "../../misc/array.hpp"
#include "../../misc/hashtable.hpp"
#include "../../map/slope.h"
#include "../../pbs.h"
#include "../../waypoint_base.h"
#include "../../date_func.h"

#define DEBUG_YAPF_CACHE 0

#if DEBUG_YAPF_CACHE
template <typename Tpf> void DumpState(Tpf &pf1, Tpf &pf2)
{
	DumpTarget dmp1, dmp2;
	pf1.Dump(dmp1);
	pf2.Dump(dmp2);
	FILE *f1 = fopen("yapf1.txt", "wt");
	FILE *f2 = fopen("yapf2.txt", "wt");
	fwrite(dmp1.m_out.Data(), 1, dmp1.m_out.Size(), f1);
	fwrite(dmp2.m_out.Data(), 1, dmp2.m_out.Size(), f2);
	fclose(f1);
	fclose(f2);
}
#endif

int _total_pf_time_us = 0;


/**
 * Base class for segment cost cache providers. Contains global counter
 *  of track layout changes and static notification function called whenever
 *  the track layout changes. It is implemented as base class because it needs
 *  to be shared between all rail YAPF types (one shared counter, one notification
 *  function.
 */
struct CSegmentCostCacheBase
{
	static int s_rail_change_counter;

	static void NotifyTrackLayoutChange(TileIndex tile, Track track)
	{
		s_rail_change_counter++;
	}
};

/** if any track changes, this counter is incremented - that will invalidate segment cost cache */
int CSegmentCostCacheBase::s_rail_change_counter = 0;

void YapfNotifyTrackLayoutChange(TileIndex tile, Track track)
{
	CSegmentCostCacheBase::NotifyTrackLayoutChange(tile, track);
}


/**
 * CSegmentCostCacheT - template class providing hash-map and storage (heap)
 *  of Tsegment structures. Each rail node contains pointer to the segment
 *  that contains cached (or non-cached) segment cost information. Nodes can
 *  differ by key type, but they use the same segment type. Segment key should
 *  be always the same (TileIndex + DiagDirection) that represent the beginning
 *  of the segment (origin tile and exit-dir from this tile).
 *  Different CYapfCachedCostT types can share the same type of CSegmentCostCacheT.
 *  Look at CYapfRailSegment (yapf_node_rail.hpp) for the segment example
 */
template <class Tsegment>
struct CSegmentCostCacheT
	: public CSegmentCostCacheBase
{
	static const int C_HASH_BITS = 14;

	typedef CHashTableT<Tsegment, C_HASH_BITS> HashTable;
	typedef SmallArray<Tsegment> Heap;
	typedef typename Tsegment::Key Key;    ///< key to hash table

	HashTable    m_map;
	Heap         m_heap;

	inline CSegmentCostCacheT() {}

	/** flush (clear) the cache */
	inline void Flush()
	{
		m_map.Clear();
		m_heap.Clear();
	}

	inline Tsegment& Get(Key& key, bool *found)
	{
		Tsegment *item = m_map.Find(key);
		if (item == NULL) {
			*found = false;
			item = new (m_heap.Append()) Tsegment(key);
			m_map.Push(*item);
		} else {
			*found = true;
		}
		return *item;
	}
};


template <class Types>
class CYapfRailT : public Types::Astar
{
public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::Astar::Node Node;     ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables
	typedef typename Node::CachedData CachedData;
	typedef typename CachedData::Key CacheKey;
	typedef CSegmentCostCacheT<CachedData> Cache;
	typedef SmallArray<CachedData> LocalCache;

protected:
	const YAPFSettings *m_settings; ///< current settings (_settings_game.yapf)
	const Train        *m_veh;      ///< vehicle that we are trying to drive

	PathPos m_org;                            ///< first origin position
	PathPos m_rev;                            ///< second (reversed) origin position
	int     m_reverse_penalty;                ///< penalty to be added for using the reversed origin
	bool    m_treat_first_red_two_way_signal_as_eol; ///< in some cases (leaving station) we need to handle first two-way signal differently

	/**
	 * @note maximum cost doesn't work with caching enabled
	 * @todo fix maximum cost failing with caching (e.g. FS#2900)
	 */
	int           m_max_cost;
	CBlobT<int>   m_sig_look_ahead_costs;
	bool          m_disable_cache;
	Cache&        m_global_cache;
	LocalCache    m_local_cache;

	int                  m_stats_cost_calcs;   ///< stats - how many node's costs were calculated
	int                  m_stats_cache_hits;   ///< stats - how many node's costs were reused from cache

	CPerformanceTimer    m_perf_cost;          ///< stats - total CPU time of this run
	CPerformanceTimer    m_perf_slope_cost;    ///< stats - slope calculation CPU time
	CPerformanceTimer    m_perf_ts_cost;       ///< stats - GetTrackStatus() CPU time
	CPerformanceTimer    m_perf_other_cost;    ///< stats - other CPU time

public:
	bool          m_stopped_on_first_two_way_signal;

protected:
	static const int s_max_segment_cost = 10000;

	CYapfRailT()
		: m_settings(&_settings_game.pf.yapf)
		, m_veh(NULL)
		, m_max_cost(0)
		, m_disable_cache(false)
		, m_global_cache(stGetGlobalCache())
		, m_stats_cost_calcs(0)
		, m_stats_cache_hits(0)
		, m_stopped_on_first_two_way_signal(false)
	{
		/* pre-compute look-ahead penalties into array */
		int p0 = Yapf().PfGetSettings().rail_look_ahead_signal_p0;
		int p1 = Yapf().PfGetSettings().rail_look_ahead_signal_p1;
		int p2 = Yapf().PfGetSettings().rail_look_ahead_signal_p2;
		int *pen = m_sig_look_ahead_costs.GrowSizeNC(Yapf().PfGetSettings().rail_look_ahead_max_signals);
		for (uint i = 0; i < Yapf().PfGetSettings().rail_look_ahead_max_signals; i++) {
			pen[i] = p0 + i * (p1 + i * p2);
		}
	}

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

	inline static Cache& stGetGlobalCache()
	{
		static int last_rail_change_counter = 0;
		static Date last_date = 0;
		static Cache C;

		/* some statistics */
		if (last_date != _date) {
			last_date = _date;
			DEBUG(yapf, 2, "Pf time today: %5d ms", _total_pf_time_us / 1000);
			_total_pf_time_us = 0;
		}

		/* delete the cache sometimes... */
		if (last_rail_change_counter != Cache::s_rail_change_counter) {
			last_rail_change_counter = Cache::s_rail_change_counter;
			C.Flush();
		}

		return C;
	}

public:
	/** return current settings (can be custom - company based - but later) */
	inline const YAPFSettings& PfGetSettings() const
	{
		return *m_settings;
	}

	const Train * GetVehicle() const
	{
		return m_veh;
	}

	/** set origin */
	void SetOrigin(const PathPos &pos, const PathPos &rev = PathPos(), int reverse_penalty = 0, bool treat_first_red_two_way_signal_as_eol = true)
	{
		m_org = pos;
		m_rev = rev;
		m_reverse_penalty = reverse_penalty;
		m_treat_first_red_two_way_signal_as_eol = treat_first_red_two_way_signal_as_eol;
	}

	/** Create and add a new node */
	inline void AddStartupNode (const PathPos &pos, bool is_choice, int cost = 0)
	{
		Node *node = Types::Astar::CreateNewNode (NULL, pos, is_choice);
		node->m_cost = cost;
		PfNodeCacheFetch(*node);
		Types::Astar::InsertInitialNode(node);
	}

	/** Called when YAPF needs to place origin nodes into open list */
	void PfSetStartupNodes()
	{
		if (m_org.tile != INVALID_TILE && m_org.td != INVALID_TRACKDIR) {
			AddStartupNode(m_org, false);
		}
		if (m_rev.tile != INVALID_TILE && m_rev.td != INVALID_TRACKDIR) {
			AddStartupNode(m_rev, false, m_reverse_penalty);
		}
	}

	/** return true if first two-way signal should be treated as dead end */
	inline bool TreatFirstRedTwoWaySignalAsEOL()
	{
		return Yapf().PfGetSettings().rail_firstred_twoway_eol && m_treat_first_red_two_way_signal_as_eol;
	}

	struct NodeData {
		int parent_cost;
		int entry_cost;
		int segment_cost;
		int extra_cost;
		PathPos pos;
		PathPos last_signal;
		EndSegmentReasonBits end_reason;
	};

	inline int SlopeCost(const PathPos &pos)
	{
		CPerfStart perf_cost(Yapf().m_perf_slope_cost);

		if (pos.in_wormhole() || !IsDiagonalTrackdir(pos.td)) return 0;

		/* Only rail tracks and bridgeheads can have sloped rail. */
		if (!IsRailwayTile(pos.tile)) return 0;

		bool uphill;
		if (IsTileSubtype(pos.tile, TT_BRIDGE)) {
			/* it is bridge ramp, check if we are entering the bridge */
			DiagDirection dir = GetTunnelBridgeDirection(pos.tile);
			if (dir != TrackdirToExitdir(pos.td)) return 0; // no, we are leaving it, no penalty
			/* we are entering the bridge */
			Slope tile_slope = GetTileSlope(pos.tile);
			Axis axis = DiagDirToAxis(dir);
			uphill = !HasBridgeFlatRamp(tile_slope, axis);
		} else {
			/* not bridge ramp */
			Slope tile_slope = GetTileSlope(pos.tile);
			uphill = IsUphillTrackdir(tile_slope, pos.td); // slopes uphill => apply penalty
		}

		return uphill ? Yapf().PfGetSettings().rail_slope_penalty : 0;
	}

	inline int TransitionCost (const PathPos &pos1, const PathPos &pos2)
	{
		assert(IsValidTrackdir(pos1.td));
		assert(IsValidTrackdir(pos2.td));

		if (pos1.in_wormhole() || !IsRailwayTile(pos1.tile)) {
			assert(IsDiagonalTrackdir(pos1.td));
			if (pos2.in_wormhole() || !IsRailwayTile(pos2.tile)) {
				/* compare only the tracks, as depots cause reversing */
				assert(TrackdirToTrack(pos1.td) == TrackdirToTrack(pos2.td));
				return 0;
			} else {
				return (pos1.td == pos2.td) ? 0 : Yapf().PfGetSettings().rail_curve45_penalty;
			}
		} else {
			if (pos2.in_wormhole() || !IsRailwayTile(pos2.tile)) {
				assert(IsDiagonalTrackdir(pos2.td));
				return (pos1.td == pos2.td) ? 0 : Yapf().PfGetSettings().rail_curve45_penalty;
			}
		}

		/* both tiles are railway tiles */

		int cost = 0;
		if (TrackFollower::Allow90degTurns()
				&& ((TrackdirToTrackdirBits(pos2.td) & (TrackdirBits)TrackdirCrossesTrackdirs(pos1.td)) != 0)) {
			/* 90-deg curve penalty */
			cost += Yapf().PfGetSettings().rail_curve90_penalty;
		} else if (pos2.td != NextTrackdir(pos1.td)) {
			/* 45-deg curve penalty */
			cost += Yapf().PfGetSettings().rail_curve45_penalty;
		}

		DiagDirection exitdir = TrackdirToExitdir(pos1.td);
		bool t1 = KillFirstBit(GetTrackBits(pos1.tile) & DiagdirReachesTracks(ReverseDiagDir(exitdir))) != TRACK_BIT_NONE;
		bool t2 = KillFirstBit(GetTrackBits(pos2.tile) & DiagdirReachesTracks(exitdir)) != TRACK_BIT_NONE;
		if (t1 && t2) cost += Yapf().PfGetSettings().rail_doubleslip_penalty;

		return cost;
	}

	/** Return one tile cost (base cost + level crossing penalty). */
	inline int OneTileCost(const PathPos &pos)
	{
		int cost = 0;
		/* set base cost */
		if (IsDiagonalTrackdir(pos.td)) {
			cost += YAPF_TILE_LENGTH;
			if (IsLevelCrossingTile(pos.tile)) {
				/* Increase the cost for level crossings */
				cost += Yapf().PfGetSettings().rail_crossing_penalty;
			}
		} else {
			/* non-diagonal trackdir */
			cost = YAPF_TILE_CORNER_LENGTH;
		}
		return cost;
	}

	/** Check for a reserved station platform. */
	inline bool IsAnyStationTileReserved(const PathPos &pos, int skipped)
	{
		TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
		for (TileIndex tile = pos.tile; skipped >= 0; skipped--, tile += diff) {
			if (HasStationReservation(tile)) return true;
		}
		return false;
	}

	/** The cost for reserved tiles, including skipped ones. */
	inline int ReservationCost(Node& n, const PathPos &pos, int skipped)
	{
		if (n.m_num_signals_passed >= m_sig_look_ahead_costs.Size() / 2) return 0;
		if (!IsPbsSignal(n.m_last_signal_type)) return 0;

		if (!pos.in_wormhole() && IsRailStationTile(pos.tile) && IsAnyStationTileReserved(pos, skipped)) {
			return Yapf().PfGetSettings().rail_pbs_station_penalty * (skipped + 1);
		} else if (TrackOverlapsTracks(GetReservedTrackbits(pos.tile), TrackdirToTrack(pos.td))) {
			int cost = Yapf().PfGetSettings().rail_pbs_cross_penalty;
			if (!IsDiagonalTrackdir(pos.td)) cost = (cost * YAPF_TILE_CORNER_LENGTH) / YAPF_TILE_LENGTH;
			return cost * (skipped + 1);
		}
		return 0;
	}

	int SignalCost(Node& n, const PathPos &pos, NodeData *data)
	{
		int cost = 0;
		/* if there is one-way signal in the opposite direction, then it is not our way */
		CPerfStart perf_cost(Yapf().m_perf_other_cost);

		if (HasSignalAlongPos(pos)) {
			SignalState sig_state = GetSignalStateByPos(pos);
			SignalType sig_type = GetSignalType(pos);

			n.m_last_signal_type = sig_type;

			/* cache the look-ahead polynomial constant only if we didn't pass more signals than the look-ahead limit is */
			int look_ahead_cost = (n.m_num_signals_passed < m_sig_look_ahead_costs.Size()) ? m_sig_look_ahead_costs.Data()[n.m_num_signals_passed] : 0;
			if (sig_state != SIGNAL_STATE_RED) {
				/* green signal */
				n.flags_u.flags_s.m_last_signal_was_red = false;
				/* negative look-ahead red-signal penalties would cause problems later, so use them as positive penalties for green signal */
				if (look_ahead_cost < 0) {
					/* add its negation to the cost */
					cost -= look_ahead_cost;
				}
			} else {
				/* we have a red signal in our direction
				 * was it first signal which is two-way? */
				if (!IsPbsSignal(sig_type) && Yapf().TreatFirstRedTwoWaySignalAsEOL() && n.flags_u.flags_s.m_choice_seen && HasSignalAgainstPos(pos) && n.m_num_signals_passed == 0) {
					/* yes, the first signal is two-way red signal => DEAD END. Prune this branch... */
					Yapf().PruneIntermediateNodeBranch();
					data->end_reason |= ESRB_DEAD_END;
					Yapf().m_stopped_on_first_two_way_signal = true;
					return -1;
				}
				n.m_last_red_signal_type = sig_type;
				n.flags_u.flags_s.m_last_signal_was_red = true;

				/* look-ahead signal penalty */
				if (!IsPbsSignal(sig_type) && look_ahead_cost > 0) {
					/* add the look ahead penalty only if it is positive */
					cost += look_ahead_cost;
				}

				/* special signal penalties */
				if (n.m_num_signals_passed == 0) {
					switch (sig_type) {
						case SIGTYPE_COMBO:
						case SIGTYPE_EXIT:   cost += Yapf().PfGetSettings().rail_firstred_exit_penalty; break; // first signal is red pre-signal-exit
						case SIGTYPE_NORMAL:
						case SIGTYPE_ENTRY:  cost += Yapf().PfGetSettings().rail_firstred_penalty; break;
						default: break;
					}
				}
			}

			n.m_num_signals_passed++;
			data->last_signal = pos;
		} else if (HasSignalAgainstPos(pos)) {
			if (GetSignalType(pos) != SIGTYPE_PBS) {
				/* one-way signal in opposite direction */
				data->end_reason |= ESRB_DEAD_END;
			} else {
				cost += n.m_num_signals_passed < Yapf().PfGetSettings().rail_look_ahead_max_signals ? Yapf().PfGetSettings().rail_pbs_signal_back_penalty : 0;
			}
		}

		return cost;
	}

	inline int PlatformLengthPenalty(int platform_length)
	{
		int cost = 0;
		const Train *v = Yapf().GetVehicle();
		assert(v != NULL);
		assert(v->type == VEH_TRAIN);
		assert(v->gcache.cached_total_length != 0);
		int missing_platform_length = CeilDiv(v->gcache.cached_total_length, TILE_SIZE) - platform_length;
		if (missing_platform_length < 0) {
			/* apply penalty for longer platform than needed */
			cost += Yapf().PfGetSettings().rail_longer_platform_penalty + Yapf().PfGetSettings().rail_longer_platform_per_tile_penalty * -missing_platform_length;
		} else if (missing_platform_length > 0) {
			/* apply penalty for shorter platform than needed */
			cost += Yapf().PfGetSettings().rail_shorter_platform_penalty + Yapf().PfGetSettings().rail_shorter_platform_per_tile_penalty * missing_platform_length;
		}
		return cost;
	}

public:
	inline void SetMaxCost(int max_cost)
	{
		m_max_cost = max_cost;
	}

	inline void PfCalcSegment (Node &n, const TrackFollower *tf, NodeData *segment)
	{
		const Train *v = Yapf().GetVehicle();
		TrackFollower tf_local(v, Yapf().GetCompatibleRailTypes(), &Yapf().m_perf_ts_cost);

		TileIndex prev = n.m_parent->GetLastPos().tile;

		/* start at n and walk to the end of segment */
		segment->segment_cost = 0;
		segment->extra_cost = 0;
		segment->pos = n.GetPos();
		segment->last_signal = PathPos();
		segment->end_reason = ESRB_NONE;

		RailType rail_type = !segment->pos.in_wormhole() ? GetTileRailType(segment->pos.tile, TrackdirToTrack(segment->pos.td)) :
			IsRailwayTile(segment->pos.wormhole) ? GetBridgeRailType(segment->pos.wormhole) : GetRailType(segment->pos.wormhole);

		for (;;) {
			/* All other tile costs will be calculated here. */
			segment->segment_cost += OneTileCost(segment->pos);

			/* If we skipped some tunnel/bridge/station tiles, add their base cost */
			segment->segment_cost += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

			/* Slope cost. */
			segment->segment_cost += SlopeCost(segment->pos);

			/* Signal cost (routine can modify segment data). */
			segment->segment_cost += SignalCost(n, segment->pos, segment);

			/* Reserved tiles. */
			segment->segment_cost += ReservationCost(n, segment->pos, tf->m_tiles_skipped);

			/* Tests for 'potential target' reasons to close the segment. */
			if (segment->pos.tile == prev) {
				/* Penalty for reversing in a depot. */
				assert(!segment->pos.in_wormhole());
				assert(IsRailDepotTile(segment->pos.tile));
				assert(segment->pos.td == DiagDirToDiagTrackdir(GetGroundDepotDirection(segment->pos.tile)));
				segment->segment_cost += Yapf().PfGetSettings().rail_depot_reverse_penalty;
				/* We will end in this pass (depot is possible target) */
				segment->end_reason |= ESRB_DEPOT;

			} else if (!segment->pos.in_wormhole() && IsRailWaypointTile(segment->pos.tile)) {
				if (v->current_order.IsType(OT_GOTO_WAYPOINT) &&
						GetStationIndex(segment->pos.tile) == v->current_order.GetDestination() &&
						!Waypoint::Get(v->current_order.GetDestination())->IsSingleTile()) {
					/* This waypoint is our destination; maybe this isn't an unreserved
					 * one, so check that and if so see that as the last signal being
					 * red. This way waypoints near stations should work better. */
					CFollowTrackRail ft(v);
					ft.SetPos(segment->pos);

					bool add_extra_cost;
					for (;;) {
						if (!ft.FollowNext()) {
							/* end of line */
							add_extra_cost = !IsWaitingPositionFree(v, ft.m_old, _settings_game.pf.forbid_90_deg);
							break;
						}

						assert(ft.m_old.tile != ft.m_new.tile);
						if (!ft.m_new.is_single()) {
							/* We encountered a junction; it's going to be too complex to
							 * handle this perfectly, so just bail out. There is no simple
							 * free path, so try the other possibilities. */
							add_extra_cost = true;
							break;
						}

						/* If this is a safe waiting position we're done searching for it */
						PBSPositionState state = CheckWaitingPosition (v, ft.m_new, _settings_game.pf.forbid_90_deg);
						if (state != PBS_UNSAFE) {
							add_extra_cost = state == PBS_BUSY;
							break;
						}
					}

					/* In the case this platform is (possibly) occupied we add penalty so the
					 * other platforms of this waypoint are evaluated as well, i.e. we assume
					 * that there is a red signal in the waypoint when it's occupied. */
					if (add_extra_cost) segment->extra_cost += Yapf().PfGetSettings().rail_lastred_penalty;
				}
				/* Waypoint is also a good reason to finish. */
				segment->end_reason |= ESRB_WAYPOINT;

			} else if (tf->m_flag == tf->TF_STATION) {
				/* Station penalties. */
				uint platform_length = tf->m_tiles_skipped + 1;
				/* We don't know yet if the station is our target or not. Act like
				 * if it is pass-through station (not our destination). */
				segment->segment_cost += Yapf().PfGetSettings().rail_station_penalty * platform_length;
				/* We will end in this pass (station is possible target) */
				segment->end_reason |= ESRB_STATION;

			} else if (TrackFollower::DoTrackMasking()) {
				/* Searching for a safe tile? */
				if (HasSignalAlongPos(segment->pos) && !IsPbsSignal(GetSignalType(segment->pos))) {
					segment->end_reason |= ESRB_SAFE_TILE;
				}
			}

			/* Apply min/max speed penalties only when inside the look-ahead radius. Otherwise
			 * it would cause desync in MP. */
			if (n.m_num_signals_passed < m_sig_look_ahead_costs.Size())
			{
				int min_speed = 0;
				int max_speed = tf->GetSpeedLimit(&min_speed);
				int max_veh_speed = v->GetDisplayMaxSpeed();
				if (max_speed < max_veh_speed) {
					segment->extra_cost += YAPF_TILE_LENGTH * (max_veh_speed - max_speed) * (1 + tf->m_tiles_skipped) / max_veh_speed;
				}
				if (min_speed > max_veh_speed) {
					segment->extra_cost += YAPF_TILE_LENGTH * (min_speed - max_veh_speed);
				}
			}

			/* Finish if we already exceeded the maximum path cost (i.e. when
			 * searching for the nearest depot). */
			if (m_max_cost > 0 && (segment->parent_cost + segment->entry_cost + segment->segment_cost) > m_max_cost) {
				segment->end_reason |= ESRB_PATH_TOO_LONG;
			}

			/* Move to the next tile/trackdir. */
			tf = &tf_local;
			assert(tf_local.m_veh_owner == v->owner);
			assert(tf_local.m_railtypes == Yapf().GetCompatibleRailTypes());
			assert(tf_local.m_pPerf == &Yapf().m_perf_ts_cost);

			if (!tf_local.Follow(segment->pos)) {
				assert(tf_local.m_err != TrackFollower::EC_NONE);
				/* Can't move to the next tile (EOL?). */
				if (tf_local.m_err == TrackFollower::EC_RAIL_TYPE) {
					segment->end_reason |= ESRB_RAIL_TYPE;
				} else {
					segment->end_reason |= ESRB_DEAD_END;
				}

				if (TrackFollower::DoTrackMasking() && !HasOnewaySignalBlockingPos(segment->pos)) {
					segment->end_reason |= ESRB_SAFE_TILE;
				}
				break;
			}

			/* Check if the next tile is not a choice. */
			if (!tf_local.m_new.is_single()) {
				/* More than one segment will follow. Close this one. */
				segment->end_reason |= ESRB_CHOICE_FOLLOWS;
				break;
			}

			/* Gather the next tile/trackdir. */

			if (TrackFollower::DoTrackMasking()) {
				if (HasSignalAlongPos(tf_local.m_new) && IsPbsSignal(GetSignalType(tf_local.m_new))) {
					/* Possible safe tile. */
					segment->end_reason |= ESRB_SAFE_TILE;
				} else if (HasSignalAgainstPos(tf_local.m_new) && GetSignalType(tf_local.m_new) == SIGTYPE_PBS_ONEWAY) {
					/* Possible safe tile, but not so good as it's the back of a signal... */
					segment->end_reason |= ESRB_SAFE_TILE | ESRB_DEAD_END;
					segment->extra_cost += Yapf().PfGetSettings().rail_lastred_exit_penalty;
				}
			}

			/* Check the next tile for the rail type. */
			if (tf_local.m_new.in_wormhole()) {
				RailType next_rail_type = IsRailwayTile(tf_local.m_new.wormhole) ? GetBridgeRailType(tf_local.m_new.wormhole) : GetRailType(tf_local.m_new.wormhole);
				assert(next_rail_type == rail_type);
			} else if (GetTileRailType(tf_local.m_new.tile, TrackdirToTrack(tf_local.m_new.td)) != rail_type) {
				/* Segment must consist from the same rail_type tiles. */
				segment->end_reason |= ESRB_RAIL_TYPE;
				break;
			}

			/* Avoid infinite looping. */
			if (tf_local.m_new == n.GetPos()) {
				segment->end_reason |= ESRB_INFINITE_LOOP;
				break;
			}

			if (segment->segment_cost > s_max_segment_cost) {
				/* Potentially in the infinite loop (or only very long segment?). We should
				 * not force it to finish prematurely unless we are on a regular tile. */
				if (!tf->m_new.in_wormhole() && IsNormalRailTile(tf->m_new.tile)) {
					segment->end_reason |= ESRB_SEGMENT_TOO_LONG;
					break;
				}
			}

			/* Any other reason bit set? */
			if (segment->end_reason != ESRB_NONE) {
				break;
			}

			/* Transition cost (cost of the move from previous tile) */
			segment->segment_cost += TransitionCost (segment->pos, tf_local.m_new);

			/* For the next loop set new prev and cur tile info. */
			prev = segment->pos.tile;
			segment->pos = tf_local.m_new;
		} // for (;;)
	}

	/**
	 * Called by YAPF to calculate the cost from the origin to the given node.
	 *  Calculates only the cost of given node, adds it to the parent node cost
	 *  and stores the result into Node::m_cost member
	 */
	inline bool PfCalcCost(Node &n, const TrackFollower *tf)
	{
		assert(!n.flags_u.flags_s.m_targed_seen);
		assert(tf->m_new.tile == n.GetPos().tile);
		assert(tf->m_new.wormhole == n.GetPos().wormhole);
		assert((TrackdirToTrackdirBits(n.GetPos().td) & tf->m_new.trackdirs) != TRACKDIR_BIT_NONE);
		assert(n.m_parent != NULL);

		CPerfStart perf_cost(Yapf().m_perf_cost);

		/* Do we already have a cached segment? */
		CachedData &segment = *n.m_segment;
		bool is_cached_segment = (segment.m_cost >= 0);

		/* Each node cost contains 2 or 3 main components:
		 *  1. Transition cost - cost of the move from previous node (tile):
		 *    - curve cost (or zero for straight move)
		 *  2. Tile cost:
		 *    - base tile cost
		 *      - YAPF_TILE_LENGTH for diagonal tiles
		 *      - YAPF_TILE_CORNER_LENGTH for non-diagonal tiles
		 *    - tile penalties
		 *      - tile slope penalty (upward slopes)
		 *      - red signal penalty
		 *      - level crossing penalty
		 *      - speed-limit penalty (bridges)
		 *      - station platform penalty
		 *      - penalty for reversing in the depot
		 *      - etc.
		 *  3. Extra cost (applies to the last node only)
		 *    - last red signal penalty
		 *    - penalty for too long or too short platform on the destination station
		 */

		/* Segment: one or more tiles connected by contiguous tracks of the same type.
		 * Each segment cost includes 'Tile cost' for all its tiles (including the first
		 * and last), and the 'Transition cost' between its tiles. The first transition
		 * cost of segment entry (move from the 'parent' node) is not included!
		 */

		NodeData segment_data;
		segment_data.parent_cost = n.m_parent->m_cost;
		segment_data.entry_cost = TransitionCost (n.m_parent->GetLastPos(), n.GetPos());

		/* It is the right time now to look if we can reuse the cached segment cost. */
		if (is_cached_segment) {
			segment_data.segment_cost = segment.m_cost;
			segment_data.extra_cost = 0;
			segment_data.pos = n.GetLastPos();
			segment_data.last_signal = segment.m_last_signal;
			segment_data.end_reason = segment.m_end_segment_reason;
			/* We will need also some information about the last signal (if it was red). */
			if (segment.m_last_signal.tile != INVALID_TILE) {
				assert(HasSignalAlongPos(segment.m_last_signal));
				SignalState sig_state = GetSignalStateByPos(segment.m_last_signal);
				bool is_red = (sig_state == SIGNAL_STATE_RED);
				n.flags_u.flags_s.m_last_signal_was_red = is_red;
				if (is_red) {
					n.m_last_red_signal_type = GetSignalType(segment.m_last_signal);
				}
			}
			/* No further calculation needed. */
		} else {
			PfCalcSegment (n, tf, &segment_data);

			/* Write back the segment information so it can be reused the next time. */
			segment.m_last = segment_data.pos;
			segment.m_cost = segment_data.segment_cost;
			segment.m_last_signal = segment_data.last_signal;
			segment.m_end_segment_reason = segment_data.end_reason & ESRB_CACHED_MASK;

		}

		bool target_seen = false;
		if ((segment_data.end_reason & ESRB_POSSIBLE_TARGET) != ESRB_NONE) {
			/* Depot, station or waypoint. */
			if (Yapf().PfDetectDestination(segment_data.pos)) {
				/* Destination found. */
				target_seen = true;
			}
		}

		/* Do we have an excuse why not to continue pathfinding in this direction? */
		if (!target_seen && (segment_data.end_reason & ESRB_ABORT_PF_MASK) != ESRB_NONE) {
			/* Reason to not continue. Stop this PF branch. */
			return false;
		}

		/* Special costs for the case we have reached our target. */
		if (target_seen) {
			n.flags_u.flags_s.m_targed_seen = true;
			/* Last-red and last-red-exit penalties. */
			if (n.flags_u.flags_s.m_last_signal_was_red) {
				if (n.m_last_red_signal_type == SIGTYPE_EXIT) {
					/* last signal was red pre-signal-exit */
					segment_data.extra_cost += Yapf().PfGetSettings().rail_lastred_exit_penalty;
				} else if (!IsPbsSignal(n.m_last_red_signal_type)) {
					/* Last signal was red, but not exit or path signal. */
					segment_data.extra_cost += Yapf().PfGetSettings().rail_lastred_penalty;
				}
			}

			/* Station platform-length penalty. */
			if ((segment_data.end_reason & ESRB_STATION) != ESRB_NONE) {
				const BaseStation *st = BaseStation::GetByTile(n.GetLastPos().tile);
				assert(st != NULL);
				uint platform_length = st->GetPlatformLength(n.GetLastPos().tile, ReverseDiagDir(TrackdirToExitdir(n.GetLastPos().td)));
				/* Reduce the extra cost caused by passing-station penalty (each station receives it in the segment cost). */
				segment_data.extra_cost -= Yapf().PfGetSettings().rail_station_penalty * platform_length;
				/* Add penalty for the inappropriate platform length. */
				segment_data.extra_cost += PlatformLengthPenalty(platform_length);
			}
		}

		/* total node cost */
		n.m_cost = segment_data.parent_cost + segment_data.entry_cost + segment_data.segment_cost + segment_data.extra_cost;

		return true;
	}

	/**
	 * In some cases an intermediate node branch should be pruned.
	 * The most prominent case is when a red EOL signal is encountered, but
	 * there was a segment change (e.g. a rail type change) before that. If
	 * the branch would not be pruned, the rail type change location would
	 * remain the best intermediate node, and thus the vehicle would still
	 * go towards the red EOL signal.
	 */
	void PruneIntermediateNodeBranch()
	{
		while (Types::Astar::best_intermediate != NULL && (Types::Astar::best_intermediate->m_segment->m_end_segment_reason & ESRB_CHOICE_FOLLOWS) == 0) {
			Types::Astar::best_intermediate = Types::Astar::best_intermediate->m_parent;
		}
	}

	inline bool CanUseGlobalCache(Node& n) const
	{
		return !m_disable_cache
			&& (n.m_parent != NULL)
			&& (n.m_parent->m_num_signals_passed >= m_sig_look_ahead_costs.Size());
	}

	inline void ConnectNodeToCachedData(Node& n, CachedData& ci)
	{
		n.m_segment = &ci;
		if (n.m_segment->m_cost < 0) {
			n.m_segment->m_last = n.GetPos();
		}
	}

	void DisableCache(bool disable)
	{
		m_disable_cache = disable;
	}

	/**
	 * Called by YAPF to attach cached or local segment cost data to the given node.
	 *  @return true if globally cached data were used or false if local data was used
	 */
	inline bool PfNodeCacheFetch(Node& n)
	{
		CacheKey key(n.GetKey());
		if (!CanUseGlobalCache(n)) {
			ConnectNodeToCachedData(n, *new (m_local_cache.Append()) CachedData(key));
			return false;
		}
		bool found;
		CachedData& item = m_global_cache.Get(key, &found);
		ConnectNodeToCachedData(n, item);
		return found;
	}

	/** add multiple nodes - direct children of the given node */
	inline void AddMultipleNodes(Node *parent, const TrackFollower &tf)
	{
		bool is_choice = !tf.m_new.is_single();
		PathPos pos = tf.m_new;
		for (TrackdirBits rtds = tf.m_new.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.td = FindFirstTrackdir(rtds);
			Node& n = *Types::Astar::CreateNewNode(parent, pos, is_choice);
			Yapf().AddNewNode(n, tf);
		}
	}

	/**
	 * AddNewNode() - called by Tderived::PfFollowNode() for each child node.
	 *  Nodes are evaluated here and added into open list
	 */
	void AddNewNode(Node &n, const TrackFollower &tf)
	{
		/* evaluate the node */
		bool bCached = PfNodeCacheFetch(n);
		if (!bCached) {
			Yapf().m_stats_cost_calcs++;
		} else {
			Yapf().m_stats_cache_hits++;
		}

		bool bValid = PfCalcCost(n, &tf) && Yapf().PfCalcEstimate(n);

		/* have the cost or estimate callbacks marked this node as invalid? */
		if (!bValid) return;

		/* detect the destination */
		if (Yapf().PfDetectDestination(n)) {
			this->FoundTarget(&n);
		} else {
			this->InsertNode(&n);
		}
	}

	/** call the node follower */
	static inline void Follow (Tpf *pf, Node *n)
	{
		pf->PfFollowNode(*n);
	}

	/**
	 * Main pathfinder routine:
	 *   - set startup node(s)
	 *   - main loop that stops if:
	 *      - the destination was found
	 *      - or the open list is empty (no route to destination).
	 *      - or the maximum amount of loops reached - m_max_search_nodes (default = 10000)
	 * @return true if the path was found
	 */
	inline bool FindPath(const Train *v)
	{
		m_veh = v;

#ifndef NO_DEBUG_MESSAGES
		CPerformanceTimer perf;
		perf.Start();
#endif /* !NO_DEBUG_MESSAGES */

		Yapf().PfSetStartupNodes();
		bool bDestFound = Types::Astar::FindPath (Follow, PfGetSettings().max_search_nodes);

#ifndef NO_DEBUG_MESSAGES
		perf.Stop();
		if (_debug_yapf_level >= 2) {
			int t = perf.Get(1000000);
			_total_pf_time_us += t;

			if (_debug_yapf_level >= 3) {
				UnitID veh_idx = (m_veh != NULL) ? m_veh->unitnumber : 0;
				float cache_hit_ratio = (m_stats_cache_hits == 0) ? 0.0f : ((float)m_stats_cache_hits / (float)(m_stats_cache_hits + m_stats_cost_calcs) * 100.0f);
				int cost = bDestFound ? Types::Astar::best->m_cost : -1;
				int dist = bDestFound ? Types::Astar::best->m_estimate - Types::Astar::best->m_cost : -1;

				DEBUG(yapf, 3, "[YAPFt]%c%4d- %d us - %d rounds - %d open - %d closed - CHR %4.1f%% - C %d D %d - c%d(sc%d, ts%d, o%d) -- ",
					bDestFound ? '-' : '!', veh_idx, t, Types::Astar::num_steps, Types::Astar::OpenCount(), Types::Astar::ClosedCount(),
					cache_hit_ratio, cost, dist, m_perf_cost.Get(1000000), m_perf_slope_cost.Get(1000000),
					m_perf_ts_cost.Get(1000000), m_perf_other_cost.Get(1000000)
				);
			}
		}
#endif /* !NO_DEBUG_MESSAGES */
		return bDestFound;
	}
};


class CYapfDestinationRailBase
{
protected:
	RailTypes m_compatible_railtypes;

public:
	void SetDestination(const Train *v, bool override_rail_type = false)
	{
		m_compatible_railtypes = v->compatible_railtypes;
		if (override_rail_type) m_compatible_railtypes |= GetRailTypeInfo(v->railtype)->compatible_railtypes;
	}

	bool IsCompatibleRailType(RailType rt)
	{
		return HasBit(m_compatible_railtypes, rt);
	}

	RailTypes GetCompatibleRailTypes() const
	{
		return m_compatible_railtypes;
	}
};

template <class Types>
class CYapfDestinationAnyDepotRailT
	: public CYapfDestinationRailBase
{
public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::Astar::Node Node;     ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(Node& n)
	{
		return PfDetectDestination(n.GetLastPos());
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(const PathPos &pos)
	{
		return !pos.in_wormhole() && IsRailDepotTile(pos.tile);
	}

	/**
	 * Called by YAPF to calculate cost estimate. Calculates distance to the destination
	 *  adds it to the actual cost from origin and stores the sum to the Node::m_estimate
	 */
	inline bool PfCalcEstimate(Node& n)
	{
		n.m_estimate = n.m_cost;
		return true;
	}
};

template <class Types>
class CYapfDestinationAnySafeTileRailT
	: public CYapfDestinationRailBase
{
public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::Astar::Node Node;     ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables
	typedef typename Types::TrackFollower TrackFollower; ///< TrackFollower. Need to typedef for gcc 2.95

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(Node& n)
	{
		return PfDetectDestination(n.GetLastPos());
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(const PathPos &pos)
	{
		return IsFreeSafeWaitingPosition(Yapf().GetVehicle(), pos, !TrackFollower::Allow90degTurns());
	}

	/**
	 * Called by YAPF to calculate cost estimate. Calculates distance to the destination
	 *  adds it to the actual cost from origin and stores the sum to the Node::m_estimate.
	 */
	inline bool PfCalcEstimate(Node& n)
	{
		n.m_estimate = n.m_cost;
		return true;
	}
};

template <class Types>
class CYapfDestinationTileOrStationRailT
	: public CYapfDestinationRailBase
{
public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::Astar::Node Node;     ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables

protected:
	TileIndex    m_destTile;
	StationID    m_dest_station_id;

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	void SetDestination(const Train *v)
	{
		switch (v->current_order.GetType()) {
			case OT_GOTO_WAYPOINT:
				if (!Waypoint::Get(v->current_order.GetDestination())->IsSingleTile()) {
					/* In case of 'complex' waypoints we need to do a look
					 * ahead. This look ahead messes a bit about, which
					 * means that it 'corrupts' the cache. To prevent this
					 * we disable caching when we're looking for a complex
					 * waypoint. */
					Yapf().DisableCache(true);
				}
				/* FALL THROUGH */
			case OT_GOTO_STATION:
				m_dest_station_id = v->current_order.GetDestination();
				m_destTile = BaseStation::Get(m_dest_station_id)->GetClosestTile(v->tile, v->current_order.IsType(OT_GOTO_STATION) ? STATION_RAIL : STATION_WAYPOINT);
				break;

			default:
				m_destTile = v->dest_tile;
				m_dest_station_id = INVALID_STATION;
				break;
		}
		CYapfDestinationRailBase::SetDestination(v);
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(Node& n)
	{
		return PfDetectDestination(n.GetLastPos());
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(const PathPos &pos)
	{
		bool bDest;
		if (m_dest_station_id != INVALID_STATION) {
			bDest = !pos.in_wormhole() && HasStationTileRail(pos.tile)
				&& (GetStationIndex(pos.tile) == m_dest_station_id)
				&& (GetRailStationTrack(pos.tile) == TrackdirToTrack(pos.td));
		} else {
			bDest = !pos.in_wormhole() && (pos.tile == m_destTile);
		}
		return bDest;
	}

	/**
	 * Called by YAPF to calculate cost estimate. Calculates distance to the destination
	 *  adds it to the actual cost from origin and stores the sum to the Node::m_estimate
	 */
	inline bool PfCalcEstimate(Node& n)
	{
		static const int dg_dir_to_x_offs[] = {-1, 0, 1, 0};
		static const int dg_dir_to_y_offs[] = {0, 1, 0, -1};
		if (PfDetectDestination(n)) {
			n.m_estimate = n.m_cost;
			return true;
		}

		TileIndex tile = n.GetLastPos().tile;
		DiagDirection exitdir = TrackdirToExitdir(n.GetLastPos().td);
		int x1 = 2 * TileX(tile) + dg_dir_to_x_offs[(int)exitdir];
		int y1 = 2 * TileY(tile) + dg_dir_to_y_offs[(int)exitdir];
		int x2 = 2 * TileX(m_destTile);
		int y2 = 2 * TileY(m_destTile);
		int dx = abs(x1 - x2);
		int dy = abs(y1 - y2);
		int dmin = min(dx, dy);
		int dxy = abs(dx - dy);
		int d = dmin * YAPF_TILE_CORNER_LENGTH + (dxy - 1) * (YAPF_TILE_LENGTH / 2);
		n.m_estimate = n.m_cost + d;
		assert(n.m_estimate >= n.m_parent->m_estimate);
		return true;
	}
};


template <class Types>
class CYapfReserveTrack
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::Astar::Node Node;            ///< this will be our node type

protected:
	/** to access inherited pathfinder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

private:
	PathPos   m_res_dest;         ///< The reservation target
	Node      *m_res_node;        ///< The reservation target node
	TileIndex m_origin_tile;      ///< Tile our reservation will originate from

	/** Try to reserve a single track/platform. */
	bool ReserveSingleTrack(const PathPos &pos, PathPos *fail)
	{
		if (!pos.in_wormhole() && IsRailStationTile(pos.tile)) {
			TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
			TileIndex t = pos.tile;

			do {
				if (HasStationReservation(t)) {
					/* Platform could not be reserved, undo. */
					TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(pos.td));
					while (t != pos.tile) {
						t = TILE_ADD(t, diff);
						SetRailStationReservation(t, false);
					}
					*fail = pos;
					return false;
				}
				SetRailStationReservation(t, true);
				MarkTileDirtyByTile(t);
				t = TILE_ADD(t, diff);
			} while (IsCompatibleTrainStationTile(t, pos.tile) && t != m_origin_tile);

			TriggerStationRandomisation(NULL, pos.tile, SRT_PATH_RESERVATION);
		} else {
			if (!TryReserveRailTrack(pos)) {
				/* Tile couldn't be reserved, undo. */
				*fail = pos;
				return false;
			}
		}

		return true;
	}

	/** Unreserve a single track/platform. Stops when the previous failer is reached. */
	bool UnreserveSingleTrack(const PathPos &pos, PathPos *stop)
	{
		if (stop != NULL && pos == *stop) return false;

		if (!pos.in_wormhole() && IsRailStationTile(pos.tile)) {
			TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
			TileIndex     t = pos.tile;
			while (IsCompatibleTrainStationTile(t, pos.tile) && t != m_origin_tile) {
				assert(HasStationReservation(t));
				SetRailStationReservation(t, false);
				t = TILE_ADD(t, diff);
			}
		} else {
			UnreserveRailTrack(pos);
		}

		return pos != m_res_dest;
	}

public:
	/** Set the target to where the reservation should be extended. */
	inline void SetReservationTarget(Node *node, const PathPos &pos)
	{
		m_res_node = node;
		m_res_dest = pos;
	}

	/** Check the node for a possible reservation target. */
	inline void FindSafePositionOnNode(Node *node)
	{
		assert(node->m_parent != NULL);

		/* We will never pass more than two signals, no need to check for a safe tile. */
		if (node->m_parent->m_num_signals_passed >= 2) return;

		const Train *v = Yapf().GetVehicle();
		TrackFollower ft (v, Yapf().GetCompatibleRailTypes());
		ft.SetPos (node->GetPos());

		while (!IsSafeWaitingPosition(v, ft.m_new, !TrackFollower::Allow90degTurns())) {
			if (ft.m_new == node->GetLastPos()) return; // no safe position found in node
			bool follow = ft.FollowNext();
			assert (follow);
			assert (ft.m_new.is_single());
		}

		m_res_dest = ft.m_new;
		m_res_node = node;
	}

	/** Try to reserve the path till the reservation target. */
	bool TryReservePath(TileIndex origin, PathPos *target = NULL)
	{
		m_origin_tile = origin;

		if (target != NULL) *target = m_res_dest;

		/* Don't bother if the target is reserved. */
		const Train *v = Yapf().GetVehicle();
		if (!IsWaitingPositionFree(v, m_res_dest)) return false;

		TrackFollower ft (v, Yapf().GetCompatibleRailTypes());
		PathPos res_fail;

		for (Node *node = m_res_node; node->m_parent != NULL; node = node->m_parent) {
			ft.SetPos (node->GetPos());
			for (;;) {
				if (!ReserveSingleTrack (ft.m_new, &res_fail)) {
					/* Reservation failed, undo. */
					Node *failed_node = node;
					for (node = m_res_node; node != failed_node; node = node->m_parent) {
						ft.SetPos (node->GetPos());
						while (UnreserveSingleTrack(ft.m_new, NULL) && ft.m_new != node->GetLastPos()) {
							ft.FollowNext();
						}
					}
					ft.SetPos (failed_node->GetPos());
					while (UnreserveSingleTrack(ft.m_new, &res_fail) && ft.m_new != node->GetLastPos()) {
						ft.FollowNext();
					}
					return false;
				}

				if (ft.m_new == m_res_dest) break;
				if (ft.m_new == node->GetLastPos()) break;
				bool follow = ft.FollowNext();
				assert (follow);
				assert (ft.m_new.is_single());
			}
		}

		if (Yapf().CanUseGlobalCache(*m_res_node)) {
			YapfNotifyTrackLayoutChange(INVALID_TILE, INVALID_TRACK);
		}

		return true;
	}
};

template <class Types>
class CYapfFollowAnyDepotRailT
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::Astar::Node Node;            ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node& old_node)
	{
		TrackFollower F(Yapf().GetVehicle());
		if (F.Follow(old_node.GetLastPos())) {
			Yapf().AddMultipleNodes(&old_node, F);
		}
	}

	static bool stFindNearestDepotTwoWay(const Train *v, const PathPos &pos1, const PathPos &pos2, int max_penalty, int reverse_penalty, TileIndex *depot_tile, bool *reversed)
	{
		Tpf pf1;
		/*
		 * With caching enabled it simply cannot get a reliable result when you
		 * have limited the distance a train may travel. This means that the
		 * cached result does not match uncached result in all cases and that
		 * causes desyncs. So disable caching when finding for a depot that is
		 * nearby. This only happens with automatic servicing of vehicles,
		 * so it will only impact performance when you do not manually set
		 * depot orders and you do not disable automatic servicing.
		 */
		if (max_penalty != 0) pf1.DisableCache(true);
		bool result1 = pf1.FindNearestDepotTwoWay(v, pos1, pos2, max_penalty, reverse_penalty, depot_tile, reversed);

#if DEBUG_YAPF_CACHE
		Tpf pf2;
		TileIndex depot_tile2 = INVALID_TILE;
		bool reversed2 = false;
		pf2.DisableCache(true);
		bool result2 = pf2.FindNearestDepotTwoWay(v, pos1, pos2, max_penalty, reverse_penalty, &depot_tile2, &reversed2);
		if (result1 != result2 || (result1 && (*depot_tile != depot_tile2 || *reversed != reversed2))) {
			DEBUG(yapf, 0, "CACHE ERROR: FindNearestDepotTwoWay() = [%s, %s]", result1 ? "T" : "F", result2 ? "T" : "F");
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	inline bool FindNearestDepotTwoWay(const Train *v, const PathPos &pos1, const PathPos &pos2, int max_penalty, int reverse_penalty, TileIndex *depot_tile, bool *reversed)
	{
		/* set origin and destination nodes */
		Yapf().SetOrigin(pos1, pos2, reverse_penalty, true);
		Yapf().SetDestination(v);
		Yapf().SetMaxCost(max_penalty);

		/* find the best path */
		bool bFound = Yapf().FindPath(v);
		if (!bFound) return false;

		/* some path found
		 * get found depot tile */
		Node *n = Yapf().GetBestNode();
		*depot_tile = n->GetLastPos().tile;

		/* walk through the path back to the origin */
		Node *pNode = n;
		while (pNode->m_parent != NULL) {
			pNode = pNode->m_parent;
		}

		/* if the origin node is our front vehicle tile/Trackdir then we didn't reverse
		 * but we can also look at the cost (== 0 -> not reversed, == reverse_penalty -> reversed) */
		*reversed = (pNode->m_cost != 0);

		return true;
	}
};

template <class Types>
class CYapfFollowAnySafeTileRailT : public CYapfReserveTrack<Types>
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::Astar::Node Node;            ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node& old_node)
	{
		TrackFollower F(Yapf().GetVehicle(), Yapf().GetCompatibleRailTypes());
		if (F.Follow(old_node.GetLastPos()) && F.MaskReservedTracks()) {
			Yapf().AddMultipleNodes(&old_node, F);
		}
	}

	static bool stFindNearestSafeTile(const Train *v, const PathPos &pos, bool override_railtype)
	{
		/* Create pathfinder instance */
		Tpf pf1;
#if !DEBUG_YAPF_CACHE
		bool result1 = pf1.FindNearestSafeTile(v, pos, override_railtype, false);

#else
		bool result2 = pf1.FindNearestSafeTile(v, pos, override_railtype, true);
		Tpf pf2;
		pf2.DisableCache(true);
		bool result1 = pf2.FindNearestSafeTile(v, pos, override_railtype, false);
		if (result1 != result2) {
			DEBUG(yapf, 0, "CACHE ERROR: FindSafeTile() = [%s, %s]", result2 ? "T" : "F", result1 ? "T" : "F");
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	bool FindNearestSafeTile(const Train *v, const PathPos &pos, bool override_railtype, bool dont_reserve)
	{
		/* Set origin and destination. */
		Yapf().SetOrigin(pos);
		Yapf().SetDestination(v, override_railtype);

		bool bFound = Yapf().FindPath(v);
		if (!bFound) return false;

		/* Found a destination, set as reservation target. */
		Node *pNode = Yapf().GetBestNode();
		this->SetReservationTarget(pNode, pNode->GetLastPos());

		/* Walk through the path back to the origin. */
		Node *pPrev = NULL;
		while (pNode->m_parent != NULL) {
			pPrev = pNode;
			pNode = pNode->m_parent;

			this->FindSafePositionOnNode(pPrev);
		}

		return dont_reserve || this->TryReservePath(pNode->GetLastPos().tile);
	}
};

template <class Types>
class CYapfFollowRailT : public CYapfReserveTrack<Types>
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::Astar::Node Node;            ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node& old_node)
	{
		TrackFollower F(Yapf().GetVehicle());
		if (F.Follow(old_node.GetLastPos())) {
			Yapf().AddMultipleNodes(&old_node, F);
		}
	}

	static Trackdir stChooseRailTrack(const Train *v, const PathPos &origin, bool reserve_track, PFResult *target)
	{
		/* create pathfinder instance */
		Tpf pf1;
#if !DEBUG_YAPF_CACHE
		Trackdir result1 = pf1.ChooseRailTrack(v, origin, reserve_track, target);

#else
		Trackdir result1 = pf1.ChooseRailTrack(v, origin, false, NULL);
		Tpf pf2;
		pf2.DisableCache(true);
		Trackdir result2 = pf2.ChooseRailTrack(v, origin, reserve_track, target);
		if (result1 != result2) {
			DEBUG(yapf, 0, "CACHE ERROR: ChooseRailTrack() = [%d, %d]", result1, result2);
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	inline Trackdir ChooseRailTrack(const Train *v, const PathPos &origin, bool reserve_track, PFResult *target)
	{
		if (target != NULL) target->pos.tile = INVALID_TILE;

		/* set origin and destination nodes */
		Yapf().SetOrigin(origin);
		Yapf().SetDestination(v);

		/* find the best path */
		bool path_found = Yapf().FindPath(v);

		/* if path not found - return INVALID_TRACKDIR */
		Trackdir next_trackdir = INVALID_TRACKDIR;
		Node *pNode = Yapf().GetBestNode();
		if (pNode != NULL) {
			/* reserve till end of path */
			this->SetReservationTarget(pNode, pNode->GetLastPos());

			/* path was found or at least suggested
			 * walk through the path back to the origin */
			Node *pPrev = NULL;
			while (pNode->m_parent != NULL) {
				pPrev = pNode;
				pNode = pNode->m_parent;

				this->FindSafePositionOnNode(pPrev);
			}
			/* return trackdir from the best origin node (one of start nodes) */
			Node& best_next_node = *pPrev;
			next_trackdir = best_next_node.GetPos().td;

			if (reserve_track && path_found) {
				bool okay = this->TryReservePath(pNode->GetLastPos().tile, target != NULL ? &target->pos : NULL);
				if (target != NULL) target->okay = okay;
			}
		}

		/* Treat the path as found if stopped on the first two way signal(s). */
		if (target != NULL) target->found = path_found | Yapf().m_stopped_on_first_two_way_signal;
		return next_trackdir;
	}

	static bool stCheckReverseTrain(const Train *v, const PathPos &pos1, const PathPos &pos2, int reverse_penalty)
	{
		Tpf pf1;
		bool result1 = pf1.CheckReverseTrain(v, pos1, pos2, reverse_penalty);

#if DEBUG_YAPF_CACHE
		Tpf pf2;
		pf2.DisableCache(true);
		bool result2 = pf2.CheckReverseTrain(v, pos1, pos2, reverse_penalty);
		if (result1 != result2) {
			DEBUG(yapf, 0, "CACHE ERROR: CheckReverseTrain() = [%s, %s]", result1 ? "T" : "F", result2 ? "T" : "F");
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	inline bool CheckReverseTrain(const Train *v, const PathPos &pos1, const PathPos &pos2, int reverse_penalty)
	{
		/* create pathfinder instance
		 * set origin and destination nodes */
		Yapf().SetOrigin(pos1, pos2, reverse_penalty, false);
		Yapf().SetDestination(v);

		/* find the best path */
		bool bFound = Yapf().FindPath(v);

		if (!bFound) return false;

		/* path was found
		 * walk through the path back to the origin */
		Node *pNode = Yapf().GetBestNode();
		while (pNode->m_parent != NULL) {
			pNode = pNode->m_parent;
		}

		/* check if it was reversed origin */
		Node& best_org_node = *pNode;
		bool reversed = (best_org_node.m_cost != 0);
		return reversed;
	}
};

template <class Tpf_, class Ttrack_follower>
struct CYapfRail_TypesT
{
	typedef CYapfRail_TypesT<Tpf_, Ttrack_follower> Types;

	typedef Tpf_                                Tpf;
	typedef Ttrack_follower                     TrackFollower;
	typedef AstarRailTrackDir                   Astar;
};

struct CYapfRail1
	: CYapfRailT<CYapfRail_TypesT<CYapfRail1, CFollowTrackRail90> >
	, CYapfDestinationTileOrStationRailT<CYapfRail_TypesT<CYapfRail1, CFollowTrackRail90> >
	, CYapfFollowRailT<CYapfRail_TypesT<CYapfRail1, CFollowTrackRail90> >
{
};

struct CYapfRail2
	: CYapfRailT<CYapfRail_TypesT<CYapfRail2, CFollowTrackRailNo90> >
	, CYapfDestinationTileOrStationRailT<CYapfRail_TypesT<CYapfRail2, CFollowTrackRailNo90> >
	, CYapfFollowRailT<CYapfRail_TypesT<CYapfRail2, CFollowTrackRailNo90> >
{
};

struct CYapfAnyDepotRail1
	: CYapfRailT<CYapfRail_TypesT<CYapfAnyDepotRail1, CFollowTrackRail90> >
	, CYapfDestinationAnyDepotRailT<CYapfRail_TypesT<CYapfAnyDepotRail1, CFollowTrackRail90> >
	, CYapfFollowAnyDepotRailT<CYapfRail_TypesT<CYapfAnyDepotRail1, CFollowTrackRail90> >
{
};

struct CYapfAnyDepotRail2
	: CYapfRailT<CYapfRail_TypesT<CYapfAnyDepotRail2, CFollowTrackRailNo90> >
	, CYapfDestinationAnyDepotRailT<CYapfRail_TypesT<CYapfAnyDepotRail2, CFollowTrackRailNo90> >
	, CYapfFollowAnyDepotRailT<CYapfRail_TypesT<CYapfAnyDepotRail2, CFollowTrackRailNo90> >
{
};

struct CYapfAnySafeTileRail1
	: CYapfRailT<CYapfRail_TypesT<CYapfAnySafeTileRail1, CFollowTrackFreeRail90> >
	, CYapfDestinationAnySafeTileRailT<CYapfRail_TypesT<CYapfAnySafeTileRail1, CFollowTrackFreeRail90> >
	, CYapfFollowAnySafeTileRailT<CYapfRail_TypesT<CYapfAnySafeTileRail1, CFollowTrackFreeRail90> >
{
};

struct CYapfAnySafeTileRail2
	: CYapfRailT<CYapfRail_TypesT<CYapfAnySafeTileRail2, CFollowTrackFreeRailNo90> >
	, CYapfDestinationAnySafeTileRailT<CYapfRail_TypesT<CYapfAnySafeTileRail2, CFollowTrackFreeRailNo90> >
	, CYapfFollowAnySafeTileRailT<CYapfRail_TypesT<CYapfAnySafeTileRail2, CFollowTrackFreeRailNo90> >
{
};


Trackdir YapfTrainChooseTrack(const Train *v, const PathPos &origin, bool reserve_track, PFResult *target)
{
	/* default is YAPF type 2 */
	typedef Trackdir (*PfnChooseRailTrack)(const Train*, const PathPos&, bool, PFResult*);
	PfnChooseRailTrack pfnChooseRailTrack = &CYapfRail1::stChooseRailTrack;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnChooseRailTrack = &CYapfRail2::stChooseRailTrack; // Trackdir, forbid 90-deg
	}

	return pfnChooseRailTrack(v, origin, reserve_track, target);
}

bool YapfTrainCheckReverse(const Train *v)
{
	const Train *last_veh = v->Last();

	/* tiles where front and back are */
	PathPos pos = v->GetPos();
	PathPos rev = last_veh->GetReversePos();

	int reverse_penalty = 0;

	if (pos.in_wormhole()) {
		/* front in tunnel / on bridge */
		assert(TrackdirToExitdir(pos.td) == ReverseDiagDir(GetTunnelBridgeDirection(pos.wormhole)));

		/* Current position of the train in the wormhole */
		TileIndex cur_tile = TileVirtXY(v->x_pos, v->y_pos);

		/* Add distance to drive in the wormhole as penalty for the forward path, i.e. bonus for the reverse path
		 * Note: Negative penalties are ok for the start tile. */
		reverse_penalty -= DistanceManhattan(cur_tile, pos.tile) * YAPF_TILE_LENGTH;
	}

	if (rev.in_wormhole()) {
		/* back in tunnel / on bridge */
		assert(TrackdirToExitdir(rev.td) == ReverseDiagDir(GetTunnelBridgeDirection(rev.wormhole)));

		/* Current position of the last wagon in the wormhole */
		TileIndex cur_tile = TileVirtXY(last_veh->x_pos, last_veh->y_pos);

		/* Add distance to drive in the wormhole as penalty for the revere path. */
		reverse_penalty += DistanceManhattan(cur_tile, rev.tile) * YAPF_TILE_LENGTH;
	}

	typedef bool (*PfnCheckReverseTrain)(const Train*, const PathPos&, const PathPos&, int);
	PfnCheckReverseTrain pfnCheckReverseTrain = CYapfRail1::stCheckReverseTrain;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnCheckReverseTrain = &CYapfRail2::stCheckReverseTrain; // Trackdir, forbid 90-deg
	}

	/* slightly hackish: If the pathfinders finds a path, the cost of the first node is tested to distinguish between forward- and reverse-path. */
	if (reverse_penalty == 0) reverse_penalty = 1;

	bool reverse = pfnCheckReverseTrain(v, pos, rev, reverse_penalty);

	return reverse;
}

bool YapfTrainFindNearestDepot(const Train *v, uint max_penalty, FindDepotData *res)
{
	PathPos origin;
	FollowTrainReservation(v, &origin);
	PathPos rev = v->Last()->GetReversePos();

	typedef bool (*PfnFindNearestDepotTwoWay)(const Train*, const PathPos&, const PathPos&, int, int, TileIndex*, bool*);
	PfnFindNearestDepotTwoWay pfnFindNearestDepotTwoWay = &CYapfAnyDepotRail1::stFindNearestDepotTwoWay;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnFindNearestDepotTwoWay = &CYapfAnyDepotRail2::stFindNearestDepotTwoWay; // Trackdir, forbid 90-deg
	}

	return pfnFindNearestDepotTwoWay(v, origin, rev, max_penalty, YAPF_INFINITE_PENALTY, &res->tile, &res->reverse);
}

bool YapfTrainFindNearestSafeTile(const Train *v, const PathPos &pos, bool override_railtype)
{
	typedef bool (*PfnFindNearestSafeTile)(const Train*, const PathPos&, bool);
	PfnFindNearestSafeTile pfnFindNearestSafeTile = CYapfAnySafeTileRail1::stFindNearestSafeTile;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnFindNearestSafeTile = &CYapfAnySafeTileRail2::stFindNearestSafeTile;
	}

	return pfnFindNearestSafeTile(v, pos, override_railtype);
}
