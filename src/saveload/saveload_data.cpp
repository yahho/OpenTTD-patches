/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_data.cpp Saveload data definitions. */

#include "../stdafx.h"
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
		case SLE_VAR_NAME: *(char**)ptr = CopyFromOldName(val); break;
		case SLE_VAR_NULL: break;
		default: NOT_REACHED();
	}
}
