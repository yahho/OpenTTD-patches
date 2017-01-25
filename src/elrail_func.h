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
static inline bool HasRailCatenary (const RailtypeInfo *rti)
{
	return HasBit (rti->flags, RTF_CATENARY);
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
static inline bool HasRailCatenaryDrawn (const RailtypeInfo *rti)
{
	return HasRailCatenary (rti) && IsCatenaryDrawn();
}

void DrawRailwayCatenary (const TileInfo *ti);

void DrawRailBridgeHeadCatenary (const TileInfo *ti, const RailtypeInfo *rti,
	DiagDirection dir);

void DrawRailAxisCatenary (const TileInfo *ti, const RailtypeInfo *rti,
	Axis axis, bool draw_pylons = true, bool draw_wire = true);

void DrawRailTunnelDepotCatenary (const TileInfo *ti, const RailtypeInfo *rti,
	bool depot, DiagDirection dir);

void DrawRailTunnelCatenary (const TileInfo *ti, DiagDirection dir);

/**
 * Draws wires on a rail depot tile.
 * @param ti The TileInfo to draw the wires for.
 * @param rti The rail type information of the rail.
 * @param dir Direction of the depot.
 */
static inline void DrawRailDepotCatenary (const TileInfo *ti,
	const RailtypeInfo *rti, DiagDirection dir)
{
	DrawRailTunnelDepotCatenary (ti, rti, true, dir);
}

void DrawRailCatenaryOnBridge(const TileInfo *ti);

bool SettingsDisableElrail(int32 p1); ///< _settings_game.disable_elrail callback

#endif /* ELRAIL_FUNC_H */
