/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_simple.hpp Simple 32 bpp blitter. */

#ifndef BLITTER_32BPP_SIMPLE_HPP
#define BLITTER_32BPP_SIMPLE_HPP

#include "32bpp_noanim.hpp"

/** The most trivial 32 bpp blitter (without palette animation). */
class Blitter_32bppSimple : public Blitter_32bppNoanim {
	struct Pixel {
		uint8 r;  ///< Red-channel
		uint8 g;  ///< Green-channel
		uint8 b;  ///< Blue-channel
		uint8 a;  ///< Alpha-channel
		uint8 m;  ///< Remap-channel
		uint8 v;  ///< Brightness-channel
	};
public:
	/** Data structure describing a sprite. */
	struct Sprite : ::Sprite {
		Pixel data[];  ///< Sprite data
	};

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	::Sprite *Encode (const RawSprite *sprite, bool is_font, AllocatorProc *allocator) OVERRIDE;

	/** Blitting surface. */
	struct Surface : Blitter_32bppNoanim::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_32bppNoanim::Surface (ptr, width, height, pitch)
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

#endif /* BLITTER_32BPP_SIMPLE_HPP */
