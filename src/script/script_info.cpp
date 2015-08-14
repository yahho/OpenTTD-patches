/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_info.cpp Implementation of ScriptInfo. */

#include "../stdafx.h"
#include "../debug.h"
#include "../string.h"
#include "../settings_type.h"

#include "convert.hpp"

#include "script_info.hpp"
#include "script_scanner.hpp"

ScriptInfo::ScriptInfo() :
	main_script(),
	tar_file(),
	author(),
	name(),
	short_name(),
	description(),
	date(),
	instance_name(),
	version(0),
	url()
{
}

ScriptInfo::~ScriptInfo()
{
	/* Free all allocated strings */
	for (ScriptConfigItemList::iterator it = this->config_list.begin(); it != this->config_list.end(); it++) {
		free((*it).name);
		free((*it).description);
		if (it->labels != NULL) {
			for (LabelMapping::iterator it2 = (*it).labels->Begin(); it2 != (*it).labels->End(); it2++) {
				free(it2->second);
			}
			delete it->labels;
		}
	}
	this->config_list.clear();
}

SQInteger ScriptInfo::AddSetting(HSQUIRRELVM vm)
{
	ScriptConfigItem config;
	memset(&config, 0, sizeof(config));
	config.max_value = 1;
	config.step_size = 1;
	uint items = 0;

	/* Read the table, and find all properties we care about */
	sq_pushnull(vm);
	while (SQ_SUCCEEDED(sq_next(vm, -2))) {
		const char *key;
		if (SQ_FAILED(sq_getstring(vm, -2, &key))) return SQ_ERROR;
		ValidateString(key);

		if (strcmp(key, "name") == 0) {
			const char *sqvalue;
			if (SQ_FAILED(sq_getstring(vm, -1, &sqvalue))) return SQ_ERROR;
			char *name = xstrdup(sqvalue);
			char *s;
			ValidateString(name);

			/* Don't allow '=' and ',' in configure setting names, as we need those
			 *  2 chars to nicely store the settings as a string. */
			while ((s = strchr(name, '=')) != NULL) *s = '_';
			while ((s = strchr(name, ',')) != NULL) *s = '_';
			config.name = name;
			items |= 0x001;
		} else if (strcmp(key, "description") == 0) {
			const char *sqdescription;
			if (SQ_FAILED(sq_getstring(vm, -1, &sqdescription))) return SQ_ERROR;
			config.description = xstrdup(sqdescription);
			ValidateString(config.description);
			items |= 0x002;
		} else if (strcmp(key, "min_value") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.min_value = res;
			items |= 0x004;
		} else if (strcmp(key, "max_value") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.max_value = res;
			items |= 0x008;
		} else if (strcmp(key, "easy_value") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.easy_value = res;
			items |= 0x010;
		} else if (strcmp(key, "medium_value") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.medium_value = res;
			items |= 0x020;
		} else if (strcmp(key, "hard_value") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.hard_value = res;
			items |= 0x040;
		} else if (strcmp(key, "random_deviation") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.random_deviation = res;
			items |= 0x200;
		} else if (strcmp(key, "custom_value") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.custom_value = res;
			items |= 0x080;
		} else if (strcmp(key, "step_size") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.step_size = res;
		} else if (strcmp(key, "flags") == 0) {
			SQInteger res;
			if (SQ_FAILED(sq_getinteger(vm, -1, &res))) return SQ_ERROR;
			config.flags = (ScriptConfigFlags)res;
			items |= 0x100;
		} else {
			char error[1024];
			bstrfmt (error, "unknown setting property '%s'", key);
			sq_throwerror (vm, error);
			return SQ_ERROR;
		}

		sq_pop(vm, 2);
	}
	sq_pop(vm, 1);

	/* Don't allow both random_deviation and SCRIPTCONFIG_RANDOM to
	 * be set for the same config item. */
	if ((items & 0x200) != 0 && (config.flags & SCRIPTCONFIG_RANDOM) != 0) {
		sq_throwerror (vm, "Setting both random_deviation and SCRIPTCONFIG_RANDOM is not allowed");
		return SQ_ERROR;
	}
	/* Reset the bit for random_deviation as it's optional. */
	items &= ~0x200;

	/* Make sure all properties are defined */
	uint mask = (config.flags & SCRIPTCONFIG_BOOLEAN) ? 0x1F3 : 0x1FF;
	if (items != mask) {
		sq_throwerror (vm, "please define all properties of a setting (min/max not allowed for booleans)");
		return SQ_ERROR;
	}

	this->config_list.push_back(config);
	return 0;
}

SQInteger ScriptInfo::AddLabels(HSQUIRRELVM vm)
{
	const char *setting_name;
	if (SQ_FAILED(sq_getstring(vm, -2, &setting_name))) return SQ_ERROR;
	ValidateString(setting_name);

	ScriptConfigItem *config = NULL;
	for (ScriptConfigItemList::iterator it = this->config_list.begin(); it != this->config_list.end(); it++) {
		if (strcmp((*it).name, setting_name) == 0) config = &(*it);
	}

	if (config == NULL) {
		char error[1024];
		bstrfmt (error, "Trying to add labels for non-defined setting '%s'", setting_name);
		sq_throwerror (vm, error);
		return SQ_ERROR;
	}
	if (config->labels != NULL) return SQ_ERROR;

	config->labels = new LabelMapping;

	/* Read the table and find all labels */
	sq_pushnull(vm);
	while (SQ_SUCCEEDED(sq_next(vm, -2))) {
		const char *key_string;
		const char *label;
		if (SQ_FAILED(sq_getstring(vm, -2, &key_string))) return SQ_ERROR;
		if (SQ_FAILED(sq_getstring(vm, -1, &label))) return SQ_ERROR;
		/* Because squirrel doesn't support identifiers starting with a digit,
		 * we skip the first character. */
		int key = atoi(key_string + 1);
		ValidateString(label);

		/* !Contains() prevents xstrdup from leaking. */
		if (!config->labels->Contains(key)) config->labels->Insert(key, xstrdup(label));

		sq_pop(vm, 2);
	}
	sq_pop(vm, 1);

	/* Check labels for completeness */
	config->complete_labels = true;
	for (int value = config->min_value; value <= config->max_value; value++) {
		if (!config->labels->Contains(value)) {
			config->complete_labels = false;
			break;
		}
	}

	return 0;
}

const ScriptConfigItemList *ScriptInfo::GetConfigList() const
{
	return &this->config_list;
}

const ScriptConfigItem *ScriptInfo::GetConfigItem(const char *name) const
{
	for (ScriptConfigItemList::const_iterator it = this->config_list.begin(); it != this->config_list.end(); it++) {
		if (strcmp((*it).name, name) == 0) return &(*it);
	}
	return NULL;
}

int ScriptInfo::GetSettingDefaultValue(const char *name) const
{
	for (ScriptConfigItemList::const_iterator it = this->config_list.begin(); it != this->config_list.end(); it++) {
		if (strcmp((*it).name, name) != 0) continue;
		/* The default value depends on the difficulty level */
		switch (GetGameSettings().script.settings_profile) {
			case SP_EASY:   return (*it).easy_value;
			case SP_MEDIUM: return (*it).medium_value;
			case SP_HARD:   return (*it).hard_value;
			case SP_CUSTOM: return (*it).custom_value;
			default: NOT_REACHED();
		}
	}

	/* There is no such setting */
	return -1;
}

SQInteger ScriptInfo::construct (ScriptScanner *scanner)
{
	/* Ensure the mandatory functions exist */
	static const char * const required_functions[] = {
		/* Keep this list in sync with required_fields below. */
		"GetAuthor",
		"GetName",
		"GetShortName",
		"GetDescription",
		"GetDate",
		"GetVersion",
		"CreateInstance",
	};
	for (size_t i = 0; i < lengthof(required_functions); i++) {
		if (!scanner->check_method (required_functions[i])) return SQ_ERROR;
	}

	/* Get location information of the scanner */
	this->main_script.reset (xstrdup (scanner->GetMainScript()));
	const char *tar_name = scanner->GetTarFile();
	if (tar_name != NULL) this->tar_file.reset (xstrdup (tar_name));

	/* Cache the data the info file gives us. */
	static ttd_unique_free_ptr<char> ScriptInfo::*const required_fields[] = {
		/* Keep this list in sync with required_functions above. */
		&ScriptInfo::author,
		&ScriptInfo::name,
		&ScriptInfo::short_name,
		&ScriptInfo::description,
		&ScriptInfo::date,
	};
	for (size_t i = 0; i < lengthof(required_fields); i++) {
		char *s = scanner->call_string_method (required_functions[i], MAX_GET_OPS);
		if (s == NULL) return SQ_ERROR;
		(this->*required_fields[i]).reset (s);
	}
	if (!scanner->call_integer_method ("GetVersion", MAX_GET_OPS, &this->version)) return SQ_ERROR;
	{
		char *s = scanner->call_string_method ("CreateInstance", MAX_CREATEINSTANCE_OPS);
		if (s == NULL) return SQ_ERROR;
		this->instance_name.reset (s);
	}

	/* The GetURL function is optional. */
	if (scanner->method_exists ("GetURL")) {
		char *s = scanner->call_string_method ("GetURL", MAX_GET_OPS);
		if (s == NULL) return SQ_ERROR;
		this->url.reset (s);
	}

	/* Check if we have settings */
	if (scanner->method_exists ("GetSettings")) {
		if (!scanner->CallMethod (scanner->instance, "GetSettings", MAX_GET_SETTING_OPS)) return SQ_ERROR;
	}

	return 0;
}

SQInteger ScriptVersionedInfo::construct (ScriptScanner *scanner,
	size_t napi, const char *const *api, const char *def)
{
	SQInteger res = this->ScriptInfo::construct (scanner);
	if (res != 0) return res;

	if (!scanner->method_exists ("MinVersionToLoad")) {
		this->min_loadable_version = this->GetVersion();
	} else if (!scanner->call_integer_method ("MinVersionToLoad",
			MAX_GET_OPS, &this->min_loadable_version)) {
		return SQ_ERROR;
	}

	/* Try to get the API version the script is written for. */
	if ((def == NULL) && !scanner->check_method ("GetAPIVersion")) {
		return SQ_ERROR;
	} else if ((def != NULL) && !scanner->method_exists ("GetAPIVersion")) {
		this->api_version = def;
	} else {
		const char *ver = scanner->call_string_method_from_set ("GetAPIVersion", napi, api, MAX_GET_OPS);
		if (ver == NULL) {
			DEBUG(script, 1, "Loading info.nut from (%s.%d): GetAPIVersion returned invalid version", this->GetName(), this->GetVersion());
			return SQ_ERROR;
		}
		this->api_version = ver;
	}

	return 0;
}

SQInteger ScriptLibraryInfo::construct (ScriptScanner *scanner)
{
	SQInteger res = this->ScriptInfo::construct (scanner);
	if (res != 0) return res;

	/* Cache the category */
	if (!scanner->check_method ("GetCategory")) return SQ_ERROR;

	char *cat = scanner->call_string_method ("GetCategory", MAX_GET_OPS);
	if (cat == NULL) return SQ_ERROR;
	this->category.reset (cat);

	return 0;
}
