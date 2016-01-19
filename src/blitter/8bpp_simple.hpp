/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file 8bpp_simple.hpp Simple (and slow) 8 bpp blitter. */

#ifndef BLITTER_8BPP_SIMPLE_HPP
#define BLITTER_8BPP_SIMPLE_HPP

#include "8bpp_base.hpp"

/** Most trivial 8bpp blitter. */
class Blitter_8bppSimple FINAL : public Blitter_8bppBase {
public:
	/** Data structure describing a sprite. */
	struct Sprite : ::Sprite {
		byte data[];   ///< Sprite data
	};

	static const char name[]; ///< Name of the blitter.
	static const char desc[]; ///< Description of the blitter.

	/* virtual */ void Draw(Blitter::BlitterParams *bp, BlitterMode mode, ZoomLevel zoom);
	/* virtual */ ::Sprite *Encode (const SpriteLoader::Sprite *sprite, SpriteType type, AllocatorProc *allocator);
};

#endif /* BLITTER_8BPP_SIMPLE_HPP */
