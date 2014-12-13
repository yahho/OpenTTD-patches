/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file roadstop_base.h Base class for roadstops. */

#ifndef ROADSTOP_BASE_H
#define ROADSTOP_BASE_H

#include "station_type.h"
#include "core/pool_type.hpp"
#include "core/bitmath_func.hpp"
#include "vehicle_type.h"

/** A Stop for a Road Vehicle */
struct RoadStop : PooledItem <RoadStop, RoadStopID, 32, 64000> {
	enum RoadStopStatusFlags {
		RSSFB_BAY0_FREE  = 0, ///< Non-zero when bay 0 is free
		RSSFB_BAY1_FREE  = 1, ///< Non-zero when bay 1 is free
		RSSFB_BAY_COUNT  = 2, ///< Max. number of bays
		RSSFB_BASE_ENTRY = 6, ///< Non-zero when the entries on this road stop are the primary, i.e. the ones to delete
		RSSFB_ENTRY_BUSY = 7, ///< Non-zero when roadstop entry is busy
	};

	/** Container for both entry points of a drive-through road stop. */
	struct Platform {
	private:
		uint length;        ///< The length of the stop in tile 'units'
		uint occupied_east; ///< The amount of occupied stop in tile 'units' to the east
		uint occupied_west; ///< The amount of occupied stop in tile 'units' to the west

	public:
		friend struct RoadStop; ///< Oh yeah, the road stop may play with me.

		/** Create a platform. */
		CONSTEXPR Platform()
			: length(0), occupied_east(), occupied_west()
		{
		}

		/**
		 * Get the length of this drive through stop.
		 * @return the length in tile units.
		 */
		inline uint GetLength() const
		{
			return this->length;
		}

		/**
		 * Get the amount of occupied space in a given direction.
		 * @param dir The direction to get the occupied space for.
		 * @return the occupied space in tile units.
		 */
		inline uint GetOccupied (DiagDirection dir) const
		{
			return HasBit ((int)dir, 1) ? this->occupied_west : this->occupied_east;
		}

	private:
		/**
		 * Get a pointer to the occupied field in a given direction.
		 * @param dir The direction to get the occupied pointer for.
		 * @return A pointer to the occupied field in the given direction.
		 */
		inline uint *GetOccupiedPtr (DiagDirection dir)
		{
			return HasBit ((int)dir, 1) ? &this->occupied_west : &this->occupied_east;
		}

	public:
		void Rebuild (TileIndex tile);
	};

	TileIndex       xy;     ///< Position on the map
	byte            status; ///< Current status of the Stop, @see RoadStopSatusFlag. Access using *Bay and *Busy functions.
	struct RoadStop *next;  ///< Next stop of the given type at this station

	/** Initializes a RoadStop */
	inline RoadStop(TileIndex tile = INVALID_TILE) :
		xy(tile),
		status((1 << RSSFB_BAY_COUNT) - 1)
	{ }

	~RoadStop();

	/**
	 * Checks whether there is a free bay in this road stop
	 * @return is at least one bay free?
	 */
	inline bool HasFreeBay() const
	{
		return GB(this->status, 0, RSSFB_BAY_COUNT) != 0;
	}

	/**
	 * Checks whether the given bay is free in this road stop
	 * @param nr bay to check
	 * @return is given bay free?
	 */
	inline bool IsFreeBay(uint nr) const
	{
		assert(nr < RSSFB_BAY_COUNT);
		return HasBit(this->status, nr);
	}

	/**
	 * Checks whether the entrance of the road stop is occupied by a vehicle
	 * @return is entrance busy?
	 */
	inline bool IsEntranceBusy() const
	{
		return HasBit(this->status, RSSFB_ENTRY_BUSY);
	}

	/**
	 * Makes an entrance occupied or free
	 * @param busy If true, marks busy; free otherwise.
	 */
	inline void SetEntranceBusy(bool busy)
	{
		SB(this->status, RSSFB_ENTRY_BUSY, 1, busy);
	}

	/**
	 * Get the drive through road stop platform struct.
	 * @return the platform
	 */
	inline const Platform *GetPlatform (void) const
	{
		return this->platform;
	}

	void MakeDriveThrough();
	void ClearDriveThrough();

	void LeaveStandard (RoadVehicle *rv);
	void LeaveDriveThrough (RoadVehicle *rv);
	bool EnterStandard (RoadVehicle *rv);
	void EnterDriveThrough (RoadVehicle *rv);

	static RoadStop *GetByTile(TileIndex tile, RoadStopType type);

	static bool IsDriveThroughRoadStopContinuation(TileIndex rs, TileIndex next);

	/** Rebuild the vehicles and other metadata on this stop. */
	void Rebuild (void)
	{
		assert (HasBit (this->status, RSSFB_BASE_ENTRY));

		this->platform->Rebuild (this->xy);
	}

	void CheckIntegrity (void) const;

private:
	Platform *platform; ///< Platform data for drive-through stops

	/**
	 * Allocates a bay
	 * @return the allocated bay number
	 * @pre this->HasFreeBay()
	 */
	inline uint AllocateBay()
	{
		assert(this->HasFreeBay());

		/* Find the first free bay. If the bit is set, the bay is free. */
		uint bay_nr = 0;
		while (!HasBit(this->status, bay_nr)) bay_nr++;

		ClrBit(this->status, bay_nr);
		return bay_nr;
	}

	/**
	 * Frees the given bay
	 * @param nr the number of the bay to free
	 */
	inline void FreeBay(uint nr)
	{
		assert(nr < RSSFB_BAY_COUNT);
		SetBit(this->status, nr);
	}
};

#define FOR_ALL_ROADSTOPS_FROM(var, start) FOR_ALL_ITEMS_FROM(RoadStop, roadstop_index, var, start)
#define FOR_ALL_ROADSTOPS(var) FOR_ALL_ROADSTOPS_FROM(var, 0)

#endif /* ROADSTOP_BASE_H */
