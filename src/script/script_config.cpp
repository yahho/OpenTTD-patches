/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_config.cpp Implementation of ScriptConfig. */

#include "../stdafx.h"
#include "../settings_type.h"
#include "../core/random_func.hpp"
#include "script_info.hpp"
#include "../textfile.h"

void ScriptConfig::Change(const char *name, int version, bool force_exact_match, bool is_random)
{
	if (name == NULL) {
		this->name.reset();
		this->info = NULL;
	} else {
		this->name.reset (xstrdup (name));
		this->info = this->FindInfo (name, version, force_exact_match);
	}

	this->version = (info == NULL) ? -1 : info->GetVersion();
	this->is_random = is_random;
	if (this->config_list != NULL) delete this->config_list;
	this->config_list = (info == NULL) ? NULL : new ScriptConfigItemList();
	if (this->config_list != NULL) this->PushExtraConfigList();

	this->ClearConfigList();

	if (_game_mode == GM_NORMAL && this->info != NULL) {
		/* If we're in an existing game and the Script is changed, set all settings
		 *  for the Script that have the random flag to a random value. */
		for (ScriptConfigItemList::const_iterator it = this->info->GetConfigList()->begin(); it != this->info->GetConfigList()->end(); it++) {
			if ((*it).flags & SCRIPTCONFIG_RANDOM) {
				this->SetSetting((*it).name, InteractiveRandomRange((*it).max_value - (*it).min_value) + (*it).min_value);
			}
		}
		this->AddRandomDeviation();
	}
}

ScriptConfig::ScriptConfig(const ScriptConfig *config)
{
	if (config->name) {
		this->name.reset (xstrdup (config->name.get()));
	} else {
		this->name.reset();
	}

	this->info = config->info;
	this->version = config->version;
	this->config_list = NULL;
	this->is_random = config->is_random;

	for (SettingValueList::const_iterator it = config->settings.begin(); it != config->settings.end(); it++) {
		this->settings[xstrdup((*it).first)] = (*it).second;
	}
	this->AddRandomDeviation();
}

ScriptConfig::~ScriptConfig()
{
	this->ResetSettings();
	if (this->config_list != NULL) delete this->config_list;
}

const ScriptConfigItemList *ScriptConfig::GetConfigList()
{
	if (this->info != NULL) return this->info->GetConfigList();
	if (this->config_list == NULL) {
		this->config_list = new ScriptConfigItemList();
		this->PushExtraConfigList();
	}
	return this->config_list;
}

void ScriptConfig::ClearConfigList()
{
	for (SettingValueList::iterator it = this->settings.begin(); it != this->settings.end(); it++) {
		free((*it).first);
	}
	this->settings.clear();
}

void ScriptConfig::AnchorUnchangeableSettings()
{
	for (ScriptConfigItemList::const_iterator it = this->GetConfigList()->begin(); it != this->GetConfigList()->end(); it++) {
		if (((*it).flags & SCRIPTCONFIG_INGAME) == 0) {
			this->SetSetting((*it).name, this->GetSetting((*it).name));
		}
	}
}

int ScriptConfig::GetSetting(const char *name) const
{
	SettingValueList::const_iterator it = this->settings.find(name);
	if (it == this->settings.end()) return this->info->GetSettingDefaultValue(name);
	return (*it).second;
}

void ScriptConfig::SetSetting(const char *name, int value)
{
	/* You can only set Script specific settings if an Script is selected. */
	if (this->info == NULL) return;

	const ScriptConfigItem *config_item = this->info->GetConfigItem(name);
	if (config_item == NULL) return;

	value = Clamp(value, config_item->min_value, config_item->max_value);

	SettingValueList::iterator it = this->settings.find(name);
	if (it != this->settings.end()) {
		(*it).second = value;
	} else {
		this->settings[xstrdup(name)] = value;
	}
}

void ScriptConfig::ResetSettings()
{
	for (SettingValueList::iterator it = this->settings.begin(); it != this->settings.end(); it++) {
		free((*it).first);
	}
	this->settings.clear();
}

void ScriptConfig::AddRandomDeviation()
{
	for (ScriptConfigItemList::const_iterator it = this->GetConfigList()->begin(); it != this->GetConfigList()->end(); it++) {
		if ((*it).random_deviation != 0) {
			this->SetSetting((*it).name, InteractiveRandomRange((*it).random_deviation * 2) - (*it).random_deviation + this->GetSetting((*it).name));
		}
	}
}

void ScriptConfig::StringToSettings(const char *value)
{
	char *value_copy = xstrdup(value);
	char *s = value_copy;

	while (s != NULL) {
		/* Analyze the string ('name=value,name=value\0') */
		char *item_name = s;
		s = strchr(s, '=');
		if (s == NULL) break;
		if (*s == '\0') break;
		*s = '\0';
		s++;

		char *item_value = s;
		s = strchr(s, ',');
		if (s != NULL) {
			*s = '\0';
			s++;
		}

		this->SetSetting(item_name, atoi(item_value));
	}
	free(value_copy);
}

void ScriptConfig::SettingsToString(char *string, size_t size) const
{
	stringb buf (size, string);
	for (SettingValueList::const_iterator it = this->settings.begin(); it != this->settings.end(); it++) {
		/* Check if the string fits. */
		size_t len = buf.length();
		if (!buf.append_fmt ("%s=%d,", (*it).first, (*it).second)) {
			/* If it doesn't, rewind and skip this setting. */
			buf.truncate (len);
		}
	}
	/* Remove the last ',', but only if at least one setting was saved. */
	if (!buf.empty()) buf.truncate (buf.length() - 1);
}

TextfileDesc ScriptConfig::GetTextfile (TextfileType type, CompanyID slot) const
{
	if (slot == INVALID_COMPANY || this->GetInfo() == NULL) return TextfileDesc();

	return TextfileDesc (type, (slot == OWNER_DEITY) ? GAME_DIR : AI_DIR, this->GetInfo()->GetMainScript());
}
