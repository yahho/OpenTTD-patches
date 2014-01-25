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
template <class Titem_, int Thash_bits_open_, int Thash_bits_closed_>
struct Astar {
public:
	/** make Titem_ visible from outside of class */
	typedef Titem_ Titem;
	/** make Titem_::Key a property of HashTable */
	typedef typename Titem_::Key Key;

protected:
	/** here we store full item data (Titem_) */
	SmallArray<Titem_, 65536, 256> m_arr;
	/** hash table of pointers to open item data */
	CHashTableT<Titem_, Thash_bits_open_  > m_open;
	/** hash table of pointers to closed item data */
	CHashTableT<Titem_, Thash_bits_closed_> m_closed;
	/** priority queue of pointers to open item data */
	CBinaryHeapT<Titem_>  m_open_queue;
	/** new open node under construction */
	Titem                *m_new_node;

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

	/** allocate new data item from m_arr */
	inline Titem_ *CreateNewNode()
	{
		if (m_new_node == NULL) m_new_node = m_arr.AppendC();
		return m_new_node;
	}

	/** Notify the nodelist that we don't want to discard the given node. */
	inline void FoundBestNode(Titem_& item)
	{
		/* for now it is enough to invalidate m_new_node if it is our given node */
		if (&item == m_new_node) {
			m_new_node = NULL;
		}
		/* TODO: do we need to store best nodes found in some extra list/array? Probably not now. */
	}

	/** insert given item as open node (into m_open and m_open_queue) */
	inline void InsertOpenNode(Titem_& item)
	{
		assert(m_closed.Find(item.GetKey()) == NULL);
		m_open.Push(item);
		m_open_queue.Include(&item);
		if (&item == m_new_node) {
			m_new_node = NULL;
		}
	}

	/** return the best open node */
	inline Titem_ *GetBestOpenNode()
	{
		if (!m_open_queue.IsEmpty()) {
			return m_open_queue.Begin();
		}
		return NULL;
	}

	/** remove and return the best open node */
	inline Titem_ *PopBestOpenNode()
	{
		if (!m_open_queue.IsEmpty()) {
			Titem_ *item = m_open_queue.Shift();
			m_open.Pop(*item);
			return item;
		}
		return NULL;
	}

	/** return the open node specified by a key or NULL if not found */
	inline Titem_ *FindOpenNode(const Key& key)
	{
		Titem_ *item = m_open.Find(key);
		return item;
	}

	/** remove and return the open node specified by a key */
	inline Titem_& PopOpenNode(const Key& key)
	{
		Titem_& item = m_open.Pop(key);
		uint idxPop = m_open_queue.FindIndex(item);
		m_open_queue.Remove(idxPop);
		return item;
	}

	/** close node */
	inline void InsertClosedNode(Titem_& item)
	{
		assert(m_open.Find(item.GetKey()) == NULL);
		m_closed.Push(item);
	}

	/** return the closed node specified by a key or NULL if not found */
	inline Titem_ *FindClosedNode(const Key& key)
	{
		Titem_ *item = m_closed.Find(key);
		return item;
	}

	/** The number of items. */
	inline int TotalCount() {return m_arr.Length();}
	/** Get a particular item. */
	inline Titem_& ItemAt(int idx) {return m_arr[idx];}

	/** Helper for creating output of this array. */
	template <class D> void Dump(D &dmp) const
	{
		dmp.WriteStructT("m_arr", &m_arr);
	}
};

#endif /* ASTAR_HPP */
