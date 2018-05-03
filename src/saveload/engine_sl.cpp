/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file engine_sl.cpp Code handling saving and loading of engines */

#include "../stdafx.h"
#include "saveload_internal.h"
#include "../engine_base.h"
#include <vector>

static const SaveLoad _engine_desc[] = {
	 SLE_VAR(EngineState, intro_date,          SLE_FILE_U16 | SLE_VAR_I32,  , ,  0,  30),
	 SLE_VAR(EngineState, intro_date,          SLE_INT32,                  0, , 31,    ),
	 SLE_VAR(EngineState, age,                 SLE_FILE_U16 | SLE_VAR_I32,  , ,  0,  30),
	 SLE_VAR(EngineState, age,                 SLE_INT32,                  0, , 31,    ),
	 SLE_VAR(EngineState, reliability,         SLE_UINT16),
	 SLE_VAR(EngineState, reliability_spd_dec, SLE_UINT16),
	 SLE_VAR(EngineState, reliability_start,   SLE_UINT16),
	 SLE_VAR(EngineState, reliability_max,     SLE_UINT16),
	 SLE_VAR(EngineState, reliability_final,   SLE_UINT16),
	 SLE_VAR(EngineState, duration_phase_1,    SLE_UINT16),
	 SLE_VAR(EngineState, duration_phase_2,    SLE_UINT16),
	 SLE_VAR(EngineState, duration_phase_3,    SLE_UINT16),

	SLE_NULL(1,                                                       , ,   0, 120),
	 SLE_VAR(EngineState, flags,               SLE_UINT8),
	SLE_NULL(1,                                                       , ,   0, 178), // old preview_company_rank
	 SLE_VAR(EngineState, preview_asked,       SLE_UINT16,                0, , 179,    ),
	 SLE_VAR(EngineState, preview_company,     SLE_UINT8,                 0, , 179,    ),
	 SLE_VAR(EngineState, preview_wait,        SLE_UINT8),
	SLE_NULL(1,                                                       , ,   0,  44),
	 SLE_VAR(EngineState, company_avail,       SLE_FILE_U8  | SLE_VAR_U16, , ,   0, 103),
	 SLE_VAR(EngineState, company_avail,       SLE_UINT16,                0, , 104,    ),
	 SLE_VAR(EngineState, company_hidden,      SLE_UINT16,               21, , 193,    ),
	 SLE_STR(EngineState, name,                SLS_NONE,                  0, ,  84,    ),

	SLE_NULL(16,                                                      , ,   2, 143), // old reserved space

	SLE_END()
};

static std::vector<EngineState> _temp_engine;

EngineState *AppendTempDataEngine (void)
{
	_temp_engine.push_back (EngineState());

	return &_temp_engine.back();
}

EngineState *GetTempDataEngine (EngineID index)
{
	assert (index < _temp_engine.size());

	return &_temp_engine[index];
}

static void Save_ENGN(SaveDumper *dumper)
{
	Engine *e;
	FOR_ALL_ENGINES(e) {
		/* Cast the pointer to the right substructure. */
		dumper->WriteElement (e->index,
				static_cast<const EngineState*>(e),
				_engine_desc);
	}
}

static void Load_ENGN(LoadBuffer *reader)
{
	/* As engine data is loaded before engines are initialized we need to load
	 * this information into a temporary array. This is then copied into the
	 * engine pool after processing NewGRFs by CopyTempEngineData(). */
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		assert ((uint)index == _temp_engine.size());

		EngineState *e = AppendTempDataEngine();
		reader->ReadObject(e, _engine_desc);

		if (reader->IsOTTDVersionBefore(179)) {
			/* preview_company_rank was replaced with preview_company and preview_asked.
			 * Just cancel any previews. */
			e->flags &= ~4; // ENGINE_OFFER_WINDOW_OPEN
			e->preview_company = INVALID_COMPANY;
			e->preview_asked = (CompanyMask)-1;
			e->preview_wait = 0;
		}

		if (reader->IsVersionBefore (21, 193)) {
			e->company_hidden = 0;
		}
	}
}

/**
 * Copy data from temporary engine array into the real engine pool.
 */
void CopyTempEngineData()
{
	Engine *e;
	FOR_ALL_ENGINES(e) {
		if (e->index >= _temp_engine.size()) break;

		EngineState *se = GetTempDataEngine (e->index);
		assert (e->name == NULL);
		e->name = se->name;
		se->name = NULL;
		e->intro_date          = se->intro_date;
		e->age                 = se->age;
		e->reliability         = se->reliability;
		e->reliability_spd_dec = se->reliability_spd_dec;
		e->reliability_start   = se->reliability_start;
		e->reliability_max     = se->reliability_max;
		e->reliability_final   = se->reliability_final;
		e->duration_phase_1    = se->duration_phase_1;
		e->duration_phase_2    = se->duration_phase_2;
		e->duration_phase_3    = se->duration_phase_3;
		e->flags               = se->flags;
		e->preview_asked       = se->preview_asked;
		e->preview_company     = se->preview_company;
		e->preview_wait        = se->preview_wait;
		e->company_avail       = se->company_avail;
		e->company_hidden      = se->company_hidden;
	}

	/* Get rid of temporary data */
	_temp_engine.clear();
}

static void Load_ENGS(LoadBuffer *reader)
{
	/* Handle buggy openttd savegame version 0,
	 * where arrays were not byte-swapped. */
	bool buggy = (reader->stv->type == SGT_OTTD)
			&& (reader->stv->ottd.version == 0);

	/* Load old separate String ID list into a temporary array. This
	 * was always 256 entries. */
	for (EngineID engine = 0; engine < 256; engine++) {
		uint16 name = reader->ReadUint16();
		if (buggy) name = BSWAP16(name);
		/* Copy each string into the temporary engine array. */
		EngineState *e = GetTempDataEngine (engine);
		e->name = CopyFromOldName (reader->stv,
						RemapOldStringID (name));
	}
}

/** Save and load the mapping between the engine id in the pool, and the grf file it came from. */
static const SaveLoad _engine_id_mapping_desc[] = {
	SLE_VAR(EngineIDMapping, grfid,         SLE_UINT32),
	SLE_VAR(EngineIDMapping, internal_id,   SLE_UINT16),
	SLE_VAR(EngineIDMapping, type,          SLE_UINT8),
	SLE_VAR(EngineIDMapping, substitute_id, SLE_UINT8),
	SLE_END()
};

static void Save_EIDS(SaveDumper *dumper)
{
	const EngineIDMapping *end = _engine_mngr.End();
	uint index = 0;
	for (EngineIDMapping *eid = _engine_mngr.Begin(); eid != end; eid++, index++) {
		dumper->WriteElement(index, eid, _engine_id_mapping_desc);
	}
}

static void Load_EIDS(LoadBuffer *reader)
{
	_engine_mngr.Clear();

	while (reader->IterateChunk() != -1) {
		EngineIDMapping *eid = _engine_mngr.Append();
		reader->ReadObject(eid, _engine_id_mapping_desc);
	}
}

extern const ChunkHandler _engine_chunk_handlers[] = {
	{ 'EIDS', Save_EIDS, Load_EIDS, NULL, NULL, CH_ARRAY          },
	{ 'ENGN', Save_ENGN, Load_ENGN, NULL, NULL, CH_ARRAY          },
	{ 'ENGS', NULL,      Load_ENGS, NULL, NULL, CH_RIFF | CH_LAST },
};
