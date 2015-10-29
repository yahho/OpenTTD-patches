/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sse4.h SSE4 blitter functions. */

#ifndef BLITTER_SSE4_H
#define BLITTER_SSE4_H

#include <smmintrin.h>

#include "sse3.h"

/** SSE4 blitter functions. */
struct SSE4 : SSE3 {
#ifndef _SQ64
	static void load_u64 (uint64 value, __m128i &into)
	{
		into = _mm_cvtsi32_si128 (value);
		into = _mm_insert_epi32 (into, value >> 32, 1);
	}
#endif
};

#endif /* BLITTER_SSE4_H */
