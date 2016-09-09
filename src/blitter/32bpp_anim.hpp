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
	int BufferSize (int width, int height) OVERRIDE
	{
		return width * height * (sizeof(uint32) + sizeof(uint16));
	}

	Blitter::PaletteAnimation UsePaletteAnimation (void) OVERRIDE
	{
		return Blitter::PALETTE_ANIMATION_BLITTER;
	}

	int GetBytesPerPixel (void) OVERRIDE
	{
		return 6;
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppBase::Surface {
		Palette palette;     ///< The current palette.
		uint16 *const anim_buf; ///< In this buffer we keep track of the 8bpp indexes so we can do palette animation

		Surface (void *ptr, uint width, uint height, uint pitch, uint16 *anim_buf)
			: Blitter_32bppBase::Surface (ptr, width, height, pitch),
			  anim_buf (anim_buf)
		{
			memset (&this->palette, 0, sizeof(this->palette));
			memset (anim_buf, 0, width * height * sizeof(uint16));
		}

		/** Look up the colour in the current palette. */
		Colour lookup_colour (uint index) const
		{
			return this->palette.palette[index];
		}

		void set_pixel (void *video, int x, int y, uint8 colour) OVERRIDE;

		void draw_rect (void *video, int width, int height, uint8 colour) OVERRIDE;

		void recolour_rect (void *video, int width, int height, PaletteID pal) OVERRIDE;

		void scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y) OVERRIDE;

		bool palette_animate (const Palette &palette) OVERRIDE;

		void copy (void *dst, int x, int y, int width, int height) OVERRIDE;

		void paste (const void *src, int x, int y, int width, int height) OVERRIDE;
	};
};

/** The optimised 32 bpp blitter with palette animation. */
class Blitter_32bppAnim : public Blitter_32bppAnimBase {
public:
	typedef Blitter_32bppOptimized::Sprite Sprite;

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	::Sprite *Encode (const SpriteLoader::Sprite *sprite, bool is_font, AllocatorProc *allocator) OVERRIDE
	{
		return Sprite::encode (sprite, is_font, allocator);
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppAnimBase::Surface, FlexArrayBase {
		uint16 anim_buf[]; ///< In this buffer we keep track of the 8bpp indexes so we can do palette animation

	private:
		void *operator new (size_t size, uint width, uint height)
		{
			size_t extra = width * height * sizeof(uint16);
			return ::operator new (size + extra);
		}

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
