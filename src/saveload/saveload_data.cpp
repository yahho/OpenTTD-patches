/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_data.cpp Saveload data definitions. */

#include <list>

#include "../stdafx.h"
#include "../string_func.h"
#include "saveload_data.h"
#include "saveload_internal.h"

extern const byte _conv_mem_size[] = {1, 1, 1, 2, 2, 4, 4, 8, 8, 0};
extern const byte _conv_file_size[] = {1, 1, 2, 2, 4, 4, 8, 8, 2};

/**
 * Return a signed-long version of the value of a setting
 * @param ptr pointer to the variable
 * @param conv type of variable, can be a non-clean
 * type, eg one with other flags because it is parsed
 * @return returns the value of the pointer-setting
 */
int64 ReadValue(const void *ptr, VarType conv)
{
	switch (GetVarMemType(conv)) {
		case SLE_VAR_BL:  return (*(const bool *)ptr != 0);
		case SLE_VAR_I8:  return *(const int8  *)ptr;
		case SLE_VAR_U8:  return *(const byte  *)ptr;
		case SLE_VAR_I16: return *(const int16 *)ptr;
		case SLE_VAR_U16: return *(const uint16*)ptr;
		case SLE_VAR_I32: return *(const int32 *)ptr;
		case SLE_VAR_U32: return *(const uint32*)ptr;
		case SLE_VAR_I64: return *(const int64 *)ptr;
		case SLE_VAR_U64: return *(const uint64*)ptr;
		case SLE_VAR_NULL:return 0;
		default: NOT_REACHED();
	}
}

/**
 * Write the value of a setting
 * @param ptr pointer to the variable
 * @param conv type of variable, can be a non-clean type, eg
 *             with other flags. It is parsed upon read
 * @param val the new value being given to the variable
 */
void WriteValue(void *ptr, VarType conv, int64 val)
{
	switch (GetVarMemType(conv)) {
		case SLE_VAR_BL:  *(bool  *)ptr = (val != 0);  break;
		case SLE_VAR_I8:  *(int8  *)ptr = val; break;
		case SLE_VAR_U8:  *(byte  *)ptr = val; break;
		case SLE_VAR_I16: *(int16 *)ptr = val; break;
		case SLE_VAR_U16: *(uint16*)ptr = val; break;
		case SLE_VAR_I32: *(int32 *)ptr = val; break;
		case SLE_VAR_U32: *(uint32*)ptr = val; break;
		case SLE_VAR_I64: *(int64 *)ptr = val; break;
		case SLE_VAR_U64: *(uint64*)ptr = val; break;
		case SLE_VAR_NULL: break;
		default: NOT_REACHED();
	}
}


/** Return the size in bytes of a reference (pointer) */
static inline size_t SlCalcRefLen()
{
	return IsSavegameVersionBefore(69) ? 2 : 4;
}

/**
 * Return the size in bytes of a certain type of atomic array
 * @param length The length of the array counted in elements
 * @param conv VarType type of the variable that is used in calculating the size
 */
static inline size_t SlCalcArrayLen(size_t length, VarType conv)
{
	return SlCalcConvFileLen(conv) * length;
}

/**
 * Calculate the gross length of the string that it
 * will occupy in the savegame. This includes the real length
 * and the length that the index will occupy.
 * @param ptr pointer to the stringbuffer
 * @param length maximum length of the string (buffer size, etc.)
 * @param conv type of data been used
 * @return return the gross length of the string
 */
static inline size_t SlCalcStringLen(const void *ptr, size_t length, StrType conv)
{
	size_t len;
	const char *str;

	if (conv & SLS_POINTER) {
		str = *(const char * const *)ptr;
		len = (str != NULL) ? strlen(str) : 0;
	} else {
		str = (const char *)ptr;
		len = ttd_strnlen(str, length - 1);
	}

	return len + GetGammaLength(len); // also include the length of the index
}

/**
 * Return the size in bytes of a list
 * @param list The std::list to find the size of
 */
static inline size_t SlCalcListLen(const void *list)
{
	const std::list<void *> *l = (const std::list<void *> *) list;

	int type_size = IsSavegameVersionBefore(69) ? 2 : 4;
	/* Each entry is saved as type_size bytes, plus type_size bytes are used for the length
	 * of the list */
	return l->size() * type_size + type_size;
}

/**
 * Calculate the size of an object.
 * @param object to be measured
 * @param sld The SaveLoad description of the object so we know how to manipulate it
 * @return size of given object
 */
size_t SlCalcObjLength(const void *object, const SaveLoad *sld)
{
	size_t length = 0;

	/* Need to determine the length and write a length tag. */
	for (; sld->type != SL_END; sld++) {
		/* CONDITIONAL saveload types depend on the savegame version */
		if (!SlIsObjectValidInSavegame(sld)) continue;

		switch (sld->type) {
			case SL_VAR: length += SlCalcConvFileLen(sld->conv); break;
			case SL_REF: length += SlCalcRefLen(); break;
			case SL_ARR: length += SlCalcArrayLen(sld->length, sld->conv); break;
			case SL_STR: length += SlCalcStringLen(GetVariableAddress(sld, object), sld->length, sld->conv); break;
			case SL_LST: length += SlCalcListLen(GetVariableAddress(sld, object)); break;
			case SL_WRITEBYTE: length++; break; // a byte is logically of size 1
			case SL_INCLUDE:   length += SlCalcObjLength(object, (SaveLoad*)sld->address); break;
			default: NOT_REACHED();
		}
	}

	return length;
}
