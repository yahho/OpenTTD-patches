/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file vehicle_func.h Functions related to vehicles. */

#ifndef VEHICLE_FUNC_H
#define VEHICLE_FUNC_H

#include "gfx_func.h"
#include "direction_type.h"
#include "command_type.h"
#include "vehicle_type.h"
#include "engine_type.h"
#include "transport_type.h"
#include "newgrf_config.h"
#include "track_type.h"
#include "livery.h"
#include "map/subcoord.h"

#include "table/strings.h"

#define is_custom_sprite(x) (x >= 0xFD)
#define IS_CUSTOM_FIRSTHEAD_SPRITE(x) (x == 0xFD)
#define IS_CUSTOM_SECONDHEAD_SPRITE(x) (x == 0xFE)

static const int VEHICLE_PROFIT_MIN_AGE = DAYS_IN_YEAR * 2; ///< Only vehicles older than this have a meaningful profit.
static const Money VEHICLE_PROFIT_THRESHOLD = 10000;        ///< Threshold for a vehicle to be considered making good profit.

/**
 * Helper to check whether an image index is valid for a particular vehicle.
 * @param <T> The type of vehicle.
 * @param image_index The image index to check.
 * @return True iff the image index is valid.
 */
template <VehicleType T>
bool IsValidImageIndex(uint8 image_index);

typedef Vehicle *VehicleFromPosProc(Vehicle *v, void *data);

void VehicleServiceInDepot(Vehicle *v);
uint CountVehiclesInChain(const Vehicle *v);
void FindVehicleOnPosXY(int x, int y, void *data, VehicleFromPosProc *proc);
bool HasVehicleOnPosXY(int x, int y, void *data, VehicleFromPosProc *proc);
void CallVehicleTicks();
uint8 CalcPercentVehicleFilled(const Vehicle *v, StringID *colour);

void VehicleLengthChanged(const Vehicle *u);

byte VehicleRandomBits();
void ResetVehicleHash();
void ResetVehicleColourMap();

byte GetBestFittingSubType(Vehicle *v_from, Vehicle *v_for, CargoID dest_cargo_type);

void ViewportAddVehicles (struct ViewportDrawer *vd, const DrawPixelInfo *dpi);

void ShowNewGrfVehicleError(EngineID engine, StringID part1, StringID part2, GRFBugs bug_type, bool critical);
CommandCost TunnelBridgeIsFree(TileIndex tile, TileIndex endtile, const Vehicle *ignore = NULL);

void DecreaseVehicleValue(Vehicle *v);
void CheckVehicleBreakdown(Vehicle *v);
void AgeVehicle(Vehicle *v);
void VehicleEnteredDepotThisTick(Vehicle *v);

UnitID GetFreeUnitNumber(VehicleType type);

void VehicleEnterDepot(Vehicle *v);

bool CanBuildVehicleInfrastructure(VehicleType type);

FullPosTile GetNewVehiclePos(const Vehicle *v);
Direction GetDirectionTowards(const Vehicle *v, int x, int y);

/**
 * Is the given vehicle type buildable by a company?
 * @param type Vehicle type being queried.
 * @return Vehicle type is buildable by a company.
 */
static inline bool IsCompanyBuildableVehicleType(VehicleType type)
{
	switch (type) {
		case VEH_TRAIN:
		case VEH_ROAD:
		case VEH_SHIP:
		case VEH_AIRCRAFT:
			return true;

		default: return false;
	}
}

/**
 * Is the given vehicle buildable by a company?
 * @param v Vehicle being queried.
 * @return Vehicle is buildable by a company.
 */
static inline bool IsCompanyBuildableVehicleType(const BaseVehicle *v)
{
	return IsCompanyBuildableVehicleType(v->type);
}

LiveryScheme GetEngineLiveryScheme(EngineID engine_type, EngineID parent_engine_type, const Vehicle *v);
const struct Livery *GetEngineLivery(EngineID engine_type, CompanyID company, EngineID parent_engine_type, const Vehicle *v, byte livery_setting);

SpriteID GetEnginePalette(EngineID engine_type, CompanyID company);
SpriteID GetVehiclePalette(const Vehicle *v);

/* Functions to find the right error string for certain vehicle type */
static inline StringID GetErrBuildVeh(VehicleType type)
{
	assert_compile (STR_ERROR_CAN_T_BUY_TRAIN + VEH_TRAIN    == STR_ERROR_CAN_T_BUY_TRAIN);
	assert_compile (STR_ERROR_CAN_T_BUY_TRAIN + VEH_ROAD     == STR_ERROR_CAN_T_BUY_ROAD_VEHICLE);
	assert_compile (STR_ERROR_CAN_T_BUY_TRAIN + VEH_SHIP     == STR_ERROR_CAN_T_BUY_SHIP);
	assert_compile (STR_ERROR_CAN_T_BUY_TRAIN + VEH_AIRCRAFT == STR_ERROR_CAN_T_BUY_AIRCRAFT);

	return STR_ERROR_CAN_T_BUY_TRAIN + type;
}

static inline StringID GetErrBuildVeh(const BaseVehicle *v)
{
	return GetErrBuildVeh(v->type);
}

static inline StringID GetErrSellVeh(VehicleType type)
{
	extern const StringID _veh_sell_error_table[];
	return _veh_sell_error_table[type];
}

static inline StringID GetErrSellVeh(const BaseVehicle *v)
{
	return GetErrSellVeh(v->type);
}

static inline StringID GetErrRefitVeh(VehicleType type)
{
	extern const StringID _veh_refit_error_table[];
	return _veh_refit_error_table[type];
}

static inline StringID GetErrRefitVeh(const BaseVehicle *v)
{
	return GetErrRefitVeh(v->type);
}

static inline StringID GetErrSendToDepot(VehicleType type)
{
	extern const StringID _send_to_depot_error_table[];
	return _send_to_depot_error_table[type];
}

static inline StringID GetErrSendToDepot(const BaseVehicle *v)
{
	return GetErrSendToDepot(v->type);
}

StringID CheckVehicleOnGround (TileIndex tile);

bool CheckTrackBitsFree (TileIndex tile, TrackBits track_bits);
bool CheckBridgeEndTrackBitsFree (TileIndex tile, TrackBits bits);
bool CheckTunnelBridgeMiddleFree (TileIndex tile1, TileIndex tile2);

extern VehicleID _new_vehicle_id;
extern uint16 _returned_refit_capacity;
extern uint16 _returned_mail_refit_capacity;

bool CanVehicleUseStation(EngineID engine_type, const struct Station *st);
bool CanVehicleUseStation(const Vehicle *v, const struct Station *st);

void ReleaseDisastersTargetingVehicle(VehicleID vehicle);

typedef SmallVector<VehicleID, 2> VehicleSet;
void GetVehicleSet(VehicleSet &set, Vehicle *v, uint8 num_vehicles);

void CheckCargoCapacity(Vehicle *v);

#endif /* VEHICLE_FUNC_H */
