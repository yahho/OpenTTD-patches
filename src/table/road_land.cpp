/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road_land.cpp Sprite constructs for road depots. */

#include "road_land.h"

const SpriteID _road_tile_sprites_1[16] = {
	    0, 0x546, 0x545, 0x53B, 0x544, 0x534, 0x53E, 0x539,
	0x543, 0x53C, 0x535, 0x538, 0x53D, 0x537, 0x53A, 0x536
};

const SpriteID _road_frontwire_sprites_1[16] = {
	0, 0x54, 0x55, 0x5B, 0x54, 0x54, 0x5E, 0x5A, 0x55, 0x5C, 0x55, 0x58, 0x5D, 0x57, 0x59, 0x56
};

const SpriteID _road_backpole_sprites_1[16] = {
	0, 0x38, 0x39, 0x40, 0x38, 0x38, 0x43, 0x3E, 0x39, 0x41, 0x39, 0x3C, 0x42, 0x3B, 0x3D, 0x3A
};

#define MAKELINE(a, b, c) { a, b, c },
#define ENDLINE { 0, 0, 0 }

static const DrawRoadTileStruct _roadside_nothing[] = {
	ENDLINE
};

static const DrawRoadTileStruct _roadside_lamps_ne[] = {
	MAKELINE(0x57f,  1,  8) // lamp at ne side
	ENDLINE
};

static const DrawRoadTileStruct _roadside_lamps_sw[] = {
	MAKELINE(0x57e, 14,  8) // lamp at sw side
	ENDLINE
};

static const DrawRoadTileStruct _roadside_lamps_y[] = {
	MAKELINE(0x57f,  1,  8) // lamp at ne side
	MAKELINE(0x57e, 14,  8) // lamp at sw side
	ENDLINE
};

static const DrawRoadTileStruct _roadside_lamps_nw[] = {
	MAKELINE(0x57e,  8,  1) // lamp at nw side
	ENDLINE
};

static const DrawRoadTileStruct _roadside_lamps_se[] = {
	MAKELINE(0x57f,  8, 14) // lamp at se side
	ENDLINE
};

static const DrawRoadTileStruct _roadside_lamps_x[] = {
	MAKELINE(0x57f,  8, 14) // lamp at se side
	MAKELINE(0x57e,  8,  1) // lamp at nw side
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_w[] = {
	MAKELINE(0x1212,  0,  2)
	MAKELINE(0x1212,  3,  9)
	MAKELINE(0x1212, 10, 12)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_y[] = {
	MAKELINE(0x1212,  0,  2)
	MAKELINE(0x1212,  0, 10)
	MAKELINE(0x1212, 12,  2)
	MAKELINE(0x1212, 12, 10)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_s[] = {
	MAKELINE(0x1212, 10,  0)
	MAKELINE(0x1212,  3,  3)
	MAKELINE(0x1212,  0, 10)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_3sw[] = {
	MAKELINE(0x1212,  0,  2)
	MAKELINE(0x1212,  0, 10)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_n[] = {
	MAKELINE(0x1212, 12,  2)
	MAKELINE(0x1212,  9,  9)
	MAKELINE(0x1212,  2, 12)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_x[] = {
	MAKELINE(0x1212,  2,  0)
	MAKELINE(0x1212, 10,  0)
	MAKELINE(0x1212,  2, 12)
	MAKELINE(0x1212, 10, 12)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_3nw[] = {
	MAKELINE(0x1212,  2, 12)
	MAKELINE(0x1212, 10, 12)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_e[] = {
	MAKELINE(0x1212,  2,  0)
	MAKELINE(0x1212,  9,  3)
	MAKELINE(0x1212, 12, 10)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_3ne[] = {
	MAKELINE(0x1212, 12,  2)
	MAKELINE(0x1212, 12, 10)
	ENDLINE
};

static const DrawRoadTileStruct _roadside_trees_3se[] = {
	MAKELINE(0x1212,  2, 0)
	MAKELINE(0x1212, 10, 0)
	ENDLINE
};

#undef MAKELINE
#undef ENDLINE

static const DrawRoadTileStruct * const _roadside_none[16] = {
	_roadside_nothing, _roadside_nothing,
	_roadside_nothing, _roadside_nothing,
	_roadside_nothing, _roadside_nothing,
	_roadside_nothing, _roadside_nothing,
	_roadside_nothing, _roadside_nothing,
	_roadside_nothing, _roadside_nothing,
	_roadside_nothing, _roadside_nothing,
	_roadside_nothing, _roadside_nothing
};

static const DrawRoadTileStruct * const _roadside_lamps[16] = {
	_roadside_nothing,      // ROAD_NONE
	_roadside_nothing,      // ROAD_NW
	_roadside_nothing,      // ROAD_SW
	_roadside_lamps_ne,     // ROAD_W
	_roadside_nothing,      // ROAD_SE
	_roadside_lamps_y,      // ROAD_Y
	_roadside_lamps_nw,     // ROAD_S
	_roadside_lamps_ne,     // ROAD_NW | ROAD_SW | ROAD_SE
	_roadside_nothing,      // ROAD_NE
	_roadside_lamps_se,     // ROAD_N
	_roadside_lamps_x,      // ROAD_X
	_roadside_lamps_se,     // ROAD_NW | ROAD_SW | ROAD_NE
	_roadside_lamps_nw,     // ROAD_E
	_roadside_lamps_sw,     // ROAD_NW | ROAD_SE | ROAD_NE
	_roadside_lamps_nw,     // ROAD_SW | ROAD_SE | ROAD_NE
	_roadside_nothing,      // ROAD_ALL
};

static const DrawRoadTileStruct * const _roadside_trees[16] = {
	_roadside_nothing,      // ROAD_NONE
	_roadside_nothing,      // ROAD_NW
	_roadside_nothing,      // ROAD_SW
	_roadside_trees_w,      // ROAD_W
	_roadside_nothing,      // ROAD_SE
	_roadside_trees_y,      // ROAD_Y
	_roadside_trees_s,      // ROAD_S
	_roadside_trees_3sw,    // ROAD_NW | ROAD_SW | ROAD_SE
	_roadside_nothing,      // ROAD_NE
	_roadside_trees_n,      // ROAD_N
	_roadside_trees_x,      // ROAD_X
	_roadside_trees_3nw,    // ROAD_NW | ROAD_SW | ROAD_NE
	_roadside_trees_e,      // ROAD_E
	_roadside_trees_3ne,    // ROAD_NW | ROAD_SE | ROAD_NE
	_roadside_trees_3se,    // ROAD_SW | ROAD_SE | ROAD_NE
	_roadside_nothing,      // ROAD_ALL
};

const DrawRoadTileStruct * const * const _road_display_table[] = {
	_roadside_none,
	_roadside_none,
	_roadside_none,
	_roadside_lamps,
	_roadside_none,
	_roadside_trees,
};
