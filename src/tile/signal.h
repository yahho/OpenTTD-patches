/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile/signal.h Tile functions for signals in tiles. */

#ifndef TILE_SIGNAL_H
#define TILE_SIGNAL_H

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "../signal_type.h"

/** Storage of a signal pair on a track in the map array. */
typedef byte SignalPair;


/*
 * Storage of signals in a byte:
 * - bit 0: signal state against trackdir (green/red)
 * - bit 1: signal state along trackdir (green/red)
 * - bit 2: signal present against trackdir
 * - bit 3: signal present along trackdir
 * - bits 4-6: signal type (normal, path, etc.)
 * - bit 7: signal variant (semaphore/electric)
 * (Each track must have a canonical 'along' and 'against' trackdir.)
 */


/**
 * Clear signals on a track
 * @param s The signal pair to clear
 */
static inline void signalpair_clear(SignalPair *s)
{
	*s = 0;
}


/**
 * Get signals present on a track
 * @param s The signal pair to check
 * @return A bitmask of present signals (bit 0 is against, bit 1 is along)
 */
static inline uint signalpair_get_present(const SignalPair *s)
{
	return GB(*s, 2, 2);
}

/**
 * Set signals present on a track
 * @param s The signal pair to set
 * @param mask A bitmask of present signals (bit 0 is against, bit 1 is along)
 */
static inline void signalpair_set_present(SignalPair *s, uint mask)
{
	assert(mask <= 3);
	assert(mask != 0); // use signalpair_clear to clear signals
	SB(*s, 2, 2, mask);
}

/**
 * Check if a track has signals at all
 * @param s The signal pair to check
 * @return Whether the track has any signals
 */
static inline bool signalpair_has_signals(const SignalPair *s)
{
	return signalpair_get_present(s) != 0;
}

/**
 * Check if a track has a signal on a particular direction (along/against)
 * @param s The signal pair to check
 * @param along Check along trackdir, else against
 * @return Whether there is a signal on the given trackdir
 */
static inline bool signalpair_has_signal(const SignalPair *s, bool along)
{
	return (*s & (along ? 0x08 : 0x04)) != 0;
}


/**
 * Get signal states on a track
 * @param s The signal pair to check
 * @return A bitmask of signal states (bit 0 is against, bit 1 is along)
 */
static inline uint signalpair_get_states(const SignalPair *s)
{
	return GB(*s, 0, 2);
}

/**
 * Set signal states on a track
 * @param s The signal pair to set
 * @param mask A bitmask of signal states (bit 0 is against, bit 1 is along)
 */
static inline void signalpair_set_states(SignalPair *s, uint mask)
{
	assert(mask <= 3);
	SB(*s, 0, 2, mask);
}

/**
 * Get the signal state on a trackdir
 * @param s The signal pair to check
 * @param along Check along trackdir, else against
 * @return The signal state on the given trackdir
 */
static inline SignalState signalpair_get_state(const SignalPair *s, bool along)
{
	assert(signalpair_has_signal(s, along));
	return (*s & (along ? 0x02 : 0x01)) != 0 ? SIGNAL_STATE_GREEN : SIGNAL_STATE_RED;
}

/**
 * Set the signal state on a trackdir
 * @param s The signal pair to set
 * @param along Set along trackdir, else against
 * @param state The state to set
 */
static inline void signalpair_set_state(SignalPair *s, bool along, SignalState state)
{
	assert(signalpair_has_signal(s, along));
	if (state == SIGNAL_STATE_GREEN) {
		*s |= along ?  0x02 :  0x01;
	} else {
		*s &= along ? ~0x02 : ~0x01;
	}
}


/**
 * Get the type of the signals on a track
 * @param s The signal pair to check
 * @return The type of the signals on the track
 */
static inline SignalType signalpair_get_type(const SignalPair *s)
{
	assert(signalpair_has_signals(s));
	return (SignalType)GB(*s, 4, 3);
}

/**
 * Set the type of the signals on a track
 * @param s The signal pair to set
 * @param type The type of signals to set
 */
static inline void signalpair_set_type(SignalPair *s, SignalType type)
{
	assert(signalpair_has_signals(s));
	SB(*s, 4, 3, type);
}


/**
 * Get the variant of the signals on a track
 * @param s The signal pair to check
 * @return The variant of the signals on the track
 */
static inline SignalVariant signalpair_get_variant(const SignalPair *s)
{
	assert(signalpair_has_signals(s));
	return (SignalVariant)GB(*s, 7, 1);
}

/**
 * Set the variant of the signals on a track
 * @param s The signal pair to set
 * @param v The variant of signals to set
 */
static inline void signalpair_set_variant(SignalPair *s, SignalVariant v)
{
	assert(signalpair_has_signals(s));
	SB(*s, 7, 1, v);
}

#endif /* TILE_SIGNAL_H */
