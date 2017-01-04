/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file elrail_func.h header file for electrified rail specific functions */

#ifndef ELRAIL_FUNC_H
#define ELRAIL_FUNC_H

#include "rail.h"
#include "tile_cmd.h"
#include "transparency.h"

/**
 * Test if a rail type has catenary
 * @param rti Rail type to test
 */
static inline bool HasCatenary (const RailtypeInfo *rti)
{
	return HasBit (rti->flags, RTF_CATENARY);
}

/**
 * Test if a rail type has catenary
 * @param rt Rail type to test
 */
static inline bool HasCatenary(RailType rt)
{
	return HasCatenary (GetRailTypeInfo (rt));
}

/**
 * Test if we should draw rail catenary
 */
static inline bool IsCatenaryDrawn()
{
	return !IsInvisibilitySet(TO_CATENARY) && !_settings_game.vehicle.disable_elrails;
}

/**
 * Test if we should draw rail catenary
 * @param rt Rail type to test
 */
static inline bool HasCatenaryDrawn (const RailtypeInfo *rti)
{
	return HasCatenary (rti) && IsCatenaryDrawn();
}

void DrawCatenary(const TileInfo *ti);

void DrawRailTunnelDepotCatenary (const TileInfo *ti, bool depot,
	DiagDirection dir);

/**
 * Draws wires on a tunnel tile.
 * @param ti The TileInfo to draw the wires for.
 */
static inline void DrawCatenaryOnTunnel (const TileInfo *ti)
{
	DiagDirection dir = GetTunnelBridgeDirection (ti->tile);
	DrawRailTunnelDepotCatenary (ti, false, dir);
}

/**
 * Draws wires on a rail depot tile.
 * @param ti The TileInfo to draw the wires for.
 * @param dir Direction of the depot.
 */
static inline void DrawRailDepotCatenary (const TileInfo *ti, DiagDirection dir)
{
	DrawRailTunnelDepotCatenary (ti, true, dir);
}

void DrawCatenaryOnBridge(const TileInfo *ti);

bool SettingsDisableElrail(int32 p1); ///< _settings_game.disable_elrail callback

#endif /* ELRAIL_FUNC_H */
