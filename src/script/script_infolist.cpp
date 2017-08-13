/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_infolist.cpp ScriptInfo list class and helpers. */

#include "../stdafx.h"
#include "../debug.h"
#include "../string.h"

#include "script_infolist.hpp"
#include "script_info.hpp"

ScriptInfoList::~ScriptInfoList()
{
	iterator it = this->full_list.begin();
	for (; it != this->full_list.end(); it++) {
		free((*it).first);
		delete (*it).second;
	}
	it = this->single_list.begin();
	for (; it != this->single_list.end(); it++) {
		free((*it).first);
	}

	this->full_list.clear();
	this->single_list.clear();
}

void ScriptInfoList::RegisterScript (ScriptInfo *info, const char *name, bool dev_only)
{
	sstring<1024> script_name;
	script_name.copy (name);
	script_name.tolower();
	size_t original_length = script_name.length();
	script_name.append_fmt (".%d", info->GetVersion());

	/* Check if GetShortName follows the rules */
	if (strlen(info->GetShortName()) != 4) {
		DEBUG(script, 0, "The script '%s' returned a string from GetShortName() which is not four characaters. Unable to load the script.", info->GetName());
		delete info;
		return;
	}

	iterator iter = this->full_list.find (script_name.c_str());
	if (iter != this->full_list.end()) {
		/* This script was already registered */
		const char *old_main = iter->second->GetMainScript();
		const char *new_main = info->GetMainScript();
#ifdef WIN32
		/* Windows doesn't care about the case */
		if (strcasecmp (old_main, new_main) != 0) {
#else
		if (strcmp (old_main, new_main) != 0) {
#endif
			DEBUG(script, 1, "Registering two scripts with the same name and version");
			DEBUG(script, 1, "  1: %s", old_main);
			DEBUG(script, 1, "  2: %s", new_main);
			DEBUG(script, 1, "The first is taking precedence.");
		}

		delete info;
		return;
	}

	this->full_list[xstrdup(script_name.c_str())] = info;

	script_name.truncate (original_length);

	if (!dev_only || _settings_client.gui.ai_developer_tools) {
		/* Add the script to the 'unique' script list, where only the highest version
		 *  of the script is registered. */
		iterator iter = this->single_list.find (script_name.c_str());
		if (iter == this->single_list.end()) {
			this->single_list[xstrdup(script_name.c_str())] = info;
		} else if (iter->second->GetVersion() < info->GetVersion()) {
			iter->second = info;
		}
	}
}

void ScriptInfoList::GetConsoleList (stringb *buf, const char *desc, bool newest_only) const
{
	buf->append_fmt ("List of %s:\n", desc);
	const List &list = newest_only ? this->single_list : this->full_list;
	const_iterator it = list.begin();
	for (; it != list.end(); it++) {
		ScriptInfo *i = (*it).second;
		buf->append_fmt ("%10s (v%d): %s\n", i->GetName(), i->GetVersion(), i->GetDescription());
	}
	buf->append ('\n');
}

ScriptInfo *ScriptInfoList::FindInfo (const char *name, int version, bool force_exact_match)
{
	if (this->full_list.size() == 0) return NULL;
	if (name == NULL) return NULL;

	sstring<1024> script_name;
	script_name.copy (name);
	script_name.tolower();

	if (version == -1) {
		/* We want to load the latest version of this script; so find it */
		ScriptInfoList::iterator iter = this->single_list.find (script_name.c_str());
		if (iter != this->single_list.end()) return iter->second;

		/* If we didn't find a match script, maybe the user included a version */
		const char *e = strrchr (script_name.c_str(), '.');
		if (e == NULL) return NULL;
		version = atoi (e + 1);
		script_name.truncate (e - script_name.c_str());
		/* Continue, like we were calling this function with a version. */
	}

	if (force_exact_match) {
		/* Try to find a direct 'name.version' match */
		size_t length = script_name.length();
		script_name.append_fmt (".%d", version);
		ScriptInfoList::iterator iter = this->full_list.find (script_name.c_str());
		if (iter != this->full_list.end()) return iter->second;
		script_name.truncate (length);
	}

	ScriptInfo *info = NULL;
	int max_version = -1;

	/* See if there is a compatible script which goes by that name, with
	 * the highest version which allows loading the requested version */
	ScriptInfoList::iterator it = this->full_list.begin();
	for (; it != this->full_list.end(); it++) {
		ScriptInfo *i = it->second;
		assert (dynamic_cast<ScriptVersionedInfo *>(i) != NULL);
		if ((strcasecmp (script_name.c_str(), i->GetName()) == 0)
				&& static_cast<ScriptVersionedInfo *>(i)->CanLoadFromVersion(version)
				&& (max_version == -1 || i->GetVersion() > max_version)) {
			max_version = i->GetVersion();
			info = i;
		}
	}

	return info;
}

ScriptInfo *ScriptInfoList::FindLibrary (const char *library, int version)
{
	/* Internally we store libraries as 'library.version' */
	char library_name[1024];
	bstrfmt (library_name, "%s.%d", library, version);
	strtolower (library_name);

	/* Check if the library + version exists */
	ScriptInfoList::iterator iter = this->full_list.find (library_name);
	if (iter == this->full_list.end()) return NULL;

	return iter->second;
}

#if defined(ENABLE_NETWORK)
#include "../network/network_content.h"
#include "../3rdparty/md5/md5.h"
#include "../tar_type.h"

/** Helper for creating a MD5sum of all files within of a script. */
struct ScriptFileChecksumCreator : FileScanner {
	byte md5sum[16];  ///< The final md5sum.
	Subdirectory dir; ///< The directory to look in.

	/**
	 * Initialise the md5sum to be all zeroes,
	 * so we can easily xor the data.
	 */
	ScriptFileChecksumCreator(Subdirectory dir)
	{
		this->dir = dir;
		memset(this->md5sum, 0, sizeof(this->md5sum));
	}

	/* Add the file and calculate the md5 sum. */
	virtual bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename)
	{
		Md5 checksum;
		uint8 buffer[1024];
		size_t len, size;
		byte tmp_md5sum[16];

		/* Open the file ... */
		FILE *f = FioFOpenFile(filename, "rb", this->dir, &size);
		if (f == NULL) return false;

		/* ... calculate md5sum... */
		while ((len = fread(buffer, 1, (size > sizeof(buffer)) ? sizeof(buffer) : size, f)) != 0 && size != 0) {
			size -= len;
			checksum.Append(buffer, len);
		}
		checksum.Finish(tmp_md5sum);

		FioFCloseFile(f);

		/* ... and xor it to the overall md5sum. */
		for (uint i = 0; i < sizeof(md5sum); i++) this->md5sum[i] ^= tmp_md5sum[i];

		return true;
	}
};

/**
 * Check whether the script given in info is the same as in ci based
 * on the shortname and md5 sum.
 * @param ci The information to compare to.
 * @param md5sum Whether to check the MD5 checksum.
 * @param info The script to get the shortname and md5 sum from.
 * @return True iff they're the same.
 */
static bool IsSameScript(const ContentInfo *ci, bool md5sum, ScriptInfo *info, Subdirectory dir)
{
	uint32 id = 0;
	const char *str = info->GetShortName();
	for (int j = 0; j < 4 && *str != '\0'; j++, str++) id |= *str << (8 * j);

	if (id != ci->unique_id) return false;
	if (!md5sum) return true;

	ScriptFileChecksumCreator checksum(dir);
	const char *tar_filename = info->GetTarFile();
	TarList::iterator iter;
	if (tar_filename != NULL && (iter = TarCache::cache[dir].tars.find(tar_filename)) != TarCache::cache[dir].tars.end()) {
		/* The main script is in a tar file, so find all files that
		 * are in the same tar and add them to the MD5 checksumming. */
		TarFileList::iterator tar;
		FOR_ALL_TARS(tar, dir) {
			/* Not in the same tar. */
			if (tar->second.tar_filename != iter->first) continue;

			/* Check the extension. */
			const char *ext = strrchr(tar->first.c_str(), '.');
			if (ext == NULL || strcasecmp(ext, ".nut") != 0) continue;

			checksum.AddFile(tar->first.c_str(), 0, tar_filename);
		}
	} else {
		const char *main = info->GetMainScript();
		/* There'll always be at least 1 path separator character in a script
		 * main script name as the search algorithm requires the main script to
		 * be in a subdirectory of the script directory; so <dir>/<path>/main.nut. */
		checksum.Scan (".nut", main, strrchr (main, PATHSEPCHAR), true);
	}

	return memcmp(ci->md5sum, checksum.md5sum, sizeof(ci->md5sum)) == 0;
}

ScriptInfo *ScriptInfoList::FindScript (const ContentInfo *ci, Subdirectory subdir, bool md5sum)
{
	for (iterator it = this->full_list.begin(); it != this->full_list.end(); it++) {
		if (IsSameScript (ci, md5sum, it->second, subdir)) return it->second;
	}
	return NULL;
}

bool ScriptInfoList::HasScript (const ContentInfo *ci, Subdirectory subdir, bool md5sum)
{
	return this->FindScript (ci, subdir, md5sum) != NULL;
}

const char *ScriptInfoList::FindMainScript (const ContentInfo *ci, Subdirectory subdir, bool md5sum)
{
	ScriptInfo *info = this->FindScript (ci, subdir, md5sum);
	return (info != NULL) ? info->GetMainScript() : NULL;
}

#endif /* ENABLE_NETWORK */
