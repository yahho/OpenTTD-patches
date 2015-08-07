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
#include "../../company_func.h"
#include "../../company_base.h"
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


/* static */ void ScriptObject::SetDoCommandCosts(Money value)
{
	GetActiveInstance()->costs = CommandCost(value);
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

/* static */ void ScriptObject::SetRoadType(RoadType road_type)
{
	GetActiveInstance()->SetRoadType (road_type);
}

/* static */ RoadType ScriptObject::GetRoadType()
{
	return GetActiveInstance()->GetRoadType();
}

/* static */ void ScriptObject::SetRailType(RailType rail_type)
{
	GetActiveInstance()->SetRailType (rail_type);
}

/* static */ RailType ScriptObject::GetRailType()
{
	return GetActiveInstance()->GetRailType();
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
	GetActiveInstance()->SetCallbackVariable (index, value);
}

/* static */ int ScriptObject::GetCallbackVariable(int index)
{
	return GetActiveInstance()->GetCallbackVariable (index);
}

/* static */ bool ScriptObject::DoCommand(TileIndex tile, uint32 p1, uint32 p2, CommandID cmd, stringb *text, Script_SuspendCallbackProc *callback)
{
	return GetActiveInstance()->DoCommand (tile, p1, p2, cmd, text, callback);
}
