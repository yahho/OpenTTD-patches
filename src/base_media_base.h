/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file base_media_base.h Generic functions for replacing base data (graphics, sounds). */

#ifndef BASE_MEDIA_BASE_H
#define BASE_MEDIA_BASE_H

#include <map>

#include "string.h"
#include "fileio_func.h"
#include "core/pointer.h"
#include "core/string_compare_type.hpp"
#include "gfx_type.h"
#include "textfile.h"
#include "ini_type.h"

/* Forward declare these; can't do 'struct X' in functions as older GCCs barf on that */
struct ContentInfo;

/** Description of a single base set. */
struct BaseSetDesc {
public:
	/** Structure holding filename and MD5 information about a single file */
	struct FileDesc {
		/** Actual status of this file. */
		enum Status {
			MATCH,    ///< The file did exist and the md5 checksum did match
			MISMATCH, ///< The file did exist, just the md5 checksum did not match
			MISSING,  ///< The file did not exist
		};

		const char *filename;        ///< filename
		uint8 hash[16];              ///< md5 sum of the file
		const char *missing_warning; ///< warning when this file is missing
		Status status;               ///< status of this file
	};

private:
	typedef std::map <const char *, ttd_unique_free_ptr<char>, StringCompare> StringMap;

	ttd_unique_free_ptr<char> name; ///< The name of the set
	ttd_unique_free_ptr<char> def;  ///< Default description of the set
	StringMap description;          ///< Descriptions of the set

public:
	uint32 shortname;       ///< Four letter short variant of the name
	uint32 version;         ///< The version of this set
	bool fallback;          ///< This set is a fallback set, i.e. it should be used only as last resort

	/** Get the name of this set. */
	const char *get_name (void) const
	{
		return this->name.get();
	}

	/** Set the name of this set. */
	void set_name (const char *s)
	{
		this->name.reset (xstrdup (s));
	}

	/** Get the default description of this set. */
	const char *get_default_desc (void) const
	{
		return this->def.get();
	}

	/** Add the default description of this set. */
	void add_default_desc (const char *desc)
	{
		this->def.reset (xstrdup (desc));
	}

	/* Get the description of this set for the given ISO code. */
	const char *get_desc (const char *isocode) const;

	/* Add a description of this set for a given language. */
	void add_desc (const char *lang, const char *desc);

	/* Try to read a single piece of metadata from an ini file. */
	static const IniItem *fetch_metadata (const IniGroup *metadata,
		const char *name, const char *type, const char *filename);

	/* Calculate and check the MD5 hash of the supplied file. */
	static FileDesc::Status CheckMD5 (const FileDesc *file);
};

/**
 * Information about a single base set.
 * @tparam T the real class we're going to be
 * @tparam Tnum_files the number of files in the set
 */
template <class T, size_t Tnum_files>
struct BaseSet : BaseSetDesc {
	/** Number of files in this set */
	static const size_t NUM_FILES = Tnum_files;

	FileDesc files[NUM_FILES];     ///< All files part of this set
	uint found_files;              ///< Number of the files that could be found
	uint valid_files;              ///< Number of the files that could be found and are valid

	T *next;                       ///< The next base set in this list

	/** Construct an instance. */
	BaseSet() : next(NULL) { }

	/** Free everything we allocated */
	~BaseSet()
	{
		for (uint i = 0; i < NUM_FILES; i++) {
			free(this->files[i].filename);
			free(this->files[i].missing_warning);
		}

		delete this->next;
	}

	/**
	 * Get the number of missing files.
	 * @return the number
	 */
	int GetNumMissing() const
	{
		return Tnum_files - this->found_files;
	}

	/**
	 * Get the number of invalid files.
	 * @note a missing file is invalid too!
	 * @return the number
	 */
	int GetNumInvalid() const
	{
		return Tnum_files - this->valid_files;
	}

	bool FillSetDetails(IniFile *ini, const char *path, const char *full_filename, bool allow_empty_filename = true);

	/**
	 * Check if this set is preferred to another one.
	 * (Used in derived classes.)
	 */
	static bool IsPreferredTo (const BaseSet &other)
	{
		return false;
	}

	/**
	 * Search a textfile file next to this base media.
	 * @param type The type of the textfile to search for.
	 * @return A description for the textfile.
	 */
	TextfileDesc GetTextfile (TextfileType type) const
	{
		for (uint i = 0; i < NUM_FILES; i++) {
			TextfileDesc txt (type, BASESET_DIR, this->files[i].filename);
			if (txt.valid()) return txt;
		}
		return TextfileDesc();
	}

	/**
	 * Try to read a single piece of metadata from an ini file.
	 * @param metadata The metadata group to search in.
	 * @param name The name of the item to fetch.
	 * @param filename The name of the filename for debugging output.
	 * @return The associated item, or NULL if it doesn't exist.
	 */
	static const IniItem *fetch_metadata (const IniGroup *metadata,
		const char *name, const char *filename)
	{
		return BaseSetDesc::fetch_metadata (metadata, name, T::set_type, filename);
	}
};

/**
 * Base for all base media (graphics, sounds)
 * @tparam Tbase_set the real set we're going to be
 */
template <class Tbase_set>
class BaseMedia {
protected:
	static Tbase_set *available_sets; ///< All available sets
	static Tbase_set *duplicate_sets; ///< All sets that aren't available, but needed for not downloading base sets when a newer version than the one on BaNaNaS is loaded.
	static const Tbase_set *used_set; ///< The currently used set

	struct Scanner : FileScanner {
		bool AddFile (const char *filename, size_t basepath_length, const char *tar_filename) OVERRIDE;
	};

public:
	/** The set as saved in the config file. */
	static const char *ini_set;

	/**
	 * Determine the graphics pack that has to be used.
	 * The one with the most correct files wins.
	 * @return true if a best set has been found.
	 */
	static bool DetermineBestSet();

	/** Do the scan for files. */
	static uint FindSets (const char *extension,
		Subdirectory dir1, Subdirectory dir2, bool search_in_tars)
	{
		Scanner fs;
		uint num = fs.Scan (extension, dir1, search_in_tars);
		return num + fs.Scan (extension, dir2, search_in_tars);
	}

	static Tbase_set *GetAvailableSets();

	static bool SetSet(const char *name);
	static void GetSetsList (stringb *buf);
	static int GetNumSets();
	static int GetIndexOfUsedSet();
	static const Tbase_set *GetSet(int index);
	static const Tbase_set *GetUsedSet();

	/**
	 * Check whether we have an set with the exact characteristics as ci.
	 * @param ci the characteristics to search on (shortname and md5sum)
	 * @param md5sum whether to check the MD5 checksum
	 * @return true iff we have an set matching.
	 */
	static bool HasSet(const ContentInfo *ci, bool md5sum);
};

/**
 * Helper class that adds a suitable scanner to a base media class.
 * @tparam Tbase_set the real set we're going to be
 * @tparam Tsearch_in_tars whether to search in the tars or not
 */
template <class Tbase_set, bool Tsearch_in_tars>
struct BaseMediaS : BaseMedia<Tbase_set> {
	/** Whether to search in the tars or not. */
	static const bool SEARCH_IN_TARS = Tsearch_in_tars;

	/** Do the scan for files. */
	static uint FindSets()
	{
		/* Searching in tars is only done in the old "data" directories basesets. */
		return BaseMedia<Tbase_set>::FindSets (Tbase_set::extension,
				SEARCH_IN_TARS ? OLD_DATA_DIR : OLD_GM_DIR,
				BASESET_DIR, SEARCH_IN_TARS);
	}
};

/**
 * Check whether there's a base set matching some information.
 * @param ci The content info to compare it to.
 * @param md5sum Should the MD5 checksum be tested as well?
 * @param s The list with sets.
 * @return The filename of the first file of the base set, or \c NULL if there is no match.
 */
template <class Tbase_set>
const char *TryGetBaseSetFile(const ContentInfo *ci, bool md5sum, const Tbase_set *s);

/** Types of graphics in the base graphics set */
enum GraphicsFileType {
	GFT_BASE,     ///< Base sprites for all climates
	GFT_LOGOS,    ///< Logos, landscape icons and original terrain generator sprites
	GFT_ARCTIC,   ///< Landscape replacement sprites for arctic
	GFT_TROPICAL, ///< Landscape replacement sprites for tropical
	GFT_TOYLAND,  ///< Landscape replacement sprites for toyland
	GFT_EXTRA,    ///< Extra sprites that were not part of the original sprites
	MAX_GFT,      ///< We are looking for this amount of GRFs
};

/** Blitter type for base graphics sets. */
enum BlitterType {
	BLT_8BPP,       ///< Base set has 8 bpp sprites only.
	BLT_32BPP,      ///< Base set has both 8 bpp and 32 bpp sprites.
};

/** All data of a graphics set. */
struct GraphicsSet : BaseSet<GraphicsSet, MAX_GFT> {
	static const char set_type[]; ///< Description of the set type
	static const char extension[]; ///< File extension

	/** Internal names of the files in this set. */
	static const char * const file_names [MAX_GFT];

	PaletteType palette;       ///< Palette of this graphics set
	BlitterType blitter;       ///< Blitter of this graphics set

	bool FillSetDetails (IniFile *ini, const char *path, const char *full_filename);

	/** Check if this set is preferred to another one. */
	bool IsPreferredTo (const GraphicsSet &other) const
	{
		return (this->palette == PAL_DOS) && (other.palette != PAL_DOS);
	}

	static FileDesc::Status CheckMD5 (const FileDesc *file);
};

/** All data/functions related with replacing the base graphics. */
class BaseGraphics : public BaseMediaS <GraphicsSet, true> {
public:
	static bool SetSet (const char *name);
};

/** All data of a sounds set. */
struct SoundsSet : BaseSet<SoundsSet, 1> {
	static const char set_type[]; ///< Description of the set type
	static const char extension[]; ///< File extension

	/** Internal names of the files in this set. */
	static const char * const file_names [1];
};

/** All data/functions related with replacing the base sounds */
class BaseSounds : public BaseMediaS <SoundsSet, true> {
public:
	static bool SetSet (const char *name);
};

/** Maximum number of songs in the 'class' playlists. */
static const uint NUM_SONGS_CLASS     = 10;
/** Number of classes for songs */
static const uint NUM_SONG_CLASSES    = 3;
/** Maximum number of songs in the full playlist; theme song + the classes */
static const uint NUM_SONGS_AVAILABLE = 1 + NUM_SONG_CLASSES * NUM_SONGS_CLASS;

/** Maximum number of songs in the (custom) playlist */
static const uint NUM_SONGS_PLAYLIST  = 32;

/** All data of a music set. */
struct MusicSet : BaseSet<MusicSet, NUM_SONGS_AVAILABLE> {
	static const char set_type[]; ///< Description of the set type
	static const char extension[]; ///< File extension

	/** Internal names of the files in this set. */
	static const char * const file_names [NUM_SONGS_AVAILABLE];

	/** The name of the different songs. */
	char song_name[NUM_SONGS_AVAILABLE][32];
	byte track_nr[NUM_SONGS_AVAILABLE];
	byte num_available;

	bool FillSetDetails (IniFile *ini, const char *path, const char *full_filename);
};

/** All data/functions related with replacing the base music */
class BaseMusic : public BaseMediaS <MusicSet, false> {
public:
};

#endif /* BASE_MEDIA_BASE_H */
