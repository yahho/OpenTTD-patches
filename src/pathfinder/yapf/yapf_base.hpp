/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_base.hpp Base classes for YAPF. */

#ifndef YAPF_BASE_HPP
#define YAPF_BASE_HPP

#include "../../debug.h"
#include "../../settings_type.h"

extern int _total_pf_time_us;

/**
 * CYapfBaseT - A-star type path finder base class.
 *  Derive your own pathfinder from it. You must provide the following template argument:
 *    Types      - used as collection of local types used in pathfinder
 *
 * Requirements for the Types struct:
 *  ----------------------------------
 *  The following types must be defined in the 'Types' argument:
 *    - Types::Tpf - your pathfinder derived from CYapfBaseT
 *    - Types::Astar - base Astar class
 *  Astar must be (a derived class of) class Astar.
 *
 *  Requirements to your pathfinder class derived from CYapfBaseT:
 *  --------------------------------------------------------------
 *  Your pathfinder derived class needs to implement following methods:
 *    inline void PfSetStartupNodes()
 *    inline void PfFollowNode(Node& org)
 *    inline bool PfCalcCost(Node& n)
 *    inline bool PfCalcEstimate(Node& n)
 *    inline bool PfDetectDestination(Node& n)
 *
 *  For more details about those methods, look at the end of CYapfBaseT
 *  declaration. There are some examples. For another example look at
 *  test_yapf.h (part or unittest project).
 */
template <class Types>
class CYapfBaseT {
public:
	typedef Types TT;
	typedef typename Types::Tpf Tpf;           ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::Astar Astar;       ///< our base pathfinder
	typedef typename Types::VehicleType VehicleType; ///< the type of vehicle
	typedef typename Astar::Node Node;         ///< this will be our node type
	typedef typename Node::Key Key;            ///< key to hash tables

protected:
	const YAPFSettings  *m_settings;           ///< current settings (_settings_game.yapf)
	const VehicleType   *m_veh;                ///< vehicle that we are trying to drive

	int                  m_stats_cost_calcs;   ///< stats - how many node's costs were calculated
	int                  m_stats_cache_hits;   ///< stats - how many node's costs were reused from cache

public:
	CPerformanceTimer    m_perf_cost;          ///< stats - total CPU time of this run
	CPerformanceTimer    m_perf_slope_cost;    ///< stats - slope calculation CPU time
	CPerformanceTimer    m_perf_ts_cost;       ///< stats - GetTrackStatus() CPU time
	CPerformanceTimer    m_perf_other_cost;    ///< stats - other CPU time

public:
	/** default constructor */
	inline CYapfBaseT()
		: m_settings(&_settings_game.pf.yapf)
		, m_veh(NULL)
		, m_stats_cost_calcs(0)
		, m_stats_cache_hits(0)
	{
	}

	/** default destructor */
	~CYapfBaseT() {}

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/** return current settings (can be custom - company based - but later) */
	inline const YAPFSettings& PfGetSettings() const
	{
		return *m_settings;
	}

	/** call the node follower */
	static inline void Follow (Astar *pf, Node *n)
	{
		static_cast<Tpf*>(pf)->PfFollowNode(*n);
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
	inline bool FindPath(const VehicleType *v)
	{
		m_veh = v;

#ifndef NO_DEBUG_MESSAGES
		CPerformanceTimer perf;
		perf.Start();
#endif /* !NO_DEBUG_MESSAGES */

		Yapf().PfSetStartupNodes();
		bool bDestFound = Yapf().Astar::FindPath (Follow, PfGetSettings().max_search_nodes);

#ifndef NO_DEBUG_MESSAGES
		perf.Stop();
		if (_debug_yapf_level >= 2) {
			int t = perf.Get(1000000);
			_total_pf_time_us += t;

			if (_debug_yapf_level >= 3) {
				UnitID veh_idx = (m_veh != NULL) ? m_veh->unitnumber : 0;
				char ttc = Yapf().TransportTypeChar();
				float cache_hit_ratio = (m_stats_cache_hits == 0) ? 0.0f : ((float)m_stats_cache_hits / (float)(m_stats_cache_hits + m_stats_cost_calcs) * 100.0f);
				int cost = bDestFound ? Yapf().Astar::best->m_cost : -1;
				int dist = bDestFound ? Yapf().Astar::best->m_estimate - Yapf().Astar::best->m_cost : -1;

				DEBUG(yapf, 3, "[YAPF%c]%c%4d- %d us - %d rounds - %d open - %d closed - CHR %4.1f%% - C %d D %d - c%d(sc%d, ts%d, o%d) -- ",
					ttc, bDestFound ? '-' : '!', veh_idx, t, Yapf().Astar::num_steps, Yapf().Astar::OpenCount(), Yapf().Astar::ClosedCount(),
					cache_hit_ratio, cost, dist, m_perf_cost.Get(1000000), m_perf_slope_cost.Get(1000000),
					m_perf_ts_cost.Get(1000000), m_perf_other_cost.Get(1000000)
				);
			}
		}
#endif /* !NO_DEBUG_MESSAGES */
		return bDestFound;
	}

	/** Add new node (created by CreateNewNode and filled with data) into open list */
	inline void AddStartupNode(Node& n)
	{
		Yapf().PfNodeCacheFetch(n);
		Yapf().Astar::InsertInitialNode(&n);
	}

	/** Create and add a new node */
	inline void AddStartupNode (const PathPos &pos, bool is_choice, int cost = 0)
	{
		Node &node = *Yapf().Astar::CreateNewNode (NULL, pos, is_choice);
		node.m_cost = cost;
		AddStartupNode (node);
	}

	/** add multiple nodes - direct children of the given node */
	inline void AddMultipleNodes(Node *parent, const TrackFollower &tf)
	{
		bool is_choice = !tf.m_new.is_single();
		PathPos pos = tf.m_new;
		for (TrackdirBits rtds = tf.m_new.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.td = FindFirstTrackdir(rtds);
			Node& n = *Yapf().Astar::CreateNewNode(parent, pos, is_choice);
			AddNewNode(n, tf);
		}
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
		while (Yapf().Astar::best_intermediate != NULL && (Yapf().Astar::best_intermediate->m_segment->m_end_segment_reason & ESRB_CHOICE_FOLLOWS) == 0) {
			Yapf().Astar::best_intermediate = Yapf().Astar::best_intermediate->m_parent;
		}
	}

	/**
	 * AddNewNode() - called by Tderived::PfFollowNode() for each child node.
	 *  Nodes are evaluated here and added into open list
	 */
	void AddNewNode(Node &n, const TrackFollower &tf)
	{
		/* evaluate the node */
		bool bCached = Yapf().PfNodeCacheFetch(n);
		if (!bCached) {
			m_stats_cost_calcs++;
		} else {
			m_stats_cache_hits++;
		}

		bool bValid = Yapf().PfCalcCost(n, &tf);

		if (bCached) {
			Yapf().PfNodeCacheFlush(n);
		}

		if (bValid) bValid = Yapf().PfCalcEstimate(n);

		/* have the cost or estimate callbacks marked this node as invalid? */
		if (!bValid) return;

		/* detect the destination */
		if (Yapf().PfDetectDestination(n)) {
			Yapf().Astar::FoundTarget(&n);
		} else {
			Yapf().Astar::InsertNode(&n);
		}
	}

	const VehicleType * GetVehicle() const
	{
		return m_veh;
	}

	void DumpBase(DumpTarget &dmp) const
	{
		Yapf().Astar::Dump(dmp);
	}

	/* methods that should be implemented at derived class Types::Tpf (derived from CYapfBaseT) */

#if 0
	/** Example: PfSetStartupNodes() - set source (origin) nodes */
	inline void PfSetStartupNodes()
	{
		/* example: */
		Node& n1 = *base::Astar::CreateNewNode();
		.
		. // setup node members here
		.
		base::Astar::InsertOpenNode(n1);
	}

	/** Example: PfFollowNode() - set following (child) nodes of the given node */
	inline void PfFollowNode(Node& org)
	{
		for (each follower of node org) {
			Node& n = *base::Astar::CreateNewNode();
			.
			. // setup node members here
			.
			n.m_parent   = &org; // set node's parent to allow back tracking
			AddNewNode(n);
		}
	}

	/** Example: PfCalcCost() - set path cost from origin to the given node */
	inline bool PfCalcCost(Node& n)
	{
		/* evaluate last step cost */
		int cost = ...;
		/* set the node cost as sum of parent's cost and last step cost */
		n.m_cost = n.m_parent->m_cost + cost;
		return true; // true if node is valid follower (i.e. no obstacle was found)
	}

	/** Example: PfCalcEstimate() - set path cost estimate from origin to the target through given node */
	inline bool PfCalcEstimate(Node& n)
	{
		/* evaluate the distance to our destination */
		int distance = ...;
		/* set estimate as sum of cost from origin + distance to the target */
		n.m_estimate = n.m_cost + distance;
		return true; // true if node is valid (i.e. not too far away :)
	}

	/** Example: PfDetectDestination() - return true if the given node is our destination */
	inline bool PfDetectDestination(Node& n)
	{
		bool bDest = (n.m_key.m_x == m_x2) && (n.m_key.m_y == m_y2);
		return bDest;
	}
#endif
};

#endif /* YAPF_BASE_HPP */
