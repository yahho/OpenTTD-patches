/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_noanim.hpp 32bpp blitter without animation support. */

#ifndef BLITTER_32BPP_NOANIM_HPP
#define BLITTER_32BPP_NOANIM_HPP

#include "32bpp_base.hpp"
#include "../gfx_func.h"

/** Base for 32bpp blitters without animation. */
class Blitter_32bppNoanim : public Blitter_32bppBase {
public:
	static const PaletteAnimation palette_animation = PALETTE_ANIMATION_NONE; ///< Palette animation.

	/**
	 * Look up the colour in the current palette.
	 */
	static inline Colour LookupColourInPalette (uint index)
	{
		return _cur_palette[index];
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppBase::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_32bppBase::Surface (ptr, width, height, pitch)
		{
		}

		void set_pixel (void *video, int x, int y, uint8 colour) OVERRIDE;

		void draw_rect (void *video, int width, int height, uint8 colour) OVERRIDE;

		void recolour_rect (void *video, int width, int height, PaletteID pal) OVERRIDE;

		void draw_checker (void *video, uint width, uint height, uint8 colour, byte bo) OVERRIDE;

		void copy (Buffer *dst, int x, int y, uint width, uint height) OVERRIDE;

		void paste (const Buffer *src, int x, int y) OVERRIDE;
	};
};

#endif /* BLITTER_32BPP_NOANIM_HPP */
