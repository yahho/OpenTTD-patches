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

#include "32bpp_base.hpp"

/** The most trivial 32 bpp blitter (without palette animation). */
class Blitter_32bppSimple : public Blitter_32bppBase {
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

	/* virtual */ void Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom);
	/* virtual */ void DrawColourMappingRect(void *dst, int width, int height, PaletteID pal);
	/* virtual */ ::Sprite *Encode (const SpriteLoader::Sprite *sprite, bool is_font, AllocatorProc *allocator);
};

#endif /* BLITTER_32BPP_SIMPLE_HPP */
