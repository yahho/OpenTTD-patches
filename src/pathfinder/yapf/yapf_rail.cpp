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

	inline bool Get(const Key& key, Tsegment **item)
	{
		*item = m_map.Find(key);
		if (*item != NULL) return true;
		*item = new (m_heap.Append()) Tsegment(key);
		m_map.Push(**item);
		return false;
	}
};


template <class TAstar>
class CYapfRailBaseT : public TAstar
{
public:
	typedef typename TAstar::Node Node;           ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables
	typedef typename Node::CachedData CachedData;
	typedef typename CachedData::Key CacheKey;
	typedef CSegmentCostCacheT<CachedData> Cache;
	typedef SmallArray<CachedData> LocalCache;

protected:
	const YAPFSettings *const m_settings; ///< current settings (_settings_game.yapf)
	const Train        *const m_veh;      ///< vehicle that we are trying to drive
	const RailTypes           m_compatible_railtypes;
	const bool                mask_reserved_tracks;
	bool          m_treat_first_red_two_way_signal_as_eol; ///< in some cases (leaving station) we need to handle first two-way signal differently
public:
	bool          m_stopped_on_first_two_way_signal;

protected:
	bool          m_disable_cache;
	Cache&        m_global_cache;
	LocalCache    m_local_cache;

	/**
	 * @note maximum cost doesn't work with caching enabled
	 * @todo fix maximum cost failing with caching (e.g. FS#2900)
	 */
	int           m_max_cost;
	CBlobT<int>   m_sig_look_ahead_costs;

	int                  m_stats_cost_calcs;   ///< stats - how many node's costs were calculated
	int                  m_stats_cache_hits;   ///< stats - how many node's costs were reused from cache

	CPerformanceTimer    m_perf_cost;          ///< stats - total CPU time of this run
	CPerformanceTimer    m_perf_slope_cost;    ///< stats - slope calculation CPU time
	CPerformanceTimer    m_perf_ts_cost;       ///< stats - GetTrackStatus() CPU time
	CPerformanceTimer    m_perf_other_cost;    ///< stats - other CPU time

	CFollowTrackRail tf;       ///< track follower to be used by Follow
	CFollowTrackRail tf_local; ///< track follower to be used by CalcSegment

	static const int s_max_segment_cost = 10000;

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

	CYapfRailBaseT (const Train *v, bool allow_90deg, bool override_rail_type, bool mask_reserved_tracks)
		: m_settings(&_settings_game.pf.yapf)
		, m_veh(v)
		, m_compatible_railtypes(v->compatible_railtypes | (override_rail_type ? GetRailTypeInfo(v->railtype)->compatible_railtypes : RAILTYPES_NONE))
		, mask_reserved_tracks(mask_reserved_tracks)
		, m_treat_first_red_two_way_signal_as_eol(true)
		, m_stopped_on_first_two_way_signal(false)
		, m_disable_cache(false)
		, m_global_cache(stGetGlobalCache())
		, m_max_cost(0)
		, m_stats_cost_calcs(0)
		, m_stats_cache_hits(0)
		, tf (v, allow_90deg, mask_reserved_tracks ? m_compatible_railtypes : v->compatible_railtypes)
		, tf_local (v, allow_90deg, m_compatible_railtypes, &m_perf_ts_cost)
	{
		/* pre-compute look-ahead penalties into array */
		int p0 = m_settings->rail_look_ahead_signal_p0;
		int p1 = m_settings->rail_look_ahead_signal_p1;
		int p2 = m_settings->rail_look_ahead_signal_p2;
		int *pen = m_sig_look_ahead_costs.GrowSizeNC(m_settings->rail_look_ahead_max_signals);
		for (uint i = 0; i < m_settings->rail_look_ahead_max_signals; i++) {
			pen[i] = p0 + i * (p1 + i * p2);
		}
	}

public:
	inline bool Allow90degTurns (void) const
	{
		return tf.m_allow_90deg;
	}

	inline bool CanUseGlobalCache(Node& n) const
	{
		return !m_disable_cache
			&& (n.m_parent != NULL)
			&& (n.m_parent->m_num_signals_passed >= m_sig_look_ahead_costs.Size());
	}

protected:
	/**
	 * Called by YAPF to attach cached or local segment cost data to the given node.
	 *  @return true if globally cached data were used or false if local data was used
	 */
	inline bool AttachSegmentToNode(Node *n)
	{
		CacheKey key(n->GetKey());
		if (!CanUseGlobalCache(*n)) {
			n->m_segment = new (m_local_cache.Append()) CachedData(key);
			n->m_segment->m_last = n->GetPos();
			return false;
		}
		bool found = m_global_cache.Get(key, &n->m_segment);
		if (!found) n->m_segment->m_last = n->GetPos();
		return found;
	}

public:
	/** Create and add a new node */
	inline void AddStartupNode (const PathPos &pos, int cost = 0)
	{
		Node *node = TAstar::CreateNewNode (NULL, pos, false);
		node->m_cost = cost;
		AttachSegmentToNode(node);
		TAstar::InsertInitialNode(node);
	}

	/** set origin */
	void SetOrigin(const PathPos &pos)
	{
		AddStartupNode (pos);
	}

	/** set origin */
	void SetOrigin(const PathPos &pos, const PathPos &rev, int reverse_penalty, bool treat_first_red_two_way_signal_as_eol)
	{
		m_treat_first_red_two_way_signal_as_eol = treat_first_red_two_way_signal_as_eol;

		AddStartupNode (pos);
		AddStartupNode (rev, reverse_penalty);
	}

	inline void SetMaxCost (int max_cost)
	{
		m_max_cost = max_cost;
	}

	inline void DisableCache(bool disable)
	{
		m_disable_cache = disable;
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
		while (TAstar::best_intermediate != NULL && (TAstar::best_intermediate->m_segment->m_end_segment_reason & ESRB_CHOICE_FOLLOWS) == 0) {
			TAstar::best_intermediate = TAstar::best_intermediate->m_parent;
		}
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

	/** Return the transition cost from one tile to another. */
	inline int TransitionCost (const PathPos &pos1, const PathPos &pos2) const
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
				return (pos1.td == pos2.td) ? 0 : m_settings->rail_curve45_penalty;
			}
		} else {
			if (pos2.in_wormhole() || !IsRailwayTile(pos2.tile)) {
				assert(IsDiagonalTrackdir(pos2.td));
				return (pos1.td == pos2.td) ? 0 : m_settings->rail_curve45_penalty;
			}
		}

		/* both tiles are railway tiles */

		int cost = 0;
		if ((TrackdirToTrackdirBits(pos2.td) & (TrackdirBits)TrackdirCrossesTrackdirs(pos1.td)) != 0) {
			/* 90-deg curve penalty */
			cost += m_settings->rail_curve90_penalty;
		} else if (pos2.td != NextTrackdir(pos1.td)) {
			/* 45-deg curve penalty */
			cost += m_settings->rail_curve45_penalty;
		}

		DiagDirection exitdir = TrackdirToExitdir(pos1.td);
		bool t1 = KillFirstBit(GetTrackBits(pos1.tile) & DiagdirReachesTracks(ReverseDiagDir(exitdir))) != TRACK_BIT_NONE;
		bool t2 = KillFirstBit(GetTrackBits(pos2.tile) & DiagdirReachesTracks(exitdir)) != TRACK_BIT_NONE;
		if (t1 && t2) cost += m_settings->rail_doubleslip_penalty;

		return cost;
	}

	/** Return one tile cost (base cost + level crossing penalty). */
	inline int OneTileCost (const PathPos &pos) const
	{
		int cost = 0;
		/* set base cost */
		if (IsDiagonalTrackdir(pos.td)) {
			cost += YAPF_TILE_LENGTH;
			if (IsLevelCrossingTile(pos.tile)) {
				/* Increase the cost for level crossings */
				cost += m_settings->rail_crossing_penalty;
			}
		} else {
			/* non-diagonal trackdir */
			cost = YAPF_TILE_CORNER_LENGTH;
		}
		return cost;
	}

	/** Return slope cost for a tile. */
	inline int SlopeCost (const PathPos &pos)
	{
		CPerfStart perf_cost(m_perf_slope_cost);

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

		return uphill ? m_settings->rail_slope_penalty : 0;
	}

	/** Check for a reserved station platform. */
	static inline bool IsAnyStationTileReserved (const PathPos &pos, int skipped)
	{
		TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
		for (TileIndex tile = pos.tile; skipped >= 0; skipped--, tile += diff) {
			if (HasStationReservation(tile)) return true;
		}
		return false;
	}

	/** The cost for reserved tiles, including skipped ones. */
	inline int ReservationCost(Node& n, const PathPos &pos, int skipped) const
	{
		if (n.m_num_signals_passed >= m_sig_look_ahead_costs.Size() / 2) return 0;
		if (!IsPbsSignal(n.m_last_signal_type)) return 0;

		if (!pos.in_wormhole() && IsRailStationTile(pos.tile) && IsAnyStationTileReserved(pos, skipped)) {
			return m_settings->rail_pbs_station_penalty * (skipped + 1);
		} else if (TrackOverlapsTracks(GetReservedTrackbits(pos.tile), TrackdirToTrack(pos.td))) {
			int cost = m_settings->rail_pbs_cross_penalty;
			if (!IsDiagonalTrackdir(pos.td)) cost = (cost * YAPF_TILE_CORNER_LENGTH) / YAPF_TILE_LENGTH;
			return cost * (skipped + 1);
		}
		return 0;
	}

	/** Penalty for platform shorter/longer than needed. */
	inline int PlatformLengthPenalty (int platform_length) const
	{
		int cost = 0;
		const Train *v = m_veh;
		assert(v->gcache.cached_total_length != 0);
		int missing_platform_length = CeilDiv(v->gcache.cached_total_length, TILE_SIZE) - platform_length;
		if (missing_platform_length < 0) {
			/* apply penalty for longer platform than needed */
			cost += m_settings->rail_longer_platform_penalty + m_settings->rail_longer_platform_per_tile_penalty * -missing_platform_length;
		} else if (missing_platform_length > 0) {
			/* apply penalty for shorter platform than needed */
			cost += m_settings->rail_shorter_platform_penalty + m_settings->rail_shorter_platform_per_tile_penalty * missing_platform_length;
		}
		return cost;
	}

	int SignalCost(Node& n, const PathPos &pos, NodeData *data)
	{
		int cost = 0;
		/* if there is one-way signal in the opposite direction, then it is not our way */
		CPerfStart perf_cost(m_perf_other_cost);

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
				if (!IsPbsSignal(sig_type)
						&& m_settings->rail_firstred_twoway_eol
						&& m_treat_first_red_two_way_signal_as_eol
						&& n.flags_u.flags_s.m_choice_seen
						&& HasSignalAgainstPos(pos)
						&& n.m_num_signals_passed == 0) {
					/* yes, the first signal is two-way red signal => DEAD END. Prune this branch... */
					PruneIntermediateNodeBranch();
					data->end_reason |= ESRB_DEAD_END;
					m_stopped_on_first_two_way_signal = true;
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
						case SIGTYPE_EXIT:   cost += m_settings->rail_firstred_exit_penalty; break; // first signal is red pre-signal-exit
						case SIGTYPE_NORMAL:
						case SIGTYPE_ENTRY:  cost += m_settings->rail_firstred_penalty; break;
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
				cost += n.m_num_signals_passed < m_settings->rail_look_ahead_max_signals ? m_settings->rail_pbs_signal_back_penalty : 0;
			}
		}

		return cost;
	}

	/** Compute cost and modify node state for a position. */
	void HandleNodeTile (Node *n, const CFollowTrackRail *tf, NodeData *segment, TileIndex prev)
	{
		/* All other tile costs will be calculated here. */
		segment->segment_cost += OneTileCost(segment->pos);

		/* If we skipped some tunnel/bridge/station tiles, add their base cost */
		segment->segment_cost += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

		/* Slope cost. */
		segment->segment_cost += SlopeCost(segment->pos);

		/* Signal cost (routine can modify segment data). */
		segment->segment_cost += SignalCost(*n, segment->pos, segment);

		/* Reserved tiles. */
		segment->segment_cost += ReservationCost(*n, segment->pos, tf->m_tiles_skipped);

		/* Tests for 'potential target' reasons to close the segment. */
		if (segment->pos.tile == prev) {
			/* Penalty for reversing in a depot. */
			assert(!segment->pos.in_wormhole());
			assert(IsRailDepotTile(segment->pos.tile));
			assert(segment->pos.td == DiagDirToDiagTrackdir(GetGroundDepotDirection(segment->pos.tile)));
			segment->segment_cost += m_settings->rail_depot_reverse_penalty;
			/* We will end in this pass (depot is possible target) */
			segment->end_reason |= ESRB_DEPOT;

		} else if (!segment->pos.in_wormhole() && IsRailWaypointTile(segment->pos.tile)) {
			const Train *v = m_veh;
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
				if (add_extra_cost) segment->extra_cost += m_settings->rail_lastred_penalty;
			}
			/* Waypoint is also a good reason to finish. */
			segment->end_reason |= ESRB_WAYPOINT;

		} else if (tf->m_flag == tf->TF_STATION) {
			/* Station penalties. */
			uint platform_length = tf->m_tiles_skipped + 1;
			/* We don't know yet if the station is our target or not. Act like
			 * if it is pass-through station (not our destination). */
			segment->segment_cost += m_settings->rail_station_penalty * platform_length;
			/* We will end in this pass (station is possible target) */
			segment->end_reason |= ESRB_STATION;

		} else if (mask_reserved_tracks) {
			/* Searching for a safe tile? */
			if (HasSignalAlongPos(segment->pos) && !IsPbsSignal(GetSignalType(segment->pos))) {
				segment->end_reason |= ESRB_SAFE_TILE;
			}
		}

		/* Apply min/max speed penalties only when inside the look-ahead radius. Otherwise
		 * it would cause desync in MP. */
		if (n->m_num_signals_passed < m_sig_look_ahead_costs.Size())
		{
			int min_speed = 0;
			int max_speed = tf->GetSpeedLimit(&min_speed);
			int max_veh_speed = m_veh->GetDisplayMaxSpeed();
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
	}

	/** Check for possible reasons to end a segment at the next tile. */
	bool HandleNodeNextTile (Node *n, CFollowTrackRail *tf, NodeData *segment, RailType rail_type)
	{
		if (!tf->Follow(segment->pos)) {
			assert(tf->m_err != CFollowTrackRail::EC_NONE);
			/* Can't move to the next tile (EOL?). */
			if (tf->m_err == CFollowTrackRail::EC_RAIL_TYPE) {
				segment->end_reason |= ESRB_RAIL_TYPE;
			} else {
				segment->end_reason |= ESRB_DEAD_END;
			}

			if (mask_reserved_tracks && !HasOnewaySignalBlockingPos(segment->pos)) {
				segment->end_reason |= ESRB_SAFE_TILE;
			}
			return false;
		}

		/* Check if the next tile is not a choice. */
		if (!tf->m_new.is_single()) {
			/* More than one segment will follow. Close this one. */
			segment->end_reason |= ESRB_CHOICE_FOLLOWS;
			return false;
		}

		/* Gather the next tile/trackdir. */

		if (mask_reserved_tracks) {
			if (HasSignalAlongPos(tf->m_new) && IsPbsSignal(GetSignalType(tf->m_new))) {
				/* Possible safe tile. */
				segment->end_reason |= ESRB_SAFE_TILE;
			} else if (HasSignalAgainstPos(tf->m_new) && GetSignalType(tf->m_new) == SIGTYPE_PBS_ONEWAY) {
				/* Possible safe tile, but not so good as it's the back of a signal... */
				segment->end_reason |= ESRB_SAFE_TILE | ESRB_DEAD_END;
				segment->extra_cost += m_settings->rail_lastred_exit_penalty;
			}
		}

		/* Check the next tile for the rail type. */
		if (tf->m_new.in_wormhole()) {
			RailType next_rail_type = IsRailwayTile(tf->m_new.wormhole) ? GetBridgeRailType(tf->m_new.wormhole) : GetRailType(tf->m_new.wormhole);
			assert(next_rail_type == rail_type);
		} else if (GetTileRailType(tf->m_new.tile, TrackdirToTrack(tf->m_new.td)) != rail_type) {
			/* Segment must consist from the same rail_type tiles. */
			segment->end_reason |= ESRB_RAIL_TYPE;
			return false;
		}

		/* Avoid infinite looping. */
		if (tf->m_new == n->GetPos()) {
			segment->end_reason |= ESRB_INFINITE_LOOP;
			return false;
		}

		if (segment->segment_cost > s_max_segment_cost) {
			/* Potentially in the infinite loop (or only very long segment?). We should
			 * not force it to finish prematurely unless we are on a regular tile. */
			if (!tf->m_new.in_wormhole() && IsNormalRailTile(tf->m_new.tile)) {
				segment->end_reason |= ESRB_SEGMENT_TOO_LONG;
				return false;
			}
		}

		/* Any other reason bit set? */
		return segment->end_reason == ESRB_NONE;
	}

	/** Compute all costs for a newly-allocated (not cached) segment. */
	inline EndSegmentReasonBits CalcSegment (Node *n, const CFollowTrackRail *tf)
	{
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

		/* start at n and walk to the end of segment */
		NodeData segment;
		segment.parent_cost = n->m_parent->m_cost;
		segment.entry_cost = TransitionCost (n->m_parent->GetLastPos(), n->GetPos());
		segment.segment_cost = 0;
		segment.extra_cost = 0;
		segment.pos = n->GetPos();
		segment.last_signal = PathPos();
		segment.end_reason = ESRB_NONE;

		TileIndex prev = n->m_parent->GetLastPos().tile;

		RailType rail_type = !segment.pos.in_wormhole() ? GetTileRailType(segment.pos.tile, TrackdirToTrack(segment.pos.td)) :
			IsRailwayTile(segment.pos.wormhole) ? GetBridgeRailType(segment.pos.wormhole) : GetRailType(segment.pos.wormhole);

		for (;;) {
			HandleNodeTile (n, tf, &segment, prev);

			/* Move to the next tile/trackdir. */
			tf = &tf_local;
			assert(tf_local.m_veh_owner == m_veh->owner);
			assert(tf_local.m_railtypes == m_compatible_railtypes);
			assert(tf_local.m_pPerf == &m_perf_ts_cost);

			if (!HandleNodeNextTile (n, &tf_local, &segment, rail_type)) break;

			/* Transition cost (cost of the move from previous tile) */
			segment.segment_cost += TransitionCost (segment.pos, tf_local.m_new);

			/* For the next loop set new prev and cur tile info. */
			prev = segment.pos.tile;
			segment.pos = tf_local.m_new;
		} // for (;;)

		/* Write back the segment information so it can be reused the next time. */
		n->m_segment->m_last = segment.pos;
		n->m_segment->m_cost = segment.segment_cost;
		n->m_segment->m_last_signal = segment.last_signal;
		n->m_segment->m_end_segment_reason = segment.end_reason & ESRB_CACHED_MASK;

		/* total node cost */
		n->m_cost = segment.parent_cost + segment.entry_cost + segment.segment_cost + segment.extra_cost;
		return segment.end_reason;
	}

	/** Fill in a node from cached data. */
	inline EndSegmentReasonBits RestoreCachedNode (Node *n)
	{
		/* total node cost */
		n->m_cost = n->m_parent->m_cost + TransitionCost (n->m_parent->GetLastPos(), n->GetPos()) + n->m_segment->m_cost;
		/* We will need also some information about the last signal (if it was red). */
		if (n->m_segment->m_last_signal.tile != INVALID_TILE) {
			assert(HasSignalAlongPos(n->m_segment->m_last_signal));
			SignalState sig_state = GetSignalStateByPos(n->m_segment->m_last_signal);
			bool is_red = (sig_state == SIGNAL_STATE_RED);
			n->flags_u.flags_s.m_last_signal_was_red = is_red;
			if (is_red) {
				n->m_last_red_signal_type = GetSignalType(n->m_segment->m_last_signal);
			}
		}
		/* No further calculation needed. */
		return n->m_segment->m_end_segment_reason;
	}

	/** Add special extra cost when the segment reaches our target. */
	inline void AddTargetCost (Node *n, bool is_station)
	{
		n->flags_u.flags_s.m_targed_seen = true;

		/* Last-red and last-red-exit penalties. */
		if (n->flags_u.flags_s.m_last_signal_was_red) {
			if (n->m_last_red_signal_type == SIGTYPE_EXIT) {
				/* last signal was red pre-signal-exit */
				n->m_cost += m_settings->rail_lastred_exit_penalty;
			} else if (!IsPbsSignal(n->m_last_red_signal_type)) {
				/* Last signal was red, but not exit or path signal. */
				n->m_cost += m_settings->rail_lastred_penalty;
			}
		}

		/* Station platform-length penalty. */
		if (is_station) {
			const BaseStation *st = BaseStation::GetByTile(n->GetLastPos().tile);
			assert(st != NULL);
			uint platform_length = st->GetPlatformLength(n->GetLastPos().tile, ReverseDiagDir(TrackdirToExitdir(n->GetLastPos().td)));
			/* Reduce the extra cost caused by passing-station penalty (each station receives it in the segment cost). */
			n->m_cost -= m_settings->rail_station_penalty * platform_length;
			/* Add penalty for the inappropriate platform length. */
			n->m_cost += PlatformLengthPenalty(platform_length);
		}
	}

	/** Struct to store a position in a path (node and path position). */
	struct NodePos {
		PathPos pos;  ///< position (tile and trackdir)
		Node   *node; ///< node where the position is
	};

	/* Find the earliest safe position on a path. */
	Node *FindSafePositionOnPath (Node *node, NodePos *res);

	/* Try to reserve the path up to a given position. */
	bool TryReservePath (TileIndex origin, const NodePos *res);
};


/**
 * Find the earliest safe position on a path.
 * @param node Node to start searching back from (usually the best found node).
 * @param res Where to store the safe position found.
 * @return The first node in the path after the initial node.
 */
template <class TAstar>
inline typename TAstar::Node *CYapfRailBaseT<TAstar>::FindSafePositionOnPath (Node *node, NodePos *res)
{
	/* We will never pass more than two signals, no need to check for a safe tile. */
	assert (node->m_parent != NULL);
	while (node->m_parent->m_num_signals_passed >= 2) {
		node = node->m_parent;
		/* If the parent node has passed at least 2 signals, it
		 * cannot be a root node, because root nodes are single-tile
		 * nodes and can therefore have only one signal, if any. */
		assert (node->m_parent != NULL);
	}

	/* Default safe position if no other, earlier one is found. */
	res->node = node;
	res->pos  = node->GetLastPos();

	/* Walk through the path back to the origin. */
	CFollowTrackRail ft (m_veh, Allow90degTurns(), m_compatible_railtypes);

	for (;;) {
		Node *parent = node->m_parent;
		assert (parent != NULL);

		/* Search node for a safe position. */
		ft.SetPos (node->GetPos());
		for (;;) {
			if (IsSafeWaitingPosition (m_veh, ft.m_new, !Allow90degTurns())) {
				/* Found a safe position in this node. */
				res->node = node;
				res->pos  = ft.m_new;
				break;
			}

			if (ft.m_new == node->GetLastPos()) break; // no safe position found in node

			bool follow = ft.FollowNext();
			assert (follow);
			assert (ft.m_new.is_single());
		}

		/* Stop at node after initial node. */
		if (parent->m_parent == NULL) return node;
		node = parent;
	}
}

/**
 * Try to reserve a single track/platform
 * @param pos The position to reserve (last tile for platforms)
 * @param origin If pos is a platform, consider the platform to begin right after this tile
 * @return Whether reservation succeeded
 */
static bool ReserveSingleTrack (const PathPos &pos, TileIndex origin = INVALID_TILE)
{
	if (pos.in_wormhole() || !IsRailStationTile(pos.tile)) {
		return TryReserveRailTrack(pos);
	}

	TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
	TileIndex t = pos.tile;

	do {
		if (HasStationReservation(t)) {
			/* Platform could not be reserved, undo. */
			diff = -diff;
			while (t != pos.tile) {
				t = TILE_ADD(t, diff);
				SetRailStationReservation(t, false);
			}
			return false;
		}

		SetRailStationReservation(t, true);
		MarkTileDirtyByTile(t);
		t = TILE_ADD(t, diff);
	} while (IsCompatibleTrainStationTile(t, pos.tile) && t != origin);

	TriggerStationRandomisation(NULL, pos.tile, SRT_PATH_RESERVATION);

	return true;
}

/**
 * Unreserve a single track/platform
 * @param pos The position to reserve (last tile for platforms)
 * @param origin If pos is a platform, consider the platform to begin right after this tile
 */
static void UnreserveSingleTrack (const PathPos &pos, TileIndex origin = INVALID_TILE)
{
	if (pos.in_wormhole() || !IsRailStationTile(pos.tile)) {
		UnreserveRailTrack(pos);
		return;
	}

	TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
	TileIndex     t = pos.tile;
	while (IsCompatibleTrainStationTile(t, pos.tile) && t != origin) {
		assert(HasStationReservation(t));
		SetRailStationReservation(t, false);
		t = TILE_ADD(t, diff);
	}
}

/**
 * Try to reserve the path up to a given position.
 * @param origin Starting tile for the reservation.
 * @param res Target tile for the reservation.
 * @return Whether reservation succeeded.
 */
template <class TAstar>
bool CYapfRailBaseT<TAstar>::TryReservePath (TileIndex origin, const NodePos *res)
{
	/* Don't bother if the target is reserved. */
	if (!IsWaitingPositionFree(m_veh, res->pos)) return false;

	CFollowTrackRail ft (m_veh, Allow90degTurns(), m_compatible_railtypes);

	for (Node *node = res->node; node->m_parent != NULL; node = node->m_parent) {
		ft.SetPos (node->GetPos());
		for (;;) {
			if (!ReserveSingleTrack (ft.m_new, origin)) {
				/* Reservation failed, undo. */
				Node *failed_node = node;
				PathPos res_fail = ft.m_new;
				for (node = res->node; node != failed_node; node = node->m_parent) {
					ft.SetPos (node->GetPos());
					for (;;) {
						UnreserveSingleTrack (ft.m_new, origin);
						if (ft.m_new == res->pos) break;
						if (ft.m_new == node->GetLastPos()) break;
						ft.FollowNext();
					}
				}
				ft.SetPos (failed_node->GetPos());
				while (ft.m_new != res_fail) {
					assert (ft.m_new != res->pos);
					assert (ft.m_new != node->GetLastPos());
					UnreserveSingleTrack (ft.m_new, origin);
					ft.FollowNext();
				}
				return false;
			}

			if (ft.m_new == res->pos) break;
			if (ft.m_new == node->GetLastPos()) break;
			bool follow = ft.FollowNext();
			assert (follow);
			assert (ft.m_new.is_single());
		}
	}

	if (CanUseGlobalCache(*res->node)) {
		YapfNotifyTrackLayoutChange(INVALID_TILE, INVALID_TRACK);
	}

	return true;
}


template <class Tpf, class TAstar, bool Tmask_reserved_tracks>
class CYapfRailT : public CYapfRailBaseT <TAstar>
{
public:
	typedef CYapfRailBaseT <TAstar> Base;
	typedef typename TAstar::Node Node;           ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables
	typedef typename Node::CachedData CachedData;
	typedef typename CachedData::Key CacheKey;
	typedef CSegmentCostCacheT<CachedData> Cache;
	typedef SmallArray<CachedData> LocalCache;

protected:
	CYapfRailT (const Train *v, bool allow_90deg, bool override_rail_type = false)
		: Base (v, allow_90deg, override_rail_type, Tmask_reserved_tracks)
	{
	}

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/** Called by the A-star underlying class to find the neighbours of a node. */
	inline void Follow (Node *old_node)
	{
		if (!Base::tf.Follow(old_node->GetLastPos())) return;
		if (Tmask_reserved_tracks && !Base::tf.MaskReservedTracks()) return;

		bool is_choice = !Base::tf.m_new.is_single();
		PathPos pos = Base::tf.m_new;
		for (TrackdirBits rtds = Base::tf.m_new.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.td = FindFirstTrackdir(rtds);
			Node *n = TAstar::CreateNewNode(old_node, pos, is_choice);

			/* evaluate the node */
			bool cached = Base::AttachSegmentToNode(n);
			if (!cached) {
				Yapf().m_stats_cost_calcs++;
			} else {
				Yapf().m_stats_cache_hits++;
			}

			CPerfStart perf_cost(Yapf().m_perf_cost);

			EndSegmentReasonBits end_reason;
			if (cached) {
				end_reason = Base::RestoreCachedNode (n);
			} else {
				end_reason = Base::CalcSegment (n, &this->tf);
			}

			assert (((end_reason & ESRB_POSSIBLE_TARGET) != ESRB_NONE) || !Yapf().PfDetectDestination(n->GetLastPos()));

			if (((end_reason & ESRB_POSSIBLE_TARGET) != ESRB_NONE) &&
					Yapf().PfDetectDestination(n->GetLastPos())) {
				/* Special costs for the case we have reached our target. */
				Base::AddTargetCost (n, (end_reason & ESRB_STATION) != ESRB_NONE);
				perf_cost.Stop();
				n->m_estimate = n->m_cost;
				this->FoundTarget(n);

			} else if ((end_reason & ESRB_ABORT_PF_MASK) != ESRB_NONE) {
				/* Reason to not continue. Stop this PF branch. */
				continue;

			} else {
				perf_cost.Stop();
				Yapf().PfCalcEstimate(*n);
				this->InsertNode(n);
			}
		}
	}

	/** call the node follower */
	static inline void Follow (Tpf *pf, Node *n)
	{
		pf->Follow(n);
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
	inline bool FindPath (void)
	{
#ifndef NO_DEBUG_MESSAGES
		CPerformanceTimer perf;
		perf.Start();
#endif /* !NO_DEBUG_MESSAGES */

		bool bDestFound = TAstar::FindPath (Follow, Base::m_settings->max_search_nodes);

#ifndef NO_DEBUG_MESSAGES
		perf.Stop();
		if (_debug_yapf_level >= 2) {
			int t = perf.Get(1000000);
			_total_pf_time_us += t;

			if (_debug_yapf_level >= 3) {
				UnitID veh_idx = (Base::m_veh != NULL) ? Base::m_veh->unitnumber : 0;
				float cache_hit_ratio = (Base::m_stats_cache_hits == 0) ? 0.0f : ((float)Base::m_stats_cache_hits / (float)(Base::m_stats_cache_hits + Base::m_stats_cost_calcs) * 100.0f);
				int cost = bDestFound ? TAstar::best->m_cost : -1;
				int dist = bDestFound ? TAstar::best->m_estimate - TAstar::best->m_cost : -1;

				DEBUG(yapf, 3, "[YAPFt]%c%4d- %d us - %d rounds - %d open - %d closed - CHR %4.1f%% - C %d D %d - c%d(sc%d, ts%d, o%d) -- ",
					bDestFound ? '-' : '!', veh_idx, t, TAstar::num_steps, TAstar::OpenCount(), TAstar::ClosedCount(),
					cache_hit_ratio, cost, dist, Base::m_perf_cost.Get(1000000), Base::m_perf_slope_cost.Get(1000000),
					Base::m_perf_ts_cost.Get(1000000), Base::m_perf_other_cost.Get(1000000)
				);
			}
		}
#endif /* !NO_DEBUG_MESSAGES */
		return bDestFound;
	}
};


struct CYapfRail
	: CYapfRailT <CYapfRail, AstarRailTrackDir, false>
{
public:
	typedef CYapfRailT <CYapfRail, AstarRailTrackDir, false> Base;
	typedef AstarRailTrackDir TAstar;
	typedef TAstar::Node      Node;

protected:
	TileIndex m_dest_tile;
	StationID m_dest_station_id;

public:
	CYapfRail (const Train *v, bool allow_90deg)
		: Base (v, allow_90deg)
	{
		switch (v->current_order.GetType()) {
			case OT_GOTO_WAYPOINT:
				if (!Waypoint::Get(v->current_order.GetDestination())->IsSingleTile()) {
					/* In case of 'complex' waypoints we need to do a look
					 * ahead. This look ahead messes a bit about, which
					 * means that it 'corrupts' the cache. To prevent this
					 * we disable caching when we're looking for a complex
					 * waypoint. */
					Base::DisableCache(true);
				}
				/* FALL THROUGH */
			case OT_GOTO_STATION:
				m_dest_station_id = v->current_order.GetDestination();
				m_dest_tile = BaseStation::Get(m_dest_station_id)->GetClosestTile(v->tile, v->current_order.IsType(OT_GOTO_STATION) ? STATION_RAIL : STATION_WAYPOINT);
				break;

			default:
				m_dest_station_id = INVALID_STATION;
				m_dest_tile = v->dest_tile;
				break;
		}
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination (const PathPos &pos) const
	{
		if (m_dest_station_id != INVALID_STATION) {
			return !pos.in_wormhole() && HasStationTileRail(pos.tile)
				&& (GetStationIndex(pos.tile) == m_dest_station_id)
				&& (GetRailStationTrack(pos.tile) == TrackdirToTrack(pos.td));
		} else {
			return !pos.in_wormhole() && (pos.tile == m_dest_tile);
		}
	}

	/**
	 * Called by YAPF to calculate cost estimate. Calculates distance to the destination
	 *  adds it to the actual cost from origin and stores the sum to the Node::m_estimate
	 */
	inline void PfCalcEstimate (Node &n) const
	{
		static const int dg_dir_to_x_offs[] = {-1, 0, 1, 0};
		static const int dg_dir_to_y_offs[] = {0, 1, 0, -1};

		TileIndex tile = n.GetLastPos().tile;
		DiagDirection exitdir = TrackdirToExitdir(n.GetLastPos().td);
		int x1 = 2 * TileX(tile) + dg_dir_to_x_offs[(int)exitdir];
		int y1 = 2 * TileY(tile) + dg_dir_to_y_offs[(int)exitdir];
		int x2 = 2 * TileX(m_dest_tile);
		int y2 = 2 * TileY(m_dest_tile);
		int dx = abs(x1 - x2);
		int dy = abs(y1 - y2);
		int dmin = min(dx, dy);
		int dxy = abs(dx - dy);
		int d = dmin * YAPF_TILE_CORNER_LENGTH + (dxy - 1) * (YAPF_TILE_LENGTH / 2);
		n.m_estimate = n.m_cost + d;
		assert(n.m_estimate >= n.m_parent->m_estimate);
	}

	inline Trackdir ChooseRailTrack(const PathPos &origin, bool reserve_track, PFResult *target)
	{
		if (target != NULL) target->pos.tile = INVALID_TILE;

		/* set origin and destination nodes */
		Base::SetOrigin(origin);

		/* find the best path */
		bool path_found = Base::FindPath();

		/* if path not found - return INVALID_TRACKDIR */
		Trackdir next_trackdir = INVALID_TRACKDIR;
		Node *pNode = Base::GetBestNode();
		if (pNode != NULL) {
			if (reserve_track && path_found) {
				typename Base::NodePos res;
				Node *best_next_node = Base::FindSafePositionOnPath (pNode, &res);
				if (target != NULL) target->pos = res.pos;
				/* return trackdir from the best origin node (one of start nodes) */
				next_trackdir = best_next_node->GetPos().td;

				assert (best_next_node->m_parent->GetPos() == origin);
				assert (best_next_node->m_parent->GetLastPos() == origin);
				bool okay = Base::TryReservePath (origin.tile, &res);
				if (target != NULL) target->okay = okay;
			} else {
				while (pNode->m_parent->m_parent != NULL) {
					pNode = pNode->m_parent;
				}
				assert (pNode->m_parent->GetPos() == origin);
				assert (pNode->m_parent->GetLastPos() == origin);
				/* return trackdir from the best origin node (one of start nodes) */
				next_trackdir = pNode->GetPos().td;
			}
		}

		/* Treat the path as found if stopped on the first two way signal(s). */
		if (target != NULL) target->found = path_found | Base::m_stopped_on_first_two_way_signal;
		return next_trackdir;
	}

	inline bool CheckReverseTrain(const PathPos &pos1, const PathPos &pos2, int reverse_penalty)
	{
		/* create pathfinder instance
		 * set origin and destination nodes */
		Base::SetOrigin(pos1, pos2, reverse_penalty, false);

		/* find the best path */
		bool bFound = Base::FindPath();

		if (!bFound) return false;

		/* path was found
		 * walk through the path back to the origin */
		Node *pNode = Base::GetBestNode();
		while (pNode->m_parent != NULL) {
			pNode = pNode->m_parent;
		}

		/* check if it was reversed origin */
		Node& best_org_node = *pNode;
		bool reversed = (best_org_node.m_cost != 0);
		return reversed;
	}
};

Trackdir YapfTrainChooseTrack(const Train *v, const PathPos &origin, bool reserve_track, PFResult *target)
{
	/* create pathfinder instance */
	CYapfRail pf1 (v, !_settings_game.pf.forbid_90_deg);
#if !DEBUG_YAPF_CACHE
	Trackdir result1 = pf1.ChooseRailTrack(origin, reserve_track, target);

#else
	Trackdir result1 = pf1.ChooseRailTrack(origin, false, NULL);
	CYapfRail pf2 (v, !_settings_game.pf.forbid_90_deg);
	pf2.DisableCache(true);
	Trackdir result2 = pf2.ChooseRailTrack(origin, reserve_track, target);
	if (result1 != result2) {
		DEBUG(yapf, 0, "CACHE ERROR: ChooseRailTrack() = [%d, %d]", result1, result2);
		DumpState(pf1, pf2);
	}
#endif

	return result1;
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

	/* slightly hackish: If the pathfinders finds a path, the cost of the first node is tested to distinguish between forward- and reverse-path. */
	if (reverse_penalty == 0) reverse_penalty = 1;

	CYapfRail pf1 (v, !_settings_game.pf.forbid_90_deg);
	bool result1 = pf1.CheckReverseTrain(pos, rev, reverse_penalty);

#if DEBUG_YAPF_CACHE
	CYapfRail pf2 (v, !_settings_game.pf.forbid_90_deg);
	pf2.DisableCache(true);
	bool result2 = pf2.CheckReverseTrain(pos, rev, reverse_penalty);
	if (result1 != result2) {
		DEBUG(yapf, 0, "CACHE ERROR: CheckReverseTrain() = [%s, %s]", result1 ? "T" : "F", result2 ? "T" : "F");
		DumpState(pf1, pf2);
	}
#endif

	return result1;
}


struct CYapfAnyDepotRail
	: CYapfRailT <CYapfAnyDepotRail, AstarRailTrackDir, false>
{
public:
	typedef CYapfRailT <CYapfAnyDepotRail, AstarRailTrackDir, false> Base;
	typedef AstarRailTrackDir TAstar;
	typedef TAstar::Node      Node;

	CYapfAnyDepotRail (const Train *v, bool allow_90deg)
		: Base (v, allow_90deg)
	{
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination (const PathPos &pos) const
	{
		return !pos.in_wormhole() && IsRailDepotTile(pos.tile);
	}

	/** Called by YAPF to calculate cost estimate. */
	inline void PfCalcEstimate (Node &n) const
	{
		n.m_estimate = n.m_cost;
	}

	inline bool FindNearestDepotTwoWay(const PathPos &pos1, const PathPos &pos2, int max_penalty, int reverse_penalty, TileIndex *depot_tile, bool *reversed)
	{
		/* set origin and destination nodes */
		Base::SetOrigin (pos1, pos2, reverse_penalty, true);
		Base::SetMaxCost(max_penalty);

		/* find the best path */
		bool bFound = Base::FindPath();
		if (!bFound) return false;

		/* some path found
		 * get found depot tile */
		Node *n = Base::GetBestNode();
		*depot_tile = n->GetLastPos().tile;

		/* walk through the path back to the origin */
		while (n->m_parent != NULL) {
			n = n->m_parent;
		}

		/* if the origin node is our front vehicle tile/Trackdir then we didn't reverse
		 * but we can also look at the cost (== 0 -> not reversed, == reverse_penalty -> reversed) */
		*reversed = (n->m_cost != 0);

		return true;
	}
};

bool YapfTrainFindNearestDepot(const Train *v, uint max_penalty, FindDepotData *res)
{
	PathPos origin;
	FollowTrainReservation(v, &origin);
	PathPos rev = v->Last()->GetReversePos();

	CYapfAnyDepotRail pf1 (v, !_settings_game.pf.forbid_90_deg);
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
	bool result1 = pf1.FindNearestDepotTwoWay (origin, rev, max_penalty, YAPF_INFINITE_PENALTY, &res->tile, &res->reverse);

#if DEBUG_YAPF_CACHE
	CYapfAnyDepotRail pf2 (v, !_settings_game.pf.forbid_90_deg);
	TileIndex depot_tile2 = INVALID_TILE;
	bool reversed2 = false;
	pf2.DisableCache(true);
	bool result2 = pf2.FindNearestDepotTwoWay (origin, rev, max_penalty, YAPF_INFINITE_PENALTY, &depot_tile2, &reversed2);
	if (result1 != result2 || (result1 && (res->tile != depot_tile2 || res->reverse != reversed2))) {
		DEBUG(yapf, 0, "CACHE ERROR: FindNearestDepotTwoWay() = [%s, %s]", result1 ? "T" : "F", result2 ? "T" : "F");
		DumpState(pf1, pf2);
	}
#endif

	return result1;
}


struct CYapfAnySafeTileRail
	: CYapfRailT <CYapfAnySafeTileRail, AstarRailTrackDir, true>
{
public:
	typedef CYapfRailT <CYapfAnySafeTileRail, AstarRailTrackDir, true> Base;
	typedef AstarRailTrackDir TAstar;
	typedef TAstar::Node      Node;

	CYapfAnySafeTileRail (const Train *v, bool allow_90deg, bool override_railtype)
		: Base (v, allow_90deg, override_railtype)
	{
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination (const PathPos &pos) const
	{
		return IsFreeSafeWaitingPosition(Base::m_veh, pos, Base::Allow90degTurns());
	}

	/** Called by YAPF to calculate cost estimate. */
	inline void PfCalcEstimate (Node &n) const
	{
		n.m_estimate = n.m_cost;
	}

	bool FindNearestSafeTile(const PathPos &pos, bool override_railtype, bool dont_reserve)
	{
		/* Set origin and destination. */
		Base::SetOrigin(pos);

		bool bFound = Base::FindPath();
		if (!bFound) return false;

		if (dont_reserve) return true;

		/* Found a destination, search for a reservation target. */
		Node *pNode = Base::GetBestNode();
		typename Base::NodePos res;
		pNode = Base::FindSafePositionOnPath(pNode, &res)->m_parent;
		assert (pNode->GetPos() == pos);
		assert (pNode->GetLastPos() == pos);

		return Base::TryReservePath (pos.tile, &res);
	}
};

bool YapfTrainFindNearestSafeTile(const Train *v, const PathPos &pos, bool override_railtype)
{
	/* Create pathfinder instance */
	CYapfAnySafeTileRail pf1 (v, !_settings_game.pf.forbid_90_deg, override_railtype);
#if !DEBUG_YAPF_CACHE
	bool result1 = pf1.FindNearestSafeTile(pos, override_railtype, false);

#else
	bool result2 = pf1.FindNearestSafeTile(pos, override_railtype, true);
	CYapfAnySafeTileRail pf2 (v, !_settings_game.pf.forbid_90_deg, override_railtype);
	pf2.DisableCache(true);
	bool result1 = pf2.FindNearestSafeTile(pos, override_railtype, false);
	if (result1 != result2) {
		DEBUG(yapf, 0, "CACHE ERROR: FindSafeTile() = [%s, %s]", result2 ? "T" : "F", result1 ? "T" : "F");
		DumpState(pf1, pf2);
	}
#endif

	return result1;
}
