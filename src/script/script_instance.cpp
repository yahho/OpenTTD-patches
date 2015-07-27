/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_instance.cpp Implementation of ScriptInstance. */

#include "../stdafx.h"
#include "../string.h"
#include "../debug.h"
#include "../core/math_func.hpp"
#include "../saveload/saveload.h"

#include <squirrel.h>
#include <sqstdmath.h>
#include "../script/convert.hpp"

#include "script_fatalerror.hpp"
#include "script_storage.hpp"
#include "script_info.hpp"
#include "script_instance.hpp"

#include "api/script_controller.hpp"
#include "api/script_error.hpp"
#include "api/script_event.hpp"
#include "api/script_log.hpp"

#include "../company_base.h"
#include "../company_func.h"
#include "../fileio_func.h"
#include "../window_func.h"

ScriptStorage::~ScriptStorage()
{
	/* Free our pointers */
	if (event_data != NULL) ScriptEventController::FreeEventPointer();
}

ScriptInstance::ScriptInstance(const char *APIName) :
	engine(NULL),
	versionAPI(NULL),
	controller(NULL),
	storage(NULL),
	instance(NULL),
	is_started(false),
	is_dead(false),
	is_save_data_on_stack(false),
	suspend(0),
	is_paused(false),
	callback(NULL)
{
	this->storage = new ScriptStorage();
	this->engine  = new Squirrel (APIName, &ScriptController::Print);
	this->engine->Initialize();
}

void ScriptInstance::Initialize (const ScriptInfo *info, CompanyID company,
	void (*load) (HSQUIRRELVM))
{
	ScriptObject::ActiveInstance active(this);

	this->controller = new ScriptController(company);

	/* Register the API functions and classes */
	this->RegisterAPI();

	try {
		ScriptObject::SetAllowDoCommand(false);
		/* Load and execute the script for this script */
		const char *main_script = info->GetMainScript();
		if (load != NULL) {
			load (this->engine->GetVM());
		} else if (!this->engine->LoadScript(main_script) || this->engine->IsSuspended()) {
			if (this->engine->IsSuspended()) ScriptLog::Error("This script took too long to load script. AI is not started.");
			this->Died();
			return;
		}

		/* Create the main-class */
		this->instance = xmalloct<SQObject>();
		if (!this->engine->CreateClassInstance (info->GetInstanceName(), this->controller, this->instance)) {
			this->Died();
			return;
		}
		ScriptObject::SetAllowDoCommand(true);
	} catch (Script_FatalError e) {
		this->is_dead = true;
		this->engine->ThrowError(e.GetErrorMessage());
		this->engine->ResumeError();
		this->Died();
	}
}

template <SQInteger op (SQInteger, SQInteger)>
static SQInteger squirrel_op (HSQUIRRELVM vm)
{
	SQInteger tmp1, tmp2;

	sq_getinteger(vm, 2, &tmp1);
	sq_getinteger(vm, 3, &tmp2);
	sq_pushinteger(vm, op (tmp1, tmp2));
	return 1;
}

void ScriptInstance::RegisterAPI()
{
	/* We don't use squirrel_helper here, as we want to register
	 * to the global scope and not to a class. */
	this->engine->AddMethod ("min", &squirrel_op <min <SQInteger> >, 3, ".ii");
	this->engine->AddMethod ("max", &squirrel_op <max <SQInteger> >, 3, ".ii");

	sqstd_register_mathlib (this->engine->GetVM());
}

bool ScriptInstance::LoadCompatibilityScripts(const char *api_version, Subdirectory dir)
{
	char script_name[32];
	bstrfmt (script_name, "compat_%s.nut", api_version);
	char buf[MAX_PATH];
	Searchpath sp;
	FOR_ALL_SEARCHPATHS(sp) {
		FioGetFullPath (buf, MAX_PATH, sp, dir, script_name);
		if (!FileExists(buf)) continue;

		if (this->engine->LoadScript(buf)) return true;

		ScriptLog::Error("Failed to load API compatibility script");
		DEBUG(script, 0, "Error compiling / running API compatibility script: %s", buf);
		return false;
	}

	ScriptLog::Warning("API compatibility script not found");
	return true;
}

ScriptInstance::~ScriptInstance()
{
	ScriptObject::ActiveInstance active(this);

	if (instance != NULL) this->engine->ReleaseObject(this->instance);
	if (engine != NULL) {
		this->engine->Uninitialize();
		delete this->engine;
	}
	delete this->storage;
	delete this->controller;
	free(this->instance);
}

void ScriptInstance::Continue()
{
	assert(this->suspend < 0);
	this->suspend = -this->suspend - 1;
}

void ScriptInstance::Died()
{
	DEBUG(script, 0, "The script died unexpectedly.");
	this->is_dead = true;

	if (this->instance != NULL) this->engine->ReleaseObject(this->instance);
	this->engine->Uninitialize();
	delete this->engine;
	this->instance = NULL;
	this->engine = NULL;
}

void ScriptInstance::GameLoop()
{
	ScriptObject::ActiveInstance active(this);

	if (this->IsDead()) return;
	if (this->engine->HasScriptCrashed()) {
		/* The script crashed during saving, kill it here. */
		this->Died();
		return;
	}
	if (this->is_paused) return;
	this->controller->ticks++;

	if (this->suspend   < -1) this->suspend++; // Multiplayer suspend, increase up to -1.
	if (this->suspend   < 0)  return;          // Multiplayer suspend, wait for Continue().
	if (--this->suspend > 0)  return;          // Singleplayer suspend, decrease to 0.

	_current_company = ScriptObject::GetCompany();

	/* If there is a callback to call, call that first */
	if (this->callback != NULL) {
		if (this->is_save_data_on_stack) {
			sq_poptop(this->engine->GetVM());
			this->is_save_data_on_stack = false;
		}
		assert (ScriptObject::GetActiveInstance() == this);
		try {
			this->callback(this);
		} catch (Script_Suspend e) {
			this->suspend  = e.GetSuspendTime();
			this->callback = e.GetSuspendCallback();

			return;
		}
	}

	this->suspend  = 0;
	this->callback = NULL;

	if (!this->is_started) {
		try {
			ScriptObject::SetAllowDoCommand(false);
			/* Run the constructor if it exists. Don't allow any DoCommands in it. */
			if (this->engine->MethodExists(*this->instance, "constructor")) {
				if (!this->engine->CallMethod(*this->instance, "constructor", MAX_CONSTRUCTOR_OPS) || this->engine->IsSuspended()) {
					if (this->engine->IsSuspended()) ScriptLog::Error("This script took too long to initialize. Script is not started.");
					this->Died();
					return;
				}
			}
			if (!this->CallLoad() || this->engine->IsSuspended()) {
				if (this->engine->IsSuspended()) ScriptLog::Error("This script took too long in the Load function. Script is not started.");
				this->Died();
				return;
			}
			ScriptObject::SetAllowDoCommand(true);
			/* Start the script by calling Start() */
			if (!this->engine->CallMethod(*this->instance, "Start",  _settings_game.script.script_max_opcode_till_suspend) || !this->engine->IsSuspended()) this->Died();
		} catch (Script_Suspend e) {
			this->suspend  = e.GetSuspendTime();
			this->callback = e.GetSuspendCallback();
		} catch (Script_FatalError e) {
			this->is_dead = true;
			this->engine->ThrowError(e.GetErrorMessage());
			this->engine->ResumeError();
			this->Died();
		}

		this->is_started = true;
		return;
	}
	if (this->is_save_data_on_stack) {
		sq_poptop(this->engine->GetVM());
		this->is_save_data_on_stack = false;
	}

	/* Continue the VM */
	try {
		if (!this->engine->Resume(_settings_game.script.script_max_opcode_till_suspend)) this->Died();
	} catch (Script_Suspend e) {
		this->suspend  = e.GetSuspendTime();
		this->callback = e.GetSuspendCallback();
	} catch (Script_FatalError e) {
		this->is_dead = true;
		this->engine->ThrowError(e.GetErrorMessage());
		this->engine->ResumeError();
		this->Died();
	}
}

void ScriptInstance::CollectGarbage() const
{
	if (this->is_started && !this->IsDead()) this->engine->CollectGarbage();
}

/* static */ void ScriptInstance::DoCommandReturn(ScriptInstance *instance)
{
	assert (ScriptObject::GetActiveInstance() == instance);
	instance->engine->InsertResult (instance->storage->last_command_res);
}

/* static */ void ScriptInstance::DoCommandReturnVehicleID(ScriptInstance *instance)
{
	assert (ScriptObject::GetActiveInstance() == instance);
	instance->engine->InsertResult (instance->storage->new_vehicle_id);
}

/* static */ void ScriptInstance::DoCommandReturnSignID(ScriptInstance *instance)
{
	assert (ScriptObject::GetActiveInstance() == instance);
	instance->engine->InsertResult (instance->storage->new_sign_id);
}

/* static */ void ScriptInstance::DoCommandReturnGroupID(ScriptInstance *instance)
{
	assert (ScriptObject::GetActiveInstance() == instance);
	instance->engine->InsertResult (instance->storage->new_group_id);
}

/* static */ void ScriptInstance::DoCommandReturnGoalID(ScriptInstance *instance)
{
	assert (ScriptObject::GetActiveInstance() == instance);
	instance->engine->InsertResult (instance->storage->new_goal_id);
}

/* static */ void ScriptInstance::DoCommandReturnStoryPageID(ScriptInstance *instance)
{
	assert (ScriptObject::GetActiveInstance() == instance);
	instance->engine->InsertResult (instance->storage->new_story_page_id);
}

/* static */ void ScriptInstance::DoCommandReturnStoryPageElementID(ScriptInstance *instance)
{
	assert (ScriptObject::GetActiveInstance() == instance);
	instance->engine->InsertResult (instance->storage->new_story_page_element_id);
}

ScriptStorage *ScriptInstance::GetStorage()
{
	return this->storage;
}

/*
 * All data is stored in the following format:
 * First 1 byte indicating if there is a data blob at all.
 * 1 byte indicating the type of data.
 * The data itself, this differs per type:
 *  - integer: a binary representation of the integer (int32).
 *  - string:  First one byte with the string length, then a 0-terminated char
 *             array. The string can't be longer than 255 bytes (including
 *             terminating '\0').
 *  - array:   All data-elements of the array are saved recursive in this
 *             format, and ended with an element of the type
 *             SQSL_ARRAY_TABLE_END.
 *  - table:   All key/value pairs are saved in this format (first key 1, then
 *             value 1, then key 2, etc.). All keys and values can have an
 *             arbitrary type (as long as it is supported by the save function
 *             of course). The table is ended with an element of the type
 *             SQSL_ARRAY_TABLE_END.
 *  - bool:    A single byte with value 1 representing true and 0 false.
 *  - null:    No data.
 */

/** The type of the data that follows in the savegame. */
enum SQSaveLoadType {
	SQSL_INT             = 0x00, ///< The following data is an integer.
	SQSL_STRING          = 0x01, ///< The following data is an string.
	SQSL_ARRAY           = 0x02, ///< The following data is an array.
	SQSL_TABLE           = 0x03, ///< The following data is an table.
	SQSL_BOOL            = 0x04, ///< The following data is a boolean.
	SQSL_NULL            = 0x05, ///< A null variable.
	SQSL_ARRAY_TABLE_END = 0xFF, ///< Marks the end of an array or table, no data follows.
};

/* static */ bool ScriptInstance::SaveObject(SaveDumper *dumper, HSQUIRRELVM vm, SQInteger index, int max_depth)
{
	if (max_depth == 0) {
		ScriptLog::Error("Savedata can only be nested to 25 deep. No data saved."); // SQUIRREL_MAX_DEPTH = 25
		return false;
	}

	switch (sq_gettype(vm, index)) {
		case OT_INTEGER: {
			SQInteger res;
			sq_getinteger(vm, index, &res);
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_INT);
				dumper->WriteUint32 ((int)res);
			}
			return true;
		}

		case OT_STRING: {
			const char *buf;
			sq_getstring(vm, index, &buf);
			size_t len = strlen(buf) + 1;
			if (len >= 255) {
				ScriptLog::Error("Maximum string length is 254 chars. No data saved.");
				return false;
			}
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_STRING);
				dumper->WriteByte(len);
				dumper->CopyBytes (buf, len);
			}
			return true;
		}

		case OT_ARRAY: {
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_ARRAY);
			}
			sq_pushnull(vm);
			while (SQ_SUCCEEDED(sq_next(vm, index - 1))) {
				/* Store the value */
				bool res = SaveObject(dumper, vm, -1, max_depth - 1);
				sq_pop(vm, 2);
				if (!res) {
					sq_pop(vm, 1);
					return false;
				}
			}
			sq_pop(vm, 1);
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_ARRAY_TABLE_END);
			}
			return true;
		}

		case OT_TABLE: {
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_TABLE);
			}
			sq_pushnull(vm);
			while (SQ_SUCCEEDED(sq_next(vm, index - 1))) {
				/* Store the key + value */
				bool res = SaveObject(dumper, vm, -2, max_depth - 1) && SaveObject(dumper, vm, -1, max_depth - 1);
				sq_pop(vm, 2);
				if (!res) {
					sq_pop(vm, 1);
					return false;
				}
			}
			sq_pop(vm, 1);
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_ARRAY_TABLE_END);
			}
			return true;
		}

		case OT_BOOL: {
			SQBool res;
			sq_getbool(vm, index, &res);
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_BOOL);
				dumper->WriteByte(res ? 1 : 0);
			}
			return true;
		}

		case OT_NULL: {
			if (dumper != NULL) {
				dumper->WriteByte(SQSL_NULL);
			}
			return true;
		}

		default:
			ScriptLog::Error("You tried to save an unsupported type. No data saved.");
			return false;
	}
}

/* static */ void ScriptInstance::SaveEmpty(SaveDumper *dumper)
{
	dumper->WriteByte(0);
}

void ScriptInstance::Save(SaveDumper *dumper)
{
	ScriptObject::ActiveInstance active(this);

	/* Don't save data if the script didn't start yet or if it crashed. */
	if (this->engine == NULL || this->engine->HasScriptCrashed()) {
		SaveEmpty(dumper);
		return;
	}

	HSQUIRRELVM vm = this->engine->GetVM();
	if (this->is_save_data_on_stack) {
		dumper->WriteByte(1);
		/* Save the data that was just loaded. */
		SaveObject(dumper, vm, -1, SQUIRREL_MAX_DEPTH);
	} else if (!this->is_started) {
		SaveEmpty(dumper);
		return;
	} else if (this->engine->MethodExists(*this->instance, "Save")) {
		HSQOBJECT savedata;
		/* We don't want to be interrupted during the save function. */
		bool backup_allow = ScriptObject::GetAllowDoCommand();
		ScriptObject::SetAllowDoCommand(false);
		try {
			if (!this->engine->CallMethod (*this->instance, "Save", MAX_SL_OPS, &savedata)) {
				/* The script crashed in the Save function. We can't kill
				 * it here, but do so in the next script tick. */
				SaveEmpty(dumper);
				this->engine->CrashOccurred();
				return;
			}
		} catch (Script_FatalError e) {
			/* If we don't mark the script as dead here cleaning up the squirrel
			 * stack could throw Script_FatalError again. */
			this->is_dead = true;
			this->engine->ThrowError(e.GetErrorMessage());
			this->engine->ResumeError();
			SaveEmpty(dumper);
			/* We can't kill the script here, so mark it as crashed (not dead) and
			 * kill it in the next script tick. */
			this->is_dead = false;
			this->engine->CrashOccurred();
			return;
		}
		ScriptObject::SetAllowDoCommand(backup_allow);

		if (!sq_istable(savedata)) {
			ScriptLog::Error(this->engine->IsSuspended() ? "This script took too long to Save." : "Save function should return a table.");
			SaveEmpty(dumper);
			this->engine->CrashOccurred();
			return;
		}
		sq_pushobject(vm, savedata);
		if (SaveObject(NULL, vm, -1, SQUIRREL_MAX_DEPTH)) {
			dumper->WriteByte(1);
			SaveObject(dumper, vm, -1, SQUIRREL_MAX_DEPTH);
			this->is_save_data_on_stack = true;
		} else {
			SaveEmpty(dumper);
			this->engine->CrashOccurred();
		}
	} else {
		ScriptLog::Warning("Save function is not implemented");
		dumper->WriteByte(0);
	}
}

void ScriptInstance::Pause()
{
	/* Suspend script. */
	HSQUIRRELVM vm = this->engine->GetVM();
	Squirrel::DecreaseOps(vm, _settings_game.script.script_max_opcode_till_suspend);

	this->is_paused = true;
}

void ScriptInstance::Unpause()
{
	this->is_paused = false;
}

bool ScriptInstance::IsPaused()
{
	return this->is_paused;
}

/* static */ bool ScriptInstance::LoadObjects(LoadBuffer *reader, HSQUIRRELVM vm)
{
	switch (reader->ReadByte()) {
		case SQSL_INT: {
			int value = reader->ReadUint32();
			if (vm != NULL) sq_pushinteger(vm, (SQInteger)value);
			return true;
		}

		case SQSL_STRING: {
			byte len = reader->ReadByte();
			static char buf[256];
			reader->CopyBytes (buf, len);
			if (vm != NULL) sq_pushstring(vm, buf, -1);
			return true;
		}

		case SQSL_ARRAY: {
			if (vm != NULL) sq_newarray(vm, 0);
			while (LoadObjects(reader, vm)) {
				if (vm != NULL) sq_arrayappend(vm, -2);
				/* The value is popped from the stack by squirrel. */
			}
			return true;
		}

		case SQSL_TABLE: {
			if (vm != NULL) sq_newtable(vm);
			while (LoadObjects(reader, vm)) {
				LoadObjects(reader, vm);
				if (vm != NULL) sq_rawset(vm, -3);
				/* The key (-2) and value (-1) are popped from the stack by squirrel. */
			}
			return true;
		}

		case SQSL_BOOL: {
			byte value = reader->ReadByte();
			if (vm != NULL) sq_pushbool(vm, (SQBool)(value != 0));
			return true;
		}

		case SQSL_NULL: {
			if (vm != NULL) sq_pushnull(vm);
			return true;
		}

		case SQSL_ARRAY_TABLE_END: {
			return false;
		}

		default: NOT_REACHED();
	}
}

/* static */ void ScriptInstance::LoadEmpty(LoadBuffer *reader)
{
	/* Check if there was anything saved at all. */
	if (reader->ReadByte() == 0) return;

	LoadObjects(reader, NULL);
}

void ScriptInstance::Load(LoadBuffer *reader, int version)
{
	ScriptObject::ActiveInstance active(this);

	if (this->engine == NULL || version == -1) {
		LoadEmpty(reader);
		return;
	}
	HSQUIRRELVM vm = this->engine->GetVM();

	/* Check if there was anything saved at all. */
	if (reader->ReadByte() == 0) return;

	sq_pushinteger(vm, version);
	LoadObjects(reader, vm);
	this->is_save_data_on_stack = true;
}

bool ScriptInstance::CallLoad()
{
	HSQUIRRELVM vm = this->engine->GetVM();
	/* Is there save data that we should load? */
	if (!this->is_save_data_on_stack) return true;
	/* Whatever happens, after CallLoad the savegame data is removed from the stack. */
	this->is_save_data_on_stack = false;

	if (!this->engine->MethodExists(*this->instance, "Load")) {
		ScriptLog::Warning("Loading failed: there was data for the script to load, but the script does not have a Load() function.");

		/* Pop the savegame data and version. */
		sq_pop(vm, 2);
		return true;
	}

	/* Go to the instance-root */
	sq_pushobject(vm, *this->instance);
	/* Find the function-name inside the script */
	sq_pushstring(vm, "Load", -1);
	/* Change the "Load" string in a function pointer */
	sq_get(vm, -2);
	/* Push the main instance as "this" object */
	sq_pushobject(vm, *this->instance);
	/* Push the version data and savegame data as arguments */
	sq_push(vm, -5);
	sq_push(vm, -5);

	/* Call the script load function. sq_call removes the arguments (but not the
	 * function pointer) from the stack. */
	if (SQ_FAILED(sq_call(vm, 3, SQFalse, SQFalse, MAX_SL_OPS))) return false;

	/* Pop 1) The version, 2) the savegame data, 3) the object instance, 4) the function pointer. */
	sq_pop(vm, 4);
	return true;
}

SQInteger ScriptInstance::GetOpsTillSuspend()
{
	return this->engine->GetOpsTillSuspend();
}

void ScriptInstance::DoCommandCallback (const CommandCost &result)
{
	ScriptObject::ActiveInstance active(this);

	ScriptObject::SetLastCommandRes(result.Succeeded());

	if (result.Failed()) {
		ScriptObject::SetLastError(ScriptError::StringToError(result.GetErrorMessage()));
	} else {
		ScriptObject::IncreaseDoCommandCosts(result.GetCost());
		ScriptObject::SetLastCost(result.GetCost());
	}
}

void ScriptInstance::InsertEvent(class ScriptEvent *event)
{
	ScriptObject::ActiveInstance active(this);

	ScriptEventController::InsertEvent(event);
}

ScriptInstance::LogData::~LogData()
{
	for (uint i = 0; i < this->used; i++) {
		free (this->lines[i]);
	}
}

inline const char *ScriptInstance::LogData::log (Level level, const char *message)
{
	/* Compute string length (cut at first newline). */
	const char *newline = strchr (message, '\n');
	size_t length = (newline != NULL) ? (newline - message) : strlen (message);

	/* Allocate buffer and advance counter. */
	Line *line;
	if (this->used < SIZE) {
		assert (this->pos == this->used);
		this->used++;
		line = NULL; // realloc (NULL, size) -> malloc (size)
	} else {
		line = this->lines[this->pos];
	}
	line = (Line *) xrealloc (line, sizeof(Line) + length + 1);
	this->lines[this->pos] = line;
	this->pos = (this->pos + 1) % SIZE;

	/* Copy contents into buffer. */
	line->level = level;
	memcpy (line->msg, message, length);
	line->msg[length] = '\0';

	return line->msg;
}

void ScriptInstance::Log (LogLevel level, const char *message)
{
	/* Push string into buffer. */
	message = this->log.log (level, message);

	/* Also still print to debug window. */
	static const char chars[] = "SEPWI";
	char logc = ((uint)level < lengthof(chars)) ? chars[level] : '?';
	CompanyID company = this->storage->root_company;
	DEBUG(script, level, "[%d] [%c] %s", (uint)company, logc, message);
	InvalidateWindowData (WC_AI_DEBUG, 0, company);
}
