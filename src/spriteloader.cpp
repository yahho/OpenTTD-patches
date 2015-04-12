/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file spriteloader.cpp Reading graphics data from (New)GRF files. */

#include "stdafx.h"
#include "gfx_func.h"
#include "fileio_func.h"
#include "debug.h"
#include "strings_func.h"
#include "table/strings.h"
#include "error.h"
#include "core/pointer.h"
#include "core/math_func.hpp"
#include "core/bitmath_func.hpp"
#include "spriteloader.h"

extern const byte _palmap_w2d[];

/** The different colour components a sprite can have. */
enum SpriteColourComponent {
	SCC_RGB   = 1 << 0, ///< Sprite has RGB.
	SCC_ALPHA = 1 << 1, ///< Sprite has alpha.
	SCC_PAL   = 1 << 2, ///< Sprite has palette data.
	SCC_MASK  = SCC_RGB | SCC_ALPHA | SCC_PAL, ///< Mask of valid colour bits.
};
DECLARE_ENUM_AS_BIT_SET(SpriteColourComponent)

ReusableBuffer<SpriteLoader::CommonPixel> SpriteLoader::Sprite::buffer[ZOOM_LVL_COUNT];

/**
 * We found a corrupted sprite. This means that the sprite itself
 * contains invalid data or is too small for the given dimensions.
 * @param file_slot the file the errored sprite is in
 * @param file_pos the location in the file of the errored sprite
 * @param line the line where the error occurs.
 * @return always false (to tell loading the sprite failed)
 */
static bool WarnCorruptSprite(uint8 file_slot, size_t file_pos, int line)
{
	static byte warning_level = 0;
	if (warning_level == 0) {
		SetDParamStr(0, FioGetFilename(file_slot));
		ShowErrorMessage(STR_NEWGRF_ERROR_CORRUPT_SPRITE, INVALID_STRING_ID, WL_ERROR);
	}
	DEBUG(sprite, warning_level, "[%i] Loading corrupted sprite from %s at position %i", line, FioGetFilename(file_slot), (int)file_pos);
	warning_level = 6;
	return false;
}

/**
 * Uncompress the raw data of a single sprite.
 * @param buf Buffer where to store the uncompressed data.
 * @param num Size of the decompressed sprite.
 * @param file_slot File slot.
 * @param file_pos File position.
 * @return Whether the sprite was successfully loaded.
 */
static bool UncompressSingleSprite (byte *buf, uint num,
	uint8 file_slot, size_t file_pos)
{
	byte *dest = buf;

	while (num > 0) {
		int8 code = FioReadByte();

		if (code >= 0) {
			/* Plain bytes to read */
			uint size = (code == 0) ? 0x80 : code;
			if (num < size) return WarnCorruptSprite (file_slot, file_pos, __LINE__);
			num -= size;
			for (; size > 0; size--) {
				*dest = FioReadByte();
				dest++;
			}
		} else {
			/* Copy bytes from earlier in the sprite */
			const uint data_offset = ((code & 7) << 8) | FioReadByte();
			if ((uint)(dest - buf) < data_offset) return WarnCorruptSprite (file_slot, file_pos, __LINE__);
			uint size = -(code >> 3);
			if (num < size) return WarnCorruptSprite (file_slot, file_pos, __LINE__);
			num -= size;
			for (; size > 0; size--) {
				*dest = *(dest - data_offset);
				dest++;
			}
		}
	}

	return true;
}

/**
 * Decode a sequence of pixels in a sprite.
 * @param sprite_type Type of the sprite we're decoding.
 * @param colour_fmt Colour format of the sprite.
 * @param remap Whether to remap palette indices.
 * @param n The number of pixels to decode.
 * @param pixel Buffer with the raw data to decode.
 * @param data Buffer where to store the decoded data.
 * @return A pointer to the first unused byte in the source data.
 */
static const byte *DecodePixelData (SpriteType sprite_type, byte colour_fmt,
	bool remap, uint n, const byte *pixel, SpriteLoader::CommonPixel *data)
{
	for (; n > 0; n--, data++) {
		if (colour_fmt & SCC_RGB) {
			data->r = *pixel++;
			data->g = *pixel++;
			data->b = *pixel++;
		}

		data->a = (colour_fmt & SCC_ALPHA) ? *pixel++ : 0xFF;

		if (colour_fmt & SCC_PAL) {
			byte m = *pixel++;

			/* Magic blue. */
			if (colour_fmt == SCC_PAL && m == 0) data->a = 0x00;

			switch (sprite_type) {
				case ST_NORMAL:
					if (remap) m = _palmap_w2d[m];
					break;

				case ST_FONT:
					if (m > 2) m = 2;
					break;

				default:
					break;
			}

			data->m = m;
		}
	}

	return pixel;
}

/**
 * Decode the image data of a single sprite without transparency.
 * @param[in,out] sprite Filled with the sprite image data.
 * @param file_slot File slot.
 * @param file_pos File position.
 * @param sprite_type Type of the sprite we're decoding.
 * @param orig Buffer with the raw data to decode.
 * @param size Size of the decompressed sprite.
 * @param colour_fmt Colour format of the sprite.
 * @param bpp Bits per pixel.
 * @return True if the sprite was successfully loaded.
 */
static bool DecodeSingleSpriteNormal (SpriteLoader::Sprite *sprite,
	uint8 file_slot, size_t file_pos, SpriteType sprite_type,
	const byte *orig, uint size, byte colour_fmt, uint bpp)
{
	uint64 check_size = (uint64) sprite->width * sprite->height * bpp;

	if (size < check_size) {
		return WarnCorruptSprite (file_slot, file_pos, __LINE__);
	}

	if (size > check_size) {
		static byte warning_level = 0;
		DEBUG (sprite, warning_level, "Ignoring " OTTD_PRINTF64 " unused extra bytes from the sprite from %s at position %i", size - check_size, FioGetFilename(file_slot), (int)file_pos);
		warning_level = 6;
	}

	DecodePixelData (sprite_type, colour_fmt, _palette_remap_grf[file_slot],
		sprite->width * sprite->height, orig, sprite->data);

	return true;
}

/**
 * Decode the image data of a single sprite with transparency.
 * @param[in,out] sprite Filled with the sprite image data.
 * @param file_slot File slot.
 * @param file_pos File position.
 * @param sprite_type Type of the sprite we're decoding.
 * @param orig Buffer with the raw data to decode.
 * @param size Size of the decompressed sprite.
 * @param colour_fmt Colour format of the sprite.
 * @param bpp Bits per pixel.
 * @param container_format Container format of the GRF this sprite is in.
 * @return True if the sprite was successfully loaded.
 */
static bool DecodeSingleSpriteTransparency (SpriteLoader::Sprite *sprite,
	uint8 file_slot, size_t file_pos, SpriteType sprite_type,
	const byte *orig, uint size, byte colour_fmt, uint bpp,
	byte container_format)
{
	for (int y = 0; y < sprite->height; y++) {
		/* Look up in the header-table where the real data is stored for this row */
		uint offset = (container_format >= 2 && size > UINT16_MAX) ?
			(orig[y * 4 + 3] << 24) | (orig[y * 4 + 2] << 16) | (orig[y * 4 + 1] << 8) | orig[y * 4] :
			(orig[y * 2 + 1] <<  8) |  orig[y * 2];

		if (offset >= size) {
			return WarnCorruptSprite (file_slot, file_pos, __LINE__);
		}

		/* Go to that row */
		const byte *src = orig + offset;

		bool last_item = false;
		do {
			uint remaining = (orig + size) - src;

			/* Read the header. */
			uint length, skip;
			if (container_format >= 2 && sprite->width > 256) {
				/*  0 .. 14  - length
				 *  15       - last_item
				 *  16 .. 31 - transparency bytes */
				if (remaining < 4) {
					return WarnCorruptSprite (file_slot, file_pos, __LINE__);
				}
				last_item =  (src[1] & 0x80) != 0;
				length    = ((src[1] & 0x7F) << 8) | src[0];
				skip      =  (src[3] << 8) | src[2];
				src += 4;
			} else {
				/*  0 .. 6  - length
				 *  7       - last_item
				 *  8 .. 15 - transparency bytes */
				if (remaining < 2) {
					return WarnCorruptSprite (file_slot, file_pos, __LINE__);
				}
				last_item = (*src & 0x80) != 0;
				length = *src++ & 0x7F;
				skip   = *src++;
			}

			if (skip + length > sprite->width || length * bpp > (uint)((orig + size) - src)) {
				return WarnCorruptSprite (file_slot, file_pos, __LINE__);
			}

			src = DecodePixelData (sprite_type, colour_fmt,
				_palette_remap_grf[file_slot], length, src,
				sprite->data + y * sprite->width + skip);

		} while (!last_item);
	}

	return true;
}

/**
 * Decode the image data of a single sprite.
 * @param[in,out] sprite Filled with the sprite image data.
 * @param file_slot File slot.
 * @param file_pos File position.
 * @param sprite_type Type of the sprite we're decoding.
 * @param dest_size Size of the decompressed sprite.
 * @param type Type of the encoded sprite.
 * @param zoom_lvl Requested zoom level.
 * @param colour_fmt Colour format of the sprite.
 * @param container_format Container format of the GRF this sprite is in.
 * @return True if the sprite was successfully loaded.
 */
static bool DecodeSingleSprite (SpriteLoader::Sprite *sprite,
	uint8 file_slot, size_t file_pos, SpriteType sprite_type, uint dest_size,
	byte type, ZoomLevel zoom_lvl, byte colour_fmt, byte container_format)
{
	ttd_unique_free_ptr<byte> dest_orig (xmalloct<byte> (dest_size));

	/* Read the file, which has some kind of compression */
	if (!UncompressSingleSprite (dest_orig.get(), dest_size, file_slot, file_pos)) {
		return false;
	}

	sprite->AllocateData(zoom_lvl, sprite->width * sprite->height);

	/* Convert colour depth to pixel size. */
	uint bpp = 0;
	if (colour_fmt & SCC_RGB)   bpp += 3; // Has RGB data.
	if (colour_fmt & SCC_ALPHA) bpp++;    // Has alpha data.
	if (colour_fmt & SCC_PAL)   bpp++;    // Has palette data.

	/* When there are transparency pixels, this format has another trick.. decode it */
	return (type & 0x08) ?
		DecodeSingleSpriteTransparency (sprite, file_slot, file_pos,
			sprite_type, dest_orig.get(), dest_size, colour_fmt, bpp,
			container_format) :
		DecodeSingleSpriteNormal (sprite, file_slot, file_pos,
			sprite_type, dest_orig.get(), dest_size, colour_fmt, bpp);
}

static uint8 LoadSpriteV1 (SpriteLoader::Sprite *sprite, uint8 file_slot, size_t file_pos, SpriteType sprite_type, bool load_32bpp)
{
	/* Check the requested colour depth. */
	if (load_32bpp) return 0;

	/* Open the right file and go to the correct position */
	FioSeekToFile(file_slot, file_pos);

	/* Read the size and type */
	uint num = FioReadWord();
	if (num < 8) {
		WarnCorruptSprite(file_slot, file_pos, __LINE__);
		return 0;
	}

	byte type = FioReadByte();

	/* Type 0xFF indicates either a colourmap or some other non-sprite info; we do not handle them here */
	if (type == 0xFF) return 0;

	ZoomLevel zoom_lvl = (sprite_type != ST_MAPGEN) ? ZOOM_LVL_OUT_4X : ZOOM_LVL_NORMAL;

	sprite[zoom_lvl].height = FioReadByte();
	sprite[zoom_lvl].width  = FioReadWord();

	if (sprite[zoom_lvl].width > INT16_MAX) {
		WarnCorruptSprite(file_slot, file_pos, __LINE__);
		return 0;
	}

	sprite[zoom_lvl].x_offs = FioReadWord();
	sprite[zoom_lvl].y_offs = FioReadWord();

	/* 0x02 indicates it is a compressed sprite, so we can't rely on 'num' to be valid.
	 * In case it is uncompressed, the size is 'num' - 8 (header-size). */
	num = (type & 0x02) ? sprite[zoom_lvl].width * sprite[zoom_lvl].height : num - 8;

	if (DecodeSingleSprite(&sprite[zoom_lvl], file_slot, file_pos, sprite_type, num, type, zoom_lvl, SCC_PAL, 1)) return 1 << zoom_lvl;

	return 0;
}

static uint8 LoadSpriteV2 (SpriteLoader::Sprite *sprite, uint8 file_slot, size_t file_pos, SpriteType sprite_type, bool load_32bpp)
{
	static const ZoomLevel zoom_lvl_map[6] = {ZOOM_LVL_OUT_4X, ZOOM_LVL_NORMAL, ZOOM_LVL_OUT_2X, ZOOM_LVL_OUT_8X, ZOOM_LVL_OUT_16X, ZOOM_LVL_OUT_32X};

	/* Is the sprite not present/stripped in the GRF? */
	if (file_pos == SIZE_MAX) return 0;

	/* Open the right file and go to the correct position */
	FioSeekToFile(file_slot, file_pos);

	uint32 id = FioReadDword();

	uint8 loaded_sprites = 0;
	do {
		uint num = FioReadDword();
		if (num < 10) {
			WarnCorruptSprite (file_slot, file_pos, __LINE__);
			return 0;
		}

		size_t start_pos = FioGetPos();
		byte type = FioReadByte();

		/* Type 0xFF indicates either a colourmap or some other non-sprite info; we do not handle them here. */
		if (type == 0xFF) return 0;

		byte colour = type & SCC_MASK;
		byte zoom = FioReadByte();

		if (colour == 0 ||
				(load_32bpp ? colour == SCC_PAL : colour != SCC_PAL) ||
				(sprite_type != ST_MAPGEN ? zoom >= lengthof(zoom_lvl_map) : zoom != 0)) {
			/* Not the wanted zoom level or colour depth, continue searching. */
			FioSkipBytes (num - 2);
			continue;
		}

		ZoomLevel zoom_lvl = (sprite_type != ST_MAPGEN) ? zoom_lvl_map[zoom] : ZOOM_LVL_NORMAL;

		if (HasBit (loaded_sprites, zoom_lvl)) {
			/* We already have this zoom level, skip sprite. */
			DEBUG (sprite, 1, "Ignoring duplicate zoom level sprite %u from %s", id, FioGetFilename(file_slot));
			FioSkipBytes (num - 2);
			continue;
		}

		sprite[zoom_lvl].height = FioReadWord();
		sprite[zoom_lvl].width  = FioReadWord();

		if (sprite[zoom_lvl].width > INT16_MAX || sprite[zoom_lvl].height > INT16_MAX) {
			WarnCorruptSprite (file_slot, file_pos, __LINE__);
			return 0;
		}

		sprite[zoom_lvl].x_offs = FioReadWord();
		sprite[zoom_lvl].y_offs = FioReadWord();

		/* Mask out colour information. */
		type = type & ~SCC_MASK;

		/* Convert colour depth to pixel size. */
		int bpp = 0;
		if (colour & SCC_RGB)   bpp += 3; // Has RGB data.
		if (colour & SCC_ALPHA) bpp++;    // Has alpha data.
		if (colour & SCC_PAL)   bpp++;    // Has palette data.

		/* For chunked encoding we store the decompressed size in the file,
		 * otherwise we can calculate it from the image dimensions. */
		uint decomp_size = (type & 0x08) ? FioReadDword() : sprite[zoom_lvl].width * sprite[zoom_lvl].height * bpp;

		if (DecodeSingleSprite (&sprite[zoom_lvl], file_slot, file_pos, sprite_type, decomp_size, type, zoom_lvl, colour, 2)) {
			SetBit (loaded_sprites, zoom_lvl);
		}

		if (FioGetPos() != start_pos + num) {
			WarnCorruptSprite (file_slot, file_pos, __LINE__);
			return 0;
		}

	} while (FioReadDword() == id);

	return loaded_sprites;
}

/**
 * Load a sprite from the disk and return a sprite struct which is the same for all loaders.
 * @param container_ver The container version.
 * @param[out] sprite The sprites to fill with data.
 * @param file_slot   The file "descriptor" of the file we read from.
 * @param file_pos    The position within the file the image begins.
 * @param sprite_type The type of sprite we're trying to load.
 * @param load_32bpp  True if 32bpp sprites should be loaded, false for a 8bpp sprite.
 * @return Bit mask of the zoom levels successfully loaded or 0 if no sprite could be loaded.
 */
uint8 LoadGrfSprite (uint container_ver, SpriteLoader::Sprite *sprite,
	uint8 file_slot, size_t file_pos, SpriteType sprite_type, bool load_32bpp)
{
	if (container_ver >= 2) {
		return LoadSpriteV2(sprite, file_slot, file_pos, sprite_type, load_32bpp);
	} else {
		return LoadSpriteV1(sprite, file_slot, file_pos, sprite_type, load_32bpp);
	}
}
