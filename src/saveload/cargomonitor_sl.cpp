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

/** Save a cargo monitor map. */
static void SaveCargoMonitorMap (SaveDumper *dumper, const CargoMonitorMap *map)
{
	int i = 0;
	CargoMonitorMap::const_iterator iter = map->begin();
	while (iter != map->end()) {
		dumper->WriteElementHeader (i, 2 * sizeof(uint32));
		dumper->WriteUint32 (iter->first);
		dumper->WriteUint32 (iter->second);

		i++;
		iter++;
	}
}

/** Load a cargo monitor map. */
static void LoadCargoMonitorMap (LoadBuffer *reader, CargoMonitorMap *map)
{
	for (;;) {
		if (reader->IterateChunk() < 0) break;
		CargoMonitorID number = reader->ReadUint32();
		uint32 amount = reader->ReadUint32();
		std::pair<CargoMonitorID, uint32> p (number, amount);
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
