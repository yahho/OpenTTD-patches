/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile_type.h Types related to tiles. */

#ifndef TILE_TYPE_H
#define TILE_TYPE_H

#include "tile/tile.h"

static const uint TILE_SIZE      = 16;            ///< Tiles are 16x16 "units" in size
static const uint TILE_UNIT_MASK = TILE_SIZE - 1; ///< For masking in/out the inner-tile units.
static const uint TILE_PIXELS    = 32;            ///< a tile is 32x32 pixels
static const uint TILE_HEIGHT    =  8;            ///< The standard height-difference between tiles on two levels is 8 (z-diff 8)

#endif /* TILE_TYPE_H */
