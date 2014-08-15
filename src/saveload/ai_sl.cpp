/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ai_sl.cpp Handles the saveload part of the AIs */

#include "../stdafx.h"
#include "../company_base.h"
#include "../debug.h"
#include "saveload_buffer.h"
#include "saveload_error.h"
#include "../string.h"

#include "../ai/ai.hpp"
#include "../ai/ai_config.hpp"
#include "../network/network.h"
#include "../ai/ai_instance.hpp"

struct AiSaveload {
	char name[64];
	char settings[1024];
	int  version;
	bool is_random;
	CompanyID id;
};

static const SaveLoad _ai_company[] = {
	SLE_STR(AiSaveload, name,        SLS_STRB),
	SLE_STR(AiSaveload, settings,    SLS_STRB),
	SLE_VAR(AiSaveload, version,   SLE_UINT32, 0, , 108, ),
	SLE_VAR(AiSaveload, is_random,   SLE_BOOL, 0, , 136, ),
	SLE_END()
};

static void Load_AIPL(LoadBuffer *reader)
{
	/* Free all current data */
	for (CompanyID c = COMPANY_FIRST; c < MAX_COMPANIES; c++) {
		AIConfig::GetConfig(c, AIConfig::SSS_FORCE_GAME)->Change(NULL);
	}

	AiSaveload aisl;
	while ((aisl.id = (CompanyID)reader->IterateChunk()) != (CompanyID)-1) {
		if (aisl.id >= MAX_COMPANIES) throw SlCorrupt("Too many AI configs");

		aisl.is_random = 0;
		aisl.version = -1;
		reader->ReadObject(&aisl, _ai_company);

		if (_networking && !_network_server) {
			if (Company::IsValidAiID(aisl.id)) AIInstance::LoadEmpty(reader);
			continue;
		}

		AIConfig *config = AIConfig::GetConfig(aisl.id, AIConfig::SSS_FORCE_GAME);
		if (StrEmpty(aisl.name)) {
			/* A random AI. */
			config->Change(NULL, -1, false, true);
		} else {
			config->Change(aisl.name, aisl.version, false, aisl.is_random);
			if (!config->HasScript()) {
				/* No version of the AI available that can load the data. Try to load the
				 * latest version of the AI instead. */
				config->Change(aisl.name, -1, false, aisl.is_random);
				if (!config->HasScript()) {
					if (strcmp(aisl.name, "%_dummy") != 0) {
						DEBUG(script, 0, "The savegame has an AI by the name '%s', version %d which is no longer available.", aisl.name, aisl.version);
						DEBUG(script, 0, "A random other AI will be loaded in its place.");
					} else {
						DEBUG(script, 0, "The savegame had no AIs available at the time of saving.");
						DEBUG(script, 0, "A random available AI will be loaded now.");
					}
				} else {
					DEBUG(script, 0, "The savegame has an AI by the name '%s', version %d which is no longer available.", aisl.name, aisl.version);
					DEBUG(script, 0, "The latest version of that AI has been loaded instead, but it'll not get the savegame data as it's incompatible.");
				}
				/* Make sure the AI doesn't get the saveload data, as he was not the
				 *  writer of the saveload data in the first place */
				aisl.version = -1;
			}
		}

		config->StringToSettings(aisl.settings);

		/* Start the AI directly if it was active in the savegame */
		if (Company::IsValidAiID(aisl.id)) {
			AI::StartNew(aisl.id, false);
			AI::Load(reader, aisl.id, aisl.version);
		}
	}
}

static void Save_AIPL(SaveDumper *dumper)
{
	AiSaveload aisl;

	for (int i = COMPANY_FIRST; i < MAX_COMPANIES; i++) {
		AIConfig *config = AIConfig::GetConfig((CompanyID)i);

		if (config->HasScript()) {
			bstrcpy (aisl.name, config->GetName());
			aisl.version = config->GetVersion();
		} else {
			/* No AI is configured for this so store an empty string as name. */
			aisl.name[0] = '\0';
			aisl.version = -1;
		}

		aisl.is_random = config->IsRandom();
		aisl.settings[0] = '\0';
		config->SettingsToString(aisl.settings, lengthof(aisl.settings));

		/* If the AI was active, store his data too */
		aisl.id = (CompanyID)(Company::IsValidAiID(i) ? i : -1);

		SaveDumper temp(1024);

		temp.WriteObject(&aisl, _ai_company);
		/* If the AI was active, store his data too */
		if (aisl.id != (CompanyID)-1) AI::Save(&temp, aisl.id);

		dumper->WriteElementHeader(i, temp.GetSize());
		temp.Dump(dumper);
	}
}

extern const ChunkHandler _ai_chunk_handlers[] = {
	{ 'AIPL', Save_AIPL, Load_AIPL, NULL, NULL, CH_ARRAY | CH_LAST},
};
