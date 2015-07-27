/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_log.cpp Implementation of ScriptLog. */

#include "../../stdafx.h"
#include "script_log.hpp"
#include "../script_instance.hpp"

/* static */ void ScriptLog::Info(const char *message)
{
	ScriptLog::Log (ScriptInstance::LogData::LOG_INFO, message);
}

/* static */ void ScriptLog::Warning(const char *message)
{
	ScriptLog::Log (ScriptInstance::LogData::LOG_WARNING, message);
}

/* static */ void ScriptLog::Error(const char *message)
{
	ScriptLog::Log (ScriptInstance::LogData::LOG_ERROR, message);
}

/* static */ void ScriptLog::Log (ScriptInstance::LogLevel level, const char *message)
{
	ScriptObject::GetActiveInstance()->Log (level, message);
}
