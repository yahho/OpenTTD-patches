/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_anim.hpp A 32 bpp blitter with animation support. */

#ifndef BLITTER_32BPP_ANIM_HPP
#define BLITTER_32BPP_ANIM_HPP

#include "32bpp_optimized.hpp"
#include "../core/flexarray.h"

/** Base for 32bpp blitters with palette animation. */
class Blitter_32bppAnimBase : public Blitter_32bppBase {
public:
	Blitter::PaletteAnimation UsePaletteAnimation (void) OVERRIDE
	{
		return Blitter::PALETTE_ANIMATION_BLITTER;
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppBase::Surface {
		Colour palette[256];    ///< The current palette.
		uint16 *const anim_buf; ///< In this buffer we keep track of the 8bpp indexes so we can do palette animation

		void set_palette (const Colour (&palette) [256])
		{
			assert_compile (sizeof(this->palette) == sizeof(palette));
			memcpy (this->palette, palette, sizeof(this->palette));
		}

		Surface (void *ptr, uint width, uint height, uint pitch, uint16 *anim_buf)
			: Blitter_32bppBase::Surface (ptr, width, height, pitch),
			  anim_buf (anim_buf)
		{
			this->set_palette (_cur_palette);
			memset (anim_buf, 0, width * height * sizeof(uint16));
		}

		/** Get the animation pointer for a given video pointer. */
		uint16 *get_anim_pos (const void *dst)
		{
			return this->anim_buf + ((const uint32 *)dst - (uint32 *)this->ptr);
		}

		/** Look up the colour in the current palette. */
		Colour lookup_colour (uint index) const
		{
			return this->palette[index];
		}

		void set_pixel (void *video, int x, int y, uint8 colour) OVERRIDE;

		void draw_rect (void *video, int width, int height, uint8 colour) OVERRIDE;

		void recolour_rect (void *video, int width, int height, PaletteID pal) OVERRIDE;

		void draw_checker (void *video, uint width, uint height, uint8 colour, byte bo) OVERRIDE;

		void scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y) OVERRIDE;

		bool palette_animate (const Colour (&palette) [256]) OVERRIDE;

		void copy (Buffer *dst, int x, int y, uint width, uint height) OVERRIDE;

		void paste (const Buffer *src, int x, int y) OVERRIDE;
	};
};

/** The optimised 32 bpp blitter with palette animation. */
class Blitter_32bppAnim : public Blitter_32bppAnimBase {
public:
	typedef Blitter_32bppOptimized::Sprite Sprite;

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	::Sprite *Encode (const RawSprite *sprite, bool is_font, AllocatorProc *allocator) OVERRIDE
	{
		return Sprite::encode (sprite, is_font, allocator);
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppAnimBase::Surface, FlexArray<uint16> {
		uint16 anim_buf[]; ///< In this buffer we keep track of the 8bpp indexes so we can do palette animation

	private:
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_32bppAnimBase::Surface (ptr, width, height, pitch, this->anim_buf)
		{
		}

	public:
		static Surface *create (void *ptr, uint width, uint height, uint pitch)
		{
			return new (width, height) Surface (ptr, width, height, pitch);
		}

		template <BlitterMode mode> void draw (const BlitterParams *bp, ZoomLevel zoom);

		void draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) OVERRIDE;
	};

	/** Create a surface for this blitter. */
	Blitter_32bppBase::Surface *create (void *ptr, uint width, uint height, uint pitch, bool anim) OVERRIDE
	{
		if (anim) {
			return Surface::create (ptr, width, height, pitch);
		} else {
			return new Blitter_32bppOptimized::Surface (ptr, width, height, pitch);
		}
	}
};

#endif /* BLITTER_32BPP_ANIM_HPP */
