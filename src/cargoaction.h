/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file cargoaction.h Actions to be applied to cargo packets. */

#ifndef CARGOACTION_H
#define CARGOACTION_H

#include "cargopacket.h"

/** Cargo movement amount tracking class. */
class CargoMovementAmount {
private:
	uint amount;    ///< Amount of cargo still unprocessed.

public:
	CargoMovementAmount (uint amount) : amount (amount)
	{
	}

	/** Get the amount of cargo still unprocessed. */
	uint Amount (void) const
	{
		return this->amount;
	}

	/**
	 * Decides if a packet needs to be split.
	 * @param cp Packet to be either split or moved in one piece.
	 * @return Either new packet if splitting was necessary or the given
	 *         one otherwise.
	 */
	CargoPacket *Preprocess (CargoPacket *cp)
	{
		if (this->amount < cp->Count()) {
			cp = cp->Split (this->amount);
			this->amount = 0;
		} else {
			this->amount -= cp->Count();
		}
		return cp;
	}
};

/** Action of loading cargo from a station onto a vehicle. */
class CargoLoad : protected CargoMovementAmount {
protected:
	StationCargoList *source;      ///< Source of the cargo.
	VehicleCargoList *destination; ///< Destination for the cargo.
	TileIndex load_place; ///< TileIndex to be saved in the packets' loaded_at_xy.
public:
	CargoLoad(StationCargoList *source, VehicleCargoList *destination, uint max_move, TileIndex load_place) :
		CargoMovementAmount (max_move), source (source),
		destination (destination), load_place (load_place)
	{
	}

	/**
	 * Returns how much more cargo can be moved with this action.
	 * @return Amount of cargo this action can still move.
	 */
	uint MaxMove() { return this->Amount(); }

	bool Load (CargoPacket *cp, bool load);
};

#endif /* CARGOACTION_H */
