/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road_land.h Sprite constructs for road depots. */

#include "../stdafx.h"
#include "../sprite.h"

struct DrawRoadTileStruct {
	uint16 image;
	byte subcoord_x;
	byte subcoord_y;
};

extern const DrawTileSprites _road_depot[], _tram_depot[], _crossing_layout;

extern const SpriteID _road_tile_sprites_1[16];
extern const SpriteID _road_frontwire_sprites_1[16];
extern const SpriteID _road_backpole_sprites_1[16];

extern const DrawRoadTileStruct * const * const _road_display_table[];
