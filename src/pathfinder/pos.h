/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file pathfinder/pos.h Path position types. */

#ifndef PATHFINDER_POS_H
#define PATHFINDER_POS_H

#include "../map/coord.h"
#include "../track_type.h"
#include "../track_func.h"

/**
 * Path tile (real map tile)
 */
struct PathMTile {
	TileIndex tile;

	/** Create a PathMTile */
	PathMTile(TileIndex t = INVALID_TILE) : tile(t) { }

	/** Set this tile to another given tile */
	void set (const PathMTile &tile) { *this = tile; }

	/** Set this tile to a given tile */
	void set (TileIndex t) { tile = t; }

	/** Check if this tile is initialised */
	bool is_valid() const { return tile != INVALID_TILE; }

	/** Check if this tile is in a wormhole */
	static bool in_wormhole() { return false; }

	/** Compare with another tile */
	bool operator == (const PathMTile &other) const
	{
		return tile == other.tile;
	}

	/** Compare with another tile */
	bool operator != (const PathMTile &other) const
	{
		return tile != other.tile;
	}
};

/**
 * Path tile (real map tile or virtual tile in wormhole)
 */
struct PathVTile {
	TileIndex tile;
	TileIndex wormhole;

	/** Create a PathVTile */
	PathVTile(TileIndex t = INVALID_TILE, TileIndex w = INVALID_TILE)
		: tile(t), wormhole(w) { }

	/** Set this tile to another given tile */
	void set (const PathVTile &tile) { *this = tile; }

	/** Set this tile to a given tile */
	void set (TileIndex t, TileIndex w = INVALID_TILE)
	{
		tile = t;
		wormhole = w;
	}

	/** Check if this tile is initialised */
	bool is_valid() const { return tile != INVALID_TILE; }

	/** Check if this tile is in a wormhole */
	bool in_wormhole() const { return wormhole != INVALID_TILE; }

	/** Compare with another tile */
	bool operator == (const PathVTile &other) const
	{
		return (tile == other.tile) && (wormhole == other.wormhole);
	}

	/** Compare with another tile */
	bool operator != (const PathVTile &other) const
	{
		return (tile != other.tile) || (wormhole != other.wormhole);
	}
};

/**
 * Path position (tile and trackdir)
 */
template <class PTile>
struct PathPos : PTile {
	typedef PTile PathTile;

	Trackdir td;

	/** Create an empty PathPos */
	PathPos() : PathTile(), td(INVALID_TRACKDIR) { }

	/** Create a PathPos for a given tile and trackdir */
	PathPos(TileIndex t, Trackdir d) : PathTile(t), td(d) { }

	/** Create a PathPos in a wormhole */
	PathPos(TileIndex t, Trackdir d, TileIndex w) : PathTile(t, w), td(d) { }

	/** Set this position to another given position */
	void set (const PathPos &pos)
	{
		PathTile::set(pos);
		td = pos.td;
	}

	/** Set this position to a given tile and trackdir */
	void set (TileIndex t, Trackdir d)
	{
		PathTile::set(t);
		td = d;
	}

	/** Set this position to a given wormhole position */
	void set (TileIndex t, Trackdir d, TileIndex w)
	{
		PathTile::set (t, w);
		td = d;
	}

	/** Set the tile of this position to a given tile */
	void set_tile (TileIndex t)
	{
		PathTile::set(t);
		td = INVALID_TRACKDIR; // trash previous trackdir
	}

	/** Set the tile of this position to a given tile */
	void set_tile (TileIndex t, TileIndex w)
	{
		PathTile::set (t, w);
		td = INVALID_TRACKDIR; // trash previous trackdir
	}

	/** Set the trackdir of this position to a given trackdir */
	void set_trackdir (Trackdir d)
	{
		assert (PathTile::is_valid()); // tile should be already set
		td = d;
	}

	/** Clear the trackdir of this position */
	void clear_trackdir()
	{
		assert (PathTile::is_valid()); // tile should be already set
		td = INVALID_TRACKDIR;
	}

	/** Compare with another PathPos */
	bool operator == (const PathPos &other) const
	{
		return PathTile::operator==(other) && (td == other.td);
	}

	/** Compare with another PathPos */
	bool operator != (const PathPos &other) const
	{
		return PathTile::operator!=(other) || (td != other.td);
	}
};

/**
 * Pathfinder new position; td will be INVALID_TRACKDIR unless trackdirs has exactly one trackdir set
 */
template <class BasePos>
struct PathMPos : BasePos {
	TrackdirBits trackdirs;

	/** Set this position to another given position */
	void set (const PathMPos &pos)
	{
		BasePos::set(pos);
		trackdirs = pos.trackdirs;
	}

	/** Set this position to another given position */
	void set (const BasePos &pos)
	{
		BasePos::set(pos);
		trackdirs = TrackdirToTrackdirBits(pos.td);
	}

	/** Set this position to a given tile and trackdir */
	void set (TileIndex t, Trackdir d)
	{
		BasePos::set(t, d);
		trackdirs = TrackdirToTrackdirBits(d);
	}

	/** Set this position to a given tile and trackdirs */
	void set (TileIndex t, TrackdirBits s)
	{
		BasePos::set(t, HasExactlyOneBit(s) ? FindFirstTrackdir(s) : INVALID_TRACKDIR);
		trackdirs = s;
	}

	/** Set this position to a given wormhole position */
	void set (TileIndex t, Trackdir d, TileIndex w)
	{
		BasePos::set (t, d, w);
		trackdirs = TrackdirToTrackdirBits(d);
	}

	/** Set trackdirs to a given set */
	void set_trackdirs (TrackdirBits s)
	{
		BasePos::set_trackdir (HasExactlyOneBit(s) ? FindFirstTrackdir(s) : INVALID_TRACKDIR);
		trackdirs = s;
	}

	/** Set trackdirs to a single trackdir */
	void set_trackdir (Trackdir d)
	{
		BasePos::set_trackdir (d);
		trackdirs = TrackdirToTrackdirBits(d);
	}

	/** Clear trackdirs */
	void clear_trackdirs()
	{
		BasePos::clear_trackdir();
		trackdirs = TRACKDIR_BIT_NONE;
	}

	/** Check whether the position has no trackdirs */
	bool is_empty() const
	{
		return trackdirs == TRACKDIR_BIT_NONE;
	}

	/** Check whether the position has exactly one trackdir */
	bool is_single() const
	{
		assert (HasExactlyOneBit(trackdirs) == (BasePos::td != INVALID_TRACKDIR));
		return BasePos::td != INVALID_TRACKDIR;
	}
};

/* Pathfinder positions for the various transport types. */
typedef PathPos<PathVTile> RailPathPos;
typedef PathPos<PathMTile> RoadPathPos;
typedef PathPos<PathMTile> ShipPathPos;

#endif /* PATHFINDER_POS_H */
