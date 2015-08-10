/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_mode.cpp Implementation of ScriptTestMode and ScriptExecMode. */

#include "../../stdafx.h"
#include "script_mode.hpp"
#include "../script_instance.hpp"
#include "../script_fatalerror.hpp"

BaseScriptMode::BaseScriptMode (bool t) : test(t)
{
	ScriptObject::GetActiveInstance()->PushBuildMode (this);
}

void BaseScriptMode::FinalRelease()
{
	ScriptObject::GetActiveInstance()->PopBuildMode (this);
}

ScriptExecMode::ScriptExecMode() : BaseScriptMode (false)
{
}

ScriptTestMode::ScriptTestMode() : BaseScriptMode (true)
{
}
