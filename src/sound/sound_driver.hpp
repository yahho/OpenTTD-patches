/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sound_driver.hpp Base for all sound drivers. */

#ifndef SOUND_SOUND_DRIVER_HPP
#define SOUND_SOUND_DRIVER_HPP

#include "../driver.h"

/** Base for all sound drivers. */
class SoundDriver : public Driver, public SharedDriverSystem <SoundDriver> {
public:
	/** Get the name of this type of driver. */
	static CONSTEXPR const char *GetSystemName (void)
	{
		return "sound";
	}

	/** Called once every tick */
	virtual void MainLoop() {}
};

/** Sound driver factory. */
template <class D>
class SoundDriverFactory : DriverFactory <SoundDriver, D> {
public:
	/**
	 * Construct a new SoundDriverFactory.
	 * @param priority    The priority within the driver class.
	 * @param name        The name of the driver.
	 * @param description A long-ish description of the driver.
	 */
	SoundDriverFactory (int priority, const char *name, const char *description)
		: DriverFactory <SoundDriver, D> (priority, name, description)
	{
	}
};

extern char *_ini_sounddriver;

#endif /* SOUND_SOUND_DRIVER_HPP */
