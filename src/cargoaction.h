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

/** Base class for CargoMovement. */
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

/**
 * Abstract action for moving cargo from one list to another.
 * @tparam Tsource CargoList subclass to remove cargo from.
 * @tparam Tdest CargoList subclass to add cargo to.
 */
template<class Tsource, class Tdest>
class CargoMovement : protected CargoMovementAmount {
protected:
	Tsource *source;    ///< Source of the cargo.
	Tdest *destination; ///< Destination for the cargo.

public:
	CargoMovement (Tsource *source, Tdest *destination, uint max_move)
		: CargoMovementAmount (max_move),
		  source (source), destination (destination)
	{
	}

	/**
	 * Returns how much more cargo can be moved with this action.
	 * @return Amount of cargo this action can still move.
	 */
	uint MaxMove() { return this->Amount(); }
};

/** Action of loading cargo from a station onto a vehicle. */
class CargoLoad : public CargoMovement<StationCargoList, VehicleCargoList> {
protected:
	TileIndex load_place; ///< TileIndex to be saved in the packets' loaded_at_xy.
public:
	CargoLoad(StationCargoList *source, VehicleCargoList *destination, uint max_move, TileIndex load_place) :
			CargoMovement<StationCargoList, VehicleCargoList>(source, destination, max_move), load_place(load_place) {}
	bool operator()(CargoPacket *cp);
};

/** Action of reserving cargo from a station to be loaded onto a vehicle. */
class CargoReservation : public CargoLoad {
public:
	CargoReservation(StationCargoList *source, VehicleCargoList *destination, uint max_move, TileIndex load_place) :
			CargoLoad(source, destination, max_move, load_place) {}
	bool operator()(CargoPacket *cp);
};

/** Action of rerouting cargo in a station. */
class StationCargoReroute : public CargoMovement <StationCargoList, StationCargoList> {
protected:
	StationID avoid;
	StationID avoid2;
	const GoodsEntry *ge;
public:
	StationCargoReroute (StationCargoList *list, StationID avoid, StationID avoid2, const GoodsEntry *ge) :
			CargoMovement <StationCargoList, StationCargoList> (list, list, UINT_MAX), avoid(avoid), avoid2(avoid2), ge(ge) {}
	bool operator()(CargoPacket *cp);
};

#endif /* CARGOACTION_H */
