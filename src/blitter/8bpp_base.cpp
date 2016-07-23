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
	return (uint8 *)video + x + y * this->pitch;
}

void Blitter_8bppBase::Surface::set_pixel (void *video, int x, int y, uint8 colour)
{
	*((uint8 *)video + x + y * this->pitch) = colour;
}

void Blitter_8bppBase::Surface::draw_rect (void *video, int width, int height, uint8 colour)
{
	do {
		memset(video, colour, width);
		video = (uint8 *)video + this->pitch;
	} while (--height);
}

void Blitter_8bppBase::CopyFromBuffer(void *video, const void *src, int width, int height)
{
	uint8 *dst = (uint8 *)video;
	const uint8 *usrc = (const uint8 *)src;

	for (; height > 0; height--) {
		memcpy(dst, usrc, width * sizeof(uint8));
		usrc += width;
		dst += _screen.surface->pitch;
	}
}

void Blitter_8bppBase::CopyToBuffer(const void *video, void *dst, int width, int height)
{
	uint8 *udst = (uint8 *)dst;
	const uint8 *src = (const uint8 *)video;

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint8));
		src += _screen.surface->pitch;
		udst += width;
	}
}

void Blitter_8bppBase::CopyImageToBuffer(const void *video, void *dst, int width, int height, int dst_pitch)
{
	uint8 *udst = (uint8 *)dst;
	const uint8 *src = (const uint8 *)video;

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint8));
		src += _screen.surface->pitch;
		udst += dst_pitch;
	}
}

void Blitter_8bppBase::Surface::scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y)
{
	const uint8 *src;
	uint8 *dst;

	if (scroll_y > 0) {
		/* Calculate pointers */
		dst = (uint8 *)video + left + (top + height - 1) * this->pitch;
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
		dst = (uint8 *)video + left + top * this->pitch;
		src = dst - scroll_y * this->pitch;

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

int Blitter_8bppBase::BufferSize(int width, int height)
{
	return width * height;
}

Blitter::PaletteAnimation Blitter_8bppBase::UsePaletteAnimation()
{
	return Blitter::PALETTE_ANIMATION_VIDEO_BACKEND;
}
