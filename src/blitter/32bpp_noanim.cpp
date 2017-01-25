/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_noanim.cpp 32bpp blitter without animation support. */

#include "../stdafx.h"
#include "../debug.h"
#include "32bpp_noanim.hpp"

#include "../table/sprites.h"

void Blitter_32bppNoanim::Surface::set_pixel (void *video, int x, int y, uint8 colour)
{
	*(this->movep <Colour> (video, x, y)) = LookupColourInPalette (colour);
}

void Blitter_32bppNoanim::Surface::draw_rect (void *video, int width, int height, uint8 colour)
{
	Colour colour32 = LookupColourInPalette (colour);

	do {
		Colour *dst = (Colour *)video;
		for (int i = width; i > 0; i--) {
			*dst = colour32;
			dst++;
		}
		video = (uint32 *)video + this->pitch;
	} while (--height);
}

void Blitter_32bppNoanim::Surface::recolour_rect (void *dst, int width, int height, PaletteID pal)
{
	Colour *udst = (Colour *)dst;

	if (pal == PALETTE_TO_TRANSPARENT) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeTransparent (*udst, 154);
				udst++;
			}
			udst = udst - width + this->pitch;
		} while (--height);
		return;
	}
	if (pal == PALETTE_NEWSPAPER) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeGrey (*udst);
				udst++;
			}
			udst = udst - width + this->pitch;
		} while (--height);
		return;
	}

	DEBUG(misc, 0, "32bpp blitter doesn't know how to draw this colour table ('%d')", pal);
}

void Blitter_32bppNoanim::Surface::draw_checker (void *video, uint width, uint height, uint8 colour, byte bo)
{
	Colour colour32 = LookupColourInPalette (colour);
	Colour *dst = (Colour *) video;
	uint i = bo;
	do {
		for (i = !(i & 1); i < width; i += 2) dst[i] = colour32;
		dst += this->pitch;
	} while (--height > 0);
}

void Blitter_32bppNoanim::Surface::copy (Buffer *dst, int x, int y, uint width, uint height)
{
	dst->resize (width, height, sizeof(uint32));
	/* Only change buffer capacity? */
	if ((x < 0) || (y < 0)) return;

	dst->width  = width;
	dst->height = height;

	byte *udst = &dst->data.front();
	const uint32 *src = (const uint32 *) this->Blitter_32bppNoanim::Surface::move (this->ptr, x, y);

	for (; height > 0; height--) {
		memcpy (udst, src, width * sizeof(uint32));
		udst += width * sizeof(uint32);
		src += this->pitch;
	}

	/* Sanity check that we did not overrun the buffer. */
	assert (udst <= &dst->data.front() + dst->data.size());
}

void Blitter_32bppNoanim::Surface::paste (const Buffer *src, int x, int y)
{
	uint32 *dst = (uint32 *) this->Blitter_32bppNoanim::Surface::move (this->ptr, x, y);
	const byte *usrc = &src->data.front();

	const uint width = src->width;
	for (uint height = src->height; height > 0; height--) {
		memcpy (dst, usrc, width * sizeof(uint32));
		usrc += width * sizeof(uint32);
		dst += this->pitch;
	}
}
