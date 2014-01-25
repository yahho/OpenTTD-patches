/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file astar.hpp A-star pathfinder implementation. */

#ifndef ASTAR_HPP
#define ASTAR_HPP

#include "../../misc/array.hpp"
#include "../../misc/hashtable.hpp"
#include "../../misc/binaryheap.hpp"

/**
 * A-star node template common base
 *
 * This struct contains the common fields that the A-star algorithm below
 * requires a node to have. Users of the A-star pathfinder must define a
 * node class that derives from this struct, using that node class itself
 * as template argument. Such a class must define a Key type to be used in
 * hashes, and a GetKey method to get the key for a particular node. It
 * may also define a Set method to initalise the node, which must take a
 * parent Node as first argument, and a Dump method to dump its contents;
 * either one defined must hook into this base class's corresponding own
 * method.
 */
template <class Node>
struct AstarNodeBase {
	Node *m_hash_next; ///< next node in hash bucket
	Node *m_parent;    ///< parent node in path
	int   m_cost;      ///< cost of this node
	int   m_estimate;  ///< estimated cost to target

	/** Initialise this node */
	inline void Set (Node *parent)
	{
		m_hash_next = NULL;
		m_parent = parent;
		m_cost = 0;
		m_estimate = 0;
	}

	/** Get the next node in the hash bucket, to be used internally */
	inline Node *GetHashNext() const
	{
		return m_hash_next;
	}

	/** Set the next node in the hash bucket, to be used internally */
	inline void SetHashNext (Node *next)
	{
		m_hash_next = next;
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
	inline bool operator < (const Node& other) const
	{
		return m_estimate < other.m_estimate;
	}

	/** Dump this node */
	template <class D> void Dump (D &dmp) const
	{
		dmp.WriteStructT("m_parent", this->m_parent);
		dmp.WriteLine("m_cost = %d", this->m_cost);
		dmp.WriteLine("m_estimate = %d", this->m_estimate);
	}
};

/**
 * A-star pathfinder implementation class
 *
 * Instantiate this class by supplying your node class as template argument;
 * such a class must derive from AstarNodeBase above, and provide a Key type
 * for hashes and a GetKey method to retrieve the key for a node.
 */
template <class TNode, int open_hash_bits, int closed_hash_bits>
struct Astar {
public:
	/** make TNode visible from outside of class */
	typedef TNode Node;
	/** make TNode::Key a property of HashTable */
	typedef typename TNode::Key Key;

protected:
	/** here we store full item data (Node) */
	SmallArray<Node, 65536, 256> m_arr;
	/** hash table of pointers to open item data */
	CHashTableT<Node, open_hash_bits  > m_open;
	/** hash table of pointers to closed item data */
	CHashTableT<Node, closed_hash_bits> m_closed;
	/** priority queue of pointers to open item data */
	CBinaryHeapT<Node> m_open_queue;
	/** new open node under construction */
	Node              *m_new_node;

public:
	/** default constructor */
	Astar() : m_open_queue(2048), m_new_node(NULL)
	{
	}

	/** return number of open nodes */
	inline int OpenCount()
	{
		return m_open.Count();
	}

	/** return number of closed nodes */
	inline int ClosedCount()
	{
		return m_closed.Count();
	}

	/** Create a new node */
	inline Node *CreateNewNode (Node *parent)
	{
		if (m_new_node == NULL) m_new_node = m_arr.AppendC();
		m_new_node->Set (parent);
		return m_new_node;
	}

	/** Create a new node, one parameter */
	template <class T1>
	inline Node *CreateNewNode (Node *parent, T1 t1)
	{
		if (m_new_node == NULL) m_new_node = m_arr.AppendC();
		m_new_node->Set (parent, t1);
		return m_new_node;
	}

	/** Create a new node, two parameters */
	template <class T1, class T2>
	inline Node *CreateNewNode (Node *parent, T1 t1, T2 t2)
	{
		if (m_new_node == NULL) m_new_node = m_arr.AppendC();
		m_new_node->Set (parent, t1, t2);
		return m_new_node;
	}

	/** Notify the nodelist that we don't want to discard the given node. */
	inline void FoundBestNode (Node *n)
	{
		/* node should be a newly created one */
		assert (n == m_new_node);

		m_new_node = NULL;
	}

	/** insert given item as open node (into m_open and m_open_queue) */
	inline void InsertOpenNode(Node& item)
	{
		assert(m_closed.Find(item.GetKey()) == NULL);
		m_open.Push(item);
		m_open_queue.Include(&item);
		if (&item == m_new_node) {
			m_new_node = NULL;
		}
	}

	/** return the best open node */
	inline Node *GetBestOpenNode()
	{
		if (!m_open_queue.IsEmpty()) {
			return m_open_queue.Begin();
		}
		return NULL;
	}

	/** remove and return the open node specified by a key */
	inline Node& PopOpenNode(const Key& key)
	{
		Node& item = m_open.Pop(key);
		uint idxPop = m_open_queue.FindIndex(item);
		m_open_queue.Remove(idxPop);
		return item;
	}

	/** close node */
	inline void InsertClosedNode(Node& item)
	{
		assert(m_open.Find(item.GetKey()) == NULL);
		m_closed.Push(item);
	}

	/** Replace an existing (open) node. */
	inline void ReplaceNode (const Key &key, Node *n1, const Node *n2)
	{
		/* pop old node from open list and queue */
		PopOpenNode (key);
		/* update old node with new data */
		*n1 = *n2;
		/* add updated node to open list and queue */
		InsertOpenNode (*n1);
	}

	/** Insert a new initial node. */
	inline void InsertInitialNode (Node *n)
	{
		/* node to insert should be a newly created one */
		assert (n == m_new_node);

		/* closed list should be empty when adding initial nodes */
		assert (m_closed.Count() == 0);

		/* insert the new node only if it is not there yet */
		const Key key = n->GetKey();
		Node *m = m_open.Find (key);
		if (m == NULL) {
			InsertOpenNode(*n);
		} else {
			/* two initial nodes with same key;
			 * pick the one with the lowest cost */
			if (n->GetCostEstimate() < m->GetCostEstimate()) {
				ReplaceNode (key, m, n);
			}
		}
	}

	/** Insert a new node */
	inline void InsertNode (Node *n)
	{
		/* node to insert should be a newly created one */
		assert (n == m_new_node);

		const Key key = n->GetKey();

		/* check new node against open list */
		Node *m = m_open.Find (key);
		if (m != NULL) {
			/* another node exists with the same key in the open list
			 * is it better than new one? */
			if (n->GetCostEstimate() < m->GetCostEstimate()) {
				ReplaceNode (key, m, n);
			}
			return;
		}

		/* check new node against closed list */
		m = m_closed.Find(key);
		if (m != NULL) {
			/* another node exists with the same key in the closed list
			 * is it better than new one? */
			assert (m->GetCostEstimate() <= n->GetCostEstimate());
			return;
		}

		/* the new node is really new
		 * add it to the open list */
		InsertOpenNode(*n);
	}

	/** Helper for creating output of this array. */
	template <class D> void Dump(D &dmp) const
	{
		dmp.WriteStructT("m_arr", &m_arr);
	}
};

#endif /* ASTAR_HPP */
