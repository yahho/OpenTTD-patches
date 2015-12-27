/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file astar.hpp A-star pathfinder implementation. */

#ifndef ASTAR_HPP
#define ASTAR_HPP

#include <deque>
#include <set>

#include "../../misc/array.hpp"
#include "../../misc/hashtable.hpp"

/**
 * A-star node template common base
 *
 * This struct contains the common fields that the A-star algorithm below
 * requires a node to have. Users of the A-star pathfinder must define a
 * node class that derives from this struct, using that node class itself
 * as template argument. Such a class must define a Key type to be used in
 * hashes, and a GetKey method to get the key for a particular node.
 */
template <class Node>
struct AstarNode : CHashTableEntryT<Node> {
	const Node *m_parent;   ///< parent node in path
	int         m_cost;     ///< cost of this node
	int         m_estimate; ///< estimated cost to target

	/** Construct a node */
	AstarNode (const Node *parent)
		: m_parent (parent), m_cost (0), m_estimate (0)
	{
	}

	/** Get the cost of this node */
	inline int GetCost() const
	{
		return m_cost;
	}

	/** Get the estimated final cost to the target */
	inline int GetCostEstimate() const
	{
		return m_estimate;
	}

	/** Compare estimated final cost with another node */
	inline bool operator < (const Node &other) const
	{
		return m_estimate < other.m_estimate;
	}
};

/**
 * A-star pathfinder implementation class
 *
 * Instantiate this class by supplying your node class as template argument;
 * such a class must derive from AstarNode above, and provide a Key type
 * for hashes and a GetKey method to retrieve the key for a node.
 */
template <class TNode, int open_hash_bits, int closed_hash_bits>
struct Astar {
public:
	typedef TNode Node;              ///< Make #TNode visible from outside of class.
	typedef typename TNode::Key Key; ///< Make TNode::Key a property of HashTable.

private:
	/** List of nodes. Note that elements are never removed. */
	struct NodeList : private std::deque <Node> {
		/** Get a reference the last element in the list. */
		Node &top (void)
		{
			return this->back();
		}

		/** Push a node onto the list. */
		void push (const Node &n)
		{
			this->push_back (n);
		}

		/** Dump list for debugging. */
		template <typename D>
		void Dump (D &dmp) const
		{
			uint size = this->size();
			dmp.WriteLine ("size = %d", size);
			char name [24];
			for (uint i = 0; i < size; i++) {
				bstrfmt (name, "item[%d]", i);
				dmp.WriteStructT (name, &(*this)[i]);
			}
		}
	};

	/** Sorting struct for pointer to nodes. */
	struct NodePtrLess {
		bool operator() (const Node *n1, const Node *n2) const
		{
			return *n1 < *n2;
		}
	};

	/** Priority queue of open nodes. */
	struct PriorityQueue : private std::multiset <Node *, NodePtrLess> {
		typedef std::multiset <Node *, NodePtrLess> base;
		typedef typename base::iterator iterator;

		/** Check if the queue is empty. */
		bool empty (void) const
		{
			return base::empty();
		}

		void insert (Node *n)
		{
			base::insert (n);
		}

		void remove (Node *n)
		{
			iterator it (base::lower_bound (n));
			for (;;) {
				/* It should really be there. */
				assert (it != base::end());
				assert (n->GetCostEstimate() == (*it)->GetCostEstimate());
				if (n == *it) break;
				it++;
			}
			base::erase (it);
		}

		Node *pop (void)
		{
			iterator it (base::begin());
			Node *n = *it;
			base::erase (it);
			return n;
		}
	};

	NodeList nodes;                               ///< Here we store full item data (Node).
	CHashTableT<Node, open_hash_bits  > m_open;   ///< Hash table of pointers to open item data.
	CHashTableT<Node, closed_hash_bits> m_closed; ///< Hash table of pointers to closed item data.
	PriorityQueue m_open_queue;                   ///< Priority queue of pointers to open item data.

public:
	const Node *best;              ///< pointer to the destination node found at last round
	const Node *best_intermediate; ///< here should be node closest to the destination if path not found
	int         max_search_nodes;  ///< maximum number of nodes we are allowed to visit before we give up
	int         num_steps;         ///< this is there for debugging purposes (hope it doesn't hurt)

	/** default constructor */
	Astar() : best(NULL),
		best_intermediate(NULL), max_search_nodes(0), num_steps(0)
	{
	}

	/** return number of open nodes */
	inline uint OpenCount()
	{
		return m_open.Count();
	}

	/** return number of closed nodes */
	inline uint ClosedCount()
	{
		return m_closed.Count();
	}

private:
	/** Insert given node as open node (into m_open and m_open_queue). */
	inline void InsertOpenNode (Node *n)
	{
		/* node to insert should be a newly created one */
		assert (n == &nodes.top());
		assert (m_closed.Find(n->GetKey()) == NULL);

		m_open.Push(*n);
		m_open_queue.insert (n);
	}

	/** Possibly replace an existing (open) node. */
	inline void ReplaceNode (const Key &key, Node *n1, const Node *n2)
	{
		if (n2->GetCostEstimate() < n1->GetCostEstimate()) {
			/* pop old node from open list and queue */
			Node &item = m_open.Pop (key);
			m_open_queue.remove (&item);
			/* update old node with new data */
			*n1 = *n2;
			/* add updated node to open list and queue */
			m_open.Push(*n1);
			m_open_queue.insert (n1);
		}
	}

public:
	/** Insert a new initial node. */
	inline void InsertInitialNode (const Node &n)
	{
		/* closed list should be empty when adding initial nodes */
		assert (m_closed.Count() == 0);

		/* insert the new node only if it is not there yet */
		const Key key = n.GetKey();
		Node *m = m_open.Find (key);
		if (m == NULL) {
			nodes.push (n);
			InsertOpenNode (&nodes.top());
		} else {
			/* two initial nodes with same key;
			 * pick the one with the lowest cost */
			ReplaceNode (key, m, &n);
		}
	}

	/** Insert a new node */
	inline void InsertNode (const Node &n)
	{
		const Key key = n.GetKey();

		/* check new node against open list */
		Node *m = m_open.Find (key);
		if (m != NULL) {
			assert (m_closed.Find (key) == NULL);
			/* another node exists with the same key in the open list
			 * is it better than new one? */
			ReplaceNode (key, m, &n);
		} else {
			/* check new node against closed list */
			m = m_closed.Find (key);
			if (m != NULL) {
				/* another node exists with the same key in the closed list
				 * is it better than new one? */
				assert (m->GetCostEstimate() <= n.GetCostEstimate());
				return;
			}

			/* the new node is really new
			 * add it to the open list */
			nodes.push (n);
			m = &nodes.top();
			InsertOpenNode (m);
		}

		if (max_search_nodes > 0 && (best_intermediate == NULL || (best_intermediate->GetCostEstimate() - best_intermediate->GetCost()) > (m->GetCostEstimate() - m->GetCost()))) {
			best_intermediate = m;
		}
	}

	/** Found target. */
	inline void InsertTarget (const Node &n)
	{
		nodes.push (n);
		Node *m = &nodes.top();
		if (best == NULL || *m < *best) best = m;
	}

	/**
	 * Find path.
	 *
	 * Call this method with a follow function to be used to find
	 * neighbours, and an optional maximum number of nodes to visit,
	 * after all initial nodes have been added with InsertInitialNode.
	 * Function follow will be called with this Astar instance as
	 * first argument and the node to follow as second argument, and
	 * should find the neigbours of the given node, create a node for
	 * each of them, compute their current cost and estimated final
	 * cost to destination and then call InsertNode to add them as open
	 * nodes; or, if one of them is a destination, call InsertTarget.
	 */
	template <class T>
	inline bool FindPath (void (*follow) (T*, Node*), uint max_nodes = 0)
	{
		max_search_nodes = max_nodes;

		for (;;) {
			num_steps++;
			if (m_open_queue.empty()) {
				return best != NULL;
			}

			Node *n = m_open_queue.pop();

			/* if the best open node was worse than the best path found, we can finish */
			if (best != NULL && best->GetCost() < n->GetCostEstimate()) {
				return true;
			}

			follow (static_cast<T*>(this), n);

			if (max_nodes > 0 && ClosedCount() >= max_nodes) {
				return best != NULL;
			}

			m_open.Pop (n->GetKey());
			m_closed.Push (*n);
		}
	}

	/**
	 * If path was found return the best node that has reached the destination.
	 * Otherwise return the best visited node (which was nearest to the destination).
	 */
	inline const Node *GetBestNode() const
	{
		return (best != NULL) ? best : best_intermediate;
	}

	/** Dump the current state of the pathfinder. */
	template <typename D>
	void DumpState (D &dmp) const
	{
		dmp.WriteLine ("num_steps = %d", num_steps);
		dmp.WriteStructT ("array", &nodes);
	}
};

#endif /* ASTAR_HPP */
