/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_controller.cpp Implementation of ScriptControler. */

#include "../../stdafx.h"
#include "../../string.h"
#include "../../script/squirrel.hpp"
#include "../../rev.h"

#include "script_controller.hpp"
#include "script_error.hpp"
#include "../script_fatalerror.hpp"
#include "../script_info.hpp"
#include "../script_instance.hpp"
#include "script_log.hpp"
#include "../../ai/ai_gui.hpp"
#include "../../settings_type.h"
#include "../../network/network.h"

/* static */ void ScriptController::SetCommandDelay(int ticks)
{
	if (ticks <= 0) return;
	ScriptObject::GetActiveInstance()->SetDoCommandDelay (ticks);
}

/* static */ void ScriptController::Sleep(int ticks)
{
	if (!ScriptObject::CanSuspend()) {
		throw Script_FatalError("You are not allowed to call Sleep in your constructor, Save(), Load(), and any valuator.");
	}

	if (ticks <= 0) {
		ScriptLog::Warning("Sleep() value should be > 0. Assuming value 1.");
		ticks = 1;
	}

	throw Script_Suspend(ticks, NULL);
}

/* static */ void ScriptController::Break(const char* message)
{
	if (_network_dedicated || !_settings_client.gui.ai_developer_tools) return;

	ScriptObject::GetActiveInstance()->Pause();

	char log_message[1024];
	bstrfmt (log_message, "Break: %s", message);
	ScriptLog::Log (ScriptInstance::LogData::LOG_SQ_ERROR, log_message);

	/* Inform script developer that his script has been paused and
	 * needs manual action to continue. */
	ShowAIDebugWindow(ScriptObject::GetRootCompany());

	if ((_pause_mode & PM_PAUSED_NORMAL) == PM_UNPAUSED) {
		ScriptObject::DoCommand(0, PM_PAUSED_NORMAL, 1, CMD_PAUSE);
	}
}

/* static */ void ScriptController::Print(bool error_msg, const char *message)
{
	ScriptLog::Log (error_msg ? ScriptInstance::LogData::LOG_SQ_ERROR : ScriptInstance::LogData::LOG_SQ_INFO, message);
}

/* static */ uint ScriptController::GetTick()
{
	return ScriptObject::GetActiveInstance()->GetTick();
}

/* static */ int ScriptController::GetOpsTillSuspend()
{
	return ScriptObject::GetActiveInstance()->GetOpsTillSuspend();
}

/* static */ int ScriptController::GetSetting(const char *name)
{
	return ScriptObject::GetActiveInstance()->GetSetting(name);
}

/* static */ uint ScriptController::GetVersion()
{
	return _openttd_newgrf_version;
}

void SQController_Register (Squirrel *engine, const char *name)
{
	engine->AddClassBegin (name);
	SQConvert::DefSQStaticMethod (engine, &ScriptController::GetTick,           "GetTick",           1, ".");
	SQConvert::DefSQStaticMethod (engine, &ScriptController::GetOpsTillSuspend, "GetOpsTillSuspend", 1, ".");
	SQConvert::DefSQStaticMethod (engine, &ScriptController::SetCommandDelay,   "SetCommandDelay",   2, ".i");
	SQConvert::DefSQStaticMethod (engine, &ScriptController::Sleep,             "Sleep",             2, ".i");
	SQConvert::DefSQStaticMethod (engine, &ScriptController::Break,             "Break",             2, ".s");
	SQConvert::DefSQStaticMethod (engine, &ScriptController::GetSetting,        "GetSetting",        2, ".s");
	SQConvert::DefSQStaticMethod (engine, &ScriptController::GetVersion,        "GetVersion",        1, ".");
	SQConvert::DefSQStaticMethod (engine, &ScriptController::Print,             "Print",             3, ".bs");
	engine->AddClassEnd();

	/* Register the import statement to the global scope */
	engine->AddMethod ("import", &ScriptInstance::Import, 4, ".ssi");
}
