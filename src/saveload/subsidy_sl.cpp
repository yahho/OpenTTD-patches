/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file subsidy_sl.cpp Code handling saving and loading of subsidies */

#include "../stdafx.h"
#include "../subsidy_base.h"

#include "saveload_buffer.h"

static const SaveLoad _subsidies_desc[] = {
	    SLE_VAR(Subsidy, cargo_type, SLE_UINT8),
	    SLE_VAR(Subsidy, remaining,  SLE_UINT8),
	SLE_CONDVAR(Subsidy, awarded,    SLE_UINT8,                 125, SL_MAX_VERSION),
	SLE_CONDVAR(Subsidy, src_type,   SLE_UINT8,                 125, SL_MAX_VERSION),
	SLE_CONDVAR(Subsidy, dst_type,   SLE_UINT8,                 125, SL_MAX_VERSION),
	SLE_CONDVAR(Subsidy, src,        SLE_FILE_U8 | SLE_VAR_U16,   0, 4),
	SLE_CONDVAR(Subsidy, src,        SLE_UINT16,                  5, SL_MAX_VERSION),
	SLE_CONDVAR(Subsidy, dst,        SLE_FILE_U8 | SLE_VAR_U16,   0, 4),
	SLE_CONDVAR(Subsidy, dst,        SLE_UINT16,                  5, SL_MAX_VERSION),
	SLE_END()
};

static void Save_SUBS(SaveDumper *dumper)
{
	Subsidy *s;
	FOR_ALL_SUBSIDIES(s) {
		dumper->WriteElement(s->index, s, _subsidies_desc);
	}
}

static void Load_SUBS(LoadBuffer *reader)
{
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		Subsidy *s = new (index) Subsidy();
		reader->ReadObject(s, _subsidies_desc);
	}
}

extern const ChunkHandler _subsidy_chunk_handlers[] = {
	{ 'SUBS', Save_SUBS, Load_SUBS, NULL, NULL, CH_ARRAY | CH_LAST},
};
