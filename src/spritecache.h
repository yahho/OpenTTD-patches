/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file spritecache.h Functions to cache sprites in memory. */

#ifndef SPRITECACHE_H
#define SPRITECACHE_H

#include "core/alloc_type.hpp"
#include "gfx_type.h"

/** Data structure describing a sprite. */
struct Sprite {
	uint16 height; ///< Height of the sprite.
	uint16 width;  ///< Width of the sprite.
	int16 x_offs;  ///< Number of pixels to shift the sprite to the right.
	int16 y_offs;  ///< Number of pixels to shift the sprite downwards.
};

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
	 */
	struct Sprite {
		uint16 height;                   ///< Height of the sprite
		uint16 width;                    ///< Width of the sprite
		int16 x_offs;                    ///< The x-offset of where the sprite will be drawn
		int16 y_offs;                    ///< The y-offset of where the sprite will be drawn
		SpriteLoader::CommonPixel *data; ///< The sprite itself
	};
};

extern uint _sprite_cache_size;

typedef void *AllocatorProc(size_t size);

void *GetRawSprite (SpriteID sprite, SpriteType type, bool cache = true);
bool SpriteExists(SpriteID sprite);

SpriteType GetSpriteType(SpriteID sprite);
uint GetOriginFileSlot(SpriteID sprite);
uint GetMaxSpriteID();


static inline const Sprite *GetSprite(SpriteID sprite, SpriteType type)
{
	assert(type != ST_MAPGEN);
	assert(type != ST_RECOLOUR);
	return (Sprite*)GetRawSprite(sprite, type);
}

static inline const byte *GetNonSprite(SpriteID sprite, SpriteType type)
{
	assert(type == ST_RECOLOUR);
	return (byte*)GetRawSprite(sprite, type);
}

/** Data structure describing a map generator sprite. */
struct MapGenSprite : Sprite {
	byte data[];   ///< Sprite data
};

const MapGenSprite *GetMapGenSprite (SpriteID sprite);

void GfxInitSpriteMem();
void GfxClearSpriteCache();
void IncreaseSpriteLRU();

void ReadGRFSpriteOffsets(byte container_version);
size_t GetGRFSpriteOffset(uint32 id);
bool LoadNextSprite(int load_index, byte file_index, uint file_sprite_id, byte container_version);
bool SkipSpriteData(byte type, uint16 num);
void DupSprite(SpriteID old_spr, SpriteID new_spr);

#endif /* SPRITECACHE_H */
