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
 * Loads some cargo onto a vehicle.
 * @param cp Packet to be loaded.
 * @return True if the packet was completely loaded, false if part of it was.
 */
bool CargoLoad::operator()(CargoPacket *cp)
{
	CargoPacket *cp_new = this->Preprocess(cp);
	if (cp_new == NULL) return false;
	cp_new->SetLoadPlace(this->load_place);
	this->source->RemoveFromCache(cp_new, cp_new->Count());
	this->destination->Append(cp_new, VehicleCargoList::MTA_KEEP);
	return cp_new == cp;
}

/**
 * Reserves some cargo for loading.
 * @param cp Packet to be reserved.
 * @return True if the packet was completely reserved, false if part of it was.
 */
bool CargoReservation::operator()(CargoPacket *cp)
{
	CargoPacket *cp_new = this->Preprocess(cp);
	if (cp_new == NULL) return false;
	cp_new->SetLoadPlace(this->load_place);
	this->source->reserved_count += cp_new->Count();
	this->source->RemoveFromCache(cp_new, cp_new->Count());
	this->destination->Append(cp_new, VehicleCargoList::MTA_LOAD);
	return cp_new == cp;
}

/**
 * Reroutes some cargo from one Station sublist to another.
 * @param cp Packet to be rerouted.
 * @return True if the packet was completely rerouted, false if part of it was.
 */
bool StationCargoReroute::operator()(CargoPacket *cp)
{
	CargoPacket *cp_new = this->Preprocess(cp);
	if (cp_new == NULL) cp_new = cp;
	StationID next = this->ge->GetVia(cp_new->SourceStation(), this->avoid, this->avoid2);
	assert(next != this->avoid && next != this->avoid2);
	if (this->source != this->destination) {
		this->source->RemoveFromCache(cp_new, cp_new->Count());
		this->destination->AddToCache(cp_new);
	}

	/* Legal, as insert doesn't invalidate iterators in the MultiMap, however
	 * this might insert the packet between range.first and range.second (which might be end())
	 * This is why we check for GetKey above to avoid infinite loops. */
	this->destination->packets.Insert(next, cp_new);
	return cp_new == cp;
}
