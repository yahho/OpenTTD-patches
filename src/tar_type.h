/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tar_type.h Structs, typedefs and macros used for TAR file handling. */

#ifndef TAR_TYPE_H
#define TAR_TYPE_H

#include <map>
#include <string>

#include "core/pointer.h"
#include "fileio_type.h"

/** The define of a TarList. */
struct TarListEntry {
	ttd_unique_free_ptr <char> filename;
	ttd_unique_free_ptr <char> dirname;
};

struct TarFileListEntry {
	const char *tar_filename;
	size_t size;
	size_t position;
};

typedef std::map<std::string, TarListEntry> TarList;
typedef std::map<std::string, TarFileListEntry> TarFileList;
typedef std::map<std::string, std::string> TarLinkList;

/** Cache of tar files and their contents under a directory. */
struct TarCache {
	TarList     tars;  ///< list of tar files
	TarFileList files; ///< list of files in those tar files
	TarLinkList links; ///< list of directory links

	void add_link (const std::string &srcp, const std::string &destp);

	bool add (const char *filename, size_t basepath_length = 0);

	static TarCache cache [NUM_SUBDIRS]; ///< global per-directory cache
};

#define FOR_ALL_TARS(tar, sd) for (tar = TarCache::cache[sd].files.begin(); tar != TarCache::cache[sd].files.end(); tar++)

#endif /* TAR_TYPE_H */
