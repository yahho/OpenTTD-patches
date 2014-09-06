/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/subcoord.cpp Tile coordinate system. */

#include "subcoord.h"

const FullPosTile::DeltaCoord FullPosTile::delta_coord [DIR_END] = {
	{-1,-1}, // DIR_N
	{-1, 0}, // DIR_NE
	{-1, 1}, // DIR_E
	{ 0, 1}, // DIR_SE
	{ 1, 1}, // DIR_S
	{ 1, 0}, // DIR_SW
	{ 1,-1}, // DIR_W
	{ 0,-1}, // DIR_NW
};
