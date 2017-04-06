/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_anim.cpp Implementation of the optimized 32 bpp blitter with animation support. */

#include "../stdafx.h"
#include "../debug.h"
#include "32bpp_anim.hpp"

#include "../table/sprites.h"

const char Blitter_32bppAnim::name[] = "32bpp-anim";
const char Blitter_32bppAnim::desc[] = "32bpp Animation Blitter (palette animation)";

template <BlitterMode mode>
inline void Blitter_32bppAnim::Surface::draw (const BlitterParams *bp, ZoomLevel zoom)
{
	const Sprite *src = static_cast<const Sprite*> (bp->sprite);

	const Colour *src_px = (const Colour *)(src->data + src->offset[zoom][0]);
	const uint16 *src_n  = (const uint16 *)(src->data + src->offset[zoom][1]);

	for (uint i = bp->skip_top; i != 0; i--) {
		src_px = (const Colour *)((const byte *)src_px + *(const uint32 *)src_px);
		src_n  = (const uint16 *)((const byte *)src_n  + *(const uint32 *)src_n);
	}

	Colour *dst = (Colour *)bp->dst + bp->top * bp->pitch + bp->left;
	uint16 *anim = this->get_anim_pos (bp->dst) + bp->top * this->width + bp->left;

	const byte *remap = bp->remap; // store so we don't have to access it via bp everytime

	for (int y = 0; y < bp->height; y++) {
		Colour *dst_ln = dst + bp->pitch;
		uint16 *anim_ln = anim + this->width;

		const Colour *src_px_ln = (const Colour *)((const byte *)src_px + *(const uint32 *)src_px);
		src_px++;

		const uint16 *src_n_ln = (const uint16 *)((const byte *)src_n + *(const uint32 *)src_n);
		src_n += 2;

		Colour *dst_end = dst + bp->skip_left;

		uint n;

		while (dst < dst_end) {
			n = *src_n++;

			if (src_px->a == 0) {
				dst += n;
				src_px ++;
				src_n++;

				if (dst > dst_end) anim += dst - dst_end;
			} else {
				if (dst + n > dst_end) {
					uint d = dst_end - dst;
					src_px += d;
					src_n += d;

					dst = dst_end - bp->skip_left;
					dst_end = dst + bp->width;

					n = min<uint>(n - d, (uint)bp->width);
					goto draw;
				}
				dst += n;
				src_px += n;
				src_n += n;
			}
		}

		dst -= bp->skip_left;
		dst_end -= bp->skip_left;

		dst_end += bp->width;

		while (dst < dst_end) {
			n = min<uint>(*src_n++, (uint)(dst_end - dst));

			if (src_px->a == 0) {
				anim += n;
				dst += n;
				src_px++;
				src_n++;
				continue;
			}

			draw:;

			switch (mode) {
				case BM_COLOUR_REMAP:
					if (src_px->a == 255) {
						do {
							uint m = *src_n;
							/* In case the m-channel is zero, do not remap this pixel in any way */
							if (m == 0) {
								*dst = src_px->data;
								*anim = 0;
							} else {
								uint r = remap[GB(m, 0, 8)];
								*anim = r | (m & 0xFF00);
								if (r != 0) *dst = AdjustBrightness (this->lookup_colour(r), GB(m, 8, 8));
							}
							anim++;
							dst++;
							src_px++;
							src_n++;
						} while (--n != 0);
					} else {
						do {
							uint m = *src_n;
							if (m == 0) {
								*dst = ComposeColourRGBANoCheck(src_px->r, src_px->g, src_px->b, src_px->a, *dst);
								*anim = 0;
							} else {
								uint r = remap[GB(m, 0, 8)];
								*anim = 0;
								if (r != 0) *dst = ComposeColourPANoCheck (AdjustBrightness (this->lookup_colour(r), GB(m, 8, 8)), src_px->a, *dst);
							}
							anim++;
							dst++;
							src_px++;
							src_n++;
						} while (--n != 0);
					}
					break;

				case BM_CRASH_REMAP:
					if (src_px->a == 255) {
						do {
							uint m = *src_n;
							if (m == 0) {
								uint8 g = MakeDark(src_px->r, src_px->g, src_px->b);
								*dst = ComposeColourRGBA(g, g, g, src_px->a, *dst);
								*anim = 0;
							} else {
								uint r = remap[GB(m, 0, 8)];
								*anim = r | (m & 0xFF00);
								if (r != 0) *dst = AdjustBrightness (this->lookup_colour(r), GB(m, 8, 8));
							}
							anim++;
							dst++;
							src_px++;
							src_n++;
						} while (--n != 0);
					} else {
						do {
							uint m = *src_n;
							if (m == 0) {
								if (src_px->a != 0) {
									uint8 g = MakeDark(src_px->r, src_px->g, src_px->b);
									*dst = ComposeColourRGBA(g, g, g, src_px->a, *dst);
									*anim = 0;
								}
							} else {
								uint r = remap[GB(m, 0, 8)];
								*anim = 0;
								if (r != 0) *dst = ComposeColourPANoCheck (AdjustBrightness (this->lookup_colour(r), GB(m, 8, 8)), src_px->a, *dst);
							}
							anim++;
							dst++;
							src_px++;
							src_n++;
						} while (--n != 0);
					}
					break;


				case BM_BLACK_REMAP:
					do {
						*dst++ = Colour(0, 0, 0);
						*anim++ = 0;
						src_px++;
						src_n++;
					} while (--n != 0);
					break;

				case BM_TRANSPARENT:
					/* TODO -- We make an assumption here that the remap in fact is transparency, not some colour.
					 *  This is never a problem with the code we produce, but newgrfs can make it fail... or at least:
					 *  we produce a result the newgrf maker didn't expect ;) */

					/* Make the current colour a bit more black, so it looks like this image is transparent */
					src_n += n;
					if (src_px->a == 255) {
						src_px += n;
						do {
							*dst = MakeTransparent(*dst, 3, 4);
							*anim = 0;
							anim++;
							dst++;
						} while (--n != 0);
					} else {
						do {
							*dst = MakeTransparent(*dst, (256 * 4 - src_px->a), 256 * 4);
							*anim = 0;
							anim++;
							dst++;
							src_px++;
						} while (--n != 0);
					}
					break;

				default:
					if (src_px->a == 255) {
						do {
							/* Compiler assumes pointer aliasing, can't optimise this on its own */
							uint m = GB(*src_n, 0, 8);
							/* Above PALETTE_ANIM_START is palette animation */
							*anim++ = *src_n;
							*dst++ = (m >= PALETTE_ANIM_START) ? AdjustBrightness (this->lookup_colour(m), GB(*src_n, 8, 8)) : src_px->data;
							src_px++;
							src_n++;
						} while (--n != 0);
					} else {
						do {
							uint m = GB(*src_n, 0, 8);
							*anim++ = 0;
							if (m >= PALETTE_ANIM_START) {
								*dst = ComposeColourPANoCheck (AdjustBrightness (this->lookup_colour(m), GB(*src_n, 8, 8)), src_px->a, *dst);
							} else {
								*dst = ComposeColourRGBANoCheck(src_px->r, src_px->g, src_px->b, src_px->a, *dst);
							}
							dst++;
							src_px++;
							src_n++;
						} while (--n != 0);
					}
					break;
			}
		}

		anim = anim_ln;
		dst = dst_ln;
		src_px = src_px_ln;
		src_n  = src_n_ln;
	}
}

void Blitter_32bppAnim::Surface::draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom)
{
	switch (mode) {
		default: NOT_REACHED();
		case BM_NORMAL:       this->draw<BM_NORMAL>      (bp, zoom); return;
		case BM_COLOUR_REMAP: this->draw<BM_COLOUR_REMAP>(bp, zoom); return;
		case BM_TRANSPARENT:  this->draw<BM_TRANSPARENT> (bp, zoom); return;
		case BM_CRASH_REMAP:  this->draw<BM_CRASH_REMAP> (bp, zoom); return;
		case BM_BLACK_REMAP:  this->draw<BM_BLACK_REMAP> (bp, zoom); return;
	}
}

void Blitter_32bppAnimBase::Surface::recolour_rect (void *dst, int width, int height, PaletteID pal)
{
	Colour *udst = (Colour *)dst;
	uint16 *anim = this->get_anim_pos (dst);

	if (pal == PALETTE_TO_TRANSPARENT) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeTransparent(*udst, 154);
				*anim = 0;
				udst++;
				anim++;
			}
			udst = udst - width + this->pitch;
			anim = anim - width + this->width;
		} while (--height);
		return;
	}
	if (pal == PALETTE_NEWSPAPER) {
		do {
			for (int i = 0; i != width; i++) {
				*udst = MakeGrey(*udst);
				*anim = 0;
				udst++;
				anim++;
			}
			udst = udst - width + this->pitch;
			anim = anim - width + this->width;
		} while (--height);
		return;
	}

	DEBUG(misc, 0, "32bpp blitter doesn't know how to draw this colour table ('%d')", pal);
}

void Blitter_32bppAnimBase::Surface::set_pixel (void *video, int x, int y, uint8 colour)
{
	*(this->movep <Colour> (video, x, y)) = this->lookup_colour (colour);

	/* Set the colour in the anim-buffer too. */
	uint16 *anim = this->get_anim_pos (video);
	*(movew <uint16> (anim, x, y, this->width)) = colour | (DEFAULT_BRIGHTNESS << 8);
}

void Blitter_32bppAnimBase::Surface::draw_rect (void *video, int width, int height, uint8 colour)
{
	Colour colour32 = this->lookup_colour (colour);
	uint16 *anim_line = this->get_anim_pos (video);

	do {
		Colour *dst = (Colour *)video;
		uint16 *anim = anim_line;

		for (int i = width; i > 0; i--) {
			*dst = colour32;
			/* Set the colour in the anim-buffer too */
			*anim = colour | (DEFAULT_BRIGHTNESS << 8);
			dst++;
			anim++;
		}
		video = (uint32 *)video + this->pitch;
		anim_line += this->width;
	} while (--height);
}

void Blitter_32bppAnimBase::Surface::draw_checker (void *video, uint width, uint height, uint8 colour, byte bo)
{
	Colour colour32 = this->lookup_colour (colour);
	uint16 acolour = colour | (DEFAULT_BRIGHTNESS << 8);

	Colour *dst  = (Colour *) video;
	uint16 *anim = this->get_anim_pos (dst);
	uint i = bo;
	do {
		for (i = !(i & 1); i < width; i += 2) {
			dst[i]  = colour32;
			anim[i] = acolour;
		}
		dst  += this->pitch;
		anim += this->width;
	} while (--height > 0);
}

void Blitter_32bppAnimBase::Surface::paste (const Buffer *src, int x, int y)
{
	void *video = this->Blitter_32bppBase::Surface::move (this->ptr, x, y);
	assert (video >= this->ptr && video <= (uint32 *)this->ptr + this->width + this->height * this->pitch);
	Colour *dst = (Colour *)video;
	const byte *usrc = &src->data.front();
	uint16 *anim_line = this->get_anim_pos (video);

	const uint width = src->width;
	for (uint height = src->height; height > 0; height--) {
		/* We need to keep those for palette animation. */
		Colour *dst_pal = dst;
		uint16 *anim_pal = anim_line;

		memcpy(dst, usrc, width * sizeof(uint32));
		usrc += width * sizeof(uint32);
		dst += this->pitch;
		/* Copy back the anim-buffer */
		memcpy(anim_line, usrc, width * sizeof(uint16));
		usrc += width * sizeof(uint16);
		anim_line += this->width;

		/* Okay, it is *very* likely that the image we stored is using
		 * the wrong palette animated colours. There are two things we
		 * can do to fix this. The first is simply reviewing the whole
		 * screen after we copied the buffer, i.e. run PaletteAnimate,
		 * however that forces a full screen redraw which is expensive
		 * for just the cursor. This just copies the implementation of
		 * palette animation, much cheaper though slightly nastier. */
		for (uint i = 0; i < width; i++) {
			uint colour = GB(*anim_pal, 0, 8);
			if (colour >= PALETTE_ANIM_START) {
				/* Update this pixel */
				*dst_pal = AdjustBrightness (this->lookup_colour (colour), GB(*anim_pal, 8, 8));
			}
			dst_pal++;
			anim_pal++;
		}
	}
}

void Blitter_32bppAnimBase::Surface::copy (Buffer *dst, int x, int y, uint width, uint height)
{
	dst->resize (width, height, sizeof(uint32) + sizeof(uint16));
	/* Only change buffer capacity? */
	if ((x < 0) || (y < 0)) return;

	dst->width  = width;
	dst->height = height;

	const void *video = this->Blitter_32bppBase::Surface::move (this->ptr, x, y);
	assert (video >= this->ptr && video <= (uint32 *)this->ptr + this->width + this->height * this->pitch);
	byte *udst = &dst->data.front();
	const uint32 *src = (const uint32 *)video;
	const uint16 *anim_line = this->get_anim_pos (video);

	for (; height > 0; height--) {
		memcpy(udst, src, width * sizeof(uint32));
		udst += width * sizeof(uint32);
		src += this->pitch;
		/* Copy the anim-buffer */
		memcpy(udst, anim_line, width * sizeof(uint16));
		udst += width * sizeof(uint16);
		anim_line += this->width;
	}

	/* Sanity check that we did not overrun the buffer. */
	assert (udst <= &dst->data.front() + dst->data.size());
}

void Blitter_32bppAnimBase::Surface::scroll (void *video, int &left, int &top, int &width, int &height, int scroll_x, int scroll_y)
{
	assert (video >= this->ptr && video <= (uint32 *)this->ptr + this->width + this->height * this->pitch);
	uint16 *dst, *src;

	/* We need to scroll the anim-buffer too */
	if (scroll_y > 0) {
		dst = movew <uint16> (this->anim_buf, left, top + height - 1, this->width);
		src = dst - scroll_y * this->width;

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst += scroll_x;
		} else {
			src -= scroll_x;
		}

		uint tw = width + (scroll_x >= 0 ? -scroll_x : scroll_x);
		uint th = height - scroll_y;
		for (; th > 0; th--) {
			memcpy(dst, src, tw * sizeof(uint16));
			src -= this->width;
			dst -= this->width;
		}
	} else {
		/* Calculate pointers */
		dst = movew <uint16> (this->anim_buf, left, top, this->width);
		src = dst + (-scroll_y) * this->width;

		/* Adjust left & width */
		if (scroll_x >= 0) {
			dst += scroll_x;
		} else {
			src -= scroll_x;
		}

		/* the y-displacement may be 0 therefore we have to use memmove,
		 * because source and destination may overlap */
		uint tw = width + (scroll_x >= 0 ? -scroll_x : scroll_x);
		uint th = height + scroll_y;
		for (; th > 0; th--) {
			memmove(dst, src, tw * sizeof(uint16));
			src += this->width;
			dst += this->width;
		}
	}

	this->Blitter_32bppBase::Surface::scroll (video, left, top, width, height, scroll_x, scroll_y);
}

bool Blitter_32bppAnimBase::Surface::palette_animate (const Palette &palette)
{
	/* If first_dirty is 0, it is for 8bpp indication to send the new
	 *  palette. However, only the animation colours might possibly change.
	 *  Especially when going between toyland and non-toyland. */
	assert (palette.first_dirty == PALETTE_ANIM_START || palette.first_dirty == 0);
	this->set_palette (palette.palette);

	const uint16 *anim = this->anim_buf;
	Colour *dst = (Colour *)this->ptr;

	/* Let's walk the anim buffer and try to find the pixels */
	for (int y = this->height; y != 0 ; y--) {
		for (int x = this->width; x != 0 ; x--) {
			uint colour = GB(*anim, 0, 8);
			if (colour >= PALETTE_ANIM_START) {
				/* Update this pixel */
				*dst = AdjustBrightness (this->lookup_colour (colour), GB(*anim, 8, 8));
			}
			dst++;
			anim++;
		}
		dst += this->pitch - this->width;
	}

	/* Make sure the backend redraws the whole screen */
	return true;
}
