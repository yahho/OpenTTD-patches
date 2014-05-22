/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload.h Functions/types related to saving and loading games. */

#ifndef SAVELOAD_H
#define SAVELOAD_H

#include "../string.h"
#include "../fileio_type.h"
#include "../strings_type.h"
#include "saveload_data.h"
#include "saveload_buffer.h"

/*
 * An experimental savegame provides no backward or forward compatibility.
 * It can only be loaded by the same binary that produced it, or one from
 * the same sources; as a safeguard, the saveload code will refuse to load
 * an experimental savegame unless the binary itself is built with this
 * flag set.
 *
 * Experimental savegames should be used sparingly, because they contain
 * no information about their layout. They should be used when a change
 * in the savegame format is too invasive to be done atomically, i.e.,
 * when it must be split into smaller commits and many of them alter the
 * savegame format, so as not to bump the savegame version at each step
 * or to add lots of compatibility checks in the code. In this situation,
 * first increase SAVEGAME_VERSION and set savegame experimentality to
 * true, then all savegame-changing commits go in, and finally savegame
 * experimentality is restored to false. (This means, of course, that
 * savegames made with intermediate source trees will not load with any
 * other version.)
 */

/** Return whether the resulting binary produces experimental savegames. */
inline bool IsExperimentalSavegameVersion()
{
	static const bool experimental_savegame = false;

	return experimental_savegame;
}

/** Save or load mode. @see SaveOrLoad */
enum LoadMode {
	SL_INVALID    = -1, ///< Invalid mode.
	SL_LOAD       =  0, ///< Load game.
	SL_OLD_LOAD   =  1, ///< Load old game.
	SL_PNG        =  2, ///< Load PNG file (height map).
	SL_BMP        =  3, ///< Load BMP file (height map).
	SL_LOAD_CHECK =  4, ///< Load for game preview.
};

void GenerateDefaultSaveName (stringb *buf);
const char *GetSaveLoadErrorString();
bool SaveGame(const char *filename, Subdirectory sb, bool threaded = true);
bool LoadGame(const char *filename, int mode, Subdirectory sb);
void WaitTillSaved();
void ProcessAsyncSaveFinish();
void DoExitSave();

bool SaveWithFilter(struct SaveFilter *writer, bool threaded);
bool LoadWithFilter(struct LoadFilter *reader);

bool SaveloadCrashWithMissingNewGRFs();

extern char _savegame_format[8];
extern bool _do_autosave;

#endif /* SAVELOAD_H */
