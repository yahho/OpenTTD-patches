/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_scanner.cpp Allows scanning for scripts. */

#include "../stdafx.h"

#include "script_scanner.hpp"
#include "script_info.hpp"

bool ScriptScanner::AddFile(const char *filename, size_t basepath_length, const char *tar_filename)
{
	free(this->main_script);

	const char *sep = strrchr (filename, PATHSEPCHAR);
	if (sep == NULL) {
		this->main_script = xstrdup ("main.nut");
	} else {
		this->main_script = str_fmt ("%.*smain.nut", (int)(sep - filename + 1), filename);
	}
	if (this->main_script == NULL) return false;

	free(this->tar_file);
	if (tar_filename != NULL) {
		this->tar_file = xstrdup(tar_filename);
	} else {
		this->tar_file = NULL;
	}

	if (!FioCheckFileExists(filename, this->subdir) || !FioCheckFileExists(this->main_script, this->subdir)) return false;

	this->Initialize();
	this->RegisterAPI();
	this->LoadScript (filename);
	this->Uninitialize();

	return true;
}

ScriptScanner::ScriptScanner (const char *name) :
	Squirrel(name),
	main_script(NULL),
	tar_file(NULL)
{
}

ScriptScanner::~ScriptScanner()
{
	free(this->main_script);
	free(this->tar_file);
}

bool ScriptScanner::check_method (const char *name)
{
	if (!this->method_exists (name)) {
		char error[1024];
		bstrfmt (error, "your info.nut/library.nut doesn't have the method '%s'", name);
		this->ThrowError (error);
		return false;
	}
	return true;
}

bool ScriptScanner::call_bool_method (const char *name, int suspend,
	bool *res)
{
	HSQOBJECT ret;
	if (!this->CallMethod (this->instance, name, suspend, &ret)) return false;
	if (ret._type != OT_BOOL) return false;
	*res = sq_objtobool (&ret) == 1;
	return true;
}

bool ScriptScanner::call_integer_method (const char *name, int suspend,
	int *res)
{
	HSQOBJECT ret;
	if (!this->CallMethod (this->instance, name, suspend, &ret)) return false;
	if (ret._type != OT_INTEGER) return false;
	*res = sq_objtointeger (&ret);
	return true;
}

char *ScriptScanner::call_string_method (const char *name, int suspend)
{
	HSQOBJECT ret;
	if (!this->CallMethod (this->instance, name, suspend, &ret)) return NULL;
	if (ret._type != OT_STRING) return NULL;
	char *res = xstrdup (sq_objtostring (&ret));
	ValidateString (res);
	return res;
}

const char *ScriptScanner::call_string_method_from_set (const char *name,
	size_t n, const char *const *val, int suspend)
{
	HSQOBJECT ret;
	if (!this->CallMethod (this->instance, name, suspend, &ret)) return NULL;
	if (ret._type != OT_STRING) return NULL;
	const char *s = sq_objtostring (&ret);
	for (size_t i = 0; i < n; i++) {
		if (strcmp (s, val[i]) == 0) {
			return val[i];
		}
	}
	return NULL;
}

SQInteger ScriptScanner::construct (ScriptInfo *info)
{
	/* Set some basic info from the parent */
	Squirrel::GetInstance (this->GetVM(), &this->instance, 2);
	/* Make sure the instance stays alive over time */
	sq_addref (this->GetVM(), &this->instance);

	return info->construct (this);
}
