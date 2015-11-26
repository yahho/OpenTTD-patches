/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_text.cpp Implementation of ScriptText. */

#include "../../stdafx.h"
#include "../../string.h"
#include "../../strings_func.h"
#include "script_text.hpp"
#include "../../table/control_codes.h"

#include "table/strings.h"


/**
 * Convert a given encoded string into a decoded normal string.
 * @param encoded The encoded string.
 * @param decoded The buffer where to store the decoded string.
 */
inline void Text::GetDecodedText (const char *encoded, stringb *decoded)
{
	::SetDParamStr (0, encoded);
	::GetString (decoded, STR_JUST_RAW_STRING);
}


bool RawText::GetDecodedText (stringb *buf)
{
	this->Text::GetDecodedText (this->text.get(), buf);
	return true;
}


void ScriptText::Param::destroy (void)
{
	switch (this->type) {
		default: NOT_REACHED();
		case TYPE_INT:                        break;
		case TYPE_STRING: free (this->s);     break;
		case TYPE_TEXT:   this->t->Release(); break;
	}

	this->type = TYPE_INT;
	this->i = 0;
}

inline void ScriptText::Param::set_int (int64 value)
{
	this->destroy();
	this->type = TYPE_INT;
	this->i = value;
}

inline void ScriptText::Param::set_string (const char *value)
{
	this->destroy();
	this->type = TYPE_STRING;
	this->s = xstrdup (value);
	ValidateString (this->s);
}

inline void ScriptText::Param::set_text (ScriptText *value)
{
	this->destroy();
	this->type = TYPE_TEXT;
	value->AddRef();
	this->t = value;
}

/**
 * Encode this param into a string buffer.Internal function for recursive calling this function over multiple
 * @param buf The buffer.
 * @return The number of parameters added to the string.
 */
inline int ScriptText::Param::encode (stringb *buf)
{
	switch (this->type) {
		default: NOT_REACHED();

		case TYPE_INT:
			buf->append_fmt (OTTD_PRINTFHEX64, this->i);
			return 1;

		case TYPE_STRING:
			buf->append_fmt ("\"%s\"", this->s);
			return 1;

		case TYPE_TEXT:
			return this->t->_GetEncodedText (buf);
	}
}


ScriptText::ScriptText(HSQUIRRELVM vm) :
	params(), paramc(0), string(STR_NULL)
{
	int nparam = sq_gettop(vm) - 1;
	if (nparam < 1) {
		throw sq_throwerror(vm, "You need to pass at least a StringID to the constructor");
	}

	/* First resolve the StringID. */
	SQInteger sqstring;
	if (SQ_FAILED(sq_getinteger(vm, 2, &sqstring))) {
		throw sq_throwerror(vm, "First argument must be a valid StringID");
	}
	this->string = sqstring;

	/* The rest of the parameters must be arguments. */
	for (int i = 0; i < nparam - 1; i++) {
		/* Push the parameter to the top of the stack. */
		sq_push(vm, i + 3);

		if (SQ_FAILED(this->_SetParam(i, vm))) {
			throw sq_throwerror(vm, "Invalid parameter");
		}

		/* Pop the parameter again. */
		sq_pop(vm, 1);
	}
}

SQInteger ScriptText::_SetParam(int parameter, HSQUIRRELVM vm)
{
	if (parameter >= SCRIPT_TEXT_MAX_PARAMETERS) return SQ_ERROR;

	switch (sq_gettype(vm, -1)) {
		case OT_STRING: {
			const char *value;
			sq_getstring(vm, -1, &value);
			this->params[parameter].set_string (value);
			break;
		}

		case OT_INTEGER: {
			SQInteger value;
			sq_getinteger(vm, -1, &value);
			this->params[parameter].set_int (value);
			break;
		}

		case OT_INSTANCE: {
			SQUserPointer real_instance = NULL;
			HSQOBJECT instance;

			sq_getstackobj(vm, -1, &instance);

			/* Validate if it is a GSText instance */
			sq_pushroottable(vm);
			sq_pushstring(vm, "GSText", -1);
			sq_get(vm, -2);
			sq_pushobject(vm, instance);
			if (sq_instanceof(vm) != SQTrue) return SQ_ERROR;
			sq_pop(vm, 3);

			/* Get the 'real' instance of this class */
			sq_getinstanceup(vm, -1, &real_instance, 0);
			if (real_instance == NULL) return SQ_ERROR;

			ScriptText *value = static_cast<ScriptText *>(real_instance);
			this->params[parameter].set_text (value);
			break;
		}

		default: return SQ_ERROR;
	}

	if (this->paramc <= parameter) this->paramc = parameter + 1;
	return 0;
}

SQInteger ScriptText::SetParam(HSQUIRRELVM vm)
{
	if (sq_gettype(vm, 2) != OT_INTEGER) return SQ_ERROR;

	SQInteger k;
	sq_getinteger(vm, 2, &k);

	if (k > SCRIPT_TEXT_MAX_PARAMETERS) return SQ_ERROR;
	if (k < 1) return SQ_ERROR;
	k--;

	return this->_SetParam(k, vm);
}

SQInteger ScriptText::AddParam(HSQUIRRELVM vm)
{
	SQInteger res;
	res = this->_SetParam(this->paramc, vm);
	if (res != 0) return res;

	/* Push our own instance back on top of the stack */
	sq_push(vm, 1);
	return 1;
}

SQInteger ScriptText::_set(HSQUIRRELVM vm)
{
	int32 k;

	if (sq_gettype(vm, 2) == OT_STRING) {
		const char *key_string;
		sq_getstring(vm, 2, &key_string);
		ValidateString(key_string);

		if (strncmp(key_string, "param_", 6) != 0 || strlen(key_string) > 8) return SQ_ERROR;
		k = atoi(key_string + 6);
	} else if (sq_gettype(vm, 2) == OT_INTEGER) {
		SQInteger key;
		sq_getinteger(vm, 2, &key);
		k = (int32)key;
	} else {
		return SQ_ERROR;
	}

	if (k > SCRIPT_TEXT_MAX_PARAMETERS) return SQ_ERROR;
	if (k < 1) return SQ_ERROR;
	k--;

	return this->_SetParam(k, vm);
}

bool ScriptText::GetEncodedText (stringb *buf)
{
	buf->clear();
	return this->_GetEncodedText (buf) <= SCRIPT_TEXT_MAX_PARAMETERS;
}

int ScriptText::_GetEncodedText (stringb *buf)
{
	int param_count = 0;
	buf->append_utf8 (SCC_ENCODED);
	buf->append_fmt ("%X", this->string);
	for (int i = 0; i < this->paramc; i++) {
		buf->append (':');
		param_count += this->params[i].encode (buf);
	}
	return param_count;
}

bool ScriptText::GetDecodedText (stringb *buf)
{
	sstring <1024> encoded;
	if (!this->ScriptText::GetEncodedText (&encoded)) return false;

	this->Text::GetDecodedText (encoded.c_str(), buf);
	return true;
}
