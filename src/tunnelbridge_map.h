/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tunnelbridge_map.h Functions that have tunnels and bridges in common */

#ifndef TUNNELBRIDGE_MAP_H
#define TUNNELBRIDGE_MAP_H

#include "bridge_map.h"
#include "map/tunnel.h"


/**
 * Determines type of the wormhole and returns its other end
 * @param t one end
 * @pre IsTunnelTile(t) || IsBridgeHeadTile(t)
 * @return other end
 */
static inline TileIndex GetOtherTunnelBridgeEnd(TileIndex t)
{
	assert(IsTunnelTile(t) || IsBridgeHeadTile(t));
	return IsTunnelTile(t) ? GetOtherTunnelEnd(t) : GetOtherBridgeEnd(t);
}


#endif /* TUNNELBRIDGE_MAP_H */
