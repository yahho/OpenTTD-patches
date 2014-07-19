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
	typedef T ForwardListLinkType;

	ForwardListLinkType *next;

	ForwardListLink() : next(NULL) { }

	ForwardListLink (const ForwardListLink &) : next(NULL) { }

	ForwardListLink& operator = (const ForwardListLink &)
	{
		next = NULL;
		return *this;
	}
};

/*
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
 */

/**
 * Generic forward list base with and without tail.
 * @tparam TLink The link type; must derive from ForwardListLink.
 * @tparam Tail Whether to store a pointer to the tail of the list.
 */
template <class TLink, bool Tail>
struct ForwardListBase;

template <class TLink>
struct ForwardListBase <TLink, false> {
	typedef TLink Link;
	typedef typename Link::ForwardListLinkType Type;

protected:
	Type *head;

public:
	ForwardListBase() : head(NULL) { }

	ForwardListBase (const ForwardListBase &other) : head(NULL)
	{
		assert (other.head == NULL);
	}

	ForwardListBase& operator = (const ForwardListBase &) DELETED;

	/** Internal find function. */
	static Type **find_internal (Type **head, const Type *t)
	{
		Type **p = head;
		while (*p != NULL && *p != t) {
			p = &(*p)->Link::next;
		}
		return p;
	}

	/** Internal find function. */
	Type **find_internal (const Type *t)
	{
		return find_internal (&head, t);
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

	/** Internal set tail function. */
	static void set_tail (Type **)
	{
		/* If you call this function, you are doing something wrong. */
		NOT_REACHED();
	}

public:
	/** Append an item or chain of items. */
	void append (Type *t)
	{
		*find_tail() = t;
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

private:
	/** Iterator class. */
	template <typename T, typename C>
	struct iterator_t {
		T *p;

		iterator_t (C *list) : p(list->head)
		{
		}

		iterator_t (void) : p(NULL)
		{
		}

		T& operator * (void) const
		{
			return *p;
		}

		T* operator -> (void) const
		{
			return p;
		}

		bool operator == (const iterator_t &other) const
		{
			return p == other.p;
		}

		bool operator != (const iterator_t &other) const
		{
			return p != other.p;
		}

		iterator_t& operator ++ (void)
		{
			p = p->Link::Next;
			return *this;
		}

		iterator_t operator ++ (int)
		{
			iterator_t r (*this);
			p = p->Link::next;
			return r;
		}
	};

public:
	typedef iterator_t <Type, ForwardListBase> iterator;
	typedef iterator_t <const Type, const ForwardListBase> const_iterator;

	iterator begin (void)
	{
		return iterator (this);
	}

	const_iterator begin (void) const
	{
		return const_iterator (this);
	}

	const_iterator cbegin (void) const
	{
		return const_iterator (this);
	}

	iterator end (void)
	{
		return iterator();
	}

	const_iterator end (void) const
	{
		return const_iterator();
	}

	const_iterator cend (void) const
	{
		return const_iterator();
	}
};

template <class TLink>
struct ForwardListBase <TLink, true> : ForwardListBase <TLink, false> {
	typedef TLink Link;
	typedef typename Link::ForwardListLinkType Type;
	typedef ForwardListBase <Link, false> Base;

protected:
	Type **tail;

	ForwardListBase() : Base(), tail(&this->Base::head) { }

	ForwardListBase (const ForwardListBase &other)
		: Base(other), tail(&this->Base::head)
	{
		assert (other.head == NULL);
		assert (other.tail == &other.head);
	}

	ForwardListBase& operator = (const ForwardListBase &) DELETED;

	/** Internal find tail function. */
	Type **find_tail (void)
	{
		return tail;
	}

	/** Internal set tail function. */
	void set_tail (Type **p)
	{
		tail = p;
	}

public:
	/** Append an item or chain of items. */
	void append (Type *t)
	{
		*tail = t;
		tail = Base::find_internal (tail, NULL);
	}
};

/**
 * Generic forward list.
 * @tparam TLink The link type; must derive from ForwardListLink.
 * @tparam Tail Whether to store a pointer to the tail of the list.
 */
template <class TLink, bool Tail = false>
struct ForwardList : ForwardListBase <TLink, Tail> {
	typedef TLink Link;
	typedef typename Link::ForwardListLinkType Type;
	typedef ForwardListBase <Link, Tail> Base;

private:
	/** Internal remove function. */
	Type *remove_internal (Type **p)
	{
		Type *r = *p;
		if (r == NULL) return NULL;
		*p = r->Link::next;
		if (Tail && *p == NULL) {
			assert (Base::find_tail() == &r->Link::next);
			Base::set_tail (p);
		}
		r->Link::next = NULL;
		return r;
	}

	/** Internal detach function. */
	Type *detach_internal (Type **p)
	{
		Type *r = *p;
		if (r == NULL) return NULL;
		if (Tail) Base::set_tail (p);
		*p = NULL;
		return r;
	}

public:
	/** Prepend an item. */
	void prepend (Type *t)
	{
		assert (t != NULL);
		assert (t->Link::next == NULL);
		if (Tail && Base::head == NULL) {
			assert (Base::find_tail() == &this->Base::head);
			Base::set_tail (&t->Link::next);
		} else {
			t->Link::next = Base::head;
		}
		Base::head = t;
	}

	/** Remove an item. */
	Type *remove (const Type *t)
	{
		return remove_internal (Base::find_internal (t));
	}

	/** Remove an item. */
	template <class Pred>
	Type *remove_pred (Pred pred)
	{
		return remove_internal (Base::find_pred_internal (pred));
	}

	/** Detach an item chain. */
	Type *detach (const Type *t)
	{
		return detach_internal (Base::find_internal (t));
	}

	/** Detach an item chain. */
	template <class Pred>
	Type *detach_pred (Pred pred)
	{
		return detach_internal (Base::find_pred_internal (pred));
	}

	/** Detach the whole list. */
	Type *detach_all (void)
	{
		Type *r = Base::head;
		Base::head = NULL;
		if (Tail) Base::set_tail (&this->Base::head);
		return r;
	}
};

#endif /* FORWARD_LIST_H */
