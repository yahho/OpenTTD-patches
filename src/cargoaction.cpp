/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file cargoaction.cpp Implementation of cargo actions. */

#include "stdafx.h"
#include "economy_base.h"
#include "cargoaction.h"
#include "station_base.h"

/**
 * Loads some cargo onto a vehicle or reserves it for loading.
 * @param cp Packet to be loaded.
 * @param load Whether to actually load the cargo, else only reserve it.
 * @return True if the packet was completely handled, false if part of it was.
 */
bool CargoLoad::Load (CargoPacket *cp, bool load)
{
	CargoPacket *cp_new = this->Preprocess(cp);
	if (cp_new == NULL) return false;
	cp_new->SetLoadPlace(this->load_place);
	this->source->RemoveFromCache(cp_new, cp_new->Count());
	VehicleCargoList::MoveToAction mta;
	if (load) {
		mta = VehicleCargoList::MTA_KEEP;
	} else {
		this->source->reserved_count += cp_new->Count();
		mta = VehicleCargoList::MTA_LOAD;
	}
	this->destination->Append (cp_new, mta);
	return cp_new == cp;
}
