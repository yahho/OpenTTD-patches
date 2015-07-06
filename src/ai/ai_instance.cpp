/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ai_instance.cpp Implementation of AIInstance. */

#include "../stdafx.h"
#include "../debug.h"
#include "../error.h"

#include "../script/convert.hpp"

#include "ai_config.hpp"
#include "ai_gui.hpp"
#include "ai.hpp"

#include "../script/script_storage.hpp"
#include "ai_info.hpp"
#include "ai_instance.hpp"

/* Manually include the Text glue. */
#include "../script/api/script_text.hpp"
#include "../script/api/script_controller.hpp"

#include "../script/api/ai/ai.hpp.sq"

#include "../company_base.h"
#include "../company_func.h"

AIInstance::AIInstance() :
	ScriptInstance("AI")
{}

void AIInstance::Initialize (const AIInfo *info)
{
	this->versionAPI = info->GetAPIVersion();

	/* Register the AIController (including the "import" command) */
	SQAIController_Register(this->engine);

	ScriptInstance::Initialize (info, _current_company);
}

void AIInstance::RegisterAPI()
{
	ScriptInstance::RegisterAPI();

	/* Register all classes */
	SQAI_Register (this->engine);

	if (!this->LoadCompatibilityScripts(this->versionAPI, AI_DIR)) this->Died();
}

void AIInstance::Died()
{
	ScriptInstance::Died();

	ShowAIDebugWindow(_current_company);

	const AIInfo *info = AIConfig::GetConfig(_current_company, AIConfig::SSS_FORCE_GAME)->GetInfo();
	if (info != NULL) {
		ShowErrorMessage(STR_ERROR_AI_PLEASE_REPORT_CRASH, INVALID_STRING_ID, WL_WARNING);

		if (info->GetURL() != NULL) {
			ScriptLog::Info("Please report the error to the following URL:");
			ScriptLog::Info(info->GetURL());
		}
	}
}

static const char dummy_script_head[] =
	"class DummyAI extends AIController { function Start() { AILog.Error (\"";
static const char dummy_script_newline[] = "\"); AILog.Error (\"";
static const char dummy_script_tail[]    = "\"); } }";

struct DummyScriptHelper {
	const char *literal;
	const char *message;
};

static WChar dummy_script_reader (SQUserPointer userdata)
{
	DummyScriptHelper *data = (DummyScriptHelper*) userdata;

	if (data->literal != NULL) {
		unsigned char c = *data->literal;
		if (c != 0) {
			assert (c < '\x7f');
			data->literal++;
			return c;
		}
		if (data->message == NULL) return 0;
		data->literal = NULL;
	}

	assert (data->message != NULL);

	WChar c = Utf8Consume (&data->message);

	switch (c) {
		case '\0': /* switch to tail literal string */
			data->message = NULL;
			data->literal = dummy_script_tail;
			break;

		case '\n': /* switch to newline literal string */
			data->literal = dummy_script_newline;
			break;

		case '"':  /* escape special chars */
			data->literal = "\\\"";
			break;

		case '\\': /* escape special chars */
			data->literal = "\\\\";
			break;

		default: return c;
	}

	return *data->literal++;
}

void AIInstance::LoadDummyScript()
{
	/* Get the (translated) error message. */
	char error_message[1024];
	GetString (error_message, STR_ERROR_AI_NO_AI_FOUND);

	/* Load and run a dummy script. */
	DummyScriptHelper data = { dummy_script_head, error_message };

	HSQUIRRELVM vm = this->engine->GetVM();
	SQRESULT res;

	sq_pushroottable (vm);
	res = sq_compile (vm, dummy_script_reader, &data, "dummy", SQTrue);
	assert (SQ_SUCCEEDED(res));

	sq_push (vm, -2);
	res = sq_call (vm, 1, SQFalse, SQTrue);
	assert (SQ_SUCCEEDED(res));

	sq_pop (vm, 1);
}

int AIInstance::GetSetting(const char *name)
{
	return AIConfig::GetConfig(_current_company)->GetSetting(name);
}

ScriptInfo *AIInstance::FindLibrary(const char *library, int version)
{
	return (ScriptInfo *)AI::FindLibrary(library, version);
}

/**
 * DoCommand callback function for all commands executed by AIs.
 * @param result The result of the command.
 */
void CcAI (const CommandCost &result)
{
	/*
	 * The company might not exist anymore. Check for this.
	 * The command checks are not useful since this callback
	 * is also called when the command fails, which is does
	 * when the company does not exist anymore.
	 */
	const Company *c = Company::GetIfValid(_current_company);
	if (c == NULL || c->ai_instance == NULL) return;

	c->ai_instance->DoCommandCallback (result);
	c->ai_instance->Continue();
}

CommandSource AIInstance::GetCommandSource()
{
	return CMDSRC_AI;
}
