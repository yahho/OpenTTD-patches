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

template <typename Tpf> void DumpState(Tpf &pf1, Tpf &pf2)
{
	DumpTarget dmp1 ("yapf1.txt");
	dmp1.WriteStructT("m_arr", pf1.GetArray());
	dmp1.WriteLine("m_num_steps = %d", pf1.num_steps);

	DumpTarget dmp2 ("yapf2.txt");
	dmp2.WriteStructT("m_arr", pf2.GetArray());
	dmp2.WriteLine("m_num_steps = %d", pf2.num_steps);
}

int _total_pf_time_us = 0;


/* Enum used in PfCalcCost() to see why was the segment closed. */
enum EndSegmentReason {
	ESR_DEAD_END = 0,      ///< track ends here
	ESR_RAIL_TYPE,         ///< the next tile has a different rail type than our tiles
	ESR_INFINITE_LOOP,     ///< infinite loop detected
	ESR_SEGMENT_TOO_LONG,  ///< the segment is too long (possible infinite loop)
	ESR_CHOICE_FOLLOWS,    ///< the next tile contains a choice (the track splits to more than one segments)
	ESR_DEPOT,             ///< stop in the depot (could be a target next time)
	ESR_WAYPOINT,          ///< waypoint encountered (could be a target next time)
	ESR_STATION,           ///< station encountered (could be a target next time)
	ESR_SAFE_TILE,         ///< safe waiting position found (could be a target)
};

enum EndSegmentReasonBits {
	ESRB_NONE = 0,

	ESRB_DEAD_END          = 1 << ESR_DEAD_END,
	ESRB_RAIL_TYPE         = 1 << ESR_RAIL_TYPE,
	ESRB_INFINITE_LOOP     = 1 << ESR_INFINITE_LOOP,
	ESRB_SEGMENT_TOO_LONG  = 1 << ESR_SEGMENT_TOO_LONG,
	ESRB_CHOICE_FOLLOWS    = 1 << ESR_CHOICE_FOLLOWS,
	ESRB_DEPOT             = 1 << ESR_DEPOT,
	ESRB_WAYPOINT          = 1 << ESR_WAYPOINT,
	ESRB_STATION           = 1 << ESR_STATION,
	ESRB_SAFE_TILE         = 1 << ESR_SAFE_TILE,

	/* Additional (composite) values. */

	/* What reasons mean that the target can be found and needs to be detected. */
	ESRB_POSSIBLE_TARGET = ESRB_DEPOT | ESRB_WAYPOINT | ESRB_STATION | ESRB_SAFE_TILE,

	/* Reasons to abort pathfinding in this direction. */
	ESRB_ABORT_PF_MASK = ESRB_DEAD_END | ESRB_INFINITE_LOOP,
};

DECLARE_ENUM_AS_BIT_SET(EndSegmentReasonBits)

inline void WriteValueStr(EndSegmentReasonBits bits, FILE *f)
{
	static const char * const end_segment_reason_names[] = {
		"DEAD_END", "RAIL_TYPE", "INFINITE_LOOP", "SEGMENT_TOO_LONG", "CHOICE_FOLLOWS",
		"DEPOT", "WAYPOINT", "STATION", "SAFE_TILE",
	};

	fprintf (f, "0x%04X (", bits);
	ComposeNameT (f, bits, end_segment_reason_names, "UNK", ESRB_NONE, "NONE");
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
		, m_end_segment_reason(ESRB_NONE)
	{}

	inline const Key& GetKey() const
	{
		return m_key;
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
		FLAG_TARGET_SEEN,
		FLAG_CHOICE_SEEN,
		FLAG_LAST_SIGNAL_WAS_RED,
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
	bool          m_disable_cache;
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
		, m_disable_cache(false)
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

	inline bool CanUseGlobalCache(Node& n) const
	{
		/* Disable the cache if explicitly asked to; if the node has
		 * no parent (initial node) or is within the signal lookahead
		 * threshold; or if we are masking reserved tracks (because
		 * that makes segments end prematurely at the first signal). */
		return !m_disable_cache && !mask_reserved_tracks
			&& (n.m_parent != NULL)
			&& (n.m_parent->m_num_signals_passed >= m_sig_look_ahead_costs.size());
	}

protected:
	/**
	 * Called by YAPF to attach cached or local segment cost data to the given node.
	 *  @return true if globally cached data were used or false if local data was used
	 */
	inline bool AttachSegmentToNode(Node *n)
	{
		const Key &key = n->GetKey();
		if (!CanUseGlobalCache(*n)) {
			n->m_segment = new (m_local_cache.Append()) CachedData(key);
			return false;
		}
		n->m_segment = m_global_cache.Find (key);
		if (n->m_segment != NULL) return true;
		n->m_segment = m_global_cache.Create (key);
		return false;
	}

public:
	/** Create and add a new node */
	inline void AddStartupNode (const RailPathPos &pos, int cost = 0)
	{
		Node *node = TAstar::CreateNewNode (NULL, pos, false);
		node->m_cost = cost;
		AttachSegmentToNode(node);
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

	inline void DisableCache(bool disable)
	{
		m_disable_cache = disable;
	}

	struct NodeData {
		int segment_cost;
		int extra_cost;
		RailPathPos pos;
		RailPathPos last_signal;
		EndSegmentReasonBits end_reason;
	};

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
	inline int SignalCost(Node *n, const RailPathPos &pos, NodeData *data);

	/* Compute cost and modify node state for a position. */
	inline void HandleNodeTile (Node *n, const CFollowTrackRail *tf, NodeData *segment, TileIndex prev);

	/* Check for possible reasons to end a segment at the next tile. */
	inline void HandleNodeNextTile (Node *n, CFollowTrackRail *tf, NodeData *segment, RailType rail_type);

	/** Compute all costs for a newly-allocated (not cached) segment. */
	inline bool CalcSegment (Node *n, const CFollowTrackRail *tf);

	/* Fill in a node from cached data. */
	inline void RestoreCachedNode (Node *n);

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
inline int CYapfRailBaseT<TAstar>::SignalCost(Node *n, const RailPathPos &pos, NodeData *data)
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
				data->end_reason |= ESRB_DEAD_END;
				m_stopped_on_first_two_way_signal = true;
				/* prune this branch, so that we will not follow a best
				 * intermediate node that heads straight into this one */
				bool found_intermediate = false;
				for (n = n->m_parent; n != NULL && (n->m_segment->m_end_segment_reason & ESRB_CHOICE_FOLLOWS) == 0; n = n->m_parent) {
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
		data->last_signal = pos;
	} else if (pos.has_signal_against()) {
		if (pos.get_signal_type() != SIGTYPE_PBS) {
			/* one-way signal in opposite direction */
			data->end_reason |= ESRB_DEAD_END;
		} else {
			cost += n->m_num_signals_passed < m_settings->rail_look_ahead_max_signals ? m_settings->rail_pbs_signal_back_penalty : 0;
		}
	}

	return cost;
}

/** Compute cost and modify node state for a position. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::HandleNodeTile (Node *n, const CFollowTrackRail *tf, NodeData *segment, TileIndex prev)
{
	/* All other tile costs will be calculated here. */
	segment->segment_cost += OneTileCost(segment->pos);

	/* If we skipped some tunnel/bridge/station tiles, add their base cost */
	segment->segment_cost += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

	/* Slope cost. */
	segment->segment_cost += SlopeCost(segment->pos);

	/* Signal cost (routine can modify segment data). */
	segment->segment_cost += SignalCost(n, segment->pos, segment);

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
		if (segment->pos.has_signal_along() && !IsPbsSignal(segment->pos.get_signal_type())) {
			segment->end_reason |= ESRB_SAFE_TILE;
		}
	}

	/* Apply max speed penalty only when inside the look-ahead radius.
	 * Otherwise it would cause desync in MP. */
	if (n->m_num_signals_passed < m_sig_look_ahead_costs.size())
	{
		int max_speed = tf->GetSpeedLimit();
		int max_veh_speed = m_veh->GetDisplayMaxSpeed();
		if (max_speed < max_veh_speed) {
			segment->extra_cost += YAPF_TILE_LENGTH * (max_veh_speed - max_speed) * (1 + tf->m_tiles_skipped) / max_veh_speed;
		}
	}
}

/** Check for possible reasons to end a segment at the next tile. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::HandleNodeNextTile (Node *n, CFollowTrackRail *tf, NodeData *segment, RailType rail_type)
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
		return;
	}

	/* Check if the next tile is not a choice. */
	if (!tf->m_new.is_single()) {
		/* More than one segment will follow. Close this one. */
		segment->end_reason |= ESRB_CHOICE_FOLLOWS;
		return;
	}

	/* Gather the next tile/trackdir. */

	if (mask_reserved_tracks) {
		if (tf->m_new.has_signal_along() && IsPbsSignal(tf->m_new.get_signal_type())) {
			/* Possible safe tile. */
			segment->end_reason |= ESRB_SAFE_TILE;
		} else if (tf->m_new.has_signal_against() && tf->m_new.get_signal_type() == SIGTYPE_PBS_ONEWAY) {
			/* Possible safe tile, but not so good as it's the back of a signal... */
			segment->end_reason |= ESRB_SAFE_TILE | ESRB_DEAD_END;
			segment->extra_cost += m_settings->rail_lastred_exit_penalty;
		}
	}

	/* Check the next tile for the rail type. */
	if (tf->m_new.in_wormhole()) {
		RailType next_rail_type = tf->m_new.get_railtype();
		assert(next_rail_type == rail_type);
	} else if (tf->m_new.get_railtype() != rail_type) {
		/* Segment must consist from the same rail_type tiles. */
		segment->end_reason |= ESRB_RAIL_TYPE;
		return;
	}

	/* Avoid infinite looping. */
	if (tf->m_new == n->GetPos()) {
		segment->end_reason |= ESRB_INFINITE_LOOP;
	} else if (segment->segment_cost > s_max_segment_cost) {
		/* Potentially in the infinite loop (or only very long segment?). We should
		 * not force it to finish prematurely unless we are on a regular tile. */
		if (!tf->m_new.in_wormhole() && IsNormalRailTile(tf->m_new.tile)) {
			segment->end_reason |= ESRB_SEGMENT_TOO_LONG;
		}
	}
}

/** Compute all costs for a newly-allocated (not cached) segment. */
template <class TAstar>
inline bool CYapfRailBaseT<TAstar>::CalcSegment (Node *n, const CFollowTrackRail *tf)
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
	int entry_cost = n->m_parent->m_cost + TransitionCost (n->m_parent->GetLastPos(), n->GetPos());
	NodeData segment;
	segment.segment_cost = 0;
	segment.extra_cost = 0;
	segment.pos = n->GetPos();
	segment.last_signal = RailPathPos();
	segment.end_reason = ESRB_NONE;

	TileIndex prev = n->m_parent->GetLastPos().tile;

	RailType rail_type = segment.pos.get_railtype();

	bool path_too_long;

	for (;;) {
		HandleNodeTile (n, tf, &segment, prev);

		/* Move to the next tile/trackdir. */
		tf = &tf_local;
		assert(tf_local.m_veh_owner == m_veh->owner);
		assert(tf_local.m_railtypes == m_compatible_railtypes);
		assert(tf_local.m_pPerf == &m_perf_ts_cost);

		HandleNodeNextTile (n, &tf_local, &segment, rail_type);

		/* Finish if we already exceeded the maximum path cost
		 * (i.e. when searching for the nearest depot). */
		path_too_long = m_max_cost > 0 && (entry_cost + segment.segment_cost) > m_max_cost;

		/* Any reason to end the segment? */
		if (path_too_long || (segment.end_reason != ESRB_NONE)) break;

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
	n->m_segment->m_end_segment_reason = segment.end_reason;

	/* total node cost */
	n->m_cost = entry_cost + segment.segment_cost + segment.extra_cost;
	return path_too_long;
}

/** Fill in a node from cached data. */
template <class TAstar>
inline void CYapfRailBaseT<TAstar>::RestoreCachedNode (Node *n)
{
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
			bool cached = Base::AttachSegmentToNode(n);

			CPerfStart perf_cost(Base::m_perf_cost);

			bool path_too_long;
			if (cached) {
				Base::m_stats_cache_hits++;
				assert (Base::m_max_cost == 0);
				Base::RestoreCachedNode (n);
				path_too_long = false;
			} else {
				Base::m_stats_cost_calcs++;
				path_too_long = Base::CalcSegment (n, &this->tf);
			}

			EndSegmentReasonBits end_reason = n->m_segment->m_end_segment_reason;
			assert (path_too_long || (end_reason != ESRB_NONE));

			if (((end_reason & ESRB_POSSIBLE_TARGET) != ESRB_NONE) &&
					Base::IsDestination(n->GetLastPos())) {
				Base::SetTarget (n);
				/* Special costs for the case we have reached our target. */
				Base::AddTargetCost (n);
				perf_cost.Stop();
				n->m_estimate = n->m_cost;
				this->FoundTarget(n);

			} else if (path_too_long || (end_reason & ESRB_ABORT_PF_MASK) != ESRB_NONE) {
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

	/** Pathfind, then return the trackdir that leads into the shortest path. */
	Trackdir ChooseRailTrack (const RailPathPos &origin, bool reserve_track, PFResult *target)
	{
		if (target != NULL) target->pos.tile = INVALID_TILE;

		/* set origin node */
		Base::SetOrigin(origin);

		/* find the best path */
		bool path_found = FindPath();

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

	/** Pathfind, then return whether return whether to use the second origin. */
	bool CheckReverseTrain (const RailPathPos &pos1, const RailPathPos &pos2, int reverse_penalty)
	{
		/* set origin nodes */
		Base::SetOrigin (pos1, pos2, reverse_penalty, false);

		/* find the best path */
		if (!FindPath()) return false;

		/* path found; walk through the path back to the origin */
		Node *pNode = Base::GetBestNode();
		while (pNode->m_parent != NULL) {
			pNode = pNode->m_parent;
		}

		/* check if it was reversed origin */
		return pNode->m_cost != 0;
	}

	/** Pathfind, then store nearest target. */
	bool FindNearestTargetTwoWay (const RailPathPos &pos1, const RailPathPos &pos2, int max_penalty, int reverse_penalty, TileIndex *target_tile, bool *reversed)
	{
		/* set origin node */
		Base::SetOrigin (pos1, pos2, reverse_penalty, true);
		Base::SetMaxCost(max_penalty);

		/* find the best path */
		if (!FindPath()) return false;

		/* path found; get found target tile */
		Node *n = Base::GetBestNode();
		*target_tile = n->GetLastPos().tile;

		/* walk through the path back to the origin */
		while (n->m_parent != NULL) {
			n = n->m_parent;
		}

		/* if the origin node is our front vehicle tile/Trackdir then we didn't reverse
		 * but we can also look at the cost (== 0 -> not reversed, == reverse_penalty -> reversed) */
		*reversed = (n->m_cost != 0);

		return true;
	}

	/** Pathfind, then optionally try to reserve the path found. */
	bool FindNearestSafeTile (const RailPathPos &pos, bool reserve)
	{
		/* Set origin. */
		Base::SetOrigin(pos);

		if (!FindPath()) return false;

		if (!reserve) return true;

		/* Found a destination, search for a reservation target. */
		Node *pNode = Base::GetBestNode();
		typename Base::NodePos res;
		pNode = Base::FindSafePositionOnPath(pNode, &res)->m_parent;
		assert (pNode->GetPos() == pos);
		assert (pNode->GetLastPos() == pos);

		return Base::TryReservePath (pos.tile, &res);
	}
};


typedef CYapfRailT <CYapfRailOrderT <AstarRailTrackDir> > CYapfRail;

Trackdir YapfTrainChooseTrack(const Train *v, const RailPathPos &origin, bool reserve_track, PFResult *target)
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


typedef CYapfRailT <CYapfAnyDepotRailT <AstarRailTrackDir> > CYapfAnyDepotRail;

bool YapfTrainFindNearestDepot(const Train *v, uint max_penalty, FindDepotData *res)
{
	RailPathPos origin;
	FollowTrainReservation(v, &origin);
	RailPathPos rev = v->Last()->GetReversePos();

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
	bool result1 = pf1.FindNearestTargetTwoWay (origin, rev, max_penalty, YAPF_INFINITE_PENALTY, &res->tile, &res->reverse);

#if DEBUG_YAPF_CACHE
	CYapfAnyDepotRail pf2 (v, !_settings_game.pf.forbid_90_deg);
	TileIndex depot_tile2 = INVALID_TILE;
	bool reversed2 = false;
	pf2.DisableCache(true);
	bool result2 = pf2.FindNearestTargetTwoWay (origin, rev, max_penalty, YAPF_INFINITE_PENALTY, &depot_tile2, &reversed2);
	if (result1 != result2 || (result1 && (res->tile != depot_tile2 || res->reverse != reversed2))) {
		DEBUG(yapf, 0, "CACHE ERROR: FindNearestDepotTwoWay() = [%s, %s]", result1 ? "T" : "F", result2 ? "T" : "F");
		DumpState(pf1, pf2);
	}
#endif

	return result1;
}


typedef CYapfRailT <CYapfAnySafeTileRailT <AstarRailTrackDir> > CYapfAnySafeTileRail;

bool YapfTrainFindNearestSafeTile(const Train *v, const RailPathPos &pos, bool override_railtype)
{
	/* Create pathfinder instance */
	CYapfAnySafeTileRail pf1 (v, !_settings_game.pf.forbid_90_deg, override_railtype);
#if !DEBUG_YAPF_CACHE
	bool result1 = pf1.FindNearestSafeTile(pos, true);

#else
	bool result2 = pf1.FindNearestSafeTile(pos, false);
	CYapfAnySafeTileRail pf2 (v, !_settings_game.pf.forbid_90_deg, override_railtype);
	pf2.DisableCache(true);
	bool result1 = pf2.FindNearestSafeTile(pos, true);
	if (result1 != result2) {
		DEBUG(yapf, 0, "CACHE ERROR: FindSafeTile() = [%s, %s]", result2 ? "T" : "F", result1 ? "T" : "F");
		DumpState(pf1, pf2);
	}
#endif

	return result1;
}
