/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file bridge_map.cpp Map accessor functions for bridges. */

#include "stdafx.h"
#include "landscape.h"
#include "map/common.h"
#include "map/slope.h"
#include "bridge.h"

/**
 * Get the height ('z') of a bridge.
 * @param tile the bridge ramp tile to get the bridge height from
 * @return the height of the bridge.
 */
int GetBridgeHeight(TileIndex t)
{
	int h;
	Slope tileh = GetTileSlope(t, &h);
	Foundation f = GetBridgeFoundation(tileh, DiagDirToAxis(GetTunnelBridgeDirection(t)));

	/* one height level extra for the ramp */
	return h + 1 + ApplyFoundationToSlope(f, &tileh);
}
