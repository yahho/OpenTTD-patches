/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file elrail_data.h Stores all the data for overhead wire and pylon drawing.
 *  @see elrail.c
 */

#ifndef ELRAIL_DATA_H
#define ELRAIL_DATA_H

/** Which PPPs are possible at all on a given PCP */
static const byte AllowedPPPonPCP[DIAGDIR_END] = {
	1 << DIR_N | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_S | 1 << DIR_W  | 1 << DIR_NW,
	1 << DIR_N | 1 << DIR_NE | 1 << DIR_E  | 1 << DIR_S | 1 << DIR_SW | 1 << DIR_W,
	1 << DIR_N | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_S | 1 << DIR_W  | 1 << DIR_NW,
	1 << DIR_N | 1 << DIR_NE | 1 << DIR_E  | 1 << DIR_S | 1 << DIR_SW | 1 << DIR_W,
};


/* Geometric placement of the PCP relative to the tile origin */
static const int8 x_pcp_offsets[DIAGDIR_END] = {0,  8, 16, 8};
static const int8 y_pcp_offsets[DIAGDIR_END] = {8, 16,  8, 0};
/* Geometric placement of the PPP relative to the PCP*/
static const int8 x_ppp_offsets[DIR_END] = {-2, -4, -2,  0,  2,  4,  2,  0};
static const int8 y_ppp_offsets[DIR_END] = {-2,  0,  2,  4,  2,  0, -2, -4};

/* The type of pylon to draw at each PPP */
static const uint8 pylon_sprites[DIR_END] = { 4, 0, 7, 3, 5, 1, 6, 2, };

/**
 * Offset for wire sprites from the base wire sprite.
 */
enum WireSpriteOffset {
	WSO_X_SHORT,
	WSO_Y_SHORT,
	WSO_EW_SHORT,
	WSO_NS_SHORT,
	WSO_X_SHORT_DOWN,
	WSO_Y_SHORT_UP,
	WSO_X_SHORT_UP,
	WSO_Y_SHORT_DOWN,

	WSO_X_SW,
	WSO_Y_SE,
	WSO_EW_E,
	WSO_NS_S,
	WSO_X_SW_DOWN,
	WSO_Y_SE_UP,
	WSO_X_SW_UP,
	WSO_Y_SE_DOWN,

	WSO_X_NE,
	WSO_Y_NW,
	WSO_EW_W,
	WSO_NS_N,
	WSO_X_NE_DOWN,
	WSO_Y_NW_UP,
	WSO_X_NE_UP,
	WSO_Y_NW_DOWN,

	WSO_ENTRANCE_NE,
	WSO_ENTRANCE_SE,
	WSO_ENTRANCE_SW,
	WSO_ENTRANCE_NW,
};

struct SortableSpriteStructM {
	int8 x_offset;
	int8 y_offset;
	int8 x_size;
	int8 y_size;
	int8 z_offset;
	uint8 image_offset[3];
};

/** Distance between wire and rail */
static const uint ELRAIL_ELEVATION = 10;
/** Wires that a draw one level higher than the north corner. */
static const uint ELRAIL_ELEVRAISE = ELRAIL_ELEVATION + TILE_HEIGHT;

static const SortableSpriteStructM CatenarySpriteData[TRACK_END] = {
	{  0,  7, 15,  1, ELRAIL_ELEVATION, { WSO_X_NE, WSO_X_SW, WSO_X_SHORT  } }, // X flat
	{  7,  0,  1, 15, ELRAIL_ELEVATION, { WSO_Y_SE, WSO_Y_NW, WSO_Y_SHORT  } }, // Y flat
	{  7,  0,  1,  1, ELRAIL_ELEVATION, { WSO_EW_W, WSO_EW_E, WSO_EW_SHORT } }, // UPPER
	{ 15,  8,  3,  3, ELRAIL_ELEVATION, { WSO_EW_E, WSO_EW_W, WSO_EW_SHORT } }, // LOWER
	{  8,  0,  8,  8, ELRAIL_ELEVATION, { WSO_NS_S, WSO_NS_N, WSO_NS_SHORT } }, // LEFT
	{  0,  8,  8,  8, ELRAIL_ELEVATION, { WSO_NS_N, WSO_NS_S, WSO_NS_SHORT } }, // RIGHT
};

static const SortableSpriteStructM CatenarySpriteDataSW =
	{  0,  7, 15,  8, ELRAIL_ELEVRAISE, { WSO_X_NE_UP,   WSO_X_SW_UP,   WSO_X_SHORT_UP   } }; // X up

static const SortableSpriteStructM CatenarySpriteDataSE =
	{  7,  0,  8, 15, ELRAIL_ELEVRAISE, { WSO_Y_SE_UP,   WSO_Y_NW_UP,   WSO_Y_SHORT_UP   } }; // Y up

static const SortableSpriteStructM CatenarySpriteDataNW =
	{  7,  0,  8, 15, ELRAIL_ELEVATION, { WSO_Y_SE_DOWN, WSO_Y_NW_DOWN, WSO_Y_SHORT_DOWN } }; // Y down

static const SortableSpriteStructM CatenarySpriteDataNE =
	{  0,  7, 15,  8, ELRAIL_ELEVATION, { WSO_X_NE_DOWN, WSO_X_SW_DOWN, WSO_X_SHORT_DOWN } }; // X down


#endif /* ELRAIL_DATA_H */
