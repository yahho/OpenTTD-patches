/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file forward_list.h Forward list implementation. */

#ifndef FORWARD_LIST_H
#define FORWARD_LIST_H

/**
 * Forward list link.
 * @tparam T The type of the objects in the list.
 * @tparam S Unused; can be used to disambiguate the class if you need to
 * derive twice from it.
 */
template <class T, typename S = void>
struct ForwardListLink {
	typedef T Type;

	Type *next;

	ForwardListLink() : next(NULL) { }

	ForwardListLink (const ForwardListLink &) : next(NULL) { }

	ForwardListLink& operator = (const ForwardListLink &)
	{
		next = NULL;
		return *this;
	}
};

/**
 * Implementation of a generic forward list.
 *
 * We have a hand-written implementation of a forward list for the following
 * reasons:
 *
 * 1. std::forward_list is not in C++98, and we still support some compilers
 * that do not implement C++11.
 *
 * 2. The implementation of a forward list below has the next element pointer
 * embedded in the struct, and it must be defined by the struct itself. This
 * allows a struct to be part of more than one list at the same time.
 *
 * @tparam TLink The link type; must derive from ForwardListLink.
 */
template <class TLink>
struct ForwardList {
	typedef TLink Link;
	typedef typename Link::Type Type;

	Type *head;

	ForwardList() : head(NULL) { }

	ForwardList (const ForwardList &other) : head(NULL)
	{
		assert (other.head == NULL);
	}

	ForwardList& operator = (const ForwardList &) DELETED;

private:
	/** Internal find function. */
	Type **find_internal (const Type *t)
	{
		Type **p = &head;
		while (*p != NULL && *p != t) {
			p = &(*p)->Link::next;
		}
		return p;
	}

	/** Internal find tail function. */
	Type **find_tail (void)
	{
		return find_internal (NULL);
	}

	/** Internal predicate-based find function. */
	template <class Pred>
	Type **find_pred_internal (Pred pred)
	{
		Type **p = &head;
		while (*p != NULL && !pred((const Type*)(*p))) {
			p = &(*p)->Link::next;
		}
		return p;
	}

	/** Internal remove function. */
	Type *remove_internal (Type **p)
	{
		Type *r = *p;
		if (r == NULL) return NULL;
		*p = r->Link::next;
		r->Link::next = NULL;
		return r;
	}

	/** Internal detach function. */
	Type *detach_internal (Type **p)
	{
		Type *r = *p;
		if (r == NULL) return NULL;
		*p = NULL;
		return r;
	}

public:
	/** Append an item or chain of items. */
	void append (Type *t)
	{
		*find_tail() = t;
	}

	/** Prepend an item. */
	void prepend (Type *t)
	{
		assert (t->Link::next == NULL);
		t->Link::next = head;
		head = t;
	}

	/** Find an item. */
	Type *find (const Type *t)
	{
		return *find_internal(t);
	}

	/** Find an item. */
	template <class Pred>
	Type *find_pred (Pred pred)
	{
		return *find_pred_internal(pred);
	}

	/** Remove an item. */
	Type *remove (const Type *t)
	{
		return remove_internal (find_internal (t));
	}

	/** Remove an item. */
	template <class Pred>
	Type *remove_pred (Pred pred)
	{
		return remove_internal (find_pred_internal (pred));
	}

	/** Detach an item chain. */
	Type *detach (const Type *t)
	{
		return detach_internal (find_internal (t));
	}

	/** Detach an item chain. */
	template <class Pred>
	Type *detach_pred (Pred pred)
	{
		return detach_internal (find_pred_internal (pred));
	}

	/** Detach the whole list. */
	Type *detach_all (void)
	{
		Type *r = head;
		head = NULL;
		return r;
	}
};

#endif /* FORWARD_LIST_H */
