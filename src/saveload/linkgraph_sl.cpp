/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file linkgraph_sl.cpp Code handling saving and loading of link graphs */

#include "../stdafx.h"
#include "../linkgraph/linkgraph.h"
#include "../linkgraph/linkgraphjob.h"
#include "../linkgraph/linkgraphschedule.h"
#include "../settings_internal.h"
#include "saveload_buffer.h"
#include "saveload_error.h"

typedef LinkGraph::BaseNode Node;
typedef LinkGraph::BaseEdge Edge;

const SettingDesc *GetSettingDescription(uint index);

static uint16 _num_nodes;

/**
 * Get a SaveLoad array for a link graph.
 * @return SaveLoad array for link graph.
 */
const SaveLoad *GetLinkGraphDesc()
{
	static const SaveLoad link_graph_desc[] = {
		 SLE_VAR(LinkGraph, last_compression, SLE_INT32),
		SLEG_VAR(_num_nodes,                  SLE_UINT16),
		 SLE_VAR(LinkGraph, cargo,            SLE_UINT8),
		 SLE_END()
	};
	return link_graph_desc;
}

/**
 * Get a SaveLoad array for a link graph job. The settings struct is derived from
 * the global settings saveload array. The exact entries are calculated when the function
 * is called the first time.
 * It's necessary to keep a copy of the settings for each link graph job so that you can
 * change the settings while in-game and still not mess with current link graph runs.
 * Of course the settings have to be saved and loaded, too, to avoid desyncs.
 * @return Array of SaveLoad structs.
 */
const SaveLoad *GetLinkGraphJobDesc()
{
	static SmallVector<SaveLoad, 16> saveloads;
	static const char prefix[] = "linkgraph.";

	/* Build the SaveLoad array on first call and don't touch it later on */
	if (saveloads.Length() == 0) {
		static const size_t offset_gamesettings = cpp_offsetof(GameSettings, linkgraph);
		static const size_t offset_component = cpp_offsetof(LinkGraphJob, settings);

		static const size_t prefixlen = sizeof(prefix) - 1;

		int setting = 0;
		const SettingDesc *desc = GetSettingDescription(setting);
		while (desc->save.type != SL_END) {
			if (desc->desc.name != NULL && strncmp(desc->desc.name, prefix, prefixlen) == 0) {
				SaveLoad sl = desc->save;
				char *&address = reinterpret_cast<char *&>(sl.address);
				address -= offset_gamesettings;
				address += offset_component;
				*(saveloads.Append()) = sl;
			}
			desc = GetSettingDescription(++setting);
		}

		static const SaveLoad job_desc[] = {
			SLE_VAR(LinkGraphJob, join_date,        SLE_INT32),
			SLE_VAR(LinkGraphJob, link_graph.index, SLE_UINT16),
			SLE_END()
		};

		for (uint i = 0; i < lengthof(job_desc); i++) {
			*(saveloads.Append()) = job_desc[i];
		}
	}

	return &saveloads[0];
}

/**
 * Get a SaveLoad array for the link graph schedule.
 * @return SaveLoad array for the link graph schedule.
 */
const SaveLoad *GetLinkGraphScheduleDesc()
{
	static const SaveLoad schedule_desc[] = {
		SLE_LST(LinkGraphSchedule, schedule, REF_LINK_GRAPH),
		SLE_LST(LinkGraphSchedule, running,  REF_LINK_GRAPH_JOB),
		SLE_END()
	};
	return schedule_desc;
}

/* Edges and nodes are saved in the correct order, so we don't need to save their IDs. */

/**
 * SaveLoad desc for a link graph node.
 */
static const SaveLoad _node_desc[] = {
	SLE_VAR(Node, supply,      SLE_UINT32),
	SLE_VAR(Node, demand,      SLE_UINT32),
	SLE_VAR(Node, station,     SLE_UINT16),
	SLE_VAR(Node, last_update, SLE_INT32),
	SLE_END()
};

/**
 * SaveLoad desc for a link graph edge.
 */
static const SaveLoad _edge_desc[] = {
	SLE_VAR(Edge, distance,                 SLE_UINT32),
	SLE_VAR(Edge, capacity,                 SLE_UINT32),
	SLE_VAR(Edge, usage,                    SLE_UINT32),
	SLE_VAR(Edge, last_unrestricted_update, SLE_INT32),
	SLE_VAR(Edge, last_restricted_update,   SLE_INT32,  13, , 187, ),
	SLE_VAR(Edge, next_edge,                SLE_UINT16),
	SLE_END()
};

/**
 * Save a link graph.
 * @param comp Link graph to be saved.
 */
void Save_LinkGraph(SaveDumper *dumper, const LinkGraph &lg)
{
	uint size = lg.Size();
	for (NodeID from = 0; from < size; ++from) {
		const Node *node = &lg.nodes[from];
		dumper->WriteObject(node, _node_desc);
		for (NodeID to = 0; to < size; ++to) {
			dumper->WriteObject(&lg.edges[from][to], _edge_desc);
		}
	}
}

/**
 * Load a link graph.
 * @param reader Reader to load the link graph from.
 * @param comp Link graph to be loaded.
 */
void Load_LinkGraph(LoadBuffer *reader, LinkGraph &lg)
{
	uint size = lg.Size();
	for (NodeID from = 0; from < size; ++from) {
		Node *node = &lg.nodes[from];
		reader->ReadObject(node, _node_desc);
		for (NodeID to = 0; to < size; ++to) {
			reader->ReadObject(&lg.edges[from][to], _edge_desc);
		}
	}
}

/**
 * Load all link graphs.
 */
static void Load_LGRP(LoadBuffer *reader)
{
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		if (!LinkGraph::CanAllocateItem()) {
			/* Impossible as they have been present in previous game. */
			throw SlCorrupt("Too many link graphs");
		}
		LinkGraph *lg = new (index) LinkGraph();
		reader->ReadObject(lg, GetLinkGraphDesc());
		lg->Init(_num_nodes);
		Load_LinkGraph(reader, *lg);
	}
}

/**
 * Load all link graph jobs.
 */
static void Load_LGRJ(LoadBuffer *reader)
{
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		if (!LinkGraphJob::CanAllocateItem()) {
			/* Impossible as they have been present in previous game. */
			throw SlCorrupt("Too many link graph jobs");
		}
		LinkGraphJob *lgj = new (index) LinkGraphJob();
		reader->ReadObject(lgj, GetLinkGraphJobDesc());
		LinkGraph &lg = const_cast<LinkGraph &>(lgj->Graph());
		reader->ReadObject(&lg, GetLinkGraphDesc());
		lg.Init(_num_nodes);
		Load_LinkGraph(reader, lg);
	}
}

/**
 * Load the link graph schedule.
 */
static void Load_LGRS(LoadBuffer *reader)
{
	reader->ReadObject(LinkGraphSchedule::Instance(), GetLinkGraphScheduleDesc());
}

/**
 * Spawn the threads for running link graph calculations.
 * Has to be done after loading as the cargo classes might have changed.
 */
void AfterLoadLinkGraphs()
{
	LinkGraphSchedule::Instance()->SpawnAll();
}

/**
 * Save all link graphs.
 */
static void Save_LGRP(SaveDumper *dumper)
{
	LinkGraph *lg;
	FOR_ALL_LINK_GRAPHS(lg) {
		SaveDumper temp(1024);

		_num_nodes = lg->Size();
		temp.WriteObject(lg, GetLinkGraphDesc());
		Save_LinkGraph(&temp, *lg);

		dumper->WriteElementHeader(lg->index, temp.GetSize());
		temp.Dump(dumper);
	}
}

/**
 * Save all link graph jobs.
 */
static void Save_LGRJ(SaveDumper *dumper)
{
	LinkGraphJob *lgj;
	FOR_ALL_LINK_GRAPH_JOBS(lgj) {
		SaveDumper temp(1024);

		temp.WriteObject(lgj, GetLinkGraphJobDesc());
		_num_nodes = lgj->Size();
		temp.WriteObject(&lgj->Graph(), GetLinkGraphDesc());
		Save_LinkGraph(&temp, lgj->Graph());

		dumper->WriteElementHeader(lgj->index, temp.GetSize());
		temp.Dump(dumper);
	}
}

/**
 * Save the link graph schedule.
 */
static void Save_LGRS(SaveDumper *dumper)
{
	dumper->WriteRIFFObject(LinkGraphSchedule::Instance(), GetLinkGraphScheduleDesc());
}

/**
 * Substitute pointers in link graph schedule.
 */
static void Ptrs_LGRS(const SavegameTypeVersion *stv)
{
	SlObjectPtrs(LinkGraphSchedule::Instance(), GetLinkGraphScheduleDesc(), stv);
}

extern const ChunkHandler _linkgraph_chunk_handlers[] = {
	{ 'LGRP', Save_LGRP, Load_LGRP, NULL,      NULL, CH_ARRAY },
	{ 'LGRJ', Save_LGRJ, Load_LGRJ, NULL,      NULL, CH_ARRAY },
	{ 'LGRS', Save_LGRS, Load_LGRS, Ptrs_LGRS, NULL, CH_LAST  }
};
