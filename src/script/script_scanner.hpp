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

#include <map>
#include "../string.h"
#include "../fileio_func.h"
#include "../core/string_compare_type.hpp"

/** Collection of scripts. */
struct ScriptInfoList {
	typedef std::map<const char *, class ScriptInfo *, StringCompare> List; ///< Type for the list of scripts.
	typedef List::iterator iterator;
	typedef List::const_iterator const_iterator;

	List full_list;   ///< The list of all scripts.
	List single_list; ///< The list of all unique scripts. The best script (highest version) is shown.

	~ScriptInfoList();

	/** Register a ScriptInfo. */
	void RegisterScript (ScriptInfo *info, const char *name, bool dev_only);

	/** Get the list of all registered scripts. */
	const List *GetInfoList() { return &this->full_list; }

	/** Get the list of the latest version of all registered scripts. */
	const List *GetUniqueInfoList() { return &this->single_list; }

	/** Get the list of registered scripts to print on the console. */
	void GetConsoleList (stringb *buf, const char *desc, bool newest_only) const;

	/**
	 * Find a script of a #ContentInfo
	 * @param ci The information to compare to.
	 * @param subdir The subdirectory under which these scripts reside.
	 * @param md5sum Whether to check the MD5 checksum.
	 * @return A filename of a file of the content, else \c NULL.
	 */
	ScriptInfo *FindScript (const struct ContentInfo *ci, Subdirectory subdir, bool md5sum);

	/**
	 * Check whether we have a script with the exact characteristics as ci.
	 * @param ci The characteristics to search on (shortname and md5sum).
	 * @param subdir The subdirectory under which these scripts reside.
	 * @param md5sum Whether to check the MD5 checksum.
	 * @return True iff we have a script matching.
	 */
	bool HasScript (const ContentInfo *ci, Subdirectory subdir, bool md5sum);

	/**
	 * Find a script of a #ContentInfo
	 * @param ci The information to compare to.
	 * @param subdir The subdirectory under which these scripts reside.
	 * @param md5sum Whether to check the MD5 checksum.
	 * @return A filename of a file of the content, else \c NULL.
	 */
	const char *FindMainScript (const ContentInfo *ci, Subdirectory subdir, bool md5sum);
};

/** ScriptInfoList helper class. */
template <typename T>
struct ScriptInfoListT : ScriptInfoList {
	/** Get the list of registered scripts to print on the console. */
	void GetConsoleList (stringb *buf, bool newest_only) const
	{
		this->ScriptInfoList::GetConsoleList (buf, T::desc, newest_only);
	}

	/**
	 * Find a script of a #ContentInfo
	 * @param ci The information to compare to.
	 * @param md5sum Whether to check the MD5 checksum.
	 * @return A filename of a file of the content, else \c NULL.
	 */
	ScriptInfo *FindScript (const struct ContentInfo *ci, bool md5sum)
	{
		return this->ScriptInfoList::FindScript (ci, T::subdir, md5sum);
	}

	/**
	 * Check whether we have a script with the exact characteristics as ci.
	 * @param ci The characteristics to search on (shortname and md5sum).
	 * @param md5sum Whether to check the MD5 checksum.
	 * @return True iff we have a script matching.
	 */
	bool HasScript (const ContentInfo *ci, bool md5sum)
	{
		return this->ScriptInfoList::HasScript (ci, T::subdir, md5sum);
	}

	/**
	 * Find a script of a #ContentInfo
	 * @param ci The information to compare to.
	 * @param md5sum Whether to check the MD5 checksum.
	 * @return A filename of a file of the content, else \c NULL.
	 */
	const char *FindMainScript (const ContentInfo *ci, bool md5sum)
	{
		return this->ScriptInfoList::FindMainScript (ci, T::subdir, md5sum);
	}
};

/** Scanner to help finding scripts. */
class ScriptScanner : public FileScanner {
public:
	ScriptScanner (ScriptInfoList *lists, const char *name);
	virtual ~ScriptScanner();

	/**
	 * Get the engine of the main squirrel handler (it indexes all available scripts).
	 */
	class Squirrel *GetEngine() { return this->engine; }

	/**
	 * Get the current main script the ScanDir is currently tracking.
	 */
	const char *GetMainScript() { return this->main_script; }

	/**
	 * Get the current tar file the ScanDir is currently tracking.
	 */
	const char *GetTarFile() { return this->tar_file; }

	/**
	 * Register a ScriptInfo to the scanner.
	 */
	void RegisterScript (ScriptInfo *info, const char *name, bool dev_only = false)
	{
		this->lists->RegisterScript (info, name, dev_only);
	}

	/* virtual */ bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename);

protected:
	class Squirrel *engine; ///< The engine we're scanning with.
	char *main_script;      ///< The full path of the script.
	char *tar_file;         ///< If, which tar file the script was in.

	ScriptInfoList *lists;  ///< The list that we are building.

	/**
	 * Register the API for this ScriptInfo.
	 */
	virtual void RegisterAPI(class Squirrel *engine) = 0;
};

/** ScriptScanner helper class. */
template <typename T>
struct ScriptScannerT : ScriptScanner {
	ScriptScannerT (ScriptInfoList *lists)
		: ScriptScanner (lists, T::desc)
	{
	}

	void RegisterAPI (class Squirrel *engine) OVERRIDE
	{
		T::InfoType::RegisterAPI (engine);
	}

	/** Scan for info files. */
	uint Scan (void)
	{
		const char *main = T::is_library ? PATHSEP "library.nut" : PATHSEP "info.nut";
		return this->ScriptScanner::Scan (main, T::subdir);
	}

	/** Scan for info files. */
	static uint Scan (ScriptInfoList *lists)
	{
		ScriptScannerT scanner (lists);
		return scanner.Scan();
	}
};

#endif /* SCRIPT_SCANNER_HPP */
