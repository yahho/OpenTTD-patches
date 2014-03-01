/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file dbg_helpers.h Functions to be used for debug printings. */

#ifndef DBG_HELPERS_H
#define DBG_HELPERS_H

#include <map>
#include <stack>
#include <string>

#include "str.hpp"

#include "../map/coord.h"
#include "../direction_type.h"
#include "../signal_type.h"
#include "../track_type.h"

/** Helper template class that provides C array length and item type */
template <typename T> struct ArrayT;

/** Helper template class that provides C array length and item type */
template <typename T, size_t N> struct ArrayT<T[N]> {
	static const size_t length = N;
	typedef T item_t;
};


/**
 * Helper template function that returns item of array at given index
 * or t_unk when index is out of bounds.
 */
template <typename E, typename T>
inline typename ArrayT<T>::item_t ItemAtT(E idx, const T &t, typename ArrayT<T>::item_t t_unk)
{
	if ((size_t)idx >= ArrayT<T>::length) {
		return t_unk;
	}
	return t[idx];
}

/**
 * Helper template function that returns item of array at given index
 * or t_inv when index == idx_inv
 * or t_unk when index is out of bounds.
 */
template <typename E, typename T>
inline typename ArrayT<T>::item_t ItemAtT(E idx, const T &t, typename ArrayT<T>::item_t t_unk, E idx_inv, typename ArrayT<T>::item_t t_inv)
{
	if ((size_t)idx < ArrayT<T>::length) {
		return t[idx];
	}
	if (idx == idx_inv) {
		return t_inv;
	}
	return t_unk;
}

/**
 * Helper template function that returns compound bitfield name that is
 * concatenation of names of each set bit in the given value
 * or t_inv when index == idx_inv
 * or t_unk when index is out of bounds.
 */
template <typename E, typename T>
inline void ComposeNameT(FILE *f, E value, const T &t, const char *t_unk, E val_inv, const char *name_inv)
{
	if (value == val_inv) {
		fputs (name_inv, f);
	} else if (value == 0) {
		fputs ("<none>", f);
	} else {
		bool join = false;
		for (size_t i = 0; i < ArrayT<T>::length; i++) {
			if ((value & (1 << i)) == 0) continue;
			if (join) {
				putc ('+', f);
			} else {
				join = true;
			}
			fputs ((const char*)t[i], f);
			value &= ~(E)(1 << i);
		}
		if (value != 0) {
			if (join) putc ('+', f);
			fputs (t_unk, f);
		}
	}
}

void WriteValueStr(Trackdir td, FILE *f);
void WriteValueStr(TrackdirBits td_bits, FILE *f);
void WriteValueStr(DiagDirection dd, FILE *f);
void WriteValueStr(SignalType t, FILE *f);

static inline bool operator < (const std::pair <size_t, const void*> &p1, const std::pair <size_t, const void*> &p2)
{
	if ((size_t)p1.second < (size_t)p2.second) return true;
	if ((size_t)p2.second > (size_t)p2.second) return false;
	return p1.first < p2.first;
}

/** Class that represents the dump-into-string target. */
struct DumpTarget {
	typedef std::pair <size_t, const void*> KnownStructKey;
	typedef std::map<KnownStructKey, std::string> KNOWN_NAMES;

	FILE                   *f;             ///< the output file
	int                     m_indent;      ///< current indent/nesting level
	std::stack<std::string> m_cur_struct;  ///< here we will track the current structure name
	KNOWN_NAMES             m_known_names; ///< map of known object instances and their structured names

	DumpTarget (const char *path)
		: m_indent(0)
	{
		f = fopen (path, "wt");
	}

	~DumpTarget()
	{
		fclose(f);
	}

	static size_t& LastTypeId();
	std::string GetCurrentStructName();
	const std::string *FindKnownName(size_t type_id, const void *ptr);

	void WriteIndent();

	void CDECL WriteLine(const char *format, ...) WARN_FORMAT(2, 3);
	void WriteValue(const char *name);
	void WriteTile(const char *name, TileIndex t);

	/** Dump given enum value (as a number and as named value) */
	template <typename E> void WriteEnumT(const char *name, E e)
	{
		WriteValue(name);
		WriteValueStr (e, f);
		putc ('\n', f);
	}

	void BeginStruct(size_t type_id, const char *name, const void *ptr);
	void EndStruct();

	/** Dump nested object (or only its name if this instance is already known). */
	template <typename S> void WriteStructT(const char *name, const S *s)
	{
		static size_t type_id = ++LastTypeId();

		if (s == NULL) {
			/* No need to dump NULL struct. */
			WriteLine("%s = <null>", name);
			return;
		}
		const std::string *known_as = FindKnownName (type_id, s);
		if (known_as != NULL) {
			/* We already know this one, no need to dump it. */
			WriteLine("%s = known_as.%s", name, known_as->c_str());
		} else {
			/* Still unknown, dump it */
			BeginStruct(type_id, name, s);
			s->Dump(*this);
			EndStruct();
		}
	}
};

#endif /* DBG_HELPERS_H */
