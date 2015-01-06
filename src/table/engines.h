/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file table/engines.h Original TTD vehicle data. */

#ifndef ENGINES_H
#define ENGINES_H

#include "../stdafx.h"
#include "../engine_type.h"

static const uint ORIG_ENGINE_COUNT = 256;
extern const EngineInfo _orig_engine_info [ORIG_ENGINE_COUNT];

static const uint ORIG_RAIL_ENGINE_COUNT = 116;
extern const RailVehicleInfo _orig_rail_vehicle_info [ORIG_RAIL_ENGINE_COUNT];

static const uint ORIG_SHIP_ENGINE_COUNT = 11;
extern const ShipVehicleInfo _orig_ship_vehicle_info [ORIG_SHIP_ENGINE_COUNT];

static const uint ORIG_AIRCRAFT_ENGINE_COUNT = 41;
extern const AircraftVehicleInfo _orig_aircraft_vehicle_info [ORIG_AIRCRAFT_ENGINE_COUNT];

static const uint ORIG_ROAD_ENGINE_COUNT = 88;
extern const RoadVehicleInfo _orig_road_vehicle_info [ORIG_ROAD_ENGINE_COUNT];

assert_compile (ORIG_RAIL_ENGINE_COUNT + ORIG_SHIP_ENGINE_COUNT + ORIG_AIRCRAFT_ENGINE_COUNT + ORIG_ROAD_ENGINE_COUNT == ORIG_ENGINE_COUNT);

#endif /* ENGINES_H */
