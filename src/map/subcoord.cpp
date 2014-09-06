/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/subcoord.cpp Tile coordinate system. */

#include "subcoord.h"

extern const InitialSubcoords initial_subcoords [TRACKDIR_END] = {
	{15, 8, DIR_NE},  // TRACKDIR_X_NE
	{ 8, 0, DIR_SE},  // TRACKDIR_Y_SE
	{ 7, 0, DIR_E },  // TRACKDIR_UPPER_E
	{15, 8, DIR_E },  // TRACKDIR_LOWER_E
	{ 8, 0, DIR_S },  // TRACKDIR_LEFT_S
	{ 0, 8, DIR_S },  // TRACKDIR_RIGHT_S
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 8, DIR_SW},  // TRACKDIR_X_SW
	{ 8,15, DIR_NW},  // TRACKDIR_Y_NW
	{ 0, 7, DIR_W },  // TRACKDIR_UPPER_W
	{ 8,15, DIR_W },  // TRACKDIR_LOWER_W
	{15, 7, DIR_N },  // TRACKDIR_LEFT_N
	{ 7,15, DIR_N },  // TRACKDIR_RIGHT_N
};

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
