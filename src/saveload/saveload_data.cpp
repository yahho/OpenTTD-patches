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
#include "saveload_error.h"
#include "../town.h"
#include "../station_base.h"
#include "../roadstop_base.h"
#include "../vehicle_base.h"
#include "../autoreplace_base.h"
#include "../linkgraph/linkgraph.h"
#include "../linkgraph/linkgraphjob.h"

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


/** Size in bytes of a reference (pointer) */
static const size_t REF_LENGTH = 4;

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

	/* Each entry is saved as REF_LENGTH bytes, plus REF_LENGTH bytes are used for the length
	 * of the list */
	return l->size() * REF_LENGTH + REF_LENGTH;
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
		if (!SlIsObjectCurrentlyValid(sld)) continue;
		if (sld->flags & SLF_NOT_IN_SAVE) continue;

		switch (sld->type) {
			case SL_VAR: length += SlCalcConvFileLen(sld->conv); break;
			case SL_REF: length += REF_LENGTH; break;
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


/**
 * Pointers cannot be loaded from a savegame, so this function
 * gets the index from the savegame and returns the appropriate
 * pointer from the already loaded base.
 * Remember that an index of 0 is a NULL pointer so all indices
 * are +1 so vehicle 0 is saved as 1.
 * @param index The index that is being converted to a pointer
 * @param rt SLRefType type of the object the pointer is sought of
 * @param stv Savegame type and version
 * @return Return the index converted to a pointer of any type
 */
static void *IntToReference(size_t index, SLRefType rt, const SavegameTypeVersion *stv)
{
	assert_compile(sizeof(size_t) <= sizeof(void *));

	/* After version 4.3 REF_VEHICLE_OLD is saved as REF_VEHICLE,
	 * and should be loaded like that */
	if (rt == REF_VEHICLE_OLD && !IsOTTDSavegameVersionBefore(stv, 4, 4)) {
		rt = REF_VEHICLE;
	}

	/* No need to look up NULL pointers, just return immediately */
	if (index == (rt == REF_VEHICLE_OLD ? 0xFFFF : 0)) return NULL;

	/* Correct index. Old vehicles were saved differently:
	 * invalid vehicle was 0xFFFF, now we use 0x0000 for everything invalid. */
	if (rt != REF_VEHICLE_OLD) index--;

	switch (rt) {
		case REF_ORDERLIST:
			if (OrderList::IsValidID(index)) return OrderList::Get(index);
			throw SlCorrupt("Referencing invalid OrderList");

		case REF_ORDER:
			if (Order::IsValidID(index)) return Order::Get(index);
			/* in old versions, invalid order was used to mark end of order list */
			if (IsOTTDSavegameVersionBefore(stv, 5, 2)) return NULL;
			throw SlCorrupt("Referencing invalid Order");

		case REF_VEHICLE_OLD:
		case REF_VEHICLE:
			if (Vehicle::IsValidID(index)) return Vehicle::Get(index);
			throw SlCorrupt("Referencing invalid Vehicle");

		case REF_STATION:
			if (Station::IsValidID(index)) return Station::Get(index);
			throw SlCorrupt("Referencing invalid Station");

		case REF_TOWN:
			if (Town::IsValidID(index)) return Town::Get(index);
			throw SlCorrupt("Referencing invalid Town");

		case REF_ROADSTOPS:
			if (RoadStop::IsValidID(index)) return RoadStop::Get(index);
			throw SlCorrupt("Referencing invalid RoadStop");

		case REF_ENGINE_RENEWS:
			if (EngineRenew::IsValidID(index)) return EngineRenew::Get(index);
			throw SlCorrupt("Referencing invalid EngineRenew");

		case REF_CARGO_PACKET:
			if (CargoPacket::IsValidID(index)) return CargoPacket::Get(index);
			throw SlCorrupt("Referencing invalid CargoPacket");

		case REF_STORAGE:
			if (PersistentStorage::IsValidID(index)) return PersistentStorage::Get(index);
			throw SlCorrupt("Referencing invalid PersistentStorage");

		case REF_LINK_GRAPH:
			if (LinkGraph::IsValidID(index)) return LinkGraph::Get(index);
			throw SlCorrupt("Referencing invalid LinkGraph");

		case REF_LINK_GRAPH_JOB:
			if (LinkGraphJob::IsValidID(index)) return LinkGraphJob::Get(index);
			throw SlCorrupt("Referencing invalid LinkGraphJob");

		default: NOT_REACHED();
	}
}

/**
 * Fix/null pointers in a SaveLoad object.
 * @param object The object whose pointers are to be fixed
 * @param sld The SaveLoad description of the object so we know how to manipulate it
 * @param stv Savegame type and version; NULL when clearing references
 */
void SlObjectPtrs(void *object, const SaveLoad *sld, const SavegameTypeVersion *stv)
{
	for (; sld->type != SL_END; sld++) {
		if ((stv != NULL) ? !SlIsObjectValidInSavegame(stv, sld) : !SlIsObjectCurrentlyValid(sld)) continue;

		switch (sld->type) {
			case SL_REF: {
				void **ptr = (void **)GetVariableAddress(sld, object);

				if (stv != NULL) {
					*ptr = IntToReference(*(size_t *)ptr, (SLRefType)sld->conv, stv);
				} else {
					*ptr = NULL;
				}
				break;
			}

			case SL_LST: {
				typedef std::list<void *> PtrList;
				PtrList *l = (PtrList *)GetVariableAddress(sld, object);

				if (stv != NULL) {
					PtrList temp = *l;

					l->clear();
					PtrList::iterator iter;
					for (iter = temp.begin(); iter != temp.end(); ++iter) {
						l->push_back(IntToReference((size_t)*iter, (SLRefType)sld->conv, stv));
					}
				} else {
					l->clear();
				}
				break;
			}

			case SL_INCLUDE:
				SlObjectPtrs(object, (SaveLoad*)sld->address, stv);
				break;

			default: break;
		}
	}
}
