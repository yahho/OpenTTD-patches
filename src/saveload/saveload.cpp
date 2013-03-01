/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file saveload.cpp
 * All actions handling saving and loading goes on in this file. The general actions
 * are as follows for saving a game (loading is analogous):
 * <ol>
 * <li>initialize the writer by creating a temporary memory-buffer for it
 * <li>go through all to-be saved elements, each 'chunk' (#ChunkHandler) prefixed by a label
 * <li>use their description array (#SaveLoad) to know what elements to save and in what version
 *    of the game it was active (used when loading)
 * <li>write all data byte-by-byte to the temporary buffer so it is endian-safe
 * <li>when the buffer is full; flush it to the output (eg save to file) (_sl.buf, _sl.bufp, _sl.bufe)
 * <li>repeat this until everything is done, and flush any remaining output to file
 * </ol>
 */

#include <signal.h>
#include <exception>

#include "../stdafx.h"
#include "../debug.h"
#include "../gfx_func.h"
#include "../thread/thread.h"
#include "../network/network.h"
#include "../window_func.h"
#include "../strings_func.h"
#include "../core/endian_func.hpp"
#include "../company_func.h"
#include "../date_func.h"
#include "../statusbar_gui.h"
#include "../fileio_func.h"
#include "../gamelog.h"
#include "../gamelog_internal.h"
#include "../string_func.h"
#include "../fios.h"
#include "../error.h"

#include "table/strings.h"

#include "saveload_internal.h"
#include "saveload_filter.h"
#include "saveload_buffer.h"
#include "saveload_error.h"

/*
 * Previous savegame versions, the trunk revision where they were
 * introduced and the released version that had that particular
 * savegame version.
 * Up to savegame version 18 there is a minor version as well.
 *
 *    1.0         0.1.x, 0.2.x
 *    2.0         0.3.0
 *    2.1         0.3.1, 0.3.2
 *    3.x         lost
 *    4.0     1
 *    4.1   122   0.3.3, 0.3.4
 *    4.2  1222   0.3.5
 *    4.3  1417
 *    4.4  1426
 *    5.0  1429
 *    5.1  1440
 *    5.2  1525   0.3.6
 *    6.0  1721
 *    6.1  1768
 *    7.0  1770
 *    8.0  1786
 *    9.0  1909
 *   10.0  2030
 *   11.0  2033
 *   11.1  2041
 *   12.1  2046
 *   13.1  2080   0.4.0, 0.4.0.1
 *   14.0  2441
 *   15.0  2499
 *   16.0  2817
 *   16.1  3155
 *   17.0  3212
 *   17.1  3218
 *   18    3227
 *   19    3396
 *   20    3403
 *   21    3472   0.4.x
 *   22    3726
 *   23    3915
 *   24    4150
 *   25    4259
 *   26    4466
 *   27    4757
 *   28    4987
 *   29    5070
 *   30    5946
 *   31    5999
 *   32    6001
 *   33    6440
 *   34    6455
 *   35    6602
 *   36    6624
 *   37    7182
 *   38    7195
 *   39    7269
 *   40    7326
 *   41    7348   0.5.x
 *   42    7573
 *   43    7642
 *   44    8144
 *   45    8501
 *   46    8705
 *   47    8735
 *   48    8935
 *   49    8969
 *   50    8973
 *   51    8978
 *   52    9066
 *   53    9316
 *   54    9613
 *   55    9638
 *   56    9667
 *   57    9691
 *   58    9762
 *   59    9779
 *   60    9874
 *   61    9892
 *   62    9905
 *   63    9956
 *   64   10006
 *   65   10210
 *   66   10211
 *   67   10236
 *   68   10266
 *   69   10319
 *   70   10541
 *   71   10567
 *   72   10601
 *   73   10903
 *   74   11030
 *   75   11107
 *   76   11139
 *   77   11172
 *   78   11176
 *   79   11188
 *   80   11228
 *   81   11244
 *   82   11410
 *   83   11589
 *   84   11822
 *   85   11874
 *   86   12042
 *   87   12129
 *   88   12134
 *   89   12160
 *   90   12293
 *   91   12347
 *   92   12381   0.6.x
 *   93   12648
 *   94   12816
 *   95   12924
 *   96   13226
 *   97   13256
 *   98   13375
 *   99   13838
 *  100   13952
 *  101   14233
 *  102   14332
 *  103   14598
 *  104   14735
 *  105   14803
 *  106   14919
 *  107   15027
 *  108   15045
 *  109   15075
 *  110   15148
 *  111   15190
 *  112   15290
 *  113   15340
 *  114   15601
 *  115   15695
 *  116   15893   0.7.x
 *  117   16037
 *  118   16129
 *  119   16242
 *  120   16439
 *  121   16694
 *  122   16855
 *  123   16909
 *  124   16993
 *  125   17113
 *  126   17433
 *  127   17439
 *  128   18281
 *  129   18292
 *  130   18404
 *  131   18481
 *  132   18522
 *  133   18674
 *  134   18703
 *  135   18719
 *  136   18764
 *  137   18912
 *  138   18942   1.0.x
 *  139   19346
 *  140   19382
 *  141   19799
 *  142   20003
 *  143   20048
 *  144   20334
 *  145   20376
 *  146   20446
 *  147   20621
 *  148   20659
 *  149   20832
 *  150   20857
 *  151   20918
 *  152   21171
 *  153   21263
 *  154   21426
 *  155   21453
 *  156   21728
 *  157   21862
 *  158   21933
 *  159   21962
 *  160   21974   1.1.x
 *  161   22567
 *  162   22713
 *  163   22767
 *  164   23290
 *  165   23304
 *  166   23415
 *  167   23504
 *  168   23637
 *  169   23816
 *  170   23826
 *  171   23835
 *  172   23947
 *  173   23967   1.2.0-RC1
 *  174   23973   1.2.x
 *  175   24136
 *  176   24446
 *  177   24619
 *  178   24789
 *  179   24810
 *  180   24998   1.3.x
 *  181   25012
 *  182   25296
 *  183   25363
 *  184   25508
 *  185   25620
 */
extern const uint16 SAVEGAME_VERSION = 185; ///< Current savegame version of OpenTTD.

char _savegame_format[8]; ///< how to compress savegames
bool _do_autosave;        ///< are we doing an autosave at the moment?

/** What are we currently doing? */
enum SaveLoadAction {
	SLA_LOAD,        ///< loading
	SLA_SAVE,        ///< saving
	SLA_PTRS,        ///< fixing pointers
	SLA_NULL,        ///< null all pointers (on loading error)
	SLA_LOAD_CHECK,  ///< partial loading into #_load_check_data
};

/** The saveload struct, containing reader-writer functions, buffer, version, etc. */
struct SaveLoadParams {
	SlErrorData error;                   ///< the error to show

	byte ff_state;                       ///< The state of fast-forward when saving started.
	bool saveinprogress;                 ///< Whether there is currently a save in progress.
};

static SaveLoadParams _sl; ///< Parameters used for/at saveload.

/* these define the chunks */
extern const ChunkHandler _gamelog_chunk_handlers[];
extern const ChunkHandler _map_chunk_handlers[];
extern const ChunkHandler _misc_chunk_handlers[];
extern const ChunkHandler _name_chunk_handlers[];
extern const ChunkHandler _cheat_chunk_handlers[] ;
extern const ChunkHandler _setting_chunk_handlers[];
extern const ChunkHandler _company_chunk_handlers[];
extern const ChunkHandler _engine_chunk_handlers[];
extern const ChunkHandler _veh_chunk_handlers[];
extern const ChunkHandler _waypoint_chunk_handlers[];
extern const ChunkHandler _depot_chunk_handlers[];
extern const ChunkHandler _order_chunk_handlers[];
extern const ChunkHandler _town_chunk_handlers[];
extern const ChunkHandler _sign_chunk_handlers[];
extern const ChunkHandler _station_chunk_handlers[];
extern const ChunkHandler _industry_chunk_handlers[];
extern const ChunkHandler _economy_chunk_handlers[];
extern const ChunkHandler _subsidy_chunk_handlers[];
extern const ChunkHandler _cargomonitor_chunk_handlers[];
extern const ChunkHandler _goal_chunk_handlers[];
extern const ChunkHandler _story_page_chunk_handlers[];
extern const ChunkHandler _ai_chunk_handlers[];
extern const ChunkHandler _game_chunk_handlers[];
extern const ChunkHandler _animated_tile_chunk_handlers[];
extern const ChunkHandler _newgrf_chunk_handlers[];
extern const ChunkHandler _group_chunk_handlers[];
extern const ChunkHandler _cargopacket_chunk_handlers[];
extern const ChunkHandler _autoreplace_chunk_handlers[];
extern const ChunkHandler _labelmaps_chunk_handlers[];
extern const ChunkHandler _linkgraph_chunk_handlers[];
extern const ChunkHandler _airport_chunk_handlers[];
extern const ChunkHandler _object_chunk_handlers[];
extern const ChunkHandler _persistent_storage_chunk_handlers[];

/** Array of all chunks in a savegame, \c NULL terminated. */
static const ChunkHandler * const _chunk_handlers[] = {
	_gamelog_chunk_handlers,
	_map_chunk_handlers,
	_misc_chunk_handlers,
	_name_chunk_handlers,
	_cheat_chunk_handlers,
	_setting_chunk_handlers,
	_veh_chunk_handlers,
	_waypoint_chunk_handlers,
	_depot_chunk_handlers,
	_order_chunk_handlers,
	_industry_chunk_handlers,
	_economy_chunk_handlers,
	_subsidy_chunk_handlers,
	_cargomonitor_chunk_handlers,
	_goal_chunk_handlers,
	_story_page_chunk_handlers,
	_engine_chunk_handlers,
	_town_chunk_handlers,
	_sign_chunk_handlers,
	_station_chunk_handlers,
	_company_chunk_handlers,
	_ai_chunk_handlers,
	_game_chunk_handlers,
	_animated_tile_chunk_handlers,
	_newgrf_chunk_handlers,
	_group_chunk_handlers,
	_cargopacket_chunk_handlers,
	_autoreplace_chunk_handlers,
	_labelmaps_chunk_handlers,
	_linkgraph_chunk_handlers,
	_airport_chunk_handlers,
	_object_chunk_handlers,
	_persistent_storage_chunk_handlers,
	NULL,
};

/**
 * Iterate over all chunk handlers.
 * @param ch the chunk handler iterator
 */
#define FOR_ALL_CHUNK_HANDLERS(ch) \
	for (const ChunkHandler * const *chsc = _chunk_handlers; *chsc != NULL; chsc++) \
		for (const ChunkHandler *ch = *chsc; ch != NULL; ch = (ch->flags & CH_LAST) ? NULL : ch + 1)


typedef void (*AsyncSaveFinishProc)();                ///< Callback for when the savegame loading is finished.
static AsyncSaveFinishProc _async_save_finish = NULL; ///< Callback to call when the savegame loading is finished.
static ThreadObject *_save_thread;                    ///< The thread we're using to compress and write a savegame

/**
 * Called by save thread to tell we finished saving.
 * @param proc The callback to call when saving is done.
 */
static void SetAsyncSaveFinish(AsyncSaveFinishProc proc)
{
	if (_exit_game) return;
	while (_async_save_finish != NULL) CSleep(10);

	_async_save_finish = proc;
}

/**
 * Handle async save finishes.
 */
void ProcessAsyncSaveFinish()
{
	if (_async_save_finish == NULL) return;

	_async_save_finish();

	_async_save_finish = NULL;

	if (_save_thread != NULL) {
		_save_thread->Join();
		delete _save_thread;
		_save_thread = NULL;
	}
}


/**
 * Save all chunks
 * @param dumper The dumper object to write to
 */
static void SlSaveChunks(SaveDumper *dumper)
{
	FOR_ALL_CHUNK_HANDLERS(ch) {
		ChunkSaveProc *proc = ch->save_proc;

		/* Don't save any chunk information if there is no save handler. */
		if (proc == NULL) continue;

		dumper->WriteUint32(ch->id);
		DEBUG(sl, 2, "Saving chunk %c%c%c%c", ch->id >> 24, ch->id >> 16, ch->id >> 8, ch->id);

		dumper->BeginChunk(ch->flags & CH_TYPE_MASK);
		proc(dumper);
		dumper->EndChunk();
	}

	/* Terminator */
	dumper->WriteUint32(0);
}

/**
 * Find the ChunkHandler that will be used for processing the found
 * chunk in the savegame or in memory
 * @param id the chunk in question
 * @return returns the appropriate chunkhandler
 */
static const ChunkHandler *SlFindChunkHandler(uint32 id)
{
	FOR_ALL_CHUNK_HANDLERS(ch) if (ch->id == id) return ch;
	return NULL;
}

/**
 * Load all chunks
 * @param reader The reader object to read from
 * @param test Whether the load is done to check a savegame
 */
static void SlLoadChunks(LoadBuffer *reader, bool check = false)
{
	uint32 id;
	const ChunkHandler *ch;

	for (id = reader->ReadUint32(); id != 0; id = reader->ReadUint32()) {
		DEBUG(sl, 2, "Loading chunk %c%c%c%c", id >> 24, id >> 16, id >> 8, id);

		ch = SlFindChunkHandler(id);
		if (ch == NULL) throw SlCorrupt("Unknown chunk type");

		reader->BeginChunk();

		if (!check) {
			ch->load_proc(reader);
		} else if (ch->load_check_proc) {
			ch->load_check_proc(reader);
		} else {
			reader->SkipChunk();
		}

		reader->EndChunk();
	}
}

/**
 * Fix all pointers (convert index -> pointer)
 * If stv is NULL, set them to NULL.
 */
static void SlFixPointers(const SavegameTypeVersion *stv)
{
	const char *const desc = (stv != NULL) ? "Fixing pointers" : "Nulling pointers";

	DEBUG(sl, 1, "%s", desc);

	FOR_ALL_CHUNK_HANDLERS(ch) {
		if (ch->ptrs_proc != NULL) {
			DEBUG(sl, 2, "%s for %c%c%c%c", desc, ch->id >> 24, ch->id >> 16, ch->id >> 8, ch->id);
			ch->ptrs_proc(stv);
		}
	}

	DEBUG(sl, 1, "%s done", desc);
}

/** Null all pointers (convert index -> NULL) */
static inline void SlNullPointers()
{
	SlFixPointers(NULL);
}


/** Yes, simply writing to a file. */
struct FileWriter : SaveFilter {
	FILE *file; ///< The file to write to.

	/**
	 * Create the file writer, so it writes to a specific file.
	 * @param file The file to write to.
	 */
	FileWriter(FILE *file) : SaveFilter(), file(file)
	{
	}

	/** Make sure everything is cleaned up. */
	~FileWriter()
	{
		this->Finish();
	}

	/* virtual */ void Write(const byte *buf, size_t size)
	{
		/* We're in the process of shutting down, i.e. in "failure" mode. */
		if (this->file == NULL) return;

		if (fwrite(buf, 1, size, this->file) != size) throw SlException(STR_GAME_SAVELOAD_ERROR_FILE_NOT_WRITEABLE);
	}

	/* virtual */ void Finish()
	{
		if (this->file != NULL) fclose(this->file);
		this->file = NULL;
	}
};

/* actual loader/saver function */
void InitializeGame(uint size_x, uint size_y, bool reset_date, bool reset_settings);
extern void AfterLoadGame(const SavegameTypeVersion *stv);
extern bool LoadOldSaveGame(LoadFilter *reader, SavegameTypeVersion *stv, SlErrorData *e);

/**
 * Update the gui accordingly when starting saving
 * and set locks on saveload. Also turn off fast-forward cause with that
 * saving takes Aaaaages
 */
static void SaveFileStart()
{
	_sl.ff_state = _fast_forward;
	_fast_forward = 0;
	if (_cursor.sprite == SPR_CURSOR_MOUSE) SetMouseCursor(SPR_CURSOR_ZZZ, PAL_NONE);

	InvalidateWindowData(WC_STATUS_BAR, 0, SBI_SAVELOAD_START);
	_sl.saveinprogress = true;
}

/** Update the gui accordingly when saving is done and release locks on saveload. */
static void SaveFileDone()
{
	if (_game_mode != GM_MENU) _fast_forward = _sl.ff_state;
	if (_cursor.sprite == SPR_CURSOR_ZZZ) SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);

	InvalidateWindowData(WC_STATUS_BAR, 0, SBI_SAVELOAD_FINISH);
	_sl.saveinprogress = false;
}

/** Get the string representation of the error message */
const char *GetSaveLoadErrorString()
{
	SetDParamStr(0, _sl.error.data);

	static char err_str[512];
	GetString(err_str, _sl.error.str, lastof(err_str));
	return err_str;
}

/** Show a gui message when saving has failed */
static void SaveFileError()
{
	SetDParamStr(0, GetSaveLoadErrorString());
	ShowErrorMessage(STR_ERROR_GAME_SAVE_FAILED, STR_JUST_RAW_STRING, WL_ERROR);
	SaveFileDone();
}

/**
 * We have written the whole game into memory, _memory_savegame, now find
 * and appropriate compressor and start writing to file.
 */
static bool SaveFileToDisk(SaveFilter *writer, SaveDumper *dumper, bool threaded)
{
	AsyncSaveFinishProc asfp = SaveFileDone;
	bool res;

	try {
		byte compression;
		const SaveLoadFormat *fmt = GetSavegameFormat(_savegame_format, &compression);

		/* We have written our stuff to memory, now write it to file! */
		uint32 hdr[2] = { fmt->tag, TO_BE32(SAVEGAME_VERSION << 16) };
		writer->Write((byte*)hdr, sizeof(hdr));

		writer = fmt->init_write(writer, compression);
		dumper->Flush(writer);

		res = true;
	} catch (SlException e) {
		_sl.error = e.error;

		/* We don't want to shout when saving is just
		 * cancelled due to a client disconnecting. */
		if (_sl.error.str != STR_NETWORK_ERROR_LOSTCONNECTION) {
			/* Skip the "colour" character */
			DEBUG(sl, 0, "%s", GetSaveLoadErrorString() + 3);
			asfp = SaveFileError;
		}

		res = false;
	}

	delete writer;
	delete dumper;

	if (threaded) {
		SetAsyncSaveFinish(asfp);
	} else {
		asfp();
	}

	return res;
}

struct WriterDumper {
	SaveFilter *writer;
	SaveDumper *dumper;
};

/** Thread run function for saving the file to disk. */
static void SaveFileToDiskThread(void *arg)
{
	WriterDumper *data = (WriterDumper*)arg;

	SaveFileToDisk(data->writer, data->dumper, true);

	delete data;
}

void WaitTillSaved()
{
	if (_save_thread == NULL) return;

	_save_thread->Join();
	delete _save_thread;
	_save_thread = NULL;

	/* Make sure every other state is handled properly as well. */
	ProcessAsyncSaveFinish();
}

/**
 * Actually perform the saving of the savegame.
 * General tactics is to first save the game to memory, then write it to file
 * using the writer, either in threaded mode if possible, or single-threaded.
 * @param writer   The filter to write the savegame to.
 * @param threaded Whether to try to perform the saving asynchronously.
 * @return Return whether saving was successful
 */
static bool DoSave(SaveFilter *writer, bool threaded)
{
	assert(!_sl.saveinprogress);

	SaveDumper *dumper = new SaveDumper();

	SaveViewportBeforeSaveGame();
	SlSaveChunks(dumper);

	SaveFileStart();

	if (threaded) {
		WriterDumper *data = new WriterDumper;
		data->writer = writer;
		data->dumper = dumper;

		if (ThreadObject::New(&SaveFileToDiskThread, data, &_save_thread)) {
			return true;
		}

		DEBUG(sl, 1, "Cannot create savegame thread, reverting to single-threaded mode...");
		delete data;
	}

	return SaveFileToDisk(writer, dumper, false);
}

/**
 * Save the game using a (writer) filter.
 * @param writer   The filter to write the savegame to.
 * @param threaded Whether to try to perform the saving asynchronously.
 * @return Return whether saving was successful
 */
bool SaveWithFilter(SaveFilter *writer, bool threaded)
{
	return DoSave(writer, threaded);
}

/**
 * Main Save function where the high-level saveload functions are
 * handled.
 * @param filename The name of the savegame being created
 * @param sb The sub directory to save the savegame in
 * @param threaded True when threaded saving is allowed
 * @return Return whether saving was successful
 */
bool SaveGame(const char *filename, Subdirectory sb, bool threaded)
{
	/* An instance of saving is already active, so don't go saving again */
	if (_sl.saveinprogress && threaded) {
		/* if not an autosave, but a user action, show error message */
		if (!_do_autosave) ShowErrorMessage(STR_ERROR_SAVE_STILL_IN_PROGRESS, INVALID_STRING_ID, WL_ERROR);
		return true;
	}
	WaitTillSaved();

	FILE *fh = FioFOpenFile(filename, "wb", sb);

	if (fh == NULL) {
		_sl.error.str = STR_GAME_SAVELOAD_ERROR_FILE_NOT_WRITEABLE;

		/* Skip the "colour" character */
		DEBUG(sl, 0, "%s", GetSaveLoadErrorString() + 3);

		return false;
	}

	DEBUG(desync, 1, "save: %08x; %02x; %s", _date, _date_fract, filename);
	if (_network_server || !_settings_client.gui.threaded_saves) threaded = false;

	return DoSave(new FileWriter(fh), threaded);
}


typedef void (CDECL *SignalHandlerPointer)(int);
static SignalHandlerPointer _prev_segfault = NULL;
static SignalHandlerPointer _prev_abort    = NULL;
static SignalHandlerPointer _prev_fpe      = NULL;

static void CDECL HandleSavegameLoadCrash(int signum);

/**
 * Replaces signal handlers of SIGSEGV and SIGABRT
 * and stores pointers to original handlers in memory.
 */
static void SetSignalHandlers()
{
	_prev_segfault = signal(SIGSEGV, HandleSavegameLoadCrash);
	_prev_abort    = signal(SIGABRT, HandleSavegameLoadCrash);
	_prev_fpe      = signal(SIGFPE,  HandleSavegameLoadCrash);
}

/**
 * Resets signal handlers back to original handlers.
 */
static void ResetSignalHandlers()
{
	signal(SIGSEGV, _prev_segfault);
	signal(SIGABRT, _prev_abort);
	signal(SIGFPE,  _prev_fpe);
}

/**
 * Try to find the overridden GRF identifier of the given GRF.
 * @param c the GRF to get the 'previous' version of.
 * @return the GRF identifier or \a c if none could be found.
 */
static const GRFIdentifier *GetOverriddenIdentifier(const GRFConfig *c)
{
	const LoggedAction *la = &_gamelog_action[_gamelog_actions - 1];
	if (la->at != GLAT_LOAD) return &c->ident;

	const LoggedChange *lcend = &la->change[la->changes];
	for (const LoggedChange *lc = la->change; lc != lcend; lc++) {
		if (lc->ct == GLCT_GRFCOMPAT && lc->grfcompat.grfid == c->ident.grfid) return &lc->grfcompat;
	}

	return &c->ident;
}

/** Was the saveload crash because of missing NewGRFs? */
static bool _saveload_crash_with_missing_newgrfs = false;

/**
 * Did loading the savegame cause a crash? If so,
 * were NewGRFs missing?
 * @return when the saveload crashed due to missing NewGRFs.
 */
bool SaveloadCrashWithMissingNewGRFs()
{
	return _saveload_crash_with_missing_newgrfs;
}

/**
 * Signal handler used to give a user a more useful report for crashes during
 * the savegame loading process; especially when there's problems with the
 * NewGRFs that are required by the savegame.
 * @param signum received signal
 */
static void CDECL HandleSavegameLoadCrash(int signum)
{
	ResetSignalHandlers();

	char buffer[8192];
	char *p = buffer;
	p += seprintf(p, lastof(buffer), "Loading your savegame caused OpenTTD to crash.\n");

	for (const GRFConfig *c = _grfconfig; !_saveload_crash_with_missing_newgrfs && c != NULL; c = c->next) {
		_saveload_crash_with_missing_newgrfs = HasBit(c->flags, GCF_COMPATIBLE) || c->status == GCS_NOT_FOUND;
	}

	if (_saveload_crash_with_missing_newgrfs) {
		p += seprintf(p, lastof(buffer),
			"This is most likely caused by a missing NewGRF or a NewGRF that\n"
			"has been loaded as replacement for a missing NewGRF. OpenTTD\n"
			"cannot easily determine whether a replacement NewGRF is of a newer\n"
			"or older version.\n"
			"It will load a NewGRF with the same GRF ID as the missing NewGRF.\n"
			"This means that if the author makes incompatible NewGRFs with the\n"
			"same GRF ID OpenTTD cannot magically do the right thing. In most\n"
			"cases OpenTTD will load the savegame and not crash, but this is an\n"
			"exception.\n"
			"Please load the savegame with the appropriate NewGRFs installed.\n"
			"The missing/compatible NewGRFs are:\n");

		for (const GRFConfig *c = _grfconfig; c != NULL; c = c->next) {
			if (HasBit(c->flags, GCF_COMPATIBLE)) {
				const GRFIdentifier *replaced = GetOverriddenIdentifier(c);
				char buf[40];
				md5sumToString(buf, lastof(buf), replaced->md5sum);
				p += seprintf(p, lastof(buffer), "NewGRF %08X (checksum %s) not found.\n  Loaded NewGRF \"%s\" with same GRF ID instead.\n", BSWAP32(c->ident.grfid), buf, c->filename);
			}
			if (c->status == GCS_NOT_FOUND) {
				char buf[40];
				md5sumToString(buf, lastof(buf), c->ident.md5sum);
				p += seprintf(p, lastof(buffer), "NewGRF %08X (%s) not found; checksum %s.\n", BSWAP32(c->ident.grfid), c->filename, buf);
			}
		}
	} else {
		p += seprintf(p, lastof(buffer),
			"This is probably caused by a corruption in the savegame.\n"
			"Please file a bug report and attach this savegame.\n");
	}

	ShowInfo(buffer);

	SignalHandlerPointer call = NULL;
	switch (signum) {
		case SIGSEGV: call = _prev_segfault; break;
		case SIGABRT: call = _prev_abort; break;
		case SIGFPE:  call = _prev_fpe; break;
		default: NOT_REACHED();
	}
	if (call != NULL) call(signum);
}


/**
 * Determine version of format of a (non-old) savegame.
 * @param chain The filter chain head to read the savegame from, to which a chain filter will be prepended
 * @param stv   The SavegameTypeVersion object in which to store the savegame version.
 */
static void LoadSavegameFormat(LoadFilter **chain, SavegameTypeVersion *stv)
{
	stv->type = SGT_OTTD;

	uint32 hdr[2];
	if ((*chain)->Read((byte*)hdr, sizeof(hdr)) != sizeof(hdr)) throw SlException(STR_GAME_SAVELOAD_ERROR_FILE_NOT_READABLE);

	/* see if we have any loader for this type. */
	const SaveLoadFormat *fmt = GetSavegameFormatByTag(hdr[0]);

	if (fmt != NULL) {
		/* check version number */
		stv->ottd.version = TO_BE32(hdr[1]) >> 16;
		/* Minor is not used anymore from version 18.0, but it is still needed
		 * in versions before that (4 cases) which can't be removed easy.
		 * Therefore it is loaded, but never saved (or, it saves a 0 in any scenario). */
		stv->ottd.minor_version = (TO_BE32(hdr[1]) >> 8) & 0xFF;

		DEBUG(sl, 1, "Loading savegame version %d", stv->ottd.version);

		/* Is the version higher than the current? */
		if (stv->ottd.version > SAVEGAME_VERSION) throw SlException(STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME);
	} else {
		/* No loader found, treat as version 0 and use LZO format */
		DEBUG(sl, 0, "Unknown savegame type, trying to load it as the buggy format");
		(*chain)->Reset();
		stv->ottd.version = 0;
		stv->ottd.minor_version = 0;

		/* Try to find the LZO savegame format. */
		fmt = GetLZO0SavegameFormat();
		/* Who removed LZO support? Bad bad boy! */
		assert(fmt != NULL);
	}

	/* loader for this savegame type is not implemented? */
	if (fmt->init_load == NULL) {
		throw SlException(STR_GAME_SAVELOAD_ERROR_MISSING_LOADER, fmt->name);
	}

	*chain = fmt->init_load(*chain);
}

/**
 * Actually perform the loading of a "non-old" savegame.
 * @param chain      The filter chain head to read the savegame from.
 * @param mode Load mode. Load can also be a TTD(Patch) game. Use #SL_LOAD, #SL_OLD_LOAD or #SL_LOAD_CHECK.
 * @return Return whether loading was successful
 */
static bool DoLoad(LoadFilter **chain, int mode)
{
	SavegameTypeVersion sl_version;

	if (mode != SL_OLD_LOAD) {
		LoadSavegameFormat(chain, &sl_version);
	}

	if (mode != SL_LOAD_CHECK) {
		/* Old maps were hardcoded to 256x256 and thus did not contain
		 * any mapsize information. Pre-initialize to 256x256 to not to
		 * confuse old games */
		InitializeGame(256, 256, true, true);

		GamelogReset();

		/*
		 * TTD/TTO savegames have no NewGRFs, and TTDP savegames have
		 * them, but the NewGRF list will be made in LoadOldSaveGame,
		 * and it has to be cleared here.
		 *
		 * For OTTD savegames, the situation is more complex.
		 * NewGRFs were introduced between 0.3,4 and 0.3.5, which both
		 * shared savegame version 4. Anything before that 'obviously'
		 * does not have any NewGRFs. Between the introduction and
		 * savegame version 41 (just before 0.5) the NewGRF settings
		 * were not stored in the savegame and they were loaded by
		 * using the settings from the main menu.
		 * So, to recap:
		 * - savegame version  <  4:  do not load any NewGRFs.
		 * - savegame version >= 41:  load NewGRFs from savegame, which is
		 *                            already done at this stage by
		 *                            overwriting the main menu settings.
		 * - other savegame versions: use main menu settings.
		 *
		 * This means that users *can* crash savegame version 4..40
		 * savegames if they set incompatible NewGRFs in the main menu,
		 * but can't crash anymore for savegame version < 4 savegames.
		 *
		 * Note: this is done here because AfterLoadGame is also called
		 * for TTO/TTD/TTDP savegames which have their own NewGRF logic.
		 */
		if (IsOTTDSavegameVersionBefore(&sl_version, 4)) {
			ClearGRFConfigList(&_grfconfig);
		}
	}

	if (mode == SL_OLD_LOAD) {
		if (!LoadOldSaveGame(*chain, &sl_version, &_sl.error)) return false;
	} else {
		/* Load chunks. */
		LoadBuffer reader(*chain, &sl_version);
		SlLoadChunks(&reader, mode == SL_LOAD_CHECK);

		/* Resolve references */
		if (mode != SL_LOAD_CHECK) {
			SlFixPointers(&sl_version);
		}
	}

	if (mode == SL_LOAD_CHECK) {
		/* The only part from AfterLoadGame() we need */
		_load_check_data.grf_compatibility = IsGoodGRFConfigList(_load_check_data.grfconfig);

		_load_check_data.sl_version = sl_version;
	} else {
		GamelogStartAction(GLAT_LOAD);

		/* After loading fix up savegame for any internal changes that
		 * might have occurred since then. */
		AfterLoadGame(&sl_version);

		GamelogStopAction();
	}

	return true;
}

/**
 * Load a game using a (reader) filter in the given mode.
 * @param reader   The filter to read the savegame from.
 * @param mode Load mode. Load can also be a TTD(Patch) game. Use #SL_LOAD, #SL_OLD_LOAD or #SL_LOAD_CHECK.
 * @return Return whether loading was successful
 */
static bool LoadWithFilterMode(LoadFilter *reader, int mode)
{
	LoadFilter *chain = reader;
	bool res;

	SetSignalHandlers();

	try {
		res = DoLoad(&chain, mode);
	} catch (SlException e) {
		/* Distinguish between loading into _load_check_data vs. normal load. */
		if (mode == SL_LOAD_CHECK) {
			_load_check_data.error = e.error;
		} else {
			_sl.error = e.error;

			SlNullPointers();

			/* Skip the "colour" character */
			DEBUG(sl, 0, "%s", GetSaveLoadErrorString() + 3);
		}

		res = false;
	}

	ResetSignalHandlers();

	delete chain;
	return res;
}

/**
 * Load the game using a (reader) filter.
 * @param reader   The filter to read the savegame from.
 * @return Return whether loading was successful
 */
bool LoadWithFilter(LoadFilter *reader)
{
	return LoadWithFilterMode(reader, SL_LOAD);
}

/**
 * Main Load function where the high-level saveload functions are
 * handled. It opens the savegame, selects format and checks versions
 * @param filename The name of the savegame being loaded
 * @param mode Load mode. Load can also be a TTD(Patch) game. Use #SL_LOAD, #SL_OLD_LOAD or #SL_LOAD_CHECK.
 * @param sb The sub directory to load the savegame from
 * @return Return whether loading was successful
 */
bool LoadGame(const char *filename, int mode, Subdirectory sb)
{
	WaitTillSaved();

	/* Load a TTDLX or TTDPatch game */
	FILE *fh;
	if (mode == SL_OLD_LOAD) {
		/* XXX Why are old savegames only searched for in NO_DIRECTORY? */
		fh  = FioFOpenFile(filename, "rb", NO_DIRECTORY);
	} else {
		fh = FioFOpenFile(filename, "rb", sb);

		/* Make it a little easier to load savegames from the console */
		if (fh == NULL) fh = FioFOpenFile(filename, "rb", SAVE_DIR);
		if (fh == NULL) fh = FioFOpenFile(filename, "rb", BASE_DIR);
		if (fh == NULL) fh = FioFOpenFile(filename, "rb", SCENARIO_DIR);

	}

	if (fh == NULL) {
		/* Distinguish between loading into _load_check_data vs. normal load. */
		if (mode == SL_LOAD_CHECK) {
			_load_check_data.error.str = STR_GAME_SAVELOAD_ERROR_FILE_NOT_READABLE;
		} else {
			DEBUG(sl, 0, "Cannot open file '%s'", filename);
			_sl.error.str = STR_GAME_SAVELOAD_ERROR_FILE_NOT_READABLE;
		}

		return false;
	}

	/* LOAD game */
	if (mode != SL_OLD_LOAD) {
		DEBUG(desync, 1, "load: %s", filename);
	}

	if (mode == SL_LOAD_CHECK) {
		/* Clear previous check data */
		_load_check_data.Clear();
		/* Mark SL_LOAD_CHECK as supported for this savegame. */
		_load_check_data.checkable = true;
	}

	return LoadWithFilterMode(new FileReader(fh), mode);
}

/** Do a save when exiting the game (_settings_client.gui.autosave_on_exit) */
void DoExitSave()
{
	SaveGame("exit.sav", AUTOSAVE_DIR);
}

/**
 * Fill the buffer with the default name for a savegame *or* screenshot.
 * @param buf the buffer to write to.
 * @param last the last element in the buffer.
 */
void GenerateDefaultSaveName(char *buf, const char *last)
{
	/* Check if we have a name for this map, which is the name of the first
	 * available company. When there's no company available we'll use
	 * 'Spectator' as "company" name. */
	CompanyID cid = _local_company;
	if (!Company::IsValidID(cid)) {
		const Company *c;
		FOR_ALL_COMPANIES(c) {
			cid = c->index;
			break;
		}
	}

	SetDParam(0, cid);

	/* Insert current date */
	switch (_settings_client.gui.date_format_in_default_names) {
		case 0: SetDParam(1, STR_JUST_DATE_LONG); break;
		case 1: SetDParam(1, STR_JUST_DATE_TINY); break;
		case 2: SetDParam(1, STR_JUST_DATE_ISO); break;
		default: NOT_REACHED();
	}
	SetDParam(2, _date);

	/* Get the correct string (special string for when there's not company) */
	GetString(buf, !Company::IsValidID(cid) ? STR_SAVEGAME_NAME_SPECTATOR : STR_SAVEGAME_NAME_DEFAULT, last);
	SanitizeFilename(buf);
}
