/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pointer.h Smart pointer types. */

#ifndef POINTER_H
#define POINTER_H

#include <cstdlib>

/*
 * Alias a number of smart pointer types to our own custom names, depending
 * on the actual types provided by the implementation:
 * * ttd_shared_ptr is aliased to std::shared_ptr.
 * * ttd_unique_ptr is aliased to std::unique_ptr if available, else to
 *   ttd_shared_ptr (as a consequence, ttd_unique_ptr can only be used with
 *   one template argument, not two).
 * * ttd_unique_free_ptr is an implementation of a ttd_unique_ptr (possibly
 *   through ttd_shared_ptr) that frees (not deletes) its managed pointer
 *   on destruction.
 */


#ifdef __GNUC__
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
#  include <memory>
#  define ttd_shared_ptr std::shared_ptr
#  define ttd_unique_ptr std::unique_ptr
#  define TTD_UNIQUE_PTR_USABLE 1
# else
#  include <tr1/memory>
#  define ttd_shared_ptr std::tr1::shared_ptr
#  define ttd_unique_ptr ttd_shared_ptr
#  define TTD_UNIQUE_PTR_USABLE 0
# endif
#endif /* __GNUC__ */


#ifdef _MSC_VER
# include <memory>
# if _MSC_VER < 1600
#  define ttd_shared_ptr std::tr1::shared_ptr
#  define ttd_unique_ptr ttd_shared_ptr
# else
#  define ttd_shared_ptr std::shared_ptr
#  define ttd_unique_ptr std::unique_ptr
# endif
/* Trying to derive from std::unique_ptr results in MSVC complaining that it
 * cannot generate the default copy constructor for the derived class. */
# define TTD_UNIQUE_PTR_USABLE 0
#endif /* _MSC_VER */


#if TTD_UNIQUE_PTR_USABLE

struct ttd_delete_free {
	void operator() (void *p) { free(p); }
};

template <typename T>
struct ttd_unique_free_ptr : ttd_unique_ptr <T, ttd_delete_free> {
	CONSTEXPR ttd_unique_free_ptr()
		: ttd_unique_ptr <T, ttd_delete_free> () { }

	explicit ttd_unique_free_ptr (T *t)
		: ttd_unique_ptr <T, ttd_delete_free> (t) { }
};

#else

template <typename T>
struct ttd_unique_free_ptr : ttd_shared_ptr <T> {
	CONSTEXPR ttd_unique_free_ptr() : ttd_shared_ptr <T> () { }

	explicit ttd_unique_free_ptr (T *t) : ttd_shared_ptr <T> (t) { }

	void reset (void)
	{
		ttd_shared_ptr<T>::reset();
	}

	template <typename TT>
	void reset (TT *p)
	{
		ttd_shared_ptr<T>::reset (p, (void (*) (void*)) free);
	}
};

#endif


#endif /* POINTER_H */
