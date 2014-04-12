/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_rail.cpp The rail pathfinding. */

#include "../../stdafx.h"

#include <bitset>
#include <vector>

#include "yapf.hpp"
#include "../railpos.h"
#include "../../viewport_func.h"
#include "../../newgrf_station.h"
#include "../../station_func.h"
#include "../../misc/array.hpp"
#include "../../misc/hashtable.hpp"
#include "../../map/slope.h"
#include "../../pbs.h"
#include "../../waypoint_base.h"
#include "../../date_func.h"

#define DEBUG_YAPF_CACHE 0

int _total_pf_time_us = 0;


/* Enum used in PfCalcCost() to see why was the segment closed. */
enum EndSegmentReason {
	ESR_DEAD_END,  ///< track ends here
	ESR_RAIL_TYPE, ///< the next tile has a different rail type than our tiles
	ESR_LOOP,      ///< loop detected
	ESR_TOO_LONG,  ///< the segment is too long (possible loop)
	ESR_CHOICE,    ///< the next tile contains a choice (the track splits to more than one segment)
	ESR_DEPOT,     ///< stop in the depot (could be a target next time)
	ESR_WAYPOINT,  ///< waypoint encountered (could be a target next time)
	ESR_STATION,   ///< station encountered (could be a target next time)
	ESR_SAFE_TILE, ///< safe waiting position found (could be a target)
	ESR_MAX_COST,  ///< maximum pathfinding cost exceeded
	ESR__N
};

typedef std::bitset<ESR__N> EndSegmentReasonBits;

/* What reasons mean that the target can be found and needs to be detected. */
static const uint ESRB_POSSIBLE_TARGET =
	(1 << ESR_DEPOT) | (1 << ESR_WAYPOINT) | (1 << ESR_STATION) | (1 << ESR_SAFE_TILE);

/* Reasons to abort pathfinding in this direction. */
static const uint ESRB_ABORT_PF_MASK =
	(1 << ESR_DEAD_END) | (1 << ESR_LOOP) | (1 << ESR_MAX_COST);

inline void WriteValueStr(EndSegmentReasonBits bits, FILE *f)
{
	static const char * const end_segment_reason_names[] = {
		"DEAD_END", "RAIL_TYPE", "LOOP", "TOO_LONG", "CHOICE",
		"DEPOT", "WAYPOINT", "STATION", "SAFE_TILE", "MAX_COST",
	};

	int esr = bits.to_ulong();
	fprintf (f, "0x%04X (", esr);
	ComposeNameT (f, esr, end_segment_reason_names, "UNK", 0, "NONE");
	putc (')', f);
}


/** key for YAPF rail nodes */
typedef CYapfNodeKeyTrackDir<RailPathPos> CYapfRailKey;

/** key for cached segment cost for rail YAPF */
struct CYapfRailSegmentKey
{
	uint32    m_value;

	inline CYapfRailSegmentKey(const CYapfRailKey& node_key) : m_value(node_key.CalcHash()) { }

	inline int32 CalcHash() const
	{
		return m_value;
	}

	inline bool operator == (const CYapfRailSegmentKey& other) const
	{
		return m_value == other.m_value;
	}

	void Dump(DumpTarget &dmp) const
	{
		dmp.WriteTile("tile", (TileIndex)(m_value >> 4));
		dmp.WriteEnumT("td", (Trackdir)(m_value & 0x0F));
	}
};

/** cached segment cost for rail YAPF */
struct CYapfRailSegment : CHashTableEntryT <CYapfRailSegment>
{
	typedef CYapfRailSegmentKey Key;

	CYapfRailSegmentKey    m_key;
	RailPathPos            m_last;
	int                    m_cost;
	RailPathPos            m_last_signal;
	EndSegmentReasonBits   m_end_segment_reason;

	inline CYapfRailSegment (const CYapfRailKey &key)
		: m_key(key)
		, m_last(key)
		, m_cost(0)
		, m_last_signal()
		, m_end_segment_reason()
	{}

	inline const Key& GetKey() const
	{
		return m_key;
	}

	inline bool operator == (const CYapfRailSegment &other)
	{
		return  m_key == other.m_key &&
			m_last == other.m_last &&
			m_cost == other.m_cost &&
			m_last_signal == other.m_last_signal &&
			m_end_segment_reason == other.m_end_segment_reason;
	}

	void Dump(DumpTarget &dmp) const
	{
		dmp.WriteStructT("m_key", &m_key);
		dmp.WriteTile("m_last.tile", m_last.tile);
		dmp.WriteEnumT("m_last.td", m_last.td);
		dmp.WriteLine("m_cost = %d", m_cost);
		dmp.WriteTile("m_last_signal.tile", m_last_signal.tile);
		dmp.WriteEnumT("m_last_signal.td", m_last_signal.td);
		dmp.WriteEnumT("m_end_segment_reason", m_end_segment_reason);
	}
};

/** Yapf Node for rail YAPF */
struct CYapfRailNodeTrackDir
	: CYapfNodeT<CYapfRailKey, CYapfRailNodeTrackDir>
{
	typedef CYapfNodeT<CYapfRailKey, CYapfRailNodeTrackDir> base;
	typedef CYapfRailSegment CachedData;

	enum {
		FLAG_CHOICE_SEEN,         ///< node starts at a junction
		FLAG_TARGET_SEEN,         ///< node ends at a target
		FLAG_LAST_SIGNAL_WAS_RED, ///< last signal in node was red
		NFLAGS
	};

	CYapfRailSegment *m_segment;
	uint16            m_num_signals_passed;
	std::bitset<NFLAGS> flags;
	SignalType        m_last_signal_type;

	inline void Set(CYapfRailNodeTrackDir *parent, const RailPathPos &pos, bool is_choice)
	{
		base::Set(parent, pos);
		m_segment = NULL;
		if (parent == NULL) {
			m_num_signals_passed      = 0;
			flags                     = 0;
			/* We use PBS as initial signal type because if we are in
			 * a PBS section and need to route, i.e. we're at a safe
			 * waiting point of a station, we need to account for the
			 * reservation costs. If we are in a normal block then we
			 * should be alone in there and as such the reservation
			 * costs should be 0 anyway. If there would be another
			 * train in the block, i.e. passing signals at danger
			 * then avoiding that train with help of the reservation
			 * costs is not a bad thing, actually it would probably
			 * be a good thing to do. */
			m_last_signal_type        = SIGTYPE_PBS;
		} else {
			m_num_signals_passed      = parent->m_num_signals_passed;
			flags                     = parent->flags;
			m_last_signal_type        = parent->m_last_signal_type;
		}
		flags.set (FLAG_CHOICE_SEEN, is_choice);
	}

	inline const RailPathPos& GetLastPos() const
	{
		assert(m_segment != NULL);
		return m_segment->m_last;
	}

	void Dump(DumpTarget &dmp) const
	{
		base::Dump(dmp);
		dmp.WriteStructT("m_segment", m_segment);
		dmp.WriteLine("m_num_signals_passed = %d", m_num_signals_passed);
		dmp.WriteLine("m_target_seen = %s", flags.test(FLAG_TARGET_SEEN) ? "Yes" : "No");
		dmp.WriteLine("m_choice_seen = %s", flags.test(FLAG_CHOICE_SEEN) ? "Yes" : "No");
		dmp.WriteLine("m_last_signal_was_red = %s", flags.test(FLAG_LAST_SIGNAL_WAS_RED) ? "Yes" : "No");
		dmp.WriteEnumT("m_last_signal_type", m_last_signal_type);
	}
};

/* Default Astar type */
typedef Astar<CYapfRailNodeTrackDir, 8, 10> AstarRailTrackDir;


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
 *  Look at CYapfRailSegment for the segment example
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

	/** find a key in the cache */
	inline Tsegment *Find (const Key &key)
	{
		return m_map.Find (key);
	}

	/** create an entry in the cache for a node */
	template <class NodeKey>
	inline Tsegment *Create (const NodeKey &key)
	{
		assert (Find(key) == NULL);
		Tsegment *item = new (m_heap.Append()) Tsegment(key);
		m_map.Push (*item);
		return item;
	}
};


template <class TAstar>
class CYapfRailBaseT : public TAstar
{
public:
	typedef typename TAstar::Node Node;           ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables
	typedef typename Node::CachedData CachedData;
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
	Cache&        m_global_cache;
	LocalCache    m_local_cache;

	/**
	 * @note maximum cost doesn't work with caching enabled
	 * @todo fix maximum cost failing with caching (e.g. FS#2900)
	 */
	int                  m_max_cost;
	std::vector<int>     m_sig_look_ahead_costs;

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
		, m_global_cache(stGetGlobalCache())
		, m_max_cost(0)
		, m_sig_look_ahead_costs(m_settings->rail_look_ahead_max_signals)
		, m_stats_cost_calcs(0)
		, m_stats_cache_hits(0)
		, tf (v, allow_90deg, mask_reserved_tracks ? m_compatible_railtypes : v->compatible_railtypes)
		, tf_local (v, allow_90deg, m_compatible_railtypes, &m_perf_ts_cost)
	{
		/* pre-compute look-ahead penalties into array */
		int p0 = m_settings->rail_look_ahead_signal_p0;
		int p1 = m_settings->rail_look_ahead_signal_p1;
		int p2 = m_settings->rail_look_ahead_signal_p2;
		for (uint i = 0; i < m_settings->rail_look_ahead_max_signals; i++) {
			m_sig_look_ahead_costs[i] = p0 + i * (p1 + i * p2);
		}
	}

public:
	inline bool Allow90degTurns (void) const
	{
		return tf.m_allow_90deg;
	}

	inline bool FindCachedSegment (Node *n)
	{
		CachedData *segment = m_global_cache.Find (n->GetKey());
		if (segment == NULL) return false;
		n->m_segment = segment;
		return true;
	}

	/** Check if a node (usually under construction) is being cached. */
	inline bool IsNodeCached (const Node *n) const
	{
		return n->m_segment == m_global_cache.Find (n->GetKey());
	}

	inline void AttachCachedSegment (Node *n)
	{
		n->m_segment = m_global_cache.Create (n->GetKey());
		assert (IsNodeCached(n));
	}

	inline void AttachLocalSegment (Node *n)
	{
		n->m_segment = new (m_local_cache.Append()) CachedData(n->GetKey());
	}

	/** Create and add a new node */
	inline void AddStartupNode (const RailPathPos &pos, int cost = 0)
	{
		Node *node = TAstar::CreateNewNode (NULL, pos, false);
		node->m_cost = cost;
		/* initial nodes can never be used from the cache */
		AttachLocalSegment(node);
		TAstar::InsertInitialNode(node);
	}

	/** set origin */
	void SetOrigin(const RailPathPos &pos)
	{
		AddStartupNode (pos);
	}

	/** set origin */
	void SetOrigin(const RailPathPos &pos, const RailPathPos &rev, int reverse_penalty, bool treat_first_red_two_way_signal_as_eol)
	{
		m_treat_first_red_two_way_signal_as_eol = treat_first_red_two_way_signal_as_eol;

		AddStartupNode (pos);
		AddStartupNode (rev, reverse_penalty);
	}

	inline void SetMaxCost (int max_cost)
	{
		m_max_cost = max_cost;
	}

	/** Return the transition cost from one tile to another. */
	inline int TransitionCost (const RailPathPos &pos1, const RailPathPos &pos2) const
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
	inline int OneTileCost (const RailPathPos &pos) const
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
	inline int SlopeCost (const RailPathPos &pos)
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
	static inline bool IsAnyStationTileReserved (const RailPathPos &pos, int skipped)
	{
		TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
		for (TileIndex tile = pos.tile; skipped >= 0; skipped--, tile += diff) {
			if (HasStationReservation(tile)) return true;
		}
		return false;
	}

	/** The cost for reserved tiles, including skipped ones. */
	inline int ReservationCost(Node& n, const RailPathPos &pos, int skipped) const
	{
		if (n.m_num_signals_passed >= m_sig_look_ahead_costs.size() / 2) return 0;
		if (!IsPbsSignal(n.m_last_signal_type)) return 0;

		assert (!IsNodeCached(&n));

		if (!pos.in_wormhole() && IsRailStationTile(pos.tile) && IsAnyStationTileReserved(pos, skipped)) {
			return m_settings->rail_pbs_station_penalty * (skipped + 1);
		} else if (TrackOverlapsTracks(GetReservedTrackbits(pos.tile), TrackdirToTrack(pos.td))) {
			int cost = m_settings->rail_pbs_cross_penalty;
			if (!IsDiagonalTrackdir(pos.td)) cost = (cost * YAPF_TILE_CORNER_LENGTH) / YAPF_TILE_LENGTH;
			return cost * (skipped + 1);
		}
		return 0;
	}

	/* Compute cost and modify node state for a signal. */
	inline int SignalCost(Node *n, const RailPathPos &pos);

	/* Compute cost and modify node state for a position. */
	inline void HandleNodeTile (Node *n, const CFollowTrackRail *tf, TileIndex prev);

	/* Check for possible reasons to end a segment at the next tile. */
	inline void HandleNodeNextTile (Node *n, CFollowTrackRail *tf, RailType rail_type);

	/** Compute all costs for a newly-allocated (not cached) segment. */
	inline void CalcSegment (Node *n, const CFollowTrackRail *tf);

	/* Compute all costs for a segment or retrieve it from cache. */
	inline void CalcNode (Node *n);

	/* Set target flag on node, and add last signal costs. */
	inline void SetTarget (Node *n);

	/** Add special extra cost when the segment reaches our target.
	 * (dummy version for derived classes that do not need it). */
	static inline void AddTargetCost (Node *n) { }

	/** Struct to store a position in a path (node and path position). */
	struct NodePos {
		RailPathPos pos;  ///< position (tile and trackdir)
		Node       *node; ///< node where the position is
	};

	/* Find the earliest safe position on a path. */
	Node *FindSafePositionOnPath (Node *node, NodePos *res);

	/* Try to reserve the path up to a given position. */
	bool TryReservePath (TileIndex origin, const NodePos *res);
};

/** Compute cost and modify node state for a signal. */
template <class TAstar>
inline int CYapfRailBaseT<TAstar>::SignalCost(Node *n, const RailPathPos &pos)
{
	int cost = 0;
	/* if there is one-way signal in the opposite direction, then it is not our way */
	CPerfStart perf_cost(m_perf_other_cost);

	if (pos.has_signal_along()) {
		SignalState sig_state = pos.get_signal_state();
		SignalType sig_type = pos.get_signal_type();

		n->m_last_signal_type = sig_type;

		/* cache the look-ahead polynomial constant only if we didn't pass more signals than the look-ahead limit is */
		int look_ahead_cost = (n->m_num_signals_passed < m_sig_look_ahead_costs.size()) ? m_sig_look_ahead_costs[n->m_num_signals_passed] : 0;
		if (sig_state != SIGNAL_STATE_RED) {
			/* green signal */
			n->flags.reset (n->FLAG_LAST_SIGNAL_WAS_RED);
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
					&& n->flags.test(n->FLAG_CHOICE_SEEN)
					&& pos.has_signal_against()
					&& n->m_num_signals_passed == 0) {
				/* yes, the first signal is two-way red signal => DEAD END */
				n->m_segment->m_end_segment_reason.set(ESR_DEAD_END);
				m_stopped_on_first_two_way_signal = true;
				/* prune this branch, so that we will not follow a best
				 * intermediate node that heads straight into this one */
				bool found_intermediate = false;
				for (n = n->m_parent; n != NULL && !n->m_segment->m_end_segment_reason.test(ESR_CHOICE); n = n->m_parent) {
					if (n == TAstar::best_intermediate) found_intermediate = true;
				}
				if (found_intermediate) TAstar::best_intermediate = n;
				return -1;
			}
			n->flags.set (n->FLAG_LAST_SIGNAL_WAS_RED);

			/* look-ahead signal penalty */
			if (!IsPbsSignal(sig_type) && look_ahead_cost > 0) {
				/* add the look ahead penalty only if it is positive */
				cost += look_ahead_cost;
			}

			/* special signal penalties */
			if (n->m_num_signals_passed == 0) {
				switch (sig_type) {
					case SIGTYPE_COMBO:
					case SIGTYPE_EXIT:   cost += m_settings->rail_firstred_exit_penalty; break; // first signal is red pre-signal-exit
					case SIGTYPE_NORMAL:
					case SIGTYPE_ENTRY:  cost += m_settings->rail_firstred_penalty; break;
					default: break;
				}
			}
		}

		n->m_num_signals_passed++;
		n->m_segment->m_last_signal = pos;
	} else if (pos.has_signal_against()) {
		if (pos.get_signal_type() != SIGTYPE_PBS) {
			/* one-way signal in opposite direction */
			n->m_segment->m_end_segment_reason.set(ESR_DEAD_END);
		} else {
			cost += n->m_num_signals_passed < m_settings->rail_look_ahead_max_signals ? m_settings->rail_pbs_signal_back_penalty : 0;
		}
	}

	assert ((cost == 0) || !IsNodeCached(n));
	return cost;
}

/** Compute cost and modify node state for a position. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::HandleNodeTile (Node *n, const CFollowTrackRail *tf, TileIndex prev)
{
	const RailPathPos &pos = n->m_segment->m_last;
	int cost = 0;

	/* All other tile costs will be calculated here. */
	cost += OneTileCost(pos);

	/* If we skipped some tunnel/bridge/station tiles, add their base cost */
	cost += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

	/* Slope cost. */
	cost += SlopeCost(pos);

	/* Signal cost (routine can modify segment data). */
	cost += SignalCost(n, pos);

	/* Reserved tiles. */
	cost += ReservationCost(*n, pos, tf->m_tiles_skipped);

	/* Tests for 'potential target' reasons to close the segment. */
	if (pos.tile == prev) {
		/* Penalty for reversing in a depot. */
		assert(!pos.in_wormhole());
		assert(IsRailDepotTile(pos.tile));
		assert(pos.td == DiagDirToDiagTrackdir(GetGroundDepotDirection(pos.tile)));
		cost += m_settings->rail_depot_reverse_penalty;
		/* We will end in this pass (depot is possible target) */
		n->m_segment->m_end_segment_reason.set(ESR_DEPOT);

	} else if (!pos.in_wormhole() && IsRailWaypointTile(pos.tile)) {
		/* Waypoint is also a good reason to finish. */
		n->m_segment->m_end_segment_reason.set(ESR_WAYPOINT);

	} else if (tf->m_flag == tf->TF_STATION) {
		/* Station penalties. */
		uint platform_length = tf->m_tiles_skipped + 1;
		/* We don't know yet if the station is our target or not. Act like
		 * if it is pass-through station (not our destination). */
		cost += m_settings->rail_station_penalty * platform_length;
		/* We will end in this pass (station is possible target) */
		n->m_segment->m_end_segment_reason.set(ESR_STATION);

	} else if (mask_reserved_tracks) {
		assert (!IsNodeCached(n));
		/* Searching for a safe tile? */
		if (pos.has_signal_along() && !IsPbsSignal(pos.get_signal_type())) {
			n->m_segment->m_end_segment_reason.set(ESR_SAFE_TILE);
		}
	}

	/* Apply max speed penalty only when inside the look-ahead radius.
	 * Otherwise it would cause desync in MP. */
	if (n->m_num_signals_passed < m_sig_look_ahead_costs.size())
	{
		assert (!IsNodeCached(n));
		int max_speed = tf->GetSpeedLimit();
		int max_veh_speed = m_veh->GetDisplayMaxSpeed();
		if (max_speed < max_veh_speed) {
			cost += YAPF_TILE_LENGTH * (max_veh_speed - max_speed) * (1 + tf->m_tiles_skipped) / max_veh_speed;
		}
	}

	n->m_segment->m_cost += cost;
}

/** Check for possible reasons to end a segment at the next tile. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::HandleNodeNextTile (Node *n, CFollowTrackRail *tf, RailType rail_type)
{
	if (!tf->Follow(n->m_segment->m_last)) {
		assert(tf->m_err != CFollowTrackRail::EC_NONE);
		/* Can't move to the next tile (EOL?). */
		if (tf->m_err == CFollowTrackRail::EC_RAIL_TYPE) {
			n->m_segment->m_end_segment_reason.set(ESR_RAIL_TYPE);
		} else {
			n->m_segment->m_end_segment_reason.set(ESR_DEAD_END);
		}

		if (mask_reserved_tracks && !HasOnewaySignalBlockingPos(tf->m_old)) {
			n->m_segment->m_end_segment_reason.set(ESR_SAFE_TILE);
		}
		return;
	}

	/* Check if the next tile is not a choice. */
	if (!tf->m_new.is_single()) {
		/* More than one segment will follow. Close this one. */
		n->m_segment->m_end_segment_reason.set(ESR_CHOICE);
		return;
	}

	/* Gather the next tile/trackdir. */

	if (mask_reserved_tracks) {
		assert (!IsNodeCached(n));
		if (tf->m_new.has_signal_along() && IsPbsSignal(tf->m_new.get_signal_type())) {
			/* Possible safe tile. */
			n->m_segment->m_end_segment_reason.set(ESR_SAFE_TILE);
		} else if (tf->m_new.has_signal_against() && tf->m_new.get_signal_type() == SIGTYPE_PBS_ONEWAY) {
			/* Possible safe tile, but not so good as it's the back of a signal... */
			n->m_segment->m_end_segment_reason.set(ESR_SAFE_TILE);
			n->m_segment->m_end_segment_reason.set(ESR_DEAD_END);
			n->m_segment->m_cost += m_settings->rail_lastred_exit_penalty;
		}
	}

	/* Check the next tile for the rail type. */
	if (tf->m_new.in_wormhole()) {
		RailType next_rail_type = tf->m_new.get_railtype();
		assert(next_rail_type == rail_type);
	} else if (tf->m_new.get_railtype() != rail_type) {
		/* Segment must consist from the same rail_type tiles. */
		n->m_segment->m_end_segment_reason.set(ESR_RAIL_TYPE);
		return;
	}

	/* Avoid infinite looping. */
	if (tf->m_new == n->GetPos()) {
		n->m_segment->m_end_segment_reason.set(ESR_LOOP);
	} else if (n->m_segment->m_cost > s_max_segment_cost) {
		/* Potentially in the infinite loop (or only very long segment?). We should
		 * not force it to finish prematurely unless we are on a regular tile. */
		if (!tf->m_new.in_wormhole() && IsNormalRailTile(tf->m_new.tile)) {
			n->m_segment->m_end_segment_reason.set(ESR_TOO_LONG);
		}
	}
}

/** Compute all costs for a newly-allocated (not cached) segment. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::CalcSegment (Node *n, const CFollowTrackRail *tf)
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

	assert (n->m_segment->m_last == n->GetPos());
	assert (n->m_segment->m_cost == 0);
	assert (!n->m_segment->m_last_signal.is_valid_tile());
	assert (n->m_segment->m_end_segment_reason.none());

	/* start at n and walk to the end of segment */
	int entry_cost = n->m_parent->m_cost + TransitionCost (n->m_parent->GetLastPos(), n->GetPos());

	TileIndex prev = n->m_parent->GetLastPos().tile;

	RailType rail_type = n->GetPos().get_railtype();

	for (;;) {
		HandleNodeTile (n, tf, prev);

		/* Move to the next tile/trackdir. */
		tf = &tf_local;
		assert(tf_local.m_veh_owner == m_veh->owner);
		assert(tf_local.m_railtypes == m_compatible_railtypes);
		assert(tf_local.m_pPerf == &m_perf_ts_cost);

		HandleNodeNextTile (n, &tf_local, rail_type);
		assert(tf_local.m_old == n->m_segment->m_last);

		/* Finish if we already exceeded the maximum path cost
		 * (i.e. when searching for the nearest depot). */
		if (m_max_cost != 0) {
			/* we shouldn't be caching a new segment with a maximum cost */
			assert (!IsNodeCached(n));
			if ((entry_cost + n->m_segment->m_cost) > m_max_cost) {
				n->m_segment->m_end_segment_reason.set(ESR_MAX_COST);
				break;
			}
		}

		/* Any other reason to end the segment? */
		if (n->m_segment->m_end_segment_reason.any()) break;

		/* Transition cost (cost of the move from previous tile) */
		n->m_segment->m_cost += TransitionCost (tf_local.m_old, tf_local.m_new);

		/* For the next loop set new prev and cur tile info. */
		prev = tf_local.m_old.tile;
		n->m_segment->m_last = tf_local.m_new;
	} // for (;;)

	/* total node cost */
	n->m_cost = entry_cost + n->m_segment->m_cost;
}

/** Compute all costs for a segment or retrieve it from cache. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::CalcNode (Node *n)
{
	/* Disable the cache if the node is within the signal lookahead
	 * threshold; if we are masking reserved tracks (because that makes
	 * segments end prematurely at the first signal); or if we have a
	 * maximum cost (because that may also make a segment end prematurely
	 * if the maximum cost is reached when computing its cost). */
	if (m_max_cost == 0 && !mask_reserved_tracks
			&& (n->m_parent->m_num_signals_passed >= m_sig_look_ahead_costs.size())) {
		/* look for the segment in the cache */
		if (FindCachedSegment(n)) {
			m_stats_cache_hits++;
			assert (!n->m_segment->m_end_segment_reason.test(ESR_MAX_COST));
			/* total node cost */
			n->m_cost = n->m_parent->m_cost + TransitionCost (n->m_parent->GetLastPos(), n->GetPos()) + n->m_segment->m_cost;
			/* We will need also some information about the last signal (if it was red). */
			if (n->m_segment->m_last_signal.is_valid_tile()) {
				assert(n->m_segment->m_last_signal.has_signal_along());
				SignalState sig_state = n->m_segment->m_last_signal.get_signal_state();
				n->flags.set (n->FLAG_LAST_SIGNAL_WAS_RED, sig_state == SIGNAL_STATE_RED);
				n->m_last_signal_type = n->m_segment->m_last_signal.get_signal_type();
			}
			/* No further calculation needed. */
			if (DEBUG_YAPF_CACHE) {
				Node test (*n);
				CachedData segment (test.GetKey());
				test.m_segment = &segment;
				CalcSegment (&test, &this->tf);
				/* m_num_signals_passed can differ when cached */
				if (!(*n->m_segment == *test.m_segment) || n->flags != test.flags || n->m_last_signal_type != test.m_last_signal_type) {
					DumpTarget dmp ("yapf.txt");
					dmp.WriteLine ("num_steps = %d", TAstar::num_steps);
					dmp.WriteStructT ("array", TAstar::GetArray());
					dmp.WriteStructT ("test_node", &test);
					NOT_REACHED();
				}
			}
			return;
		}

		/* segment not found, but we can cache it for next time */
		AttachCachedSegment(n);
	} else {
		/* segment not cacheable */
		AttachLocalSegment(n);
	}

	m_stats_cost_calcs++;
	CalcSegment (n, &this->tf);
}

/** Set target flag on node, and add last signal costs. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::SetTarget (Node *n)
{
	n->flags.set (n->FLAG_TARGET_SEEN);

	/* Last-red and last-red-exit penalties. */
	if (n->flags.test (n->FLAG_LAST_SIGNAL_WAS_RED)) {
		if (n->m_last_signal_type == SIGTYPE_EXIT) {
			/* last signal was red pre-signal-exit */
			n->m_cost += m_settings->rail_lastred_exit_penalty;
		} else if (!IsPbsSignal(n->m_last_signal_type)) {
			/* Last signal was red, but not exit or path signal. */
			n->m_cost += m_settings->rail_lastred_penalty;
		}
	}
}

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
static bool ReserveSingleTrack (const RailPathPos &pos, TileIndex origin = INVALID_TILE)
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
static void UnreserveSingleTrack (const RailPathPos &pos, TileIndex origin = INVALID_TILE)
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
				RailPathPos res_fail = ft.m_new;
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

	return true;
}


template <class TAstar>
struct CYapfRailOrderT : CYapfRailBaseT <TAstar>
{
public:
	typedef CYapfRailBaseT <TAstar> Base;
	typedef typename TAstar::Node   Node;

private:
	TileIndex m_dest_tile;
	StationID m_dest_station_id;

public:
	CYapfRailOrderT (const Train *v, bool allow_90deg)
		: Base (v, allow_90deg, false, false)
	{
		switch (v->current_order.GetType()) {
			case OT_GOTO_WAYPOINT:
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

	/** Check if a position is a destination. */
	inline bool IsDestination (const RailPathPos &pos) const
	{
		if (m_dest_station_id != INVALID_STATION) {
			return !pos.in_wormhole() && HasStationTileRail(pos.tile)
				&& (GetStationIndex(pos.tile) == m_dest_station_id)
				&& (GetRailStationTrack(pos.tile) == TrackdirToTrack(pos.td));
		} else {
			return !pos.in_wormhole() && (pos.tile == m_dest_tile);
		}
	}

	/** Estimate the cost from a node to the destination. */
	inline void CalcEstimate (Node *n) const
	{
		n->m_estimate = n->m_cost + YapfCalcEstimate (n->GetLastPos(), m_dest_tile);
		assert (n->m_estimate >= n->m_parent->m_estimate);
	}

	/** Add special extra cost when the segment reaches our target. */
	inline void AddTargetCost (Node *n)
	{
		const Train *v = Base::m_veh;
		/* Station platform-length penalty. */
		if (v->current_order.GetType() == OT_GOTO_STATION) {
			const RailPathPos &pos = n->GetLastPos();
			assert (GetStationIndex(pos.tile) == m_dest_station_id);
			const BaseStation *st = BaseStation::Get(m_dest_station_id);
			assert(st != NULL);
			uint platform_length = st->GetPlatformLength(pos.tile, ReverseDiagDir(TrackdirToExitdir(pos.td)));
			/* Reduce the extra cost caused by passing-station penalty (each station receives it in the segment cost). */
			n->m_cost -= Base::m_settings->rail_station_penalty * platform_length;
			/* Add penalty for the inappropriate platform length. */
			assert (v->gcache.cached_total_length != 0);
			int missing_platform_length = CeilDiv(v->gcache.cached_total_length, TILE_SIZE) - platform_length;
			if (missing_platform_length < 0) {
				/* apply penalty for longer platform than needed */
				n->m_cost += Base::m_settings->rail_longer_platform_penalty + Base::m_settings->rail_longer_platform_per_tile_penalty * -missing_platform_length;
			} else if (missing_platform_length > 0) {
				/* apply penalty for shorter platform than needed */
				n->m_cost += Base::m_settings->rail_shorter_platform_penalty + Base::m_settings->rail_shorter_platform_per_tile_penalty * missing_platform_length;
			}
		} else if (v->current_order.IsType(OT_GOTO_WAYPOINT)) {
			const RailPathPos &pos = n->GetLastPos();
			assert (GetStationIndex(pos.tile) == m_dest_station_id);
			if (!Waypoint::Get(m_dest_station_id)->IsSingleTile()) {
				/* This waypoint is our destination; maybe this isn't an unreserved
				 * one, so check that and if so see that as the last signal being
				 * red. This way waypoints near stations should work better. */
				CFollowTrackRail ft(v);
				ft.SetPos(pos);

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
				if (add_extra_cost) n->m_cost += Base::m_settings->rail_lastred_penalty;
			}
		}
	}
};

template <class TAstar>
struct CYapfAnyDepotRailT : CYapfRailBaseT <TAstar>
{
	typedef CYapfRailBaseT <TAstar> Base;
	typedef typename TAstar::Node   Node;

	CYapfAnyDepotRailT (const Train *v, bool allow_90deg)
		: Base (v, allow_90deg, false, false)
	{
	}

	/** Check if a position is a destination. */
	inline bool IsDestination (const RailPathPos &pos) const
	{
		return !pos.in_wormhole() && IsRailDepotTile(pos.tile);
	}

	/** Estimate the cost from a node to the destination. */
	inline void CalcEstimate (Node *n) const
	{
		n->m_estimate = n->m_cost;
	}
};

template <class TAstar>
struct CYapfAnySafeTileRailT : CYapfRailBaseT <TAstar>
{
	typedef CYapfRailBaseT <TAstar> Base;
	typedef typename TAstar::Node   Node;

	CYapfAnySafeTileRailT (const Train *v, bool allow_90deg, bool override_railtype)
		: Base (v, allow_90deg, override_railtype, true)
	{
	}

	/** Check if a position is a destination. */
	inline bool IsDestination (const RailPathPos &pos) const
	{
		return IsFreeSafeWaitingPosition (Base::m_veh, pos, Base::Allow90degTurns());
	}

	/** Estimate the cost from a node to the destination. */
	inline void CalcEstimate (Node *n) const
	{
		n->m_estimate = n->m_cost;
	}
};


template <class Base>
struct CYapfRailT : public Base
{
	typedef typename Base::Node Node; ///< this will be our node type

	CYapfRailT (const Train *v, bool allow_90deg)
		: Base (v, allow_90deg)
	{
	}

	CYapfRailT (const Train *v, bool allow_90deg, bool override_rail_type)
		: Base (v, allow_90deg, override_rail_type)
	{
	}

	/** Called by the A-star underlying class to find the neighbours of a node. */
	inline void Follow (Node *old_node)
	{
		if (!Base::tf.Follow(old_node->GetLastPos())) return;
		if (Base::mask_reserved_tracks && !Base::tf.MaskReservedTracks()) return;

		bool is_choice = !Base::tf.m_new.is_single();
		RailPathPos pos = Base::tf.m_new;
		for (TrackdirBits rtds = Base::tf.m_new.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.set_trackdir (FindFirstTrackdir(rtds));
			Node *n = Base::CreateNewNode(old_node, pos, is_choice);

			/* evaluate the node */
			CPerfStart perf_cost(Base::m_perf_cost);

			Base::CalcNode(n);

			uint end_reason = n->m_segment->m_end_segment_reason.to_ulong();
			assert (end_reason != 0);

			if (((end_reason & ESRB_POSSIBLE_TARGET) != 0) &&
					Base::IsDestination(n->GetLastPos())) {
				Base::SetTarget (n);
				/* Special costs for the case we have reached our target. */
				Base::AddTargetCost (n);
				perf_cost.Stop();
				n->m_estimate = n->m_cost;
				this->FoundTarget(n);

			} else if ((end_reason & ESRB_ABORT_PF_MASK) != 0) {
				/* Reason to not continue. Stop this PF branch. */
				continue;

			} else {
				perf_cost.Stop();
				Base::CalcEstimate(n);
				this->InsertNode(n);
			}
		}
	}

	/** call the node follower */
	static inline void Follow (CYapfRailT *pf, Node *n)
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

		bool bDestFound = Base::FindPath (Follow, Base::m_settings->max_search_nodes);

#ifndef NO_DEBUG_MESSAGES
		perf.Stop();
		if (_debug_yapf_level >= 2) {
			int t = perf.Get(1000000);
			_total_pf_time_us += t;

			if (_debug_yapf_level >= 3) {
				UnitID veh_idx = (Base::m_veh != NULL) ? Base::m_veh->unitnumber : 0;
				float cache_hit_ratio = (Base::m_stats_cache_hits == 0) ? 0.0f : ((float)Base::m_stats_cache_hits / (float)(Base::m_stats_cache_hits + Base::m_stats_cost_calcs) * 100.0f);
				int cost = bDestFound ? Base::best->m_cost : -1;
				int dist = bDestFound ? Base::best->m_estimate - Base::best->m_cost : -1;

				DEBUG(yapf, 3, "[YAPFt]%c%4d- %d us - %d rounds - %d open - %d closed - CHR %4.1f%% - C %d D %d - c%d(sc%d, ts%d, o%d) -- ",
					bDestFound ? '-' : '!', veh_idx, t, Base::num_steps, Base::OpenCount(), Base::ClosedCount(),
					cache_hit_ratio, cost, dist, Base::m_perf_cost.Get(1000000), Base::m_perf_slope_cost.Get(1000000),
					Base::m_perf_ts_cost.Get(1000000), Base::m_perf_other_cost.Get(1000000)
				);
			}
		}
#endif /* !NO_DEBUG_MESSAGES */
		return bDestFound;
	}
};


typedef CYapfRailT <CYapfRailOrderT <AstarRailTrackDir> > CYapfRail;

Trackdir YapfTrainChooseTrack(const Train *v, const RailPathPos &origin, bool reserve_track, PFResult *target)
{
	if (target != NULL) target->pos.tile = INVALID_TILE;

	/* create pathfinder instance */
	CYapfRail pf (v, !_settings_game.pf.forbid_90_deg);

	/* set origin node */
	pf.SetOrigin(origin);

	/* find the best path */
	bool path_found = pf.FindPath();

	/* if path not found - return INVALID_TRACKDIR */
	Trackdir next_trackdir = INVALID_TRACKDIR;
	CYapfRail::Node *node = pf.GetBestNode();
	if (node != NULL) {
		if (reserve_track && path_found) {
			CYapfRail::NodePos res;
			CYapfRail::Node *best_next_node = pf.FindSafePositionOnPath (node, &res);
			if (target != NULL) target->pos = res.pos;
			/* return trackdir from the best origin node (one of start nodes) */
			next_trackdir = best_next_node->GetPos().td;

			assert (best_next_node->m_parent->GetPos() == origin);
			assert (best_next_node->m_parent->GetLastPos() == origin);
			bool okay = pf.TryReservePath (origin.tile, &res);
			if (target != NULL) target->okay = okay;
		} else {
			while (node->m_parent->m_parent != NULL) {
				node = node->m_parent;
			}
			assert (node->m_parent->GetPos() == origin);
			assert (node->m_parent->GetLastPos() == origin);
			/* return trackdir from the best origin node (one of start nodes) */
			next_trackdir = node->GetPos().td;
		}
	}

	/* Treat the path as found if stopped on the first two way signal(s). */
	if (target != NULL) target->found = path_found | pf.m_stopped_on_first_two_way_signal;
	return next_trackdir;
}

bool YapfTrainCheckReverse(const Train *v)
{
	const Train *last_veh = v->Last();

	/* tiles where front and back are */
	RailPathPos pos = v->GetPos();
	RailPathPos rev = last_veh->GetReversePos();

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

	CYapfRail pf (v, !_settings_game.pf.forbid_90_deg);

	/* set origin nodes */
	pf.SetOrigin (pos, rev, reverse_penalty, false);

	/* find the best path */
	if (!pf.FindPath()) return false;

	/* path found; walk through the path back to the origin */
	CYapfRail::Node *node = pf.GetBestNode();
	while (node->m_parent != NULL) {
		node = node->m_parent;
	}

	/* check if it was reversed origin */
	return node->m_cost != 0;
}


typedef CYapfRailT <CYapfAnyDepotRailT <AstarRailTrackDir> > CYapfAnyDepotRail;

bool YapfTrainFindNearestDepot(const Train *v, uint max_penalty, FindDepotData *res)
{
	RailPathPos origin;
	FollowTrainReservation(v, &origin);
	RailPathPos rev = v->Last()->GetReversePos();

	CYapfAnyDepotRail pf (v, !_settings_game.pf.forbid_90_deg);

	/* set origin node */
	pf.SetOrigin (origin, rev, YAPF_INFINITE_PENALTY, true);
	pf.SetMaxCost(max_penalty);

	/* find the best path */
	if (!pf.FindPath()) return false;

	/* path found; get found target tile */
	CYapfAnyDepotRail::Node *n = pf.GetBestNode();
	res->tile = n->GetLastPos().tile;

	/* walk through the path back to the origin */
	while (n->m_parent != NULL) {
		n = n->m_parent;
	}

	/* if the origin node is our front vehicle tile/Trackdir then we didn't reverse
	 * but we can also look at the cost (== 0 -> not reversed, == reverse_penalty -> reversed) */
	res->reverse = (n->m_cost != 0);

	return true;
}


typedef CYapfRailT <CYapfAnySafeTileRailT <AstarRailTrackDir> > CYapfAnySafeTileRail;

bool YapfTrainFindNearestSafeTile(const Train *v, const RailPathPos &pos, bool override_railtype)
{
	/* Create pathfinder instance */
	CYapfAnySafeTileRail pf (v, !_settings_game.pf.forbid_90_deg, override_railtype);

	/* Set origin. */
	pf.SetOrigin(pos);

	if (!pf.FindPath()) return false;

	/* Found a destination, search for a reservation target. */
	CYapfAnySafeTileRail::Node *node = pf.GetBestNode();
	CYapfAnySafeTileRail::NodePos res;
	node = pf.FindSafePositionOnPath(node, &res)->m_parent;
	assert (node->GetPos() == pos);
	assert (node->GetLastPos() == pos);

	return pf.TryReservePath (pos.tile, &res);
}
