/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fios.h Declarations for savegames operations */

#ifndef FIOS_H
#define FIOS_H

#include "gfx_type.h"
#include "company_base.h"
#include "gamelog.h"
#include "newgrf_config.h"
#include "network/core/tcp_content.h"
#include "saveload/saveload_data.h"
#include "saveload/saveload_error.h"


/** The different abstract types of files that the system knows about. */
enum AbstractFileType {
	FT_NONE,      ///< nothing to do
	FT_SAVEGAME,  ///< old or new savegame
	FT_SCENARIO,  ///< old or new scenario
	FT_HEIGHTMAP, ///< heightmap file

	FT_INVALID = 7, ///< Invalid or unknown file type.
	FT_NUMBITS = 3, ///< Number of bits required for storing a #AbstractFileType value.
	FT_MASK = (1 << FT_NUMBITS) - 1, ///< Bitmask for extracting an abstract file type.
};

/** Kinds of files in each #AbstractFileType. */
enum DetailedFileType {
	/* Save game and scenario files. */
	DFT_OLD_GAME_FILE, ///< Old save game or scenario file.
	DFT_GAME_FILE,     ///< Save game or scenario file.

	/* Heightmap files. */
	DFT_HEIGHTMAP_BMP, ///< BMP file.
	DFT_HEIGHTMAP_PNG, ///< PNG file.

	/* fios 'files' */
	DFT_FIOS_DRIVE,  ///< A drive (letter) entry.
	DFT_FIOS_PARENT, ///< A parent directory entry.
	DFT_FIOS_DIR,    ///< A directory entry.

	DFT_INVALID = 255, ///< Unknown or invalid file.
};

/**
 * Construct an enum value for #FiosType as a combination of an abstract and a detailed file type.
 * @param abstract Abstract file type (one of #AbstractFileType).
 * @param detailed Detailed file type (one of #DetailedFileType).
 */
#define MAKE_FIOS_TYPE(abstract, detailed) ((abstract) | ((detailed) << FT_NUMBITS))

/**
 * Elements of a file system that are recognized.
 * Values are a combination of #AbstractFileType and #DetailedFileType.
 * @see GetAbstractFileType GetDetailedFileType
 */
enum FiosType {
	FIOS_TYPE_DRIVE  = MAKE_FIOS_TYPE(FT_NONE, DFT_FIOS_DRIVE),
	FIOS_TYPE_PARENT = MAKE_FIOS_TYPE(FT_NONE, DFT_FIOS_PARENT),
	FIOS_TYPE_DIR    = MAKE_FIOS_TYPE(FT_NONE, DFT_FIOS_DIR),

	FIOS_TYPE_FILE         = MAKE_FIOS_TYPE(FT_SAVEGAME, DFT_GAME_FILE),
	FIOS_TYPE_OLDFILE      = MAKE_FIOS_TYPE(FT_SAVEGAME, DFT_OLD_GAME_FILE),
	FIOS_TYPE_SCENARIO     = MAKE_FIOS_TYPE(FT_SCENARIO, DFT_GAME_FILE),
	FIOS_TYPE_OLD_SCENARIO = MAKE_FIOS_TYPE(FT_SCENARIO, DFT_OLD_GAME_FILE),
	FIOS_TYPE_PNG          = MAKE_FIOS_TYPE(FT_HEIGHTMAP, DFT_HEIGHTMAP_PNG),
	FIOS_TYPE_BMP          = MAKE_FIOS_TYPE(FT_HEIGHTMAP, DFT_HEIGHTMAP_BMP),

	FIOS_TYPE_INVALID = MAKE_FIOS_TYPE(FT_INVALID, DFT_INVALID),
};

#undef MAKE_FIOS_TYPE

/**
 * Extract the abstract file type from a #FiosType.
 * @param fios_type Type to query.
 * @return The Abstract file type of the \a fios_type.
 */
inline AbstractFileType GetAbstractFileType(FiosType fios_type)
{
	return static_cast<AbstractFileType>(fios_type & FT_MASK);
}

/**
 * Extract the detailed file type from a #FiosType.
 * @param fios_type Type to query.
 * @return The Detailed file type of the \a fios_type.
 */
inline DetailedFileType GetDetailedFileType(FiosType fios_type)
{
	return static_cast<DetailedFileType>(fios_type >> FT_NUMBITS);
}


typedef SmallMap<uint, CompanyProperties *> CompanyPropertiesMap;

/**
 * Container for loading in mode SL_LOAD_CHECK.
 */
struct LoadCheckData {
	bool checkable;     ///< True if the savegame could be checked by SL_LOAD_CHECK. (Old savegames are not checkable.)
	SlErrorData error;  ///< Error message from loading. INVALID_STRING_ID if no error.

	SavegameTypeVersion sl_version;               ///< Savegame type and version

	uint32 map_size_x, map_size_y;
	Date current_date;

	GameSettings settings;

	CompanyPropertiesMap companies;               ///< Company information.

	GRFConfig *grfconfig;                         ///< NewGrf configuration from save.
	GRFListCompatibility grf_compatibility;       ///< Summary state of NewGrfs, whether missing files or only compatible found.

	Gamelog gamelog;                              ///< Gamelog

	LoadCheckData() : grfconfig(NULL), grf_compatibility(GLC_NOT_FOUND), gamelog()
	{
		this->Clear();
	}

	/**
	 * Don't leak memory at program exit
	 */
	~LoadCheckData()
	{
		this->Clear();
	}

	/**
	 * Check whether loading the game resulted in errors.
	 * @return true if errors were encountered.
	 */
	bool HasErrors()
	{
		return this->checkable && this->error.str != INVALID_STRING_ID;
	}

	/**
	 * Check whether the game uses any NewGrfs.
	 * @return true if NewGrfs are used.
	 */
	bool HasNewGrfs()
	{
		return this->checkable && this->error.str == INVALID_STRING_ID && this->grfconfig != NULL;
	}

	void Clear();
};

extern LoadCheckData _load_check_data;


enum FileSlots {
	/**
	 * Slot used for the GRF scanning and such.
	 * This slot is used for all temporary accesses to files when scanning/testing files,
	 * and thus cannot be used for files, which are continuously accessed during a game.
	 */
	CONFIG_SLOT    =  0,
	/** Slot for the sound. */
	SOUND_SLOT     =  1,
	/** First slot usable for (New)GRFs used during the game. */
	FIRST_GRF_SLOT =  2,
	/** Maximum number of slots. */
	MAX_FILE_SLOTS = 128,
};

/** Deals with finding savegames */
struct FiosItem {
	FiosType type;
	uint64 mtime;
	char title[64];
	char name[MAX_PATH];
};

/** List of file information. */
class FileList {
public:
	struct Path {
		char home [MAX_PATH];   ///< The home path.
		char cur  [MAX_PATH];   ///< The current path.

		void reset (void)
		{
			memcpy (this->cur, this->home, MAX_PATH);
		}
	};

	SmallVector<FiosItem, 32> files; ///< The list of files.
	Path *path;                      ///< The current and home paths.

	/**
	 * Construct a new entry in the file list.
	 * @return Pointer to the new items to be initialized.
	 */
	inline FiosItem *Append()
	{
		return this->files.Append();
	}

	/**
	 * Get the number of files in the list.
	 * @return The number of files stored in the list.
	 */
	inline uint Length() const
	{
		return this->files.Length();
	}

	/**
	 * Get a pointer to the first file information.
	 * @return Address of the first file information.
	 */
	inline const FiosItem *Begin() const
	{
		return this->files.Begin();
	}

	/**
	 * Get a pointer behind the last file information.
	 * @return Address behind the last file information.
	 */
	inline const FiosItem *End() const
	{
		return this->files.End();
	}

	/**
	 * Get a pointer to the indicated file information. File information must exist.
	 * @return Address of the indicated existing file information.
	 */
	inline const FiosItem *Get(uint index) const
	{
		return this->files.Get(index);
	}

	/**
	 * Get a pointer to the indicated file information. File information must exist.
	 * @return Address of the indicated existing file information.
	 */
	inline FiosItem *Get(uint index)
	{
		return this->files.Get(index);
	}

	inline const FiosItem &operator[](uint index) const
	{
		return this->files[index];
	}

	/**
	 * Get a reference to the indicated file information. File information must exist.
	 * @return The requested file information.
	 */
	inline FiosItem &operator[](uint index)
	{
		return this->files[index];
	}

	/** Remove all items from the list. */
	inline void Clear()
	{
		this->files.Clear();
	}

	void BuildFileList (AbstractFileType abstract_filetype, bool save);
};

enum SortingBits {
	SORT_ASCENDING  = 0,
	SORT_DESCENDING = 1,
	SORT_BY_DATE    = 0,
	SORT_BY_NAME    = 2
};
DECLARE_ENUM_AS_BIT_SET(SortingBits)

/* Variables to display file lists */
extern SortingBits _savegame_sort_order;

void ShowSaveLoadDialog (AbstractFileType abstract_filetype, bool save = false);

const char *FiosBrowseTo (char *path, const FiosItem *item);

/* OS-specific functions are taken from their respective files (win32/unix/os2 .c) */
bool FiosGetDiskFreeSpace (const char *path, uint64 *tot);

bool FiosDelete (const char *path, const char *name);
void FiosMakeHeightmapName (char *buf, const char *path, const char *name, size_t size);
void FiosMakeSavegameName (char *buf, const char *path, const char *name, size_t size);

FiosType FiosGetSavegameListCallback (const char *file, const char *ext, stringb *title = NULL, bool save = false);

int CDECL CompareFiosItems(const FiosItem *a, const FiosItem *b);

#endif /* FIOS_H */
