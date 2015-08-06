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

bool ScriptExecMode::ModeProc()
{
	/* In execution mode we only return 'true', telling the DoCommand it
	 *  should continue with the real execution of the command. */
	return true;
}

ScriptExecMode::ScriptExecMode()
{
	this->last_mode     = this->GetDoCommandMode();
	this->last_instance = this->GetDoCommandModeInstance();
	this->SetDoCommandMode(&ScriptExecMode::ModeProc, this);
}

ScriptExecMode::~ScriptExecMode()
{
	if (this->GetDoCommandModeInstance() != this) {
		/* Ignore this error if the script already died. */
		if (!ScriptObject::GetActiveInstance()->IsDead()) {
			throw Script_FatalError("ScriptExecMode object was removed while it was not the latest *Mode object created.");
		}
	}
	this->SetDoCommandMode(this->last_mode, this->last_instance);
}

bool ScriptTestMode::ModeProc()
{
	/* In test mode we only return 'false', telling the DoCommand it
	 *  should stop after testing the command and return with that result. */
	return false;
}

ScriptTestMode::ScriptTestMode()
{
	this->last_mode     = this->GetDoCommandMode();
	this->last_instance = this->GetDoCommandModeInstance();
	this->SetDoCommandMode(&ScriptTestMode::ModeProc, this);
}

ScriptTestMode::~ScriptTestMode()
{
	if (this->GetDoCommandModeInstance() != this) {
		/* Ignore this error if the script already died. */
		if (!ScriptObject::GetActiveInstance()->IsDead()) {
			throw Script_FatalError("Testmode object was removed while it was not the latest *Mode object created.");
		}
	}
	this->SetDoCommandMode(this->last_mode, this->last_instance);
}
