/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_tunnel.cpp Implementation of ScriptTunnel. */

#include "../../stdafx.h"
#include "script_tunnel.hpp"
#include "script_rail.hpp"
#include "../script_instance.hpp"
#include "../../map/class.h"
#include "../../map/common.h"
#include "../../map/tunnel.h"
#include "../../map/slope.h"

/* static */ bool ScriptTunnel::IsTunnelTile(TileIndex tile)
{
	if (!::IsValidTile(tile)) return false;
	return ::IsTunnelTile(tile);
}

/* static */ TileIndex ScriptTunnel::GetOtherTunnelEnd(TileIndex tile)
{
	if (!::IsValidTile(tile)) return INVALID_TILE;

	/* If it's a tunnel already, take the easy way out! */
	if (IsTunnelTile(tile)) return ::GetOtherTunnelEnd(tile);

	int start_z;
	Slope start_tileh = ::GetTileSlope(tile, &start_z);
	DiagDirection direction = ::GetInclinedSlopeDirection(start_tileh);
	if (direction == INVALID_DIAGDIR) return INVALID_TILE;

	TileIndexDiff delta = ::TileOffsByDiagDir(direction);
	int end_z;
	do {
		tile += delta;
		if (!::IsValidTile(tile)) return INVALID_TILE;

		::GetTileSlope(tile, &end_z);
	} while (start_z != end_z);

	return tile;
}

/**
 * Helper function to connect a just built tunnel to nearby roads.
 * @param instance The script instance we have to built the road for.
 * @param far Connect the far end of the tunnel.
 * @param callback Function to call after the command is run.
 */
static void callback_tunnel (ScriptInstance *instance, bool far,
	Script_SuspendCallbackProc *callback)
{
	assert (ScriptObject::GetActiveInstance() == instance);

	/* Build the piece of road on the 'end' side of the tunnel */
	TileIndex tile = instance->GetCallbackVariable (0);
	if (far) tile = ::GetOtherTunnelEnd (tile);

	DiagDirection dir_1 = ::GetTunnelBridgeDirection (tile);
	DiagDirection dir_2 = ::ReverseDiagDir (dir_1);

	if (!instance->DoCommand (tile + ::TileOffsByDiagDir(dir_2),
			::DiagDirToRoadBits(dir_1) | (instance->GetRoadType() << 4),
			0, CMD_BUILD_ROAD, NULL, callback)) {
		ScriptInstance::DoCommandReturn(instance);
		return;
	}

	/* This can never happen, as in test-mode this callback is never executed,
	 *  and in execute-mode, the other callback is called. */
	NOT_REACHED();
}

/**
 * Helper function to connect a just built tunnel to nearby roads.
 * @param instance The script instance we have to built the road for.
 */
static void callback_tunnel2 (ScriptInstance *instance)
{
	return callback_tunnel (instance, false, NULL);
}

/**
 * Helper function to connect a just built tunnel to nearby roads.
 * @param instance The script instance we have to built the road for.
 */
static void callback_tunnel1 (ScriptInstance *instance)
{
	return callback_tunnel (instance, true, &callback_tunnel2);
}

/* static */ bool ScriptTunnel::BuildTunnel(ScriptVehicle::VehicleType vehicle_type, TileIndex start)
{
	EnforcePrecondition(false, ::IsValidTile(start));
	EnforcePrecondition(false, vehicle_type == ScriptVehicle::VT_RAIL || vehicle_type == ScriptVehicle::VT_ROAD);
	EnforcePrecondition(false, vehicle_type != ScriptVehicle::VT_RAIL || ScriptRail::IsRailTypeAvailable(ScriptRail::GetCurrentRailType()));
	EnforcePrecondition(false, vehicle_type != ScriptVehicle::VT_ROAD || ScriptRoad::IsRoadTypeAvailable(ScriptRoad::GetCurrentRoadType()));
	EnforcePrecondition(false, ScriptObject::GetCompany() != OWNER_DEITY || vehicle_type == ScriptVehicle::VT_ROAD);

	uint type = 0;
	if (vehicle_type == ScriptVehicle::VT_ROAD) {
		type |= (TRANSPORT_ROAD << 8);
		type |= ::RoadTypeToRoadTypes((::RoadType)ScriptObject::GetRoadType());
	} else {
		type |= (TRANSPORT_RAIL << 8);
		type |= ScriptRail::GetCurrentRailType();
	}

	/* For rail we do nothing special */
	if (vehicle_type == ScriptVehicle::VT_RAIL) {
		return ScriptObject::DoCommand(start, type, 0, CMD_BUILD_TUNNEL);
	}

	ScriptObject::SetCallbackVariable(0, start);
	return ScriptObject::DoCommand(start, type, 0, CMD_BUILD_TUNNEL, NULL, &callback_tunnel1);
}

/* static */ bool ScriptTunnel::RemoveTunnel(TileIndex tile)
{
	EnforcePrecondition(false, ScriptObject::GetCompany() != OWNER_DEITY);
	EnforcePrecondition(false, IsTunnelTile(tile));

	return ScriptObject::DoCommand(tile, 0, 0, CMD_LANDSCAPE_CLEAR);
}
