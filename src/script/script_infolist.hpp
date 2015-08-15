/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_infolist.hpp ScriptInfo list class and helpers. */

#ifndef SCRIPT_INFOLIST_HPP
#define SCRIPT_INFOLIST_HPP

#include <map>
#include "../string.h"
#include "../core/string_compare_type.hpp"
#include "script_scanner.hpp"

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

	/** Check if the list is empty. */
	bool Empty() { return this->full_list.empty(); }

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


/** Collection of scripts and libraries. */
template <typename T>
struct ScriptInfoLists {
	/** ScriptInfoList helper class. */
	template <Subdirectory dir, const char *desc>
	struct List : ScriptInfoList {
		/** Get the list of registered scripts to print on the console. */
		void GetConsoleList (stringb *buf, bool newest_only) const
		{
			this->ScriptInfoList::GetConsoleList (buf, desc, newest_only);
		}

		/**
		 * Check whether we have a script with the exact characteristics as ci.
		 * @param ci The characteristics to search on (shortname and md5sum).
		 * @param md5sum Whether to check the MD5 checksum.
		 * @return True iff we have a script matching.
		 */
		bool HasScript (const ContentInfo *ci, bool md5sum)
		{
			return this->ScriptInfoList::HasScript (ci, dir, md5sum);
		}

		/**
		 * Find a script of a #ContentInfo
		 * @param ci The information to compare to.
		 * @param md5sum Whether to check the MD5 checksum.
		 * @return A filename of a file of the content, else \c NULL.
		 */
		const char *FindMainScript (const ContentInfo *ci, bool md5sum)
		{
			return this->ScriptInfoList::FindMainScript (ci, dir, md5sum);
		}
	};

	List <T::script_dir,  T::script_list_desc>  scripts;
	List <T::library_dir, T::library_list_desc> libraries;

	/** ScriptScanner helper class for scripts. */
	struct InfoScanner : ScriptScanner {
		ScriptInfoList *lists;  ///< The list that we are building.

		InfoScanner (ScriptInfoList *lists)
			: ScriptScanner (T::scanner_desc), lists (lists)
		{
		}

		/** Get the ScriptScanner class associated with a VM. */
		static InfoScanner *Get (HSQUIRRELVM vm)
		{
			return static_cast<InfoScanner *> (ScriptScanner::Get(vm));
		}

		void RegisterAPI (void) OVERRIDE;

		/** Register a ScriptInfo to the scanner. */
		void RegisterScript (ScriptInfo *info, const char *name, bool dev_only = false)
		{
			this->lists->RegisterScript (info, name, dev_only);
		}

		/** Scan for info files. */
		uint Scan (void)
		{
			return this->ScriptScanner::Scan (PATHSEP "info.nut", T::script_dir);
		}

		/** Scan for info files. */
		static uint Scan (ScriptInfoList *lists)
		{
			InfoScanner scanner (lists);
			return scanner.Scan();
		}
	};

	/** ScriptScanner helper class for libraries. */
	struct LibraryScanner : ScriptScanner {
		ScriptInfoList *lists;  ///< The list that we are building.

		LibraryScanner (ScriptInfoList *lists)
			: ScriptScanner (T::scanner_desc), lists (lists)
		{
		}

		/** Get the ScriptScanner class associated with a VM. */
		static LibraryScanner *Get (HSQUIRRELVM vm)
		{
			return static_cast<LibraryScanner *> (ScriptScanner::Get(vm));
		}

		void RegisterAPI (void) OVERRIDE;

		/** Register a ScriptInfo to the scanner. */
		void RegisterScript (ScriptInfo *info, const char *name)
		{
			this->lists->RegisterScript (info, name, false);
		}

		/** Scan for info files. */
		uint Scan (void)
		{
			return this->ScriptScanner::Scan (PATHSEP "library.nut", T::library_dir);
		}

		/** Scan for info files. */
		static uint Scan (ScriptInfoList *lists)
		{
			LibraryScanner scanner (lists);
			return scanner.Scan();
		}
	};

	/** Scan for info files. */
	void Scan (void)
	{
		InfoScanner   ::Scan (&this->scripts);
		LibraryScanner::Scan (&this->libraries);
	}
};

#endif /* SCRIPT_INFOLIST_HPP */
