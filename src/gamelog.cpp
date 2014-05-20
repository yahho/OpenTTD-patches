/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelog.cpp Definition of functions used for logging of important changes in the game */

#include "stdafx.h"
#include "saveload/saveload.h"
#include "string.h"
#include "settings_type.h"
#include "gamelog_entries.h"
#include "console_func.h"
#include "debug.h"
#include "date_func.h"
#include "rev.h"

#include <stdarg.h>


Gamelog _gamelog; ///< gamelog

/**
 * Resets and frees all memory allocated - used before loading or starting a new game
 */
void GamelogReset()
{
	_gamelog.clear();
}


/**
 * Get some basic information from the given gamelog.
 * @param gamelog Pointer to the gamelog to extract information from.
 * @param [out] last_rev NewGRF version from the binary that last saved the savegame.
 * @param [out] ever_modified Max value of 'modified' from all binaries that ever saved this savegame.
 * @param [out] removed_newgrfs Set to true if any NewGRFs have been removed.
 */
void GamelogInfo(const Gamelog *gamelog, uint32 *last_rev, byte *ever_modified, bool *removed_newgrfs)
{
	for (Gamelog::const_iterator entry = gamelog->cbegin(); entry != gamelog->cend(); entry++) {
		switch ((*entry)->type) {
			default: break;

			case GLOG_REVISION: {
				const GamelogEntryRevision *rev = (GamelogEntryRevision*)entry->get();
				*last_rev = rev->newgrf;
				*ever_modified = max(*ever_modified, rev->modified);
				break;
			}

			case GLOG_GRFREM:
				*removed_newgrfs = true;
				break;
		}
	}
}


GamelogEntry *GamelogEntryByType(uint type)
{
	switch (type) {
		case GLOG_START:     return new GamelogEntryStart();
		case GLOG_STARTED:   return new GamelogEntryStarted();
		case GLOG_LOAD:      return new GamelogEntryLoad();
		case GLOG_LOADED:    return new GamelogEntryLoaded();
		case GLOG_MODE:      return new GamelogEntryMode();
		case GLOG_REVISION:  return new GamelogEntryRevision();
		case GLOG_LEGACYREV: return new GamelogEntryLegacyRev();
		case GLOG_OLDVER:    return new GamelogEntryOldVer();
		case GLOG_EMERGENCY: return new GamelogEntryEmergency();
		case GLOG_SETTING:   return new GamelogEntrySetting();
		case GLOG_CHEAT:     return new GamelogEntryCheat();
		case GLOG_GRFBEGIN:  return new GamelogEntryGRFBegin();
		case GLOG_GRFEND:    return new GamelogEntryGRFEnd();
		case GLOG_GRFADD:    return new GamelogEntryGRFAdd();
		case GLOG_GRFREM:    return new GamelogEntryGRFRemove();
		case GLOG_GRFCOMPAT: return new GamelogEntryGRFCompat();
		case GLOG_GRFPARAM:  return new GamelogEntryGRFParam();
		case GLOG_GRFMOVE:   return new GamelogEntryGRFMove();
		case GLOG_GRFBUG:    return new GamelogEntryGRFBug();
		default: NOT_REACHED();
	}
}


/**
 * Information about the presence of a Grf at a certain point during gamelog history
 * Note about missing Grfs:
 * Changes to missing Grfs are not logged including manual removal of the Grf.
 * So if the gamelog tells a Grf is missing we do not know whether it was readded or completely removed
 * at some later point.
 */
struct GRFPresence{
	const GRFConfig *gc;  ///< GRFConfig, if known
	bool was_missing;     ///< Grf was missing during some gameload in the past

	GRFPresence(const GRFConfig *gc) : gc(gc), was_missing(false) {}
};
typedef SmallMap<uint32, GRFPresence> GrfIDMapping;


/** Gamelog print buffer */
class GamelogPrintBuffer {
	static const uint LENGTH = 1024; ///< length of buffer for one line of text

	char buffer[LENGTH]; ///< output buffer
	uint offset;         ///< offset in buffer

public:
	GrfIDMapping grf_names; ///< keep track of this so that inconsistencies can be detected
	bool in_load;           ///< currently printing between GLOG_LOAD and GLOG_LOADED

	void reset() {
		this->offset = 0;
	}

	void append(const char *s, ...) WARN_FORMAT(2, 3) {
		if (this->offset >= this->LENGTH) return;

		va_list va;

		va_start(va, s);
		this->offset += vsnprintf(this->buffer + this->offset, this->LENGTH - this->offset, s, va);
		va_end(va);
	}

	void dump(GamelogPrintProc *proc) {
		proc(this->buffer);
	}
};

/**
 * Prints GRF ID, checksum and filename if found
 * @param buf The buffer to write to
 * @param grfid GRF ID
 * @param md5sum array of md5sum to print, if known
 * @param gc GrfConfig, if known
 */
static void PrintGrfInfo(GamelogPrintBuffer *buf, uint grfid, const uint8 *md5sum, const GRFConfig *gc)
{
	if (md5sum != NULL) {
		char txt [40];
		md5sumToString (txt, md5sum);
		buf->append("GRF ID %08X, checksum %s", BSWAP32(grfid), txt);
	} else {
		buf->append("GRF ID %08X", BSWAP32(grfid));
	}

	if (gc != NULL) {
		buf->append(", filename: %s (md5sum matches)", gc->filename);
	} else {
		gc = FindGRFConfig(grfid, FGCM_ANY);
		if (gc != NULL) {
			buf->append(", filename: %s (matches GRFID only)", gc->filename);
		} else {
			buf->append(", unknown GRF");
		}
	}
}

/**
 * Prints active gamelog
 * @param proc the procedure to draw with
 */
void GamelogPrint(GamelogPrintProc *proc)
{
	GamelogPrintBuffer buf;

	proc("---- gamelog start ----");

	for (Gamelog::const_iterator entry = _gamelog.cbegin(); entry != _gamelog.cend(); entry++) {
		buf.reset();
		(*entry)->Print(&buf);
		buf.dump(proc);
	}

	proc("---- gamelog end ----");
}


static void GamelogPrintConsoleProc(const char *s)
{
	IConsolePrint(CC_WARNING, s);
}

/** Print the gamelog data to the console. */
void GamelogPrintConsole()
{
	GamelogPrint(&GamelogPrintConsoleProc);
}

static int _gamelog_print_level = 0; ///< gamelog debug level we need to print stuff

static void GamelogPrintDebugProc(const char *s)
{
	DEBUG(gamelog, _gamelog_print_level, "%s", s);
}

/**
 * Prints gamelog to debug output. Code is executed even when
 * there will be no output. It is called very seldom, so it
 * doesn't matter that much. At least it gives more uniform code...
 * @param level debug level we need to print stuff
 */
void GamelogPrintDebug(int level)
{
	_gamelog_print_level = level;
	GamelogPrint(&GamelogPrintDebugProc);
}


/* Gamelog entry types */


/* Gamelog entry base class for entries with tick information. */

void GamelogEntryTimed::PrependTick(GamelogPrintBuffer *buf) {
	buf->append("Tick %u: ", (uint)this->tick);
}


/* Gamelog entry for game start */

void GamelogEntryStart::Print(GamelogPrintBuffer *buf) {
	this->PrependTick(buf);
	buf->append("New game");
}

void GamelogAddStart()
{
	_gamelog.append(new GamelogEntryStart());
}


/* Gamelog entry after game start */

void GamelogEntryStarted::Print(GamelogPrintBuffer *buf) {
	buf->append("    Game started");
}

void GamelogAddStarted()
{
	_gamelog.append(new GamelogEntryStarted());
}


/** Gamelog entry for game load */
void GamelogEntryLoad::Print(GamelogPrintBuffer *buf) {
	this->PrependTick(buf);
	buf->append("Load game");
	buf->in_load = true;
}

void GamelogAddLoad()
{
	_gamelog.append(new GamelogEntryLoad());
}


/* Gamelog entry after game load */

void GamelogEntryLoaded::Print(GamelogPrintBuffer *buf) {
	buf->append("    Game loaded");
	buf->in_load = false;
}

void GamelogAddLoaded()
{
	_gamelog.append(new GamelogEntryLoaded());
}


/* Gamelog entry for mode switch between scenario editor and game */

void GamelogEntryMode::Print(GamelogPrintBuffer *buf) {
	buf->append("    New game mode %u, landscape %u",
		(uint)this->mode, (uint)this->landscape);
}

/**
 * Logs a change in game mode (scenario editor or game)
 */
void GamelogAddMode()
{
	_gamelog.append(new GamelogEntryMode());
}

/**
 * Finds last stored game mode or landscape.
 * Any change is logged
 */
void GamelogTestMode()
{
	const GamelogEntryMode *mode = NULL;

	for (Gamelog::const_iterator entry = _gamelog.cbegin(); entry != _gamelog.cend(); entry++) {
		if ((*entry)->type == GLOG_MODE) {
			mode = (GamelogEntryMode*)entry->get();
		}
	}

	if (mode == NULL || mode->mode != _game_mode || mode->landscape != _settings_game.game_creation.landscape) GamelogAddMode();
}


/* Gamelog entry for game revision string */

void GamelogEntryRevision::Print(GamelogPrintBuffer *buf) {
	buf->append("    Revision text changed to %s, savegame version %u, %smodified, newgrf version 0x%08x",
		this->text, this->slver,
		(this->modified == 0) ? "not " : (this->modified == 1) ? "maybe " : "",
		this->newgrf);
}

/**
 * Logs a change in game revision
 */
void GamelogAddRevision()
{
	_gamelog.append(new GamelogEntryRevision());
}

/**
 * Finds out if current revision is different than last revision stored in the savegame.
 * Appends a revision entry when the revision string changed
 */
void GamelogTestRevision()
{
	const GamelogEntryRevision *rev = NULL;

	for (Gamelog::const_iterator entry = _gamelog.cbegin(); entry != _gamelog.cend(); entry++) {
		if ((*entry)->type == GLOG_REVISION) {
			rev = (GamelogEntryRevision*)entry->get();
		}
	}

	if (rev == NULL || strcmp(rev->text, _openttd_revision) != 0 ||
			rev->modified != _openttd_revision_modified ||
			rev->newgrf != _openttd_newgrf_version) {
		GamelogAddRevision();
	}
}


/* Gamelog entry for game revision string (legacy) */

void GamelogEntryLegacyRev::Print(GamelogPrintBuffer *buf) {
	buf->append("    Revision text changed to %s (legacy), savegame version %u, %smodified, newgrf version 0x%08x",
		this->text, this->slver,
		(this->modified == 0) ? "not " : (this->modified == 1) ? "maybe " : "",
		this->newgrf);
}


/* Gamelog entry for savegames without log */

void GamelogEntryOldVer::Print(GamelogPrintBuffer *buf) {
	switch (this->type) {
		case SGT_TTO:
			buf->append("    Conversion from TTO savegame");
			break;

		case SGT_TTD:
			buf->append("    Conversion from TTD savegame");
			break;

		case SGT_TTDP1:
		case SGT_TTDP2:
			buf->append("    Conversion from %s TTDP savegame version %u.%u.%u.%u",
				(this->type == SGT_TTDP1) ? "old" : "new",
				GB(this->version, 24,  8),
				GB(this->version, 20,  4),
				GB(this->version, 16,  4),
				GB(this->version,  0, 16));
			break;

		default: NOT_REACHED();
		case SGT_OTTD:
			buf->append("    Conversion from OTTD savegame without gamelog, version %u, %u",
				GB(this->version, 8, 16), GB(this->version, 0, 8));
			break;
	}
}

/**
 * Logs loading from savegame without gamelog
 */
void GamelogOldver(const SavegameTypeVersion *stv)
{
	_gamelog.append(new GamelogEntryOldVer(stv));
}


/* Gamelog entry for emergency savegames. */

void GamelogEntryEmergency::Print(GamelogPrintBuffer *buf) {
	this->PrependTick(buf);
	buf->append("Emergency savegame");
}

/**
 * Logs an emergency savegame
 */
void GamelogEmergency()
{
	_gamelog.append(new GamelogEntryEmergency());
}

/**
 * Finds out if current game is a loaded emergency savegame.
 */
bool GamelogTestEmergency()
{
	for (Gamelog::const_iterator entry = _gamelog.cbegin(); entry != _gamelog.cend(); entry++) {
		if ((*entry)->type == GLOG_EMERGENCY) return true;
	}

	return false;
}


/* Gamelog entry for settings change */

void GamelogEntrySetting::Print(GamelogPrintBuffer *buf) {
	this->PrependTick(buf);
	buf->append("Setting '%s' changed from %d to %d",
		this->name, this->oldval, this->newval);
}

/**
 * Logs change in game settings. Only non-networksafe settings are logged
 * @param name setting name
 * @param oldval old setting value
 * @param newval new setting value
 */
void GamelogSetting(const char *name, int32 oldval, int32 newval)
{
	_gamelog.append(new GamelogEntrySetting(name, oldval, newval));
}


/* Gamelog entry for cheating */

void GamelogEntryCheat::Print(GamelogPrintBuffer *buf) {
	this->PrependTick(buf);
	buf->append("Cheat used");
}


/* Gamelog entry for GRF config change begin */

void GamelogEntryGRFBegin::Print(GamelogPrintBuffer *buf) {
	this->PrependTick(buf);
	buf->append("GRF config change");
}

/**
 * Log GRF config change begin
 */
void GamelogGRFBegin()
{
	_gamelog.append(new GamelogEntryGRFBegin());
}


/* Gamelog entry for GRF config change end */

void GamelogEntryGRFEnd::Print(GamelogPrintBuffer *buf) {
	buf->append("    GRF config change end");
}

/**
 * Log GRF config change end
 */
void GamelogGRFEnd()
{
	_gamelog.append(new GamelogEntryGRFEnd());
}


/**
 * Decides if GRF should be logged
 * @param g grf to determine
 * @return true iff GRF is not static and is loaded
 */
static inline bool IsLoggableGrfConfig(const GRFConfig *g)
{
	return !HasBit(g->flags, GCF_STATIC) && g->status != GCS_NOT_FOUND;
}


/* Gamelog entry for GRF addition */

void GamelogEntryGRFAdd::Print(GamelogPrintBuffer *buf) {
	const GRFConfig *gc = FindGRFConfig(this->grf.grfid, FGCM_EXACT, this->grf.md5sum);
	buf->append("    Added NewGRF: ");
	PrintGrfInfo(buf, this->grf.grfid, this->grf.md5sum, gc);
	GrfIDMapping::Pair *gm = buf->grf_names.Find(this->grf.grfid);
	if (gm != buf->grf_names.End() && !gm->second.was_missing) buf->append(" (inconsistency: already added)");
	buf->grf_names[this->grf.grfid] = gc;
}

/**
 * Logs adding of a GRF
 * @param newg added GRF
 */
void GamelogGRFAdd(const GRFConfig *newg)
{
	if (!IsLoggableGrfConfig(newg)) return;

	_gamelog.append(new GamelogEntryGRFAdd(&newg->ident));
}

/**
 * Logs adding of list of GRFs.
 * Useful when old savegame is loaded or when new game is started
 * @param newg head of GRF linked list
 */
void GamelogGRFAddList(const GRFConfig *newg)
{
	for (; newg != NULL; newg = newg->next) {
		GamelogGRFAdd(newg);
	}
}


/* Gamelog entry for GRF removal */

void GamelogEntryGRFRemove::Print(GamelogPrintBuffer *buf) {
	GrfIDMapping::Pair *gm = buf->grf_names.Find(this->grfid);
	buf->append(buf->in_load ? "    Missing NewGRF: " : "    Removed NewGRF: ");
	PrintGrfInfo(buf, this->grfid, NULL, gm != buf->grf_names.End() ? gm->second.gc : NULL);
	if (gm == buf->grf_names.End()) {
		buf->append(" (inconsistency: never added)");
	} else if (buf->in_load) {
		/* Missing grfs on load are not removed from the configuration */
		gm->second.was_missing = true;
	} else {
		buf->grf_names.Erase(gm);
	}
}

/**
 * Logs removal of a GRF
 * @param grfid ID of removed GRF
 */
void GamelogGRFRemove(uint32 grfid)
{
	_gamelog.append(new GamelogEntryGRFRemove(grfid));
}


/* Gamelog entry for compatible GRF load */

void GamelogEntryGRFCompat::Print(GamelogPrintBuffer *buf) {
	const GRFConfig *gc = FindGRFConfig(this->grf.grfid, FGCM_EXACT, this->grf.md5sum);
	buf->append("    Compatible NewGRF loaded: ");
	PrintGrfInfo(buf, this->grf.grfid, this->grf.md5sum, gc);
	if (!buf->grf_names.Contains(this->grf.grfid)) buf->append(" (inconsistency: never added)");
	buf->grf_names[this->grf.grfid] = gc;
}

/**
 * Logs loading compatible GRF
 * (the same ID, but different MD5 hash)
 * @param newg new (updated) GRF
 */
void GamelogGRFCompatible(const GRFIdentifier *newg)
{
	_gamelog.append(new GamelogEntryGRFCompat(newg));
}


/* Gamelog entry for GRF parameter changes */

void GamelogEntryGRFParam::Print(GamelogPrintBuffer *buf) {
	GrfIDMapping::Pair *gm = buf->grf_names.Find(this->grfid);
	buf->append("    GRF parameter changed: ");
	PrintGrfInfo(buf, this->grfid, NULL, gm != buf->grf_names.End() ? gm->second.gc : NULL);
	if (gm == buf->grf_names.End()) buf->append(" (inconsistency: never added)");
}

/**
 * Logs change in GRF parameters.
 * Details about parameters changed are not stored
 * @param grfid ID of GRF to store
 */
static void GamelogGRFParameters(uint32 grfid)
{
	_gamelog.append(new GamelogEntryGRFParam(grfid));
}


/* Gamelog entry for GRF order change */

void GamelogEntryGRFMove::Print(GamelogPrintBuffer *buf) {
	GrfIDMapping::Pair *gm = buf->grf_names.Find(this->grfid);
	buf->append("GRF order changed: %08X moved %d places %s, ",
		BSWAP32(this->grfid), abs(this->offset), this->offset >= 0 ? "down" : "up");
	PrintGrfInfo(buf, this->grfid, NULL, gm != buf->grf_names.End() ? gm->second.gc : NULL);
	if (gm == buf->grf_names.End()) buf->append(" (inconsistency: never added)");
}

/**
 * Logs changing GRF order
 * @param grfid GRF that is moved
 * @param offset how far it is moved, positive = moved down
 */
static void GamelogGRFMove(uint32 grfid, int32 offset)
{
	_gamelog.append(new GamelogEntryGRFMove(grfid, offset));
}


/** List of GRFs using array of pointers instead of linked list */
struct GRFList {
	uint n;
	const GRFConfig *grf[];
};

/**
 * Generates GRFList
 * @param grfc head of GRF linked list
 */
static GRFList *GenerateGRFList(const GRFConfig *grfc)
{
	uint n = 0;
	for (const GRFConfig *g = grfc; g != NULL; g = g->next) {
		if (IsLoggableGrfConfig(g)) n++;
	}

	GRFList *list = (GRFList*)MallocT<byte>(sizeof(GRFList) + n * sizeof(GRFConfig*));

	list->n = 0;
	for (const GRFConfig *g = grfc; g != NULL; g = g->next) {
		if (IsLoggableGrfConfig(g)) list->grf[list->n++] = g;
	}

	return list;
}

/**
 * Compares two NewGRF lists and logs any change
 * @param oldc original GRF list
 * @param newc new GRF list
 */
void GamelogGRFUpdate(const GRFConfig *oldc, const GRFConfig *newc)
{
	GRFList *ol = GenerateGRFList(oldc);
	GRFList *nl = GenerateGRFList(newc);

	uint o = 0, n = 0;

	while (o < ol->n && n < nl->n) {
		const GRFConfig *og = ol->grf[o];
		const GRFConfig *ng = nl->grf[n];

		if (og->ident.grfid != ng->ident.grfid) {
			uint oi, ni;
			for (oi = 0; oi < ol->n; oi++) {
				if (ol->grf[oi]->ident.grfid == nl->grf[n]->ident.grfid) break;
			}
			if (oi < o) {
				/* GRF was moved, this change has been logged already */
				n++;
				continue;
			}
			if (oi == ol->n) {
				/* GRF couldn't be found in the OLD list, GRF was ADDED */
				GamelogGRFAdd(nl->grf[n++]);
				continue;
			}
			for (ni = 0; ni < nl->n; ni++) {
				if (nl->grf[ni]->ident.grfid == ol->grf[o]->ident.grfid) break;
			}
			if (ni < n) {
				/* GRF was moved, this change has been logged already */
				o++;
				continue;
			}
			if (ni == nl->n) {
				/* GRF couldn't be found in the NEW list, GRF was REMOVED */
				GamelogGRFRemove(ol->grf[o++]->ident.grfid);
				continue;
			}

			/* o < oi < ol->n
			 * n < ni < nl->n */
			assert(ni > n && ni < nl->n);
			assert(oi > o && oi < ol->n);

			ni -= n; // number of GRFs it was moved downwards
			oi -= o; // number of GRFs it was moved upwards

			if (ni >= oi) { // prefer the one that is moved further
				/* GRF was moved down */
				GamelogGRFMove(ol->grf[o++]->ident.grfid, ni);
			} else {
				GamelogGRFMove(nl->grf[n++]->ident.grfid, -(int)oi);
			}
		} else {
			if (memcmp(og->ident.md5sum, ng->ident.md5sum, sizeof(og->ident.md5sum)) != 0) {
				/* md5sum changed, probably loading 'compatible' GRF */
				GamelogGRFCompatible(&nl->grf[n]->ident);
			}

			if (og->num_params != ng->num_params || memcmp(og->param, ng->param, og->num_params * sizeof(og->param[0])) != 0) {
				GamelogGRFParameters(ol->grf[o]->ident.grfid);
			}

			o++;
			n++;
		}
	}

	while (o < ol->n) GamelogGRFRemove(ol->grf[o++]->ident.grfid); // remaining GRFs were removed ...
	while (n < nl->n) GamelogGRFAdd   (nl->grf[n++]);              // ... or added

	free(ol);
	free(nl);
}


/* Gamelog entry for GRF bugs */

void GamelogEntryGRFBug::Print(GamelogPrintBuffer *buf) {
	this->PrependTick(buf);
	GrfIDMapping::Pair *gm = buf->grf_names.Find(this->grfid);
	switch (this->bug) {
		default: NOT_REACHED();
		case GBUG_VEH_LENGTH:
			buf->append("Rail vehicle changes length outside a depot: GRF ID %08X, internal ID 0x%X", BSWAP32(this->grfid), (uint)this->data);
			break;
	}
	PrintGrfInfo(buf, this->grfid, NULL, gm != buf->grf_names.End() ? gm->second.gc : NULL);
	if (gm == buf->grf_names.End()) buf->append(" (inconsistency: never added)");
}

/**
 * Logs triggered GRF bug.
 * @param grfid ID of problematic GRF
 * @param bug type of bug, @see enum GRFBugs
 * @param data additional data
 */
static inline void GamelogGRFBug(uint32 grfid, byte bug, uint64 data)
{
	_gamelog.append(new GamelogEntryGRFBug(grfid, bug, data));
}

/**
 * Logs GRF bug - rail vehicle has different length after reversing.
 * Ensures this is logged only once for each GRF and engine type
 * This check takes some time, but it is called pretty seldom, so it
 * doesn't matter that much (ideally it shouldn't be called at all).
 * @param grfid the broken NewGRF
 * @param internal_id the internal ID of whatever's broken in the NewGRF
 * @return true iff a unique record was done
 */
bool GamelogGRFBugReverse(uint32 grfid, uint16 internal_id)
{
	for (Gamelog::const_iterator entry = _gamelog.cbegin(); entry != _gamelog.end(); entry++) {
		if ((*entry)->type == GLOG_GRFBUG) {
			const GamelogEntryGRFBug *bug = (GamelogEntryGRFBug*)entry->get();
			if (bug->bug == GBUG_VEH_LENGTH && bug->grfid == grfid && bug->data == internal_id) {
				return false;
			}
		}
	}

	GamelogGRFBug(grfid, GBUG_VEH_LENGTH, internal_id);

	return true;
}
