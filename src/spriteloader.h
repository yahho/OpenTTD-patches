/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file spriteloader.h Base for loading sprites. */

#ifndef SPRITELOADER_H
#define SPRITELOADER_H

#include "core/alloc_type.hpp"
#include "gfx_type.h"

/** Interface for the loader of our sprites. */
namespace SpriteLoader {
	/** Definition of a common pixel in OpenTTD's realm. */
	struct CommonPixel {
		uint8 r;  ///< Red-channel
		uint8 g;  ///< Green-channel
		uint8 b;  ///< Blue-channel
		uint8 a;  ///< Alpha-channel
		uint8 m;  ///< Remap-channel
	};

	/**
	 * Structure for passing information from the sprite loader to the blitter.
	 * You can only use this struct once at a time when using AllocateData to
	 * allocate the memory as that will always return the same memory address.
	 * This to prevent thousands of malloc + frees just to load a sprite.
	 */
	struct Sprite {
		uint16 height;                   ///< Height of the sprite
		uint16 width;                    ///< Width of the sprite
		int16 x_offs;                    ///< The x-offset of where the sprite will be drawn
		int16 y_offs;                    ///< The y-offset of where the sprite will be drawn
		SpriteType type;                 ///< The sprite type
		SpriteLoader::CommonPixel *data; ///< The sprite itself

		/**
		 * Allocate the sprite data of this sprite.
		 * @param zoom Zoom level to allocate the data for.
		 * @param size the minimum size of the data field.
		 */
		void AllocateData(ZoomLevel zoom, size_t size) { this->data = Sprite::buffer[zoom].ZeroAllocate(size); }
	private:
		/** Allocated memory to pass sprite data around */
		static ReusableBuffer<SpriteLoader::CommonPixel> buffer[ZOOM_LVL_COUNT];
	};
};

/** Sprite loader for graphics coming from a (New)GRF. */
class SpriteLoaderGrf {
	byte container_ver;
public:
	SpriteLoaderGrf(byte container_ver) : container_ver(container_ver) {}
	uint8 LoadSprite(SpriteLoader::Sprite *sprite, uint8 file_slot, size_t file_pos, SpriteType sprite_type, bool load_32bpp);
};

#endif /* SPRITELOADER_H */
