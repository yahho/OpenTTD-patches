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
 * @tparam T The type of the objects in the list, which must derive from
 * ForwardListLink <T, S>.
 * @tparam S Unused; can be used to disambiguate the class if you need to
 * derive twice from it.
 */
template <class T, typename S = void>
struct ForwardListLink {
	T *next;

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
 * @tparam T The type of the objects in the list, which must derive from
 * ForwardListLink <T, S>.
 * @tparam Tail Whether to store a pointer to the tail of the list.
 * @tparam S See T and ForwardListLink.
 */
template <class T, bool Tail, typename S>
struct ForwardListBase;

template <class T, typename S>
struct ForwardListBase <T, false, S> {
	typedef ForwardListLink <T, S> Link;

protected:
	T *head;

public:
	ForwardListBase() : head(NULL) { }

	ForwardListBase (const ForwardListBase &other) : head(NULL)
	{
		assert (other.head == NULL);
	}

	ForwardListBase& operator = (const ForwardListBase &) DELETED;

protected:
	/** Cast an object pointer to a pointer to its link base class. */
	static Link *link_cast (T *t)
	{
		return static_cast<Link*>(t);
	}

	/** Cast an object pointer to a pointer to its link base class, const version. */
	static const Link *link_cast (const T *t)
	{
		return static_cast<const Link*>(t);
	}

	/** Get the next pointer in an item. */
	static T *get_next (T *t)
	{
		return link_cast(t)->next;
	}

	/** Get the next pointer in an item, const version. */
	static const T *get_next (const T *t)
	{
		return link_cast(t)->next;
	}

	/** Get the address of the next pointer in an item. */
	static T **get_next_addr (T *t)
	{
		return &link_cast(t)->next;
	}

	/** Set the next pointer in an item. */
	static T *set_next (T *t, T *n)
	{
		link_cast(t)->next = n;
		return n;
	}

	/** Internal find function. */
	static T **find_internal (T **head, const T *t)
	{
		T **p = head;
		while (*p != NULL && *p != t) {
			p = get_next_addr(*p);
		}
		return p;
	}

	/** Internal find function. */
	T **find_internal (const T *t)
	{
		return find_internal (&head, t);
	}

	/** Internal find tail function. */
	T **find_tail (void)
	{
		return find_internal (NULL);
	}

	/** Internal predicate-based find function. */
	template <class Pred>
	T **find_pred_internal (Pred pred)
	{
		T **p = &head;
		while (*p != NULL && !pred((const T*)(*p))) {
			p = get_next_addr(*p);
		}
		return p;
	}

	/** Internal set tail function. */
	static void set_tail (T **)
	{
		/* If you call this function, you are doing something wrong. */
		NOT_REACHED();
	}

public:
	/** Append an item or chain of items. */
	void append (T *t)
	{
		*find_tail() = t;
	}

	/** Find an item. */
	T *find (const T *t)
	{
		return *find_internal(t);
	}

	/** Find an item. */
	template <class Pred>
	T *find_pred (Pred pred)
	{
		return *find_pred_internal(pred);
	}

private:
	/** Iterator class. */
	template <typename Q, typename C>
	struct iterator_t {
		Q *p;

		iterator_t (C *list) : p(list->head)
		{
		}

		iterator_t (void) : p(NULL)
		{
		}

		Q& operator * (void) const
		{
			return *p;
		}

		Q* operator -> (void) const
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
			p = get_next (p);
			return *this;
		}

		iterator_t operator ++ (int)
		{
			iterator_t r (*this);
			p = get_next (p);
			return r;
		}
	};

public:
	typedef iterator_t <T, ForwardListBase> iterator;
	typedef iterator_t <const T, const ForwardListBase> const_iterator;

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

template <class T, typename S>
struct ForwardListBase <T, true, S> : ForwardListBase <T, false, S> {
	typedef ForwardListBase <T, false, S> Base;
	typedef ForwardListLink <T, S> Link;

protected:
	T **tail;

	ForwardListBase() : Base(), tail(&this->Base::head) { }

	ForwardListBase (const ForwardListBase &other)
		: Base(other), tail(&this->Base::head)
	{
		assert (other.head == NULL);
		assert (other.tail == &other.head);
	}

	ForwardListBase& operator = (const ForwardListBase &) DELETED;

	/** Internal find tail function. */
	T **find_tail (void)
	{
		return tail;
	}

	/** Internal set tail function. */
	void set_tail (T **p)
	{
		tail = p;
	}

public:
	/** Append an item or chain of items. */
	void append (T *t)
	{
		*tail = t;
		tail = Base::find_internal (tail, NULL);
	}
};

/**
 * Generic forward list.
 * @tparam T The link type; must derive from ForwardListLink.
 * @tparam Tail Whether to store a pointer to the tail of the list.
 * @tparam S Unused; see ForwardListLink.
 */
template <class T, bool Tail = false, typename S = void>
struct ForwardList : ForwardListBase <T, Tail, S> {
	typedef ForwardListBase <T, Tail, S> Base;

private:
	/** Internal remove function. */
	T *remove_internal (T **p)
	{
		T *r = *p;
		if (r == NULL) return NULL;
		*p = Base::get_next (r);
		if (Tail && *p == NULL) {
			assert (Base::find_tail() == p);
			Base::set_tail (p);
		}
		Base::set_next (r, NULL);
		return r;
	}

	/** Internal detach function. */
	T *detach_internal (T **p)
	{
		T *r = *p;
		if (r == NULL) return NULL;
		if (Tail) Base::set_tail (p);
		*p = NULL;
		return r;
	}

public:
	/** Prepend an item. */
	void prepend (T *t)
	{
		assert (t != NULL);
		assert (Base::get_next(t) == NULL);
		if (Tail && Base::head == NULL) {
			assert (Base::find_tail() == &this->Base::head);
			Base::set_tail (Base::get_next_addr(t));
		} else {
			Base::set_next (t, Base::head);
		}
		Base::head = t;
	}

	/** Remove an item. */
	T *remove (const T *t)
	{
		return remove_internal (Base::find_internal (t));
	}

	/** Remove an item. */
	template <class Pred>
	T *remove_pred (Pred pred)
	{
		return remove_internal (Base::find_pred_internal (pred));
	}

	/** Detach an item chain. */
	T *detach (const T *t)
	{
		return detach_internal (Base::find_internal (t));
	}

	/** Detach an item chain. */
	template <class Pred>
	T *detach_pred (Pred pred)
	{
		return detach_internal (Base::find_pred_internal (pred));
	}

	/** Detach the whole list. */
	T *detach_all (void)
	{
		T *r = Base::head;
		Base::head = NULL;
		if (Tail) Base::set_tail (&this->Base::head);
		return r;
	}
};

#endif /* FORWARD_LIST_H */
