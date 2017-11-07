/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite.cpp Handling of sprites */

#include "stdafx.h"
#include "sprite.h"
#include "viewport_func.h"
#include "landscape.h"
#include "spritecache.h"
#include "zoom_func.h"

/** Compute the palette to use for a layout sprite. */
static inline PaletteID SpriteLayoutPalette (SpriteID image, PaletteID pal,
	uint32 offset, PaletteID def)
{
	if (HasBit(image, PALETTE_MODIFIER_TRANSPARENT) || HasBit(image, PALETTE_MODIFIER_COLOUR)) {
		if (HasBit(pal, SPRITE_MODIFIER_CUSTOM_SPRITE)) pal += offset;
		return (pal != 0 ? pal : def);
	} else {
		return PAL_NONE;
	}
}

/**
 * Draws a tile sprite sequence.
 * @param ti The tile to draw on
 * @param seq Sprite and subsprites to draw
 * @param to The transparency bit that toggles drawing of these sprites
 * @param orig_offset Sprite-Offset for original sprites
 * @param newgrf_offset Sprite-Offset for NewGRF defined sprites
 * @param default_palette The default recolour sprite to use (typically company colour)
 * @param child_offset_is_unsigned Whether child sprite offsets are interpreted signed or unsigned
 */
void DrawCommonTileSeq (const TileInfo *ti, const DrawTileSeqStruct *seq,
	TransparencyOption to, int32 orig_offset, uint32 newgrf_offset,
	PaletteID default_palette, bool child_offset_is_unsigned)
{
	bool parent_sprite_encountered = false;
	const DrawTileSeqStruct *dtss;
	bool skip_childs = false;
	foreach_draw_tile_seq(dtss, seq) {
		SpriteID image = dtss->image.sprite;

		if (skip_childs) {
			if (!dtss->IsParentSprite()) continue;
			skip_childs = false;
		}

		/* TTD sprite 0 means no sprite */
		if ((GB(image, 0, SPRITE_WIDTH) == 0 && !HasBit(image, SPRITE_MODIFIER_CUSTOM_SPRITE)) ||
				(IsInvisibilitySet(to) && !HasBit(image, SPRITE_MODIFIER_OPAQUE))) {
			skip_childs = dtss->IsParentSprite();
			continue;
		}

		image += (HasBit(image, SPRITE_MODIFIER_CUSTOM_SPRITE) ? newgrf_offset : orig_offset);

		PaletteID pal = SpriteLayoutPalette (image, dtss->image.pal,
					newgrf_offset, default_palette);

		bool transparent = !HasBit(image, SPRITE_MODIFIER_OPAQUE) && IsTransparencySet(to);
		if (dtss->IsParentSprite()) {
			parent_sprite_encountered = true;
			AddSortableSpriteToDraw (ti->vd,
				image, pal,
				ti->x + dtss->delta_x, ti->y + dtss->delta_y,
				dtss->size_x, dtss->size_y,
				dtss->size_z, ti->z + dtss->delta_z,
				transparent
			);
		} else {
			int offs_x = child_offset_is_unsigned ? (uint8)dtss->delta_x : dtss->delta_x;
			int offs_y = child_offset_is_unsigned ? (uint8)dtss->delta_y : dtss->delta_y;
			if (parent_sprite_encountered) {
				AddChildSpriteScreen (ti->vd, image, pal, offs_x, offs_y, transparent);
			} else {
				if (transparent) {
					SetBit(image, PALETTE_MODIFIER_TRANSPARENT);
					pal = PALETTE_TO_TRANSPARENT;
				}
				DrawGroundSprite (ti, image, pal, NULL, offs_x, offs_y);
			}
		}
	}
}

/**
 * Draws a tile sprite sequence in the GUI
 * @param dpi Area to draw on.
 * @param x X position to draw to
 * @param y Y position to draw to
 * @param seq Sprite and subsprites to draw
 * @param orig_offset Sprite-Offset for original sprites
 * @param newgrf_offset Sprite-Offset for NewGRF defined sprites
 * @param default_palette The default recolour sprite to use (typically company colour)
 * @param child_offset_is_unsigned Whether child sprite offsets are interpreted signed or unsigned
 */
void DrawCommonTileSeqInGUI (BlitArea *dpi, int x, int y,
	const DrawTileSeqStruct *seq, int32 orig_offset, uint32 newgrf_offset,
	PaletteID default_palette, bool child_offset_is_unsigned)
{
	const DrawTileSeqStruct *dtss;
	Point child_offset = {0, 0};

	bool skip_childs = false;
	foreach_draw_tile_seq(dtss, seq) {
		SpriteID image = dtss->image.sprite;

		if (skip_childs) {
			if (!dtss->IsParentSprite()) continue;
			skip_childs = false;
		}

		/* TTD sprite 0 means no sprite */
		if (GB(image, 0, SPRITE_WIDTH) == 0 && !HasBit(image, SPRITE_MODIFIER_CUSTOM_SPRITE)) {
			skip_childs = dtss->IsParentSprite();
			continue;
		}

		image += (HasBit(image, SPRITE_MODIFIER_CUSTOM_SPRITE) ? newgrf_offset : orig_offset);

		PaletteID pal = SpriteLayoutPalette (image, dtss->image.pal,
					newgrf_offset, default_palette);

		if (dtss->IsParentSprite()) {
			Point pt = RemapCoords(dtss->delta_x, dtss->delta_y, dtss->delta_z);
			DrawSprite (dpi, image, pal, x + UnScaleGUI(pt.x), y + UnScaleGUI(pt.y));

			const Sprite *spr = GetSprite(image & SPRITE_MASK, ST_NORMAL);
			child_offset.x = UnScaleGUI(pt.x + spr->x_offs);
			child_offset.y = UnScaleGUI(pt.y + spr->y_offs);
		} else {
			int offs_x = child_offset_is_unsigned ? (uint8)dtss->delta_x : dtss->delta_x;
			int offs_y = child_offset_is_unsigned ? (uint8)dtss->delta_y : dtss->delta_y;
			DrawSprite (dpi, image, pal, x + child_offset.x + ScaleGUITrad(offs_x), y + child_offset.y + ScaleGUITrad(offs_y));
		}
	}
}
