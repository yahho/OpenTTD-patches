/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file station_gui.h Contains enums and function declarations connected with stations GUI */

#ifndef STATION_GUI_H
#define STATION_GUI_H

#include "command_type.h"
#include "map/tilearea.h"
#include "window_type.h"


/** Types of cargo to display for station coverage. */
enum StationCoverageType {
	SCT_PASSENGERS_ONLY,     ///< Draw only passenger class cargoes.
	SCT_NON_PASSENGERS_ONLY, ///< Draw all non-passenger class cargoes.
	SCT_ALL,                 ///< Draw all cargoes.
};

int DrawStationCoverageAreaText(int left, int right, int top, StationCoverageType sct, int rad, bool supplies);
void CheckRedrawStationCoverage(const Window *w);

void ShowSelectBaseStationIfNeeded (const CommandContainer &cmd, const TileArea &ta, bool waypoint);

/**
 * Show the station selection window when needed. If not, build the station.
 * @param cmd Command to build the station.
 * @param ta Area to build the station in
 */
static inline void ShowSelectStationIfNeeded (const CommandContainer &cmd, const TileArea &ta)
{
	ShowSelectBaseStationIfNeeded (cmd, ta, false);
}

/**
 * Show the waypoint selection window when needed. If not, build the waypoint.
 * @param cmd Command to build the waypoint.
 * @param ta Area to build the waypoint in
 */
static inline void ShowSelectWaypointIfNeeded (const CommandContainer &cmd, const TileArea &ta)
{
	ShowSelectBaseStationIfNeeded (cmd, ta, true);
}

#endif /* STATION_GUI_H */
