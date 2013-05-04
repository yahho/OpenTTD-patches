/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rail_map.h Hides the direct accesses to the map array with map accessors */

#ifndef RAIL_MAP_H
#define RAIL_MAP_H

#include "tile/misc.h"
#include "map/rail.h"
#include "rail_type.h"
#include "depot_type.h"
#include "signal_func.h"
#include "track_func.h"
#include "signal_type.h"
#include "bridge.h"


static inline bool IsPbsSignal(SignalType s)
{
	return s == SIGTYPE_PBS || s == SIGTYPE_PBS_ONEWAY;
}

static inline bool IsPresignalEntry(TileIndex t, Track track)
{
	return GetSignalType(t, track) == SIGTYPE_ENTRY || GetSignalType(t, track) == SIGTYPE_COMBO;
}

static inline bool IsPresignalExit(TileIndex t, Track track)
{
	return GetSignalType(t, track) == SIGTYPE_EXIT || GetSignalType(t, track) == SIGTYPE_COMBO;
}

/** One-way signals can't be passed the 'wrong' way. */
static inline bool IsOnewaySignal(TileIndex t, Track track)
{
	return GetSignalType(t, track) != SIGTYPE_PBS;
}

#endif /* RAIL_MAP_H */
