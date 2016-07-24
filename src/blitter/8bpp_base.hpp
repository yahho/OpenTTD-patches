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
	/* virtual */ uint8 GetScreenDepth() { return 8; }
	/* virtual */ void CopyFromBuffer(void *video, const void *src, int width, int height);
	/* virtual */ void CopyImageToBuffer(const void *video, void *dst, int width, int height, int dst_pitch);
	/* virtual */ int BufferSize(int width, int height);
	/* virtual */ Blitter::PaletteAnimation UsePaletteAnimation();
	/* virtual */ int GetBytesPerPixel() { return 1; }

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

		void scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y) OVERRIDE;

		void copy (void *dst, int x, int y, int width, int height) OVERRIDE;
	};

	/** Create a surface for this blitter. */
	Surface *create (void *ptr, uint width, uint height, uint pitch) OVERRIDE
	{
		return new Surface (ptr, width, height, pitch);
	}
};

#endif /* BLITTER_8BPP_BASE_HPP */
