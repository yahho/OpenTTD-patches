/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 8bpp_optimized.hpp An optimized 8 bpp blitter. */

#ifndef BLITTER_8BPP_OPTIMIZED_HPP
#define BLITTER_8BPP_OPTIMIZED_HPP

#include "8bpp_base.hpp"

/** 8bpp blitter optimised for speed. */
class Blitter_8bppOptimized FINAL : public Blitter_8bppBase {
public:
	/** Data stored about a (single) sprite. */
	struct Sprite : ::Sprite {
		uint32 offset[ZOOM_LVL_COUNT]; ///< Offsets (from .data) to streams for different zoom levels.
		byte data[];                   ///< Data, all zoomlevels.
	};

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	::Sprite *Encode (const RawSprite *sprite, bool is_font, AllocatorProc *allocator) OVERRIDE;

	/** Blitting surface. */
	struct Surface : Blitter_8bppBase::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_8bppBase::Surface (ptr, width, height, pitch)
		{
		}

		void draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) OVERRIDE;
	};

	/** Create a surface for this blitter. */
	Surface *create (void *ptr, uint width, uint height, uint pitch, bool anim) OVERRIDE
	{
		return new Surface (ptr, width, height, pitch);
	}
};

#endif /* BLITTER_8BPP_OPTIMIZED_HPP */
