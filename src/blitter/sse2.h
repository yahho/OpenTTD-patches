/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sse2.h SSE2 blitter functions. */

#ifndef BLITTER_SSE2_H
#define BLITTER_SSE2_H

#include <emmintrin.h>

#ifdef _MSC_VER
	#define ALIGN(n) __declspec(align(n))
#else
	#define ALIGN(n) __attribute__ ((aligned (n)))
#endif

typedef union ALIGN(16) um128i {
	__m128i m128i;
	uint8 m128i_u8[16];
	uint16 m128i_u16[8];
	uint32 m128i_u32[4];
	uint64 m128i_u64[2];
} um128i;

/** SSE2 blitter functions. */
struct SSE2 {
	static void load_u64 (uint64 value, __m128i &into)
	{
#ifdef _SQ64
		into = _mm_cvtsi64_si128 (value);
#else
		(*(um128i*) &into).m128i_u64[0] = value;
#endif
	}

	static __m128i pack_unsaturated (__m128i from, __m128i mask)
	{
		from = _mm_and_si128 (from, mask);    // PAND, wipe high bytes to keep low bytes when packing
		return _mm_packus_epi16 (from, from); // PACKUSWB, pack 2 colours (with saturation)
	}

	static __m128i distribute_alpha (__m128i from, __m128i mask)
	{
		__m128i alphaAB = _mm_shufflelo_epi16 (from, 0x3F); // PSHUFLW, put alpha1 in front of each rgb1
		return _mm_shufflehi_epi16 (alphaAB, 0x3F);         // PSHUFHW, put alpha2 in front of each rgb2
	}
};

#endif /* BLITTER_SSE2_H */
