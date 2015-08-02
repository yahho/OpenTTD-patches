/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_instance.hpp The ScriptInstance tracks a script. */

#ifndef SCRIPT_INSTANCE_HPP
#define SCRIPT_INSTANCE_HPP

#include <bitset>
#include <vector>
#include <queue>

#include "squirrel.hpp"
#include "script_suspend.hpp"

#include "../command_type.h"
#include "../company_type.h"
#include "../fileio_type.h"
#include "../saveload/saveload_buffer.h"

#include "../signs_func.h"
#include "../vehicle_func.h"
#include "../road_type.h"
#include "../group.h"
#include "../goal_type.h"
#include "../story_type.h"

#include "table/strings.h"

static const uint SQUIRREL_MAX_DEPTH = 25; ///< The maximum recursive depth for items stored in the savegame.

/**
 * The callback function for Mode-classes.
 */
typedef bool (ScriptModeProc)();

/** Runtime information about a script like a pointer to the squirrel vm and the current state. */
class ScriptInstance : protected Squirrel {
public:
	friend class ScriptObject;
	friend class ScriptController;
	friend class ScriptLog;

	/**
	 * Create a new script.
	 */
	ScriptInstance(const char *APIName);
	virtual ~ScriptInstance();

	/**
	 * Initialize the script and prepare it for its first run.
	 * @param info The ScriptInfo of the script.
	 * @param company Which company this script is serving.
	 * @param load Use this function to load the script (for the dummy script)
	 */
	void Initialize (const class ScriptInfo *info, CompanyID company,
		void (*load) (HSQUIRRELVM) = NULL);

	/**
	 * Get the value of a setting of the current instance.
	 * @param name The name of the setting.
	 * @return the value for the setting, or -1 if the setting is not known.
	 */
	virtual int GetSetting(const char *name) = 0;

	/**
	 * Find a library.
	 * @param library The library name to find.
	 * @param version The version the library should have.
	 * @return The library if found, NULL otherwise.
	 */
	virtual class ScriptInfo *FindLibrary(const char *library, int version) = 0;

	/**
	 * A script in multiplayer waits for the server to handle his DoCommand.
	 *  It keeps waiting for this until this function is called.
	 */
	void Continue();

	/**
	 * Run the GameLoop of a script.
	 */
	void GameLoop();

	/**
	 * Let the VM collect any garbage.
	 */
	void CollectGarbage();

	/**
	 * Return a true/false reply for a DoCommand.
	 */
	static void DoCommandReturn(ScriptInstance *instance);

	/**
	 * Return a VehicleID reply for a DoCommand.
	 */
	static void DoCommandReturnVehicleID(ScriptInstance *instance);

	/**
	 * Return a SignID reply for a DoCommand.
	 */
	static void DoCommandReturnSignID(ScriptInstance *instance);

	/**
	 * Return a GroupID reply for a DoCommand.
	 */
	static void DoCommandReturnGroupID(ScriptInstance *instance);

	/**
	 * Return a GoalID reply for a DoCommand.
	 */
	static void DoCommandReturnGoalID(ScriptInstance *instance);

	/**
	 * Return a StoryPageID reply for a DoCommand.
	 */
	static void DoCommandReturnStoryPageID(ScriptInstance *instance);

	/**
	 * Return a StoryPageElementID reply for a DoCommand.
	 */
	static void DoCommandReturnStoryPageElementID(ScriptInstance *instance);

	/**
	 * Get the controller attached to the instance.
	 */
	class ScriptController *GetController() { return controller; }

	/**
	 * Return the "this script died" value
	 */
	inline bool IsDead() const { return this->state.test (STATE_DEAD); }

	/**
	 * Call the script Save function and save all data in the savegame.
	 * @param dumper The dumper to save the data to
	 */
	void Save(SaveDumper *dumper);

	/**
	 * Don't save any data in the savegame.
	 * @param dumper The dumper to save the data to
	 */
	static void SaveEmpty(SaveDumper *dumper);

	/**
	 * Load data from a savegame and store it on the stack.
	 * @param reader The buffer to load the data from
	 * @param version The version of the script when saving, or -1 if this was
	 *  not the original script saving the game.
	 */
	void Load(LoadBuffer *reader, int version);

	/**
	 * Load and discard data from a savegame.
	 * @param reader The buffer to load the data from
	 */
	static void LoadEmpty(LoadBuffer *reader);

	/**
	 * Suspends the script for the current tick and then pause the execution
	 * of script. The script will not be resumed from its suspended state
	 * until the script has been unpaused.
	 */
	void Pause();

	/**
	 * Checks if the script is paused.
	 * @return true if the script is paused, otherwise false
	 */
	bool IsPaused() const
	{
		return this->state.test (STATE_PAUSED);
	}

	/**
	 * Resume execution of the script. This function will not actually execute
	 * the script, but set a flag so that the script is executed my the usual
	 * mechanism that executes the script.
	 */
	void Unpause()
	{
		this->state.reset (STATE_PAUSED);
	}

	/**
	 * Get the number of operations the script can execute before being suspended.
	 * This function is safe to call from within a function called by the script.
	 * @return The number of operations to execute.
	 */
	SQInteger GetOpsTillSuspend();

	/**
	 * DoCommand callback function for all commands executed by scripts.
	 * @param result The result of the command.
	 */
	void DoCommandCallback (const CommandCost &result);

	/**
	 * Check if there is an event waiting.
	 * @return true if there is an event on the stack.
	 */
	bool IsEventWaiting (void) const
	{
		return !this->events.empty();
	}

	/**
	 * Get the next event.
	 * @return a class of the event-child issues.
	 */
	class ScriptEvent *GetNextEvent (void);

	/**
	 * Insert an event for this script.
	 * @param event The event to insert.
	 */
	void InsertEvent(class ScriptEvent *event);

	/**
	 * Check if the instance is sleeping, which either happened because the
	 *  script executed a DoCommand, executed this.Sleep() or it has been
	 *  paused.
	 */
	bool IsSleeping() { return this->suspend != 0; }

protected:
	const char *versionAPI;               ///< Current API used by this script.

	/**
	 * Register all API functions to the VM.
	 */
	virtual void RegisterAPI();

	/**
	 * Load squirrel scripts to emulate an older API.
	 * @param api_version: API version to load scripts for
	 * @param dir Subdirectory to find the scripts in
	 * @return true iff script loading should proceed
	 */
	bool LoadCompatibilityScripts(const char *api_version, Subdirectory dir);

	/**
	 * Tell the script it died.
	 */
	virtual void Died();

	/**
	 * Get the command source to be used in DoCommand (to determine the callback)
	 */
	virtual CommandSource GetCommandSource() = 0;

private:
	class ScriptController *controller;   ///< The script main class.
	SQObject *instance;                   ///< Squirrel-pointer to the script main class.

	enum {
		STATE_INIT,     ///< The script engine is initialised.
		STATE_STARTED,  ///< The script constructor has run.
		STATE_PAUSED,   ///< The script is paused.
		STATE_DEAD,     ///< The script has been stopped.
		STATE_SAVEDATA, ///< The save data is still on the stack.
		STATE__N,
	};

	std::bitset <STATE__N> state;         ///< State flags of the script.
	int suspend;                          ///< The amount of ticks to suspend this script before it's allowed to continue.
	Script_SuspendCallbackProc *callback; ///< Callback that should be called in the next tick the script runs.

public:
	/** Log struct. */
	struct LogData {
		/** Log levels. */
		enum Level {
			LOG_SQ_ERROR = 0, ///< Squirrel printed an error.
			LOG_ERROR = 1,    ///< User printed an error.
			LOG_SQ_INFO = 2,  ///< Squirrel printed some info.
			LOG_WARNING = 3,  ///< User printed some warning.
			LOG_INFO = 4,     ///< User printed some info.
		};

		/** Log line struct. */
		struct Line {
			Level level;    ///< Log level.
			char msg [];    ///< Log message.
		};

		static const uint SIZE = 400;

		Line *lines [SIZE];     ///< The log lines.
		uint pos;               ///< Current position in lines.
		uint used;              ///< Total amount of used lines.

		LogData (void) : pos(0), used(0) { }

		~LogData();

		/** Log a message. */
		const char *log (Level level, const char *message);
	};

	typedef LogData::Level LogLevel;
	typedef LogData::Line  LogLine;

	LogData log; ///< Log data

	/** Internal command to log the message in a common way. */
	void Log (LogLevel level, const char *message);

private:
	std::queue <class ScriptEvent *> events; ///< The event queue.

	/* Storage for the script. It keeps track of important information. */
	ScriptModeProc *mode;             ///< The current build mode we are int.
	class ScriptObject *mode_instance; ///< The instance belonging to the current build mode.
	CompanyID root_company;          ///< The root company, the company that the script really belongs to.
	CompanyID company;               ///< The current company.

	uint delay;                      ///< The ticks of delay each DoCommand has.
	bool allow_do_command;           ///< Is the usage of DoCommands restricted?

	CommandCost costs;               ///< The costs the script is tracking.
	Money last_cost;                 ///< The last cost of the command.
	uint last_error;                 ///< The last error of the command.
	bool last_command_res;           ///< The last result of the command.

	VehicleID new_vehicle_id;        ///< The ID of the new Vehicle.
	SignID new_sign_id;              ///< The ID of the new Sign.
	GroupID new_group_id;            ///< The ID of the new Group.
	GoalID new_goal_id;              ///< The ID of the new Goal.
	StoryPageID new_story_page_id;   ///< The ID of the new StoryPage.
	StoryPageID new_story_page_element_id; ///< The ID of the new StoryPageElement.

	std::vector<int> callback_value; ///< The values which need to survive a callback.

	RoadType road_type;              ///< The current roadtype we build.
	RailType rail_type;              ///< The current railtype we build.

	/**
	 * Call the script Load function if it exists and data was loaded
	 *  from a savegame.
	 */
	bool CallLoad();

	/**
	 * Save one object (int / string / array / table) to the savegame.
	 * @param dumper The dumper to save the data to; NULL to only check if they are valid
	 * @param vm The virtual machine to get all the data from.
	 * @param index The index on the squirrel stack of the element to save.
	 * @param max_depth The maximum depth recursive arrays / tables will be stored
	 *   with before an error is returned.
	 * @return True if the saving was successful.
	 */
	static bool SaveObject(SaveDumper *dumper, HSQUIRRELVM vm, SQInteger index, int max_depth);

	/**
	 * Load all objects from a savegame.
	 * @param reader The buffer to load the data from
	 * @return True if the loading was successful.
	 */
	static bool LoadObjects(LoadBuffer *reader, HSQUIRRELVM vm);
};

#endif /* SCRIPT_INSTANCE_HPP */
