/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file signal_func.h Functions related to signals. */

#ifndef SIGNAL_FUNC_H
#define SIGNAL_FUNC_H

#include "map/coord.h"
#include "track_type.h"
#include "direction_type.h"
#include "company_type.h"
#include "pathfinder/pathfinder_type.h"

/** State of the signal segment */
enum SigSegState {
	SIGSEG_NONE,    ///< There was no segment (not usually a returned value)
	SIGSEG_FREE,    ///< Free and has no pre-signal exits or at least one green exit
	SIGSEG_FULL,    ///< Occupied by a train
	SIGSEG_PBS,     ///< Segment is a PBS segment
};

bool IsSignalBufferEmpty();

void AddTrackToSignalBuffer(TileIndex tile, Track track, Owner owner);
void AddSideToSignalBuffer(TileIndex tile, DiagDirection side, Owner owner);
void AddDepotToSignalBuffer(TileIndex tile, Owner owner);
void AddBridgeToSignalBuffer(TileIndex tile, Owner owner);
void AddTunnelToSignalBuffer(TileIndex tile, Owner owner);
void AddPosToSignalBuffer(const PFPos &pos, Owner owner);
SigSegState UpdateSignalsInBuffer();

#endif /* SIGNAL_FUNC_H */
