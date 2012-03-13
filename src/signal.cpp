/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file signal.cpp functions related to rail signals updating */

#include "stdafx.h"
#include "debug.h"
#include "station_map.h"
#include "tunnelbridge_map.h"
#include "vehicle_func.h"
#include "viewport_func.h"
#include "train.h"
#include "company_base.h"
#include "depot_map.h"


/** these are the maximums used for updating signal blocks */
static const uint SIG_TBU_SIZE    =  64; ///< number of signals entering to block
static const uint SIG_TBD_SIZE    = 256; ///< number of intersections - open nodes in current block
static const uint SIG_GLOB_SIZE   = 128; ///< number of open blocks (block can be opened more times until detected)
static const uint SIG_GLOB_UPDATE =  64; ///< how many items need to be in _globset to force update

assert_compile(SIG_GLOB_UPDATE <= SIG_GLOB_SIZE);

/** incidating trackbits with given enterdir */
static const TrackBits _enterdir_to_trackbits[DIAGDIR_END] = {
	TRACK_BIT_3WAY_NE,
	TRACK_BIT_3WAY_SE,
	TRACK_BIT_3WAY_SW,
	TRACK_BIT_3WAY_NW
};

/** incidating trackdirbits with given enterdir */
static const TrackdirBits _enterdir_to_trackdirbits[DIAGDIR_END] = {
	TRACKDIR_BIT_X_SW | TRACKDIR_BIT_UPPER_W | TRACKDIR_BIT_RIGHT_S,
	TRACKDIR_BIT_Y_NW | TRACKDIR_BIT_LOWER_W | TRACKDIR_BIT_RIGHT_N,
	TRACKDIR_BIT_X_NE | TRACKDIR_BIT_LOWER_E | TRACKDIR_BIT_LEFT_N,
	TRACKDIR_BIT_Y_SE | TRACKDIR_BIT_UPPER_E | TRACKDIR_BIT_LEFT_S
};

/**
 * Set containing up to 'maxitems' items of 'T'
 * No tree structure is used because it would cause
 * slowdowns in most usual cases
 */
template <typename T, uint maxitems>
struct SmallSet {
private:
	uint n;           // actual number of units
	bool overflowed;  // did we try to overflow the set?
	const char *name; // name, used for debugging purposes...
	T data[maxitems]; // elements of set

public:
	/** Constructor - just set default values and 'name' */
	SmallSet(const char *name) : n(0), overflowed(false), name(name) { }

	/** Reset variables to default values */
	void Reset()
	{
		this->n = 0;
		this->overflowed = false;
	}

	/**
	 * Returns value of 'overflowed'
	 * @return did we try to overflow the set?
	 */
	bool Overflowed()
	{
		return this->overflowed;
	}

	/**
	 * Checks for empty set
	 * @return is the set empty?
	 */
	bool IsEmpty()
	{
		return this->n == 0;
	}

	/**
	 * Checks for full set
	 * @return is the set full?
	 */
	bool IsFull()
	{
		return this->n == lengthof(data);
	}

	/**
	 * Reads the number of items
	 * @return current number of items
	 */
	uint Items()
	{
		return this->n;
	}


	/**
	 * Tries to remove first instance of given item
	 * @param item item to remove
	 * @return element was found and removed
	 */
	bool Remove(const T &item)
	{
		for (uint i = 0; i < this->n; i++) {
			if (this->data[i] == item) {
				this->data[i] = this->data[--this->n];
				return true;
			}
		}

		return false;
	}

	/**
	 * Tries to find given item in the set
	 * @param item item to find
	 * @return true iff the item was found
	 */
	bool IsIn(const T &item)
	{
		for (uint i = 0; i < this->n; i++) {
			if (this->data[i] == item) return true;
		}

		return false;
	}

	/**
	 * Adds item into the set, checks for full set
	 * Sets the 'overflowed' flag if the set was full
	 * @param item item to add
	 * @return true iff the item could be added (set wasn't full)
	 */
	bool Add(const T &item)
	{
		if (this->IsFull()) {
			overflowed = true;
			DEBUG(misc, 0, "SignalSegment too complex. Set %s is full (maximum %d)", name, maxitems);
			return false; // set is full
		}

		this->data[this->n] = item;
		this->n++;

		return true;
	}

	/**
	 * Reads the last added element into the set
	 * @param item pointer where the item is written to
	 * @return false iff the set was empty
	 */
	bool Get(T *item)
	{
		if (this->n == 0) return false;

		this->n--;
		*item = this->data[this->n];

		return true;
	}
};


struct SignalPos {
	TileIndex tile;
	Trackdir td;

	bool operator == (const SignalPos &other) const {
		return (tile == other.tile) && (td == other.td);
	}
};

static SignalPos SignalPosFrom(TileIndex tile, Trackdir td)
{
	SignalPos pos = { tile, td };
	return pos;
}

struct SignalSide {
	TileIndex tile;
	DiagDirection side;

	bool operator == (const SignalSide &other) const {
		return (tile == other.tile) && (side == other.side);
	}
};

static SignalSide SignalSideFrom(TileIndex tile, DiagDirection side)
{
	SignalSide ss = { tile, side };
	return ss;
}

static SmallSet<SignalPos,  SIG_TBU_SIZE>  _tbuset("_tbuset");   ///< set of signals that will be updated
static SmallSet<SignalSide, SIG_TBD_SIZE>  _tbdset("_tbdset");   ///< set of open nodes in current signal block
static SmallSet<SignalSide, SIG_GLOB_SIZE> _globset("_globset"); ///< set of places to be updated in following runs
static Owner _owner = INVALID_OWNER; ///< owner of tracks in _globset, or INVALID_OWNER if empty


/** Check whether there is a train on rail, not in a depot */
static Vehicle *TrainOnTileEnum(Vehicle *v, void *)
{
	if (v->type != VEH_TRAIN || Train::From(v)->trackdir == TRACKDIR_DEPOT) return NULL;

	return v;
}


/**
 * Perform some operations before adding data into Todo set
 * The new and reverse direction is removed from Global set, because we are sure
 * it doesn't need to be checked again
 * Also, remove reverse direction from Todo set
 * This is the 'core' part so the graph searching won't enter any tile twice
 *
 * @param ss1 tile and side we are entering
 * @param ss2 tile and side we are leaving
 * @return false iff the Todo buffer would be overrun
 */
static inline bool MaybeAddToTodoSet(const SignalSide &ss1, const SignalSide &ss2)
{
	_globset.Remove(ss1); // it can be in Global but not in Todo
	_globset.Remove(ss2); // remove in all cases

	assert(!_tbdset.IsIn(ss1)); // it really shouldn't be there already

	if (_tbdset.Remove(ss2)) return true;

	return _tbdset.Add(ss1);
}


/** Current signal block state flags */
enum SigFlags {
	SF_NONE   = 0,
	SF_TRAIN  = 1 << 0, ///< train found in segment
	SF_EXIT   = 1 << 1, ///< exitsignal found
	SF_EXIT2  = 1 << 2, ///< two or more exits found
	SF_GREEN  = 1 << 3, ///< green exitsignal found
	SF_GREEN2 = 1 << 4, ///< two or more green exits found
	SF_FULL   = 1 << 5, ///< some of buffers was full, do not continue
	SF_PBS    = 1 << 6, ///< pbs signal found
};

DECLARE_ENUM_AS_BIT_SET(SigFlags)


/**
 * Search signal block
 *
 * @param owner owner whose signals we are updating
 * @return SigFlags
 */
static SigFlags ExploreSegment(Owner owner)
{
	SigFlags flags = SF_NONE;

	SignalSide ss;

	while (_tbdset.Get(&ss)) {
		SignalSide newss;
		newss.tile = ss.tile; // tile we will enter

		switch (GetTileType(ss.tile)) {
			case TT_RAILWAY:
				if (GetTileOwner(ss.tile) != owner) continue; // do not propagate signals on others' tiles (remove for tracksharing)

				if (IsTileSubtype(ss.tile, TT_TRACK)) {
					TrackBits tracks = GetTrackBits(ss.tile); // trackbits of tile
					TrackBits tracks_masked = (TrackBits)(tracks & _enterdir_to_trackbits[ss.side]); // only incidating trackbits

					if (tracks == TRACK_BIT_HORZ || tracks == TRACK_BIT_VERT) { // there is exactly one incidating track, no need to check
						tracks = tracks_masked;
						/* If no train detected yet, and there is not no train -> there is a train -> set the flag */
						if (!(flags & SF_TRAIN) && EnsureNoTrainOnTrackBits(ss.tile, tracks).Failed()) flags |= SF_TRAIN;
					} else {
						if (tracks_masked == TRACK_BIT_NONE) continue; // no incidating track
						if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
					}

					assert((tracks_masked & ~tracks) == TRACK_BIT_NONE); // tracks_masked must be a subset of tracks
					assert(tracks_masked != TRACK_BIT_NONE);
					assert(tracks_masked != TRACK_BIT_HORZ);
					assert(tracks_masked != TRACK_BIT_VERT);
					assert(tracks != TRACK_BIT_HORZ);
					assert(tracks != TRACK_BIT_VERT);

					// tile can only have signals if it only has one bit
					if (HasAtMostOneBit(tracks)) {
						Track track = TrackBitsToTrack(tracks); // get the only track
						if (HasSignalOnTrack(ss.tile, track)) { // now check whole track, not trackdir
							SignalType sig = GetSignalType(ss.tile, track);
							Trackdir trackdir = (Trackdir)FindFirstBit(TrackBitsToTrackdirBits(tracks) & _enterdir_to_trackdirbits[ss.side]);
							Trackdir reversedir = ReverseTrackdir(trackdir);
							/* add (tile, reversetrackdir) to 'to-be-updated' set when there is
							 * ANY conventional signal in REVERSE direction
							 * (if it is a presignal EXIT and it changes, it will be added to 'to-be-done' set later) */
							if (HasSignalOnTrackdir(ss.tile, reversedir)) {
								if (IsPbsSignal(sig)) {
									flags |= SF_PBS;
								} else if (!_tbuset.Add(SignalPosFrom(ss.tile, reversedir))) {
									return flags | SF_FULL;
								}
							}
							if (HasSignalOnTrackdir(ss.tile, trackdir) && !IsOnewaySignal(ss.tile, track)) flags |= SF_PBS;

							/* if it is a presignal EXIT in OUR direction and we haven't found 2 green exits yes, do special check */
							if (!(flags & SF_GREEN2) && IsPresignalExit(ss.tile, track) && HasSignalOnTrackdir(ss.tile, trackdir)) { // found presignal exit
								if (flags & SF_EXIT) flags |= SF_EXIT2; // found two (or more) exits
								flags |= SF_EXIT; // found at least one exit - allow for compiler optimizations
								if (GetSignalStateByTrackdir(ss.tile, trackdir) == SIGNAL_STATE_GREEN) { // found green presignal exit
									if (flags & SF_GREEN) flags |= SF_GREEN2;
									flags |= SF_GREEN;
								}
							}

							continue;
						}
					}

					DiagDirection enterdir = ss.side;
					for (ss.side = DIAGDIR_BEGIN; ss.side < DIAGDIR_END; ss.side++) { // test all possible exit directions
						if (ss.side != enterdir && (tracks & _enterdir_to_trackbits[ss.side])) { // any track incidating?
							newss.tile = ss.tile + TileOffsByDiagDir(ss.side);  // new tile to check
							newss.side = ReverseDiagDir(ss.side); // direction we are entering from
							if (!MaybeAddToTodoSet(newss, ss)) return flags | SF_FULL;
						}
					}

					continue; // continue the while() loop
				} else {
					DiagDirection dir = GetTunnelBridgeDirection(ss.tile);

					if (ss.side == INVALID_DIAGDIR) { // incoming from the wormhole
						if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
						ss.side = ReverseDiagDir(dir); // exitdir
						newss.tile += TileOffsByDiagDir(ss.side); // just skip to next tile
						newss.side = dir;
					} else { // NOT incoming from the wormhole!
						if (ReverseDiagDir(ss.side) != dir) continue;
						if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
						ss.side = INVALID_DIAGDIR;
						newss.tile = GetOtherTunnelBridgeEnd(ss.tile); // just skip to exit tile
						newss.side = INVALID_DIAGDIR;
					}
				}
				break;

			case TT_MISC:
				if (GetTileOwner(ss.tile) != owner) continue;

				switch (GetTileSubtype(ss.tile)) {
					default: continue;

					case TT_MISC_CROSSING:
						if (DiagDirToAxis(ss.side) == GetCrossingRoadAxis(ss.tile)) continue; // different axis
						if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
						newss.side = ss.side;
						ss.side = ReverseDiagDir(ss.side); // exitdir
						newss.tile += TileOffsByDiagDir(ss.side);
						break;

					case TT_MISC_TUNNEL: {
						if (GetTunnelTransportType(ss.tile) != TRANSPORT_RAIL) continue;
						DiagDirection dir = GetTunnelBridgeDirection(ss.tile);

						if (ss.side == INVALID_DIAGDIR) { // incoming from the wormhole
							if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
							ss.side = ReverseDiagDir(dir); // exitdir
							newss.tile += TileOffsByDiagDir(ss.side); // just skip to next tile
							newss.side = dir;
						} else { // NOT incoming from the wormhole!
							if (ReverseDiagDir(ss.side) != dir) continue;
							if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
							ss.side = INVALID_DIAGDIR;
							newss.tile = GetOtherTunnelBridgeEnd(ss.tile); // just skip to exit tile
							newss.side = INVALID_DIAGDIR;
						}
						break;
					}

					case TT_MISC_DEPOT:
						if (!IsRailDepot(ss.tile)) continue;
						if (ss.side == INVALID_DIAGDIR) { // from 'inside' - train just entered or left the depot
							if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
							ss.side = GetGroundDepotDirection(ss.tile); // exitdir
							newss.tile += TileOffsByDiagDir(ss.side);
							newss.side = ReverseDiagDir(ss.side);
							break;
						} else if (ss.side == GetGroundDepotDirection(ss.tile)) { // entered a depot
							if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
						}
						continue;
				}
				break;

			case TT_STATION:
				if (!HasStationRail(ss.tile)) continue;
				if (GetTileOwner(ss.tile) != owner) continue;
				if (DiagDirToAxis(ss.side) != GetRailStationAxis(ss.tile)) continue; // different axis
				if (IsStationTileBlocked(ss.tile)) continue; // 'eye-candy' station tile

				if (!(flags & SF_TRAIN) && HasVehicleOnPos(ss.tile, NULL, &TrainOnTileEnum)) flags |= SF_TRAIN;
				newss.side = ss.side;
				ss.side = ReverseDiagDir(ss.side); // exitdir
				newss.tile += TileOffsByDiagDir(ss.side);
				break;

			default:
				continue; // continue the while() loop
		}

		if (!MaybeAddToTodoSet(newss, ss)) return flags | SF_FULL;
	}

	return flags;
}


/**
 * Update signals around segment in _tbuset
 *
 * @param flags info about segment
 */
static void UpdateSignalsAroundSegment(SigFlags flags)
{
	SignalPos pos;

	while (_tbuset.Get(&pos)) {
		assert(HasSignalOnTrackdir(pos.tile, pos.td));

		SignalType sig = GetSignalType(pos.tile, TrackdirToTrack(pos.td));
		SignalState newstate = SIGNAL_STATE_GREEN;

		/* determine whether the new state is red */
		if (flags & SF_TRAIN) {
			/* train in the segment */
			newstate = SIGNAL_STATE_RED;
		} else {
			/* is it a bidir combo? - then do not count its other signal direction as exit */
			if (sig == SIGTYPE_COMBO && HasSignalOnTrackdir(pos.tile, ReverseTrackdir(pos.td))) {
				/* at least one more exit */
				if ((flags & SF_EXIT2) &&
						/* no green exit */
						(!(flags & SF_GREEN) ||
						/* only one green exit, and it is this one - so all other exits are red */
						(!(flags & SF_GREEN2) && GetSignalStateByTrackdir(pos.tile, ReverseTrackdir(pos.td)) == SIGNAL_STATE_GREEN))) {
					newstate = SIGNAL_STATE_RED;
				}
			} else { // entry, at least one exit, no green exit
				if (IsPresignalEntry(pos.tile, TrackdirToTrack(pos.td)) && (flags & SF_EXIT) && !(flags & SF_GREEN)) newstate = SIGNAL_STATE_RED;
			}
		}

		/* only when the state changes */
		if (newstate != GetSignalStateByTrackdir(pos.tile, pos.td)) {
			if (IsPresignalExit(pos.tile, TrackdirToTrack(pos.td))) {
				/* for pre-signal exits, add block to the global set */
				DiagDirection exitdir = TrackdirToExitdir(ReverseTrackdir(pos.td));
				_globset.Add(SignalSideFrom(pos.tile, exitdir)); // do not check for full global set, first update all signals
			}
			SetSignalStateByTrackdir(pos.tile, pos.td, newstate);
			MarkTileDirtyByTile(pos.tile);
		}
	}

}


/** Reset all sets after one set overflowed */
static inline void ResetSets()
{
	_tbuset.Reset();
	_tbdset.Reset();
	_globset.Reset();
}


/**
 * Updates blocks in _globset buffer
 *
 * @param owner company whose signals we are updating
 * @return state of the first block from _globset
 * @pre _globset.IsEmpty() || Company::IsValidID(_owner)
 */
SigSegState UpdateSignalsInBuffer()
{
	assert(_globset.IsEmpty() || Company::IsValidID(_owner));

	SigSegState state = SIGSEG_NONE; // value to return

	SignalSide ss;

	while (_globset.Get(&ss)) {
		assert(_tbuset.IsEmpty());
		assert(_tbdset.IsEmpty());

		/* After updating signal, data stored are always railway with signals.
		 * Other situations happen when data are from outside functions -
		 * modification of railbits (including both rail building and removal),
		 * train entering/leaving block, train leaving depot...
		 */
		switch (GetTileType(ss.tile)) {
			case TT_RAILWAY:
				if (IsTileSubtype(ss.tile, TT_TRACK)) goto check_track;
			bridge_head:
				assert(ss.side == INVALID_DIAGDIR || ss.side == ReverseDiagDir(GetTunnelBridgeDirection(ss.tile)));
				_tbdset.Add(SignalSideFrom(ss.tile, INVALID_DIAGDIR));  // we can safely start from wormhole centre
				_tbdset.Add(SignalSideFrom(GetOtherTunnelBridgeEnd(ss.tile), INVALID_DIAGDIR));
				break;

			case TT_MISC:
				if (IsTunnelTile(ss.tile)) {
					/* 'optimization assert' - do not try to update signals when it is not needed */
					assert(GetTunnelTransportType(ss.tile) == TRANSPORT_RAIL);
					goto bridge_head;
				}
				if (IsRailDepotTile(ss.tile)) {
					/* 'optimization assert' do not try to update signals in other cases */
					assert(ss.side == INVALID_DIAGDIR || ss.side == GetGroundDepotDirection(ss.tile));
					_tbdset.Add(SignalSideFrom(ss.tile, INVALID_DIAGDIR)); // start from depot inside
					break;
				}
				if (!IsLevelCrossingTile(ss.tile)) goto next_tile;
				/* fall through */
			case TT_STATION:
			check_track:
				if ((TrackStatusToTrackBits(GetTileTrackStatus(ss.tile, TRANSPORT_RAIL, 0)) & _enterdir_to_trackbits[ss.side]) != TRACK_BIT_NONE) {
					/* only add to set when there is some 'interesting' track */
					_tbdset.Add(ss);
					_tbdset.Add(SignalSideFrom(ss.tile + TileOffsByDiagDir(ss.side), ReverseDiagDir(ss.side)));
					break;
				}
				/* FALL THROUGH */
			default:
			next_tile:
				/* jump to next tile */
				ss.tile = ss.tile + TileOffsByDiagDir(ss.side);
				ss.side = ReverseDiagDir(ss.side);
				if ((TrackStatusToTrackBits(GetTileTrackStatus(ss.tile, TRANSPORT_RAIL, 0)) & _enterdir_to_trackbits[ss.side]) != TRACK_BIT_NONE) {
					_tbdset.Add(ss);
					break;
				}
				/* happens when removing a rail that wasn't connected at one or both sides */
				continue; // continue the while() loop
		}

		assert(!_tbdset.Overflowed()); // it really shouldn't overflow by these one or two items
		assert(!_tbdset.IsEmpty()); // it wouldn't hurt anyone, but shouldn't happen too

		SigFlags flags = ExploreSegment(_owner);

		if (state == SIGSEG_NONE) {
			if (flags & SF_PBS) {
				state = SIGSEG_PBS;
			} else if ((flags & SF_TRAIN) || ((flags & SF_EXIT) && !(flags & SF_GREEN)) || (flags & SF_FULL)) {
				state = SIGSEG_FULL;
			} else {
				state = SIGSEG_FREE;
			}
		}

		/* do not do anything when some buffer was full */
		if (flags & SF_FULL) {
			ResetSets(); // free all sets
			break;
		}

		UpdateSignalsAroundSegment(flags);
	}

	_owner = INVALID_OWNER;

	return state;
}


/**
 * Check if signal buffer is empty
 * @returns whether the signal buffer is empty
 */
bool IsSignalBufferEmpty()
{
	return _globset.IsEmpty();
}

/**
 * Set signal buffer owner
 */
static inline void SetBufferOwner(Owner owner)
{
	/* do not allow signal updates for two companies in one run */
	assert(_globset.IsEmpty() || owner == _owner);
	_owner = owner;
}

/**
 * Update signals in buffer if it has too many items
 */
static inline void UpdateSignalsInBufferAuto()
{
	if (_globset.Items() >= SIG_GLOB_UPDATE) {
		/* too many items, force update */
		UpdateSignalsInBuffer();
	}
}


/**
 * Add track to signal update buffer
 *
 * @param tile tile where we start
 * @param track track at which ends we will update signals
 * @param owner owner whose signals we will update
 */
void AddTrackToSignalBuffer(TileIndex tile, Track track, Owner owner)
{
	SetBufferOwner(owner);

	_globset.Add(SignalSideFrom(tile, TrackdirToExitdir(TrackToTrackdir(track))));
	_globset.Add(SignalSideFrom(tile, TrackdirToExitdir(ReverseTrackdir(TrackToTrackdir(track)))));

	UpdateSignalsInBufferAuto();
}


/**
 * Add side of tile to signal update buffer
 *
 * @param tile tile where we start
 * @param side side of tile
 * @param owner owner whose signals we will update
 */
void AddSideToSignalBuffer(TileIndex tile, DiagDirection side, Owner owner)
{
	SetBufferOwner(owner);

	_globset.Add(SignalSideFrom(tile, side));

	UpdateSignalsInBufferAuto();
}
