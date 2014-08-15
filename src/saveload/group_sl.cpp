/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file group_sl.cpp Code handling saving and loading of economy data */

#include "../stdafx.h"
#include "../group.h"

#include "saveload_buffer.h"

static const SaveLoad _group_desc[] = {
	 SLE_VAR(Group, name,               SLE_NAME,            , ,   0,  83),
	 SLE_STR(Group, name,               SLS_ALLOW_CONTROL,  0, ,  84,    ),
	SLE_NULL(2,                                              , ,   0, 163), // num_vehicle
	 SLE_VAR(Group, owner,              SLE_UINT8),
	 SLE_VAR(Group, vehicle_type,       SLE_UINT8),
	 SLE_VAR(Group, replace_protection, SLE_BOOL),
	 SLE_VAR(Group, parent,             SLE_UINT16,        18, , 189,    ),
	 SLE_END()
};

static void Save_GRPS(SaveDumper *dumper)
{
	Group *g;

	FOR_ALL_GROUPS(g) {
		dumper->WriteElement(g->index, g, _group_desc);
	}
}


static void Load_GRPS(LoadBuffer *reader)
{
	int index;

	while ((index = reader->IterateChunk()) != -1) {
		Group *g = new (index) Group();
		reader->ReadObject(g, _group_desc);

		if (reader->IsVersionBefore (18, 189)) g->parent = INVALID_GROUP;
	}
}

extern const ChunkHandler _group_chunk_handlers[] = {
	{ 'GRPS', Save_GRPS, Load_GRPS, NULL, NULL, CH_ARRAY | CH_LAST},
};
