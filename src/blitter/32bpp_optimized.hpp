/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_optimized.hpp Optimized 32 bpp blitter. */

#ifndef BLITTER_32BPP_OPTIMIZED_HPP
#define BLITTER_32BPP_OPTIMIZED_HPP

#include "32bpp_simple.hpp"

/** The optimised 32 bpp blitter (without palette animation). */
class Blitter_32bppOptimized : public Blitter_32bppSimple {
public:
	/** Data stored about a (single) sprite. */
	struct Sprite : ::Sprite {
		uint32 offset[ZOOM_LVL_COUNT][2]; ///< Offsets (from .data) to streams for different zoom levels, and the normal and remap image information.
		byte data[];                      ///< Data, all zoomlevels.
	};

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	/* virtual */ ::Sprite *Encode (const SpriteLoader::Sprite *sprite, bool is_font, AllocatorProc *allocator);

	/** Blitting surface. */
	struct Surface : Blitter_32bppSimple::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_32bppSimple::Surface (ptr, width, height, pitch)
		{
		}

		template <BlitterMode mode> void draw (const BlitterParams *bp, ZoomLevel zoom);

		void draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) OVERRIDE;
	};

	/** Create a surface for this blitter. */
	Blitter_32bppSimple::Surface *create (void *ptr, uint width, uint height, uint pitch, bool anim) OVERRIDE
	{
		return new Surface (ptr, width, height, pitch);
	}
};

#endif /* BLITTER_32BPP_OPTIMIZED_HPP */
