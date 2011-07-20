/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file bridge_cmd.cpp
 * This file deals with bridges (non-gui stuff)
 */

#include "stdafx.h"
#include "bridge.h"
#include "bridge_map.h"
#include "landscape.h"
#include "slope_func.h"
#include "command_func.h"
#include "date_func.h"

#include "table/sprites.h"
#include "table/strings.h"
#include "table/bridge_land.h"


BridgeSpec _bridge[MAX_BRIDGES]; ///< The specification of all bridges.

/** Reset the data been eventually changed by the grf loaded. */
void ResetBridges()
{
	/* First, free sprite table data */
	for (BridgeType i = 0; i < MAX_BRIDGES; i++) {
		if (_bridge[i].sprite_table != NULL) {
			for (BridgePieces j = BRIDGE_PIECE_NORTH; j < BRIDGE_PIECE_INVALID; j++) free(_bridge[i].sprite_table[j]);
			free(_bridge[i].sprite_table);
		}
	}

	/* Then, wipe out current bridges */
	memset(&_bridge, 0, sizeof(_bridge));
	/* And finally, reinstall default data */
	memcpy(&_bridge, &_orig_bridge, sizeof(_orig_bridge));
}

/**
 * Calculate the price factor for building a long bridge.
 * Basically the cost delta is 1,1, 1, 2,2, 3,3,3, 4,4,4,4, 5,5,5,5,5, 6,6,6,6,6,6,  7,7,7,7,7,7,7,  8,8,8,8,8,8,8,8,
 * @param length Length of the bridge.
 * @return Price factor for the bridge.
 */
int CalcBridgeLenCostFactor(int length)
{
	if (length <= 2) return length;

	length -= 2;
	int sum = 2;

	int delta;
	for (delta = 1; delta < length; delta++) {
		sum += delta * delta;
		length -= delta;
	}
	sum += delta * length;
	return sum;
}

/**
 * Get the foundation for a bridge.
 * @param tileh The slope to build the bridge on.
 * @param axis The axis of the bridge entrance.
 * @return The foundation required.
 */
Foundation GetBridgeFoundation(Slope tileh, Axis axis)
{
	if (tileh == SLOPE_FLAT ||
			((tileh == SLOPE_NE || tileh == SLOPE_SW) && axis == AXIS_X) ||
			((tileh == SLOPE_NW || tileh == SLOPE_SE) && axis == AXIS_Y)) return FOUNDATION_NONE;

	return (HasSlopeHighestCorner(tileh) ? InclinedFoundation(axis) : FlatteningFoundation(tileh));
}

/**
 * Determines if the track on a bridge ramp is flat or goes up/down.
 *
 * @param tileh Slope of the tile under the bridge head
 * @param axis Orientation of bridge
 * @return true iff the track is flat.
 */
bool HasBridgeFlatRamp(Slope tileh, Axis axis)
{
	ApplyFoundationToSlope(GetBridgeFoundation(tileh, axis), &tileh);
	/* If the foundation slope is flat the bridge has a non-flat ramp and vice versa. */
	return (tileh != SLOPE_FLAT);
}

/**
 * Is a bridge of the specified type and length available?
 * @param bridge_type Wanted type of bridge.
 * @param bridge_len  Wanted length of the bridge.
 * @return A succeeded (the requested bridge is available) or failed (it cannot be built) command.
 */
CommandCost CheckBridgeAvailability(BridgeType bridge_type, uint bridge_len, DoCommandFlag flags)
{
	if (flags & DC_QUERY_COST) {
		if (bridge_len <= _settings_game.construction.max_bridge_length) return CommandCost();
		return_cmd_error(STR_ERROR_BRIDGE_TOO_LONG);
	}

	if (bridge_type >= MAX_BRIDGES) return CMD_ERROR;

	const BridgeSpec *b = GetBridgeSpec(bridge_type);
	if (b->avail_year > _cur_year) return CMD_ERROR;

	uint max = min(b->max_length, _settings_game.construction.max_bridge_length);

	if (b->min_length > bridge_len) return CMD_ERROR;
	if (bridge_len <= max) return CommandCost();
	return_cmd_error(STR_ERROR_BRIDGE_TOO_LONG);
}
