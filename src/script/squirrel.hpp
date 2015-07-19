/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file squirrel.hpp defines the Squirrel class */

#ifndef SQUIRREL_HPP
#define SQUIRREL_HPP

#include <squirrel.h>

/** The type of script we're working with, i.e. for who is it? */
enum ScriptType {
	ST_AI, ///< The script is for AI scripts.
	ST_GS, ///< The script is for Game scripts.
};

class Squirrel {
private:
	typedef void (SQPrintFunc)(bool error_msg, const char *message);

	HSQUIRRELVM vm;          ///< The VirtualMachine instance for squirrel
	SQPrintFunc *const print_func; ///< Either NULL or a custom print handler
	bool crashed;            ///< True if the squirrel script made an error.
	int overdrawn_ops;       ///< The amount of operations we have overdrawn.
	const char *APIName;     ///< Name of the API used for this squirrel.

	/** The internal RunError handler. */
	static SQInteger RunError (HSQUIRRELVM vm);

	/**
	 * Get the API name.
	 */
	const char *GetAPIName() { return this->APIName; }

protected:
	/**
	 * The CompileError handler.
	 */
	static void CompileError(HSQUIRRELVM vm, const char *desc, const char *source, SQInteger line, SQInteger column);

	/**
	 * If a user runs 'print' inside a script, this function gets the params.
	 */
	static void PrintFunc(HSQUIRRELVM vm, const char *s, ...);

	/**
	 * If an error has to be print, this function is called.
	 */
	static void ErrorPrintFunc(HSQUIRRELVM vm, const char *s, ...);

public:
	Squirrel (const char *APIName, SQPrintFunc *print_func = NULL)
		: print_func(print_func), APIName(APIName)
	{
	}

	/** Perform all initialization steps to create the engine. */
	void Initialize();

	/** Perform all the cleanups for the engine. */
	void Uninitialize();

	/** Get the Squirrel class associated with a VM. */
	static Squirrel *Get (HSQUIRRELVM vm)
	{
		Squirrel *engine = (Squirrel *) sq_getforeignptr (vm);
		assert (engine->vm == vm);
		return engine;
	}

	/**
	 * Get the squirrel VM. Try to avoid using this.
	 */
	HSQUIRRELVM GetVM() { return this->vm; }

	/**
	 * Load a script.
	 * @param script The full script-name to load.
	 * @return False if loading failed.
	 */
	bool LoadScript (const char *script, bool in_root = true);

	/**
	 * Load a file to a given VM.
	 */
	SQRESULT LoadFile(HSQUIRRELVM vm, const char *filename, SQBool printerror);

	/**
	 * Adds a function to the stack. Depending on the current state this means
	 *  either a method or a global function.
	 */
	void AddMethod(const char *method_name, SQFUNCTION proc, uint nparam = 0, const char *params = NULL, void *userdata = NULL, int size = 0);

	/**
	 * Adds a const to the stack. Depending on the current state this means
	 *  either a const to a class or to the global space.
	 */
	void AddConst(const char *var_name, int value);

	/**
	 * Adds a const to the stack. Depending on the current state this means
	 *  either a const to a class or to the global space.
	 */
	void AddConst(const char *var_name, uint value) { this->AddConst(var_name, (int)value); }

	/**
	 * Adds a const to the stack. Depending on the current state this means
	 *  either a const to a class or to the global space.
	 */
	void AddConst(const char *var_name, bool value);

	/**
	 * Adds a class to the global scope. Make sure to call AddClassEnd when you
	 *  are done adding methods.
	 */
	void AddClassBegin(const char *class_name);

	/**
	 * Adds a class to the global scope, extending 'parent_class'.
	 * Make sure to call AddClassEnd when you are done adding methods.
	 */
	void AddClassBegin(const char *class_name, const char *parent_class);

	/**
	 * Finishes adding a class to the global scope. If this isn't called, no
	 *  class is really created.
	 */
	void AddClassEnd();

	/**
	 * Resume a VM when it was suspended via a throw.
	 */
	bool Resume(int suspend = -1);

	/**
	 * Resume the VM with an error so it prints a stack trace.
	 */
	void ResumeError();

	/**
	 * Tell the VM to do a garbage collection run.
	 */
	void CollectGarbage();

	void InsertResult(bool result);
	void InsertResult(int result);
	void InsertResult(uint result) { this->InsertResult((int)result); }

	/**
	 * Call a method of an instance.
	 * @return False if the script crashed or returned a wrong type.
	 */
	bool CallMethod(HSQOBJECT instance, const char *method_name, int suspend, HSQOBJECT *ret = NULL);

	/**
	 * Check if a method exists in an instance.
	 */
	bool MethodExists(HSQOBJECT instance, const char *method_name);

	/**
	 * Creates a class instance, prefixed with the current API name.
	 * @param vm The VM to create the class instance for
	 * @param class_name The name of the class of which we create an instance.
	 * @param real_instance The instance to the real class, if it represents a real class.
	 * @param release_hook Optional param to give a release hook.
	 * @return False if creating failed.
	 */
	static bool CreatePrefixedClassInstance (HSQUIRRELVM vm, const char *class_name, void *real_instance, SQRELEASEHOOK release_hook);

	/**
	 * Creates a class instance.
	 * @param class_name The name of the class of which we create an instance.
	 * @param real_instance The instance to the real class, if it represents a real class.
	 * @param instance Returning value with the pointer to the instance.
	 * @return False if creating failed.
	 */
	bool CreateClassInstance(const char *class_name, void *real_instance, HSQOBJECT *instance);

	/**
	 * Get the Squirrel-instance pointer.
	 * @note This will only work just after a function-call from within Squirrel
	 *  to your C++ function.
	 */
	static bool GetInstance(HSQUIRRELVM vm, HSQOBJECT *ptr, int pos = 1) { sq_getclass(vm, pos); sq_getstackobj(vm, pos, ptr); sq_pop(vm, 1); return true; }

	/**
	 * Throw a Squirrel error that will be nicely displayed to the user.
	 */
	void ThrowError(const char *error) { sq_throwerror(this->vm, error); }

	/**
	 * Release a SQ object.
	 */
	void ReleaseObject(HSQOBJECT *ptr) { sq_release(this->vm, ptr); }

	/**
	 * Tell the VM to remove \c amount ops from the number of ops till suspend.
	 */
	static void DecreaseOps(HSQUIRRELVM vm, int amount);

	/**
	 * Did the squirrel code suspend or return normally.
	 * @return True if the function suspended.
	 */
	bool IsSuspended();

	/**
	 * Find out if the squirrel script made an error before.
	 */
	bool HasScriptCrashed();

	/**
	 * Set the script status to crashed.
	 */
	void CrashOccurred();

	/**
	 * Are we allowed to suspend the squirrel script at this moment?
	 */
	bool CanSuspend();

	/**
	 * How many operations can we execute till suspension?
	 */
	SQInteger GetOpsTillSuspend();
};

#endif /* SQUIRREL_HPP */
