/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_scanner.hpp Declarations of the class for the script scanner. */

#ifndef SCRIPT_SCANNER_HPP
#define SCRIPT_SCANNER_HPP

#include "../fileio_func.h"
#include "squirrel.hpp"

/** Scanner to help finding scripts. */
class ScriptScanner : public FileScanner, public Squirrel {
public:
	ScriptScanner (const char *name);
	virtual ~ScriptScanner();

	/**
	 * Get the current main script the ScanDir is currently tracking.
	 */
	const char *GetMainScript() { return this->main_script; }

	/**
	 * Get the current tar file the ScanDir is currently tracking.
	 */
	const char *GetTarFile() { return this->tar_file; }

	/** Get the ScriptScanner class associated with a VM. */
	static ScriptScanner *Get (HSQUIRRELVM vm)
	{
		return static_cast<ScriptScanner *> (Squirrel::Get(vm));
	}

	/** Check if a given method exists. */
	bool method_exists (const char *name)
	{
		return this->MethodExists (this->instance, name);
	}

	/** Check if a given method exists, and throw an error otherwise. */
	bool check_method (const char *name);

	/** Call a boolean method. */
	bool call_bool_method (const char *name, int suspend, bool *res);

	/** Call an integer method. */
	bool call_integer_method (const char *name, int suspend, int *res);

	/** Call a string method and return allocated storage. */
	char *call_string_method (const char *name, int suspend);

	/** Call a string method to get a string from a set. */
	const char *call_string_method_from_set (const char *name,
		size_t n, const char *const *val, int suspend);

	template <size_t N>
	const char *call_string_method_from_set (const char *name,
		const char *const (*val) [N], int suspend)
	{
		return this->call_string_method_from_set (name, N,
				&(*val)[0], suspend);
	}

	template <size_t N>
	const char *call_string_method_from_set (const char *name,
		const char *const (&val) [N], int suspend)
	{
		return this->call_string_method_from_set (name, N,
				&val[0], suspend);
	}

	/** Begin construction of a ScriptInfo object. */
	SQInteger construct (class ScriptInfo *info);

	/* virtual */ bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename);

	HSQOBJECT instance;     ///< The Squirrel instance created for the current info.

protected:
	char *main_script;      ///< The full path of the script.
	char *tar_file;         ///< If, which tar file the script was in.

	/**
	 * Register the API for this ScriptInfo.
	 */
	virtual void RegisterAPI (void) = 0;
};

#endif /* SCRIPT_SCANNER_HPP */
