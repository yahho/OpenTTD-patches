/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport_sprite_sorter.h Types related to sprite sorting. */

#include "stdafx.h"
#include "core/smallvec_type.hpp"
#include "gfx_type.h"

#ifndef VIEWPORT_SPRITE_SORTER_H
#define VIEWPORT_SPRITE_SORTER_H

/** Parent sprite that should be drawn */
struct ParentSpriteToDraw {
	/* Block of 16B loadable in xmm register */
	int32 xmin;                     ///< minimal world X coordinate of bounding box
	int32 ymin;                     ///< minimal world Y coordinate of bounding box
	int32 zmin;                     ///< minimal world Z coordinate of bounding box
	int32 x;                        ///< screen X coordinate of sprite

	/* Second block of 16B loadable in xmm register */
	int32 xmax;                     ///< maximal world X coordinate of bounding box
	int32 ymax;                     ///< maximal world Y coordinate of bounding box
	int32 zmax;                     ///< maximal world Z coordinate of bounding box
	int32 y;                        ///< screen Y coordinate of sprite

	SpriteID image;                 ///< sprite to draw
	PaletteID pal;                  ///< palette to use
	const SubSprite *sub;           ///< only draw a rectangular part of the sprite

	int32 left;                     ///< minimal screen X coordinate of sprite (= x + sprite->x_offs), reference point for child sprites
	int32 top;                      ///< minimal screen Y coordinate of sprite (= y + sprite->y_offs), reference point for child sprites

	int32 first_child;              ///< the first child to draw.
	bool comparison_done;           ///< Used during sprite sorting: true if sprite has been compared with all other sprites
};

template <typename T>
static inline void SortParentSprites (const T &comparator,
	ParentSpriteToDraw **psd, const ParentSpriteToDraw *const *psdvend)
{
	while (psd != psdvend) {
		ParentSpriteToDraw *const ps = *psd;

		if (ps->comparison_done) {
			psd++;
			continue;
		}

		ps->comparison_done = true;

		for (ParentSpriteToDraw **psd2 = psd + 1; psd2 != psdvend; psd2++) {
			ParentSpriteToDraw *const ps2 = *psd2;

			if (ps2->comparison_done) continue;

			if (comparator (ps, ps2)) continue;

			/* Move ps2 in front of ps */
			for (ParentSpriteToDraw **psd3 = psd2; psd3 > psd; psd3--) {
				*psd3 = *(psd3 - 1);
			}
			*psd = ps2;
		}
	}
}

#ifdef WITH_SSE
void ViewportSortParentSpritesSSE41 (ParentSpriteToDraw **psd, const ParentSpriteToDraw *const *psdvend);
#endif

#endif /* VIEWPORT_SPRITE_SORTER_H */
