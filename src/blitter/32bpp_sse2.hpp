/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 32bpp_sse2.hpp SSE2 32 bpp blitter. */

#ifndef BLITTER_32BPP_SSE2_HPP
#define BLITTER_32BPP_SSE2_HPP

#ifdef WITH_SSE

#include "32bpp_noanim.hpp"

#include "../cpu.h"

/** Data structure describing a sprite for the SSE blitters. */
struct SSESprite : Sprite {
	struct MapValue {
		uint8 m;
		uint8 v;
	};
	assert_compile(sizeof(MapValue) == 2);

	/** Helper for creating specialised functions for specific optimisations. */
	enum ReadMode {
		RM_WITH_SKIP,   ///< Use normal code for skipping empty pixels.
		RM_WITH_MARGIN, ///< Use cached number of empty pixels at begin and end of line to reduce work.
		RM_NONE,        ///< No specialisation.
	};

	/** Helper for creating specialised functions for the case where the sprite width is odd or even. */
	enum BlockType {
		BT_EVEN, ///< An even number of pixels in the width; no need for a special case for the last pixel.
		BT_ODD,  ///< An odd number of pixels in the width; special case for the last pixel.
		BT_NONE, ///< No specialisation for either case.
	};

	/** Helper for using specialised functions designed to prevent whenever it's possible things like:
	 *  - IO (reading video buffer),
	 *  - calculations (alpha blending),
	 *  - heavy branching (remap lookups and animation buffer handling).
	 */
	enum SpriteFlags {
		SF_NONE        = 0,
		SF_TRANSLUCENT = 1 << 1, ///< The sprite has at least 1 translucent pixel.
		SF_NO_REMAP    = 1 << 2, ///< The sprite has no remappable colour pixel.
		SF_NO_ANIM     = 1 << 3, ///< The sprite has no palette animated pixel.
	};

	/** Data stored about a (single) sprite. */
	struct SpriteInfo {
		uint32 sprite_offset;    ///< The offset to the sprite data.
		uint32 mv_offset;        ///< The offset to the map value data.
		uint16 sprite_line_size; ///< The size of a single line (pitch).
		uint16 sprite_width;     ///< The width of the sprite.
	};

	SpriteFlags flags;
	SpriteInfo infos[ZOOM_LVL_COUNT];
	byte data[]; ///< Data, all zoomlevels.

	static SSESprite *encode (const Blitter::RawSprite *sprite, bool is_font, Blitter::AllocatorProc *allocator);
};

DECLARE_ENUM_AS_BIT_SET(SSESprite::SpriteFlags);

/** The SSE2 32 bpp blitter (without palette animation). */
class Blitter_32bppSSE2 : public Blitter_32bppNoanim {
public:
	typedef SSESprite Sprite;

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	static bool usable (void)
	{
		return HasCPUIDFlag (1, 3, 26);
	}

	/** Convert a sprite from the loader to our own format. */
	static ::Sprite *Encode (const RawSprite *sprite, bool is_font, AllocatorProc *allocator)
	{
		return SSESprite::encode (sprite, is_font, allocator);
	}

	/** Blitting surface. */
	struct Surface : Blitter_32bppNoanim::Surface {
		Surface (void *ptr, uint width, uint height, uint pitch)
			: Blitter_32bppNoanim::Surface (ptr, width, height, pitch)
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
#endif /* BLITTER_32BPP_SSE2_HPP */
