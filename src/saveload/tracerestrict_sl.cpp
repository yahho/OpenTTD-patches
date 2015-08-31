/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tracerestrict_sl.cpp Code handling saving and loading of trace restrict programs */

#include "../stdafx.h"
#include "../tracerestrict.h"
#include "saveload.h"
#include <vector>
#include "saveload.h"

static const SaveLoad _trace_restrict_mapping_desc[] = {
  SLE_VAR(TraceRestrictMappingItem, program_id, SLE_UINT32),
  SLE_END()
};

static void Load_TRRM(LoadBuffer *reader)
{
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		TraceRestrictMappingItem &item = _tracerestrictprogram_mapping[index];
		reader->ReadObject(&item, _trace_restrict_mapping_desc);
	}
}

static void Save_TRRM(SaveDumper *dumper)
{
	for (TraceRestrictMapping::iterator iter = _tracerestrictprogram_mapping.begin();
			iter != _tracerestrictprogram_mapping.end(); ++iter) {
		dumper->WriteElementHeader(iter->first, SlCalcObjLength(&(iter->second), _trace_restrict_mapping_desc));
		dumper->WriteObject(&(iter->second), _trace_restrict_mapping_desc);
	}
}

struct TraceRestrictProgramStub {
	uint32 length;
};

static const SaveLoad _trace_restrict_program_stub_desc[] = {
	SLE_VAR(TraceRestrictProgramStub, length, SLE_UINT32),
	SLE_END()
};

static void Load_TRRP(LoadBuffer *reader)
{
	int index;
	TraceRestrictProgramStub stub;
	while ((index = reader->IterateChunk()) != -1) {
		TraceRestrictProgram *prog = new (index) TraceRestrictProgram();
		reader->ReadObject(&stub, _trace_restrict_program_stub_desc);
		prog->items.resize(stub.length);
		reader->ReadArray(&(prog->items[0]), stub.length, SLE_UINT32);
		assert(prog->Validate().Succeeded());
	}
}

static void Save_TRRP(SaveDumper *dumper)
{
	TraceRestrictProgram *prog;

	FOR_ALL_TRACE_RESTRICT_PROGRAMS(prog) {
		SaveDumper temp_dumper(1024);
		TraceRestrictProgramStub stub;
		stub.length = prog->items.size();
		temp_dumper.WriteObject(&stub, _trace_restrict_program_stub_desc);
		temp_dumper.WriteArray(&(prog->items[0]), stub.length, SLE_UINT32);
		dumper->WriteElementHeader(prog->index, temp_dumper.GetSize());
		temp_dumper.Dump(dumper);
	}
}

void AfterLoadTraceRestrict()
{
	for (TraceRestrictMapping::iterator iter = _tracerestrictprogram_mapping.begin();
			iter != _tracerestrictprogram_mapping.end(); ++iter) {
		TraceRestrictProgram::Get(iter->second.program_id)->IncrementRefCount();
	}
}

extern const ChunkHandler _trace_restrict_chunk_handlers[] = {
	{ 'TRRM', Save_TRRM, Load_TRRM, NULL, NULL, CH_SPARSE_ARRAY},
	{ 'TRRP', Save_TRRP, Load_TRRP, NULL, NULL, CH_ARRAY | CH_LAST},
};
