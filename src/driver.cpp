/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file driver.cpp Base for all driver handling. */

#include "stdafx.h"
#include "debug.h"
#include "sound/sound_driver.hpp"
#include "music/music_driver.hpp"
#include "video/video_driver.hpp"
#include "string.h"

/** The video driver as stored in the configuration file. */
char *VideoDriver::ini;

int _num_resolutions;       ///< The number of resolutions.
Dimension _resolutions[32]; ///< List of resolutions.
Dimension _cur_resolution;  ///< The current resolution.
bool _rightclick_emulate;   ///< Whether right clicking is emulated.

/** The sound driver as stored in the configuration file. */
char *SoundDriver::ini;

/** The music driver as stored in the configuration file. */
char *MusicDriver::ini;

/**
 * Get a string parameter the list of parameters.
 * @param parm The parameters.
 * @param name The parameter name we're looking for.
 * @return The parameter value.
 */
const char *GetDriverParam(const char * const *parm, const char *name)
{
	size_t len;

	if (parm == NULL) return NULL;

	len = strlen(name);
	for (; *parm != NULL; parm++) {
		const char *p = *parm;

		if (strncmp(p, name, len) == 0) {
			if (p[len] == '=')  return p + len + 1;
			if (p[len] == '\0') return p + len;
		}
	}
	return NULL;
}

/**
 * Get a boolean parameter the list of parameters.
 * @param parm The parameters.
 * @param name The parameter name we're looking for.
 * @return The parameter value.
 */
bool GetDriverParamBool(const char * const *parm, const char *name)
{
	return GetDriverParam(parm, name) != NULL;
}

/**
 * Get an integer parameter the list of parameters.
 * @param parm The parameters.
 * @param name The parameter name we're looking for.
 * @param def  The default value if the parameter doesn't exist.
 * @return The parameter value.
 */
int GetDriverParamInt(const char * const *parm, const char *name, int def)
{
	const char *p = GetDriverParam(parm, name);
	return p != NULL ? atoi(p) : def;
}


/** Construct a DriverSystem. */
DriverSystem::DriverSystem (const char *desc)
	: drivers (new map), desc(desc), active(NULL), name(NULL)
{
}

/**
 * Insert a driver factory into the list.
 * @param name    The name of the driver.
 * @param factory The factory of the driver.
 */
void DriverSystem::insert (const char *name, DriverFactoryBase *factory)
{
	std::pair <map::iterator, bool> ins = this->drivers->insert (map::value_type (name, factory));
	assert (ins.second);
}

/** Remove a driver factory from the list. */
void DriverSystem::erase (const char *name)
{
	map::iterator it = this->drivers->find (name);
	assert (it != this->drivers->end());

	this->drivers->erase (it);

	if (this->drivers->empty()) delete this->drivers;
}

/**
 * Find the requested driver and return its class.
 * @param name the driver to select.
 * @post Sets the driver so GetCurrentDriver() returns it too.
 */
void DriverSystem::select (const char *name)
{
	if (this->drivers->empty()) {
		usererror ("No %s drivers found", this->desc);
	}

	if (StrEmpty(name)) {
		/* Probe for this driver, but do not fall back to dedicated/null! */
		for (int priority = 10; priority > 0; priority--) {
			map::iterator it = this->drivers->begin();
			for (; it != this->drivers->end(); ++it) {
				DriverFactoryBase *d = (*it).second;
				if (d->priority != priority) continue;

				Driver *oldd = this->active;
				const char *oldn = this->name;
				Driver *newd = d->CreateInstance();
				this->active = newd;
				this->name = d->name;

				const char *err = newd->Start(NULL);
				if (err == NULL) {
					DEBUG(driver, 1, "Successfully probed %s driver '%s'", this->desc, d->name);
					delete oldd;
					return;
				}

				this->active = oldd;
				this->name = oldn;
				DEBUG(driver, 1, "Probing %s driver '%s' failed with error: %s", this->desc, d->name, err);
				delete newd;
			}
		}
		usererror ("Couldn't find any suitable %s driver", this->desc);
	} else {
		char *parm;
		char buffer[256];
		const char *parms[32];

		/* Extract the driver name and put parameter list in parm */
		bstrcpy (buffer, name);
		parm = strchr(buffer, ':');
		uint np = 0;
		while (parm != NULL) {
			*parm++ = '\0';
			if (np == lengthof(parms) - 1) break;
			parms[np++] = parm;
			parm = strchr (parm, ',');
		}
		parms[np] = NULL;

		/* Find this driver */
		map::iterator it = this->drivers->begin();
		for (; it != this->drivers->end(); ++it) {
			DriverFactoryBase *d = (*it).second;

			/* Check driver name */
			if (strcasecmp(buffer, d->name) != 0) continue;

			/* Found our driver, let's try it */
			Driver *newd = d->CreateInstance();

			const char *err = newd->Start(parms);
			if (err != NULL) {
				delete newd;
				usererror("Unable to load driver '%s'. The error was: %s", d->name, err);
			}

			DEBUG(driver, 1, "Successfully loaded %s driver '%s'", this->desc, d->name);
			delete this->active;
			this->active = newd;
			this->name = d->name;
			return;
		}
		usererror ("No such %s driver: %s\n", this->desc, buffer);
	}
}

/**
 * Build a human readable list of available drivers.
 * @param buf The buffer to write to.
 */
void DriverSystem::list (stringb *buf)
{
	buf->append_fmt ("List of %s drivers:\n", this->desc);

	for (int priority = 10; priority >= 0; priority--) {
		map::iterator it = this->drivers->begin();
		for (; it != this->drivers->end(); it++) {
			DriverFactoryBase *d = (*it).second;
			if (d->priority != priority) continue;
			buf->append_fmt ("%18s: %s\n", d->name, d->description);
		}
	}

	buf->append ('\n');
}
