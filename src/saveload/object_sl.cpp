/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file object_sl.cpp Code handling saving and loading of objects */

#include "../stdafx.h"
#include "../object_base.h"
#include "../object_map.h"

#include "saveload_buffer.h"
#include "newgrf_sl.h"

static const SaveLoad _object_desc[] = {
	SLE_VAR(Object, location.tile, SLE_UINT32),
	SLE_VAR(Object, location.w,    SLE_FILE_U8 | SLE_VAR_U16),
	SLE_VAR(Object, location.h,    SLE_FILE_U8 | SLE_VAR_U16),
	SLE_REF(Object, town,          REF_TOWN),
	SLE_VAR(Object, build_date,    SLE_UINT32),
	SLE_VAR(Object, colour,        SLE_UINT8,    0, , 148, ),
	SLE_VAR(Object, view,          SLE_UINT8,    0, , 155, ),

	SLE_END()
};

static void Save_OBJS(SaveDumper *dumper)
{
	Object *o;

	/* Write the objects */
	FOR_ALL_OBJECTS(o) {
		dumper->WriteElement(o->index, o, _object_desc);
	}
}

static void Load_OBJS(LoadBuffer *reader)
{
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		Object *o = new (index) Object();
		reader->ReadObject(o, _object_desc);
	}
}

static void Ptrs_OBJS(const SavegameTypeVersion *stv)
{
	Object *o;
	FOR_ALL_OBJECTS(o) {
		SlObjectPtrs(o, _object_desc, stv);
	}
}

void AfterLoadObjects(const SavegameTypeVersion *stv)
{
	Object *o;
	FOR_ALL_OBJECTS(o) {
		if ((stv != NULL) && IsOTTDSavegameVersionBefore(stv, 148) && !IsTileType(o->location.tile, MP_OBJECT)) {
			/* Due to a small bug stale objects could remain. */
			delete o;
		} else {
			Object::IncTypeCount(GetObjectType(o->location.tile));
		}
	}
}

static void Save_OBID(SaveDumper *dumper)
{
	Save_NewGRFMapping(dumper, _object_mngr);
}

static void Load_OBID(LoadBuffer *reader)
{
	Load_NewGRFMapping(reader, _object_mngr);
}

extern const ChunkHandler _object_chunk_handlers[] = {
	{ 'OBID', Save_OBID, Load_OBID, NULL,      NULL, CH_ARRAY },
	{ 'OBJS', Save_OBJS, Load_OBJS, Ptrs_OBJS, NULL, CH_ARRAY | CH_LAST},
};
