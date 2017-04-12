/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 8bpp_base.hpp Base for all 8 bpp blitters. */

#ifndef BLITTER_8BPP_BASE_HPP
#define BLITTER_8BPP_BASE_HPP

#include "blitter.h"

/** Base for all 8bpp blitters. */
class Blitter_8bppBase : public Blitter {
public:
	static const uint screen_depth = 8; ///< Screen depth.
	static const PaletteAnimation palette_animation = PALETTE_ANIMATION_VIDEO_BACKEND; ///< Palette animation.

	/** Blitting surface. */
	struct Surface : Blitter::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter::Surface (ptr, width, height, pitch)
		{
		}

		void *move (void *video, int x, int y) OVERRIDE;

		void set_pixel (void *video, int x, int y, uint8 colour) OVERRIDE;

		void draw_rect (void *video, int width, int height, uint8 colour) OVERRIDE;

		void recolour_rect (void *video, int width, int height, PaletteID pal) OVERRIDE;

		void draw_checker (void *video, uint width, uint height, uint8 colour, byte bo) OVERRIDE;

		void scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y) OVERRIDE;

		void copy (Buffer *dst, int x, int y, uint width, uint height) OVERRIDE;

		void paste (const Buffer *src, int x, int y) OVERRIDE;

		void export_lines (void *dst, uint dst_pitch, uint y, uint height) OVERRIDE;
	};
};

#endif /* BLITTER_8BPP_BASE_HPP */
