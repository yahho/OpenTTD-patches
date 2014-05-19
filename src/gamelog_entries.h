/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelog_entries.h Declaration of gamelog entries, for use in saveload/gamelog_sl.cpp */

#ifndef GAMELOG_ENTRIES_H
#define GAMELOG_ENTRIES_H

#include "openttd.h"
#include "rev.h"
#include "date_func.h"
#include "settings_type.h"
#include "network/core/config.h"
#include "saveload/saveload_data.h"
#include "gamelog.h"
#include "string.h"

extern const uint16 SAVEGAME_VERSION;  ///< current savegame version

/** Gamelog entry base class for entries with tick information. */
struct GamelogEntryTimed : GamelogEntry {
	uint16 tick;

	GamelogEntryTimed(GamelogEntryType t) : GamelogEntry(t), tick(_tick_counter) { }

	void PrependTick(GamelogPrintBuffer *buf);
};

/** Gamelog entry for game start */
struct GamelogEntryStart : GamelogEntryTimed {
	GamelogEntryStart() : GamelogEntryTimed(GLOG_START) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry after game start */
struct GamelogEntryStarted : GamelogEntry {
	GamelogEntryStarted() : GamelogEntry(GLOG_STARTED) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for game load */
struct GamelogEntryLoad : GamelogEntryTimed {
	GamelogEntryLoad() : GamelogEntryTimed(GLOG_LOAD) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry after game load */
struct GamelogEntryLoaded : GamelogEntry {
	GamelogEntryLoaded() : GamelogEntry(GLOG_LOADED) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for mode switch between scenario editor and game */
struct GamelogEntryMode : GamelogEntry {
	byte mode;         ///< new game mode (editor or game)
	byte landscape;    ///< landscape (temperate, arctic, ...)

	GamelogEntryMode() : GamelogEntry(GLOG_MODE), mode(_game_mode),
		landscape(_settings_game.game_creation.landscape) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for game revision string */
struct GamelogEntryRevision : GamelogEntry {
	char text[NETWORK_REVISION_LENGTH]; ///< revision string,
	uint32 newgrf;     ///< newgrf version
	uint16 slver;      ///< savegame version
	byte modified;     ///< modified flag

	GamelogEntryRevision() : GamelogEntry(GLOG_REVISION),
			newgrf(_openttd_newgrf_version),
			slver(SAVEGAME_VERSION),
			modified(_openttd_revision_modified) {
		memset(this->text, 0, sizeof(this->text));
		strecpy(this->text, _openttd_revision, lastof(this->text));
	}

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for game revision string (legacy) */
struct GamelogEntryLegacyRev : GamelogEntry {
	char text[NETWORK_REVISION_LENGTH]; ///< revision string,
	uint32 newgrf;     ///< openttd newgrf version
	uint16 slver;      ///< openttd savegame version
	byte modified;     ///< modified flag

	/* This entry should only be generated from savegames. */
	GamelogEntryLegacyRev() : GamelogEntry(GLOG_LEGACYREV),
		newgrf(), slver(), modified() { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for savegames without log */
struct GamelogEntryOldVer : GamelogEntry {
	uint32 type;       ///< type of savegame
	uint32 version;    ///< combined ottd or ttdp version

	GamelogEntryOldVer() : GamelogEntry(GLOG_OLDVER), type(), version() { }

	GamelogEntryOldVer(const SavegameTypeVersion *stv) :
			GamelogEntry(GLOG_OLDVER) {
		this->type = stv->type;
		switch (type) {
			case SGT_TTDP1:
			case SGT_TTDP2: this->version = stv->ttdp.version; break;
			case SGT_OTTD:  this->version = (stv->ottd.version << 8) || (stv->ottd.minor_version & 0xFF); break;
			default:        this->version = 0; break;
		}
	}

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for emergency savegames. */
struct GamelogEntryEmergency : GamelogEntryTimed {
	GamelogEntryEmergency() : GamelogEntryTimed(GLOG_EMERGENCY) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for settings change */
struct GamelogEntrySetting : GamelogEntryTimed {
	char *name;        ///< name of the setting
	int32 oldval;      ///< old value
	int32 newval;      ///< new value

	GamelogEntrySetting() : GamelogEntryTimed(GLOG_SETTING),
		name(NULL), oldval(), newval() { }

	GamelogEntrySetting(const char *name, int32 oldval, int32 newval) :
		GamelogEntryTimed(GLOG_SETTING),
		name(strdup(name)), oldval(oldval), newval(newval) { }

	~GamelogEntrySetting() {
		free(this->name);
	}

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for cheating */
struct GamelogEntryCheat : GamelogEntryTimed {
	GamelogEntryCheat() : GamelogEntryTimed(GLOG_CHEAT) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for GRF config change begin */
struct GamelogEntryGRFBegin : GamelogEntryTimed {
	GamelogEntryGRFBegin() : GamelogEntryTimed(GLOG_GRFBEGIN) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for GRF config change end */
struct GamelogEntryGRFEnd : GamelogEntry {
	GamelogEntryGRFEnd() : GamelogEntry(GLOG_GRFEND) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for GRF addition */
struct GamelogEntryGRFAdd : GamelogEntry {
	GRFIdentifier grf; ///< ID and md5sum of added GRF

	GamelogEntryGRFAdd() : GamelogEntry(GLOG_GRFADD), grf() { }

	GamelogEntryGRFAdd(const GRFIdentifier *ident) :
		GamelogEntry(GLOG_GRFADD), grf(*ident) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for GRF removal */
struct GamelogEntryGRFRemove : GamelogEntry {
	uint32 grfid;      ///< ID of removed GRF

	GamelogEntryGRFRemove(uint32 grfid = 0) :
		GamelogEntry(GLOG_GRFREM), grfid(grfid) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for compatible GRF load */
struct GamelogEntryGRFCompat : GamelogEntry {
	GRFIdentifier grf; ///< ID and new md5sum of changed GRF

	GamelogEntryGRFCompat() : GamelogEntry(GLOG_GRFCOMPAT), grf() { }

	GamelogEntryGRFCompat(const GRFIdentifier *ident) :
		GamelogEntry(GLOG_GRFCOMPAT), grf(*ident) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for GRF parameter changes */
struct GamelogEntryGRFParam : GamelogEntry {
	uint32 grfid;      ///< ID of GRF with changed parameters

	GamelogEntryGRFParam(uint32 grfid = 0) :
		GamelogEntry(GLOG_GRFPARAM), grfid(grfid) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for GRF order change */
struct GamelogEntryGRFMove : GamelogEntry {
	uint32 grfid;      ///< ID of moved GRF
	int32 offset;      ///< offset, positive = move down

	GamelogEntryGRFMove() : GamelogEntry(GLOG_GRFMOVE), grfid(), offset() { }

	GamelogEntryGRFMove(uint32 grfid, int32 offset) :
		GamelogEntry(GLOG_GRFMOVE), grfid(grfid), offset(offset) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Gamelog entry for GRF bugs */
struct GamelogEntryGRFBug : GamelogEntryTimed {
	uint64 data;       ///< additional data
	uint32 grfid;      ///< ID of problematic GRF
	byte bug;          ///< type of bug, @see enum GRFBugs

	GamelogEntryGRFBug() : GamelogEntryTimed(GLOG_GRFBUG),
		data(), grfid(), bug() { }

	GamelogEntryGRFBug(uint32 grfid, byte bug, uint64 data) :
		GamelogEntryTimed(GLOG_GRFBUG),
		data(data), grfid(grfid), bug(bug) { }

	void Print(GamelogPrintBuffer *buf);
};

/** Get a new GamelogEntry by type (when loading a savegame) */
GamelogEntry *GamelogEntryByType(uint type);

#endif /* GAMELOG_ENTRIES_H */
