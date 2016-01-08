/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file autorail.h Highlight/sprite information for autorail. */

/* Rail selection types (directions):
 *  / \    / \    / \    / \   / \   / \
 * /  /\  /\  \  /===\  /   \ /|  \ /  |\
 * \/  /  \  \/  \   /  \===/ \|  / \  |/
 *  \ /    \ /    \ /    \ /   \ /   \ /
 *   0      1      2      3     4     5
 */

/* maps each pixel of a tile (16x16) to a selection type
 * (0,0) is the top corner, (16,16) the bottom corner */
static const byte _autorail_piece[16][16] = {
	{ HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR },
	{ HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR },
	{ HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR },
	{ HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR },
	{ HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR },
	{ HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_HU, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR, HT_RAIL_VR },
	{ HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_Y, HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y  },
	{ HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y, HT_RAIL_X, HT_RAIL_Y, HT_RAIL_Y, HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y  },
	{ HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y, HT_RAIL_Y, HT_RAIL_X, HT_RAIL_Y, HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y  },
	{ HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y,  HT_RAIL_Y  },
	{ HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL },
	{ HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL },
	{ HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL },
	{ HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL },
	{ HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL },
	{ HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_VL, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_X, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL, HT_RAIL_HL }
};
