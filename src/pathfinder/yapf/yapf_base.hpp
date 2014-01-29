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
	typedef typename Astar::Node Node;         ///< this will be our node type
	typedef typename Node::Key Key;            ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
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
