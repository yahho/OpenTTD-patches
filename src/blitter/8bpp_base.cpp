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

void Blitter_8bppBase::Surface::scroll (int left, int top, int width, int height, int dx, int dy)
{
	this->Blitter::Surface::scroll<uint8> (this->ptr, left, top, width, height, dx, dy);
}
