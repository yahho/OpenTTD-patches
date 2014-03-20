/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf.h Entry point for OpenTTD to YAPF. */

#ifndef YAPF_H
#define YAPF_H

#include "../../direction_type.h"
#include "../../track_type.h"
#include "../../vehicle_type.h"
#include "../types.h"

/** Length (penalty) of one tile with YAPF */
static const int YAPF_TILE_LENGTH = 100;

/** Length (penalty) of a corner with YAPF */
static const int YAPF_TILE_CORNER_LENGTH = 71;

/**
 * This penalty is the equivalent of "infinite", which means that paths that
 * get this penalty will be chosen, but only if there is no other route
 * without it. Be careful with not applying this penalty to often, or the
 * total path cost might overflow..
 */
static const int YAPF_INFINITE_PENALTY = 1000 * YAPF_TILE_LENGTH;

/**
 * Finds the best path for given ship using YAPF.
 * @param v        the ship that needs to find a path
 * @param tile     the tile to find the path from (should be next tile the ship is about to enter)
 * @param enterdir diagonal direction which the ship will enter this new tile from
 * @param trackdirs available trackdirs on the new tile (to choose from)
 * @param path_found [out] Whether a path has been found (true) or has been guessed (false)
 * @return         the best trackdir for next turn or INVALID_TRACKDIR if the path could not be found
 */
Trackdir YapfShipChooseTrack(const Ship *v, TileIndex tile, DiagDirection enterdir, TrackdirBits trackdirs, bool &path_found);

/**
 * Returns true if it is better to reverse the ship before leaving depot using YAPF.
 * @param v the ship leaving the depot
 * @return true if reversing is better
 */
bool YapfShipCheckReverse(const Ship *v);

/**
 * Find the nearest depot to a ship
 * @param v the ship looking for a depot
 * @param max_distance maximum allowed distance, or 0 for any distance
 * @return the tile of the nearest depot
 */
TileIndex YapfShipFindNearestDepot (const Ship *v, uint max_distance);

/**
 * Finds the best path for given road vehicle using YAPF.
 * @param v         the RV that needs to find a path
 * @param tile      the tile to find the path from (should be next tile the RV is about to enter)
 * @param enterdir  diagonal direction which the RV will enter this new tile from
 * @param trackdirs available trackdirs on the new tile (to choose from)
 * @param path_found [out] Whether a path has been found (true) or has been guessed (false)
 * @return          the best trackdir for next turn or INVALID_TRACKDIR if the path could not be found
 */
Trackdir YapfRoadVehicleChooseTrack(const RoadVehicle *v, TileIndex tile, DiagDirection enterdir, TrackdirBits trackdirs, bool &path_found);

/**
 * Finds the best path for given train using YAPF.
 * @param v        the train that needs to find a path
 * @param origin   the end of the current reservation
 * @param reserve_track indicates whether YAPF should try to reserve the found path
 * @param target   [out] the target tile of the reservation, free is set to true if path was reserved
 * @return         the best trackdir for next turn
 */
Trackdir YapfTrainChooseTrack(const Train *v, const RailPathPos &origin, bool reserve_track, struct PFResult *target);

/**
 * Used when user sends road vehicle to the nearest depot or if road vehicle needs servicing using YAPF.
 * @param v            vehicle that needs to go to some depot
 * @param max_penalty  max distance (in pathfinder penalty) from the current vehicle position
 *                     (used also as optimization - the pathfinder can stop path finding if max_penalty
 *                     was reached and no depot was seen)
 * @return             depot tile found, or INVALID_TILE if no depot was found
 */
TileIndex YapfRoadVehicleFindNearestDepot(const RoadVehicle *v, uint max_penalty);

/**
 * Used when user sends train to the nearest depot or if train needs servicing using YAPF.
 * @param v            train that needs to go to some depot
 * @param max_distance max distance (int pathfinder penalty) from the current train position
 *                     (used also as optimization - the pathfinder can stop path finding if max_penalty
 *                     was reached and no depot was seen)
 * @param res          pointer to store the data about the depot
 * @return             whether a depot was found
 */
bool YapfTrainFindNearestDepot(const Train *v, uint max_distance, FindDepotData *res);

/**
 * Returns true if it is better to reverse the train before leaving station using YAPF.
 * @param v the train leaving the station
 * @return true if reversing is better
 */
bool YapfTrainCheckReverse(const Train *v);

/**
 * Try to extend the reserved path of a train to the nearest safe tile using YAPF.
 *
 * @param v    The train that needs to find a safe tile.
 * @param pos  Last position of the current reserved path.
 * @param override_railtype Should all physically compatible railtypes be searched, even if the vehicle can't run on them on its own?
 * @return True if the path could be extended to a safe tile.
 */
bool YapfTrainFindNearestSafeTile(const Train *v, const RailPathPos &pos, bool override_railtype);

/**
 * Use this function to notify YAPF that track layout (or signal configuration) has change.
 * @param tile  the tile that is changed
 * @param track what piece of track is changed
 */
void YapfNotifyTrackLayoutChange(TileIndex tile, Track track);

#endif /* YAPF_H */
