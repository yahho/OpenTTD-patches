/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport.cpp Functions related to airports. */

#include "stdafx.h"
#include "newgrf_airport.h"
#include "table/strings.h"
#include "table/airport_movement.h"
#include "table/airporttile_ids.h"


/**
 * Define a generic airport.
 * @param name Suffix of the names of the airport data.
 * @param terminals The terminals.
 * @param num_helipads Number of heli pads.
 * @param flags Information about the class of FTA.
 * @param delta_z Height of the airport above the land.
 */
#define AIRPORT_GENERIC(name, terminals, num_helipads, ...) \
	static const AirportFTA _airportfta_ ## name (_airport_fta_ ## name, terminals, \
			num_helipads, _airport_entries_ ## name, __VA_ARGS__);

/**
 * Define an airport.
 * @param name Suffix of the names of the airport data.
 * @param num_helipads Number of heli pads.
 * @param short_strip Airport has a short land/take-off strip.
 */
#define AIRPORT(name, num_helipads, short_strip) \
	AIRPORT_GENERIC(name, _airport_terminal_ ## name, num_helipads, \
		AirportFTA::ALL | (short_strip ? AirportFTA::SHORT_STRIP : \
			(AirportFTA::Flags)0), 0, _airport_depots_ ## name)

/**
 * Define a heliport.
 * @param name Suffix of the names of the helipad data.
 * @param num_helipads Number of heli pads.
 * @param delta_z Height of the airport above the land.
 */
#define HELIPORT(name, num_helipads, ...) \
	AIRPORT_GENERIC(name, NULL, num_helipads, AirportFTA::HELICOPTERS, __VA_ARGS__)

AIRPORT(country, 0, true)
AIRPORT(city, 0, false)
HELIPORT(heliport, 1, 60)
AIRPORT(metropolitan, 0, false)
AIRPORT(international, 2, false)
AIRPORT(commuter, 2, true)
HELIPORT(helidepot, 1, 0, _airport_depots_helidepot)
AIRPORT(intercontinental, 2, false)
HELIPORT(helistation, 3, 0, _airport_depots_helistation)
HELIPORT(oilrig, 1, 54)

#undef HELIPORT
#undef AIRPORT
#undef AIRPORT_GENERIC

#include "table/airport_defaults.h"

const AirportFTA AirportFTA::dummy (_airport_fta_dummy, NULL, 0,
				_airport_entries_dummy, AirportFTA::ALL, 0);
