/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file game_core.cpp Implementation of Game. */

#include "../stdafx.h"
#include "../core/backup_type.hpp"
#include "../company_base.h"
#include "../company_func.h"
#include "../network/network.h"
#include "../window_func.h"
#include "../script/script_scanner.hpp"
#include "game.hpp"
#include "game_config.hpp"
#include "game_instance.hpp"
#include "game_info.hpp"

/* static */ uint Game::frame_counter = 0;
/* static */ const GameInfo *Game::info = NULL;
/* static */ GameInstance *Game::instance = NULL;


struct GameScriptData {
	static const Subdirectory script_dir  = GAME_DIR;
	static const Subdirectory library_dir = GAME_LIBRARY_DIR;
	static const char script_list_desc[];
	static const char library_list_desc[];
	static const char scanner_desc[];
};

const char GameScriptData::script_list_desc[]  = "Game Scripts";
const char GameScriptData::library_list_desc[] = "GS Libraries";
const char GameScriptData::scanner_desc[]      = "GSScanner";

typedef ScriptInfoLists<GameScriptData> GameInfoLists;

static GameInfoLists *lists;


static SQInteger register_gs (HSQUIRRELVM vm)
{
	/* Get the GameInfo */
	SQUserPointer instance = NULL;
	if (SQ_FAILED(sq_getinstanceup (vm, 2, &instance, 0)) || instance == NULL) return sq_throwerror(vm, "Pass an instance of a child class of GameInfo to RegisterGame");
	GameInfo *info = (GameInfo *)instance;

	GameInfoLists::InfoScanner *scanner = GameInfoLists::InfoScanner::Get (vm);

	SQInteger res = scanner->construct (info);
	if (res != 0) return res;

	/* When there is an IsSelectable function, call it. */
	bool is_developer_only;
	if (scanner->method_exists ("IsDeveloperOnly")) {
		if (!scanner->call_bool_method ("IsDeveloperOnly", MAX_GET_OPS, &is_developer_only)) return SQ_ERROR;
	} else {
		is_developer_only = false;
	}

	/* Remove the link to the real instance, else it might get deleted by RegisterGame() */
	sq_setinstanceup (vm, 2, NULL);
	/* Register the Game to the base system */
	scanner->RegisterScript (info, info->GetName(), is_developer_only);
	return 0;
}

template<>
void GameInfoLists::InfoScanner::RegisterAPI (void)
{
	/* Create the GSInfo class, and add the RegisterGS function */
	this->AddClassBegin ("GSInfo");
	SQConvert::AddConstructor <void (GameInfo::*)(), 1> (this, "x");
	SQConvert::DefSQAdvancedMethod (this, "GSInfo", &GameInfo::AddSetting, "AddSetting");
	SQConvert::DefSQAdvancedMethod (this, "GSInfo", &GameInfo::AddLabels,  "AddLabels");
	this->AddConst ("CONFIG_NONE",      SCRIPTCONFIG_NONE);
	this->AddConst ("CONFIG_RANDOM",    SCRIPTCONFIG_RANDOM);
	this->AddConst ("CONFIG_BOOLEAN",   SCRIPTCONFIG_BOOLEAN);
	this->AddConst ("CONFIG_INGAME",    SCRIPTCONFIG_INGAME);
	this->AddConst ("CONFIG_DEVELOPER", SCRIPTCONFIG_DEVELOPER);
	this->AddClassEnd();

	this->AddMethod ("RegisterGS", &register_gs, 2, "tx");
}


static SQInteger register_library (HSQUIRRELVM vm)
{
	/* Create a new library */
	GameLibrary *library = new GameLibrary();

	GameInfoLists::LibraryScanner *scanner = GameInfoLists::LibraryScanner::Get (vm);

	SQInteger res = scanner->construct (library);
	if (res != 0) {
		delete library;
		return res;
	}

	/* Register the Library to the base system */
	char name [1024];
	bstrfmt (name, "%s.%s", library->GetCategory(), library->GetInstanceName());
	scanner->RegisterScript (library, name);

	return 0;
}

template<>
void GameInfoLists::LibraryScanner::RegisterAPI (void)
{
	/* Create the GameLibrary class, and add the RegisterLibrary function */
	this->AddClassBegin ("GSLibrary");
	this->AddClassEnd();
	this->AddMethod ("RegisterLibrary", &register_library, 2, "tx");
}


/* static */ void Game::GameLoop()
{
	if (_networking && !_network_server) return;
	if (Game::instance == NULL) return;

	Game::frame_counter++;

	Backup<CompanyByte> cur_company(_current_company, FILE_LINE);
	cur_company.Change(OWNER_DEITY);
	Game::instance->GameLoop();
	cur_company.Restore();

	/* Occasionally collect garbage */
	if ((Game::frame_counter & 255) == 0) {
		Game::instance->CollectGarbage();
	}
}

/* static */ void Game::Initialize()
{
	if (Game::instance != NULL) Game::Uninitialize(true);

	Game::frame_counter = 0;

	if (lists == NULL) {
		TarScanner::DoScan(TarScanner::GAME);

		lists = new GameInfoLists();
		lists->Scan();
	}
}

/* static */ void Game::StartNew()
{
	if (Game::instance != NULL) return;

	/* Clients shouldn't start GameScripts */
	if (_networking && !_network_server) return;

	GameConfig *config = GameConfig::GetConfig(GameConfig::SSS_FORCE_GAME);
	const GameInfo *info = config->GetInfo();
	if (info == NULL) return;

	config->AnchorUnchangeableSettings();

	Backup<CompanyByte> cur_company(_current_company, FILE_LINE);
	cur_company.Change(OWNER_DEITY);

	Game::info = info;
	Game::instance = new GameInstance();
	Game::instance->Initialize(info);

	cur_company.Restore();

	InvalidateWindowData(WC_AI_DEBUG, 0, -1);
}

/* static */ void Game::Uninitialize(bool keepConfig)
{
	Backup<CompanyByte> cur_company(_current_company, FILE_LINE);

	delete Game::instance;
	Game::instance = NULL;
	Game::info = NULL;

	cur_company.Restore();

	if (keepConfig) {
		Rescan();
	} else {
		delete lists;
		lists = NULL;

		if (_settings_game.game_config != NULL) {
			delete _settings_game.game_config;
			_settings_game.game_config = NULL;
		}
		if (_settings_newgame.game_config != NULL) {
			delete _settings_newgame.game_config;
			_settings_newgame.game_config = NULL;
		}
	}
}

/* static */ void Game::Pause()
{
	if (Game::instance != NULL) Game::instance->Pause();
}

/* static */ void Game::Unpause()
{
	if (Game::instance != NULL) Game::instance->Unpause();
}

/* static */ bool Game::IsPaused()
{
	return Game::instance != NULL? Game::instance->IsPaused() : false;
}

/* static */ void Game::NewEvent(ScriptEvent *event)
{
	/* AddRef() and Release() need to be called at least once, so do it here */
	event->AddRef();

	/* Clients should ignore events */
	if (_networking && !_network_server) {
		event->Release();
		return;
	}

	/* Check if Game instance is alive */
	if (Game::instance == NULL) {
		event->Release();
		return;
	}

	/* Queue the event */
	Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
	Game::instance->InsertEvent(event);
	cur_company.Restore();

	event->Release();
}

/* static */ void Game::ResetConfig()
{
	/* Check for both newgame as current game if we can reload the GameInfo inside
	 *  the GameConfig. If not, remove the Game from the list. */
	if (_settings_game.game_config != NULL && _settings_game.game_config->HasScript()) {
		if (!_settings_game.game_config->ResetInfo(true)) {
			DEBUG(script, 0, "After a reload, the GameScript by the name '%s' was no longer found, and removed from the list.", _settings_game.game_config->GetName());
			_settings_game.game_config->Change(NULL);
			if (Game::instance != NULL) {
				delete Game::instance;
				Game::instance = NULL;
				Game::info = NULL;
			}
		} else if (Game::instance != NULL) {
			Game::info = _settings_game.game_config->GetInfo();
		}
	}
	if (_settings_newgame.game_config != NULL && _settings_newgame.game_config->HasScript()) {
		if (!_settings_newgame.game_config->ResetInfo(false)) {
			DEBUG(script, 0, "After a reload, the GameScript by the name '%s' was no longer found, and removed from the list.", _settings_newgame.game_config->GetName());
			_settings_newgame.game_config->Change(NULL);
		}
	}
}

/* static */ void Game::Rescan()
{
	TarScanner::DoScan(TarScanner::GAME);
	lists->Scan();

	ResetConfig();

	InvalidateWindowData(WC_AI_LIST, 0, 1);
	SetWindowClassesDirty(WC_AI_DEBUG);
	InvalidateWindowClassesData(WC_AI_SETTINGS);
}


/* static */ void Game::Save(SaveDumper *dumper)
{
	if (Game::instance != NULL && (!_networking || _network_server)) {
		Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
		Game::instance->Save(dumper);
		cur_company.Restore();
	} else {
		GameInstance::SaveEmpty(dumper);
	}
}

/* static */ void Game::Load(LoadBuffer *reader, int version)
{
	if (Game::instance != NULL && (!_networking || _network_server)) {
		Backup<CompanyByte> cur_company(_current_company, OWNER_DEITY, FILE_LINE);
		Game::instance->Load(reader, version);
		cur_company.Restore();
	} else {
		/* Read, but ignore, the load data */
		GameInstance::LoadEmpty(reader);
	}
}

/* static */ void Game::GetConsoleList (stringb *buf, bool newest_only)
{
	lists->scripts.GetConsoleList (buf, newest_only);
}

/* static */ void Game::GetConsoleLibraryList (stringb *buf)
{
	lists->libraries.GetConsoleList (buf, true);
}

/* static */ const ScriptInfoList::List *Game::GetUniqueInfoList()
{
	return lists->scripts.GetUniqueInfoList();
}

/* static */ GameInfo *Game::FindInfo(const char *name, int version, bool force_exact_match)
{
	ScriptInfo *i = lists->scripts.FindInfo (name, version, force_exact_match);
	if (i == NULL) return NULL;
	assert (dynamic_cast<GameInfo *>(i) != NULL);
	return static_cast<GameInfo *>(i);
}

/* static */ GameLibrary *Game::FindLibrary(const char *library, int version)
{
	/* Internally we store libraries as 'library.version' */
	char library_name[1024];
	bstrfmt (library_name, "%s.%d", library, version);
	strtolower(library_name);

	/* Check if the library + version exists */
	ScriptInfoList::iterator iter = lists->libraries.full_list.find (library_name);
	if (iter == lists->libraries.full_list.end()) return NULL;

	return static_cast<GameLibrary *>((*iter).second);
}

#if defined(ENABLE_NETWORK)

/**
 * Check whether we have an Game (library) with the exact characteristics as ci.
 * @param ci the characteristics to search on (shortname and md5sum)
 * @param md5sum whether to check the MD5 checksum
 * @return true iff we have an Game (library) matching.
 */
/* static */ bool Game::HasGame(const ContentInfo *ci, bool md5sum)
{
	return lists->scripts.HasScript (ci, md5sum);
}

/* static */ bool Game::HasGameLibrary(const ContentInfo *ci, bool md5sum)
{
	return lists->libraries.HasScript (ci, md5sum);
}

/**
 * Find a script of a #ContentInfo.
 * @param ci The information to compare to.
 * @return A filename of a file of the content, else \c NULL.
 */
const char *Game::FindInfoMainScript (const ContentInfo *ci)
{
	return lists->scripts.FindMainScript (ci, true);
}

/**
 * Find a script of a #ContentInfo.
 * @param ci The information to compare to.
 * @return A filename of a file of the content, else \c NULL.
 */
const char *Game::FindLibraryMainScript (const ContentInfo *ci)
{
	return lists->libraries.FindMainScript (ci, true);
}

#endif /* defined(ENABLE_NETWORK) */
