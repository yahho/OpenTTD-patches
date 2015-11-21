/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file driver.h Base for all drivers (video, sound, music, etc). */

#ifndef DRIVER_H
#define DRIVER_H

#include <map>

#include "core/enum_type.hpp"
#include "core/string_compare_type.hpp"
#include "core/pointer.h"
#include "string.h"

const char *GetDriverParam(const char * const *parm, const char *name);
bool GetDriverParamBool(const char * const *parm, const char *name);
int GetDriverParamInt(const char * const *parm, const char *name, int def);

/** A driver for communicating with the user. */
class Driver {
public:
	/**
	 * Start this driver.
	 * @param parm Parameters passed to the driver.
	 * @return NULL if everything went okay, otherwise an error message.
	 */
	virtual const char *Start(const char * const *parm) = 0;

	/**
	 * Stop this driver.
	 */
	virtual void Stop() = 0;

	virtual ~Driver() { }

	/** The type of driver */
	enum Type {
		DT_BEGIN = 0, ///< Helper for iteration
		DT_MUSIC = 0, ///< A music driver, needs to be before sound to properly shut down extmidi forked music players
		DT_SOUND,     ///< A sound driver
		DT_VIDEO,     ///< A video driver
		DT_END,       ///< Helper for iteration
	};

	/**
	 * Get the name of this driver.
	 * @return The name of the driver.
	 */
	virtual const char *GetName() const = 0;
};

DECLARE_POSTFIX_INCREMENT(Driver::Type)


/** Encapsulation of a driver system (music, sound, video). */
struct DriverSystem {
	typedef std::map <const char *, class DriverFactoryBase *, StringCompare> map;

	map *drivers;           ///< Map of available drivers.
	Driver *active;         ///< Currently active driver.
	const char *const desc; ///< Name of the driver system.

	DriverSystem (const char *desc);

	void insert (const char *name, DriverFactoryBase *factory);

	void erase (const char *name);

	void select (const char *name);

	void list (stringb *buf);
};


/** Base for all driver factories. */
class DriverFactoryBase {
private:
	friend class DriverSystem;
	friend class MusicDriver;
	friend class SoundDriver;
	friend class VideoDriver;

	Driver::Type type;             ///< The type of driver.
	int priority;                  ///< The priority of this factory.
	const char *const name;        ///< The name of the drivers of this factory.
	const char *const description; ///< The description of this driver.

	/** Get the driver system. */
	static DriverSystem &GetSystem (Driver::Type type)
	{
		static DriverSystem systems [Driver::DT_END] = {
			DriverSystem ("music"),
			DriverSystem ("sound"),
			DriverSystem ("video"),
		};
		return systems[type];
	}

	/**
	 * Get the active driver for the given type.
	 * @param type The type to get the driver for.
	 * @return The active driver.
	 */
	static Driver **GetActiveDriver(Driver::Type type)
	{
		return &GetSystem(type).active;
	}

protected:
	/**
	 * Construct a new DriverFactory.
	 * @param type        The type of driver.
	 * @param priority    The priority within the driver class.
	 * @param name        The name of the driver.
	 * @param description A long-ish description of the driver.
	 */
	DriverFactoryBase (Driver::Type type, int priority, const char *name, const char *description)
		: type(type), priority(priority), name(name), description(description)
	{
		assert (type < Driver::DT_END);

		GetSystem(type).insert (name, this);
	}

	/** Destruct a DriverFactory. */
	virtual ~DriverFactoryBase()
	{
		GetSystem(this->type).erase (this->name);
	}

public:
	/**
	 * Shuts down all active drivers
	 */
	static void ShutdownDrivers()
	{
		for (Driver::Type dt = Driver::DT_BEGIN; dt < Driver::DT_END; dt++) {
			Driver *driver = *GetActiveDriver(dt);
			if (driver != NULL) driver->Stop();
		}
	}

	/**
	 * Find the requested driver and return its class.
	 * @param name the driver to select.
	 * @param type the type of driver to select
	 * @post Sets the driver so GetCurrentDriver() returns it too.
	 */
	static void SelectDriver(const char *name, Driver::Type type)
	{
		assert (type < Driver::DT_END);

		GetSystem(type).select (name);
	}

	/**
	 * Build a human readable list of available drivers, grouped by type.
	 * @param buf The buffer to write to.
	 */
	static void GetDriversInfo (stringb *buf)
	{
		for (Driver::Type type = Driver::DT_BEGIN; type != Driver::DT_END; type++) {
			GetSystem(type).list (buf);
		}
	}

	/**
	 * Create an instance of this driver-class.
	 * @return The instance.
	 */
	virtual Driver *CreateInstance() const = 0;
};

#endif /* DRIVER_H */
