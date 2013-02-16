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

#include "../fileio_type.h"
#include "../strings_type.h"
#include "saveload_data.h"
#include "saveload_buffer.h"

/** Save or load mode. @see SaveOrLoad */
enum LoadMode {
	SL_INVALID    = -1, ///< Invalid mode.
	SL_LOAD       =  0, ///< Load game.
	SL_OLD_LOAD   =  1, ///< Load old game.
	SL_PNG        =  2, ///< Load PNG file (height map).
	SL_BMP        =  3, ///< Load BMP file (height map).
	SL_LOAD_CHECK =  4, ///< Load for game preview.
};

void GenerateDefaultSaveName(char *buf, const char *last);
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
