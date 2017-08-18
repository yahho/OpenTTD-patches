/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file fios.cpp
 * This file contains functions for building file lists for the save/load dialogs.
 */

#include "stdafx.h"
#include "fios.h"
#include "fileio_func.h"
#include "tar_type.h"
#include "screenshot.h"
#include "string.h"
#include <sys/stat.h>

#ifndef WIN32
# include <unistd.h>
#endif /* WIN32 */

#include "table/strings.h"

/* Variables to display file lists */
static char *_fios_path;
SortingBits _savegame_sort_order = SORT_BY_DATE | SORT_DESCENDING;

/* OS-specific functions are taken from their respective files (win32/unix/os2 .c) */
extern bool FiosIsRoot(const char *path);
extern bool FiosIsValidFile(const char *path, const struct dirent *ent, struct stat *sb);
extern bool FiosIsHiddenFile(const struct dirent *ent);
extern void FiosGetDrives(FileList &file_list);

/* get the name of an oldstyle savegame */
extern void GetOldSaveGameName (const char *file, stringb *title);

/**
 * Compare two FiosItem's. Used with sort when sorting the file list.
 * @param da A pointer to the first FiosItem to compare.
 * @param db A pointer to the second FiosItem to compare.
 * @return -1, 0 or 1, depending on how the two items should be sorted.
 */
int CDECL CompareFiosItems(const FiosItem *da, const FiosItem *db)
{
	int r = 0;

	if ((_savegame_sort_order & SORT_BY_NAME) == 0 && da->mtime != db->mtime) {
		r = da->mtime < db->mtime ? -1 : 1;
	} else {
		r = strcasecmp(da->title, db->title);
	}

	if (_savegame_sort_order & SORT_DESCENDING) r = -r;
	return r;
}

/** Get the current value of #_fios_path. */
const char *FiosGetPath (void)
{
	return _fios_path;
}

/**
 * Browse to a new path based on the passed \a item, starting at #_fios_path.
 * @param *item Item telling us what to do.
 * @return A filename w/path if we reached a file, otherwise \c NULL.
 */
const char *FiosBrowseTo(const FiosItem *item)
{
	char *path = _fios_path;

	switch (item->type) {
		case FIOS_TYPE_DRIVE:
#if defined(WINCE)
			snprintf(path, MAX_PATH, PATHSEP "");
#elif defined(WIN32) || defined(__OS2__)
			snprintf(path, MAX_PATH, "%c:" PATHSEP, item->title[0]);
#endif
			/* FALL THROUGH */
		case FIOS_TYPE_INVALID:
			break;

		case FIOS_TYPE_PARENT: {
			/* Check for possible NULL ptr (not required for UNIXes, but AmigaOS-alikes) */
			char *s = strrchr(path, PATHSEPCHAR);
			if (s != NULL && s != path) {
				s[0] = '\0'; // Remove last path separator character, so we can go up one level.
			}
			s = strrchr(path, PATHSEPCHAR);
			if (s != NULL) {
				s[1] = '\0'; // go up a directory
#if defined(__MORPHOS__) || defined(__AMIGAOS__)
			/* On MorphOS or AmigaOS paths look like: "Volume:directory/subdirectory" */
			} else if ((s = strrchr(path, ':')) != NULL) {
				s[1] = '\0';
#endif
			}
			break;
		}

		case FIOS_TYPE_DIR:
			strcat(path, item->name);
			strcat(path, PATHSEP);
			break;

		case FIOS_TYPE_DIRECT:
			snprintf(path, MAX_PATH, "%s", item->name);
			break;

		case FIOS_TYPE_FILE:
		case FIOS_TYPE_OLDFILE:
		case FIOS_TYPE_SCENARIO:
		case FIOS_TYPE_OLD_SCENARIO:
		case FIOS_TYPE_PNG:
		case FIOS_TYPE_BMP:
			return item->name;
	}

	return NULL;
}

/**
 * Construct a filename from its components in destination buffer \a buf.
 * @param buf Destination buffer.
 * @param path Directory path, may be \c NULL.
 * @param name Filename.
 * @param ext Filename extension (use \c "" for no extension).
 * @param size Size of \a buf.
 */
static void FiosMakeFilename(char *buf, const char *path, const char *name, const char *ext, size_t size)
{
	const char *period;

	/* Don't append the extension if it is already there */
	period = strrchr(name, '.');
	if (period != NULL && strcasecmp(period, ext) == 0) ext = "";
#if  defined(__MORPHOS__) || defined(__AMIGAOS__)
	if (path != NULL) {
		unsigned char sepchar = path[(strlen(path) - 1)];

		if (sepchar != ':' && sepchar != '/') {
			snprintf(buf, size, "%s" PATHSEP "%s%s", path, name, ext);
		} else {
			snprintf(buf, size, "%s%s%s", path, name, ext);
		}
	} else {
		snprintf(buf, size, "%s%s", name, ext);
	}
#else
	snprintf(buf, size, "%s" PATHSEP "%s%s", path, name, ext);
#endif
}

/**
 * Make a save game or scenario filename from a name.
 * @param buf Destination buffer for saving the filename.
 * @param name Name of the file.
 * @param size Length of buffer \a buf.
 */
void FiosMakeSavegameName(char *buf, const char *name, size_t size)
{
	const char *extension = (_game_mode == GM_EDITOR) ? ".scn" : ".sav";

	FiosMakeFilename(buf, _fios_path, name, extension, size);
}

/**
 * Construct a filename for a height map.
 * @param buf Destination buffer.
 * @param name Filename.
 * @param size Size of \a buf.
 */
void FiosMakeHeightmapName(char *buf, const char *name, size_t size)
{
	FiosMakeFilename (buf, _fios_path, name, GetCurrentScreenshotExtension(), size);
}

/**
 * Delete a file.
 * @param name Filename to delete.
 */
bool FiosDelete(const char *name)
{
	char filename[512];

	FiosMakeSavegameName(filename, name, lengthof(filename));
	return unlink(filename) == 0;
}

typedef FiosType fios_getlist_callback_proc (const char *filename, const char *ext, stringb *title, bool save);

/**
 * Scanner to scan for a particular type of FIOS file.
 */
class FiosFileScanner : public FileScanner {
	fios_getlist_callback_proc *callback_proc; ///< Callback to check whether the file may be added
	FileList &file_list;     ///< Destination of the found files.
	bool save;               ///< The kind of file we are looking for.
public:
	/**
	 * Create the scanner
	 * @param callback_proc The function that is called where you need to do the filtering.
	 * @param file_list Destination of the found files.
	 * @param save Purpose of collecting the list, true for saving.
	 */
	FiosFileScanner (fios_getlist_callback_proc *callback_proc, FileList &file_list, bool save) :
			callback_proc(callback_proc), file_list(file_list), save(save)
	{}

	/* virtual */ bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename);
};

/**
 * Try to add a fios item set with the given filename.
 * @param filename        the full path to the file to read
 * @param basepath_length amount of characters to chop of before to get a relative filename
 * @return true if the file is added.
 */
bool FiosFileScanner::AddFile(const char *filename, size_t basepath_length, const char *tar_filename)
{
	const char *ext = strrchr(filename, '.');
	if (ext == NULL) return false;

	sstring<64> fios_title;

	FiosType type = this->callback_proc (filename, ext, &fios_title, this->save);
	if (type == FIOS_TYPE_INVALID) return false;

	for (const FiosItem *fios = file_list.Begin(); fios != file_list.End(); fios++) {
		if (strcmp(fios->name, filename) == 0) return false;
	}

	FiosItem *fios = file_list.Append();
#ifdef WIN32
	struct _stat sb;
	if (_tstat(OTTD2FS(filename), &sb) == 0) {
#else
	struct stat sb;
	if (stat(filename, &sb) == 0) {
#endif
		fios->mtime = sb.st_mtime;
	} else {
		fios->mtime = 0;
	}

	fios->type = type;
	bstrcpy (fios->name, filename);

	/* If the file doesn't have a title, use its filename */
	const char *t = fios_title.c_str();
	if (fios_title.empty()) {
		t = strrchr(filename, PATHSEPCHAR);
		t = (t == NULL) ? filename : (t + 1);
	}
	bstrcpy (fios->title, t);
	str_validate(fios->title, lastof(fios->title));

	return true;
}


/**
 * Get the title of a file, which (if exists) is stored in a file named
 * the same as the data file but with '.title' added to it.
 * @param file filename to get the title for
 * @param title the title buffer to fill
 * @param subdir the sub directory to search in
 */
static void GetFileTitle (const char *file, stringb *title, Subdirectory subdir)
{
	char buf[MAX_PATH];
	bstrfmt (buf, "%s.title", file);

	FILE *f = FioFOpenFile(buf, "r", subdir);
	if (f == NULL) return;

	title->len = fread (title->buffer, 1, title->capacity - 1, f);
	assert (title->len < title->capacity);
	title->buffer[title->len] = '\0';
	title->validate();
	FioFCloseFile(f);
}

/**
 * Callback for FileList::BuildFileList. It tells if a file is a savegame or not.
 * @param file Name of the file to check.
 * @param ext A pointer to the extension identifier inside file
 * @param title Buffer if a callback wants to lookup the title of the file; NULL to skip the lookup
 * @param save Purpose of collecting the list, true for saving.
 * @return a FIOS_TYPE_* type of the found file, FIOS_TYPE_INVALID if not a savegame
 * @see FileList::BuildFileList
 */
FiosType FiosGetSavegameListCallback (const char *file, const char *ext, stringb *title, bool save)
{
	/* Show savegame files
	 * .SAV OpenTTD saved game
	 * .SS1 Transport Tycoon Deluxe preset game
	 * .SV1 Transport Tycoon Deluxe (Patch) saved game
	 * .SV2 Transport Tycoon Deluxe (Patch) saved 2-player game */

	assert (ext != NULL);

	if (strcasecmp(ext, ".sav") == 0) {
		GetFileTitle (file, title, SAVE_DIR);
		return FIOS_TYPE_FILE;
	}

	if (!save) {
		if (strcasecmp(ext, ".ss1") == 0 || strcasecmp(ext, ".sv1") == 0 ||
				strcasecmp(ext, ".sv2") == 0) {
			if (title != NULL) GetOldSaveGameName (file, title);
			return FIOS_TYPE_OLDFILE;
		}
	}

	return FIOS_TYPE_INVALID;
}

/**
 * Callback for FileList::BuildFileList. It tells if a file is a scenario or not.
 * @param file Name of the file to check.
 * @param ext A pointer to the extension identifier inside file
 * @param title Buffer if a callback wants to lookup the title of the file
 * @param save Purpose of collecting the list, true for saving.
 * @return a FIOS_TYPE_* type of the found file, FIOS_TYPE_INVALID if not a scenario
 * @see FileList::BuildFileList
 */
static FiosType FiosGetScenarioListCallback (const char *file, const char *ext, stringb *title, bool save)
{
	/* Show scenario files
	 * .SCN OpenTTD style scenario file
	 * .SV0 Transport Tycoon Deluxe (Patch) scenario
	 * .SS0 Transport Tycoon Deluxe preset scenario */
	if (strcasecmp(ext, ".scn") == 0) {
		GetFileTitle (file, title, SCENARIO_DIR);
		return FIOS_TYPE_SCENARIO;
	}

	if (!save) {
		if (strcasecmp(ext, ".sv0") == 0 || strcasecmp(ext, ".ss0") == 0 ) {
			GetOldSaveGameName (file, title);
			return FIOS_TYPE_OLD_SCENARIO;
		}
	}

	return FIOS_TYPE_INVALID;
}

static FiosType FiosGetHeightmapListCallback (const char *file, const char *ext, stringb *title, bool save)
{
	/* Show heightmap files
	 * .PNG PNG Based heightmap files
	 * .BMP BMP Based heightmap files
	 */

	FiosType type = FIOS_TYPE_INVALID;

#ifdef WITH_PNG
	if (strcasecmp(ext, ".png") == 0) type = FIOS_TYPE_PNG;
#endif /* WITH_PNG */

	if (strcasecmp(ext, ".bmp") == 0) type = FIOS_TYPE_BMP;

	if (type == FIOS_TYPE_INVALID) return FIOS_TYPE_INVALID;

	TarFileList::iterator it = TarCache::cache[SCENARIO_DIR].files.find(file);
	if (it != TarCache::cache[SCENARIO_DIR].files.end()) {
		/* If the file is in a tar and that tar is not in a heightmap
		 * directory we are for sure not supposed to see it.
		 * Examples of this are pngs part of documentation within
		 * collections of NewGRFs or 32 bpp graphics replacement PNGs.
		 */
		bool match = false;
		Searchpath sp;
		FOR_ALL_SEARCHPATHS(sp) {
			char buf[MAX_PATH];
			FioGetFullPath (buf, sizeof(buf), sp, HEIGHTMAP_DIR);

			if (strncmp(buf, it->second.tar_filename, strlen(buf)) == 0) {
				match = true;
				break;
			}
		}

		if (!match) return FIOS_TYPE_INVALID;
	}

	GetFileTitle (file, title, HEIGHTMAP_DIR);

	return type;
}

/**
 * Construct a file list with the given kind of files, for the stated purpose.
 * @param abstract_filetype Kind of files to collect.
 * @param save Purpose of the collection, true for saving.
 */
void FileList::BuildFileList (AbstractFileType abstract_filetype, bool save)
{
	this->Clear();

	fios_getlist_callback_proc *callback;
	Subdirectory subdir;
	switch (abstract_filetype) {
		default:
			NOT_REACHED();

		case FT_NONE:
			return;

		case FT_SAVEGAME: {
			static char *fios_save_path = NULL;

			if (fios_save_path == NULL) {
				fios_save_path = xmalloc (MAX_PATH);
				FioGetDirectory (fios_save_path, MAX_PATH, SAVE_DIR);
			}

			_fios_path = fios_save_path;

			subdir = NO_DIRECTORY;
			callback = &FiosGetSavegameListCallback;
			break;
		}

		case FT_SCENARIO: {
			static char *fios_scn_path = NULL;

			/* Copy the default path on first run or on 'New Game' */
			if (fios_scn_path == NULL) {
				fios_scn_path = xmalloc (MAX_PATH);
				FioGetDirectory (fios_scn_path, MAX_PATH, SCENARIO_DIR);
			}

			_fios_path = fios_scn_path;

			char base_path[MAX_PATH];
			FioGetDirectory (base_path, sizeof(base_path), SCENARIO_DIR);

			subdir = (!save && strcmp (base_path, _fios_path) == 0) ? SCENARIO_DIR : NO_DIRECTORY;
			callback = &FiosGetScenarioListCallback;
			break;
		}

		case FT_HEIGHTMAP: {
			static char *fios_hmap_path = NULL;

			if (fios_hmap_path == NULL) {
				fios_hmap_path = xmalloc (MAX_PATH);
				FioGetDirectory (fios_hmap_path, MAX_PATH, HEIGHTMAP_DIR);
			}

			_fios_path = fios_hmap_path;

			char base_path[MAX_PATH];
			FioGetDirectory (base_path, sizeof(base_path), HEIGHTMAP_DIR);

			subdir = strcmp (base_path, _fios_path) == 0 ? HEIGHTMAP_DIR : NO_DIRECTORY;
			callback = &FiosGetHeightmapListCallback;
			break;
		}
	}

	/* A parent directory link exists if we are not in the root directory */
	if (!FiosIsRoot (_fios_path)) {
		FiosItem *fios = this->Append();
		fios->type = FIOS_TYPE_PARENT;
		fios->mtime = 0;
		bstrcpy (fios->name, "..");
		bstrcpy (fios->title, ".. (Parent directory)");
	}

	/* Show subdirectories */
	DIR *dir;
	if ((dir = ttd_opendir (_fios_path)) != NULL) {
		struct dirent *dirent;
		while ((dirent = readdir (dir)) != NULL) {
			FiosItem *fios;
			char d_name[sizeof(fios->name)];
			bstrcpy (d_name, FS2OTTD(dirent->d_name));

			/* found file must be directory, but not '.' or '..' */
			struct stat sb;
			if (FiosIsValidFile (_fios_path, dirent, &sb) && S_ISDIR(sb.st_mode) &&
					(!FiosIsHiddenFile (dirent) || strncasecmp (d_name, PERSONAL_DIR, strlen(d_name)) == 0) &&
					strcmp (d_name, ".") != 0 && strcmp (d_name, "..") != 0) {
				fios = this->Append();
				fios->type = FIOS_TYPE_DIR;
				fios->mtime = 0;
				bstrcpy (fios->name, d_name);
				bstrfmt (fios->title, "%s" PATHSEP " (Directory)", d_name);
				str_validate (fios->title, lastof(fios->title));
			}
		}
		closedir(dir);
	}

	/* Sort the subdirs always by name, ascending, remember user-sorting order */
	{
		SortingBits order = _savegame_sort_order;
		_savegame_sort_order = SORT_BY_NAME | SORT_ASCENDING;
		this->files.Sort (CompareFiosItems);
		_savegame_sort_order = order;
	}

	/* This is where to start sorting for the filenames */
	int sort_start = this->Length();

	/* Show files */
	FiosFileScanner scanner (callback, *this, save);
	if (subdir == NO_DIRECTORY) {
		scanner.Scan (NULL, _fios_path, NULL, false);
	} else {
		scanner.Scan (NULL, subdir, true, true);
	}

	this->files.Sort (CompareFiosItems, sort_start);

	/* Show drives */
	FiosGetDrives (*this);

	this->Compact();
}

/**
 * Get the directory for screenshots.
 * @return path to screenshots
 */
const char *FiosGetScreenshotDir()
{
	static char *fios_screenshot_path = NULL;

	if (fios_screenshot_path == NULL) {
		fios_screenshot_path = xmalloc (MAX_PATH);
		FioGetDirectory(fios_screenshot_path, MAX_PATH, SCREENSHOT_DIR);
	}

	return fios_screenshot_path;
}

#if defined(ENABLE_NETWORK)
#include "network/network_content.h"
#include "3rdparty/md5/md5.h"

/** Basic data to distinguish a scenario. Used in the server list window */
struct ScenarioIdentifier {
	uint32 scenid;           ///< ID for the scenario (generated by content).
	uint8 md5sum[16];        ///< MD5 checksum of file.
	char filename[MAX_PATH]; ///< filename of the file.

	bool operator == (const ScenarioIdentifier &other) const
	{
		return this->scenid == other.scenid &&
				memcmp(this->md5sum, other.md5sum, sizeof(this->md5sum)) == 0;
	}

	bool operator != (const ScenarioIdentifier &other) const
	{
		return !(*this == other);
	}
};

/**
 * Scanner to find the unique IDs of scenarios
 */
class ScenarioScanner : protected FileScanner, public SmallVector<ScenarioIdentifier, 8> {
	bool scanned; ///< Whether we've already scanned
public:
	/** Initialise */
	ScenarioScanner() : scanned(false) {}

	/**
	 * Scan, but only if it's needed.
	 * @param rescan whether to force scanning even when it's not necessary
	 */
	void Scan(bool rescan)
	{
		if (this->scanned && !rescan) return;

		this->FileScanner::Scan(".id", SCENARIO_DIR, true, true);
		this->scanned = true;
	}

	/* virtual */ bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename)
	{
		FILE *f = FioFOpenFile(filename, "r", SCENARIO_DIR);
		if (f == NULL) return false;

		ScenarioIdentifier id;
		int fret = fscanf(f, "%i", &id.scenid);
		FioFCloseFile(f);
		if (fret != 1) return false;
		bstrcpy (id.filename, filename);

		Md5 checksum;
		uint8 buffer[1024];
		char basename[MAX_PATH]; ///< \a filename without the extension.
		size_t len, size;

		/* open the scenario file, but first get the name.
		 * This is safe as we check on extension which
		 * must always exist. */
		bstrcpy (basename, filename);
		*strrchr(basename, '.') = '\0';
		f = FioFOpenFile(basename, "rb", SCENARIO_DIR, &size);
		if (f == NULL) return false;

		/* calculate md5sum */
		while ((len = fread(buffer, 1, (size > sizeof(buffer)) ? sizeof(buffer) : size, f)) != 0 && size != 0) {
			size -= len;
			checksum.Append(buffer, len);
		}
		checksum.Finish(id.md5sum);

		FioFCloseFile(f);

		this->Include(id);
		return true;
	}
};

/** Scanner for scenarios */
static ScenarioScanner _scanner;

/**
 * Find a given scenario based on its unique ID.
 * @param ci The content info to compare it to.
 * @param md5sum Whether to look at the md5sum or the id.
 * @return The filename of the file, else \c NULL.
 */
const char *FindScenario(const ContentInfo *ci, bool md5sum)
{
	_scanner.Scan(false);

	for (ScenarioIdentifier *id = _scanner.Begin(); id != _scanner.End(); id++) {
		if (md5sum ? (memcmp(id->md5sum, ci->md5sum, sizeof(id->md5sum)) == 0)
		           : (id->scenid == ci->unique_id)) {
			return id->filename;
		}
	}

	return NULL;
}

/**
 * Check whether we've got a given scenario based on its unique ID.
 * @param ci The content info to compare it to.
 * @param md5sum Whether to look at the md5sum or the id.
 * @return True iff we've got the scenario.
 */
bool HasScenario(const ContentInfo *ci, bool md5sum)
{
	return (FindScenario(ci, md5sum) != NULL);
}

/**
 * Force a (re)scan of the scenarios.
 */
void ScanScenarios()
{
	_scanner.Scan(true);
}

#endif /* ENABLE_NETWORK */
