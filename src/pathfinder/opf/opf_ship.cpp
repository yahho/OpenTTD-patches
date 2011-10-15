/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file opf_ship.cpp Implementation of the oldest supported ship pathfinder. */

#include "../../stdafx.h"
#include "../../tunnelbridge_map.h"
#include "../../tunnelbridge.h"
#include "../../ship.h"
#include "../../core/random_func.hpp"

struct RememberData {
	uint16 cur_length;
	byte depth;
	Track last_choosen_track;
};

struct TrackPathFinder {
	TileIndex skiptile;
	TileIndex dest_coords;
	uint best_bird_dist;
	uint best_length;
	RememberData rd;
	TrackdirByte the_dir;
};

static bool ShipTrackFollower(TileIndex tile, TrackPathFinder *pfs, uint length)
{
	/* Found dest? */
	if (tile == pfs->dest_coords) {
		pfs->best_bird_dist = 0;

		pfs->best_length = minu(pfs->best_length, length);
		return true;
	}

	/* Skip this tile in the calculation */
	if (tile != pfs->skiptile) {
		pfs->best_bird_dist = minu(pfs->best_bird_dist, DistanceMaxPlusManhattan(pfs->dest_coords, tile));
	}

	return false;
}

static void TPFModeShip(TrackPathFinder *tpf, TileIndex tile, DiagDirection direction)
{
	if (IsAqueductTile(tile)) {
		DiagDirection dir = GetTunnelBridgeDirection(tile);
		/* entering aqueduct? */
		if (dir == direction) {
			TileIndex endtile = GetOtherBridgeEnd(tile);

			tpf->rd.cur_length += GetTunnelBridgeLength(tile, endtile) + 1;

			tile = endtile;
		} else {
			/* leaving tunnel / bridge? */
			if (ReverseDiagDir(dir) != direction) return;
		}
	}

	/* This addition will sometimes overflow by a single tile.
	 * The use of TILE_MASK here makes sure that we still point at a valid
	 * tile, and then this tile will be in the sentinel row/col, so GetTileTrackStatus will fail. */
	tile = TILE_MASK(tile + TileOffsByDiagDir(direction));

	if (++tpf->rd.cur_length > 50) return;

	TrackBits bits = TrackStatusToTrackBits(GetTileTrackStatus(tile, TRANSPORT_WATER, 0)) & DiagdirReachesTracks(direction);
	if (bits == TRACK_BIT_NONE) return;

	assert(TileX(tile) != MapMaxX() && TileY(tile) != MapMaxY());

	bool only_one_track = true;
	do {
		Track track = RemoveFirstTrack(&bits);
		if (bits != TRACK_BIT_NONE) only_one_track = false;
		RememberData rd = tpf->rd;

		/* Change direction 4 times only */
		if (!only_one_track && track != tpf->rd.last_choosen_track) {
			if (++tpf->rd.depth > 4) {
				tpf->rd = rd;
				return;
			}
			tpf->rd.last_choosen_track = track;
		}

		tpf->the_dir = TrackEnterdirToTrackdir(track, direction);

		if (!ShipTrackFollower(tile, tpf, tpf->rd.cur_length)) {
			TPFModeShip(tpf, tile, TrackdirToExitdir(tpf->the_dir));
		}

		tpf->rd = rd;
	} while (bits != TRACK_BIT_NONE);
}

static void OPFShipFollowTrack(TileIndex tile, DiagDirection direction, TrackPathFinder *tpf)
{
	assert(IsValidDiagDirection(direction));

	/* initialize path finder variables */
	tpf->rd.cur_length = 0;
	tpf->rd.depth = 0;
	tpf->rd.last_choosen_track = INVALID_TRACK;

	ShipTrackFollower(tile, tpf, 0);
	TPFModeShip(tpf, tile, direction);
}

/** Track to "direction (& 3)" mapping. */
static const byte _pick_shiptrack_table[6] = {DIR_NE, DIR_SE, DIR_E, DIR_E, DIR_N, DIR_N};

static uint FindShipTrack(const Ship *v, TileIndex tile, DiagDirection dir, TrackdirBits bits, TileIndex skiptile, Trackdir *trackdir)
{
	TrackPathFinder pfs;
	uint best_bird_dist = 0;
	uint best_length    = 0;
	byte ship_dir = v->direction & 3;

	pfs.dest_coords = v->dest_tile;
	pfs.skiptile = skiptile;

	Trackdir best_trackdir = INVALID_TRACKDIR;

	do {
		Trackdir td = RemoveFirstTrackdir(&bits);

		pfs.best_bird_dist = UINT_MAX;
		pfs.best_length = UINT_MAX;

		OPFShipFollowTrack(tile, TrackdirToExitdir(td), &pfs);

		if (best_trackdir != INVALID_TRACKDIR) {
			if (pfs.best_bird_dist != 0) {
				/* neither reached the destination, pick the one with the smallest bird dist */
				if (pfs.best_bird_dist > best_bird_dist) goto bad;
				if (pfs.best_bird_dist < best_bird_dist) goto good;
			} else {
				if (pfs.best_length > best_length) goto bad;
				if (pfs.best_length < best_length) goto good;
			}

			/* if we reach this position, there's two paths of equal value so far.
			 * pick one randomly. */
			uint r = GB(Random(), 0, 8);
			if (_pick_shiptrack_table[TrackdirToTrack(td)] == ship_dir) r += 80;
			if (_pick_shiptrack_table[TrackdirToTrack(best_trackdir)] == ship_dir) r -= 80;
			if (r <= 127) goto bad;
		}
good:;
		best_trackdir = td;
		best_bird_dist = pfs.best_bird_dist;
		best_length = pfs.best_length;
bad:;

	} while (bits != 0);

	*trackdir = best_trackdir;
	return best_bird_dist;
}

/**
 * returns the trackdir to choose on the next tile, or -1 when it's better to
 * reverse. The tile given is the tile we are about to enter, enterdir is the
 * direction in which we are entering the tile
 */
Trackdir OPFShipChooseTrack(const Ship *v, TileIndex tile, DiagDirection enterdir, TrackdirBits trackdirs, bool &path_found)
{
	assert(IsValidDiagDirection(enterdir));
	assert(TILE_ADD(tile, -TileOffsByDiagDir(enterdir)) == v->tile);

	/* Let's find out how far it would be if we would reverse first */
	Trackdir trackdir = v->GetPos().td;
	assert(HasBit(DiagdirReachesTrackdirs(ReverseDiagDir(enterdir)), ReverseTrackdir(trackdir)));
	TrackdirBits b = TrackStatusToTrackdirBits(GetTileTrackStatus(v->tile, TRANSPORT_WATER, 0)) & TrackdirToTrackdirBits(ReverseTrackdir(trackdir));

	uint distr = UINT_MAX; // distance if we reversed
	if (b != 0) {
		distr = FindShipTrack(v, v->tile, ReverseDiagDir(enterdir), b, tile, &trackdir);
		if (distr != UINT_MAX) distr++; // penalty for reversing
	}

	/* And if we would not reverse? */
	uint dist = FindShipTrack(v, tile, enterdir, trackdirs, 0, &trackdir);

	/* Due to the way this pathfinder works we cannot determine whether we're lost or not. */
	path_found = true;
	if (dist <= distr) return trackdir;
	return INVALID_TRACKDIR; // We could better reverse
}
