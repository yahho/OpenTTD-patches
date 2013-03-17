/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelog.h Functions to be called to log possibly unsafe game events */

#ifndef GAMELOG_H
#define GAMELOG_H

#include <memory>
#include <vector>

#include "newgrf_config.h"
#include "saveload/saveload_data.h"

/** The type of entries we log. */
enum GamelogEntryType {
	GLOG_START,         ///< Game starts
	GLOG_STARTED,       ///< Game started
	GLOG_LOAD,          ///< Game load
	GLOG_LOADED,        ///< Game loaded
	GLOG_MODE,          ///< Switch between scenario editor and game
	GLOG_REVISION,      ///< Changed game revision string
	GLOG_LEGACYREV,     ///< Changed game revision string (legacy)
	GLOG_OLDVER,        ///< Loaded from savegame without logged data
	GLOG_EMERGENCY,     ///< Emergency savegame
	GLOG_SETTING,       ///< Setting changed
	GLOG_CHEAT,         ///< Cheat was used
	GLOG_GRFBEGIN,      ///< GRF config change beginning
	GLOG_GRFEND,        ///< GRF config change end
	GLOG_GRFADD,        ///< GRF added
	GLOG_GRFREM,        ///< GRF removed
	GLOG_GRFCOMPAT,     ///< Compatible GRF loaded
	GLOG_GRFPARAM,      ///< GRF parameter changed
	GLOG_GRFMOVE,       ///< GRF order changed
	GLOG_GRFBUG,        ///< GRF bug was triggered
	GLOG_ENTRYTYPE_END, ///< So we know how many entry types there are
};

struct GamelogPrintBuffer;

/** Gamelog entry base class. */
struct GamelogEntry {
	GamelogEntryType type;

	GamelogEntry(GamelogEntryType t) : type(t) { }

	virtual ~GamelogEntry()
	{
	}

	virtual void Print(GamelogPrintBuffer *buf) = 0;
};

/** Gamelog structure. */
struct Gamelog : std::vector<std::unique_ptr<GamelogEntry> > {
	void append(GamelogEntry *entry) {
		this->push_back(std::unique_ptr<GamelogEntry>(entry));
	}
};

void GamelogReset();

void GamelogInfo(const Gamelog *gamelog, uint32 *last_rev, byte *ever_modified, bool *removed_newgrfs);

/**
 * Callback for printing text.
 * @param s The string to print.
 */
typedef void GamelogPrintProc(const char *s);
void GamelogPrint(GamelogPrintProc *proc); // needed for WIN32 / WINCE crash.log

void GamelogPrintDebug(int level);
void GamelogPrintConsole();

void GamelogAddStart();
void GamelogAddStarted();

void GamelogAddLoad();
void GamelogAddLoaded();

void GamelogAddRevision();
void GamelogTestRevision();
void GamelogAddMode();
void GamelogTestMode();
void GamelogOldver(const SavegameTypeVersion *stv);

void GamelogEmergency();
bool GamelogTestEmergency();

void GamelogSetting(const char *name, int32 oldval, int32 newval);

void GamelogGRFBegin();
void GamelogGRFEnd();

void GamelogGRFAdd(const GRFConfig *newg);
void GamelogGRFAddList(const GRFConfig *newg);
void GamelogGRFRemove(uint32 grfid);
void GamelogGRFCompatible(const GRFIdentifier *newg);
void GamelogGRFUpdate(const GRFConfig *oldg, const GRFConfig *newg);

bool GamelogGRFBugReverse(uint32 grfid, uint16 internal_id);

#endif /* GAMELOG_H */
