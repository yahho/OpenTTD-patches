/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file null.hpp The blitter that doesn't blit. */

#ifndef BLITTER_NULL_HPP
#define BLITTER_NULL_HPP

#include "blitter.h"

/** Blitter that does nothing. */
class Blitter_Null : public Blitter {
public:
	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	/* virtual */ uint8 GetScreenDepth() { return 0; }
	/* virtual */ Sprite *Encode (const RawSprite *sprite, bool is_font, AllocatorProc *allocator);
	/* virtual */ Blitter::PaletteAnimation UsePaletteAnimation() { return Blitter::PALETTE_ANIMATION_NONE; };

	/** Blitting surface. */
	struct Surface : Blitter::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter::Surface (ptr, width, height, pitch)
		{
		}

		void *move (void *video, int x, int y) OVERRIDE
		{
			return NULL;
		}

		void set_pixel (void *video, int x, int y, uint8 colour) OVERRIDE
		{
		}

		void draw_line (void *video, int x, int y, int x2, int y2, int screen_width, int screen_height, uint8 colour, int width, int dash = 0) OVERRIDE
		{
		}

		void draw_rect (void *video, int width, int height, uint8 colour) OVERRIDE
		{
		}

		void recolour_rect (void *video, int width, int height, PaletteID pal) OVERRIDE
		{
		}

		void draw_checker (void *video, uint width, uint height, uint8 colour, byte bo) OVERRIDE
		{
		}

		void scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y) OVERRIDE
		{
		}

		void draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) OVERRIDE
		{
		}

		void copy (Buffer *dst, int x, int y, uint width, uint height) OVERRIDE
		{
		}

		void paste (const Buffer *src, int x, int y) OVERRIDE
		{
		}

		void export_lines (void *dst, uint dst_pitch, uint y, uint height) OVERRIDE
		{
		}
	};

	/** Create a surface for this blitter. */
	Surface *create (void *ptr, uint width, uint height, uint pitch, bool anim) OVERRIDE
	{
		return new Surface (ptr, width, height, pitch);
	}
};

#endif /* BLITTER_NULL_HPP */
