/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file train_cmd.cpp Handling of trains. */

#include "stdafx.h"
#include "error.h"
#include "articulated_vehicles.h"
#include "command_func.h"
#include "pathfinder/npf/npf.h"
#include "pathfinder/yapf/yapf.hpp"
#include "news_func.h"
#include "company_func.h"
#include "newgrf_sound.h"
#include "newgrf_text.h"
#include "strings_func.h"
#include "viewport_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "ai/ai.hpp"
#include "game/game.hpp"
#include "newgrf_station.h"
#include "effectvehicle_func.h"
#include "network/network.h"
#include "spritecache.h"
#include "core/random_func.hpp"
#include "company_base.h"
#include "newgrf.h"
#include "order_backup.h"
#include "zoom_func.h"
#include "signal_func.h"
#include "station_func.h"
#include "newgrf_debug.h"

#include "table/strings.h"
#include "table/train_cmd.h"

static bool ChooseTrainTrack(Train *v, PathPos origin, TileIndex tile, TrackdirBits trackdirs, bool force_res, Trackdir *best_trackdir = NULL);
static bool TryPathReserveFromDepot(Train *v);
static bool TrainCheckIfLineEnds(Train *v, bool reverse = true);
static StationID TrainEnterTile(Train *v, TileIndex tile, int x, int y);
bool TrainController(Train *v, Vehicle *nomove, bool reverse = true); // Also used in vehicle_sl.cpp.
static TileIndex TrainApproachingCrossingTile(const Train *v);
static void CheckIfTrainNeedsService(Train *v);
static void CheckNextTrainTile(Train *v);

static const byte _vehicle_initial_x_fract[4] = {10, 8, 4,  8};
static const byte _vehicle_initial_y_fract[4] = { 8, 4, 8, 10};

template <>
bool IsValidImageIndex<VEH_TRAIN>(uint8 image_index)
{
	return image_index < lengthof(_engine_sprite_base);
}

/**
 * Return the cargo weight multiplier to use for a rail vehicle
 * @param cargo Cargo type to get multiplier for
 * @return Cargo weight multiplier
 */
byte FreightWagonMult(CargoID cargo)
{
	if (!CargoSpec::Get(cargo)->is_freight) return 1;
	return _settings_game.vehicle.freight_trains;
}

/** Checks if lengths of all rail vehicles are valid. If not, shows an error message. */
void CheckTrainsLengths()
{
	const Train *v;
	bool first = true;

	FOR_ALL_TRAINS(v) {
		if (v->First() == v && !(v->vehstatus & VS_CRASHED)) {
			for (const Train *u = v, *w = v->Next(); w != NULL; u = w, w = w->Next()) {
				if (u->trackdir != TRACKDIR_DEPOT) {
					if ((w->trackdir != TRACKDIR_DEPOT &&
							max(abs(u->x_pos - w->x_pos), abs(u->y_pos - w->y_pos)) != u->CalcNextVehicleOffset()) ||
							(w->trackdir == TRACKDIR_DEPOT && TicksToLeaveDepot(u) <= 0)) {
						SetDParam(0, v->index);
						SetDParam(1, v->owner);
						ShowErrorMessage(STR_BROKEN_VEHICLE_LENGTH, INVALID_STRING_ID, WL_CRITICAL);

						if (!_networking && first) {
							first = false;
							DoCommandP(0, PM_PAUSED_ERROR, 1, CMD_PAUSE);
						}
						/* Break so we warn only once for each train. */
						break;
					}
				}
			}
		}
	}
}

/**
 * Recalculates the cached stuff of a train. Should be called each time a vehicle is added
 * to/removed from the chain, and when the game is loaded.
 * Note: this needs to be called too for 'wagon chains' (in the depot, without an engine)
 * @param same_length should length of vehicles stay the same?
 */
void Train::ConsistChanged(bool same_length)
{
	uint16 max_speed = UINT16_MAX;

	assert(this->IsFrontEngine() || this->IsFreeWagon());

	const RailVehicleInfo *rvi_v = RailVehInfo(this->engine_type);
	EngineID first_engine = this->IsFrontEngine() ? this->engine_type : INVALID_ENGINE;
	this->gcache.cached_total_length = 0;
	this->compatible_railtypes = RAILTYPES_NONE;

	bool train_can_tilt = true;

	for (Train *u = this; u != NULL; u = u->Next()) {
		const RailVehicleInfo *rvi_u = RailVehInfo(u->engine_type);

		/* Check the this->first cache. */
		assert(u->First() == this);

		/* update the 'first engine' */
		u->gcache.first_engine = this == u ? INVALID_ENGINE : first_engine;
		u->railtype = rvi_u->railtype;

		if (u->IsEngine()) first_engine = u->engine_type;

		/* Set user defined data to its default value */
		u->tcache.user_def_data = rvi_u->user_def_data;
		this->InvalidateNewGRFCache();
		u->InvalidateNewGRFCache();
	}

	for (Train *u = this; u != NULL; u = u->Next()) {
		/* Update user defined data (must be done before other properties) */
		u->tcache.user_def_data = GetVehicleProperty(u, PROP_TRAIN_USER_DATA, u->tcache.user_def_data);
		this->InvalidateNewGRFCache();
		u->InvalidateNewGRFCache();
	}

	for (Train *u = this; u != NULL; u = u->Next()) {
		const Engine *e_u = u->GetEngine();
		const RailVehicleInfo *rvi_u = &e_u->u.rail;

		if (!HasBit(e_u->info.misc_flags, EF_RAIL_TILTS)) train_can_tilt = false;

		/* Cache wagon override sprite group. NULL is returned if there is none */
		u->tcache.cached_override = GetWagonOverrideSpriteSet(u->engine_type, u->cargo_type, u->gcache.first_engine);

		/* Reset colour map */
		u->colourmap = PAL_NONE;

		/* Update powered-wagon-status and visual effect */
		u->UpdateVisualEffect(true);

		if (rvi_v->pow_wag_power != 0 && rvi_u->railveh_type == RAILVEH_WAGON &&
				UsesWagonOverride(u) && !HasBit(u->vcache.cached_vis_effect, VE_DISABLE_WAGON_POWER)) {
			/* wagon is powered */
			SetBit(u->flags, VRF_POWEREDWAGON); // cache 'powered' status
		} else {
			ClrBit(u->flags, VRF_POWEREDWAGON);
		}

		if (!u->IsArticulatedPart()) {
			/* Do not count powered wagons for the compatible railtypes, as wagons always
			   have railtype normal */
			if (rvi_u->power > 0) {
				this->compatible_railtypes |= GetRailTypeInfo(u->railtype)->powered_railtypes;
			}

			/* Some electric engines can be allowed to run on normal rail. It happens to all
			 * existing electric engines when elrails are disabled and then re-enabled */
			if (HasBit(u->flags, VRF_EL_ENGINE_ALLOWED_NORMAL_RAIL)) {
				u->railtype = RAILTYPE_RAIL;
				u->compatible_railtypes |= RAILTYPES_RAIL;
			}

			/* max speed is the minimum of the speed limits of all vehicles in the consist */
			if ((rvi_u->railveh_type != RAILVEH_WAGON || _settings_game.vehicle.wagon_speed_limits) && !UsesWagonOverride(u)) {
				uint16 speed = GetVehicleProperty(u, PROP_TRAIN_SPEED, rvi_u->max_speed);
				if (speed != 0) max_speed = min(speed, max_speed);
			}
		}

		uint16 new_cap = e_u->DetermineCapacity(u);
		u->refit_cap = min(new_cap, u->refit_cap);
		u->cargo_cap = new_cap;
		u->vcache.cached_cargo_age_period = GetVehicleProperty(u, PROP_TRAIN_CARGO_AGE_PERIOD, e_u->info.cargo_age_period);

		/* check the vehicle length (callback) */
		uint16 veh_len = CALLBACK_FAILED;
		if (e_u->GetGRF() != NULL && e_u->GetGRF()->grf_version >= 8) {
			/* Use callback 36 */
			veh_len = GetVehicleProperty(u, PROP_TRAIN_SHORTEN_FACTOR, CALLBACK_FAILED);

			if (veh_len != CALLBACK_FAILED && veh_len >= VEHICLE_LENGTH) {
				ErrorUnknownCallbackResult(e_u->GetGRFID(), CBID_VEHICLE_LENGTH, veh_len);
			}
		} else if (HasBit(e_u->info.callback_mask, CBM_VEHICLE_LENGTH)) {
			/* Use callback 11 */
			veh_len = GetVehicleCallback(CBID_VEHICLE_LENGTH, 0, 0, u->engine_type, u);
		}
		if (veh_len == CALLBACK_FAILED) veh_len = rvi_u->shorten_factor;
		veh_len = VEHICLE_LENGTH - Clamp(veh_len, 0, VEHICLE_LENGTH - 1);

		/* verify length hasn't changed */
		if (same_length && veh_len != u->gcache.cached_veh_length) VehicleLengthChanged(u);

		/* update vehicle length? */
		if (!same_length) u->gcache.cached_veh_length = veh_len;

		this->gcache.cached_total_length += u->gcache.cached_veh_length;
		this->InvalidateNewGRFCache();
		u->InvalidateNewGRFCache();
	}

	/* store consist weight/max speed in cache */
	this->vcache.cached_max_speed = max_speed;
	this->tcache.cached_tilt = train_can_tilt;
	this->tcache.cached_max_curve_speed = this->GetCurveSpeedLimit();

	/* recalculate cached weights and power too (we do this *after* the rest, so it is known which wagons are powered and need extra weight added) */
	this->CargoChanged();

	if (this->IsFrontEngine()) {
		this->UpdateAcceleration();
		SetWindowDirty(WC_VEHICLE_DETAILS, this->index);
		InvalidateWindowData(WC_VEHICLE_REFIT, this->index, VIWD_CONSIST_CHANGED);
		InvalidateWindowData(WC_VEHICLE_ORDERS, this->index, VIWD_CONSIST_CHANGED);
		InvalidateNewGRFInspectWindow(GSF_TRAINS, this->index);
	}
}

/**
 * Get the stop location of (the center) of the front vehicle of a train at
 * a platform of a station.
 * @param station_id     the ID of the station where we're stopping
 * @param tile           the tile where the vehicle currently is
 * @param v              the vehicle to get the stop location of
 * @param station_ahead  'return' the amount of 1/16th tiles in front of the train
 * @param station_length 'return' the station length in 1/16th tiles
 * @return the location, calculated from the begin of the station to stop at.
 */
int GetTrainStopLocation(StationID station_id, TileIndex tile, const Train *v, int *station_ahead, int *station_length)
{
	const Station *st = Station::Get(station_id);
	*station_ahead  = st->GetPlatformLength(tile, DirToDiagDir(v->direction)) * TILE_SIZE;
	*station_length = st->GetPlatformLength(tile) * TILE_SIZE;

	/* Default to the middle of the station for stations stops that are not in
	 * the order list like intermediate stations when non-stop is disabled */
	OrderStopLocation osl = OSL_PLATFORM_MIDDLE;
	if (v->gcache.cached_total_length >= *station_length) {
		/* The train is longer than the station, make it stop at the far end of the platform */
		osl = OSL_PLATFORM_FAR_END;
	} else if (v->current_order.IsType(OT_GOTO_STATION) && v->current_order.GetDestination() == station_id) {
		osl = v->current_order.GetStopLocation();
	}

	/* The stop location of the FRONT! of the train */
	int stop;
	switch (osl) {
		default: NOT_REACHED();

		case OSL_PLATFORM_NEAR_END:
			stop = v->gcache.cached_total_length;
			break;

		case OSL_PLATFORM_MIDDLE:
			stop = *station_length - (*station_length - v->gcache.cached_total_length) / 2;
			break;

		case OSL_PLATFORM_FAR_END:
			stop = *station_length;
			break;
	}

	/* Subtract half the front vehicle length of the train so we get the real
	 * stop location of the train. */
	return stop - (v->gcache.cached_veh_length + 1) / 2;
}


/**
 * Computes train speed limit caused by curves
 * @return imposed speed limit
 */
int Train::GetCurveSpeedLimit() const
{
	assert(this->First() == this);

	static const int absolute_max_speed = UINT16_MAX;
	int max_speed = absolute_max_speed;

	if (_settings_game.vehicle.train_acceleration_model == AM_ORIGINAL) return max_speed;

	int curvecount[2] = {0, 0};

	/* first find the curve speed limit */
	int numcurve = 0;
	int sum = 0;
	int pos = 0;
	int lastpos = -1;
	for (const Vehicle *u = this; u->Next() != NULL; u = u->Next(), pos++) {
		Direction this_dir = u->direction;
		Direction next_dir = u->Next()->direction;

		DirDiff dirdiff = DirDifference(this_dir, next_dir);
		if (dirdiff == DIRDIFF_SAME) continue;

		if (dirdiff == DIRDIFF_45LEFT) curvecount[0]++;
		if (dirdiff == DIRDIFF_45RIGHT) curvecount[1]++;
		if (dirdiff == DIRDIFF_45LEFT || dirdiff == DIRDIFF_45RIGHT) {
			if (lastpos != -1) {
				numcurve++;
				sum += pos - lastpos;
				if (pos - lastpos == 1 && max_speed > 88) {
					max_speed = 88;
				}
			}
			lastpos = pos;
		}

		/* if we have a 90 degree turn, fix the speed limit to 60 */
		if (dirdiff == DIRDIFF_90LEFT || dirdiff == DIRDIFF_90RIGHT) {
			max_speed = 61;
		}
	}

	if (numcurve > 0 && max_speed > 88) {
		if (curvecount[0] == 1 && curvecount[1] == 1) {
			max_speed = absolute_max_speed;
		} else {
			sum /= numcurve;
			max_speed = 232 - (13 - Clamp(sum, 1, 12)) * (13 - Clamp(sum, 1, 12));
		}
	}

	if (max_speed != absolute_max_speed) {
		/* Apply the engine's rail type curve speed advantage, if it slowed by curves */
		const RailtypeInfo *rti = GetRailTypeInfo(this->railtype);
		max_speed += (max_speed / 2) * rti->curve_speed;

		if (this->tcache.cached_tilt) {
			/* Apply max_speed bonus of 20% for a tilting train */
			max_speed += max_speed / 5;
		}
	}

	return max_speed;
}

static uint FindTunnelPrevTrain(const Train *t, Vehicle **vv = NULL)
{
	assert(maptile_is_rail_tunnel(t->tile));
	assert(t->trackdir == TRACKDIR_WORMHOLE);

	VehicleTileIterator iter (t->tile);
	Vehicle *closest = NULL;
	uint dist;

	switch (GetTunnelBridgeDirection(t->tile)) {
		default: NOT_REACHED();

		case DIAGDIR_NE:
			while (!iter.finished()) {
				Vehicle *v = iter.next();
				int pos = v->x_pos;
				if (pos <= t->x_pos) continue; // v is behind
				if (closest == NULL || pos < closest->x_pos) {
					closest = v;
				} else {
					assert (pos != closest->x_pos);
				}
			}
			dist = closest == NULL ? UINT_MAX : closest->x_pos - t->x_pos;
			break;

		case DIAGDIR_NW:
			while (!iter.finished()) {
				Vehicle *v = iter.next();
				int pos = v->y_pos;
				if (pos <= t->y_pos) continue; // v is behind
				if (closest == NULL || pos < closest->y_pos) {
					closest = v;
				} else {
					assert (pos != closest->y_pos);
				}
			}
			dist = closest == NULL ? UINT_MAX : closest->y_pos - t->y_pos;
			break;

		case DIAGDIR_SW:
			while (!iter.finished()) {
				Vehicle *v = iter.next();
				int pos = v->x_pos;
				if (pos >= t->x_pos) continue; // v is behind
				if (closest == NULL || pos > closest->x_pos) {
					closest = v;
				} else {
					assert (pos != closest->x_pos);
				}
			}
			dist = closest == NULL ? UINT_MAX : t->x_pos - closest->x_pos;
			break;

		case DIAGDIR_SE:
			while (!iter.finished()) {
				Vehicle *v = iter.next();
				int pos = v->y_pos;
				if (pos >= t->y_pos) continue; // v is behind
				if (closest == NULL || pos > closest->y_pos) {
					closest = v;
				} else {
					assert (pos != closest->y_pos);
				}
			}
			dist = closest == NULL ? UINT_MAX : t->y_pos - closest->y_pos;
			break;
	}

	if (vv != NULL) *vv = closest;

	return dist;
}

/**
 * Calculates the maximum speed of the vehicle under its current conditions.
 * @return Maximum speed of the vehicle.
 */
int Train::GetCurrentMaxSpeed() const
{
	int max_speed = _settings_game.vehicle.train_acceleration_model == AM_ORIGINAL ?
			this->gcache.cached_max_track_speed :
			this->tcache.cached_max_curve_speed;

	if (_settings_game.vehicle.train_acceleration_model == AM_REALISTIC && IsRailStationTile(this->tile)) {
		StationID sid = GetStationIndex(this->tile);
		if (this->current_order.ShouldStopAtStation(this, sid)) {
			int station_ahead;
			int station_length;
			int stop_at = GetTrainStopLocation(sid, this->tile, this, &station_ahead, &station_length);

			/* The distance to go is whatever is still ahead of the train minus the
			 * distance from the train's stop location to the end of the platform */
			int distance_to_go = station_ahead / TILE_SIZE - (station_length - stop_at) / TILE_SIZE;

			if (distance_to_go > 0) {
				int st_max_speed = 120;

				int delta_v = this->cur_speed / (distance_to_go + 1);
				if (max_speed > (this->cur_speed - delta_v)) {
					st_max_speed = this->cur_speed - (delta_v / 10);
				}

				st_max_speed = max(st_max_speed, 25 * distance_to_go);
				max_speed = min(max_speed, st_max_speed);
			}
		}
	}

	for (const Train *u = this; u != NULL; u = u->Next()) {
		if (_settings_game.vehicle.train_acceleration_model == AM_REALISTIC && u->trackdir == TRACKDIR_DEPOT) {
			max_speed = min(max_speed, 61);
			break;
		}

		/* Vehicle is on the middle part of a bridge. */
		if (u->trackdir == TRACKDIR_WORMHOLE && !(u->vehstatus & VS_HIDDEN)) {
			max_speed = min(max_speed, GetBridgeSpec(GetRailBridgeType(u->tile))->speed);
		}
	}

	if (this->trackdir == TRACKDIR_WORMHOLE && (this->vehstatus & VS_HIDDEN) && maptile_has_tunnel_signal(this->tile, false)) {
		Vehicle *v;
		uint dist = FindTunnelPrevTrain(this, &v);

		if (dist <= TILE_SIZE) {
			max_speed = 0;
		} else if (v != NULL) {
			max_speed = min(max_speed, (dist - TILE_SIZE) * this->GetAdvanceDistance() / 2);
			if (dist < 2 * TILE_SIZE) max_speed = min(max_speed, v->cur_speed);
		}
	}

	max_speed = min(max_speed, this->current_order.max_speed);
	return min(max_speed, this->gcache.cached_max_track_speed);
}

/** Update acceleration of the train from the cached power and weight. */
void Train::UpdateAcceleration()
{
	assert(this->IsFrontEngine() || this->IsFreeWagon());

	uint power = this->gcache.cached_power;
	uint weight = this->gcache.cached_weight;
	assert(weight != 0);
	this->acceleration = Clamp(power / weight * 4, 1, 255);
}

/**
 * Get the width of a train vehicle image in the GUI.
 * @param offset Additional offset for positioning the sprite; set to NULL if not needed
 * @return Width in pixels
 */
int Train::GetDisplayImageWidth(Point *offset) const
{
	int reference_width = TRAININFO_DEFAULT_VEHICLE_WIDTH;
	int vehicle_pitch = 0;

	const Engine *e = this->GetEngine();
	if (e->GetGRF() != NULL && is_custom_sprite(e->u.rail.image_index)) {
		reference_width = e->GetGRF()->traininfo_vehicle_width;
		vehicle_pitch = e->GetGRF()->traininfo_vehicle_pitch;
	}

	if (offset != NULL) {
		offset->x = reference_width / 2;
		offset->y = vehicle_pitch;
	}
	return this->gcache.cached_veh_length * reference_width / VEHICLE_LENGTH;
}

static SpriteID GetDefaultTrainSprite(uint8 spritenum, Direction direction)
{
	assert(IsValidImageIndex<VEH_TRAIN>(spritenum));
	return ((direction + _engine_sprite_add[spritenum]) & _engine_sprite_and[spritenum]) + _engine_sprite_base[spritenum];
}

/**
 * Get the sprite to display the train.
 * @param direction Direction of view/travel.
 * @param image_type Visualisation context.
 * @return Sprite to display.
 */
SpriteID Train::GetImage(Direction direction, EngineImageType image_type) const
{
	uint8 spritenum = this->spritenum;
	SpriteID sprite;

	if (HasBit(this->flags, VRF_REVERSE_DIRECTION)) direction = ReverseDir(direction);

	if (is_custom_sprite(spritenum)) {
		sprite = GetCustomVehicleSprite(this, (Direction)(direction + 4 * IS_CUSTOM_SECONDHEAD_SPRITE(spritenum)), image_type);
		if (sprite != 0) return sprite;

		spritenum = this->GetEngine()->original_image_index;
	}

	assert(IsValidImageIndex<VEH_TRAIN>(spritenum));
	sprite = GetDefaultTrainSprite(spritenum, direction);

	if (this->cargo.StoredCount() >= this->cargo_cap / 2U) sprite += _wagon_full_adder[spritenum];

	return sprite;
}

static SpriteID GetRailIcon(EngineID engine, bool rear_head, int &y, EngineImageType image_type)
{
	const Engine *e = Engine::Get(engine);
	Direction dir = rear_head ? DIR_E : DIR_W;
	uint8 spritenum = e->u.rail.image_index;

	if (is_custom_sprite(spritenum)) {
		SpriteID sprite = GetCustomVehicleIcon(engine, dir, image_type);
		if (sprite != 0) {
			if (e->GetGRF() != NULL) {
				y += e->GetGRF()->traininfo_vehicle_pitch;
			}
			return sprite;
		}

		spritenum = Engine::Get(engine)->original_image_index;
	}

	if (rear_head) spritenum++;

	return GetDefaultTrainSprite(spritenum, DIR_W);
}

void DrawTrainEngine(int left, int right, int preferred_x, int y, EngineID engine, PaletteID pal, EngineImageType image_type)
{
	if (RailVehInfo(engine)->railveh_type == RAILVEH_MULTIHEAD) {
		int yf = y;
		int yr = y;

		SpriteID spritef = GetRailIcon(engine, false, yf, image_type);
		SpriteID spriter = GetRailIcon(engine, true, yr, image_type);
		const Sprite *real_spritef = GetSprite(spritef, ST_NORMAL);
		const Sprite *real_spriter = GetSprite(spriter, ST_NORMAL);

		preferred_x = Clamp(preferred_x, left - UnScaleByZoom(real_spritef->x_offs, ZOOM_LVL_GUI) + 14, right - UnScaleByZoom(real_spriter->width, ZOOM_LVL_GUI) - UnScaleByZoom(real_spriter->x_offs, ZOOM_LVL_GUI) - 15);

		DrawSprite(spritef, pal, preferred_x - 14, yf);
		DrawSprite(spriter, pal, preferred_x + 15, yr);
	} else {
		SpriteID sprite = GetRailIcon(engine, false, y, image_type);
		const Sprite *real_sprite = GetSprite(sprite, ST_NORMAL);
		preferred_x = Clamp(preferred_x, left - UnScaleByZoom(real_sprite->x_offs, ZOOM_LVL_GUI), right - UnScaleByZoom(real_sprite->width, ZOOM_LVL_GUI) - UnScaleByZoom(real_sprite->x_offs, ZOOM_LVL_GUI));
		DrawSprite(sprite, pal, preferred_x, y);
	}
}

/**
 * Get the size of the sprite of a train sprite heading west, or both heads (used for lists).
 * @param engine The engine to get the sprite from.
 * @param[out] width The width of the sprite.
 * @param[out] height The height of the sprite.
 * @param[out] xoffs Number of pixels to shift the sprite to the right.
 * @param[out] yoffs Number of pixels to shift the sprite downwards.
 * @param image_type Context the sprite is used in.
 */
void GetTrainSpriteSize(EngineID engine, uint &width, uint &height, int &xoffs, int &yoffs, EngineImageType image_type)
{
	int y = 0;

	SpriteID sprite = GetRailIcon(engine, false, y, image_type);
	const Sprite *real_sprite = GetSprite(sprite, ST_NORMAL);

	width  = UnScaleByZoom(real_sprite->width, ZOOM_LVL_GUI);
	height = UnScaleByZoom(real_sprite->height, ZOOM_LVL_GUI);
	xoffs  = UnScaleByZoom(real_sprite->x_offs, ZOOM_LVL_GUI);
	yoffs  = UnScaleByZoom(real_sprite->y_offs, ZOOM_LVL_GUI);

	if (RailVehInfo(engine)->railveh_type == RAILVEH_MULTIHEAD) {
		sprite = GetRailIcon(engine, true, y, image_type);
		real_sprite = GetSprite(sprite, ST_NORMAL);

		/* Calculate values relative to an imaginary center between the two sprites. */
		width = TRAININFO_DEFAULT_VEHICLE_WIDTH + UnScaleByZoom(real_sprite->width, ZOOM_LVL_GUI) + UnScaleByZoom(real_sprite->x_offs, ZOOM_LVL_GUI) - xoffs;
		height = max<uint>(height, UnScaleByZoom(real_sprite->height, ZOOM_LVL_GUI));
		xoffs  = xoffs - TRAININFO_DEFAULT_VEHICLE_WIDTH / 2;
		yoffs  = min(yoffs, UnScaleByZoom(real_sprite->y_offs, ZOOM_LVL_GUI));
	}
}

/**
 * Build a railroad wagon.
 * @param tile     tile of the depot where rail-vehicle is built.
 * @param flags    type of operation.
 * @param e        the engine to build.
 * @param ret[out] the vehicle that has been built.
 * @return the cost of this operation or an error.
 */
static CommandCost CmdBuildRailWagon(TileIndex tile, DoCommandFlag flags, const Engine *e, Vehicle **ret)
{
	const RailVehicleInfo *rvi = &e->u.rail;

	/* Check that the wagon can drive on the track in question */
	if (!IsCompatibleRail(rvi->railtype, GetRailType(tile))) return CMD_ERROR;

	if (flags & DC_EXEC) {
		Train *v = new Train();
		*ret = v;
		v->spritenum = rvi->image_index;

		v->engine_type = e->index;
		v->gcache.first_engine = INVALID_ENGINE; // needs to be set before first callback

		DiagDirection dir = GetGroundDepotDirection(tile);

		v->direction = DiagDirToDir(dir);
		v->tile = tile;

		int x = TileX(tile) * TILE_SIZE | _vehicle_initial_x_fract[dir];
		int y = TileY(tile) * TILE_SIZE | _vehicle_initial_y_fract[dir];

		v->x_pos = x;
		v->y_pos = y;
		v->z_pos = GetSlopePixelZ(x, y);
		v->owner = _current_company;
		v->trackdir = TRACKDIR_DEPOT;
		v->vehstatus = VS_HIDDEN | VS_DEFPAL;

		v->SetWagon();

		v->SetFreeWagon();
		InvalidateWindowData(WC_VEHICLE_DEPOT, v->tile);

		v->cargo_type = e->GetDefaultCargoType();
		v->cargo_cap = rvi->capacity;
		v->refit_cap = 0;

		v->railtype = rvi->railtype;

		v->build_year = _cur_year;
		v->cur_image = SPR_IMG_QUERY;
		v->random_bits = VehicleRandomBits();

		v->group_id = DEFAULT_GROUP;

		AddArticulatedParts(v);

		_new_vehicle_id = v->index;

		VehicleUpdatePosition(v);
		v->First()->ConsistChanged(false);
		UpdateTrainGroupID(v->First());

		CheckConsistencyOfArticulatedVehicle(v);

		/* Try to connect the vehicle to one of free chains of wagons. */
		Train *w;
		FOR_ALL_TRAINS(w) {
			if (w->tile == tile &&              ///< Same depot
					w->IsFreeWagon() &&             ///< A free wagon chain
					w->engine_type == e->index &&   ///< Same type
					w->First() != v &&              ///< Don't connect to ourself
					!(w->vehstatus & VS_CRASHED)) { ///< Not crashed/flooded
				DoCommand(0, v->index | 1 << 20, w->Last()->index, DC_EXEC, CMD_MOVE_RAIL_VEHICLE);
				break;
			}
		}
	}

	return CommandCost();
}

/** Move all free vehicles in the depot to the train */
static void NormalizeTrainVehInDepot(const Train *u)
{
	const Train *v;
	FOR_ALL_TRAINS(v) {
		if (v->IsFreeWagon() && v->tile == u->tile &&
				v->trackdir == TRACKDIR_DEPOT) {
			if (DoCommand(0, v->index | 1 << 20, u->index, DC_EXEC,
					CMD_MOVE_RAIL_VEHICLE).Failed())
				break;
		}
	}
}

static void AddRearEngineToMultiheadedTrain(Train *v)
{
	Train *u = new Train();
	v->value >>= 1;
	u->value = v->value;
	u->direction = v->direction;
	u->owner = v->owner;
	u->tile = v->tile;
	u->x_pos = v->x_pos;
	u->y_pos = v->y_pos;
	u->z_pos = v->z_pos;
	u->trackdir = TRACKDIR_DEPOT;
	u->vehstatus = v->vehstatus & ~VS_STOPPED;
	u->spritenum = v->spritenum + 1;
	u->cargo_type = v->cargo_type;
	u->cargo_subtype = v->cargo_subtype;
	u->cargo_cap = v->cargo_cap;
	u->refit_cap = v->refit_cap;
	u->railtype = v->railtype;
	u->engine_type = v->engine_type;
	u->build_year = v->build_year;
	u->cur_image = SPR_IMG_QUERY;
	u->random_bits = VehicleRandomBits();
	v->SetMultiheaded();
	u->SetMultiheaded();
	v->SetNext(u);
	VehicleUpdatePosition(u);

	/* Now we need to link the front and rear engines together */
	v->other_multiheaded_part = u;
	u->other_multiheaded_part = v;
}

/**
 * Build a railroad vehicle.
 * @param tile     tile of the depot where rail-vehicle is built.
 * @param flags    type of operation.
 * @param e        the engine to build.
 * @param data     bit 0 prevents any free cars from being added to the train.
 * @param ret[out] the vehicle that has been built.
 * @return the cost of this operation or an error.
 */
CommandCost CmdBuildRailVehicle(TileIndex tile, DoCommandFlag flags, const Engine *e, uint16 data, Vehicle **ret)
{
	const RailVehicleInfo *rvi = &e->u.rail;

	if (rvi->railveh_type == RAILVEH_WAGON) return CmdBuildRailWagon(tile, flags, e, ret);

	/* Check if depot and new engine uses the same kind of tracks *
	 * We need to see if the engine got power on the tile to avoid electric engines in non-electric depots */
	if (!HasPowerOnRail(rvi->railtype, GetRailType(tile))) return CMD_ERROR;

	if (flags & DC_EXEC) {
		DiagDirection dir = GetGroundDepotDirection(tile);
		int x = TileX(tile) * TILE_SIZE + _vehicle_initial_x_fract[dir];
		int y = TileY(tile) * TILE_SIZE + _vehicle_initial_y_fract[dir];

		Train *v = new Train();
		*ret = v;
		v->direction = DiagDirToDir(dir);
		v->tile = tile;
		v->owner = _current_company;
		v->x_pos = x;
		v->y_pos = y;
		v->z_pos = GetSlopePixelZ(x, y);
		v->trackdir = TRACKDIR_DEPOT;
		v->vehstatus = VS_HIDDEN | VS_STOPPED | VS_DEFPAL;
		v->spritenum = rvi->image_index;
		v->cargo_type = e->GetDefaultCargoType();
		v->cargo_cap = rvi->capacity;
		v->refit_cap = 0;
		v->last_station_visited = INVALID_STATION;
		v->last_loading_station = INVALID_STATION;

		v->engine_type = e->index;
		v->gcache.first_engine = INVALID_ENGINE; // needs to be set before first callback

		v->reliability = e->reliability;
		v->reliability_spd_dec = e->reliability_spd_dec;
		v->max_age = e->GetLifeLengthInDays();

		v->railtype = rvi->railtype;
		_new_vehicle_id = v->index;

		v->SetServiceInterval(Company::Get(_current_company)->settings.vehicle.servint_trains);
		v->date_of_last_service = _date;
		v->build_year = _cur_year;
		v->cur_image = SPR_IMG_QUERY;
		v->random_bits = VehicleRandomBits();

		if (e->flags & ENGINE_EXCLUSIVE_PREVIEW) SetBit(v->vehicle_flags, VF_BUILT_AS_PROTOTYPE);
		v->SetServiceIntervalIsPercent(Company::Get(_current_company)->settings.vehicle.servint_ispercent);

		v->group_id = DEFAULT_GROUP;

		v->SetFrontEngine();
		v->SetEngine();

		VehicleUpdatePosition(v);

		if (rvi->railveh_type == RAILVEH_MULTIHEAD) {
			AddRearEngineToMultiheadedTrain(v);
		} else {
			AddArticulatedParts(v);
		}

		v->ConsistChanged(false);
		UpdateTrainGroupID(v);

		if (!HasBit(data, 0) && !(flags & DC_AUTOREPLACE)) { // check if the cars should be added to the new vehicle
			NormalizeTrainVehInDepot(v);
		}

		CheckConsistencyOfArticulatedVehicle(v);
	}

	return CommandCost();
}

static Train *FindGoodVehiclePos(const Train *src)
{
	EngineID eng = src->engine_type;
	TileIndex tile = src->tile;

	Train *dst;
	FOR_ALL_TRAINS(dst) {
		if (dst->IsFreeWagon() && dst->tile == tile && !(dst->vehstatus & VS_CRASHED)) {
			/* check so all vehicles in the line have the same engine. */
			Train *t = dst;
			while (t->engine_type == eng) {
				t = t->Next();
				if (t == NULL) return dst;
			}
		}
	}

	return NULL;
}

/** Helper type for lists/vectors of trains */
typedef SmallVector<Train *, 16> TrainList;

/**
 * Make a backup of a train into a train list.
 * @param list to make the backup in
 * @param t    the train to make the backup of
 */
static void MakeTrainBackup(TrainList &list, Train *t)
{
	for (; t != NULL; t = t->Next()) *list.Append() = t;
}

/**
 * Restore the train from the backup list.
 * @param list the train to restore.
 */
static void RestoreTrainBackup(TrainList &list)
{
	/* No train, nothing to do. */
	if (list.Length() == 0) return;

	Train *prev = NULL;
	/* Iterate over the list and rebuild it. */
	for (Train **iter = list.Begin(); iter != list.End(); iter++) {
		Train *t = *iter;
		if (prev != NULL) {
			prev->SetNext(t);
		} else if (t->Previous() != NULL) {
			/* Make sure the head of the train is always the first in the chain. */
			t->Previous()->SetNext(NULL);
		}
		prev = t;
	}
}

/**
 * Remove the given wagon from its consist.
 * @param part the part of the train to remove.
 * @param chain whether to remove the whole chain.
 */
static void RemoveFromConsist(Train *part, bool chain = false)
{
	Train *tail = chain ? part->Last() : part->GetLastEnginePart();

	/* Unlink at the front, but make it point to the next
	 * vehicle after the to be remove part. */
	if (part->Previous() != NULL) part->Previous()->SetNext(tail->Next());

	/* Unlink at the back */
	tail->SetNext(NULL);
}

/**
 * Inserts a chain into the train at dst.
 * @param dst   the place where to append after.
 * @param chain the chain to actually add.
 */
static void InsertInConsist(Train *dst, Train *chain)
{
	/* We do not want to add something in the middle of an articulated part. */
	assert(dst->Next() == NULL || !dst->Next()->IsArticulatedPart());

	chain->Last()->SetNext(dst->Next());
	dst->SetNext(chain);
}

/**
 * Normalise the dual heads in the train, i.e. if one is
 * missing move that one to this train.
 * @param t the train to normalise.
 */
static void NormaliseDualHeads(Train *t)
{
	for (; t != NULL; t = t->GetNextVehicle()) {
		if (!t->IsMultiheaded() || !t->IsEngine()) continue;

		/* Make sure that there are no free cars before next engine */
		Train *u;
		for (u = t; u->Next() != NULL && !u->Next()->IsEngine(); u = u->Next()) {}

		if (u == t->other_multiheaded_part) continue;

		/* Remove the part from the 'wrong' train */
		RemoveFromConsist(t->other_multiheaded_part);
		/* And add it to the 'right' train */
		InsertInConsist(u, t->other_multiheaded_part);
	}
}

/**
 * Normalise the sub types of the parts in this chain.
 * @param chain the chain to normalise.
 */
static void NormaliseSubtypes(Train *chain)
{
	/* Nothing to do */
	if (chain == NULL) return;

	/* We must be the first in the chain. */
	assert(chain->Previous() == NULL);

	/* Set the appropriate bits for the first in the chain. */
	if (chain->IsWagon()) {
		chain->SetFreeWagon();
	} else {
		assert(chain->IsEngine());
		chain->SetFrontEngine();
	}

	/* Now clear the bits for the rest of the chain */
	for (Train *t = chain->Next(); t != NULL; t = t->Next()) {
		t->ClearFreeWagon();
		t->ClearFrontEngine();
	}
}

/**
 * Check/validate whether we may actually build a new train.
 * @note All vehicles are/were 'heads' of their chains.
 * @param original_dst The original destination chain.
 * @param dst          The destination chain after constructing the train.
 * @param original_dst The original source chain.
 * @param dst          The source chain after constructing the train.
 * @return possible error of this command.
 */
static CommandCost CheckNewTrain(Train *original_dst, Train *dst, Train *original_src, Train *src)
{
	/* Just add 'new' engines and subtract the original ones.
	 * If that's less than or equal to 0 we can be sure we did
	 * not add any engines (read: trains) along the way. */
	if ((src          != NULL && src->IsEngine()          ? 1 : 0) +
			(dst          != NULL && dst->IsEngine()          ? 1 : 0) -
			(original_src != NULL && original_src->IsEngine() ? 1 : 0) -
			(original_dst != NULL && original_dst->IsEngine() ? 1 : 0) <= 0) {
		return CommandCost();
	}

	/* Get a free unit number and check whether it's within the bounds.
	 * There will always be a maximum of one new train. */
	if (GetFreeUnitNumber(VEH_TRAIN) <= _settings_game.vehicle.max_trains) return CommandCost();

	return_cmd_error(STR_ERROR_TOO_MANY_VEHICLES_IN_GAME);
}

/**
 * Check whether the train parts can be attached.
 * @param t the train to check
 * @return possible error of this command.
 */
static CommandCost CheckTrainAttachment(Train *t)
{
	/* No multi-part train, no need to check. */
	if (t == NULL || t->Next() == NULL || !t->IsEngine()) return CommandCost();

	/* The maximum length for a train. For each part we decrease this by one
	 * and if the result is negative the train is simply too long. */
	int allowed_len = _settings_game.vehicle.max_train_length * TILE_SIZE - t->gcache.cached_veh_length;

	Train *head = t;
	Train *prev = t;

	/* Break the prev -> t link so it always holds within the loop. */
	t = t->Next();
	prev->SetNext(NULL);

	/* Make sure the cache is cleared. */
	head->InvalidateNewGRFCache();

	while (t != NULL) {
		allowed_len -= t->gcache.cached_veh_length;

		Train *next = t->Next();

		/* Unlink the to-be-added piece; it is already unlinked from the previous
		 * part due to the fact that the prev -> t link is broken. */
		t->SetNext(NULL);

		/* Don't check callback for articulated or rear dual headed parts */
		if (!t->IsArticulatedPart() && !t->IsRearDualheaded()) {
			/* Back up and clear the first_engine data to avoid using wagon override group */
			EngineID first_engine = t->gcache.first_engine;
			t->gcache.first_engine = INVALID_ENGINE;

			/* We don't want the cache to interfere. head's cache is cleared before
			 * the loop and after each callback does not need to be cleared here. */
			t->InvalidateNewGRFCache();

			uint16 callback = GetVehicleCallbackParent(CBID_TRAIN_ALLOW_WAGON_ATTACH, 0, 0, head->engine_type, t, head);

			/* Restore original first_engine data */
			t->gcache.first_engine = first_engine;

			/* We do not want to remember any cached variables from the test run */
			t->InvalidateNewGRFCache();
			head->InvalidateNewGRFCache();

			if (callback != CALLBACK_FAILED) {
				/* A failing callback means everything is okay */
				StringID error = STR_NULL;

				if (head->GetGRF()->grf_version < 8) {
					if (callback == 0xFD) error = STR_ERROR_INCOMPATIBLE_RAIL_TYPES;
					if (callback  < 0xFD) error = GetGRFStringID(head->GetGRFID(), 0xD000 + callback);
					if (callback >= 0x100) ErrorUnknownCallbackResult(head->GetGRFID(), CBID_TRAIN_ALLOW_WAGON_ATTACH, callback);
				} else {
					if (callback < 0x400) {
						error = GetGRFStringID(head->GetGRFID(), 0xD000 + callback);
					} else {
						switch (callback) {
							case 0x400: // allow if railtypes match (always the case for OpenTTD)
							case 0x401: // allow
								break;

							default:    // unknown reason -> disallow
							case 0x402: // disallow attaching
								error = STR_ERROR_INCOMPATIBLE_RAIL_TYPES;
								break;
						}
					}
				}

				if (error != STR_NULL) return_cmd_error(error);
			}
		}

		/* And link it to the new part. */
		prev->SetNext(t);
		prev = t;
		t = next;
	}

	if (allowed_len < 0) return_cmd_error(STR_ERROR_TRAIN_TOO_LONG);
	return CommandCost();
}

/**
 * Validate whether we are going to create valid trains.
 * @note All vehicles are/were 'heads' of their chains.
 * @param original_dst The original destination chain.
 * @param dst          The destination chain after constructing the train.
 * @param original_dst The original source chain.
 * @param dst          The source chain after constructing the train.
 * @param check_limit  Whether to check the vehicle limit.
 * @return possible error of this command.
 */
static CommandCost ValidateTrains(Train *original_dst, Train *dst, Train *original_src, Train *src, bool check_limit)
{
	/* Check whether we may actually construct the trains. */
	CommandCost ret = CheckTrainAttachment(src);
	if (ret.Failed()) return ret;
	ret = CheckTrainAttachment(dst);
	if (ret.Failed()) return ret;

	/* Check whether we need to build a new train. */
	return check_limit ? CheckNewTrain(original_dst, dst, original_src, src) : CommandCost();
}

/**
 * Arrange the trains in the wanted way.
 * @param dst_head   The destination chain of the to be moved vehicle.
 * @param dst        The destination for the to be moved vehicle.
 * @param src_head   The source chain of the to be moved vehicle.
 * @param src        The to be moved vehicle.
 * @param move_chain Whether to move all vehicles after src or not.
 */
static void ArrangeTrains(Train **dst_head, Train *dst, Train **src_head, Train *src, bool move_chain)
{
	/* First determine the front of the two resulting trains */
	if (*src_head == *dst_head) {
		/* If we aren't moving part(s) to a new train, we are just moving the
		 * front back and there is not destination head. */
		*dst_head = NULL;
	} else if (*dst_head == NULL) {
		/* If we are moving to a new train the head of the move train would become
		 * the head of the new vehicle. */
		*dst_head = src;
	}

	if (src == *src_head) {
		/* If we are moving the front of a train then we are, in effect, creating
		 * a new head for the train. Point to that. Unless we are moving the whole
		 * train in which case there is not 'source' train anymore.
		 * In case we are a multiheaded part we want the complete thing to come
		 * with us, so src->GetNextUnit(), however... when we are e.g. a wagon
		 * that is followed by a rear multihead we do not want to include that. */
		*src_head = move_chain ? NULL :
				(src->IsMultiheaded() ? src->GetNextUnit() : src->GetNextVehicle());
	}

	/* Now it's just simply removing the part that we are going to move from the
	 * source train and *if* the destination is a not a new train add the chain
	 * at the destination location. */
	RemoveFromConsist(src, move_chain);
	if (*dst_head != src) InsertInConsist(dst, src);

	/* Now normalise the dual heads, that is move the dual heads around in such
	 * a way that the head and rear of a dual head are in the same train */
	NormaliseDualHeads(*src_head);
	NormaliseDualHeads(*dst_head);
}

/**
 * Normalise the head of the train again, i.e. that is tell the world that
 * we have changed and update all kinds of variables.
 * @param head the train to update.
 */
static void NormaliseTrainHead(Train *head)
{
	/* Not much to do! */
	if (head == NULL) return;

	/* Tell the 'world' the train changed. */
	head->ConsistChanged(false);
	UpdateTrainGroupID(head);

	/* Not a front engine, i.e. a free wagon chain. No need to do more. */
	if (!head->IsFrontEngine()) return;

	/* Update the refit button and window */
	InvalidateWindowData(WC_VEHICLE_REFIT, head->index, VIWD_CONSIST_CHANGED);
	SetWindowWidgetDirty(WC_VEHICLE_VIEW, head->index, WID_VV_REFIT);

	/* If we don't have a unit number yet, set one. */
	if (head->unitnumber != 0) return;
	head->unitnumber = GetFreeUnitNumber(VEH_TRAIN);
}

/**
 * Move a rail vehicle around inside the depot.
 * @param tile unused
 * @param flags type of operation
 *              Note: DC_AUTOREPLACE is set when autoreplace tries to undo its modifications or moves vehicles to temporary locations inside the depot.
 * @param p1 various bitstuffed elements
 * - p1 (bit  0 - 19) source vehicle index
 * - p1 (bit      20) move all vehicles following the source vehicle
 * @param p2 what wagon to put the source wagon AFTER, XXX - INVALID_VEHICLE to make a new line
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdMoveRailVehicle(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID s = GB(p1, 0, 20);
	VehicleID d = GB(p2, 0, 20);
	bool move_chain = HasBit(p1, 20);

	Train *src = Train::GetIfValid(s);
	if (src == NULL) return CMD_ERROR;

	CommandCost ret = CheckOwnership(src->owner);
	if (ret.Failed()) return ret;

	/* Do not allow moving crashed vehicles inside the depot, it is likely to cause asserts later */
	if (src->vehstatus & VS_CRASHED) return CMD_ERROR;

	/* if nothing is selected as destination, try and find a matching vehicle to drag to. */
	Train *dst;
	if (d == INVALID_VEHICLE) {
		dst = src->IsEngine() ? NULL : FindGoodVehiclePos(src);
	} else {
		dst = Train::GetIfValid(d);
		if (dst == NULL) return CMD_ERROR;

		CommandCost ret = CheckOwnership(dst->owner);
		if (ret.Failed()) return ret;

		/* Do not allow appending to crashed vehicles, too */
		if (dst->vehstatus & VS_CRASHED) return CMD_ERROR;
	}

	/* if an articulated part is being handled, deal with its parent vehicle */
	src = src->GetFirstEnginePart();
	if (dst != NULL) {
		dst = dst->GetFirstEnginePart();
	}

	/* don't move the same vehicle.. */
	if (src == dst) return CommandCost();

	/* locate the head of the two chains */
	Train *src_head = src->First();
	Train *dst_head;
	if (dst != NULL) {
		dst_head = dst->First();
		if (dst_head->tile != src_head->tile) return CMD_ERROR;
		/* Now deal with articulated part of destination wagon */
		dst = dst->GetLastEnginePart();
	} else {
		dst_head = NULL;
	}

	if (src->IsRearDualheaded()) return_cmd_error(STR_ERROR_REAR_ENGINE_FOLLOW_FRONT);

	/* When moving all wagons, we can't have the same src_head and dst_head */
	if (move_chain && src_head == dst_head) return CommandCost();

	/* When moving a multiheaded part to be place after itself, bail out. */
	if (!move_chain && dst != NULL && dst->IsRearDualheaded() && src == dst->other_multiheaded_part) return CommandCost();

	/* Check if all vehicles in the source train are stopped inside a depot. */
	if (!src_head->IsStoppedInDepot()) return_cmd_error(STR_ERROR_TRAINS_CAN_ONLY_BE_ALTERED_INSIDE_A_DEPOT);

	/* Check if all vehicles in the destination train are stopped inside a depot. */
	if (dst_head != NULL && !dst_head->IsStoppedInDepot()) return_cmd_error(STR_ERROR_TRAINS_CAN_ONLY_BE_ALTERED_INSIDE_A_DEPOT);

	/* First make a backup of the order of the trains. That way we can do
	 * whatever we want with the order and later on easily revert. */
	TrainList original_src;
	TrainList original_dst;

	MakeTrainBackup(original_src, src_head);
	MakeTrainBackup(original_dst, dst_head);

	/* Also make backup of the original heads as ArrangeTrains can change them.
	 * For the destination head we do not care if it is the same as the source
	 * head because in that case it's just a copy. */
	Train *original_src_head = src_head;
	Train *original_dst_head = (dst_head == src_head ? NULL : dst_head);

	/* We want this information from before the rearrangement, but execute this after the validation.
	 * original_src_head can't be NULL; src is by definition != NULL, so src_head can't be NULL as
	 * src->GetFirst() always yields non-NULL, so eventually original_src_head != NULL as well. */
	bool original_src_head_front_engine = original_src_head->IsFrontEngine();
	bool original_dst_head_front_engine = original_dst_head != NULL && original_dst_head->IsFrontEngine();

	/* (Re)arrange the trains in the wanted arrangement. */
	ArrangeTrains(&dst_head, dst, &src_head, src, move_chain);

	if ((flags & DC_AUTOREPLACE) == 0) {
		/* If the autoreplace flag is set we do not need to test for the validity
		 * because we are going to revert the train to its original state. As we
		 * assume the original state was correct autoreplace can skip this. */
		CommandCost ret = ValidateTrains(original_dst_head, dst_head, original_src_head, src_head, true);
		if (ret.Failed()) {
			/* Restore the train we had. */
			RestoreTrainBackup(original_src);
			RestoreTrainBackup(original_dst);
			return ret;
		}
	}

	/* do it? */
	if (flags & DC_EXEC) {
		/* Remove old heads from the statistics */
		if (original_src_head_front_engine) GroupStatistics::CountVehicle(original_src_head, -1);
		if (original_dst_head_front_engine) GroupStatistics::CountVehicle(original_dst_head, -1);

		/* First normalise the sub types of the chains. */
		NormaliseSubtypes(src_head);
		NormaliseSubtypes(dst_head);

		/* There are 14 different cases:
		 *  1) front engine gets moved to a new train, it stays a front engine.
		 *     a) the 'next' part is a wagon that becomes a free wagon chain.
		 *     b) the 'next' part is an engine that becomes a front engine.
		 *     c) there is no 'next' part, nothing else happens
		 *  2) front engine gets moved to another train, it is not a front engine anymore
		 *     a) the 'next' part is a wagon that becomes a free wagon chain.
		 *     b) the 'next' part is an engine that becomes a front engine.
		 *     c) there is no 'next' part, nothing else happens
		 *  3) front engine gets moved to later in the current train, it is not a front engine anymore.
		 *     a) the 'next' part is a wagon that becomes a free wagon chain.
		 *     b) the 'next' part is an engine that becomes a front engine.
		 *  4) free wagon gets moved
		 *     a) the 'next' part is a wagon that becomes a free wagon chain.
		 *     b) the 'next' part is an engine that becomes a front engine.
		 *     c) there is no 'next' part, nothing else happens
		 *  5) non front engine gets moved and becomes a new train, nothing else happens
		 *  6) non front engine gets moved within a train / to another train, nothing hapens
		 *  7) wagon gets moved, nothing happens
		 */
		if (src == original_src_head && src->IsEngine() && !src->IsFrontEngine()) {
			/* Cases #2 and #3: the front engine gets trashed. */
			DeleteWindowById(WC_VEHICLE_VIEW, src->index);
			DeleteWindowById(WC_VEHICLE_ORDERS, src->index);
			DeleteWindowById(WC_VEHICLE_REFIT, src->index);
			DeleteWindowById(WC_VEHICLE_DETAILS, src->index);
			DeleteWindowById(WC_VEHICLE_TIMETABLE, src->index);
			DeleteNewGRFInspectWindow(GSF_TRAINS, src->index);
			SetWindowDirty(WC_COMPANY, _current_company);

			/* Delete orders, group stuff and the unit number as we're not the
			 * front of any vehicle anymore. */
			DeleteVehicleOrders(src);
			RemoveVehicleFromGroup(src);
			src->unitnumber = 0;
		}

		/* We weren't a front engine but are becoming one. So
		 * we should be put in the default group. */
		if (original_src_head != src && dst_head == src) {
			SetTrainGroupID(src, DEFAULT_GROUP);
			SetWindowDirty(WC_COMPANY, _current_company);
		}

		/* Add new heads to statistics */
		if (src_head != NULL && src_head->IsFrontEngine()) GroupStatistics::CountVehicle(src_head, 1);
		if (dst_head != NULL && dst_head->IsFrontEngine()) GroupStatistics::CountVehicle(dst_head, 1);

		/* Handle 'new engine' part of cases #1b, #2b, #3b, #4b and #5 in NormaliseTrainHead. */
		NormaliseTrainHead(src_head);
		NormaliseTrainHead(dst_head);

		if ((flags & DC_NO_CARGO_CAP_CHECK) == 0) {
			CheckCargoCapacity(src_head);
			CheckCargoCapacity(dst_head);
		}

		if (src_head != NULL) src_head->First()->MarkDirty();
		if (dst_head != NULL) dst_head->First()->MarkDirty();

		/* We are undoubtedly changing something in the depot and train list. */
		InvalidateWindowData(WC_VEHICLE_DEPOT, src->tile);
		InvalidateWindowClassesData(WC_TRAINS_LIST, 0);
	} else {
		/* We don't want to execute what we're just tried. */
		RestoreTrainBackup(original_src);
		RestoreTrainBackup(original_dst);
	}

	return CommandCost();
}

/**
 * Sell a (single) train wagon/engine.
 * @param flags type of operation
 * @param t     the train wagon to sell
 * @param data  the selling mode
 * - data = 0: only sell the single dragged wagon/engine (and any belonging rear-engines)
 * - data = 1: sell the vehicle and all vehicles following it in the chain
 *             if the wagon is dragged, don't delete the possibly belonging rear-engine to some front
 * @param user  the user for the order backup.
 * @return the cost of this operation or an error
 */
CommandCost CmdSellRailWagon(DoCommandFlag flags, Vehicle *t, uint16 data, uint32 user)
{
	/* Sell a chain of vehicles or not? */
	bool sell_chain = HasBit(data, 0);

	Train *v = Train::From(t)->GetFirstEnginePart();
	Train *first = v->First();

	if (v->IsRearDualheaded()) return_cmd_error(STR_ERROR_REAR_ENGINE_FOLLOW_FRONT);

	/* First make a backup of the order of the train. That way we can do
	 * whatever we want with the order and later on easily revert. */
	TrainList original;
	MakeTrainBackup(original, first);

	/* We need to keep track of the new head and the head of what we're going to sell. */
	Train *new_head = first;
	Train *sell_head = NULL;

	/* Split the train in the wanted way. */
	ArrangeTrains(&sell_head, NULL, &new_head, v, sell_chain);

	/* We don't need to validate the second train; it's going to be sold. */
	CommandCost ret = ValidateTrains(NULL, NULL, first, new_head, (flags & DC_AUTOREPLACE) == 0);
	if (ret.Failed()) {
		/* Restore the train we had. */
		RestoreTrainBackup(original);
		return ret;
	}

	CommandCost cost(EXPENSES_NEW_VEHICLES);
	for (Train *t = sell_head; t != NULL; t = t->Next()) cost.AddCost(-t->value);

	if (first->orders.list == NULL && !OrderList::CanAllocateItem()) {
		return_cmd_error(STR_ERROR_NO_MORE_SPACE_FOR_ORDERS);
	}

	/* do it? */
	if (flags & DC_EXEC) {
		/* First normalise the sub types of the chain. */
		NormaliseSubtypes(new_head);

		if (v == first && v->IsEngine() && !sell_chain && new_head != NULL && new_head->IsFrontEngine()) {
			/* We are selling the front engine. In this case we want to
			 * 'give' the order, unit number and such to the new head. */
			new_head->orders.list = first->orders.list;
			new_head->AddToShared(first);
			DeleteVehicleOrders(first);

			/* Copy other important data from the front engine */
			new_head->CopyVehicleConfigAndStatistics(first);
			GroupStatistics::CountVehicle(new_head, 1); // after copying over the profit
		} else if (v->IsPrimaryVehicle() && data & (MAKE_ORDER_BACKUP_FLAG >> 20)) {
			OrderBackup::Backup(v, user);
		}

		/* We need to update the information about the train. */
		NormaliseTrainHead(new_head);

		/* We are undoubtedly changing something in the depot and train list. */
		InvalidateWindowData(WC_VEHICLE_DEPOT, v->tile);
		InvalidateWindowClassesData(WC_TRAINS_LIST, 0);

		/* Actually delete the sold 'goods' */
		delete sell_head;
	} else {
		/* We don't want to execute what we're just tried. */
		RestoreTrainBackup(original);
	}

	return cost;
}

void Train::UpdateDeltaXY(Direction direction)
{
	/* Set common defaults. */
	this->x_offs    = -1;
	this->y_offs    = -1;
	this->x_extent  =  3;
	this->y_extent  =  3;
	this->z_extent  =  6;
	this->x_bb_offs =  0;
	this->y_bb_offs =  0;

	if (!IsDiagonalDirection(direction)) {
		static const int _sign_table[] =
		{
			// x, y
			-1, -1, // DIR_N
			-1,  1, // DIR_E
			 1,  1, // DIR_S
			 1, -1, // DIR_W
		};

		int half_shorten = (VEHICLE_LENGTH - this->gcache.cached_veh_length) / 2;

		/* For all straight directions, move the bound box to the centre of the vehicle, but keep the size. */
		this->x_offs -= half_shorten * _sign_table[direction];
		this->y_offs -= half_shorten * _sign_table[direction + 1];
		this->x_extent += this->x_bb_offs = half_shorten * _sign_table[direction];
		this->y_extent += this->y_bb_offs = half_shorten * _sign_table[direction + 1];
	} else {
		switch (direction) {
				/* Shorten southern corner of the bounding box according the vehicle length
				 * and center the bounding box on the vehicle. */
			case DIR_NE:
				this->x_offs    = 1 - (this->gcache.cached_veh_length + 1) / 2;
				this->x_extent  = this->gcache.cached_veh_length - 1;
				this->x_bb_offs = -1;
				break;

			case DIR_NW:
				this->y_offs    = 1 - (this->gcache.cached_veh_length + 1) / 2;
				this->y_extent  = this->gcache.cached_veh_length - 1;
				this->y_bb_offs = -1;
				break;

				/* Move northern corner of the bounding box down according to vehicle length
				 * and center the bounding box on the vehicle. */
			case DIR_SW:
				this->x_offs    = 1 + (this->gcache.cached_veh_length + 1) / 2 - VEHICLE_LENGTH;
				this->x_extent  = VEHICLE_LENGTH - 1;
				this->x_bb_offs = VEHICLE_LENGTH - this->gcache.cached_veh_length - 1;
				break;

			case DIR_SE:
				this->y_offs    = 1 + (this->gcache.cached_veh_length + 1) / 2 - VEHICLE_LENGTH;
				this->y_extent  = VEHICLE_LENGTH - 1;
				this->y_bb_offs = VEHICLE_LENGTH - this->gcache.cached_veh_length - 1;
				break;

			default:
				NOT_REACHED();
		}
	}
}

/**
 * Mark a train as stuck and stop it if it isn't stopped right now.
 * @param v %Train to mark as being stuck.
 */
static void MarkTrainAsStuck(Train *v)
{
	if (!HasBit(v->flags, VRF_TRAIN_STUCK)) {
		/* It is the first time the problem occurred, set the "train stuck" flag. */
		SetBit(v->flags, VRF_TRAIN_STUCK);

		v->wait_counter = 0;

		/* Stop train */
		v->cur_speed = 0;
		v->subspeed = 0;
		v->SetLastSpeed();

		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}
}

/**
 * Swap the two up/down flags in two ways:
 * - Swap values of \a swap_flag1 and \a swap_flag2, and
 * - If going up previously (#GVF_GOINGUP_BIT set), the #GVF_GOINGDOWN_BIT is set, and vice versa.
 * @param swap_flag1 [inout] First train flag.
 * @param swap_flag2 [inout] Second train flag.
 */
static void SwapTrainFlags(uint16 *swap_flag1, uint16 *swap_flag2)
{
	uint16 flag1 = *swap_flag1;
	uint16 flag2 = *swap_flag2;

	/* Clear the flags */
	ClrBit(*swap_flag1, GVF_GOINGUP_BIT);
	ClrBit(*swap_flag1, GVF_GOINGDOWN_BIT);
	ClrBit(*swap_flag2, GVF_GOINGUP_BIT);
	ClrBit(*swap_flag2, GVF_GOINGDOWN_BIT);

	/* Reverse the rail-flags (if needed) */
	if (HasBit(flag1, GVF_GOINGUP_BIT)) {
		SetBit(*swap_flag2, GVF_GOINGDOWN_BIT);
	} else if (HasBit(flag1, GVF_GOINGDOWN_BIT)) {
		SetBit(*swap_flag2, GVF_GOINGUP_BIT);
	}
	if (HasBit(flag2, GVF_GOINGUP_BIT)) {
		SetBit(*swap_flag1, GVF_GOINGDOWN_BIT);
	} else if (HasBit(flag2, GVF_GOINGDOWN_BIT)) {
		SetBit(*swap_flag1, GVF_GOINGUP_BIT);
	}
}

/**
 * Updates some variables after swapping the vehicle.
 * @param v swapped vehicle
 */
static void UpdateStatusAfterSwap(Train *v)
{
	/* Reverse the direction. */
	if (v->trackdir != TRACKDIR_DEPOT) v->direction = ReverseDir(v->direction);

	if (v->trackdir < TRACKDIR_END) v->trackdir = ReverseTrackdir(v->trackdir);

	/* Call the proper EnterTile function unless we are in a wormhole. */
	if (v->trackdir != TRACKDIR_WORMHOLE) {
		TrainEnterTile(v, v->tile, v->x_pos, v->y_pos);
	} else {
		assert(v->direction == DiagDirToDir(GetTunnelBridgeDirection(v->tile)));
		v->tile = GetOtherTunnelBridgeEnd(v->tile);
	}

	VehicleUpdatePosition(v);
	v->UpdateViewport(true, true);
}

/**
 * Swap vehicles \a l and \a r in consist \a v, and reverse their direction.
 * @param v Consist to change.
 * @param l %Vehicle index in the consist of the first vehicle.
 * @param r %Vehicle index in the consist of the second vehicle.
 */
void ReverseTrainSwapVeh(Train *v, int l, int r)
{
	Train *a, *b;

	/* locate vehicles to swap */
	for (a = v; l != 0; l--) a = a->Next();
	for (b = v; r != 0; r--) b = b->Next();

	if (a != b) {
		/* swap the hidden bits */
		{
			uint16 tmp = (a->vehstatus & ~VS_HIDDEN) | (b->vehstatus & VS_HIDDEN);
			b->vehstatus = (b->vehstatus & ~VS_HIDDEN) | (a->vehstatus & VS_HIDDEN);
			a->vehstatus = tmp;
		}

		Swap(a->trackdir, b->trackdir);
		Swap(a->direction, b->direction);
		Swap(a->x_pos, b->x_pos);
		Swap(a->y_pos, b->y_pos);
		Swap(a->tile,  b->tile);
		Swap(a->z_pos, b->z_pos);

		SwapTrainFlags(&a->gv_flags, &b->gv_flags);

		UpdateStatusAfterSwap(a);
		UpdateStatusAfterSwap(b);
	} else {
		/* Swap GVF_GOINGUP_BIT/GVF_GOINGDOWN_BIT.
		 * This is a little bit redundant way, a->gv_flags will
		 * be (re)set twice, but it reduces code duplication */
		SwapTrainFlags(&a->gv_flags, &a->gv_flags);
		UpdateStatusAfterSwap(a);
	}
}


/**
 * Check if there is a train on a tile
 * @param tile the tile to check
 * @return true if a train is on the tile
 */
static bool TrainOnTile (TileIndex tile)
{
	VehicleTileFinder iter (tile);
	while (!iter.finished()) {
		Vehicle *v = iter.next();
		if (v->type == VEH_TRAIN) iter.set_found();
	}
	return iter.was_found();
}

/**
 * Checks whether a train is approaching a rail-road crossing from a neighbour tile
 * @param tile the crossing tile
 * @param from the neighbour tile
 * @return true if a train is approaching the crossing
 */
static bool TrainApproachingCrossingFrom (TileIndex tile, TileIndex from)
{
	VehicleTileFinder iter (from);
	while (!iter.finished()) {
		Vehicle *v = iter.next();
		if (v->type != VEH_TRAIN || (v->vehstatus & VS_CRASHED)) continue;

		Train *t = Train::From(v);
		if (t->IsFrontEngine() && TrainApproachingCrossingTile(t) == tile) iter.set_found();
	}
	return iter.was_found();
}

/**
 * Finds a vehicle approaching rail-road crossing
 * @param tile tile to test
 * @return true if a vehicle is approaching the crossing
 * @pre tile is a rail-road crossing
 */
static bool TrainApproachingCrossing(TileIndex tile)
{
	assert(IsLevelCrossingTile(tile));

	TileIndexDiff delta = TileOffsByDiagDir(AxisToDiagDir(GetCrossingRailAxis(tile)));
	return  TrainApproachingCrossingFrom (tile, tile + delta) ||
		TrainApproachingCrossingFrom (tile, tile - delta);
}


/**
 * Sets correct crossing state
 * @param tile tile to update
 * @param sound should we play sound?
 * @pre tile is a rail-road crossing
 */
void UpdateLevelCrossing(TileIndex tile, bool sound)
{
	assert(IsLevelCrossingTile(tile));

	/* reserved || train on crossing || train approaching crossing */
	bool new_state = HasCrossingReservation(tile) || TrainOnTile(tile) || TrainApproachingCrossing(tile);

	if (new_state != IsCrossingBarred(tile)) {
		if (new_state && sound) {
			if (_settings_client.sound.ambient) SndPlayTileFx(SND_0E_LEVEL_CROSSING, tile);
		}
		SetCrossingBarred(tile, new_state);
		MarkTileDirtyByTile(tile);
	}
}


/**
 * Bars crossing and plays ding-ding sound if not barred already
 * @param tile tile with crossing
 * @pre tile is a rail-road crossing
 */
static inline void MaybeBarCrossingWithSound(TileIndex tile)
{
	if (!IsCrossingBarred(tile)) {
		BarCrossing(tile);
		if (_settings_client.sound.ambient) SndPlayTileFx(SND_0E_LEVEL_CROSSING, tile);
		MarkTileDirtyByTile(tile);
	}
}


/**
 * Advances wagons for train reversing, needed for variable length wagons.
 * This one is called before the train is reversed.
 * @param v First vehicle in chain
 */
static void AdvanceWagonsBeforeSwap(Train *v)
{
	Train *base = v;
	Train *first = base; // first vehicle to move
	Train *last = v->Last(); // last vehicle to move
	uint length = CountVehiclesInChain(v);

	while (length > 2) {
		last = last->Previous();
		first = first->Next();

		int differential = base->CalcNextVehicleOffset() - last->CalcNextVehicleOffset();

		/* do not update images now
		 * negative differential will be handled in AdvanceWagonsAfterSwap() */
		for (int i = 0; i < differential; i++) TrainController(first, last->Next());

		base = first; // == base->Next()
		length -= 2;
	}
}


/**
 * Advances wagons for train reversing, needed for variable length wagons.
 * This one is called after the train is reversed.
 * @param v First vehicle in chain
 */
static void AdvanceWagonsAfterSwap(Train *v)
{
	/* first of all, fix the situation when the train was entering a depot */
	Train *dep = v; // last vehicle in front of just left depot
	while (dep->Next() != NULL && (dep->trackdir == TRACKDIR_DEPOT || dep->Next()->trackdir != TRACKDIR_DEPOT)) {
		dep = dep->Next(); // find first vehicle outside of a depot, with next vehicle inside a depot
	}

	Train *leave = dep->Next(); // first vehicle in a depot we are leaving now

	if (leave != NULL) {
		/* 'pull' next wagon out of the depot, so we won't miss it (it could stay in depot forever) */
		int d = TicksToLeaveDepot(dep);

		if (d <= 0) {
			leave->vehstatus &= ~VS_HIDDEN; // move it out of the depot
			leave->trackdir = DiagDirToDiagTrackdir(GetGroundDepotDirection(leave->tile));
			for (int i = 0; i >= d; i--) TrainController(leave, NULL); // maybe move it, and maybe let another wagon leave
		}
	} else {
		dep = NULL; // no vehicle in a depot, so no vehicle leaving a depot
	}

	Train *base = v;
	Train *first = base; // first vehicle to move
	Train *last = v->Last(); // last vehicle to move
	uint length = CountVehiclesInChain(v);

	/* We have to make sure all wagons that leave a depot because of train reversing are moved correctly
	 * they have already correct spacing, so we have to make sure they are moved how they should */
	bool nomove = (dep == NULL); // If there is no vehicle leaving a depot, limit the number of wagons moved immediately.

	while (length > 2) {
		/* we reached vehicle (originally) in front of a depot, stop now
		 * (we would move wagons that are already moved with new wagon length). */
		if (base == dep) break;

		/* the last wagon was that one leaving a depot, so do not move it anymore */
		if (last == dep) nomove = true;

		last = last->Previous();
		first = first->Next();

		int differential = last->CalcNextVehicleOffset() - base->CalcNextVehicleOffset();

		/* do not update images now */
		for (int i = 0; i < differential; i++) TrainController(first, (nomove ? last->Next() : NULL));

		base = first; // == base->Next()
		length -= 2;
	}
}

/**
 * Turn a train around.
 * @param v %Train to turn around.
 */
void ReverseTrainDirection(Train *v)
{
	if (IsRailDepotTile(v->tile)) {
		InvalidateWindowData(WC_VEHICLE_DEPOT, v->tile);
	}

	/* Clear path reservation in front if train is not stuck. */
	if (!HasBit(v->flags, VRF_TRAIN_STUCK)) FreeTrainTrackReservation(v);

	/* Check if we were approaching a rail/road-crossing */
	TileIndex crossing = TrainApproachingCrossingTile(v);

	/* count number of vehicles */
	int r = CountVehiclesInChain(v) - 1;  // number of vehicles - 1

	AdvanceWagonsBeforeSwap(v);

	/* swap start<>end, start+1<>end-1, ... */
	int l = 0;
	do {
		ReverseTrainSwapVeh(v, l++, r--);
	} while (l <= r);

	AdvanceWagonsAfterSwap(v);

	if (IsRailDepotTile(v->tile)) {
		InvalidateWindowData(WC_VEHICLE_DEPOT, v->tile);
	}

	ToggleBit(v->flags, VRF_TOGGLE_REVERSE);

	ClrBit(v->flags, VRF_REVERSING);

	/* recalculate cached data */
	v->ConsistChanged(true);

	/* update all images */
	for (Train *u = v; u != NULL; u = u->Next()) u->UpdateViewport(false, false);

	/* update crossing we were approaching */
	if (crossing != INVALID_TILE) UpdateLevelCrossing(crossing);

	/* maybe we are approaching crossing now, after reversal */
	crossing = TrainApproachingCrossingTile(v);
	if (crossing != INVALID_TILE) MaybeBarCrossingWithSound(crossing);

	/* If we are inside a depot after reversing, don't bother with path reserving. */
	if (v->trackdir == TRACKDIR_DEPOT) {
		/* Can't be stuck here as inside a depot is always a safe tile. */
		if (HasBit(v->flags, VRF_TRAIN_STUCK)) SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		ClrBit(v->flags, VRF_TRAIN_STUCK);
		return;
	}

	assert(IsSignalBufferEmpty());
	AddPosToSignalBuffer(v->GetPos(), v->owner);

	if (UpdateSignalsInBuffer() == SIGSEG_PBS || _settings_game.pf.reserve_paths) {
		PathPos pos = v->GetPos();

		/* If we are currently on a tile with conventional signals, we can't treat the
		 * current tile as a safe tile or we would enter a PBS block without a reservation. */
		bool first_tile_okay = !(HasSignalAlongPos(pos) &&
			!IsPbsSignal(GetSignalType(pos)));

		/* If we are on a depot tile facing outwards, do not treat the current tile as safe. */
		if (!pos.InWormhole() && IsRailDepotTile(pos.tile) && TrackdirToExitdir(pos.td) == GetGroundDepotDirection(pos.tile)) first_tile_okay = false;

		if (!pos.InWormhole() && IsRailStationTile(pos.tile)) SetRailStationPlatformReservation(pos, true);
		if (TryPathReserve(v, false, first_tile_okay)) {
			/* Do a look-ahead now in case our current tile was already a safe tile. */
			CheckNextTrainTile(v);
		} else if (v->current_order.GetType() != OT_LOADING) {
			/* Do not wait for a way out when we're still loading */
			MarkTrainAsStuck(v);
		}
	} else if (HasBit(v->flags, VRF_TRAIN_STUCK)) {
		/* A train not inside a PBS block can't be stuck. */
		ClrBit(v->flags, VRF_TRAIN_STUCK);
		v->wait_counter = 0;
	}
}

/**
 * Reverse train.
 * @param tile unused
 * @param flags type of operation
 * @param p1 train to reverse
 * @param p2 if true, reverse a unit in a train (needs to be in a depot)
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdReverseTrainDirection(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Train *v = Train::GetIfValid(p1);
	if (v == NULL) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (p2 != 0) {
		/* turn a single unit around */

		if (v->IsMultiheaded() || HasBit(EngInfo(v->engine_type)->callback_mask, CBM_VEHICLE_ARTIC_ENGINE)) {
			return_cmd_error(STR_ERROR_CAN_T_REVERSE_DIRECTION_RAIL_VEHICLE_MULTIPLE_UNITS);
		}
		if (!HasBit(EngInfo(v->engine_type)->misc_flags, EF_RAIL_FLIPS)) return CMD_ERROR;

		Train *front = v->First();
		/* make sure the vehicle is stopped in the depot */
		if (!front->IsStoppedInDepot()) {
			return_cmd_error(STR_ERROR_TRAINS_CAN_ONLY_BE_ALTERED_INSIDE_A_DEPOT);
		}

		if (flags & DC_EXEC) {
			ToggleBit(v->flags, VRF_REVERSE_DIRECTION);

			front->ConsistChanged(false);
			SetWindowDirty(WC_VEHICLE_DEPOT, front->tile);
			SetWindowDirty(WC_VEHICLE_DETAILS, front->index);
			SetWindowDirty(WC_VEHICLE_VIEW, front->index);
			SetWindowClassesDirty(WC_TRAINS_LIST);
		}
	} else {
		/* turn the whole train around */
		if ((v->vehstatus & VS_CRASHED) || v->breakdown_ctr != 0) return CMD_ERROR;

		if (flags & DC_EXEC) {
			/* Properly leave the station if we are loading and won't be loading anymore */
			if (v->current_order.IsType(OT_LOADING)) {
				const Vehicle *last = v;
				while (last->Next() != NULL) last = last->Next();

				/* not a station || different station --> leave the station */
				if (!IsStationTile(last->tile) || GetStationIndex(last->tile) != GetStationIndex(v->tile)) {
					v->LeaveStation();
				}
			}

			/* We cancel any 'skip signal at dangers' here */
			v->force_proceed = TFP_NONE;
			SetWindowDirty(WC_VEHICLE_VIEW, v->index);

			if (_settings_game.vehicle.train_acceleration_model != AM_ORIGINAL && v->cur_speed != 0) {
				ToggleBit(v->flags, VRF_REVERSING);
			} else {
				v->cur_speed = 0;
				v->SetLastSpeed();
				HideFillingPercent(&v->fill_percent_te_id);
				ReverseTrainDirection(v);
			}
		}
	}
	return CommandCost();
}

/**
 * Force a train through a red signal
 * @param tile unused
 * @param flags type of operation
 * @param p1 train to ignore the red signal
 * @param p2 unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdForceTrainProceed(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Train *t = Train::GetIfValid(p1);
	if (t == NULL) return CMD_ERROR;

	if (!t->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(t->owner);
	if (ret.Failed()) return ret;


	if (flags & DC_EXEC) {
		/* If we are forced to proceed, cancel that order.
		 * If we are marked stuck we would want to force the train
		 * to proceed to the next signal. In the other cases we
		 * would like to pass the signal at danger and run till the
		 * next signal we encounter. */
		t->force_proceed = t->force_proceed == TFP_SIGNAL ? TFP_NONE : HasBit(t->flags, VRF_TRAIN_STUCK) || t->IsChainInDepot() ? TFP_STUCK : TFP_SIGNAL;
		SetWindowDirty(WC_VEHICLE_VIEW, t->index);
	}

	return CommandCost();
}

/**
 * Try to find a depot.
 * @param v %Train that wants a depot.
 * @param nearby Only consider nearby depots.
 * @param res Pointer to store information where the closest train depot is located.
 * @return Whether a depot was found.
 * @pre The given vehicle must not be crashed!
 */
static bool FindClosestTrainDepot(Train *v, bool nearby, FindDepotData *res)
{
	assert(!(v->vehstatus & VS_CRASHED));

	if (IsRailDepotTile(v->tile) && v->trackdir == DiagDirToDiagTrackdir(ReverseDiagDir(GetGroundDepotDirection(v->tile)))) {
		*res = FindDepotData(v->tile);
		return true;
	}

	PathPos origin;
	FollowTrainReservation(v, &origin);
	if (IsRailDepotTile(origin.tile) && origin.td == DiagDirToDiagTrackdir(ReverseDiagDir(GetGroundDepotDirection(origin.tile)))) {
		*res = FindDepotData(origin.tile);
		return true;
	}

	switch (_settings_game.pf.pathfinder_for_trains) {
		case VPF_NPF:
			return NPFTrainFindNearestDepot(v,
				nearby ? _settings_game.pf.npf.maximum_go_to_depot_penalty : 0, res);

		case VPF_YAPF:
			return YapfTrainFindNearestDepot(v,
				nearby ? _settings_game.pf.yapf.maximum_go_to_depot_penalty : 0, res);

		default: NOT_REACHED();
	}
}

/**
 * Locate the closest depot for this consist, and return the information to the caller.
 * @param location [out]    If not \c NULL and a depot is found, store its location in the given address.
 * @param destination [out] If not \c NULL and a depot is found, store its index in the given address.
 * @param reverse [out]     If not \c NULL and a depot is found, store reversal information in the given address.
 * @return A depot has been found.
 */
bool Train::FindClosestDepot(TileIndex *location, DestinationID *destination, bool *reverse)
{
	FindDepotData tfdd;
	if (!FindClosestTrainDepot(this, false, &tfdd)) return false;

	if (location    != NULL) *location    = tfdd.tile;
	if (destination != NULL) *destination = GetDepotIndex(tfdd.tile);
	if (reverse     != NULL) *reverse     = tfdd.reverse;

	return true;
}

/** Play a sound for a train leaving the station. */
void Train::PlayLeaveStationSound() const
{
	static const SoundFx sfx[] = {
		SND_04_TRAIN,
		SND_0A_TRAIN_HORN,
		SND_0A_TRAIN_HORN,
		SND_47_MAGLEV_2,
		SND_41_MAGLEV
	};

	if (PlayVehicleSound(this, VSE_START)) return;

	EngineID engtype = this->engine_type;
	SndPlayVehicleFx(sfx[RailVehInfo(engtype)->engclass], this);
}

/**
 * Check if the train is on the last reserved tile and try to extend the path then.
 * @param v %Train that needs its path extended.
 */
static void CheckNextTrainTile(Train *v)
{
	/* Don't do any look-ahead if path_backoff_interval is 255. */
	if (_settings_game.pf.path_backoff_interval == 255) return;

	/* Exit if we are inside a depot. */
	if (v->trackdir == TRACKDIR_DEPOT) return;

	switch (v->current_order.GetType()) {
		/* Exit if we reached our destination depot. */
		case OT_GOTO_DEPOT:
			if (v->tile == v->dest_tile) return;
			break;

		case OT_GOTO_WAYPOINT:
			/* If we reached our waypoint, make sure we see that. */
			if (IsRailWaypointTile(v->tile) && GetStationIndex(v->tile) == v->current_order.GetDestination()) ProcessOrders(v);
			break;

		case OT_NOTHING:
		case OT_LEAVESTATION:
		case OT_LOADING:
			/* Exit if the current order doesn't have a destination, but the train has orders. */
			if (v->GetNumOrders() > 0) return;
			break;

		default:
			break;
	}
	/* Exit if we are on a station tile and are going to stop. */
	if (IsRailStationTile(v->tile) && v->current_order.ShouldStopAtStation(v, GetStationIndex(v->tile))) return;

	PathPos pos = v->GetPos();

	/* On a tile with a red non-pbs signal, don't look ahead. */
	if (HasSignalAlongPos(pos) &&
			!IsPbsSignal(GetSignalType(pos)) &&
			GetSignalStateByPos(pos) == SIGNAL_STATE_RED) return;

	CFollowTrackRail ft(v, !_settings_game.pf.forbid_90_deg);
	if (!ft.Follow(pos)) return;

	if (ft.m_new.IsTrackdirSet()) {
		/* Next tile is not reserved. */
		if (!HasReservedPos(ft.m_new)) {
			if (HasPbsSignalAlongPos(ft.m_new)) {
				/* If the next tile is a PBS signal, try to make a reservation. */
				ChooseTrainTrack(v, pos, ft.m_new.tile, ft.m_new.trackdirs, false);
			}
		}
	}
}

/**
 * Will the train stay in the depot the next tick?
 * @param v %Train to check.
 * @return True if it stays in the depot, false otherwise.
 */
static bool CheckTrainStayInDepot(Train *v)
{
	/* bail out if not all wagons are in the same depot or not in a depot at all */
	for (const Train *u = v; u != NULL; u = u->Next()) {
		if (u->trackdir != TRACKDIR_DEPOT || u->tile != v->tile) return false;
	}

	/* if the train got no power, then keep it in the depot */
	if (v->gcache.cached_power == 0) {
		v->vehstatus |= VS_STOPPED;
		SetWindowDirty(WC_VEHICLE_DEPOT, v->tile);
		return true;
	}

	bool try_reserve;

	if (v->force_proceed == TFP_NONE) {
		/* force proceed was not pressed */
		if (++v->wait_counter < 37) {
			SetWindowClassesDirty(WC_TRAINS_LIST);
			return true;
		}

		v->wait_counter = 0;

		if (HasDepotReservation(v->tile)) {
			/* Depot reserved, can't exit. */
			SetWindowClassesDirty(WC_TRAINS_LIST);
			return true;
		}

		if (_settings_game.pf.reserve_paths) {
			try_reserve = true;
		} else {
			assert(IsSignalBufferEmpty());
			AddDepotToSignalBuffer(v->tile, v->owner);
			SigSegState seg_state = UpdateSignalsInBuffer();
			if (seg_state == SIGSEG_FULL) {
				/* Full and no PBS signal in block, can't exit. */
				SetWindowClassesDirty(WC_TRAINS_LIST);
				return true;
			}
			try_reserve = seg_state == SIGSEG_PBS;
		}
	} else if (_settings_game.pf.reserve_paths) {
		try_reserve = true;
	} else {
		assert(IsSignalBufferEmpty());
		AddDepotToSignalBuffer(v->tile, v->owner);
		try_reserve = UpdateSignalsInBuffer() == SIGSEG_PBS;
	}

	/* We are leaving a depot, but have to go to the exact same one; re-enter */
	if (v->current_order.IsType(OT_GOTO_DEPOT) && v->tile == v->dest_tile) {
		/* We need to have a reservation for this to work. */
		if (HasDepotReservation(v->tile)) return true;
		SetDepotReservation(v->tile, true);
		VehicleEnterDepot(v);
		return true;
	}

	/* Only leave when we can reserve a path to our destination. */
	if (try_reserve && !TryPathReserveFromDepot(v) && v->force_proceed == TFP_NONE) {
		/* No path and no force proceed. */
		SetWindowClassesDirty(WC_TRAINS_LIST);
		MarkTrainAsStuck(v);
		return true;
	}

	SetDepotReservation(v->tile, true);
	if (_settings_client.gui.show_track_reservation) MarkTileDirtyByTile(v->tile);

	VehicleServiceInDepot(v);
	SetWindowClassesDirty(WC_TRAINS_LIST);
	v->PlayLeaveStationSound();

	v->trackdir = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));

	v->vehstatus &= ~VS_HIDDEN;
	v->cur_speed = 0;

	v->UpdateViewport(true, true);
	VehicleUpdatePosition(v);

	assert(IsSignalBufferEmpty());
	AddDepotToSignalBuffer(v->tile, v->owner);
	UpdateSignalsInBuffer();

	v->UpdateAcceleration();
	InvalidateWindowData(WC_VEHICLE_DEPOT, v->tile);

	return false;
}

/**
 * Clear the reservation of \a tile that was just left by a wagon on \a track_dir.
 * @param v %Train owning the reservation.
 * @param pos position to clear.
 */
static void ClearPathReservation(const Train *v, const PathPos &pos)
{
	DiagDirection dir = TrackdirToExitdir(pos.td);

	if (pos.InWormhole()) {
		UnreserveRailTrack(pos);
	} else if (IsRailStationTile(pos.tile)) {
		TileIndex new_tile = TileAddByDiagDir(pos.tile, dir);
		/* If the new tile is not a further tile of the same station, we
		 * clear the reservation for the whole platform. */
		if (!IsCompatibleTrainStationTile(new_tile, pos.tile)) {
			SetRailStationPlatformReservation(pos.tile, ReverseDiagDir(dir), false);
		}
	} else {
		/* Any other tile */
		UnreserveRailTrack(pos);
	}
}

/**
 * Free the reserved path in front of a vehicle.
 * @param v %Train owning the reserved path.
 */
void FreeTrainTrackReservation(const Train *v)
{
	assert(v->IsFrontEngine());

	PathPos   pos = v->GetPos();
	bool      first = true;

	/* Can't be holding a reservation if we enter a depot. */
	if (IsRailDepotTile(pos.tile) && TrackdirToExitdir(pos.td) != GetGroundDepotDirection(pos.tile)) return;
	if (v->trackdir == TRACKDIR_DEPOT) {
		/* Front engine is in a depot. We enter if some part is not in the depot. */
		for (const Train *u = v; u != NULL; u = u->Next()) {
			if (u->trackdir != TRACKDIR_DEPOT || u->tile != v->tile) return;
		}
	}
	/* Don't free reservation if it's not ours. */
	if (!pos.InWormhole() && TracksOverlap(GetReservedTrackbits(pos.tile) | TrackToTrackBits(TrackdirToTrack(pos.td)))) return;

	CFollowTrackRail ft(v, true, true);
	ft.SetPos(pos);

	while (ft.FollowNext()) {
		if (!ft.m_new.InWormhole()) {
			ft.m_new.trackdirs &= TrackBitsToTrackdirBits(GetReservedTrackbits(ft.m_new.tile));
			if (ft.m_new.trackdirs == TRACKDIR_BIT_NONE) break;
			assert(KillFirstBit(ft.m_new.trackdirs) == TRACKDIR_BIT_NONE);
			ft.m_new.td = FindFirstTrackdir(ft.m_new.trackdirs);
		}

		if (HasSignalAlongPos(ft.m_new) && !IsPbsSignal(GetSignalType(ft.m_new))) {
			/* Conventional signal along trackdir: remove reservation and stop. */
			UnreserveRailTrack(ft.m_new);
			break;
		}

		if (HasPbsSignalAlongPos(ft.m_new)) {
			if (GetSignalStateByPos(ft.m_new) == SIGNAL_STATE_RED) {
				/* Red PBS signal? Can't be our reservation, would be green then. */
				break;
			} else {
				/* Turn the signal back to red. */
				SetSignalState(ft.m_new.tile, ft.m_new.td, SIGNAL_STATE_RED);
				MarkTileDirtyByTile(ft.m_new.tile);
			}
		} else if (HasSignalAgainstPos(ft.m_new) && IsOnewaySignal(GetSignalType(ft.m_new))) {
			break;
		}

		if (first) {
			if (ft.m_flag == ft.TF_BRIDGE) {
				assert(IsRailBridgeTile(ft.m_old.InWormhole() ? ft.m_old.wormhole : ft.m_old.tile));
			} else if (ft.m_flag == ft.TF_TUNNEL) {
				assert(IsTunnelTile(ft.m_old.InWormhole() ? ft.m_old.wormhole : ft.m_old.tile));
			}
		}

		/* Don't free first station if we are on it. */
		if (!first || (ft.m_flag != ft.TF_STATION) ||
				!IsRailStationTile(ft.m_old.tile) || GetStationIndex(ft.m_new.tile) != GetStationIndex(ft.m_old.tile)) {
			ClearPathReservation(v, ft.m_new);
		}

		first = false;
	}
}

static const byte _initial_tile_subcoord[TRACKDIR_END][3] = {
	{15, 8, 1},  // TRACKDIR_X_NE
	{ 8, 0, 3},  // TRACKDIR_Y_SE
	{ 7, 0, 2},  // TRACKDIR_UPPER_E
	{15, 8, 2},  // TRACKDIR_LOWER_E
	{ 8, 0, 4},  // TRACKDIR_LEFT_S
	{ 0, 8, 4},  // TRACKDIR_RIGHT_S
	{ 0, 0, 0},
	{ 0, 0, 0},
	{ 0, 8, 5},  // TRACKDIR_X_SW
	{ 8,15, 7},  // TRACKDIR_Y_NW
	{ 0, 7, 6},  // TRACKDIR_UPPER_W
	{ 8,15, 6},  // TRACKDIR_LOWER_W
	{15, 7, 0},  // TRACKDIR_LEFT_N
	{ 7,15, 0},  // TRACKDIR_RIGHT_N
};

/**
 * Perform pathfinding for a train.
 *
 * @param v The train
 * @param origin The end of the current reservation
 * @param do_track_reservation Path reservation is requested
 * @param dest [out] State and destination of the requested path
 * @return The best trackdir the train should follow
 */
static Trackdir DoTrainPathfind(const Train *v, const PathPos &origin, bool do_track_reservation, PFResult *dest)
{
	switch (_settings_game.pf.pathfinder_for_trains) {
		case VPF_NPF: return NPFTrainChooseTrack(v, origin, do_track_reservation, dest);
		case VPF_YAPF: return YapfTrainChooseTrack(v, origin, do_track_reservation, dest);

		default: NOT_REACHED();
	}
}

/**
 * Return value type for ExtendTrainReservation
 */
enum ExtendReservationResult {
	EXTEND_RESERVATION_SAFE,   ///< Reservation extended to a safe tile
	EXTEND_RESERVATION_UNSAFE, ///< Reservation extended to an unsafe tile
	EXTEND_RESERVATION_FAILED, ///< Reservation could not be extended
};

/**
 * Extend a train path as far as possible. Stops on encountering a safe tile,
 * another reservation or a track choice.
 * @param v The train whose reservation to extend
 * @param origin The end of the current reservation, which will be updated
 * @return An ExtendReservationResult value, showing reservation extension outcome
 */
static ExtendReservationResult ExtendTrainReservation(const Train *v, PathPos *origin)
{
	CFollowTrackRail ft(v, !_settings_game.pf.forbid_90_deg);
	ft.SetPos(*origin);

	for (;;) {
		if (!ft.FollowNext()) {
			if (ft.m_err == CFollowTrackRail::EC_OWNER || ft.m_err == CFollowTrackRail::EC_NO_WAY) {
				/* End of line, path valid and okay. */
				*origin = ft.m_old;
				return EXTEND_RESERVATION_SAFE;
			}
			break;
		}

		/* A depot is always a safe waiting position. */
		if (!ft.m_new.InWormhole() && IsRailDepotTile(ft.m_new.tile)) {
			/* Depot must be free for reservation to continue. */
			if (HasDepotReservation(ft.m_new.tile)) break;

			SetDepotReservation(ft.m_new.tile, true);
			*origin = ft.m_new;
			return EXTEND_RESERVATION_SAFE;
		}

		/* Station and waypoints are possible targets. */
		if (ft.m_flag == ft.TF_STATION) {
			/* Possible target encountered.
			 * On finding a possible target, we need to stop and let the pathfinder handle the
			 * remaining path. This is because we don't know if this target is in one of our
			 * orders, so we might cause pathfinding to fail later on if we find a choice.
			 * This failure would cause a bogous call to TryReserveSafePath which might reserve
			 * a wrong path not leading to our next destination. */
			if (!ft.MaskReservedTracks()) break;

			/* If we did skip some tiles, backtrack to the first skipped tile so the pathfinder
			 * actually starts its search at the first unreserved tile. */
			ft.m_new.tile -= TileOffsByDiagDir(ft.m_exitdir) * ft.m_tiles_skipped;

			/* Possible target found, path valid but not okay. */
			*origin = ft.m_old;
			return EXTEND_RESERVATION_UNSAFE;
		}

		if (!ft.m_new.IsTrackdirSet()) {
			/* Choice found. */
			if (HasReservedTracks(ft.m_new.tile, TrackdirBitsToTrackBits(ft.m_new.trackdirs))) break;

			assert(ft.m_tiles_skipped == 0);

			/* Choice found, path valid but not okay. Save info about the choice tile as well. */
			*origin = ft.m_old;
			return EXTEND_RESERVATION_UNSAFE;
		}

		/* Possible signal tile. */
		if (HasOnewaySignalBlockingPos(ft.m_new)) break;

		PBSPositionState state = CheckWaitingPosition(v, ft.m_new, _settings_game.pf.forbid_90_deg);
		if (state == PBS_BUSY) break;

		if (!TryReserveRailTrack(ft.m_new)) break;

		if (state == PBS_FREE) {
			/* Safe position is all good, path valid and okay. */
			*origin = ft.m_new;
			return EXTEND_RESERVATION_SAFE;
		}
	}

	/* Sorry, can't reserve path, back out. */
	PathPos stopped = ft.m_old;
	ft.SetPos(*origin);
	while (ft.m_new != stopped) {
		if (!ft.FollowNext()) NOT_REACHED();

		assert(ft.m_new.trackdirs != TRACKDIR_BIT_NONE);
		assert(ft.m_new.IsTrackdirSet());

		UnreserveRailTrack(ft.m_new);
	}

	/* Path invalid. */
	return EXTEND_RESERVATION_FAILED;
}

/**
 * Try to reserve any path to a safe tile, ignoring the vehicle's destination.
 * Safe tiles are tiles in front of a signal, depots and station tiles at end of line.
 *
 * @param v The vehicle.
 * @param pos The position the search should start from.
 * @param override_railtype Whether all physically compatible railtypes should be followed.
 * @return True if a path to a safe stopping tile could be reserved.
 */
static bool TryReserveSafeTrack(const Train *v, const PathPos &pos, bool override_tailtype)
{
	switch (_settings_game.pf.pathfinder_for_trains) {
		case VPF_NPF: return NPFTrainFindNearestSafeTile(v, pos, override_tailtype);
		case VPF_YAPF: return YapfTrainFindNearestSafeTile(v, pos, override_tailtype);

		default: NOT_REACHED();
	}
}

/** This class will save the current order of a vehicle and restore it on destruction. */
class VehicleOrderSaver {
private:
	Train          *v;
	Order          old_order;
	TileIndex      old_dest_tile;
	StationID      old_last_station_visited;
	VehicleOrderID index;
	bool           suppress_implicit_orders;

public:
	VehicleOrderSaver(Train *_v) :
		v(_v),
		old_order(_v->current_order),
		old_dest_tile(_v->dest_tile),
		old_last_station_visited(_v->last_station_visited),
		index(_v->cur_real_order_index),
		suppress_implicit_orders(HasBit(_v->gv_flags, GVF_SUPPRESS_IMPLICIT_ORDERS))
	{
	}

	~VehicleOrderSaver()
	{
		this->v->current_order = this->old_order;
		this->v->dest_tile = this->old_dest_tile;
		this->v->last_station_visited = this->old_last_station_visited;
		SB(this->v->gv_flags, GVF_SUPPRESS_IMPLICIT_ORDERS, 1, suppress_implicit_orders ? 1: 0);
	}

	/**
	 * Set the current vehicle order to the next order in the order list.
	 * @param skip_first Shall the first (i.e. active) order be skipped?
	 * @return True if a suitable next order could be found.
	 */
	bool SwitchToNextOrder(bool skip_first)
	{
		if (this->v->GetNumOrders() == 0) return false;

		if (skip_first) ++this->index;

		int depth = 0;

		do {
			/* Wrap around. */
			if (this->index >= this->v->GetNumOrders()) this->index = 0;

			Order *order = this->v->GetOrder(this->index);
			assert(order != NULL);

			switch (order->GetType()) {
				case OT_GOTO_DEPOT:
					/* Skip service in depot orders when the train doesn't need service. */
					if ((order->GetDepotOrderType() & ODTFB_SERVICE) && !this->v->NeedsServicing()) break;
				case OT_GOTO_STATION:
				case OT_GOTO_WAYPOINT:
					this->v->current_order = *order;
					return UpdateOrderDest(this->v, order, 0, true);
				case OT_CONDITIONAL: {
					VehicleOrderID next = ProcessConditionalOrder(order, this->v);
					if (next != INVALID_VEH_ORDER_ID) {
						depth++;
						this->index = next;
						/* Don't increment next, so no break here. */
						continue;
					}
					break;
				}
				default:
					break;
			}
			/* Don't increment inside the while because otherwise conditional
			 * orders can lead to an infinite loop. */
			++this->index;
			depth++;
		} while (this->index != this->v->cur_real_order_index && depth < this->v->GetNumOrders());

		return false;
	}
};

/* choose a track */
static bool ChooseTrainTrack(Train *v, PathPos origin, TileIndex tile, TrackdirBits trackdirs, bool force_res, Trackdir *best_trackdir)
{
	bool do_track_reservation = _settings_game.pf.reserve_paths || force_res;
	bool change_signal = false;

	assert (trackdirs != TRACKDIR_BIT_NONE);

	/* Quick return in case only one possible trackdir is available */
	Trackdir single_trackdir = INVALID_TRACKDIR;
	if (HasAtMostOneBit(trackdirs)) {
		single_trackdir = FindFirstTrackdir(trackdirs);
		if (best_trackdir != NULL) *best_trackdir = single_trackdir;
		/* We need to check for signals only here, as a junction tile can't have signals. */
		if (HasPbsSignalOnTrackdir(tile, single_trackdir)) {
			do_track_reservation = true;
			change_signal = true;
		} else if (!do_track_reservation) {
			return true;
		}
	}

	if (do_track_reservation) {
		switch (ExtendTrainReservation(v, &origin)) {
			default: NOT_REACHED();
			case EXTEND_RESERVATION_FAILED:
				if (best_trackdir != NULL) *best_trackdir = FindFirstTrackdir(trackdirs);
				return false;
			case EXTEND_RESERVATION_SAFE:
				if (change_signal) {
					SetSignalState(tile, single_trackdir, SIGNAL_STATE_GREEN);
					MarkTileDirtyByTile(tile);
				}
				TryReserveRailTrack(v->GetPos());
				assert (single_trackdir != INVALID_TRACKDIR);
				return true;
			case EXTEND_RESERVATION_UNSAFE:
				break;
		}

		/* Check if the train needs service here, so it has a chance to always find a depot.
		 * Also check if the current order is a service order so we don't reserve a path to
		 * the destination but instead to the next one if service isn't needed. */
		CheckIfTrainNeedsService(v);
		if (v->current_order.IsType(OT_DUMMY) || v->current_order.IsType(OT_CONDITIONAL) || v->current_order.IsType(OT_GOTO_DEPOT)) ProcessOrders(v);
	}

	/* Save the current train order. The destructor will restore the old order on function exit. */
	VehicleOrderSaver orders(v);

	/* If the current tile is the destination of the current order and
	 * a reservation was requested, advance to the next order.
	 * Don't advance on a depot order as depots are always safe end points
	 * for a path and no look-ahead is necessary. This also avoids a
	 * problem with depot orders not part of the order list when the
	 * order list itself is empty. */
	if (v->current_order.IsType(OT_LEAVESTATION)) {
		orders.SwitchToNextOrder(false);
	} else if (v->current_order.IsType(OT_LOADING) || (!v->current_order.IsType(OT_GOTO_DEPOT) && (
			v->current_order.IsType(OT_GOTO_STATION) ?
			IsRailStationTile(v->tile) && v->current_order.GetDestination() == GetStationIndex(v->tile) :
			v->tile == v->dest_tile))) {
		orders.SwitchToNextOrder(true);
	}

	/* Call the pathfinder... */
	PFResult res_dest;
	Trackdir next_trackdir = DoTrainPathfind(v, origin, do_track_reservation, &res_dest);
	v->HandlePathfindingResult(res_dest.found);
	/* ...but only use the result if we were at the original tile. */
	if (best_trackdir != NULL && single_trackdir == INVALID_TRACKDIR) {
		/* The initial tile had more than one available trackdir.
		 * This means that ExtendTrainReservation cannot possibly
		 * have extended the reservation, and we are pathfinding
		 * at the original tile. */
		*best_trackdir = next_trackdir != INVALID_TRACKDIR ? next_trackdir : FindFirstTrackdir(trackdirs);
		/* If the initial tile only had one available trackdir, then
		 * *best_trackdir was set along with single_trackdir. */
	}

	/* No track reservation requested -> finished. */
	if (!do_track_reservation) {
		assert (!change_signal);
		return true;
	}

	if (change_signal) SetSignalState(tile, single_trackdir, SIGNAL_STATE_GREEN);

	/* A path was found, but could not be reserved. */
	if (res_dest.pos.tile != INVALID_TILE && !res_dest.okay) {
		FreeTrainTrackReservation(v);
		return false;
	}

	/* No possible reservation target found, we are probably lost. */
	if (res_dest.pos.tile == INVALID_TILE) {
		/* Try to find any safe destination. */
		if (TryReserveSafeTrack(v, origin, false)) {
			if (best_trackdir != NULL && single_trackdir == INVALID_TRACKDIR) {
				TrackBits res = GetReservedTrackbits(tile);
				*best_trackdir = FindFirstTrackdir(TrackBitsToTrackdirBits(res) & trackdirs);
			}
			TryReserveRailTrack(v->GetPos());
			if (change_signal) MarkTileDirtyByTile(tile);
			return true;
		} else {
			FreeTrainTrackReservation(v);
			return false;
		}
	}

	TryReserveRailTrack(v->GetPos());

	/* Extend reservation until we have found a safe position. */
	bool safe = false;
	for (;;) {
		origin = res_dest.pos;
		if (IsSafeWaitingPosition(v, origin, _settings_game.pf.forbid_90_deg)) {
			safe = true;
			break;
		}

		/* Get next order with destination. */
		if (!orders.SwitchToNextOrder(true)) break;

		DoTrainPathfind(v, origin, true, &res_dest);
		/* Break if no safe position was found. */
		if (res_dest.pos.tile == INVALID_TILE) break;

		if (!res_dest.okay) {
			/* Path found, but could not be reserved. */
			FreeTrainTrackReservation(v);
			return false;
		}
	}

	/* No order or no safe position found, try any position. */
	if (!safe) safe = TryReserveSafeTrack(v, origin, true);

	if (!safe) {
		FreeTrainTrackReservation(v);
	} else if (change_signal) {
		MarkTileDirtyByTile(tile);
	}

	return safe;
}

/**
 * Try to reserve a path to a safe position.
 *
 * @param v The vehicle
 * @param mark_as_stuck Should the train be marked as stuck on a failed reservation?
 * @param first_tile_okay True if no path should be reserved if the current tile is a safe position.
 * @return True if a path could be reserved.
 */
bool TryPathReserve(Train *v, bool mark_as_stuck, bool first_tile_okay)
{
	assert(v->IsFrontEngine());
	assert(v->trackdir != TRACKDIR_DEPOT);

	Vehicle *other_train = NULL;
	PathPos origin;
	FollowTrainReservation(v, &origin, &other_train);
	/* The path we are driving on is already blocked by some other train.
	 * This can only happen in certain situations when mixing path and
	 * block signals or when changing tracks and/or signals.
	 * Exit here as doing any further reservations will probably just
	 * make matters worse. */
	if (other_train != NULL && other_train->index != v->index) {
		if (mark_as_stuck) MarkTrainAsStuck(v);
		return false;
	}

	/* If we have a reserved path and the path ends at a safe tile, we are finished already. */
	if ((v->tile != origin.tile || first_tile_okay) && IsSafeWaitingPosition(v, origin, _settings_game.pf.forbid_90_deg)) {
		/* Can't be stuck then. */
		if (HasBit(v->flags, VRF_TRAIN_STUCK)) SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		ClrBit(v->flags, VRF_TRAIN_STUCK);
		return true;
	}

	DiagDirection exitdir = TrackdirToExitdir(origin.td);
	TileIndex     new_tile = TileAddByDiagDir(origin.tile, exitdir);
	TrackdirBits  reachable = TrackStatusToTrackdirBits(GetTileRailwayStatus(new_tile)) & DiagdirReachesTrackdirs(exitdir);

	if (_settings_game.pf.forbid_90_deg) reachable &= ~TrackdirCrossesTrackdirs(origin.td);

	if (reachable != TRACKDIR_BIT_NONE && !ChooseTrainTrack(v, origin, new_tile, reachable, true)) {
		if (mark_as_stuck) MarkTrainAsStuck(v);
		return false;
	}

	if (HasBit(v->flags, VRF_TRAIN_STUCK)) {
		v->wait_counter = 0;
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}
	ClrBit(v->flags, VRF_TRAIN_STUCK);
	return true;
}

/**
 * Try to reserve a path to a safe position from a depot.
 *
 * @param v The vehicle
 * @param mark_as_stuck Should the train be marked as stuck on a failed reservation?
 * @param first_tile_okay True if no path should be reserved if the current tile is a safe position.
 * @return True if a path could be reserved.
 */
static bool TryPathReserveFromDepot(Train *v)
{
	assert(v->IsFrontEngine());
	assert(v->trackdir == TRACKDIR_DEPOT);

	/* We have to handle depots specially as the track follower won't look
	 * at the depot tile itself but starts from the next tile. If we are still
	 * inside the depot, a depot reservation can never be ours. */
	if (HasDepotReservation(v->tile)) return false;

	/* Depot not reserved, but the next tile might be. */
	DiagDirection exitdir = GetGroundDepotDirection(v->tile);
	TileIndex new_tile = TileAddByDiagDir(v->tile, exitdir);
	if (HasReservedTracks(new_tile, DiagdirReachesTracks(exitdir))) return false;

	TrackdirBits reachable = TrackStatusToTrackdirBits(GetTileRailwayStatus(new_tile)) & DiagdirReachesTrackdirs(exitdir);

	if (reachable != TRACKDIR_BIT_NONE && !ChooseTrainTrack(v, v->GetPos(), new_tile, reachable, true)) {
		return false;
	}

	SetDepotReservation(v->tile, true);
	if (_settings_client.gui.show_track_reservation) MarkTileDirtyByTile(v->tile);

	if (HasBit(v->flags, VRF_TRAIN_STUCK)) {
		v->wait_counter = 0;
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		ClrBit(v->flags, VRF_TRAIN_STUCK);
	}

	return true;
}


static bool CheckReverseTrain(const Train *v)
{
	if (_settings_game.difficulty.line_reverse_mode != 0 ||
			v->trackdir == TRACKDIR_DEPOT || v->trackdir == TRACKDIR_WORMHOLE ||
			!(v->direction & 1)) {
		return false;
	}

	assert(IsValidTrackdir(v->trackdir));

	switch (_settings_game.pf.pathfinder_for_trains) {
		case VPF_NPF: return NPFTrainCheckReverse(v);
		case VPF_YAPF: return YapfTrainCheckReverse(v);

		default: NOT_REACHED();
	}
}

/**
 * Get the location of the next station to visit.
 * @param station Next station to visit.
 * @return Location of the new station.
 */
TileIndex Train::GetOrderStationLocation(StationID station)
{
	if (station == this->last_station_visited) this->last_station_visited = INVALID_STATION;

	const Station *st = Station::Get(station);
	if (!(st->facilities & FACIL_TRAIN)) {
		/* The destination station has no trainstation tiles. */
		this->IncrementRealOrderIndex();
		return 0;
	}

	return st->xy;
}

/** Goods at the consist have changed, update the graphics, cargo, and acceleration. */
void Train::MarkDirty()
{
	Train *v = this;
	do {
		v->colourmap = PAL_NONE;
		v->UpdateViewport(true, false);
	} while ((v = v->Next()) != NULL);

	/* need to update acceleration and cached values since the goods on the train changed. */
	this->CargoChanged();
	this->UpdateAcceleration();
}

/**
 * This function looks at the vehicle and updates its speed (cur_speed
 * and subspeed) variables. Furthermore, it returns the distance that
 * the train can drive this tick. #Vehicle::GetAdvanceDistance() determines
 * the distance to drive before moving a step on the map.
 * @return distance to drive.
 */
int Train::UpdateSpeed()
{
	switch (_settings_game.vehicle.train_acceleration_model) {
		default: NOT_REACHED();
		case AM_ORIGINAL:
			return this->DoUpdateSpeed(this->acceleration * (this->GetAccelerationStatus() == AS_BRAKE ? -4 : 2), 0, this->GetCurrentMaxSpeed());

		case AM_REALISTIC:
			return this->DoUpdateSpeed(this->GetAcceleration(), this->GetAccelerationStatus() == AS_BRAKE ? 0 : 2, this->GetCurrentMaxSpeed());
	}
}

/**
 * Trains enters a station, send out a news item if it is the first train, and start loading.
 * @param v Train that entered the station.
 * @param station Station visited.
 */
static void TrainEnterStation(Train *v, StationID station)
{
	v->last_station_visited = station;

	/* check if a train ever visited this station before */
	Station *st = Station::Get(station);
	if (!(st->had_vehicle_of_type & HVOT_TRAIN)) {
		st->had_vehicle_of_type |= HVOT_TRAIN;
		SetDParam(0, st->index);
		AddVehicleNewsItem(
			STR_NEWS_FIRST_TRAIN_ARRIVAL,
			v->owner == _local_company ? NT_ARRIVAL_COMPANY : NT_ARRIVAL_OTHER,
			v->index,
			st->index
		);
		AI::NewEvent(v->owner, new ScriptEventStationFirstVehicle(st->index, v->index));
		Game::NewEvent(new ScriptEventStationFirstVehicle(st->index, v->index));
	}

	v->force_proceed = TFP_NONE;
	SetWindowDirty(WC_VEHICLE_VIEW, v->index);

	v->BeginLoading();

	TriggerStationRandomisation(st, v->tile, SRT_TRAIN_ARRIVES);
	TriggerStationAnimation(st, v->tile, SAT_TRAIN_ARRIVES);
}

/* Check if the vehicle is compatible with the specified tile */
static inline bool CheckCompatibleRail(const Train *v, TileIndex tile, Track track = INVALID_TRACK)
{
	return IsTileOwner(tile, v->owner) &&
			(!v->IsFrontEngine() || HasBit(v->compatible_railtypes, GetRailType(tile, track)));
}

/** Data structure for storing engine speed changes of an acceleration type. */
struct AccelerationSlowdownParams {
	byte small_turn; ///< Speed change due to a small turn.
	byte large_turn; ///< Speed change due to a large turn.
	byte z_up;       ///< Fraction to remove when moving up.
	byte z_down;     ///< Fraction to add when moving down.
};

/** Speed update fractions for each acceleration type. */
static const AccelerationSlowdownParams _accel_slowdown[] = {
	/* normal accel */
	{256 / 4, 256 / 2, 256 / 4, 2}, ///< normal
	{256 / 4, 256 / 2, 256 / 4, 2}, ///< monorail
	{0,       256 / 2, 256 / 4, 2}, ///< maglev
};

/**
 * Modify the speed of the vehicle due to a change in altitude.
 * @param v %Train to update.
 * @param old_z Previous height.
 */
static inline void AffectSpeedByZChange(Train *v, int old_z)
{
	if (old_z == v->z_pos || _settings_game.vehicle.train_acceleration_model != AM_ORIGINAL) return;

	const AccelerationSlowdownParams *asp = &_accel_slowdown[GetRailTypeInfo(v->railtype)->acceleration_type];

	if (old_z < v->z_pos) {
		v->cur_speed -= (v->cur_speed * asp->z_up >> 8);
	} else {
		uint16 spd = v->cur_speed + asp->z_down;
		if (spd <= v->gcache.cached_max_track_speed) v->cur_speed = spd;
	}
}

/** Tries to reserve track under whole train consist. */
void Train::ReserveTrackUnderConsist() const
{
	for (const Train *u = this; u != NULL; u = u->Next()) {
		switch (u->trackdir) {
			case TRACKDIR_WORMHOLE:
				if (IsRailwayTile(u->tile)) {
					SetBridgeMiddleReservation(u->tile, true);
					SetBridgeMiddleReservation(GetOtherBridgeEnd(u->tile), true);
				} else {
					SetTunnelMiddleReservation(u->tile, true);
					SetTunnelMiddleReservation(GetOtherTunnelEnd(u->tile), true);
				}
				break;
			case TRACKDIR_DEPOT:
				break;
			default:
				TryReserveRailTrack(u->tile, TrackdirToTrack(u->trackdir));
				break;
		}
	}
}

/**
 * The train vehicle crashed!
 * Update its status and other parts around it.
 * @param flooded Crash was caused by flooding.
 * @return Number of people killed.
 */
uint Train::Crash(bool flooded)
{
	uint pass = 0;
	if (this->IsFrontEngine()) {
		pass += 2; // driver

		/* Remove the reserved path in front of the train if it is not stuck.
		 * Also clear all reserved tracks the train is currently on. */
		if (!HasBit(this->flags, VRF_TRAIN_STUCK)) FreeTrainTrackReservation(this);
		for (const Train *v = this; v != NULL; v = v->Next()) {
			ClearPathReservation(v, v->GetPos());
		}

		/* we may need to update crossing we were approaching,
		 * but must be updated after the train has been marked crashed */
		TileIndex crossing = TrainApproachingCrossingTile(this);
		if (crossing != INVALID_TILE) UpdateLevelCrossing(crossing);

		/* Remove the loading indicators (if any) */
		HideFillingPercent(&this->fill_percent_te_id);
	}

	pass += this->GroundVehicleBase::Crash(flooded);

	this->crash_anim_pos = flooded ? 4000 : 1; // max 4440, disappear pretty fast when flooded
	return pass;
}

/**
 * Marks train as crashed and creates an AI event.
 * Doesn't do anything if the train is crashed already.
 * @param v first vehicle of chain
 * @return number of victims (including 2 drivers; zero if train was already crashed)
 */
static uint TrainCrashed(Train *v)
{
	uint num = 0;

	/* do not crash train twice */
	if (!(v->vehstatus & VS_CRASHED)) {
		num = v->Crash();
		AI::NewEvent(v->owner, new ScriptEventVehicleCrashed(v->index, v->tile, ScriptEventVehicleCrashed::CRASH_TRAIN));
		Game::NewEvent(new ScriptEventVehicleCrashed(v->index, v->tile, ScriptEventVehicleCrashed::CRASH_TRAIN));
	}

	/* Try to re-reserve track under already crashed train too.
	 * Crash() clears the reservation! */
	v->ReserveTrackUnderConsist();

	return num;
}

/**
 * Collision test function.
 * @param tcc %Train being examined.
 * @param v %Train vehicle to test collision with.
 * @return Total number of victims if train collided, else 0.
 */
static uint FindTrainCollision(Train *tcc, Vehicle *v)
{
	/* not a train or in depot */
	if (v->type != VEH_TRAIN || Train::From(v)->trackdir == TRACKDIR_DEPOT) return 0;

	/* do not crash into trains of another company. */
	if (v->owner != tcc->owner) return 0;

	/* get first vehicle now to make most usual checks faster */
	Train *coll = Train::From(v)->First();

	/* can't collide with own wagons */
	if (coll == tcc) return 0;

	int x_diff = v->x_pos - tcc->x_pos;
	int y_diff = v->y_pos - tcc->y_pos;

	/* Do fast calculation to check whether trains are not in close vicinity
	 * and quickly reject trains distant enough for any collision.
	 * Differences are shifted by 7, mapping range [-7 .. 8] into [0 .. 15]
	 * Differences are then ORed and then we check for any higher bits */
	uint hash = (y_diff + 7) | (x_diff + 7);
	if (hash & ~15) return 0;

	/* Slower check using multiplication */
	int min_diff = (Train::From(v)->gcache.cached_veh_length + 1) / 2 + (tcc->gcache.cached_veh_length + 1) / 2 - 1;
	if (x_diff * x_diff + y_diff * y_diff > min_diff * min_diff) return 0;

	/* Happens when there is a train under bridge next to bridge head */
	if (abs(v->z_pos - tcc->z_pos) > 5) return 0;

	/* crash both trains */
	return TrainCrashed(tcc) + TrainCrashed(coll);
}

/** Temporary data storage for testing collisions. */
struct TrainCollideChecker {
	Train *v; ///< %Vehicle we are testing for collision.
	uint num; ///< Total number of victims if train collided.
};

/**
 * Collision test function.
 * @param v %Train vehicle to test collision with.
 * @param data %Train being examined.
 * @return \c NULL (always continue search)
 */
static Vehicle *FindTrainCollideEnum(Vehicle *v, void *data)
{
	TrainCollideChecker *tcc = (TrainCollideChecker*)data;

	tcc->num += FindTrainCollision (tcc->v, v);

	return NULL;
}

/**
 * Checks whether the specified train has a collision with another vehicle. If
 * so, destroys this vehicle, and the other vehicle if its subtype has TS_Front.
 * Reports the incident in a flashy news item, modifies station ratings and
 * plays a sound.
 * @param v %Train to test.
 */
static bool CheckTrainCollision(Train *v)
{
	/* can't collide in depot */
	if (v->trackdir == TRACKDIR_DEPOT) return false;

	assert(v->trackdir == TRACKDIR_WORMHOLE || TileVirtXY(v->x_pos, v->y_pos) == v->tile);

	TrainCollideChecker tcc;
	tcc.v = v;
	tcc.num = 0;

	/* find colliding vehicles */
	if (v->trackdir == TRACKDIR_WORMHOLE) {
		VehicleTileIterator iter1 (v->tile);
		while (!iter1.finished()) {
			tcc.num += FindTrainCollision (v, iter1.next());
		}
		VehicleTileIterator iter2 (GetOtherTunnelBridgeEnd(v->tile));
		while (!iter2.finished()) {
			tcc.num += FindTrainCollision (v, iter2.next());
		}
	} else {
		FindVehicleOnPosXY(v->x_pos, v->y_pos, &tcc, FindTrainCollideEnum);
	}

	/* any dead -> no crash */
	if (tcc.num == 0) return false;

	SetDParam(0, tcc.num);
	AddVehicleNewsItem(STR_NEWS_TRAIN_CRASH, NT_ACCIDENT, v->index);

	ModifyStationRatingAround(v->tile, v->owner, -160, 30);
	if (_settings_client.sound.disaster) SndPlayVehicleFx(SND_13_BIG_CRASH, v);
	return true;
}


/**
 * Tile callback routine when vehicle enters a track tile
 * @see vehicle_enter_tile_proc
 */
static void TrainEnter_Track(Train *v, TileIndex tile, int x, int y)
{
	if (IsTileSubtype(tile, TT_TRACK)) return;

	assert(abs((int)(GetSlopePixelZ(x, y) - v->z_pos)) < 3);

	/* modify speed of vehicle */
	uint16 spd = GetBridgeSpec(GetRailBridgeType(tile))->speed;
	Vehicle *first = v->First();
	first->cur_speed = min(first->cur_speed, spd);
}

/**
 * Frame when the 'enter tunnel' sound should be played. This is the second
 * frame on a tile, so the sound is played shortly after entering the tunnel
 * tile, while the vehicle is still visible.
 */
static const byte TUNNEL_SOUND_FRAME = 1;

extern const byte _tunnel_visibility_frame[DIAGDIR_END];

/**
 * Compute number of ticks when next wagon will leave a depot.
 * Negative means next wagon should have left depot n ticks before.
 * @param v vehicle outside (leaving) the depot
 * @return number of ticks when the next wagon will leave
 */
int TicksToLeaveDepot(const Train *v)
{
	DiagDirection dir = GetGroundDepotDirection(v->tile);
	int length = v->CalcNextVehicleOffset();

	switch (dir) {
		case DIAGDIR_NE: return  ((int)(v->x_pos & 0x0F) - (_vehicle_initial_x_fract[dir] - (length + 1)));
		case DIAGDIR_SE: return -((int)(v->y_pos & 0x0F) - (_vehicle_initial_y_fract[dir] + (length + 1)));
		case DIAGDIR_SW: return -((int)(v->x_pos & 0x0F) - (_vehicle_initial_x_fract[dir] + (length + 1)));
		default:
		case DIAGDIR_NW: return  ((int)(v->y_pos & 0x0F) - (_vehicle_initial_y_fract[dir] - (length + 1)));
	}

	return 0; // make compilers happy
}

static void TrainEnter_Misc(Train *u, TileIndex tile, int x, int y)
{
	switch (GetTileSubtype(tile)) {
		default: break;

		case TT_MISC_TUNNEL: {
			assert(abs((int)GetSlopePixelZ(x, y) - u->z_pos) < 3);

			/* Direction into the wormhole */
			const DiagDirection dir = GetTunnelBridgeDirection(tile);

			if (u->direction == DiagDirToDir(dir)) {
				uint frame = DistanceFromTileEdge(ReverseDiagDir(dir), x & 0xF, y & 0xF);
				if (u->IsFrontEngine() && frame == TUNNEL_SOUND_FRAME) {
					if (!PlayVehicleSound(u, VSE_TUNNEL) && RailVehInfo(u->engine_type)->engclass == 0) {
						SndPlayVehicleFx(SND_05_TRAIN_THROUGH_TUNNEL, u);
					}
				}
				if (frame == _tunnel_visibility_frame[dir]) {
					u->vehstatus |= VS_HIDDEN;
				}
			} else if (u->direction == ReverseDir(DiagDirToDir(dir))) {
				uint frame = DistanceFromTileEdge(dir, x & 0xF, y & 0xF);
				if (frame == TILE_SIZE - _tunnel_visibility_frame[dir]) {
					u->vehstatus &= ~VS_HIDDEN;
				}
			}

			break;
		}

		case TT_MISC_DEPOT: {
			if (!IsRailDepot(tile)) break;

			/* depot direction */
			DiagDirection dir = GetGroundDepotDirection(tile);

			/* make sure a train is not entering the tile from behind */
			assert(DistanceFromTileEdge(ReverseDiagDir(dir), x & 0xF, y & 0xF) != 0);

			int fract_x = (int)(x & 0xF) - _vehicle_initial_x_fract[dir];
			int fract_y = (int)(y & 0xF) - _vehicle_initial_y_fract[dir];

			if (u->direction == DiagDirToDir(ReverseDiagDir(dir))) {
				if (fract_x == 0 && fract_y == 0) {
					/* enter the depot */
					u->trackdir = TRACKDIR_DEPOT,
					u->vehstatus |= VS_HIDDEN; // hide it
					u->direction = ReverseDir(u->direction);
					if (u->Next() == NULL) VehicleEnterDepot(u->First());
					u->tile = tile;

					InvalidateWindowData(WC_VEHICLE_DEPOT, u->tile);
				}
			} else if (u->direction == DiagDirToDir(dir)) {
				static const int8 delta_x[4] = { -1,  0,  1,  0 };
				static const int8 delta_y[4] = {  0,  1,  0, -1 };

				/* Calculate the point where the following wagon should be activated. */
				int length = u->CalcNextVehicleOffset() + 1;

				if ((fract_x == length * delta_x[dir]) && (fract_y == length * delta_y[dir])) {
					/* leave the depot? */
					if ((u = u->Next()) != NULL) {
						u->vehstatus &= ~VS_HIDDEN;
						u->trackdir = DiagDirToDiagTrackdir(dir);
					}
				}
			}

			break;
		}
	}
}

static StationID TrainEnter_Station(Train *v, TileIndex tile, int x, int y)
{
	StationID station_id = GetStationIndex(tile);
	if (!v->current_order.ShouldStopAtStation(v, station_id)) return INVALID_STATION;
	if (!IsRailStation(tile) || !v->IsFrontEngine()) return INVALID_STATION;

	int station_ahead;
	int station_length;
	int stop = GetTrainStopLocation(station_id, tile, Train::From(v), &station_ahead, &station_length);

	/* Stop whenever that amount of station ahead + the distance from the
	 * begin of the platform to the stop location is longer than the length
	 * of the platform. Station ahead 'includes' the current tile where the
	 * vehicle is on, so we need to subtract that. */
	if (stop + station_ahead - (int)TILE_SIZE >= station_length) return INVALID_STATION;

	DiagDirection dir = DirToDiagDir(v->direction);

	x &= 0xF;
	y &= 0xF;

	if (DiagDirToAxis(dir) != AXIS_X) Swap(x, y);
	if (y == TILE_SIZE / 2) {
		if (dir != DIAGDIR_SE && dir != DIAGDIR_SW) x = TILE_SIZE - 1 - x;
		stop &= TILE_SIZE - 1;

		if (x == stop) {
			return station_id; // enter station
		} else if (x < stop) {
			v->vehstatus |= VS_TRAIN_SLOWING;
			uint16 spd = max(0, (stop - x) * 20 - 15);
			if (spd < v->cur_speed) v->cur_speed = spd;
		}
	}

	return INVALID_STATION;
}

/**
 * Call the tile callback function for a train entering a tile
 * @param v    Train entering the tile
 * @param tile Tile entered
 * @param x    X position
 * @param y    Y position
 * @return Station ID of an entered station, or INVALID_STATION otherwise
 */
static StationID TrainEnterTile(Train *v, TileIndex tile, int x, int y)
{
	switch (GetTileType(tile)) {
		default: NOT_REACHED();

		case TT_RAILWAY:
			TrainEnter_Track(v, tile, x, y);
			break;

		case TT_MISC:
			TrainEnter_Misc(v, tile, x, y);
			break;

		case TT_STATION:
			return TrainEnter_Station(v, tile, x, y);
	}

	return INVALID_STATION;
}

/**
 * Choose the trackdir to follow when a train enters a new tile.
 * @param v The train that enters the tile
 * @param tile The tile entered
 * @param enterdir The direction the train is moving between tiles
 * @param tsdir The direction to use for GetTileRailwayStatus
 * @param check_90deg Check for 90-degree turns and disallow them
 * @param reverse Set to false to not execute the vehicle reversing. This does not change any other logic.
 * @return The trackdir to use, or INVALID_TRACKDIR if the train cannot go forward
 */
static Trackdir TrainControllerChooseTrackdir(Train *v, TileIndex tile, DiagDirection enterdir, DiagDirection tsdir, bool check_90deg, bool reverse)
{
	/* Get the status of the tracks in the new tile and mask
	 * away the bits that aren't reachable. */
	TrackStatus ts = GetTileRailwayStatus(tile, tsdir);
	TrackdirBits reachable_trackdirs = DiagdirReachesTrackdirs(enterdir);

	TrackdirBits trackdirbits = TrackStatusToTrackdirBits(ts) & reachable_trackdirs;
	if (check_90deg) trackdirbits &= ~TrackdirCrossesTrackdirs(v->trackdir);

	TrackdirBits red_signals = TrackStatusToRedSignals(ts);

	/* Check if the new tile constrains tracks that are compatible
	 * with the current train, if not, bail out. */
	if (trackdirbits == TRACKDIR_BIT_NONE || !CheckCompatibleRail(v, tile, TrackdirToTrack(FindFirstTrackdir(trackdirbits)))) {
		if (reverse) {
			v->wait_counter = 0;
			v->cur_speed = 0;
			v->subspeed = 0;
			ReverseTrainDirection(v);
		}

		return INVALID_TRACKDIR;
	}

	Trackdir chosen_trackdir;

	/* Don't use trackdirbits here as the setting to forbid 90 deg turns might have been switched between reservation and now. */
	TrackdirBits res_trackdirs = TrackBitsToTrackdirBits(GetReservedTrackbits(tile)) & reachable_trackdirs;
	/* Do we have a suitable reserved trackdir? */
	if (res_trackdirs != TRACKDIR_BIT_NONE) {
		chosen_trackdir = FindFirstTrackdir(res_trackdirs);
	} else {
		if (!ChooseTrainTrack(v, v->GetPos(), tile, trackdirbits, false, &chosen_trackdir)) {
			MarkTrainAsStuck(v);
		}
		assert(chosen_trackdir != INVALID_TRACKDIR);
		assert(HasBit(trackdirbits, chosen_trackdir));
	}

	/* Make sure chosen trackdir is a valid trackdir */
	assert(IsValidTrackdir(chosen_trackdir));

	if (v->force_proceed != TFP_NONE) {
		/* For each signal we find decrease the counter by one.
		 * We start at two, so the first signal we pass decreases
		 * this to one, then if we reach the next signal it is
		 * decreased to zero and we won't pass that new signal. */
		bool at_signal;
		if (IsRailwayTile(tile)) {
			Track track = TrackdirToTrack(chosen_trackdir);
			/* However, we do not want to be stopped by PBS
			 * signals entered via the back. */
			at_signal = HasSignalOnTrack(tile, track) &&
				(GetSignalType(tile, track) != SIGTYPE_PBS ||
					HasSignalOnTrackdir(tile, chosen_trackdir));
		} else if (maptile_is_rail_tunnel(tile)) {
			at_signal = maptile_has_tunnel_signals(tile);
		} else {
			at_signal = false;
		}

		if (at_signal) {
			v->force_proceed = (v->force_proceed == TFP_SIGNAL) ? TFP_STUCK : TFP_NONE;
			SetWindowDirty(WC_VEHICLE_VIEW, v->index);
		}
	}

	/* Check if it's a red signal and if force proceed is clicked. */
	if (!HasBit(red_signals, chosen_trackdir) || v->force_proceed != TFP_NONE) {
		/* Proceed */
		TryReserveRailTrack(tile, TrackdirToTrack(chosen_trackdir), false);
		return chosen_trackdir;
	}

	/* In front of a red signal */
	assert(trackdirbits == TrackdirToTrackdirBits(chosen_trackdir));

	/* Don't handle stuck trains here. */
	if (HasBit(v->flags, VRF_TRAIN_STUCK)) return INVALID_TRACKDIR;

	if (!HasSignalOnTrackdir(tile, ReverseTrackdir(chosen_trackdir))) {
		v->cur_speed = 0;
		v->subspeed = 0;
		v->progress = 255 - 100;
		if (!_settings_game.pf.reverse_at_signals || ++v->wait_counter < _settings_game.pf.wait_oneway_signal * 20) return INVALID_TRACKDIR;
	} else if (HasSignalOnTrackdir(tile, chosen_trackdir)) {
		v->cur_speed = 0;
		v->subspeed = 0;
		v->progress = 255 - 10;
		if (!_settings_game.pf.reverse_at_signals || ++v->wait_counter < _settings_game.pf.wait_twoway_signal * 73) {
			DiagDirection exitdir = TrackdirToExitdir(chosen_trackdir);
			TileIndex o_tile = TileAddByDiagDir(tile, exitdir);

			exitdir = ReverseDiagDir(exitdir);

			/* check if a train is waiting on the other side */
			VehicleTileFinder iter (o_tile);
			while (!iter.finished()) {
				Vehicle *v = iter.next();
				if (v->type != VEH_TRAIN || (v->vehstatus & VS_CRASHED)) continue;

				Train *t = Train::From(v);
				if (t->IsFrontEngine() && (t->trackdir < TRACKDIR_END) && (t->cur_speed <= 5) && TrackdirToExitdir(t->trackdir) == exitdir) {
					iter.set_found();
				}
			}
			if (!iter.was_found()) return INVALID_TRACKDIR;
		}
	}

	/* If we would reverse but are currently in a PBS block and
	 * reversing of stuck trains is disabled, don't reverse.
	 * This does not apply if the reason for reversing is a one-way
	 * signal blocking us, because a train would then be stuck forever. */
	if (!_settings_game.pf.reverse_at_signals && !HasOnewaySignalBlockingTrackdir(tile, chosen_trackdir)) {
		assert(IsSignalBufferEmpty());
		AddPosToSignalBuffer(v->GetPos(), v->owner);
		if (UpdateSignalsInBuffer() == SIGSEG_PBS) {
			v->wait_counter = 0;
			return INVALID_TRACKDIR;
		}
	}

	if (reverse) {
		v->wait_counter = 0;
		v->cur_speed = 0;
		v->subspeed = 0;
		ReverseTrainDirection(v);
	}

	return INVALID_TRACKDIR;
}

/**
 * Move a vehicle chain one movement stop forwards.
 * @param v First vehicle to move.
 * @param nomove Stop moving this and all following vehicles.
 * @param reverse Set to false to not execute the vehicle reversing. This does not change any other logic.
 * @return True if the vehicle could be moved forward, false otherwise.
 */
bool TrainController(Train *v, Vehicle *nomove, bool reverse)
{
	Train *first = v->First();
	Train *prev;
	bool direction_changed = false; // has direction of any part changed?

	/* For every vehicle after and including the given vehicle */
	for (prev = v->Previous(); v != nomove; prev = v, v = v->Next()) {
		TileIndex old_tile; // old virtual tile
		bool old_in_wormhole, new_in_wormhole; // old position was in wormhole, new position is in wormhole
		DiagDirection enterdir = INVALID_DIAGDIR; // direction into the new tile, or INVALID_DIAGDIR if we stay on the old tile
		DiagDirection tsdir = INVALID_DIAGDIR; // direction to use for GetTileRailwayStatus, only when moving into a new tile

		VehiclePos gp = GetNewVehiclePos(v);
		if (v->trackdir == TRACKDIR_WORMHOLE) {
			/* In a tunnel or on a bridge (middle part) */
			old_tile = TileVirtXY(v->x_pos, v->y_pos);
			old_in_wormhole = true;

			if (gp.new_tile != v->tile) {
				/* Still in the wormhole */
				new_in_wormhole = true;
				if (v->IsFrontEngine() && (v->vehstatus & VS_HIDDEN) && maptile_has_tunnel_signal(v->tile, false) && (FindTunnelPrevTrain(v) < TILE_SIZE)) {
					/* too close to train ahead, stop */
					return false;
				}
			} else {
				new_in_wormhole = false;
				enterdir = ReverseDiagDir(GetTunnelBridgeDirection(gp.new_tile));
				tsdir = INVALID_DIAGDIR;
			}
		} else if (v->trackdir == TRACKDIR_DEPOT) {
			/* Inside depot */
			assert(gp.new_tile == v->tile);
			continue;
		} else if (gp.new_tile == v->tile) {
			/* Not inside tunnel or depot, staying in the old tile */
			old_tile = v->tile;
			old_in_wormhole = false;
			new_in_wormhole = false;
		} else {
			/* Not inside tunnel or depot, about to enter a new tile */
			old_tile = v->tile;
			old_in_wormhole = false;

			/* Determine what direction we're entering the new tile from */
			enterdir = DiagdirBetweenTiles(v->tile, gp.new_tile);
			assert(IsValidDiagDirection(enterdir));

			if (IsTunnelTile(v->tile) && GetTunnelBridgeDirection(v->tile) == enterdir) {
				TileIndex end_tile = GetOtherTunnelEnd(v->tile);
				if (end_tile != gp.new_tile) {
					/* Entering a tunnel */
					new_in_wormhole = true;
					gp.new_tile = end_tile;
				} else {
					new_in_wormhole = false;
					tsdir = INVALID_DIAGDIR;
				}
			} else if (IsRailBridgeTile(v->tile) && GetTunnelBridgeDirection(v->tile) == enterdir) {
				TileIndex end_tile = GetOtherBridgeEnd(v->tile);
				if (end_tile != gp.new_tile) {
					/* Entering a bridge */
					new_in_wormhole = true;
					gp.new_tile = end_tile;
					ClrBit(v->gv_flags, GVF_GOINGUP_BIT);
					ClrBit(v->gv_flags, GVF_GOINGDOWN_BIT);

					first->cur_speed = min(first->cur_speed, GetBridgeSpec(GetRailBridgeType(v->tile))->speed);
				} else {
					new_in_wormhole = false;
					tsdir = INVALID_DIAGDIR;
				}
			} else {
				new_in_wormhole = false;
				tsdir = ReverseDiagDir(enterdir);
			}
		}

		if (enterdir == INVALID_DIAGDIR) {
			/* Staying on the same tile */

			/* Reverse when we are at the end of the track already, do not move to the new position */
			if (!new_in_wormhole && v->IsFrontEngine() && !TrainCheckIfLineEnds(v, reverse)) return false;
		} else {
			/* Entering a new tile */

			Trackdir chosen_trackdir;
			if (!new_in_wormhole) {
				if (prev == NULL) {
					/* Currently the locomotive is active. Determine which one of the
					 * available tracks to choose */
					chosen_trackdir = TrainControllerChooseTrackdir(v, gp.new_tile, enterdir, tsdir, !old_in_wormhole && _settings_game.pf.forbid_90_deg, reverse);
					if (chosen_trackdir == INVALID_TRACKDIR) return false;

					if (HasPbsSignalOnTrackdir(gp.new_tile, chosen_trackdir)) {
						SetSignalState(gp.new_tile, chosen_trackdir, SIGNAL_STATE_RED);
						MarkTileDirtyByTile(gp.new_tile);
					}
				} else {
					/* The wagon is active, simply follow the prev vehicle. */
					if (prev->tile == gp.new_tile) {
						/* Choose the same track as prev */
						assert(prev->trackdir != TRACKDIR_WORMHOLE);
						chosen_trackdir = prev->trackdir;
					} else {
						/* Choose the track that leads to the tile where prev is.
						 * This case is active if 'prev' is already on the second next tile, when 'v' just enters the next tile.
						 * I.e. when the tile between them has only space for a single vehicle like
						 *  1) horizontal/vertical track tiles and
						 *  2) some orientations of tunnel entries, where the vehicle is already inside the wormhole at 8/16 from the tile edge.
						 *     Is also the train just reversing, the wagon inside the tunnel is 'on' the tile of the opposite tunnel entry.
						 */
						DiagDirection exitdir = DiagdirBetweenTiles(gp.new_tile, prev->tile);
						assert(IsValidDiagDirection(exitdir));
						chosen_trackdir = EnterdirExitdirToTrackdir(enterdir, exitdir);
						assert(!IsReversingRoadTrackdir(chosen_trackdir));
					}

					assert(CheckCompatibleRail(v, gp.new_tile, TrackdirToTrack(chosen_trackdir)));
				}
			} else {
				/* new_in_wormhole */
				assert(!old_in_wormhole);
				if (prev == NULL) {
					if (IsRailwayTile(old_tile)) {
						SetBridgeMiddleReservation(old_tile, true);
						SetBridgeMiddleReservation(gp.new_tile, true);
					} else {
						SetTunnelMiddleReservation(old_tile, true);
						SetTunnelMiddleReservation(gp.new_tile, true);
					}
				}
			}

			if (v->Next() == NULL) {
				/* Clear any track reservation when the last vehicle leaves the tile */
				ClearPathReservation(v, v->GetPos());

				PathPos rev = v->GetReversePos();
				if (HasSignalOnPos(rev)) {
					assert(IsSignalBufferEmpty());
					AddPosToSignalBuffer(rev, v->owner);
					/* Defer actual updating of signals until the train has moved */
				}
			}

			if (new_in_wormhole) {
				/* Just entered the wormhole */
				v->tile = gp.new_tile;
				v->trackdir = TRACKDIR_WORMHOLE;
			} else {
				RailType old_rt = v->GetTrackRailType();

				v->tile = gp.new_tile;
				v->trackdir = chosen_trackdir;

				if (GetRailType(gp.new_tile, TrackdirToTrack(chosen_trackdir)) != old_rt) {
					v->First()->ConsistChanged(true);
				}
			}

			Direction chosen_dir;
			if (new_in_wormhole) {
				chosen_dir = DiagDirToDir(enterdir);
			} else {
				/* Update XY to reflect the entrance to the new tile, and select the direction to use */
				const byte *b = _initial_tile_subcoord[chosen_trackdir];
				gp.x = (gp.x & ~0xF) | b[0];
				gp.y = (gp.y & ~0xF) | b[1];
				chosen_dir = (Direction)b[2];
			}

			if (chosen_dir != v->direction) {
				if (prev == NULL && _settings_game.vehicle.train_acceleration_model == AM_ORIGINAL) {
					const AccelerationSlowdownParams *asp = &_accel_slowdown[GetRailTypeInfo(v->railtype)->acceleration_type];
					DirDiff diff = DirDifference(v->direction, chosen_dir);
					v->cur_speed -= (diff == DIRDIFF_45RIGHT || diff == DIRDIFF_45LEFT ? asp->small_turn : asp->large_turn) * v->cur_speed >> 8;
				}
				direction_changed = true;
				v->direction = chosen_dir;
			}

			/* update image of train, as well as delta XY */
			v->UpdateDeltaXY(v->direction);
		}

		if (!new_in_wormhole) {
			/* Call the landscape function and tell it that the vehicle entered the tile */
			StationID sid = TrainEnterTile(v, gp.new_tile, gp.x, gp.y);
			if (sid != INVALID_STATION) {
				/* The new position is the location where we want to stop */
				TrainEnterStation(v, sid);
			}
		}

		if (v->IsFrontEngine()) {
			v->wait_counter = 0;

			/* Always try to extend the reservation when entering a tile. */
			bool check_next_tile;
			if (!new_in_wormhole) {
				/* If we are approaching a crossing that is reserved, play the sound now. */
				TileIndex crossing = TrainApproachingCrossingTile(v);
				if (crossing != INVALID_TILE && HasCrossingReservation(crossing) && _settings_client.sound.ambient) SndPlayTileFx(SND_0E_LEVEL_CROSSING, crossing);

				check_next_tile = enterdir != INVALID_DIAGDIR;
			} else if (old_in_wormhole) {
				TileIndex last_wormhole_tile = TileAddByDiagDir(v->tile, GetTunnelBridgeDirection(v->tile));
				check_next_tile = (gp.new_tile == last_wormhole_tile) && (gp.new_tile != old_tile);
			} else {
				TileIndexDiff diff = TileOffsByDiagDir(GetTunnelBridgeDirection(v->tile));
				check_next_tile = (old_tile == TILE_ADD(v->tile, 2*diff));
			}

			if (check_next_tile) CheckNextTrainTile(v);
		}

		v->x_pos = gp.x;
		v->y_pos = gp.y;
		VehicleUpdatePosition(v);

		if (new_in_wormhole) {
			if ((v->vehstatus & VS_HIDDEN) == 0) VehicleUpdateViewport(v, true);
		} else {
			/* update the Z position of the vehicle */
			int old_z = v->UpdateInclination(enterdir != INVALID_DIAGDIR, false);

			if (prev == NULL) {
				/* This is the first vehicle in the train */
				AffectSpeedByZChange(v, old_z);
			}
		}

		if (enterdir != INVALID_DIAGDIR) {
			/* Update signals or crossing state if we changed tile */
			/* Signals can only change when the first or the last vehicle moves. */
			if (v->Next() == NULL) {
				/* Update the signal segment added before, if any */
				UpdateSignalsInBuffer();
				if (!old_in_wormhole && IsLevelCrossingTile(old_tile)) UpdateLevelCrossing(old_tile);
			}

			if (v->IsFrontEngine()) {
				PathPos pos = v->GetPos();
				if (HasSignalOnPos(pos)) {
					assert(IsSignalBufferEmpty());
					AddPosToSignalBuffer(pos, v->owner);

					if (UpdateSignalsInBuffer() == SIGSEG_PBS &&
							HasSignalAlongPos(pos) &&
							/* A PBS block with a non-PBS signal facing us? */
							!IsPbsSignal(GetSignalType(pos))) {
						/* We are entering a block with PBS signals right now, but
						 * not through a PBS signal. This means we don't have a
						 * reservation right now. As a conventional signal will only
						 * ever be green if no other train is in the block, getting
						 * a path should always be possible. If the player built
						 * such a strange network that it is not possible, the train
						 * will be marked as stuck and the player has to deal with
						 * the problem. */
						TryReserveRailTrack(pos);
						/* Signals cannot be built on junctions, so
						 * a track on which there is a signal either
						 * is already reserved or can be reserved. */
						assert (HasReservedPos(pos));
						if (!TryPathReserve(v)) {
							MarkTrainAsStuck(v);
						}
					}
				}
			}
		}

		if (old_in_wormhole && old_tile != gp.new_tile && v->Next() == NULL && maptile_is_rail_tunnel(v->tile) && maptile_has_tunnel_signal(v->tile, false)
				&& TileAddByDiagDir(old_tile, GetTunnelBridgeDirection(v->tile)) == GetOtherTunnelEnd(v->tile)) {
			AddTunnelToSignalBuffer(v->tile, v->owner);
			UpdateSignalsInBuffer();
		}

		/* Do not check on every tick to save some computing time. */
		if (v->IsFrontEngine() && v->tick_counter % _settings_game.pf.path_backoff_interval == 0) {
			CheckNextTrainTile(v);
		}
	}

	if (direction_changed) first->tcache.cached_max_curve_speed = first->GetCurveSpeedLimit();

	return true;
}


/**
 * Deletes/Clears the last wagon of a crashed train. It takes the engine of the
 * train, then goes to the last wagon and deletes that. Each call to this function
 * will remove the last wagon of a crashed train. If this wagon was on a crossing,
 * or inside a tunnel/bridge, recalculate the signals as they might need updating
 * @param v the Vehicle of which last wagon is to be removed
 */
static void DeleteLastWagon(Train *v)
{
	Train *first = v->First();

	/* Go to the last wagon and delete the link pointing there
	 * *u is then the one-before-last wagon, and *v the last
	 * one which will physically be removed */
	Train *u = v;
	for (; v->Next() != NULL; v = v->Next()) u = v;
	u->SetNext(NULL);

	if (first != v) {
		/* Recalculate cached train properties */
		first->ConsistChanged(false);
		/* Update the depot window if the first vehicle is in depot -
		 * if v == first, then it is updated in PreDestructor() */
		if (first->trackdir == TRACKDIR_DEPOT) {
			SetWindowDirty(WC_VEHICLE_DEPOT, first->tile);
		}
		v->last_station_visited = first->last_station_visited; // for PreDestructor
	}

	/* 'v' shouldn't be accessed after it has been deleted */
	Trackdir trackdir = v->trackdir;
	TileIndex tile = v->tile;
	Owner owner = v->owner;

	delete v;
	v = NULL; // make sure nobody will try to read 'v' anymore

	if (trackdir == TRACKDIR_DEPOT) return;

	if (trackdir == TRACKDIR_WORMHOLE) {
		TileIndex endtile = GetOtherTunnelBridgeEnd(tile);
		if (EnsureNoTrainOnTunnelBridgeMiddle(tile, endtile).Succeeded()) {
			if (IsRailwayTile(tile)) {
				SetBridgeMiddleReservation(tile, false);
				SetBridgeMiddleReservation(endtile, false);
			} else {
				SetTunnelMiddleReservation(tile, false);
				SetTunnelMiddleReservation(endtile, false);
			}
		}

		assert(IsSignalBufferEmpty());
		if (IsRailwayTile(tile)) {
			AddBridgeToSignalBuffer(tile, owner);
		} else {
			AddTunnelToSignalBuffer(tile, owner);
		}
		UpdateSignalsInBuffer();
		return;
	}

	Track track = TrackdirToTrack(trackdir);
	if (HasReservedTrack(tile, track)) {
		UnreserveRailTrack(tile, track);

		/* If there are still crashed vehicles on the tile, give the track reservation to them */
		TrackBits remaining_trackbits = TRACK_BIT_NONE;
		VehicleTileIterator iter (tile);
		while (!iter.finished()) {
			Vehicle *v = iter.next();
			if (v->type == VEH_TRAIN && (v->vehstatus & VS_CRASHED) != 0) {
				Trackdir trackdir = Train::From(v)->trackdir;
				if (trackdir == TRACKDIR_WORMHOLE) {
					/* Vehicle is inside a wormhole, v->trackdir contains no useful value then. */
					remaining_trackbits |= DiagDirToDiagTrackBits(GetTunnelBridgeDirection(v->tile));
				} else if (trackdir != TRACKDIR_DEPOT) {
					remaining_trackbits |= TrackToTrackBits(TrackdirToTrack(trackdir));
				}
			}
		}

		/* It is important that these two are the first in the loop, as reservation cannot deal with every trackbit combination */
		assert(TRACK_BEGIN == TRACK_X && TRACK_Y == TRACK_BEGIN + 1);
		Track t;
		FOR_EACH_SET_TRACK(t, remaining_trackbits) TryReserveRailTrack(tile, t);
	}

	/* check if the wagon was on a road/rail-crossing */
	if (IsLevelCrossingTile(tile)) UpdateLevelCrossing(tile);

	/* Update signals */
	assert(IsSignalBufferEmpty());
	if (IsRailDepotTile(tile)) {
		AddDepotToSignalBuffer(tile, owner);
	} else if (IsTunnelTile(tile)) {
		AddTunnelToSignalBuffer(tile, owner);
	} else {
		AddTrackToSignalBuffer(tile, track, owner);
	}
	UpdateSignalsInBuffer();
}

/**
 * Rotate all vehicles of a (crashed) train chain randomly to animate the crash.
 * @param v First crashed vehicle.
 */
static void ChangeTrainDirRandomly(Train *v)
{
	static const DirDiff delta[] = {
		DIRDIFF_45LEFT, DIRDIFF_SAME, DIRDIFF_SAME, DIRDIFF_45RIGHT
	};

	do {
		/* We don't need to twist around vehicles if they're not visible */
		if (!(v->vehstatus & VS_HIDDEN)) {
			v->direction = ChangeDir(v->direction, delta[GB(Random(), 0, 2)]);
			v->UpdateDeltaXY(v->direction);
			v->cur_image = v->GetImage(v->direction, EIT_ON_MAP);
			/* Refrain from updating the z position of the vehicle when on
			 * a bridge, because UpdateInclination() will put the vehicle under
			 * the bridge in that case */
			if (v->trackdir != TRACKDIR_WORMHOLE) {
				VehicleUpdatePosition(v);
				v->UpdateInclination(false, false);
			}
		}
	} while ((v = v->Next()) != NULL);
}

/**
 * Handle a crashed train.
 * @param v First train vehicle.
 * @return %Vehicle chain still exists.
 */
static bool HandleCrashedTrain(Train *v)
{
	int state = ++v->crash_anim_pos;

	if (state == 4 && !(v->vehstatus & VS_HIDDEN)) {
		CreateEffectVehicleRel(v, 4, 4, 8, EV_EXPLOSION_LARGE);
	}

	uint32 r;
	if (state <= 200 && Chance16R(1, 7, r)) {
		int index = (r * 10 >> 16);

		Vehicle *u = v;
		do {
			if (--index < 0) {
				r = Random();

				CreateEffectVehicleRel(u,
					GB(r,  8, 3) + 2,
					GB(r, 16, 3) + 2,
					GB(r,  0, 3) + 5,
					EV_EXPLOSION_SMALL);
				break;
			}
		} while ((u = u->Next()) != NULL);
	}

	if (state <= 240 && !(v->tick_counter & 3)) ChangeTrainDirRandomly(v);

	if (state >= 4440 && !(v->tick_counter & 0x1F)) {
		bool ret = v->Next() != NULL;
		DeleteLastWagon(v);
		return ret;
	}

	return true;
}

/** Maximum speeds for train that is broken down or approaching line end */
static const uint16 _breakdown_speeds[16] = {
	225, 210, 195, 180, 165, 150, 135, 120, 105, 90, 75, 60, 45, 30, 15, 15
};


/**
 * Train is approaching line end, slow down and possibly reverse
 *
 * @param v front train engine
 * @param signal not line end, just a red signal
 * @param reverse Set to false to not execute the vehicle reversing. This does not change any other logic.
 * @return true iff we did NOT have to reverse
 */
static bool TrainApproachingLineEnd(Train *v, bool signal, bool reverse)
{
	/* Calc position within the current tile */
	uint x = v->x_pos & 0xF;
	uint y = v->y_pos & 0xF;

	/* for diagonal directions, 'x' will be 0..15 -
	 * for other directions, it will be 1, 3, 5, ..., 15 */
	switch (v->direction) {
		case DIR_N : x = ~x + ~y + 25; break;
		case DIR_NW: x = y;            // FALL THROUGH
		case DIR_NE: x = ~x + 16;      break;
		case DIR_E : x = ~x + y + 9;   break;
		case DIR_SE: x = y;            break;
		case DIR_S : x = x + y - 7;    break;
		case DIR_W : x = ~y + x + 9;   break;
		default: break;
	}

	/* Do not reverse when approaching red signal. Make sure the vehicle's front
	 * does not cross the tile boundary when we do reverse, but as the vehicle's
	 * location is based on their center, use half a vehicle's length as offset.
	 * Multiply the half-length by two for straight directions to compensate that
	 * we only get odd x offsets there. */
	if (!signal && x + (v->gcache.cached_veh_length + 1) / 2 * (IsDiagonalDirection(v->direction) ? 1 : 2) >= TILE_SIZE) {
		/* we are too near the tile end, reverse now */
		v->cur_speed = 0;
		if (reverse) ReverseTrainDirection(v);
		return false;
	}

	/* slow down */
	v->vehstatus |= VS_TRAIN_SLOWING;
	uint16 break_speed = _breakdown_speeds[x & 0xF];
	if (break_speed < v->cur_speed) v->cur_speed = break_speed;

	return true;
}


/**
 * Determines whether a train is on the map and will stay on it after leaving the current tile
 * (as opposed to being in a wormhole or depot)
 * @param v train to test
 * @return true iff vehicle is NOT entering or inside a depot or tunnel/bridge
 */
static bool TrainStayOnMap(const Train *v)
{
	/* Exit if inside a tunnel/bridge or a depot */
	if (v->trackdir == TRACKDIR_WORMHOLE || v->trackdir == TRACKDIR_DEPOT) return false;

	TileIndex tile = v->tile;

	/* entering a tunnel/bridge? */
	if (IsRailBridgeTile(tile)) {
		if (TrackdirToExitdir(v->trackdir) == GetTunnelBridgeDirection(tile)) return false;
	}

	if (IsTunnelTile(tile)) {
		DiagDirection dir = GetTunnelBridgeDirection(tile);
		if (DiagDirToDir(dir) == v->direction) return false;
	}

	/* entering a depot? */
	if (IsRailDepotTile(tile)) {
		DiagDirection dir = ReverseDiagDir(GetGroundDepotDirection(tile));
		if (DiagDirToDir(dir) == v->direction) return false;
	}

	return true;
}


/**
 * Determines whether train is approaching a rail-road crossing
 *   (thus making it barred)
 * @param v front engine of train
 * @return TileIndex of crossing the train is approaching, else INVALID_TILE
 * @pre v in non-crashed front engine
 */
static TileIndex TrainApproachingCrossingTile(const Train *v)
{
	assert(v->IsFrontEngine());
	assert(!(v->vehstatus & VS_CRASHED));

	if (!TrainStayOnMap(v)) return INVALID_TILE;

	DiagDirection dir = TrackdirToExitdir(v->trackdir);
	TileIndex tile = v->tile + TileOffsByDiagDir(dir);

	/* not a crossing || wrong axis || unusable rail (wrong type or owner) */
	if (!IsLevelCrossingTile(tile) || DiagDirToAxis(dir) == GetCrossingRoadAxis(tile) ||
			!CheckCompatibleRail(v, tile)) {
		return INVALID_TILE;
	}

	return tile;
}


/**
 * Checks for line end. Also, bars crossing at next tile if needed
 *
 * @param v vehicle we are checking
 * @param reverse Set to false to not execute the vehicle reversing. This does not change any other logic.
 * @return true iff we did NOT have to reverse
 */
static bool TrainCheckIfLineEnds(Train *v, bool reverse)
{
	/* First, handle broken down train */

	int t = v->breakdown_ctr;
	if (t > 1) {
		v->vehstatus |= VS_TRAIN_SLOWING;

		uint16 break_speed = _breakdown_speeds[GB(~t, 4, 4)];
		if (break_speed < v->cur_speed) v->cur_speed = break_speed;
	} else {
		v->vehstatus &= ~VS_TRAIN_SLOWING;
	}

	if (v->trackdir == TRACKDIR_WORMHOLE) {
		DiagDirection dir = GetTunnelBridgeDirection(v->tile);

		/* Only check when the train is on the last tile segment */
		if (TileVirtXY(v->x_pos, v->y_pos) != v->tile + TileOffsByDiagDir(dir)) return true;

		TrackStatus ts = GetTileRailwayStatus(v->tile, INVALID_DIAGDIR);
		TrackdirBits reachable_trackdirs = DiagdirReachesTrackdirs(ReverseDiagDir(dir));

		TrackdirBits trackdirbits = TrackStatusToTrackdirBits(ts) & reachable_trackdirs;
		TrackdirBits red_signals = TrackStatusToRedSignals(ts) & reachable_trackdirs;

		assert(trackdirbits != TRACKDIR_BIT_NONE);
		assert(CheckCompatibleRail(v, v->tile, TrackdirToTrack(FindFirstTrackdir(trackdirbits))));

		return (trackdirbits & red_signals) == 0 || TrainApproachingLineEnd(v, true, reverse);
	}

	if (!TrainStayOnMap(v)) return true;

	/* Determine the non-diagonal direction in which we will exit this tile */
	DiagDirection dir = TrackdirToExitdir(v->trackdir);
	/* Calculate next tile */
	TileIndex tile = v->tile + TileOffsByDiagDir(dir);

	/* Determine the track status on the next tile */
	TrackStatus ts = GetTileRailwayStatus(tile, ReverseDiagDir(dir));
	TrackdirBits reachable_trackdirs = DiagdirReachesTrackdirs(dir);

	TrackdirBits trackdirbits = TrackStatusToTrackdirBits(ts) & reachable_trackdirs;
	TrackdirBits red_signals = TrackStatusToRedSignals(ts) & reachable_trackdirs;

	/* We are sure the train is not entering a depot, it is detected above */

	/* mask unreachable track bits if we are forbidden to do 90deg turns */
	if (_settings_game.pf.forbid_90_deg) {
		trackdirbits &= ~TrackdirCrossesTrackdirs(v->trackdir);
	}

	/* no suitable trackbits at all || unusable rail (wrong type or owner) */
	if (trackdirbits == TRACKDIR_BIT_NONE || !CheckCompatibleRail(v, tile, TrackdirToTrack(FindFirstTrackdir(trackdirbits)))) {
		return TrainApproachingLineEnd(v, false, reverse);
	}

	/* approaching red signal */
	if ((trackdirbits & red_signals) != 0) return TrainApproachingLineEnd(v, true, reverse);

	/* approaching a rail/road crossing? then make it red */
	if (IsLevelCrossingTile(tile)) MaybeBarCrossingWithSound(tile);

	return true;
}


static bool TrainLocoHandler(Train *v, bool mode)
{
	/* train has crashed? */
	if (v->vehstatus & VS_CRASHED) {
		return mode ? true : HandleCrashedTrain(v); // 'this' can be deleted here
	}

	if (v->force_proceed != TFP_NONE) {
		ClrBit(v->flags, VRF_TRAIN_STUCK);
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
	}

	/* train is broken down? */
	if (v->HandleBreakdown()) return true;

	if (HasBit(v->flags, VRF_REVERSING) && v->cur_speed == 0) {
		ReverseTrainDirection(v);
	}

	/* exit if train is stopped */
	if ((v->vehstatus & VS_STOPPED) && v->cur_speed == 0) return true;

	bool valid_order = !v->current_order.IsType(OT_NOTHING) && v->current_order.GetType() != OT_CONDITIONAL;
	if (ProcessOrders(v) && CheckReverseTrain(v)) {
		v->wait_counter = 0;
		v->cur_speed = 0;
		v->subspeed = 0;
		ClrBit(v->flags, VRF_LEAVING_STATION);
		ReverseTrainDirection(v);
		return true;
	} else if (HasBit(v->flags, VRF_LEAVING_STATION)) {
		/* Try to reserve a path when leaving the station as we
		 * might not be marked as wanting a reservation, e.g.
		 * when an overlength train gets turned around in a station. */
		assert(IsSignalBufferEmpty());
		AddPosToSignalBuffer(v->GetPos(), v->owner);
		if (UpdateSignalsInBuffer() == SIGSEG_PBS || _settings_game.pf.reserve_paths) {
			TryPathReserve(v, true, true);
		}
		ClrBit(v->flags, VRF_LEAVING_STATION);
	}

	v->HandleLoading(mode);

	if (v->current_order.IsType(OT_LOADING)) return true;

	if (CheckTrainStayInDepot(v)) return true;

	if (!mode) v->ShowVisualEffect();

	/* We had no order but have an order now, do look ahead. */
	if (!valid_order && !v->current_order.IsType(OT_NOTHING)) {
		CheckNextTrainTile(v);
	}

	/* Handle stuck trains. */
	if (!mode && HasBit(v->flags, VRF_TRAIN_STUCK)) {
		++v->wait_counter;

		/* Should we try reversing this tick if still stuck? */
		bool turn_around = v->wait_counter % (_settings_game.pf.wait_for_pbs_path * DAY_TICKS) == 0 && _settings_game.pf.reverse_at_signals;

		if (!turn_around && v->wait_counter % _settings_game.pf.path_backoff_interval != 0 && v->force_proceed == TFP_NONE) return true;
		if (!TryPathReserve(v)) {
			/* Still stuck. */
			if (turn_around) ReverseTrainDirection(v);

			if (HasBit(v->flags, VRF_TRAIN_STUCK) && v->wait_counter > 2 * _settings_game.pf.wait_for_pbs_path * DAY_TICKS) {
				/* Show message to player. */
				if (_settings_client.gui.lost_vehicle_warn && v->owner == _local_company) {
					SetDParam(0, v->index);
					AddVehicleAdviceNewsItem(STR_NEWS_TRAIN_IS_STUCK, v->index);
				}
				v->wait_counter = 0;
			}
			/* Exit if force proceed not pressed, else reset stuck flag anyway. */
			if (v->force_proceed == TFP_NONE) return true;
			ClrBit(v->flags, VRF_TRAIN_STUCK);
			v->wait_counter = 0;
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		}
	}

	if (v->current_order.IsType(OT_LEAVESTATION)) {
		v->current_order.Free();
		SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		return true;
	}

	int j = v->UpdateSpeed();

	/* we need to invalidate the widget if we are stopping from 'Stopping 0 km/h' to 'Stopped' */
	if (v->cur_speed == 0 && (v->vehstatus & VS_STOPPED)) {
		/* If we manually stopped, we're not force-proceeding anymore. */
		v->force_proceed = TFP_NONE;
		SetWindowDirty(WC_VEHICLE_VIEW, v->index);
	}

	int adv_spd = v->GetAdvanceDistance();
	if (j < adv_spd) {
		/* if the vehicle has speed 0, update the last_speed field. */
		if (v->cur_speed == 0) v->SetLastSpeed();
	} else {
		TrainCheckIfLineEnds(v);
		/* Loop until the train has finished moving. */
		for (;;) {
			j -= adv_spd;
			TrainController(v, NULL);
			/* Don't continue to move if the train crashed. */
			if (CheckTrainCollision(v)) break;
			/* Determine distance to next map position */
			adv_spd = v->GetAdvanceDistance();

			/* No more moving this tick */
			if (j < adv_spd || v->cur_speed == 0) break;

			OrderType order_type = v->current_order.GetType();
			/* Do not skip waypoints (incl. 'via' stations) when passing through at full speed. */
			if ((order_type == OT_GOTO_WAYPOINT || order_type == OT_GOTO_STATION) &&
						(v->current_order.GetNonStopType() & ONSF_NO_STOP_AT_DESTINATION_STATION) &&
						IsStationTile(v->tile) &&
						v->current_order.GetDestination() == GetStationIndex(v->tile)) {
				ProcessOrders(v);
			}
		}
		v->SetLastSpeed();
	}

	for (Train *u = v; u != NULL; u = u->Next()) {
		if ((u->vehstatus & VS_HIDDEN) != 0) continue;

		u->UpdateViewport(false, false);
	}

	if (v->progress == 0) v->progress = j; // Save unused spd for next time, if TrainController didn't set progress

	return true;
}

/**
 * Get running cost for the train consist.
 * @return Yearly running costs.
 */
Money Train::GetRunningCost() const
{
	Money cost = 0;
	const Train *v = this;

	do {
		const Engine *e = v->GetEngine();
		if (e->u.rail.running_cost_class == INVALID_PRICE) continue;

		uint cost_factor = GetVehicleProperty(v, PROP_TRAIN_RUNNING_COST_FACTOR, e->u.rail.running_cost);
		if (cost_factor == 0) continue;

		/* Halve running cost for multiheaded parts */
		if (v->IsMultiheaded()) cost_factor /= 2;

		cost += GetPrice(e->u.rail.running_cost_class, cost_factor, e->GetGRF());
	} while ((v = v->GetNextVehicle()) != NULL);

	return cost;
}

/**
 * Update train vehicle data for a tick.
 * @return True if the vehicle still exists, false if it has ceased to exist (front of consists only).
 */
bool Train::Tick()
{
	this->tick_counter++;

	if (this->IsFrontEngine()) {
		if (!(this->vehstatus & VS_STOPPED) || this->cur_speed > 0) this->running_ticks++;

		this->current_order_time++;

		if (!TrainLocoHandler(this, false)) return false;

		return TrainLocoHandler(this, true);
	} else if (this->IsFreeWagon() && (this->vehstatus & VS_CRASHED)) {
		/* Delete flooded standalone wagon chain */
		if (++this->crash_anim_pos >= 4400) {
			delete this;
			return false;
		}
	}

	return true;
}

/**
 * Check whether a train needs service, and if so, find a depot or service it.
 * @return v %Train to check.
 */
static void CheckIfTrainNeedsService(Train *v)
{
	if (Company::Get(v->owner)->settings.vehicle.servint_trains == 0 || !v->NeedsAutomaticServicing()) return;
	if (v->IsChainInDepot()) {
		VehicleServiceInDepot(v);
		return;
	}

	FindDepotData tfdd;
	/* Only go to the depot if it is not too far out of our way. */
	if (!FindClosestTrainDepot(v, true, &tfdd)) {
		if (v->current_order.IsType(OT_GOTO_DEPOT)) {
			/* If we were already heading for a depot but it has
			 * suddenly moved farther away, we continue our normal
			 * schedule? */
			v->current_order.MakeDummy();
			SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
		}
		return;
	}

	DepotID depot = GetDepotIndex(tfdd.tile);

	if (v->current_order.IsType(OT_GOTO_DEPOT) &&
			v->current_order.GetDestination() != depot &&
			!Chance16(3, 16)) {
		return;
	}

	SetBit(v->gv_flags, GVF_SUPPRESS_IMPLICIT_ORDERS);
	v->current_order.MakeGoToDepot(depot, ODTFB_SERVICE);
	v->dest_tile = tfdd.tile;
	SetWindowWidgetDirty(WC_VEHICLE_VIEW, v->index, WID_VV_START_STOP);
}

/** Update day counters of the train vehicle. */
void Train::OnNewDay()
{
	AgeVehicle(this);

	if ((++this->day_counter & 7) == 0) DecreaseVehicleValue(this);

	if (this->IsFrontEngine()) {
		CheckVehicleBreakdown(this);

		CheckIfTrainNeedsService(this);

		CheckOrders(this);

		/* update destination */
		if (this->current_order.IsType(OT_GOTO_STATION)) {
			TileIndex tile = Station::Get(this->current_order.GetDestination())->train_station.tile;
			if (tile != INVALID_TILE) this->dest_tile = tile;
		}

		if (this->running_ticks != 0) {
			/* running costs */
			CommandCost cost(EXPENSES_TRAIN_RUN, this->GetRunningCost() * this->running_ticks / (DAYS_IN_YEAR  * DAY_TICKS));

			this->profit_this_year -= cost.GetCost();
			this->running_ticks = 0;

			SubtractMoneyFromCompanyFract(this->owner, cost);

			SetWindowDirty(WC_VEHICLE_DETAILS, this->index);
			SetWindowClassesDirty(WC_TRAINS_LIST);
		}
	}
}

/**
 * Get the trackdir of the train vehicle.
 * @return Current trackdir of the vehicle.
 */
Trackdir Train::GetTrackdir() const
{
	assert(!(this->vehstatus & VS_CRASHED));
	assert(this->trackdir != TRACKDIR_WORMHOLE);

	if (this->trackdir == TRACKDIR_DEPOT) {
		/* We'll assume the train is facing outwards */
		return DiagDirToDiagTrackdir(GetGroundDepotDirection(this->tile)); // Train in depot
	}

	return this->trackdir;
}

PathPos Train::GetPos() const
{
	if (this->vehstatus & VS_CRASHED) return PathPos();

	if (this->trackdir == TRACKDIR_WORMHOLE) {
		DiagDirection rev = GetTunnelBridgeDirection(this->tile);
		assert(ReverseDiagDir(rev) == DirToDiagDir(this->direction));
		return PathPos(TILE_ADD(this->tile, TileOffsByDiagDir(rev)), DiagDirToDiagTrackdir(ReverseDiagDir(rev)), this->tile);
	}

	return PathPos(this->tile, this->GetTrackdir());
}

PathPos Train::GetReversePos() const
{
	if (this->vehstatus & VS_CRASHED) return PathPos();

	if (this->trackdir == TRACKDIR_WORMHOLE) {
		TileIndex other_end = GetOtherTunnelBridgeEnd(this->tile);
		DiagDirection dir = GetTunnelBridgeDirection(other_end);
		assert(dir == DirToDiagDir(this->direction));
		return PathPos(TILE_ADD(other_end, TileOffsByDiagDir(dir)), DiagDirToDiagTrackdir(ReverseDiagDir(dir)), other_end);
	}

	return PathPos(this->tile, ReverseTrackdir(this->GetTrackdir()));
}
