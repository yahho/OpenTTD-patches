/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file storage_sl.cpp Code handling saving and loading of persistent storages. */

#include "../stdafx.h"
#include "../newgrf_storage.h"
#include "saveload_buffer.h"

/** Description of the data to save and load in #PersistentStorage. */
static const SaveLoad _storage_desc[] = {
	 SLE_VAR(PersistentStorage, grfid,   SLE_UINT32,     0, ,   6, ),
	 SLE_ARR(PersistentStorage, storage, SLE_UINT32, 16, 0, , 161, ),
	 SLE_END()
};

/** Load persistent storage data. */
static void Load_PSAC(LoadBuffer *reader)
{
	int index;

	while ((index = reader->IterateChunk()) != -1) {
		assert(PersistentStorage::CanAllocateItem());
		PersistentStorage *ps = new (index) PersistentStorage(0, 0, 0);
		reader->ReadObject(ps, _storage_desc);
	}
}

/** Save persistent storage data. */
static void Save_PSAC(SaveDumper *dumper)
{
	PersistentStorage *ps;

	/* Write the industries */
	FOR_ALL_STORAGES(ps) {
		dumper->WriteElement(ps->index, ps, _storage_desc);
	}
}

/** Chunk handler for persistent storages. */
extern const ChunkHandler _persistent_storage_chunk_handlers[] = {
	{ 'PSAC', Save_PSAC, Load_PSAC, NULL, NULL, CH_ARRAY | CH_LAST},
};
