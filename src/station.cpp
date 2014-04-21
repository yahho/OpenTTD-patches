/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file station.cpp Implementation of the station base class. */

#include "stdafx.h"
#include "company_func.h"
#include "company_base.h"
#include "roadveh.h"
#include "viewport_func.h"
#include "date_func.h"
#include "command_func.h"
#include "news_func.h"
#include "aircraft.h"
#include "vehiclelist.h"
#include "core/pool_func.hpp"
#include "station_base.h"
#include "roadstop_base.h"
#include "industry.h"
#include "core/random_func.hpp"
#include "linkgraph/linkgraph.h"
#include "linkgraph/linkgraphschedule.h"
#include "map/road.h"

#include "table/strings.h"

/** The pool of stations. */
template<> BaseStation::Pool BaseStation::PoolItem::pool ("Station");
INSTANTIATE_POOL_METHODS(Station)

typedef StationIDStack::SmallStackPool StationIDStackPool;
template<> StationIDStackPool StationIDStack::_pool = StationIDStackPool();

BaseStation::~BaseStation()
{
	free(this->name);
	free(this->speclist);

	if (CleaningPool()) return;

	Owner owner = this->owner;
	if (!Company::IsValidID(owner)) owner = _local_company;
	if (!Company::IsValidID(owner)) return; // Spectators
	DeleteWindowById(WC_TRAINS_LIST,   VehicleListIdentifier(VL_STATION_LIST, VEH_TRAIN,    owner, this->index).Pack());
	DeleteWindowById(WC_ROADVEH_LIST,  VehicleListIdentifier(VL_STATION_LIST, VEH_ROAD,     owner, this->index).Pack());
	DeleteWindowById(WC_SHIPS_LIST,    VehicleListIdentifier(VL_STATION_LIST, VEH_SHIP,     owner, this->index).Pack());
	DeleteWindowById(WC_AIRCRAFT_LIST, VehicleListIdentifier(VL_STATION_LIST, VEH_AIRCRAFT, owner, this->index).Pack());

	this->sign.MarkDirty();
}

Station::Station(TileIndex tile) :
	SpecializedStation<Station, false>(tile),
	bus_station(INVALID_TILE, 0, 0),
	truck_station(INVALID_TILE, 0, 0),
	dock_area(INVALID_TILE, 0, 0),
	indtype(IT_INVALID),
	time_since_load(255),
	time_since_unload(255),
	last_vehicle_type(VEH_INVALID)
{
	/* this->random_bits is set in Station::AddFacility() */
}

/**
 * Clean up a station by clearing vehicle orders, invalidating windows and
 * removing link stats.
 * Aircraft-Hangar orders need special treatment here, as the hangars are
 * actually part of a station (tiletype is STATION), but the order type
 * is OT_GOTO_DEPOT.
 */
Station::~Station()
{
	if (CleaningPool()) {
		for (CargoID c = 0; c < NUM_CARGO; c++) {
			this->goods[c].cargo.OnCleanPool();
		}
		return;
	}

	while (!this->loading_vehicles.empty()) {
		this->loading_vehicles.front()->LeaveStation();
	}

	Aircraft *a;
	FOR_ALL_AIRCRAFT(a) {
		if (!a->IsNormalAircraft()) continue;
		if (a->targetairport == this->index) a->targetairport = INVALID_STATION;
	}

	for (CargoID c = 0; c < NUM_CARGO; ++c) {
		LinkGraph *lg = LinkGraph::GetIfValid(this->goods[c].link_graph);
		if (lg == NULL) continue;

		for (NodeID node = 0; node < lg->Size(); ++node) {
			Station *st = Station::Get((*lg)[node].Station());
			st->goods[c].flows.erase(this->index);
			if ((*lg)[node][this->goods[c].node].LastUpdate() != INVALID_DATE) {
				st->goods[c].flows.DeleteFlows(this->index);
				RerouteCargo(st, c, this->index, st->index);
			}
		}
		lg->RemoveNode(this->goods[c].node);
		if (lg->Size() == 0) {
			LinkGraphSchedule::Instance()->Unqueue(lg);
			delete lg;
		}
	}

	Vehicle *v;
	FOR_ALL_VEHICLES(v) {
		/* Forget about this station if this station is removed */
		if (v->last_station_visited == this->index) {
			v->last_station_visited = INVALID_STATION;
		}
		if (v->last_loading_station == this->index) {
			v->last_loading_station = INVALID_STATION;
		}
	}

	/* Clear the persistent storage. */
	delete this->airport.psa;

	if (this->owner == OWNER_NONE) {
		/* Invalidate all in case of oil rigs. */
		InvalidateWindowClassesData(WC_STATION_LIST, 0);
	} else {
		InvalidateWindowData(WC_STATION_LIST, this->owner, 0);
	}

	DeleteWindowById(WC_STATION_VIEW, index);

	/* Now delete all orders that go to the station */
	RemoveOrderFromAllVehicles(OT_GOTO_STATION, this->index);

	/* Remove all news items */
	DeleteStationNews(this->index);

	for (CargoID c = 0; c < NUM_CARGO; c++) {
		this->goods[c].cargo.Truncate();
	}

	CargoPacket::InvalidateAllFrom(this->index);
}


/**
 * Invalidating of the JoinStation window has to be done
 * after removing item from the pool.
 * @param index index of deleted item
 */
void BaseStation::PostDestructor(size_t index)
{
	InvalidateWindowData(WC_SELECT_STATION, 0, 0);
}

/**
 * Get the primary road stop (the first road stop) that the given vehicle can load/unload.
 * @param v the vehicle to get the first road stop for
 * @return the first roadstop that this vehicle can load at
 */
RoadStop *Station::GetPrimaryRoadStop(const RoadVehicle *v) const
{
	RoadStop *rs = this->GetPrimaryRoadStop(v->IsBus() ? ROADSTOP_BUS : ROADSTOP_TRUCK);

	for (; rs != NULL; rs = rs->next) {
		/* The vehicle cannot go to this roadstop (different roadtype) */
		if ((GetRoadTypes(rs->xy) & v->compatible_roadtypes) == ROADTYPES_NONE) continue;
		/* The vehicle is articulated and can therefore not go to a standard road stop. */
		if (IsStandardRoadStopTile(rs->xy) && v->HasArticulatedPart()) continue;

		/* The vehicle can actually go to this road stop. So, return it! */
		break;
	}

	return rs;
}

/**
 * Called when new facility is built on the station. If it is the first facility
 * it initializes also 'xy' and 'random_bits' members
 */
void Station::AddFacility(StationFacility new_facility_bit, TileIndex facil_xy)
{
	if (this->facilities == FACIL_NONE) {
		this->xy = facil_xy;
		this->random_bits = Random();
	}
	this->facilities |= new_facility_bit;
	this->owner = _current_company;
	this->build_date = _date;
}

/**
 * Marks the tiles of the station as dirty.
 *
 * @ingroup dirty
 */
void Station::MarkTilesDirty(bool cargo_change) const
{
	TileIndex tile = this->train_station.tile;
	int w, h;

	if (tile == INVALID_TILE) return;

	/* cargo_change is set if we're refreshing the tiles due to cargo moving
	 * around. */
	if (cargo_change) {
		/* Don't waste time updating if there are no custom station graphics
		 * that might change. Even if there are custom graphics, they might
		 * not change. Unfortunately we have no way of telling. */
		if (this->num_specs == 0) return;
	}

	for (h = 0; h < train_station.h; h++) {
		for (w = 0; w < train_station.w; w++) {
			if (this->TileBelongsToRailStation(tile)) {
				MarkTileDirtyByTile(tile);
			}
			tile += TileDiffXY(1, 0);
		}
		tile += TileDiffXY(-w, 1);
	}
}

/* virtual */ uint Station::GetPlatformLength(TileIndex tile) const
{
	assert(this->TileBelongsToRailStation(tile));

	TileIndexDiff delta = (GetRailStationAxis(tile) == AXIS_X ? TileDiffXY(1, 0) : TileDiffXY(0, 1));

	TileIndex t = tile;
	uint len = 0;
	do {
		t -= delta;
		len++;
	} while (IsCompatibleTrainStationTile(t, tile));

	t = tile;
	do {
		t += delta;
		len++;
	} while (IsCompatibleTrainStationTile(t, tile));

	return len - 1;
}

/* virtual */ uint Station::GetPlatformLength(TileIndex tile, DiagDirection dir) const
{
	TileIndex start_tile = tile;
	uint length = 0;
	assert(IsRailStationTile(tile));
	assert(dir < DIAGDIR_END);

	do {
		length++;
		tile += TileOffsByDiagDir(dir);
	} while (IsCompatibleTrainStationTile(tile, start_tile));

	return length;
}

/**
 * Determines the catchment radius of the station
 * @return The radius
 */
uint Station::GetCatchmentRadius() const
{
	uint ret = CA_NONE;

	if (_settings_game.station.modified_catchment) {
		if (this->bus_stops          != NULL)         ret = max<uint>(ret, CA_BUS);
		if (this->truck_stops        != NULL)         ret = max<uint>(ret, CA_TRUCK);
		if (this->train_station.tile != INVALID_TILE) ret = max<uint>(ret, CA_TRAIN);
		if (this->docks              != NULL)         ret = max<uint>(ret, CA_DOCK);
		if (this->airport.tile       != INVALID_TILE) ret = max<uint>(ret, this->airport.GetSpec()->catchment);
	} else {
		if (this->bus_stops != NULL || this->truck_stops != NULL || this->train_station.tile != INVALID_TILE || this->docks != NULL || this->airport.tile != INVALID_TILE) {
			ret = CA_UNMODIFIED;
		}
	}

	return ret;
}

/**
 * Determines the catchment area of this station
 * @return clamped catchment rectangle
 */
TileArea Station::GetCatchmentArea() const
{
	assert(!this->rect.empty());

	TileArea catchment (this->rect);
	catchment.expand (this->GetCatchmentRadius());
	return catchment;
}

/** Tile area and pointer to IndustryVector */
struct TileAreaAndIndustryVector {
	TileArea area;                   ///< The rectangle to search the industries in.
	IndustryVector *industries_near; ///< The nearby industries.
};

/**
 * Callback function for Station::RecomputeIndustriesNear()
 * Tests whether tile is an industry and possibly adds
 * the industry to station's industries_near list.
 * @param ind_tile tile to check
 * @param user_data pointer to RectAndIndustryVector
 * @return always false, we want to search all tiles
 */
static bool FindIndustryToDeliver(TileIndex ind_tile, void *user_data)
{
	/* Only process industry tiles */
	if (!IsIndustryTile(ind_tile)) return false;

	TileAreaAndIndustryVector *riv = (TileAreaAndIndustryVector *)user_data;
	Industry *ind = Industry::GetByTile(ind_tile);

	/* Don't check further if this industry is already in the list */
	if (riv->industries_near->Contains(ind)) return false;

	/* Only process tiles in the station acceptance rectangle */
	if (!riv->area.Contains(ind_tile)) return false;

	/* Include only industries that can accept cargo */
	uint cargo_index;
	for (cargo_index = 0; cargo_index < lengthof(ind->accepts_cargo); cargo_index++) {
		if (ind->accepts_cargo[cargo_index] != CT_INVALID) break;
	}
	if (cargo_index >= lengthof(ind->accepts_cargo)) return false;

	*riv->industries_near->Append() = ind;

	return false;
}

/**
 * Recomputes Station::industries_near, list of industries possibly
 * accepting cargo in station's catchment radius
 */
void Station::RecomputeIndustriesNear()
{
	this->industries_near.Clear();
	if (this->rect.empty()) return;

	TileAreaAndIndustryVector riv = {
		this->GetCatchmentArea(),
		&this->industries_near
	};

	/* Compute maximum extent of acceptance rectangle wrt. station sign */
	TileIndex start_tile = this->xy;
	uint max_radius = riv.area.get_radius_max (start_tile);

	CircularTileSearch(&start_tile, 2 * max_radius + 1, &FindIndustryToDeliver, &riv);
}

/**
 * Recomputes Station::industries_near for all stations
 */
/* static */ void Station::RecomputeIndustriesNearForAll()
{
	Station *st;
	FOR_ALL_STATIONS(st) st->RecomputeIndustriesNear();
}

/************************************************************************/
/*                     StationRect implementation                       */
/************************************************************************/

CommandCost StationRect::BeforeAddTile(TileIndex tile, StationRectMode mode)
{
	if (this->empty()) {
		/* we are adding the first station tile */
	} else if (!this->Contains(tile)) {
		/* current rect is not empty and new point is outside this rect
		 * make new spread-out rectangle */
		StationRect new_rect (*this);
		new_rect.Add(tile);

		/* check new rect dimensions against preset max */
		if (new_rect.w > _settings_game.station.station_spread || new_rect.h > _settings_game.station.station_spread) {
			return_cmd_error(STR_ERROR_STATION_TOO_SPREAD_OUT);
		}

		/* spread-out ok, return true */
	} else {
		; // new point is inside the rect, we don't need to do anything
	}
	return CommandCost();
}

CommandCost StationRect::BeforeAddRect(TileIndex tile, int w, int h, StationRectMode mode)
{
	if (w <= _settings_game.station.station_spread && h <= _settings_game.station.station_spread) {
		/* Important when the old rect is completely inside the new rect, resp. the old one was empty. */
		CommandCost ret = this->BeforeAddTile(tile, mode);
		if (ret.Succeeded()) ret = this->BeforeAddTile(TILE_ADDXY(tile, w - 1, h - 1), mode);
		return ret;
	}
	return CommandCost();
}

/** Scan a tile row/column for station tiles. */
static bool ScanForStationTiles (StationID st, TileIndex tile, uint diff, uint n)
{
	while (n > 0) {
		if (IsStationTile(tile) && GetStationIndex(tile) == st) return true;
		tile += diff;
		n--;
	}

	return false;
}

/** Shrink station area after removal of a rectangle. */
void StationRect::AfterRemoveTiles (BaseStation *st, TileIndex tile1, TileIndex tile2)
{
	assert (TileX(tile1) <= TileX(tile2));
	assert (TileY(tile1) <= TileY(tile2));

	assert (this->Contains(tile1));
	assert (this->Contains(tile2));

	const TileIndexDiff diff_x = TileDiffXY (1, 0); // rightwards
	const TileIndexDiff diff_y = TileDiffXY (0, 1); // upwards

	if (TileX(tile1) == TileX(this->tile)) {
		/* scan initial columns for station tiles */
		for (;;) {
			if (ScanForStationTiles (st->index, this->tile, diff_y, this->h)) break;
			this->tile += diff_x;
			this->w--;
			if (this->w == 0) {
				this->Clear();
				return;
			}
		}
	}

	if (TileX(tile2) == TileX(this->tile) + this->w - 1) {
		/* scan final columns for station tiles */
		TileIndex t = this->tile + (this->w - 1) * diff_x;
		for (;;) {
			if (ScanForStationTiles (st->index, t, diff_y, this->h)) break;
			t -= diff_x;
			this->w--;
			if (this->w == 0) {
				this->Clear();
				return;
			}
		}
	}

	if (TileY(tile1) == TileY(this->tile)) {
		/* scan initial rows for station tiles */
		for (;;) {
			if (ScanForStationTiles (st->index, this->tile, diff_x, this->w)) break;
			this->tile += diff_y;
			this->h--;
			if (this->h == 0) {
				this->Clear();
				return;
			}
		}
	}

	if (TileY(tile2) == TileY(this->tile) + this->h - 1) {
		/* scan final rows for station tiles */
		TileIndex t = this->tile + (this->h - 1) * diff_y;
		for (;;) {
			if (ScanForStationTiles (st->index, t, diff_x, this->w)) break;
			t -= diff_y;
			this->h--;
			if (this->h == 0) {
				this->Clear();
				return;
			}
		}
	}
}

/** The pool of docks. */
template<> Dock::Pool Dock::PoolItem::pool ("Dock");
INSTANTIATE_POOL_METHODS(Dock)

/**
 * Calculates the maintenance cost of all airports of a company.
 * @param owner Company.
 * @return Total cost.
 */
Money AirportMaintenanceCost(Owner owner)
{
	Money total_cost = 0;

	const Station *st;
	FOR_ALL_STATIONS(st) {
		if (st->owner == owner && (st->facilities & FACIL_AIRPORT)) {
			total_cost += _price[PR_INFRASTRUCTURE_AIRPORT] * st->airport.GetSpec()->maintenance_cost;
		}
	}
	/* 3 bits fraction for the maintenance cost factor. */
	return total_cost >> 3;
}
