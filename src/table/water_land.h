/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file water_land.h Sprites to use and how to display them for water tiles (depots/locks). */

/**
 * Constructor macro for an image without a palette in a DrawTileSeqStruct array.
 * @param dx  Offset in x direction
 * @param dy  Offset in y direction
 * @param dz  Offset in z direction
 * @param sx  Size in x direction
 * @param sy  Size in y direction
 * @param sz  Size in z direction
 * @param img Sprite to draw
 */
#define TILE_SEQ_LINE(dx, dy, dz, sx, sy, sz, img) { dx, dy, dz, sx, sy, sz, {img, PAL_NONE} },

/** Constructor macro for a terminating DrawTileSeqStruct entry in an array */
#define TILE_SEQ_END() { (int8)0x80, 0, 0, 0, 0, 0, {0, 0} }

static const DrawTileSeqStruct _shipdepot_display_seq_ne[] = {
	TILE_SEQ_LINE( 0, 15, 0, 16, 1, 0x14, 0xFE8 | (1 << PALETTE_MODIFIER_COLOUR))
	TILE_SEQ_END()
};

static const DrawTileSeqStruct _shipdepot_display_seq_sw[] = {
	TILE_SEQ_LINE( 0,  0, 0, 16, 1, 0x14, 0xFEA)
	TILE_SEQ_LINE( 0, 15, 0, 16, 1, 0x14, 0xFE6 | (1 << PALETTE_MODIFIER_COLOUR))
	TILE_SEQ_END()
};

static const DrawTileSeqStruct _shipdepot_display_seq_nw[] = {
	TILE_SEQ_LINE( 15, 0, 0, 1, 0x10, 0x14, 0xFE9 | (1 << PALETTE_MODIFIER_COLOUR))
	TILE_SEQ_END()
};

static const DrawTileSeqStruct _shipdepot_display_seq_se[] = {
	TILE_SEQ_LINE(  0, 0, 0, 1, 16, 0x14, 0xFEB)
	TILE_SEQ_LINE( 15, 0, 0, 1, 16, 0x14, 0xFE7 | (1 << PALETTE_MODIFIER_COLOUR))
	TILE_SEQ_END()
};

static const DrawTileSeqStruct *_shipdepot_display_data[DIAGDIR_END] = {
	_shipdepot_display_seq_ne, // DIAGDIR_NE
	_shipdepot_display_seq_se, // DIAGDIR_SE
	_shipdepot_display_seq_sw, // DIAGDIR_SW
	_shipdepot_display_seq_nw, // DIAGDIR_NW
};


static const DrawTileSeqStruct _lock_display_data[3][DIAGDIR_END][3] = {
	{ // LOCK_PART_MIDDLE
		{    // NE
			TILE_SEQ_LINE( 0,   0, 0, 0x10, 1, 0x14,  0 + 1)
			TILE_SEQ_LINE( 0, 0xF, 0, 0x10, 1, 0x14,  4 + 1)
			TILE_SEQ_END()
		}, { // SE
			TILE_SEQ_LINE(   0, 0, 0, 1, 0x10, 0x14,  0)
			TILE_SEQ_LINE( 0xF, 0, 0, 1, 0x10, 0x14,  4)
			TILE_SEQ_END()
		}, { // SW
			TILE_SEQ_LINE( 0,   0, 0, 0x10, 1, 0x14,  0 + 2)
			TILE_SEQ_LINE( 0, 0xF, 0, 0x10, 1, 0x14,  4 + 2)
			TILE_SEQ_END()
		}, { // NW
			TILE_SEQ_LINE(   0, 0, 0, 1, 0x10, 0x14,  0 + 3)
			TILE_SEQ_LINE( 0xF, 0, 0, 1, 0x10, 0x14,  4 + 3)
			TILE_SEQ_END()
		}
	},

	{ // LOCK_PART_LOWER
		{    // NE
			TILE_SEQ_LINE( 0,   0, 0, 0x10, 1, 0x14,  8 + 1)
			TILE_SEQ_LINE( 0, 0xF, 0, 0x10, 1, 0x14, 12 + 1)
			TILE_SEQ_END()
		}, { // SE
			TILE_SEQ_LINE(   0, 0, 0, 1, 0x10, 0x14,  8)
			TILE_SEQ_LINE( 0xF, 0, 0, 1, 0x10, 0x14, 12)
			TILE_SEQ_END()
		}, { // SW
			TILE_SEQ_LINE( 0,   0, 0, 0x10, 1, 0x14,  8 + 2)
			TILE_SEQ_LINE( 0, 0xF, 0, 0x10, 1, 0x14, 12 + 2)
			TILE_SEQ_END()
		}, { // NW
			TILE_SEQ_LINE(   0, 0, 0, 1, 0x10, 0x14,  8 + 3)
			TILE_SEQ_LINE( 0xF, 0, 0, 1, 0x10, 0x14, 12 + 3)
			TILE_SEQ_END()
		}
	},

	{ // LOCK_PART_UPPER
		{    // NE
			TILE_SEQ_LINE( 0,   0, 0, 0x10, 1, 0x14, 16 + 1)
			TILE_SEQ_LINE( 0, 0xF, 0, 0x10, 1, 0x14, 20 + 1)
			TILE_SEQ_END()
		}, { // SE
			TILE_SEQ_LINE(   0, 0, 0, 1, 0x10, 0x14, 16)
			TILE_SEQ_LINE( 0xF, 0, 0, 1, 0x10, 0x14, 20)
			TILE_SEQ_END()
		}, { // SW
			TILE_SEQ_LINE( 0,   0, 0, 0x10, 1, 0x14, 16 + 2)
			TILE_SEQ_LINE( 0, 0xF, 0, 0x10, 1, 0x14, 20 + 2)
			TILE_SEQ_END()
		}, { // NW
			TILE_SEQ_LINE(   0, 0, 0, 1, 0x10, 0x14, 16 + 3)
			TILE_SEQ_LINE( 0xF, 0, 0, 1, 0x10, 0x14, 20 + 3)
			TILE_SEQ_END()
		}
	},
};

#undef TILE_SEQ_LINE
#undef TILE_SEQ_END
