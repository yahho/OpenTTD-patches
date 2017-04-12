/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_ssse3.hpp SSSE3 32 bpp blitter. */

#ifndef BLITTER_32BPP_SSSE3_HPP
#define BLITTER_32BPP_SSSE3_HPP

#ifdef WITH_SSE

#include "32bpp_sse2.hpp"

/** The SSSE3 32 bpp blitter (without palette animation). */
class Blitter_32bppSSSE3 : public Blitter_32bppSSE2 {
public:
	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	static bool usable (void)
	{
		return HasCPUIDFlag (1, 2, 9);
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppSSE2::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_32bppSSE2::Surface (ptr, width, height, pitch)
		{
		}

		template <BlitterMode mode, SSESprite::ReadMode read_mode, SSESprite::BlockType bt_last, bool translucent>
		void draw (const BlitterParams *bp, ZoomLevel zoom);

		void draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) OVERRIDE;
	};

	/** Create a surface for this blitter. */
	static Blitter::Surface *create (void *ptr, uint width, uint height, uint pitch, bool anim)
	{
		return new Surface (ptr, width, height, pitch);
	}
};

#endif /* WITH_SSE */
#endif /* BLITTER_32BPP_SSSE3_HPP */
