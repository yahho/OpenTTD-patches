/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file convert.cpp Conversion of types to and from squirrel. */

#include "../stdafx.h"
#include "convert.hpp"

char *SQConvert::GetString (HSQUIRRELVM vm, int index)
{
	/* Convert whatever there is as parameter to a string. */
	sq_tostring (vm, index);
	const char *tmp;
	sq_getstring (vm, -1, &tmp);
	char *tmp_str = xstrdup (tmp);
	sq_poptop (vm);
	str_validate (tmp_str, tmp_str + strlen (tmp_str));
	return tmp_str;
}

const char *SQConvert::GetMethodPointers (HSQUIRRELVM vm,
	SQUserPointer *obj, SQUserPointer *method, const char *cname)
{
	/* Find the amount of params we got */
	int nparam = sq_gettop (vm);
	HSQOBJECT instance;

	/* Get the 'SQ' instance of this class */
	Squirrel::GetInstance (vm, &instance);

	/* Protect against calls to a non-static method in a static way */
	sq_pushroottable (vm);
	sq_pushstring (vm, cname, -1);
	sq_get (vm, -2);
	sq_pushobject (vm, instance);
	if (sq_instanceof (vm) != SQTrue) return "class method is non-static";
	sq_pop (vm, 3);

	/* Get the 'real' instance of this class */
	sq_getinstanceup (vm, 1, obj, 0);
	/* Get the real function pointer */
	sq_getuserdata (vm, nparam, method, 0);
	if (*obj == NULL) return "couldn't detect real instance of class for non-static call";
	/* Remove the userdata from the stack */
	sq_pop (vm, 1);

	return NULL;
}
