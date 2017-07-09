/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file align.h Alignment helper functions. */

#ifndef ALIGN_H
#define ALIGN_H

template <size_t N>
static inline CONSTEXPR size_t ttd_align_down (size_t x)
{
	assert_tcompile (N > 0);
	assert_tcompile ((N & (N - 1)) == 0);
	return x & ~(N - 1);
}

template <size_t N>
static inline CONSTEXPR size_t ttd_align_up (size_t x)
{
	return ttd_align_down<N> (x + N - 1);
}

template <typename T>
struct ttd_aligner {
	struct aligner {
		char c;
		T t;
	};

	static const size_t alignment = sizeof(aligner) - sizeof(T);
};

template <typename T>
static inline CONSTEXPR size_t ttd_align_up (size_t x)
{
	return ttd_align_up <ttd_aligner<T>::alignment> (x);
}

#endif /* ALIGN_H */
