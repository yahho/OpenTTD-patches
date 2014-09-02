/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file hashtable.hpp Hash table support. */

#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <functional>

#include "../core/math_func.hpp"
#include "../core/forward_list.h"

template <class Titem_>
struct CHashTableEntryT : ForwardListLink <Titem_, CHashTableEntryT<Titem_> >
{
};

/**
 * class CHashTableT<Titem, Thash_bits> - simple hash table
 *  of pointers allocated elsewhere.
 *
 *  Supports: Add/Find/Remove of Titems.
 *
 *  Your Titem must meet some extra requirements to be CHashTableT
 *  compliant:
 *    - its constructor/destructor (if any) must be public
 *    - if the copying of item requires an extra resource management,
 *        you must define also copy constructor
 *    - must support nested type (struct, class or typedef) Titem::Key
 *        that defines the type of key class for that item
 *    - must support public method:
 *        const Key& GetKey() const; // return the item's key object
 *
 *  In addition, the Titem::Key class must support:
 *    - public method that calculates key's hash:
 *        int CalcHash() const;
 *    - public 'equality' operator to compare the key with another one
 *        bool operator == (const Key& other) const;
 */
template <class Titem_, int Thash_bits_>
class CHashTableT {
public:
	typedef Titem_ Titem;                         // make Titem_ visible from outside of class
	typedef typename Titem_::Key Tkey;            // make Titem_::Key a property of HashTable
	static const int Thash_bits = Thash_bits_;    // publish num of hash bits
	static const int Tcapacity = 1 << Thash_bits; // and num of slots 2^bits

protected:
	/**
	 * each slot contains pointer to the first item in the list
	 */
	typedef ForwardList <Titem_, false, CHashTableEntryT<Titem_> > Slot;

	Slot  m_slots[Tcapacity]; // here we store our data (array of blobs)
	uint  m_num_items;        // item counter

public:
	/* default constructor */
	inline CHashTableT() : m_num_items(0)
	{
	}

protected:
	/** static helper - return hash for the given key modulo number of slots */
	inline static int CalcHash(const Tkey& key)
	{
		int32 hash = key.CalcHash();
		if ((8 * Thash_bits) < 32) hash ^= hash >> (min(8 * Thash_bits, 31));
		if ((4 * Thash_bits) < 32) hash ^= hash >> (min(4 * Thash_bits, 31));
		if ((2 * Thash_bits) < 32) hash ^= hash >> (min(2 * Thash_bits, 31));
		if ((1 * Thash_bits) < 32) hash ^= hash >> (min(1 * Thash_bits, 31));
		hash &= (1 << Thash_bits) - 1;
		return hash;
	}

private:
	/** predicate function for item search */
	static bool IsKey (const Titem_ *item, const Tkey *key)
	{
		return item->GetKey() == *key;
	}

	/** helper function to find a key in a slot */
	static Titem_ *FindKeyInSlot (Slot *slot, const Tkey &key)
	{
		return slot->find_pred (std::bind2nd (std::ptr_fun(&IsKey), &key));
	}

	/** helper function to remove a key from a slot */
	static Titem_ *RemoveKeyFromSlot (Slot *slot, const Tkey &key)
	{
		return slot->remove_pred (std::bind2nd (std::ptr_fun(&IsKey), &key));
	}

public:
	/** item count */
	inline uint Count() const {return m_num_items;}

	/** simple clear - forget all items - used by CSegmentCostCacheT.Flush() */
	inline void Clear() {for (int i = 0; i < Tcapacity; i++) m_slots[i].detach_all();}

	/** const item search */
	const Titem_ *Find(const Tkey& key) const
	{
		int hash = CalcHash(key);
		return FindKeyInSlot (&m_slots[hash], key);
	}

	/** non-const item search */
	Titem_ *Find(const Tkey& key)
	{
		int hash = CalcHash(key);
		return FindKeyInSlot (&m_slots[hash], key);
	}

	/** non-const item search & optional removal (if found) */
	Titem_ *TryPop(const Tkey& key)
	{
		int hash = CalcHash(key);
		Titem_ *item = RemoveKeyFromSlot (&m_slots[hash], key);
		if (item != NULL) {
			m_num_items--;
		}
		return item;
	}

	/** non-const item search & removal */
	Titem_& Pop(const Tkey& key)
	{
		Titem_ *item = TryPop(key);
		assert(item != NULL);
		return *item;
	}

	/** non-const item search & optional removal (if found) */
	bool TryPop(Titem_& item)
	{
		const Tkey& key = item.GetKey();
		int hash = CalcHash(key);
		if (m_slots[hash].remove(&item) == NULL) return false;
		m_num_items--;
		return true;
	}

	/** non-const item search & removal */
	void Pop(Titem_& item)
	{
		bool ret = TryPop(item);
		assert(ret);
	}

	/** add one item - copy it from the given item */
	void Push(Titem_& new_item)
	{
		const Tkey& key = new_item.GetKey();
		int hash = CalcHash(key);
		assert(FindKeyInSlot (&m_slots[hash], key) == NULL);
		m_slots[hash].prepend (&new_item);
		m_num_items++;
	}
};

#endif /* HASHTABLE_HPP */
