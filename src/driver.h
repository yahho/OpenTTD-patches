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
};


/** Base for all driver factories. */
struct DriverFactoryBase {
	const char *const name;        ///< The name of the drivers of this factory.
	const char *const description; ///< The description of this driver.
	const int priority;            ///< The priority of this factory.

	/**
	 * Construct a new DriverFactory.
	 * @param name        The name of the driver.
	 * @param description A long-ish description of the driver.
	 * @param priority    The priority within the driver class.
	 */
	CONSTEXPR DriverFactoryBase (const char *name, const char *description, int priority)
		: name(name), description(description), priority(priority)
	{
	}

	/** Destruct a DriverFactory. */
	virtual ~DriverFactoryBase()
	{
	}

	/**
	 * Create an instance of this driver-class.
	 * @return The instance.
	 */
	virtual Driver *CreateInstance() const = 0;
};


/** Encapsulation of a driver system (music, sound, video). */
struct DriverSystem {
	typedef std::map <const char *, DriverFactoryBase *, StringCompare> map;

	map drivers;            ///< Map of available drivers.
	const char *const desc; ///< Name of the driver system.
	Driver *active;         ///< Currently active driver.
	const char *name;       ///< Name of the currently active driver.

	DriverSystem (const char *desc);

	void insert (const char *name, DriverFactoryBase *factory);

	void erase (const char *name);

	void select (const char *name);

	void list (stringb *buf);
};


/** Driver system struct to share a common static DriverSystem. */
template <class T>
class SharedDriverSystem {
private:
	/** Get the driver system. */
	static DriverSystem &GetSystem (void)
	{
		static DriverSystem system (T::GetSystemName());
		return system;
	}

protected:
	/** Insert a driver factory into the list. */
	static void insert (const char *name, DriverFactoryBase *factory)
	{
		GetSystem().insert (name, factory);
	}

	/** Remove a driver factory from the list. */
	static void erase (const char *name)
	{
		GetSystem().erase (name);
	}

public:
	/** Shuts down the active driver. */
	static void ShutdownDriver (void)
	{
		Driver *driver = GetSystem().active;
		if (driver != NULL) driver->Stop();
	}

	/**
	 * Find the requested driver and return its class.
	 * @param name the driver to select.
	 * @post Sets the driver so GetCurrentDriver() returns it too.
	 */
	static void SelectDriver (const char *name)
	{
		GetSystem().select (name);
	}

	/**
	 * Get the active driver.
	 * @return The active driver.
	 */
	static T *GetActiveDriver (void)
	{
		return static_cast<T*> (GetSystem().active);
	}

	/**
	 * Get the name of the active driver.
	 * @return The name of the active driver.
	 */
	static const char *GetActiveDriverName (void)
	{
		return GetSystem().name;
	}

	/**
	 * Build a human readable list of available drivers.
	 * @param buf The buffer to write to.
	 */
	static void GetDriversInfo (stringb *buf)
	{
		GetSystem().list (buf);
	}
};


/** Specialised driver factory helper class. */
template <class T, class D>
class DriverFactory : DriverFactoryBase, public SharedDriverSystem <T> {
public:
	/**
	 * Construct a new DriverFactory.
	 * @param priority    The priority within the driver class.
	 * @param name        The name of the driver.
	 * @param description A long-ish description of the driver.
	 */
	DriverFactory (int priority, const char *name, const char *description)
		: DriverFactoryBase (name, description, priority)
	{
		SharedDriverSystem<T>::insert (name, this);
	}

	/** Destruct a DriverFactory. */
	~DriverFactory()
	{
		SharedDriverSystem<T>::erase (this->name);
	}

	/**
	 * Create an instance of this driver-class.
	 * @return The instance.
	 */
	D *CreateInstance (void) const FINAL_OVERRIDE
	{
		return new D;
	}
};

#endif /* DRIVER_H */
