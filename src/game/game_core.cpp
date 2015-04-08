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


struct GameInfoList : ScriptInfoListT<GameInfoList> {
	static const Subdirectory subdir = GAME_DIR;
	static const char desc[];
};

const char GameInfoList::desc[] = "Game Scripts";

static GameInfoList *scripts;


struct GameLibraryList : ScriptInfoListT<GameLibraryList> {
	static const Subdirectory subdir = GAME_LIBRARY_DIR;
	static const char desc[];
};

const char GameLibraryList::desc[] = "GS Libraries";

static GameLibraryList *libraries;


struct GameScannerDesc {
	static const char desc[];
};

const char GameScannerDesc::desc[] = "GSScanner";


struct GameInfoScanner : ScriptScannerT<GameInfoScanner>, GameScannerDesc {
	typedef GameInfo InfoType;
	static const Subdirectory subdir = GAME_DIR;
	static const bool is_library = false;
};


struct GameLibraryScanner : ScriptScannerT<GameLibraryScanner>, GameScannerDesc {
	typedef GameLibrary InfoType;
	static const Subdirectory subdir = GAME_LIBRARY_DIR;
	static const bool is_library = true;
};


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

	if (scripts == NULL) {
		TarScanner::DoScan(TarScanner::GAME);

		scripts = new GameInfoList();
		GameInfoScanner::Scan (scripts);

		libraries = new GameLibraryList();
		GameLibraryScanner::Scan (libraries);
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
		delete scripts;
		scripts = NULL;
		delete libraries;
		libraries = NULL;

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
	GameInfoScanner::Scan (scripts);
	GameLibraryScanner::Scan (libraries);

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
	scripts->GetConsoleList (buf, newest_only);
}

/* static */ void Game::GetConsoleLibraryList (stringb *buf)
{
	libraries->GetConsoleList (buf, true);
}

/* static */ const ScriptInfoList::List *Game::GetInfoList()
{
	return scripts->GetInfoList();
}

/* static */ const ScriptInfoList::List *Game::GetUniqueInfoList()
{
	return scripts->GetUniqueInfoList();
}

/* static */ GameInfo *Game::FindInfo(const char *name, int version, bool force_exact_match)
{
	if (scripts->full_list.size() == 0) return NULL;
	if (name == NULL) return NULL;

	sstring<1024> game_name;
	game_name.copy (name);
	game_name.tolower();

	if (version == -1) {
		/* We want to load the latest version of this Game script; so find it */
		ScriptInfoList::iterator iter = scripts->single_list.find (game_name.c_str());
		if (iter != scripts->single_list.end()) return static_cast<GameInfo *>(iter->second);

		/* If we didn't find a match Game script, maybe the user included a version */
		const char *e = strrchr (game_name.c_str(), '.');
		if (e == NULL) return NULL;
		version = atoi (e + 1);
		game_name.truncate (e - game_name.c_str());
		/* FALL THROUGH, like we were calling this function with a version. */
	}

	if (force_exact_match) {
		/* Try to find a direct 'name.version' match */
		size_t length = game_name.length();
		game_name.append_fmt (".%d", version);
		ScriptInfoList::iterator iter = scripts->full_list.find (game_name.c_str());
		if (iter != scripts->full_list.end()) return static_cast<GameInfo *>(iter->second);
		game_name.truncate (length);
	}

	GameInfo *info = NULL;
	int max_version = -1;

	/* See if there is a compatible Game script which goes by that name,
	 * with the highest version which allows loading the requested version */
	ScriptInfoList::iterator it = scripts->full_list.begin();
	for (; it != scripts->full_list.end(); it++) {
		GameInfo *i = static_cast<GameInfo *>(it->second);
		if (strcasecmp (game_name.c_str(), i->GetName()) == 0 && i->CanLoadFromVersion(version) && (max_version == -1 || i->GetVersion() > max_version)) {
			max_version = i->GetVersion();
			info = i;
		}
	}

	return info;
}

/* static */ GameLibrary *Game::FindLibrary(const char *library, int version)
{
	/* Internally we store libraries as 'library.version' */
	char library_name[1024];
	bstrfmt (library_name, "%s.%d", library, version);
	strtolower(library_name);

	/* Check if the library + version exists */
	ScriptInfoList::iterator iter = libraries->full_list.find(library_name);
	if (iter == libraries->full_list.end()) return NULL;

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
	return scripts->HasScript(ci, md5sum);
}

/* static */ bool Game::HasGameLibrary(const ContentInfo *ci, bool md5sum)
{
	return libraries->HasScript(ci, md5sum);
}

/**
 * Find a script of a #ContentInfo.
 * @param ci The information to compare to.
 * @return A filename of a file of the content, else \c NULL.
 */
const char *Game::FindInfoMainScript (const ContentInfo *ci)
{
	return scripts->FindMainScript (ci, true);
}

/**
 * Find a script of a #ContentInfo.
 * @param ci The information to compare to.
 * @return A filename of a file of the content, else \c NULL.
 */
const char *Game::FindLibraryMainScript (const ContentInfo *ci)
{
	return libraries->FindMainScript (ci, true);
}

#endif /* defined(ENABLE_NETWORK) */
