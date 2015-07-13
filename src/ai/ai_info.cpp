/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ai_info.cpp Implementation of AIInfo and AILibrary */

#include "../stdafx.h"

#include "../script/convert.hpp"
#include "../script/script_scanner.hpp"
#include "ai_info.hpp"
#include "../string.h"
#include "../debug.h"
#include "../rev.h"

static const char *const ai_api_versions[] =
	{ "0.7", "1.0", "1.1", "1.2", "1.3", "1.4", "1.5", "1.6" };

/* static */ void AIInfo::RegisterAPI(Squirrel *engine)
{
	/* Create the AIInfo class, and add the RegisterAI function */
	engine->AddClassBegin ("AIInfo");
	SQConvert::AddConstructor <void (AIInfo::*)(), 1> (engine, "x");
	SQConvert::DefSQAdvancedMethod (engine, "AIInfo", &AIInfo::AddSetting, "AddSetting");
	SQConvert::DefSQAdvancedMethod (engine, "AIInfo", &AIInfo::AddLabels,  "AddLabels");
	engine->AddConst ("CONFIG_NONE",      SCRIPTCONFIG_NONE);
	engine->AddConst ("CONFIG_RANDOM",    SCRIPTCONFIG_RANDOM);
	engine->AddConst ("CONFIG_BOOLEAN",   SCRIPTCONFIG_BOOLEAN);
	engine->AddConst ("CONFIG_INGAME",    SCRIPTCONFIG_INGAME);
	engine->AddConst ("CONFIG_DEVELOPER", SCRIPTCONFIG_DEVELOPER);

	/* Pre 1.2 had an AI prefix */
	engine->AddConst ("AICONFIG_NONE",    SCRIPTCONFIG_NONE);
	engine->AddConst ("AICONFIG_RANDOM",  SCRIPTCONFIG_RANDOM);
	engine->AddConst ("AICONFIG_BOOLEAN", SCRIPTCONFIG_BOOLEAN);
	engine->AddConst ("AICONFIG_INGAME",  SCRIPTCONFIG_INGAME);

	engine->AddClassEnd();

	engine->AddMethod("RegisterAI", &AIInfo::Constructor, 2, "tx");
}

/* static */ SQInteger AIInfo::Constructor(HSQUIRRELVM vm)
{
	/* Get the AIInfo */
	SQUserPointer instance = NULL;
	if (SQ_FAILED(sq_getinstanceup(vm, 2, &instance, 0)) || instance == NULL) return sq_throwerror(vm, "Pass an instance of a child class of AIInfo to RegisterAI");
	AIInfo *info = (AIInfo *)instance;

	ScriptScanner *scanner = ScriptScanner::Get (vm);

	SQInteger res = scanner->construct (info);
	if (res != 0) return res;

	ScriptConfigItem config = _start_date_config;
	config.name = xstrdup(config.name);
	config.description = xstrdup(config.description);
	info->config_list.push_front(config);

	if (scanner->method_exists ("MinVersionToLoad")) {
		if (!scanner->CallIntegerMethod (scanner->instance, "MinVersionToLoad", &info->min_loadable_version, MAX_GET_OPS)) return SQ_ERROR;
	} else {
		info->min_loadable_version = info->GetVersion();
	}
	/* When there is an UseAsRandomAI function, call it. */
	if (scanner->method_exists ("UseAsRandomAI")) {
		if (!scanner->CallBoolMethod (scanner->instance, "UseAsRandomAI", &info->use_as_random, MAX_GET_OPS)) return SQ_ERROR;
	} else {
		info->use_as_random = true;
	}
	/* Try to get the API version the AI is written for. */
	if (scanner->method_exists ("GetAPIVersion")) {
		const char *ver = scanner->CallStringMethodFromSet (scanner->instance, "GetAPIVersion", ai_api_versions, MAX_GET_OPS);
		if (ver == NULL) {
			DEBUG(script, 1, "Loading info.nut from (%s.%d): GetAPIVersion returned invalid version", info->GetName(), info->GetVersion());
			return SQ_ERROR;
		}
		info->api_version = ver;
	} else {
		info->api_version = ai_api_versions[0];
	}

	/* Remove the link to the real instance, else it might get deleted by RegisterAI() */
	sq_setinstanceup(vm, 2, NULL);
	/* Register the AI to the base system */
	scanner->RegisterScript (info, info->GetName());
	return 0;
}

AIInfo::AIInfo() :
	min_loadable_version(0),
	use_as_random(false),
	api_version(NULL)
{
}

AIInfo::AIInfo (bool ignored)
	: min_loadable_version(0), use_as_random(false),
	  api_version(*lastof(ai_api_versions))
{
	this->main_script.reset (xstrdup ("%_dummy"));
	this->author.reset (xstrdup ("OpenTTD Developers Team"));
	this->name.reset (xstrdup ("DummyAI"));
	this->short_name.reset (xstrdup ("DUMM"));
	this->description.reset (xstrdup ("A Dummy AI that is loaded when your ai/ dir is empty"));
	this->date.reset (xstrdup ("2008-07-26"));
	this->instance_name.reset (xstrdup ("DummyAI"));
	this->version = 1;
}

bool AIInfo::CanLoadFromVersion(int version) const
{
	if (version == -1) return true;
	return version >= this->min_loadable_version && version <= this->GetVersion();
}


/* static */ void AILibrary::RegisterAPI(Squirrel *engine)
{
	/* Create the AILibrary class, and add the RegisterLibrary function */
	engine->AddClassBegin("AILibrary");
	engine->AddClassEnd();
	engine->AddMethod("RegisterLibrary", &AILibrary::Constructor, 2, "tx");
}

/* static */ SQInteger AILibrary::Constructor(HSQUIRRELVM vm)
{
	/* Create a new library */
	AILibrary *library = new AILibrary();

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
	char *cat = scanner->CallStringMethodStrdup (scanner->instance, "GetCategory", MAX_GET_OPS);
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
