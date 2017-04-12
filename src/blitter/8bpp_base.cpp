/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 8bpp_base.cpp Implementation of the base for all 8 bpp blitters. */

#include "../stdafx.h"
#include "../gfx_func.h"
#include "../spritecache.h"
#include "8bpp_base.hpp"

void Blitter_8bppBase::Surface::recolour_rect (void *dst, int width, int height, PaletteID pal)
{
	const uint8 *ctab = GetNonSprite(pal, ST_RECOLOUR) + 1;

	do {
		for (int i = 0; i != width; i++) *((uint8 *)dst + i) = ctab[((uint8 *)dst)[i]];
		dst = (uint8 *)dst + this->pitch;
	} while (--height);
}

void *Blitter_8bppBase::Surface::move (void *video, int x, int y)
{
	return this->movep <uint8> (video, x, y);
}

void Blitter_8bppBase::Surface::set_pixel (void *video, int x, int y, uint8 colour)
{
	*(this->movep <uint8> (video, x, y)) = colour;
}

void Blitter_8bppBase::Surface::draw_rect (void *video, int width, int height, uint8 colour)
{
	do {
		memset(video, colour, width);
		video = (uint8 *)video + this->pitch;
	} while (--height);
}

void Blitter_8bppBase::Surface::draw_checker (void *video, uint width, uint height, uint8 colour, byte bo)
{
	uint8 *dst = (uint8 *) video;
	uint i = bo;
	do {
		for (i = !(i & 1); i < width; i += 2) dst[i] = colour;
		dst += this->pitch;
	} while (--height > 0);
}

void Blitter_8bppBase::Surface::paste (const Buffer *src, int x, int y)
{
	uint8 *dst = this->movep <uint8> (this->ptr, x, y);
	const byte *usrc = &src->data.front();

	const uint width = src->width;
	for (uint height = src->height; height > 0; height--) {
		memcpy(dst, usrc, width * sizeof(uint8));
		usrc += width;
		dst += this->pitch;
	}
}

void Blitter_8bppBase::Surface::copy (Buffer *dst, int x, int y, uint width, uint height)
{
	dst->resize (width, height, sizeof(uint8));
	/* Only change buffer capacity? */
	if ((x < 0) || (y < 0)) return;

	dst->width  = width;
	dst->height = height;

	byte *udst = &dst->data.front();
	const uint8 *src = this->movep <const uint8> (this->ptr, x, y);

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint8));
		src += this->pitch;
		udst += width;
	}

	/* Sanity check that we did not overrun the buffer. */
	assert (udst <= &dst->data.front() + dst->data.size());
}

void Blitter_8bppBase::Surface::export_lines (void *dst, uint dst_pitch, uint y, uint height)
{
	uint8 *udst = (uint8 *)dst;
	const uint8 *src = this->movep <const uint8> (this->ptr, 0, y);

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint8));
		src += this->pitch;
		udst += dst_pitch;
	}
}

void Blitter_8bppBase::Surface::scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y)
{
	const uint8 *src;
	uint8 *dst;

	if (scroll_y > 0) {
		/* Calculate pointers */
		dst = this->movep <uint8> (video, left, top + height - 1);
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
			memcpy(dst, src, width * sizeof(uint8));
			src -= this->pitch;
			dst -= this->pitch;
		}
	} else {
		/* Calculate pointers */
		dst = this->movep <uint8> (video, left, top);
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
			memmove(dst, src, width * sizeof(uint8));
			src += this->pitch;
			dst += this->pitch;
		}
	}
}
