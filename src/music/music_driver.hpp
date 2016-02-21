/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file music_driver.hpp Base for all music playback. */

#ifndef MUSIC_MUSIC_DRIVER_HPP
#define MUSIC_MUSIC_DRIVER_HPP

#include "../driver.h"

/** Driver for all music playback. */
class MusicDriver : public Driver, public SharedDriverSystem <MusicDriver> {
public:
	static char *ini; ///< The music driver as stored in the configuration file.

	/** Get the name of this type of driver. */
	static CONSTEXPR const char *GetSystemName (void)
	{
		return "music";
	}

	/**
	 * Play a particular song.
	 * @param filename The name of file with the song to play.
	 */
	virtual void PlaySong(const char *filename) = 0;

	/**
	 * Stop playing the current song.
	 */
	virtual void StopSong() = 0;

	/**
	 * Are we currently playing a song?
	 * @return True if a song is being played.
	 */
	virtual bool IsSongPlaying() = 0;

	/**
	 * Set the volume, if possible.
	 * @param vol The new volume.
	 */
	virtual void SetVolume(byte vol) = 0;
};

/** Music driver factory. */
template <class D>
class MusicDriverFactory : DriverFactory <MusicDriver, D> {
public:
	/**
	 * Construct a new MusicDriverFactory.
	 * @param priority    The priority within the driver class.
	 * @param name        The name of the driver.
	 * @param description A long-ish description of the driver.
	 */
	MusicDriverFactory (int priority, const char *name, const char *description)
		: DriverFactory <MusicDriver, D> (priority, name, description)
	{
	}
};

#endif /* MUSIC_MUSIC_DRIVER_HPP */
