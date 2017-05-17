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

void Blitter_32bppBase::Surface::scroll (int left, int top, int width, int height, int dx, int dy)
{
	this->Blitter::Surface::scroll<uint32> (this->ptr, left, top, width, height, dx, dy);
}
