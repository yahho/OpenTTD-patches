/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_base.cpp Implementation of base for 32 bpp blitters. */

#include "../stdafx.h"
#include "32bpp_base.hpp"

void *Blitter_32bppBase::Surface::move (void *video, int x, int y)
{
	return this->movep <uint32> (video, x, y);
}

void Blitter_32bppBase::Surface::export_lines (void *dst, uint dst_pitch, uint y, uint height)
{
	uint32 *udst = (uint32 *)dst;
	const uint32 *src = this->movep <const uint32> (this->ptr, 0, y);

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint32));
		src += this->pitch;
		udst += dst_pitch;
	}
}

void Blitter_32bppBase::Surface::scroll (int left, int top, int width, int height, int scroll_x, int scroll_y)
{
	const uint32 *src;
	uint32 *dst;

	if (scroll_y > 0) {
		/* Calculate pointers */
		dst = this->movep <uint32> (this->ptr, left, top + height - 1);
		src = dst - scroll_y * this->pitch;

		/* Decrease height and increase top */
		top += scroll_y;
		height -= scroll_y;
		assert(height > 0);

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst += scroll_x;
			left += scroll_x;
			width -= scroll_x;
		} else {
			src -= scroll_x;
			width += scroll_x;
		}

		for (int h = height; h > 0; h--) {
			memcpy(dst, src, width * sizeof(uint32));
			src -= this->pitch;
			dst -= this->pitch;
		}
	} else {
		/* Calculate pointers */
		dst = this->movep <uint32> (this->ptr, left, top);
		src = dst + (-scroll_y) * this->pitch;

		/* Decrease height. (scroll_y is <=0). */
		height += scroll_y;
		assert(height > 0);

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst += scroll_x;
			left += scroll_x;
			width -= scroll_x;
		} else {
			src -= scroll_x;
			width += scroll_x;
		}

		/* the y-displacement may be 0 therefore we have to use memmove,
		 * because source and destination may overlap */
		for (int h = height; h > 0; h--) {
			memmove(dst, src, width * sizeof(uint32));
			src += this->pitch;
			dst += this->pitch;
		}
	}
}
