/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_sse4_anim.hpp A SSE4 32 bpp blitter with animation support. */

#ifndef BLITTER_32BPP_SSE4_ANIM_HPP
#define BLITTER_32BPP_SSE4_ANIM_HPP

#ifdef WITH_SSE

#include "32bpp_anim.hpp"
#include "32bpp_sse4.hpp"

/** The SSE4 32 bpp blitter with palette animation. */
class Blitter_32bppSSE4_Anim FINAL : public Blitter_32bppAnimBase {
private:

public:
	typedef SSESprite Sprite;

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	static bool usable (void)
	{
		return HasCPUIDFlag (1, 2, 19);
	}

	::Sprite *Encode (const RawSprite *sprite, bool is_font, AllocatorProc *allocator) OVERRIDE
	{
		return SSESprite::encode (sprite, is_font, allocator);
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppAnimBase::Surface, FlexArray<uint16> {
		uint16 anim_buf[]; ///< In this buffer we keep track of the 8bpp indexes so we can do palette animation

	private:
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_32bppAnimBase::Surface (ptr, width, height, pitch, this->anim_buf)
		{
		}

	public:
		static Surface *create (void *ptr, uint width, uint height, uint pitch)
		{
			return new (width, height) Surface (ptr, width, height, pitch);
		}

		template <BlitterMode mode, SSESprite::ReadMode read_mode, SSESprite::BlockType bt_last, bool translucent, bool animated>
		void draw (const BlitterParams *bp, ZoomLevel zoom);

		template <BlitterMode mode, SSESprite::ReadMode read_mode, SSESprite::BlockType bt_last, bool translucent>
		void draw (const BlitterParams *bp, ZoomLevel zoom, bool animated);

		void draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) OVERRIDE;
	};

	/** Create a surface for this blitter. */
	Blitter_32bppBase::Surface *create (void *ptr, uint width, uint height, uint pitch, bool anim) OVERRIDE
	{
		if (anim) {
			return Surface::create (ptr, width, height, pitch);
		} else {
			return new Blitter_32bppSSE4::Surface (ptr, width, height, pitch);
		}
	}
};

#endif /* WITH_SSE */
#endif /* BLITTER_32BPP_SSE4_ANIM_HPP */
