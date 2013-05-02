/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/bridge.cpp Map accessor functions for bridges. */

#include "../stdafx.h"
#include "bridge.h"

/**
 * Finds the end of a bridge in the specified direction starting at the other end or at a middle tile
 * @param tile the bridge tile to find the bridge ramp for
 * @param dir  the direction to search in
 */
TileIndex GetBridgeEnd(TileIndex tile, DiagDirection dir)
{
	TileIndexDiff delta = TileOffsByDiagDir(dir);

	dir = ReverseDiagDir(dir);
	do {
		tile += delta;
	} while (!IsBridgeHeadTile(tile) || GetTunnelBridgeDirection(tile) != dir);

	return tile;
}
