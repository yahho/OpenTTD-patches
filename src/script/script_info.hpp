/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_info.hpp ScriptInfo keeps track of all information of a script, like Author, Description, ... */

#ifndef SCRIPT_INFO_HPP
#define SCRIPT_INFO_HPP

#include "squirrel.hpp"
#include "../core/pointer.h"
#include "../misc/countedptr.hpp"

#include "script_config.hpp"

/** The maximum number of operations for saving or loading the data of a script. */
static const int MAX_SL_OPS             = 100000;
/** The maximum number of operations for initial start of a script. */
static const int MAX_CONSTRUCTOR_OPS    = 100000;
/** Number of operations to create an instance of a script. */
static const int MAX_CREATEINSTANCE_OPS = 100000;
/** Number of operations to get the author and similar information. */
static const int MAX_GET_OPS            =   1000;
/** Maximum number of operations allowed for getting a particular setting. */
static const int MAX_GET_SETTING_OPS    = 100000;

/** All static information from an Script like name, version, etc. */
class ScriptInfo : public SimpleCountedObject {
public:
	ScriptInfo();
	~ScriptInfo();

	/**
	 * Get the Author of the script.
	 */
	const char *GetAuthor() const { return this->author.get(); }

	/**
	 * Get the Name of the script.
	 */
	const char *GetName() const { return this->name.get(); }

	/**
	 * Get the 4 character long short name of the script.
	 */
	const char *GetShortName() const { return this->short_name.get(); }

	/**
	 * Get the description of the script.
	 */
	const char *GetDescription() const { return this->description.get(); }

	/**
	 * Get the version of the script.
	 */
	int GetVersion() const { return this->version; }

	/**
	 * Get the last-modified date of the script.
	 */
	const char *GetDate() const { return this->date.get(); }

	/**
	 * Get the name of the instance of the script to create.
	 */
	const char *GetInstanceName() const { return this->instance_name.get(); }

	/**
	 * Get the website for this script.
	 */
	const char *GetURL() const { return this->url.get(); }

	/**
	 * Get the filename of the main.nut script.
	 */
	const char *GetMainScript() const { return this->main_script.get(); }

	/**
	 * Get the filename of the tar the script is in.
	 */
	const char *GetTarFile() const { return this->tar_file.get(); }

	/**
	 * Get the config list for this Script.
	 */
	const ScriptConfigItemList *GetConfigList() const;

	/**
	 * Get the description of a certain Script config option.
	 */
	const ScriptConfigItem *GetConfigItem(const char *name) const;

	/**
	 * Set a setting.
	 */
	SQInteger AddSetting(HSQUIRRELVM vm);

	/**
	 * Add labels for a setting.
	 */
	SQInteger AddLabels(HSQUIRRELVM vm);

	/**
	 * Get the default value for a setting.
	 */
	int GetSettingDefaultValue(const char *name) const;

	friend class ScriptScanner;

	/** Gather all the information on registration. */
	virtual SQInteger construct (class ScriptScanner *scanner);

protected:
	ScriptConfigItemList config_list; ///< List of settings from this Script.

	ttd_unique_free_ptr<char> main_script;    ///< The full path of the script.
	ttd_unique_free_ptr<char> tar_file;       ///< If, which tar file the script was in.
	ttd_unique_free_ptr<char> author;         ///< Author of the script.
	ttd_unique_free_ptr<char> name;           ///< Full name of the script.
	ttd_unique_free_ptr<char> short_name;     ///< Short name (4 chars) which uniquely identifies the script.
	ttd_unique_free_ptr<char> description;    ///< Small description of the script.
	ttd_unique_free_ptr<char> date;           ///< The date the script was written at.
	ttd_unique_free_ptr<char> instance_name;  ///< Name of the main class in the script.
	int version;                              ///< Version of the script.
	ttd_unique_free_ptr<char> url;            ///< URL of the script.
};

/** Intermediate class for versioned scripts (not libraries). */
class ScriptVersionedInfo : public ScriptInfo {
protected:
	int min_loadable_version; ///< The script can load savegame data if the version is equal or greater than this.
	const char *api_version;  ///< API version used by this script.

public:
	ScriptVersionedInfo (const char *api_version = NULL)
		: ScriptInfo(), min_loadable_version(0),
		  api_version(api_version)
	{
	}

	/** Gather all the information on registration. */
	SQInteger construct (class ScriptScanner *scanner) OVERRIDE = 0;

	/** Gather all the information on registration. */
	SQInteger construct (class ScriptScanner *scanner,
		size_t napi, const char *const *api, const char *def);

	template <size_t N>
	SQInteger construct (class ScriptScanner *scanner,
		const char *const (*api) [N], const char *def)
	{
		return this->construct (scanner, N, &(*api)[0], def);
	}

	template <size_t N>
	SQInteger construct (class ScriptScanner *scanner,
		const char *const (&api) [N], const char *def)
	{
		return this->construct (scanner, N, &api[0], def);
	}

	/** Get the API version this script is written for. */
	const char *GetAPIVersion() const { return this->api_version; }

	/** Check if we can start this script. */
	bool CanLoadFromVersion (int version) const
	{
		if (version == -1) return true;
		return version >= this->min_loadable_version && version <= this->version;
	}
};

#endif /* SCRIPT_INFO_HPP */
