/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_object.hpp Main object, on which all objects depend. */

#ifndef SCRIPT_OBJECT_HPP
#define SCRIPT_OBJECT_HPP

#include "../../misc/countedptr.hpp"
#include "../../road_type.h"
#include "../../rail_type.h"
#include "../../command_type.h"

#include "script_types.hpp"
#include "../script_suspend.hpp"

/**
 * Uper-parent object of all API classes. You should never use this class in
 *   your script, as it doesn't publish any public functions. It is used
 *   internally to have a common place to handle general things, like internal
 *   command processing, and command-validation checks.
 * @api none
 */
class ScriptObject : public SimpleCountedObject {
friend class ScriptInstance;
protected:
	/**
	 * A class that handles the current active instance. By instantiating it at
	 *  the beginning of a function with the current active instance, it remains
	 *  active till the scope of the variable closes. It then automatically
	 *  reverts to the active instance it was before instantiating.
	 */
	class ActiveInstance {
	friend class ScriptObject;
	public:
		ActiveInstance(ScriptInstance *instance);
		~ActiveInstance();
	private:
		ScriptInstance *last_active;    ///< The active instance before we go instantiated.

		static ScriptInstance *active;  ///< The global current active instance.
	};

public:
	/**
	 * Store the latest result of a DoCommand per company.
	 * @param res The result of the last command.
	 */
	static void SetLastCommandRes(bool res);

	/**
	 * Get the currently active instance.
	 * @return The instance.
	 */
	static class ScriptInstance *GetActiveInstance();

protected:
	/**
	 * Executes a raw DoCommand for the script.
	 */
	static bool DoCommand(TileIndex tile, uint32 p1, uint32 p2, CommandID cmd, stringb *text = NULL, Script_SuspendCallbackProc *callback = NULL);

	/**
	 * Sets the DoCommand costs counter to a value.
	 */
	static void SetDoCommandCosts(Money value);

	/**
	 * Get the current DoCommand costs counter.
	 */
	static Money GetDoCommandCosts();

	/**
	 * Set the DoCommand last error.
	 */
	static void SetLastError(ScriptErrorType last_error);

	/**
	 * Get the DoCommand last error.
	 */
	static ScriptErrorType GetLastError();

	/**
	 * Set the road type.
	 */
	static void SetRoadType(RoadType road_type);

	/**
	 * Get the road type.
	 */
	static RoadType GetRoadType();

	/**
	 * Set the rail type.
	 */
	static void SetRailType(RailType rail_type);

	/**
	 * Get the rail type.
	 */
	static RailType GetRailType();

	/**
	 * Set the current company to execute commands for or request
	 *  information about.
	 * @param company The new company.
	 */
	static void SetCompany(CompanyID company);

	/**
	 * Get the current company we are executing commands for or
	 *  requesting information about.
	 * @return The current company.
	 */
	static CompanyID GetCompany();

	/**
	 * Set a variable that can be used by callback functions to pass information.
	 */
	static void SetCallbackVariable(int index, int value);

	/**
	 * Get the variable that is used by callback functions to pass information.
	 */
	static int GetCallbackVariable(int index);

	/**
	 * Get an allocated string with all control codes stripped off.
	 */
	static char *GetString(StringID string);
};

#endif /* SCRIPT_OBJECT_HPP */
