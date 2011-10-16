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

#include "rail_type.h"
#include "depot_type.h"
#include "signal_func.h"
#include "track_func.h"
#include "tile_map.h"
#include "signal_type.h"
#include "bridge.h"


/**
 * Is this tile rail tile and a rail depot?
 * @param t the tile to get the information from
 * @return true if and only if the tile is a rail depot
 */
static inline bool IsRailDepotTile(TileIndex t)
{
	return IsGroundDepotTile(t) && !HasBit(_mc[t].m1, 5);
}

/**
 * Gets the rail type of the given tile
 * @param t the tile to get the rail type from
 * @return the rail type of the tile
 */
static inline RailType GetRailType(TileIndex t, Track track = INVALID_TRACK)
{
	if (IsRailwayTile(t) && (track == TRACK_LOWER || track == TRACK_RIGHT)) {
		return (RailType)GB(_mc[t].m5, 0, 4);
	} else {
		return (RailType)GB(_mc[t].m3, 0, 4);
	}
}

/**
 * Sets the rail type of the given tile
 * @param t the tile to set the rail type of
 * @param r the new rail type for the tile
 */
static inline void SetRailType(TileIndex t, RailType r, Track track = INVALID_TRACK)
{
	if (!IsRailwayTile(t)) {
		assert(track == INVALID_TRACK);
		SB(_mc[t].m3, 0, 4, r);
	} else {
		switch (track) {
			case INVALID_TRACK:
				SB(_mc[t].m3, 0, 4, r);
				SB(_mc[t].m5, 0, 4, r);
				break;
			case TRACK_LOWER:
			case TRACK_RIGHT:
				SB(_mc[t].m5, 0, 4, r);
				break;
			default:
				SB(_mc[t].m3, 0, 4, r);
				break;
		}
	}
}


/**
 * Gets the track bits of the given tile
 * @param tile the tile to get the track bits from
 * @return the track bits of the tile
 */
static inline TrackBits GetTrackBits(TileIndex tile)
{
	assert(IsRailwayTile(tile));
	return (TrackBits)GB(_mc[tile].m2, 0, 6);
}

/**
 * Sets the track bits of the given tile
 * @param t the tile to set the track bits of
 * @param b the new track bits for the tile
 */
static inline void SetTrackBits(TileIndex t, TrackBits b)
{
	assert(IsRailwayTile(t));
	SB(_mc[t].m2, 0, 6, b);
}

/**
 * Returns whether the given track is present on the given tile.
 * @param tile  the tile to check the track presence of
 * @param track the track to search for on the tile
 * @pre IsRailwayTile(tile)
 * @return true if and only if the given track exists on the tile
 */
static inline bool HasTrack(TileIndex tile, Track track)
{
	assert(IsRailwayTile(tile));
	return HasBit(GetTrackBits(tile), track);
}

/**
 * Returns the track of a depot, ignoring direction
 * @pre IsRailDepotTile(t)
 * @param t the tile to get the depot track from
 * @return the track of the depot
 */
static inline Track GetRailDepotTrack(TileIndex t)
{
	assert(IsRailDepotTile(t));
	return DiagDirToDiagTrack(GetGroundDepotDirection(t));
}


/**
 * Gets the rail type of the rail inciding on a given tile side
 * @param t the tile to get the rail type from
 * @param dir the side to get the track to check
 * @return the rail type of the tracks in the tile inciding on the given side, or INVALID_RAILTYPE if there are none
 */
static inline RailType GetSideRailType(TileIndex t, DiagDirection dir)
{
	TrackBits trackbits = GetTrackBits(t) & DiagdirReachesTracks(ReverseDiagDir(dir));
	if (trackbits == TRACK_BIT_NONE) return INVALID_RAILTYPE;
	return GetRailType(t, FindFirstTrack(trackbits));
}


/**
 * Returns the reserved track bits of the tile
 * @pre IsRailwayTile(t)
 * @param t the tile to query
 * @return the track bits
 */
static inline TrackBits GetRailReservationTrackBits(TileIndex t)
{
	assert(IsRailwayTile(t));
	byte track_b = GB(_mc[t].m2, 8, 3);
	if (track_b == 0) return TRACK_BIT_NONE;
	Track track = (Track)(track_b - 1);    // map array saves Track+1
	return (TrackBits)(TrackToTrackBits(track) | (HasBit(_mc[t].m2, 11) ? TrackToTrackBits(TrackToOppositeTrack(track)) : 0));
}

/**
 * Sets the reserved track bits of the tile
 * @pre IsRailwayTile(t) && !TracksOverlap(b)
 * @param t the tile to change
 * @param b the track bits
 */
static inline void SetTrackReservation(TileIndex t, TrackBits b)
{
	assert(IsRailwayTile(t));
	assert(b != INVALID_TRACK_BIT);
	assert(!TracksOverlap(b));
	Track track = RemoveFirstTrack(&b);
	SB(_mc[t].m2, 8, 3, track == INVALID_TRACK ? 0 : track + 1);
	SB(_mc[t].m2, 11, 1, (byte)(b != TRACK_BIT_NONE));
}

/**
 * Try to reserve a specific track on a tile
 * @pre IsRailwayTile(t) && HasTrack(tile, t)
 * @param tile the tile
 * @param t the rack to reserve
 * @return true if successful
 */
static inline bool TryReserveTrack(TileIndex tile, Track t)
{
	assert(IsRailwayTile(tile));
	assert(HasTrack(tile, t));
	TrackBits bits = TrackToTrackBits(t);
	TrackBits res = GetRailReservationTrackBits(tile);
	if ((res & bits) != TRACK_BIT_NONE) return false;  // already reserved
	res |= bits;
	if (TracksOverlap(res)) return false;  // crossing reservation present
	SetTrackReservation(tile, res);
	return true;
}

/**
 * Lift the reservation of a specific track on a tile
 * @pre IsRailwayTile(t) && HasTrack(tile, t)
 * @param tile the tile
 * @param t the track to free
 */
static inline void UnreserveTrack(TileIndex tile, Track t)
{
	assert(IsRailwayTile(tile));
	assert(HasTrack(tile, t));
	TrackBits res = GetRailReservationTrackBits(tile);
	res &= ~TrackToTrackBits(t);
	SetTrackReservation(tile, res);
}

/**
 * Get the reservation state of the depot
 * @pre IsRailDepotTile(t)
 * @param t the depot tile
 * @return reservation state
 */
static inline bool HasDepotReservation(TileIndex t)
{
	assert(IsRailDepotTile(t));
	return HasBit(_mc[t].m5, 4);
}

/**
 * Set the reservation state of the depot
 * @pre IsRailDepotTile(t)
 * @param t the depot tile
 * @param b the reservation state
 */
static inline void SetDepotReservation(TileIndex t, bool b)
{
	assert(IsRailDepotTile(t));
	SB(_mc[t].m5, 4, 1, (byte)b);
}

/**
 * Get the reserved track bits for a depot
 * @pre IsRailDepotTile(t)
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetDepotReservationTrackBits(TileIndex t)
{
	assert(IsRailDepotTile(t));
	return HasDepotReservation(t) ? TrackToTrackBits(GetRailDepotTrack(t)) : TRACK_BIT_NONE;
}


static inline byte *SignalByte(TileIndex t, Track track)
{
	assert(track < TRACK_END); // do not use this for INVALID_TRACK
	return (track == TRACK_LOWER || track == TRACK_RIGHT) ? &_mc[t].m7 : &_mc[t].m4;
}

/**
 * Clear signals on a track
 * @param tile  the tile to clear the signals from
 * @param track the track to clear the signals from
 */
static inline void ClearSignals(TileIndex tile, Track track)
{
	if (track == INVALID_TRACK) {
		_mc[tile].m4 = _mc[tile].m7 = 0;
	} else {
		*SignalByte(tile, track) = 0;
	}
}

/**
 * Set whether the given signals are present (Along/AgainstTrackDir)
 * @param tile    the tile to set the present signals for
 * @param track   the track to set the present signals for
 * @param signals the signals that have to be present
 */
static inline void SetPresentSignals(TileIndex tile, Track track, uint signals)
{
	SB(*SignalByte(tile, track), 2, 2, signals);
}

/**
 * Get whether the given signals are present (Along/AgainstTrackDir)
 * @param tile the tile to get the present signals for
 * @param track the track to get the present signals for
 * @return the signals that are present
 */
static inline uint GetPresentSignals(TileIndex tile, Track track)
{
	return GB(*SignalByte(tile, track), 2, 2);
}

/**
 * Checks for the presence of signals (either way) on the given track on the
 * given rail tile.
 */
static inline bool HasSignalOnTrack(TileIndex tile, Track track)
{
	assert(IsValidTrack(track));
	return GetPresentSignals(tile, track) != 0;
}

static inline bool IsPbsSignal(SignalType s)
{
	return s == SIGTYPE_PBS || s == SIGTYPE_PBS_ONEWAY;
}

static inline SignalType GetSignalType(TileIndex t, Track track)
{
	assert(HasSignalOnTrack(t, track));
	return (SignalType)GB(*SignalByte(t, track), 4, 3);
}

static inline void SetSignalType(TileIndex t, Track track, SignalType s)
{
	assert(HasSignalOnTrack(t, track));
	if (track == INVALID_TRACK) {
		SB(_mc[t].m4, 4, 3, s);
		SB(_mc[t].m7, 4, 3, s);
	} else {
		SB(*SignalByte(t, track), 4, 3, s);
	}
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

static inline void CycleSignalSide(TileIndex t, Track track)
{
	byte *p = SignalByte(t, track);
	byte sig = GB(*p, 2, 2);
	if (--sig == 0) sig = IsPbsSignal(GetSignalType(t, track)) ? 2 : 3;
	SB(*p, 2, 2, sig);
}

static inline SignalVariant GetSignalVariant(TileIndex t, Track track)
{
	return (SignalVariant)GB(*SignalByte(t, track), 7, 1);
}

static inline void SetSignalVariant(TileIndex t, Track track, SignalVariant v)
{
	if (track == INVALID_TRACK) {
		SB(_mc[t].m4, 7, 1, v);
		SB(_mc[t].m7, 7, 1, v);
	} else {
		SB(*SignalByte(t, track), 7, 1, v);
	}
}

/**
 * Set the states of the signals (Along/AgainstTrackDir)
 * @param tile  the tile to set the states for
 * @param track the track to set the states for
 * @param state the new state
 */
static inline void SetSignalStates(TileIndex tile, Track track, uint state)
{
	SB(*SignalByte(tile, track), 0, 2, state);
}

/**
 * Get the states of the signals (Along/AgainstTrackDir)
 * @param tile  the tile to get the states for
 * @param track the tile to get the states for
 * @return the state of the signals
 */
static inline uint GetSignalStates(TileIndex tile, Track track)
{
	return GB(*SignalByte(tile, track), 0, 2);
}

/**
 * Check if the given trackdir is stored in the high bit of its track
 * @param trackdir the trackdir to check
 * @return whether the trackdir data is stored in the high bit
 */
static inline bool IsSignalHighBit(Trackdir trackdir)
{
	/* return 1 for trackdirs 0 1 2 3 12 13 */
	/* return 0 for trackdirs 4 5 8 9 10 11 */
	return HasBit(trackdir + 0xC, 3);
}

/**
 * Signal bit to use to check presence and state
 * @param trackdir the trackdir to check
 * @return the bit to be used (1 or 2)
 */
static inline uint SignalBit(Trackdir trackdir)
{
	return IsSignalHighBit(trackdir) ? 2 : 1;
}

/**
 * Checks for the presence of signals along the given trackdir on the given
 * rail tile.
 *
 * Along meaning if you are currently driving on the given trackdir, this is
 * the signal that is facing us (for which we stop when it's red).
 */
static inline bool HasSignalOnTrackdir(TileIndex tile, Trackdir trackdir)
{
	assert (IsValidTrackdir(trackdir));
	return (*SignalByte(tile, TrackdirToTrack(trackdir)) & (IsSignalHighBit(trackdir) ? 0x08 : 0x04)) != 0;
}

/**
 * Gets the state of the signal along the given trackdir.
 *
 * Along meaning if you are currently driving on the given trackdir, this is
 * the signal that is facing us (for which we stop when it's red).
 */
static inline SignalState GetSignalStateByTrackdir(TileIndex tile, Trackdir trackdir)
{
	assert(IsValidTrackdir(trackdir));
	assert(HasSignalOnTrack(tile, TrackdirToTrack(trackdir)));
	return (*SignalByte(tile, TrackdirToTrack(trackdir)) & (IsSignalHighBit(trackdir) ? 0x02 : 0x01)) != 0 ?
		SIGNAL_STATE_GREEN : SIGNAL_STATE_RED;
}

/**
 * Sets the state of the signal along the given trackdir.
 */
static inline void SetSignalStateByTrackdir(TileIndex tile, Trackdir trackdir, SignalState state)
{
	if (state == SIGNAL_STATE_GREEN) { // set 1
		*SignalByte(tile, TrackdirToTrack(trackdir)) |= (IsSignalHighBit(trackdir) ? 0x02 : 0x01);
	} else {
		*SignalByte(tile, TrackdirToTrack(trackdir)) &= (IsSignalHighBit(trackdir) ? 0xFD : 0xFE);
	}
}


RailType GetTileRailType(TileIndex tile, Track track = INVALID_TRACK);

/** The ground 'under' the rail */
enum RailGroundType {
	RAIL_GROUND_BARREN       =  0, ///< Nothing (dirt)
	RAIL_GROUND_GRASS        =  1, ///< Grassy
	RAIL_GROUND_FENCE_NW     =  2, ///< Grass with a fence at the NW edge
	RAIL_GROUND_FENCE_SE     =  3, ///< Grass with a fence at the SE edge
	RAIL_GROUND_FENCE_SENW   =  4, ///< Grass with a fence at the NW and SE edges
	RAIL_GROUND_FENCE_NE     =  5, ///< Grass with a fence at the NE edge
	RAIL_GROUND_FENCE_SW     =  6, ///< Grass with a fence at the SW edge
	RAIL_GROUND_FENCE_NESW   =  7, ///< Grass with a fence at the NE and SW edges
	RAIL_GROUND_FENCE_VERT1  =  8, ///< Grass with a fence at the eastern side
	RAIL_GROUND_FENCE_VERT2  =  9, ///< Grass with a fence at the western side
	RAIL_GROUND_FENCE_HORIZ1 = 10, ///< Grass with a fence at the southern side
	RAIL_GROUND_FENCE_HORIZ2 = 11, ///< Grass with a fence at the northern side
	RAIL_GROUND_ICE_DESERT   = 12, ///< Icy or sandy
	RAIL_GROUND_WATER        = 13, ///< Grass with a fence and shore or water on the free halftile
	RAIL_GROUND_HALF_SNOW    = 14, ///< Snow only on higher part of slope (steep or one corner raised)
};

static inline void SetRailGroundType(TileIndex t, RailGroundType rgt)
{
	assert(IsNormalRailTile(t));
	SB(_mc[t].m3, 4, 4, rgt);
}

static inline RailGroundType GetRailGroundType(TileIndex t)
{
	assert(IsNormalRailTile(t));
	return (RailGroundType)GB(_mc[t].m3, 4, 4);
}


/**
 * Determines the type of rail bridge on a tile
 * @param t The tile to analyze
 * @pre IsRailBridgeTile(t)
 * @return The bridge type
 */
static inline BridgeType GetRailBridgeType(TileIndex t)
{
	assert(IsRailBridgeTile(t));
	return GB(_mc[t].m2, 12, 4);
}

/**
 * Set the type of rail bridge on a tile
 * @param t The tile to set
 * @param type The type to set
 */
static inline void SetRailBridgeType(TileIndex t, BridgeType type)
{
	assert(IsRailBridgeTile(t));
	SB(_mc[t].m2, 12, 4, type);
}

/**
 * Check if a rail bridge is an extended bridge head
 * @param t The tile to check
 * @return Whether there are track bits set other than the axis track bit
 */
static inline bool IsExtendedRailBridge(TileIndex t)
{
	assert(IsRailBridgeTile(t));
	return GetTrackBits(t) != DiagDirToDiagTrackBits(GetTunnelBridgeDirection(t));
}


static inline void MakeRailNormal(TileIndex t, Owner o, TrackBits b, RailType r)
{
	SetTileTypeSubtype(t, TT_RAILWAY, TT_TRACK);
	SetTileOwner(t, o);
	SB(_mc[t].m0, 2, 2, 0);
	_mc[t].m2 = b;
	_mc[t].m3 = r;
	_mc[t].m4 = 0;
	_mc[t].m5 = r;
	_mc[t].m7 = 0;
}

/**
 * Make a bridge ramp for rails.
 * @param t          the tile to make a bridge ramp
 * @param o          the new owner of the bridge ramp
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @param r          the rail type of the bridge
 */
static inline void MakeRailBridgeRamp(TileIndex t, Owner o, BridgeType bridgetype, DiagDirection d, RailType r)
{
	SetTileTypeSubtype(t, TT_RAILWAY, TT_BRIDGE);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, o);
	_mc[t].m2 = (bridgetype << 12) | DiagDirToDiagTrackBits(d);
	_mc[t].m3 = (d << 6) | r;
	_mc[t].m4 = 0;
	_mc[t].m5 = 0;
	_mc[t].m7 = 0;
}

static inline void MakeRailDepot(TileIndex t, Owner o, DepotID did, DiagDirection d, RailType r)
{
	SetTileTypeSubtype(t, TT_MISC, TT_MISC_DEPOT);
	ClrBit(_mc[t].m1, 5);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, o);
	_mc[t].m2 = did;
	_mc[t].m3 = r;
	_mc[t].m4 = 0;
	_mc[t].m5 = d;
	_mc[t].m7 = 0;
}

#endif /* RAIL_MAP_H */
