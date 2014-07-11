/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelog_sl.cpp Code handling saving and loading of gamelog data */

#include "../stdafx.h"
#include "../gamelog_entries.h"
#include "../fios.h"

#include "saveload_buffer.h"

extern Gamelog _gamelog;

static const SaveLoad _glog_start[] = {
	SLE_WRITEBYTE(GamelogEntryStart, type, GLOG_START),
	SLE_VAR(GamelogEntryStart, tick, SLE_UINT16),
	SLE_END()
};

static const SaveLoad _glog_started[] = {
	SLE_WRITEBYTE(GamelogEntryStarted, type, GLOG_STARTED),
	SLE_END()
};

static const SaveLoad _glog_load[] = {
	SLE_WRITEBYTE(GamelogEntryLoad, type, GLOG_LOAD),
	SLE_VAR(GamelogEntryLoad, tick, SLE_UINT16),
	SLE_END()
};

static const SaveLoad _glog_loaded[] = {
	SLE_WRITEBYTE(GamelogEntryLoaded, type, GLOG_LOADED),
	SLE_END()
};

static const SaveLoad _glog_mode[] = {
	SLE_WRITEBYTE(GamelogEntryMode, type, GLOG_MODE),
	SLE_VAR(GamelogEntryMode, mode,      SLE_UINT8),
	SLE_VAR(GamelogEntryMode, landscape, SLE_UINT8),
	SLE_END()
};

static const SaveLoad _glog_revision[] = {
	SLE_WRITEBYTE(GamelogEntryRevision, type, GLOG_REVISION),
	SLE_ARR(GamelogEntryRevision, text,     SLE_UINT8, NETWORK_REVISION_LENGTH),
	SLE_VAR(GamelogEntryRevision, newgrf,   SLE_UINT32),
	SLE_VAR(GamelogEntryRevision, slver,    SLE_UINT16),
	SLE_VAR(GamelogEntryRevision, modified, SLE_UINT8),
	SLE_END()
};

static const SaveLoad _glog_legacyrev[] = {
	SLE_WRITEBYTE(GamelogEntryRevision, type, GLOG_LEGACYREV),
	SLE_ARR(GamelogEntryLegacyRev, text,     SLE_UINT8, NETWORK_REVISION_LENGTH),
	SLE_VAR(GamelogEntryLegacyRev, newgrf,   SLE_UINT32),
	SLE_VAR(GamelogEntryLegacyRev, slver,    SLE_UINT16),
	SLE_VAR(GamelogEntryLegacyRev, modified, SLE_UINT8),
	SLE_END()
};

static const SaveLoad _glog_oldver[] = {
	SLE_WRITEBYTE(GamelogEntryOldVer, type, GLOG_OLDVER),
	SLE_VAR(GamelogEntryOldVer, savetype, SLE_UINT32),
	SLE_VAR(GamelogEntryOldVer, version,  SLE_UINT32),
	SLE_END()
};

static const SaveLoad _glog_emergency[] = {
	SLE_WRITEBYTE(GamelogEntryEmergency, type, GLOG_EMERGENCY),
	SLE_VAR(GamelogEntryEmergency, tick, SLE_UINT16),
	SLE_END()
};

static const SaveLoad _glog_setting[] = {
	SLE_WRITEBYTE(GamelogEntrySetting, type, GLOG_SETTING),
	SLE_VAR(GamelogEntrySetting, tick,   SLE_UINT16),
	SLE_STR(GamelogEntrySetting, name,   SLS_STR, 0),
	SLE_VAR(GamelogEntrySetting, oldval, SLE_INT32),
	SLE_VAR(GamelogEntrySetting, newval, SLE_INT32),
	SLE_END()
};

static const SaveLoad *_glog_setting_legacy = _glog_setting + 2;

static const SaveLoad _glog_cheat[] = {
	SLE_WRITEBYTE(GamelogEntryCheat, type, GLOG_CHEAT),
	SLE_VAR(GamelogEntryCheat, tick, SLE_UINT16),
	SLE_END()
};

static const SaveLoad _glog_grfbegin[] = {
	SLE_WRITEBYTE(GamelogEntryGRFBegin, type, GLOG_GRFBEGIN),
	SLE_VAR(GamelogEntryGRFBegin, tick, SLE_UINT16),
	SLE_END()
};

static const SaveLoad _glog_grfend[] = {
	SLE_WRITEBYTE(GamelogEntryGRFEnd, type, GLOG_GRFEND),
	SLE_END()
};

static const SaveLoad _glog_grfadd[] = {
	SLE_WRITEBYTE(GamelogEntryGRFAdd, type, GLOG_GRFADD),
	SLE_VAR(GamelogEntryGRFAdd, grf.grfid,  SLE_UINT32),
	SLE_ARR(GamelogEntryGRFAdd, grf.md5sum, SLE_UINT8, 16),
	SLE_END()
};

static const SaveLoad _glog_grfremove[] = {
	SLE_WRITEBYTE(GamelogEntryGRFRemove, type, GLOG_GRFREM),
	SLE_VAR(GamelogEntryGRFRemove, grfid, SLE_UINT32),
	SLE_END()
};

static const SaveLoad _glog_grfcompat[] = {
	SLE_WRITEBYTE(GamelogEntryGRFCompat, type, GLOG_GRFCOMPAT),
	SLE_VAR(GamelogEntryGRFCompat, grf.grfid,  SLE_UINT32),
	SLE_ARR(GamelogEntryGRFCompat, grf.md5sum, SLE_UINT8, 16),
	SLE_END()
};

static const SaveLoad _glog_grfparam[] = {
	SLE_WRITEBYTE(GamelogEntryGRFParam, type, GLOG_GRFPARAM),
	SLE_VAR(GamelogEntryGRFParam, grfid, SLE_UINT32),
	SLE_END()
};

static const SaveLoad _glog_grfmove[] = {
	SLE_WRITEBYTE(GamelogEntryGRFMove, type, GLOG_GRFMOVE),
	SLE_VAR(GamelogEntryGRFMove, grfid,  SLE_UINT32),
	SLE_VAR(GamelogEntryGRFMove, offset, SLE_INT32),
	SLE_END()
};

static const SaveLoad _glog_grfbug[] = {
	SLE_WRITEBYTE(GamelogEntryGRFBug, type, GLOG_GRFBUG),
	SLE_VAR(GamelogEntryGRFBug, tick,  SLE_UINT16),
	SLE_VAR(GamelogEntryGRFBug, data,  SLE_UINT64),
	SLE_VAR(GamelogEntryGRFBug, grfid, SLE_UINT32),
	SLE_VAR(GamelogEntryGRFBug, bug,   SLE_UINT8),
	SLE_END()
};

static const SaveLoad *_glog_grfbug_legacy = _glog_grfbug + 2;

static const SaveLoad *const _glog_desc[] = {
	_glog_start,
	_glog_started,
	_glog_load,
	_glog_loaded,
	_glog_mode,
	_glog_revision,
	_glog_legacyrev,
	_glog_oldver,
	_glog_emergency,
	_glog_setting,
	_glog_cheat,
	_glog_grfbegin,
	_glog_grfend,
	_glog_grfadd,
	_glog_grfremove,
	_glog_grfcompat,
	_glog_grfparam,
	_glog_grfmove,
	_glog_grfbug,
};

assert_compile(lengthof(_glog_desc) == GLOG_ENTRYTYPE_END);

static void Save_GLOG(SaveDumper *dumper)
{
	Gamelog::const_iterator entry = _gamelog.begin();
	uint i = 0;

	while (entry != _gamelog.end()) {
		const GamelogEntry *e = (entry++)->get();
		dumper->WriteElement(i++, e, _glog_desc[e->type]);
	}
}

static void Load_GLOG_common(LoadBuffer *reader, Gamelog *gamelog)
{
	assert(gamelog->size() == 0);

	if (reader->stv->type == SGT_FTTD) {
		while (reader->IterateChunk() != -1) {
			byte type = reader->ReadByte();
			if (type >= GLOG_ENTRYTYPE_END) throw SlCorrupt("Invalid gamelog entry type");

			GamelogEntry *e = GamelogEntryByType(type);
			reader->ReadObject(e, _glog_desc[type]);
			assert(e->type == type); // make sure descriptions are right
			gamelog->append(e);
		}
	} else {
		/* Import legacy gamelog */
		/* Note that in OTTD the gamelog was saved as a RIFF chunk */
		byte at;
		while ((at = reader->ReadByte()) != 0xFF) {
			uint16 tick = reader->ReadUint16();

			GamelogEntryTimed *t;
			switch (at) {
				case 0: // GLAT_START
					t = new GamelogEntryStart();
					break;
				case 1: // GLAT_LOAD
					t = new GamelogEntryLoad();
					break;
				case 2: // GLAT_GRF
					t = new GamelogEntryGRFBegin();
					break;

				case 3: { // GLAT_CHEAT
					GamelogEntryCheat *cheat = new GamelogEntryCheat();
					cheat->tick = tick;
					if (reader->ReadByte() != 0xFF) throw SlCorrupt("Invalid legacy gamelog cheat entry");
					gamelog->append(cheat);
					continue;
				}

				case 4: { // GLAT_SETTING
					GamelogEntrySetting *s = new GamelogEntrySetting();
					s->tick = tick;
					if (reader->ReadByte() != 3 /* GLCT_SETTING */) throw SlCorrupt("Invalid legacy gamelog setting entry");
					reader->ReadObject(s, _glog_setting_legacy);
					if (reader->ReadByte() != 0xFF) throw SlCorrupt("Unexpected legacy gamelog setting entry");
					gamelog->append(s);
					continue;
				}

				case 5: { // GLAT_GRFBUG
					GamelogEntryGRFBug *bug = new GamelogEntryGRFBug();
					bug->tick = tick;
					if (reader->ReadByte() != 9 /* GLCT_GRFBUG */) throw SlCorrupt("Invalid legacy gamelog grfbug entry");
					reader->ReadObject(bug, _glog_grfbug_legacy);
					if (reader->ReadByte() != 0xFF) throw SlCorrupt("Unexpected legacy gamelog grfbug entry");
					gamelog->append(bug);
					continue;
				}

				case 6: { // GLAT_EMERGENCY
					GamelogEntryEmergency *e = new GamelogEntryEmergency();
					e->tick = tick;
					if (reader->ReadByte() != 10 /* GLCT_EMERGENCY */) throw SlCorrupt("Invalid legacy gamelog emergency entry");
					if (reader->ReadByte() != 0xFF) throw SlCorrupt("Unexpected legacy gamelog emergencty entry");
					gamelog->append(e);
					continue;
				}

				default:
					throw SlCorrupt("Invalid legacy gamelog entry group type");
			}
			t->tick = tick;
			gamelog->append(t);

			byte ct;
			while ((ct = reader->ReadByte()) != 0xFF) {
				GamelogEntry *entry;
				const SaveLoad *desc;

				switch (ct) {
					case 0: // GLCT_MODE
						entry = new GamelogEntryMode();
						desc = _glog_mode;
						break;
					case 1: // GLCT_REVISION
						entry = new GamelogEntryLegacyRev();
						desc = _glog_legacyrev;
						break;
					case 2: // GLCT_OLDVER
						entry = new GamelogEntryOldVer();
						desc = _glog_oldver;
						break;
					case 4: // GLCT_GRFADD
						entry = new GamelogEntryGRFAdd();
						desc = _glog_grfadd;
						break;
					case 5: // GLCT_GRFREM
						entry = new GamelogEntryGRFRemove();
						desc = _glog_grfremove;
						break;
					case 6: // GLCT_GRFCOMPAT
						entry = new GamelogEntryGRFCompat();
						desc = _glog_grfcompat;
						break;
					case 7: // GLCT_GRFPARAM
						entry = new GamelogEntryGRFParam();
						desc = _glog_grfparam;
						break;
					case 8: // GLCT_GRFMOVE
						entry = new GamelogEntryGRFMove();
						desc = _glog_grfmove;
						break;
					default:
						throw SlCorrupt("Invalid legacy gamelog entry type");
				}

				reader->ReadObject(entry, desc);
				gamelog->append(entry);
			}

			GamelogEntry *f;
			switch (at) {
				case 0: // GLAT_START
					f = new GamelogEntryStarted();
					break;
				case 1: // GLAT_LOAD
					f = new GamelogEntryLoaded();
					break;
				case 2: // GLAT_GRF
					f = new GamelogEntryGRFEnd();
					break;
			}
			gamelog->append(f);
		}
	}
}

static void Load_GLOG(LoadBuffer *reader)
{
	Load_GLOG_common(reader, &_gamelog);
}

static void Check_GLOG(LoadBuffer *reader)
{
	Load_GLOG_common(reader, &_load_check_data.gamelog);
}

extern const ChunkHandler _gamelog_chunk_handlers[] = {
	{ 'GLOG', Save_GLOG, Load_GLOG, NULL, Check_GLOG, CH_ARRAY | CH_LAST }
};
