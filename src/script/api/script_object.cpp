/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_object.cpp Implementation of ScriptObject. */

#include "../../stdafx.h"
#include "../../script/squirrel.hpp"
#include "../../command_func.h"
#include "../../company_func.h"
#include "../../company_base.h"
#include "../../network/network.h"
#include "../../genworld.h"
#include "../../string.h"
#include "../../strings_func.h"

#include "../script_instance.hpp"
#include "../script_fatalerror.hpp"
#include "script_error.hpp"

/* static */ ScriptInstance *ScriptObject::ActiveInstance::active = NULL;

ScriptObject::ActiveInstance::ActiveInstance(ScriptInstance *instance)
{
	this->last_active = ScriptObject::ActiveInstance::active;
	ScriptObject::ActiveInstance::active = instance;
}

ScriptObject::ActiveInstance::~ActiveInstance()
{
	ScriptObject::ActiveInstance::active = this->last_active;
}

/* static */ ScriptInstance *ScriptObject::GetActiveInstance()
{
	assert(ScriptObject::ActiveInstance::active != NULL);
	return ScriptObject::ActiveInstance::active;
}


/* static */ uint ScriptObject::GetDoCommandDelay()
{
	return GetActiveInstance()->delay;
}

/* static */ void ScriptObject::SetDoCommandMode(ScriptModeProc *proc, ScriptObject *instance)
{
	GetActiveInstance()->mode = proc;
	GetActiveInstance()->mode_instance = instance;
}

/* static */ ScriptModeProc *ScriptObject::GetDoCommandMode()
{
	return GetActiveInstance()->mode;
}

/* static */ ScriptObject *ScriptObject::GetDoCommandModeInstance()
{
	return GetActiveInstance()->mode_instance;
}

/* static */ void ScriptObject::SetDoCommandCosts(Money value)
{
	GetActiveInstance()->costs = CommandCost(value);
}

/* static */ void ScriptObject::IncreaseDoCommandCosts(Money value)
{
	GetActiveInstance()->costs.AddCost(value);
}

/* static */ Money ScriptObject::GetDoCommandCosts()
{
	return GetActiveInstance()->costs.GetCost();
}

/* static */ void ScriptObject::SetLastError(ScriptErrorType last_error)
{
	GetActiveInstance()->last_error = last_error;
}

/* static */ ScriptErrorType ScriptObject::GetLastError()
{
	return GetActiveInstance()->last_error;
}

/* static */ void ScriptObject::SetLastCost(Money last_cost)
{
	GetActiveInstance()->last_cost = last_cost;
}

/* static */ Money ScriptObject::GetLastCost()
{
	return GetActiveInstance()->last_cost;
}

/* static */ void ScriptObject::SetRoadType(RoadType road_type)
{
	GetActiveInstance()->road_type = road_type;
}

/* static */ RoadType ScriptObject::GetRoadType()
{
	return GetActiveInstance()->road_type;
}

/* static */ void ScriptObject::SetRailType(RailType rail_type)
{
	GetActiveInstance()->rail_type = rail_type;
}

/* static */ RailType ScriptObject::GetRailType()
{
	return GetActiveInstance()->rail_type;
}

/* static */ void ScriptObject::SetLastCommandRes(bool res)
{
	GetActiveInstance()->SetLastCommandRes (res);
}

/* static */ void ScriptObject::SetCompany(CompanyID company)
{
	if (GetActiveInstance()->root_company == INVALID_OWNER) GetActiveInstance()->root_company = company;
	GetActiveInstance()->company = company;

	_current_company = company;
}

/* static */ CompanyID ScriptObject::GetCompany()
{
	return GetActiveInstance()->company;
}

/* static */ CompanyID ScriptObject::GetRootCompany()
{
	return GetActiveInstance()->root_company;
}

/* static */ bool ScriptObject::CanSuspend()
{
	return GetActiveInstance()->CanSuspend();
}

/* static */ char *ScriptObject::GetString(StringID string)
{
	char buffer[64];
	::GetString (buffer, string);
	::str_validate(buffer, lastof(buffer), SVS_NONE);
	return ::xstrdup(buffer);
}

/* static */ void ScriptObject::SetCallbackVariable(int index, int value)
{
	if ((size_t)index >= GetActiveInstance()->callback_value.size()) GetActiveInstance()->callback_value.resize(index + 1);
	GetActiveInstance()->callback_value[index] = value;
}

/* static */ int ScriptObject::GetCallbackVariable(int index)
{
	return GetActiveInstance()->callback_value[index];
}

/* static */ bool ScriptObject::DoCommand(TileIndex tile, uint32 p1, uint32 p2, CommandID cmd, stringb *text, Script_SuspendCallbackProc *callback)
{
	if (!ScriptObject::CanSuspend()) {
		throw Script_FatalError("You are not allowed to execute any DoCommand (even indirect) in your constructor, Save(), Load(), and any valuator.");
	}

	if (ScriptObject::GetCompany() != OWNER_DEITY && !::Company::IsValidID(ScriptObject::GetCompany())) {
		ScriptObject::SetLastError(ScriptError::ERR_PRECONDITION_INVALID_COMPANY);
		return false;
	}

	if (text != NULL && (GetCommandFlags(cmd) & CMDF_STR_CTRL) == 0) {
		/* The string must be valid, i.e. not contain special codes. Since some
		 * can be made with GSText, make sure the control codes are removed. */
		text->validate (SVS_NONE);
	}

	/* Set the default callback to return a true/false result of the DoCommand */
	if (callback == NULL) callback = &ScriptInstance::DoCommandReturn;

	/* Are we only interested in the estimate costs? */
	bool estimate_only = GetDoCommandMode() != NULL && !GetDoCommandMode()();

#ifdef ENABLE_NETWORK
	/* Only set p2 when the command does not come from the network. */
	if (GetCommandFlags(cmd) & CMDF_CLIENT_ID && p2 == 0) p2 = UINT32_MAX;
#endif

	/* Try to perform the command. */
	CommandCost res = ::DoCommandPInternal(tile, p1, p2, cmd, text != NULL ? text->c_str() : NULL, estimate_only,
		_networking && !_generating_world ? ScriptObject::GetActiveInstance()->GetCommandSource() : CMDSRC_OTHER);

	/* We failed; set the error and bail out */
	if (res.Failed()) {
		SetLastError(ScriptError::StringToError(res.GetErrorMessage()));
		return false;
	}

	/* No error, then clear it. */
	SetLastError(ScriptError::ERR_NONE);

	/* Estimates, update the cost for the estimate and be done */
	if (estimate_only) {
		IncreaseDoCommandCosts(res.GetCost());
		return true;
	}

	/* Costs of this operation. */
	SetLastCost(res.GetCost());
	SetLastCommandRes(true);

	if (_generating_world) {
		IncreaseDoCommandCosts(res.GetCost());
		if (callback != NULL) {
			/* Insert return value into to stack and throw a control code that
			 * the return value in the stack should be used. */
			callback(GetActiveInstance());
			throw SQInteger(1);
		}
		return true;
	} else if (_networking) {
		/* Suspend the script till the command is really executed. */
		throw Script_Suspend(-(int)GetDoCommandDelay(), callback);
	} else {
		IncreaseDoCommandCosts(res.GetCost());

		/* Suspend the script player for 1+ ticks, so it simulates multiplayer. This
		 *  both avoids confusion when a developer launched his script in a
		 *  multiplayer game, but also gives time for the GUI and human player
		 *  to interact with the game. */
		throw Script_Suspend(GetDoCommandDelay(), callback);
	}

	NOT_REACHED();
}
