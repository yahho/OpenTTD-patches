/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sse3.h SSE3 blitter functions. */

#ifndef BLITTER_SSE3_H
#define BLITTER_SSE3_H

#include <tmmintrin.h>

#include "sse2.h"

/** SSE3 blitter functions. */
struct SSE3 : SSE2 {
	static __m128i pack_unsaturated (__m128i from, __m128i mask)
	{
		return _mm_shuffle_epi8 (from, mask);
	}

	static __m128i distribute_alpha (__m128i from, __m128i mask)
	{
		return _mm_shuffle_epi8 (from, mask);
	}

	static __m128i shuffle_epi8 (__m128i x, __m128i y)
	{
		return _mm_shuffle_epi8 (x, y);
	}

	static __m128i hadd_epi16 (__m128i x, __m128i y)
	{
		return _mm_hadd_epi16 (x, y);
	}
};

#endif /* BLITTER_SSE3_H */
