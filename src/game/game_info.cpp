/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file game_info.cpp Implementation of GameInfo */

#include "../stdafx.h"

#include "../script/convert.hpp"
#include "../script/script_scanner.hpp"
#include "game_info.hpp"
#include "../debug.h"

static const char *const game_api_versions[] =
	{ "1.2", "1.3", "1.4", "1.5", "1.6" };

/* static */ void GameInfo::RegisterAPI(Squirrel *engine)
{
	/* Create the GSInfo class, and add the RegisterGS function */
	engine->AddClassBegin ("GSInfo");
	SQConvert::AddConstructor <void (GameInfo::*)(), 1> (engine, "x");
	SQConvert::DefSQAdvancedMethod (engine, "GSInfo", &GameInfo::AddSetting, "AddSetting");
	SQConvert::DefSQAdvancedMethod (engine, "GSInfo", &GameInfo::AddLabels,  "AddLabels");
	engine->AddConst ("CONFIG_NONE",      SCRIPTCONFIG_NONE);
	engine->AddConst ("CONFIG_RANDOM",    SCRIPTCONFIG_RANDOM);
	engine->AddConst ("CONFIG_BOOLEAN",   SCRIPTCONFIG_BOOLEAN);
	engine->AddConst ("CONFIG_INGAME",    SCRIPTCONFIG_INGAME);
	engine->AddConst ("CONFIG_DEVELOPER", SCRIPTCONFIG_DEVELOPER);
	engine->AddClassEnd();

	engine->AddMethod("RegisterGS", &GameInfo::Constructor, 2, "tx");
}

/* static */ SQInteger GameInfo::Constructor(HSQUIRRELVM vm)
{
	/* Get the GameInfo */
	SQUserPointer instance = NULL;
	if (SQ_FAILED(sq_getinstanceup(vm, 2, &instance, 0)) || instance == NULL) return sq_throwerror(vm, "Pass an instance of a child class of GameInfo to RegisterGame");
	GameInfo *info = (GameInfo *)instance;

	ScriptScanner *scanner = ScriptScanner::Get (vm);

	SQInteger res = scanner->construct (info);
	if (res != 0) return res;

	if (scanner->method_exists ("MinVersionToLoad")) {
		if (!scanner->call_integer_method ("MinVersionToLoad", MAX_GET_OPS, &info->min_loadable_version)) return SQ_ERROR;
	} else {
		info->min_loadable_version = info->GetVersion();
	}
	/* When there is an IsSelectable function, call it. */
	bool is_developer_only;
	if (scanner->method_exists ("IsDeveloperOnly")) {
		if (!scanner->call_bool_method ("IsDeveloperOnly", MAX_GET_OPS, &is_developer_only)) return SQ_ERROR;
	} else {
		is_developer_only = false;
	}
	/* Try to get the API version the AI is written for. */
	if (!scanner->check_method ("GetAPIVersion")) return SQ_ERROR;
	const char *ver = scanner->call_string_method_from_set ("GetAPIVersion", game_api_versions, MAX_GET_OPS);
	if (ver == NULL) {
		DEBUG(script, 1, "Loading info.nut from (%s.%d): GetAPIVersion returned invalid version", info->GetName(), info->GetVersion());
		return SQ_ERROR;
	}
	info->api_version = ver;

	/* Remove the link to the real instance, else it might get deleted by RegisterGame() */
	sq_setinstanceup(vm, 2, NULL);
	/* Register the Game to the base system */
	scanner->RegisterScript (info, info->GetName(), is_developer_only);
	return 0;
}

GameInfo::GameInfo() :
	min_loadable_version(0),
	api_version(NULL)
{
}

bool GameInfo::CanLoadFromVersion(int version) const
{
	if (version == -1) return true;
	return version >= this->min_loadable_version && version <= this->GetVersion();
}


/* static */ void GameLibrary::RegisterAPI(Squirrel *engine)
{
	/* Create the GameLibrary class, and add the RegisterLibrary function */
	engine->AddClassBegin("GSLibrary");
	engine->AddClassEnd();
	engine->AddMethod("RegisterLibrary", &GameLibrary::Constructor, 2, "tx");
}

/* static */ SQInteger GameLibrary::Constructor(HSQUIRRELVM vm)
{
	/* Create a new library */
	GameLibrary *library = new GameLibrary();

	ScriptScanner *scanner = ScriptScanner::Get (vm);

	SQInteger res = scanner->construct (library);
	if (res != 0) {
		delete library;
		return res;
	}

	/* Cache the category */
	if (!scanner->check_method ("GetCategory")) {
		delete library;
		return SQ_ERROR;
	}
	char *cat = scanner->call_string_method ("GetCategory", MAX_GET_OPS);
	if (cat == NULL) {
		delete library;
		return SQ_ERROR;
	}
	library->category.reset (cat);

	/* Register the Library to the base system */
	char name [1024];
	bstrfmt (name, "%s.%s", library->GetCategory(), library->GetInstanceName());
	scanner->RegisterScript (library, name);

	return 0;
}
