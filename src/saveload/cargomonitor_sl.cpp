/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file cargomonitor_sl.cpp Code handling saving and loading of Cargo monitoring. */

#include "../stdafx.h"
#include "../cargomonitor.h"

#include "saveload_buffer.h"

/** Temporary storage of cargo monitoring data for loading or saving it. */
struct TempStorage {
	CargoMonitorID number;
	uint32 amount;
};

/** Description of the #TempStorage structure for the purpose of load and save. */
static const SaveLoad _cargomonitor_pair_desc[] = {
	SLE_VAR(TempStorage, number, SLE_UINT32),
	SLE_VAR(TempStorage, amount, SLE_UINT32),
	SLE_END()
};

/** Save a cargo monitor map. */
static void SaveCargoMonitorMap (SaveDumper *dumper, const CargoMonitorMap *map)
{
	TempStorage storage;

	int i = 0;
	CargoMonitorMap::const_iterator iter = map->begin();
	while (iter != map->end()) {
		storage.number = iter->first;
		storage.amount = iter->second;

		dumper->WriteElement(i, &storage, _cargomonitor_pair_desc);

		i++;
		iter++;
	}
}

/** Load a cargo monitor map. */
static void LoadCargoMonitorMap (LoadBuffer *reader, CargoMonitorMap *map)
{
	TempStorage storage;

	for (;;) {
		if (reader->IterateChunk() < 0) break;
		reader->ReadObject(&storage, _cargomonitor_pair_desc);

		std::pair<CargoMonitorID, uint32> p(storage.number, storage.amount);
		map->insert(p);
	}
}

/** Save the #_cargo_deliveries monitoring map. */
static void SaveDelivery(SaveDumper *dumper)
{
	SaveCargoMonitorMap (dumper, &_cargo_deliveries);
}

/** Load the #_cargo_deliveries monitoring map. */
static void LoadDelivery(LoadBuffer *reader)
{
	ClearCargoDeliveryMonitoring();
	LoadCargoMonitorMap (reader, &_cargo_deliveries);
}

/** Save the #_cargo_pickups monitoring map. */
static void SavePickup(SaveDumper *dumper)
{
	SaveCargoMonitorMap (dumper, &_cargo_pickups);
}

/** Load the #_cargo_pickups monitoring map. */
static void LoadPickup(LoadBuffer *reader)
{
	ClearCargoPickupMonitoring();
	LoadCargoMonitorMap (reader, &_cargo_pickups);
}

/** Chunk definition of the cargomonitoring maps. */
extern const ChunkHandler _cargomonitor_chunk_handlers[] = {
	{ 'CMDL', SaveDelivery, LoadDelivery, NULL, NULL, CH_ARRAY},
	{ 'CMPU', SavePickup,   LoadPickup,   NULL, NULL, CH_ARRAY | CH_LAST},
};
