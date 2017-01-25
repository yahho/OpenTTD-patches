/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport_sprite_sorter_sse.cpp Sprite sorter that uses SSE4.1. */

#ifdef WITH_SSE

#include "stdafx.h"
#include "smmintrin.h"
#include "viewport_sprite_sorter.h"

#ifdef _SQ64
	assert_compile((sizeof(ParentSpriteToDraw) % 16) == 0);
	#define LOAD_128 _mm_load_si128
#else
	#define LOAD_128 _mm_loadu_si128
#endif

struct CompareParentSpritesSSE41 {
	const __m128i mask_ptest;

	CompareParentSpritesSSE41 (void) :
		mask_ptest (_mm_setr_epi8 (-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0))
	{
	}

	bool operator() (const ParentSpriteToDraw *ps1,
				const ParentSpriteToDraw *ps2) const;
};

inline bool CompareParentSpritesSSE41::operator()
	(const ParentSpriteToDraw *ps1, const ParentSpriteToDraw *ps2) const
{
	/*
	 * Original code:
	 * if (ps->xmax >= ps2->xmin && ps->xmin <= ps2->xmax && // overlap in X?
	 *     ps->ymax >= ps2->ymin && ps->ymin <= ps2->ymax && // overlap in Y?
	 *     ps->zmax >= ps2->zmin && ps->zmin <= ps2->zmax) { // overlap in Z?
	 *
	 * Above conditions are equivalent to:
	 * 1/    !( (ps->xmax >= ps2->xmin) && (ps->ymax >= ps2->ymin) && (ps->zmax >= ps2->zmin)   &&    (ps->xmin <= ps2->xmax) && (ps->ymin <= ps2->ymax) && (ps->zmin <= ps2->zmax) )
	 * 2/    !( (ps->xmax >= ps2->xmin) && (ps->ymax >= ps2->ymin) && (ps->zmax >= ps2->zmin)   &&    (ps2->xmax >= ps->xmin) && (ps2->ymax >= ps->ymin) && (ps2->zmax >= ps->zmin) )
	 * 3/  !( ( (ps->xmax >= ps2->xmin) && (ps->ymax >= ps2->ymin) && (ps->zmax >= ps2->zmin) ) &&  ( (ps2->xmax >= ps->xmin) && (ps2->ymax >= ps->ymin) && (ps2->zmax >= ps->zmin) ) )
	 * 4/ !( !( (ps->xmax <  ps2->xmin) || (ps->ymax <  ps2->ymin) || (ps->zmax <  ps2->zmin) ) && !( (ps2->xmax <  ps->xmin) || (ps2->ymax <  ps->ymin) || (ps2->zmax <  ps->zmin) ) )
	 * 5/ PTEST <---------------------------------- rslt1 ---------------------------------->         <------------------------------ rslt2 -------------------------------------->
	 */
	__m128i ps1_max = LOAD_128((const __m128i*) &ps1->xmax);
	__m128i ps2_min = LOAD_128((const __m128i*) &ps2->xmin);
	__m128i rslt1 = _mm_cmplt_epi32 (ps1_max, ps2_min);
	if (!_mm_testz_si128 (this->mask_ptest, rslt1)) return true;

	__m128i ps1_min = LOAD_128((const __m128i*) &ps1->xmin);
	__m128i ps2_max = LOAD_128((const __m128i*) &ps2->xmax);
	__m128i rslt2 = _mm_cmplt_epi32 (ps2_max, ps1_min);
	if (!_mm_testz_si128 (this->mask_ptest, rslt2)) return false;

	/* Use X+Y+Z as the sorting order, so sprites closer to the bottom of
	 * the screen and with higher Z elevation, are drawn in front. Here
	 * X,Y,Z are the coordinates of the "center of mass" of the sprite,
	 * i.e. X=(left+right)/2, etc. However, since we only care about
	 * order, don't actually divide / 2. */
	return  ps1->xmin + ps1->xmax + ps1->ymin + ps1->ymax + ps1->zmin + ps1->zmax <=
		ps2->xmin + ps2->xmax + ps2->ymin + ps2->ymax + ps2->zmin + ps2->zmax;
}

/** Sort parent sprites pointer array using SSE4.1 optimizations. */
void ViewportSortParentSpritesSSE41 (ParentSpriteToDraw **psd,
	const ParentSpriteToDraw *const *psdvend)
{
	CompareParentSpritesSSE41 comparator;
	SortParentSprites (comparator, psd, psdvend);
}

#endif /* WITH_SSE */
