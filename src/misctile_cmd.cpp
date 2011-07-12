/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file misctile_cmd.cpp Handling of misc tiles. */

#include "stdafx.h"
#include "tile_cmd.h"
#include "rail_map.h"
#include "command_func.h"
#include "company_func.h"
#include "vehicle_func.h"
#include "pbs.h"
#include "train.h"
#include "roadveh.h"
#include "depot_base.h"
#include "viewport_func.h"
#include "newgrf_railtype.h"
#include "elrail_func.h"
#include "depot_func.h"
#include "autoslope.h"
#include "road_cmd.h"
#include "town.h"
#include "company_base.h"
#include "strings_func.h"
#include "company_gui.h"

#include "pathfinder/yapf/yapf_cache.h"

#include "table/strings.h"
#include "table/track_land.h"
#include "table/road_land.h"


static void DrawTrainDepot(TileInfo *ti)
{
	assert(IsRailDepotTile(ti->tile));

	const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(ti->tile));

	uint32 palette = COMPANY_SPRITE_COLOUR(GetTileOwner(ti->tile));

	/* draw depot */
	const DrawTileSprites *dts;
	PaletteID pal = PAL_NONE;
	SpriteID relocation;

	if (ti->tileh != SLOPE_FLAT) DrawFoundation(ti, FOUNDATION_LEVELED);

	if (IsInvisibilitySet(TO_BUILDINGS)) {
		/* Draw rail instead of depot */
		dts = &_depot_invisible_gfx_table[GetRailDepotDirection(ti->tile)];
	} else {
		dts = &_depot_gfx_table[GetRailDepotDirection(ti->tile)];
	}

	SpriteID image;
	if (rti->UsesOverlay()) {
		image = SPR_FLAT_GRASS_TILE;
	} else {
		image = dts->ground.sprite;
		if (image != SPR_FLAT_GRASS_TILE) image += rti->GetRailtypeSpriteOffset();
	}

	/* adjust ground tile for desert
	 * don't adjust for snow, because snow in depots looks weird */
	if (IsSnowRailGround(ti->tile) && _settings_game.game_creation.landscape == LT_TROPIC) {
		if (image != SPR_FLAT_GRASS_TILE) {
			image += rti->snow_offset; // tile with tracks
		} else {
			image = SPR_FLAT_SNOW_DESERT_TILE; // flat ground
		}
	}

	DrawGroundSprite(image, GroundSpritePaletteTransform(image, pal, palette));

	if (rti->UsesOverlay()) {
		SpriteID ground = GetCustomRailSprite(rti, ti->tile, RTSG_GROUND);

		switch (GetRailDepotDirection(ti->tile)) {
			case DIAGDIR_NE: if (!IsInvisibilitySet(TO_BUILDINGS)) break; // else FALL THROUGH
			case DIAGDIR_SW: DrawGroundSprite(ground + RTO_X, PAL_NONE); break;
			case DIAGDIR_NW: if (!IsInvisibilitySet(TO_BUILDINGS)) break; // else FALL THROUGH
			case DIAGDIR_SE: DrawGroundSprite(ground + RTO_Y, PAL_NONE); break;
			default: break;
		}

		if (_settings_client.gui.show_track_reservation && HasDepotReservation(ti->tile)) {
			SpriteID overlay = GetCustomRailSprite(rti, ti->tile, RTSG_OVERLAY);

			switch (GetRailDepotDirection(ti->tile)) {
				case DIAGDIR_NE: if (!IsInvisibilitySet(TO_BUILDINGS)) break; // else FALL THROUGH
				case DIAGDIR_SW: DrawGroundSprite(overlay + RTO_X, PALETTE_CRASH); break;
				case DIAGDIR_NW: if (!IsInvisibilitySet(TO_BUILDINGS)) break; // else FALL THROUGH
				case DIAGDIR_SE: DrawGroundSprite(overlay + RTO_Y, PALETTE_CRASH); break;
				default: break;
			}
		}
	} else {
		/* PBS debugging, draw reserved tracks darker */
		if (_game_mode != GM_MENU && _settings_client.gui.show_track_reservation && HasDepotReservation(ti->tile)) {
			switch (GetRailDepotDirection(ti->tile)) {
				case DIAGDIR_NE: if (!IsInvisibilitySet(TO_BUILDINGS)) break; // else FALL THROUGH
				case DIAGDIR_SW: DrawGroundSprite(rti->base_sprites.single_x, PALETTE_CRASH); break;
				case DIAGDIR_NW: if (!IsInvisibilitySet(TO_BUILDINGS)) break; // else FALL THROUGH
				case DIAGDIR_SE: DrawGroundSprite(rti->base_sprites.single_y, PALETTE_CRASH); break;
				default: break;
			}
		}
	}

	int depot_sprite = GetCustomRailSprite(rti, ti->tile, RTSG_DEPOT);
	relocation = depot_sprite != 0 ? depot_sprite - SPR_RAIL_DEPOT_SE_1 : rti->GetRailtypeSpriteOffset();

	if (HasCatenaryDrawn(GetRailType(ti->tile))) DrawCatenary(ti);

	DrawRailTileSeq(ti, dts, TO_BUILDINGS, relocation, 0, palette);
}

void DrawTrainDepotSprite(int x, int y, int dir, RailType railtype)
{
	const DrawTileSprites *dts = &_depot_gfx_table[dir];
	const RailtypeInfo *rti = GetRailTypeInfo(railtype);
	SpriteID image = rti->UsesOverlay() ? SPR_FLAT_GRASS_TILE : dts->ground.sprite;
	uint32 offset = rti->GetRailtypeSpriteOffset();

	x += 33;
	y += 17;

	if (image != SPR_FLAT_GRASS_TILE) image += offset;
	PaletteID palette = COMPANY_SPRITE_COLOUR(_local_company);

	DrawSprite(image, PAL_NONE, x, y);

	if (rti->UsesOverlay()) {
		SpriteID ground = GetCustomRailSprite(rti, INVALID_TILE, RTSG_GROUND);

		switch (dir) {
			case DIAGDIR_SW: DrawSprite(ground + RTO_X, PAL_NONE, x, y); break;
			case DIAGDIR_SE: DrawSprite(ground + RTO_Y, PAL_NONE, x, y); break;
			default: break;
		}
	}

	int depot_sprite = GetCustomRailSprite(rti, INVALID_TILE, RTSG_DEPOT);
	if (depot_sprite != 0) offset = depot_sprite - SPR_RAIL_DEPOT_SE_1;

	DrawRailTileSeqInGUI(x, y, dts, offset, 0, palette);
}

static void DrawRoadDepot(TileInfo *ti)
{
	assert(IsRoadDepotTile(ti->tile));

	if (ti->tileh != SLOPE_FLAT) DrawFoundation(ti, FOUNDATION_LEVELED);

	PaletteID palette = COMPANY_SPRITE_COLOUR(GetTileOwner(ti->tile));

	const DrawTileSprites *dts;
	if (HasTileRoadType(ti->tile, ROADTYPE_TRAM)) {
		dts =  &_tram_depot[GetRoadDepotDirection(ti->tile)];
	} else {
		dts =  &_road_depot[GetRoadDepotDirection(ti->tile)];
	}

	DrawGroundSprite(dts->ground.sprite, PAL_NONE);
	DrawOrigTileSeq(ti, dts, TO_BUILDINGS, palette);
}

/**
 * Draw the road depot sprite.
 * @param x   The x offset to draw at.
 * @param y   The y offset to draw at.
 * @param dir The direction the depot must be facing.
 * @param rt  The road type of the depot to draw.
 */
void DrawRoadDepotSprite(int x, int y, DiagDirection dir, RoadType rt)
{
	PaletteID palette = COMPANY_SPRITE_COLOUR(_local_company);
	const DrawTileSprites *dts = (rt == ROADTYPE_TRAM) ? &_tram_depot[dir] : &_road_depot[dir];

	x += 33;
	y += 17;

	DrawSprite(dts->ground.sprite, PAL_NONE, x, y);
	DrawOrigTileSeqInGUI(x, y, dts, palette);
}

static void DrawTile_Misc(TileInfo *ti)
{
	switch (GetTileSubtype(ti->tile)) {
		default: NOT_REACHED();

		case TT_MISC_CROSSING:
			DrawLevelCrossing(ti);
			break;

		case TT_MISC_DEPOT:
			if (IsRailDepot(ti->tile)) {
				DrawTrainDepot(ti);
			} else {
				DrawRoadDepot(ti);
			}
			break;
	}
}

static int GetSlopePixelZ_Misc(TileIndex tile, uint x, uint y)
{
	return GetTileMaxPixelZ(tile);
}


static CommandCost RemoveTrainDepot(TileIndex tile, DoCommandFlag flags)
{
	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckTileOwnership(tile);
		if (ret.Failed()) return ret;
	}

	CommandCost ret = EnsureNoVehicleOnGround(tile);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		/* read variables before the depot is removed */
		DiagDirection dir = GetRailDepotDirection(tile);
		Owner owner = GetTileOwner(tile);
		Train *v = NULL;

		if (HasDepotReservation(tile)) {
			v = GetTrainForReservation(tile, DiagDirToDiagTrack(dir));
			if (v != NULL) FreeTrainTrackReservation(v);
		}

		Company::Get(owner)->infrastructure.rail[GetRailType(tile)]--;
		DirtyCompanyInfrastructureWindows(owner);

		delete Depot::GetByTile(tile);
		DoClearSquare(tile);
		AddSideToSignalBuffer(tile, dir, owner);
		YapfNotifyTrackLayoutChange(tile, DiagDirToDiagTrack(dir));
		if (v != NULL) TryPathReserve(v, true);
	}

	return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_CLEAR_DEPOT_TRAIN]);
}

static CommandCost RemoveRoadDepot(TileIndex tile, DoCommandFlag flags)
{
	if (_current_company != OWNER_WATER) {
		CommandCost ret = CheckTileOwnership(tile);
		if (ret.Failed()) return ret;
	}

	CommandCost ret = EnsureNoVehicleOnGround(tile);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		Company *c = Company::GetIfValid(GetTileOwner(tile));
		if (c != NULL) {
			/* A road depot has two road bits. */
			c->infrastructure.road[FIND_FIRST_BIT(GetRoadTypes(tile))] -= 2;
			DirtyCompanyInfrastructureWindows(c->index);
		}

		delete Depot::GetByTile(tile);
		DoClearSquare(tile);
	}

	return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_CLEAR_DEPOT_ROAD]);
}

extern CommandCost RemoveRoad(TileIndex tile, DoCommandFlag flags, RoadBits pieces, RoadType rt, bool crossing_check, bool town_check = true);

static CommandCost ClearTile_Misc(TileIndex tile, DoCommandFlag flags)
{
	switch (GetTileSubtype(tile)) {
		default: NOT_REACHED();

		case TT_MISC_CROSSING: {
			RoadTypes rts = GetRoadTypes(tile);
			CommandCost ret(EXPENSES_CONSTRUCTION);

			if (flags & DC_AUTO) return_cmd_error(STR_ERROR_MUST_REMOVE_ROAD_FIRST);

			/* Must iterate over the roadtypes in a reverse manner because
			 * tram tracks must be removed before the road bits. */
			RoadType rt = ROADTYPE_TRAM;
			do {
				if (HasBit(rts, rt)) {
					CommandCost tmp_ret = RemoveRoad(tile, flags, GetCrossingRoadBits(tile), rt, false);
					if (tmp_ret.Failed()) return tmp_ret;
					ret.AddCost(tmp_ret);
				}
			} while (rt-- != ROADTYPE_ROAD);

			if (flags & DC_EXEC) {
				DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
			}
			return ret;
		}

		case TT_MISC_DEPOT:
			if (flags & DC_AUTO) {
				if (!IsTileOwner(tile, _current_company)) {
					return_cmd_error(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
				}
				return_cmd_error(STR_ERROR_BUILDING_MUST_BE_DEMOLISHED);
			}
			return IsRailDepot(tile) ? RemoveTrainDepot(tile, flags) : RemoveRoadDepot(tile, flags);
	}
}


static void GetTileDesc_Misc(TileIndex tile, TileDesc *td)
{
	switch (GetTileSubtype(tile)) {
		default: NOT_REACHED();

		case TT_MISC_CROSSING: {
			td->str = STR_LAI_ROAD_DESCRIPTION_ROAD_RAIL_LEVEL_CROSSING;

			RoadTypes rts = GetRoadTypes(tile);
			Owner road_owner = HasBit(rts, ROADTYPE_ROAD) ? GetRoadOwner(tile, ROADTYPE_ROAD) : INVALID_OWNER;
			Owner tram_owner = HasBit(rts, ROADTYPE_TRAM) ? GetRoadOwner(tile, ROADTYPE_TRAM) : INVALID_OWNER;
			Owner rail_owner = GetTileOwner(tile);

			td->rail_speed = GetRailTypeInfo(GetRailType(tile))->max_speed;

			Owner first_owner = (road_owner == INVALID_OWNER ? tram_owner : road_owner);
			bool mixed_owners = (tram_owner != INVALID_OWNER && tram_owner != first_owner) || (rail_owner != INVALID_OWNER && rail_owner != first_owner);

			if (mixed_owners) {
				/* Multiple owners */
				td->owner_type[0] = (rail_owner == INVALID_OWNER ? STR_NULL : STR_LAND_AREA_INFORMATION_RAIL_OWNER);
				td->owner[0] = rail_owner;
				td->owner_type[1] = (road_owner == INVALID_OWNER ? STR_NULL : STR_LAND_AREA_INFORMATION_ROAD_OWNER);
				td->owner[1] = road_owner;
				td->owner_type[2] = (tram_owner == INVALID_OWNER ? STR_NULL : STR_LAND_AREA_INFORMATION_TRAM_OWNER);
				td->owner[2] = tram_owner;
			} else {
				/* One to rule them all */
				td->owner[0] = first_owner;
			}

			break;
		}

		case TT_MISC_DEPOT:
			td->owner[0] = GetTileOwner(tile);
			td->build_date = Depot::GetByTile(tile)->build_date;

			if (IsRailDepot(tile)) {
				td->str = STR_LAI_RAIL_DESCRIPTION_TRAIN_DEPOT;

				const RailtypeInfo *rti = GetRailTypeInfo(GetRailType(tile));
				SetDParamX(td->dparam, 0, rti->strings.name);
				td->rail_speed = rti->max_speed;

				if (_settings_game.vehicle.train_acceleration_model != AM_ORIGINAL) {
					if (td->rail_speed > 0) {
						td->rail_speed = min(td->rail_speed, 61);
					} else {
						td->rail_speed = 61;
					}
				}
			} else {
				td->str = STR_LAI_ROAD_DESCRIPTION_ROAD_VEHICLE_DEPOT;
			}

			break;
	}
}


static TrackStatus GetTileTrackStatus_Misc(TileIndex tile, TransportType mode, uint sub_mode, DiagDirection side)
{
	switch (GetTileSubtype(tile)) {
		default: NOT_REACHED();

		case TT_MISC_CROSSING: {
			TrackdirBits trackdirbits = TRACKDIR_BIT_NONE;
			TrackdirBits red_signals = TRACKDIR_BIT_NONE; // crossing barred

			switch (mode) {
				case TRANSPORT_RAIL:
					trackdirbits = TrackBitsToTrackdirBits(GetCrossingRailBits(tile));
					break;

				case TRANSPORT_ROAD: {
					if ((GetRoadTypes(tile) & sub_mode) == 0) break;
					Axis axis = GetCrossingRoadAxis(tile);

					if (side != INVALID_DIAGDIR && axis != DiagDirToAxis(side)) break;

					trackdirbits = TrackBitsToTrackdirBits(AxisToTrackBits(axis));
					if (IsCrossingBarred(tile)) red_signals = trackdirbits;
					break;
				}

				default: break;
			}
			return CombineTrackStatus(trackdirbits, red_signals);
		}

		case TT_MISC_DEPOT: {
			DiagDirection dir;

			if (IsRailDepot(tile)) {
				if (mode != TRANSPORT_RAIL) return 0;

				dir = GetRailDepotDirection(tile);
			} else {
				if ((mode != TRANSPORT_ROAD) || ((GetRoadTypes(tile) & sub_mode) == 0)) return 0;

				dir = GetRoadDepotDirection(tile);
			}

			if (side != INVALID_DIAGDIR && side != dir) return 0;

			TrackdirBits trackdirbits = TrackBitsToTrackdirBits(DiagDirToDiagTrackBits(dir));
			return CombineTrackStatus(trackdirbits, TRACKDIR_BIT_NONE);
		}
	}
}


static bool ClickTile_Misc(TileIndex tile)
{
	if (!IsGroundDepotTile(tile)) return false;

	ShowDepotWindow(tile, IsRailDepot(tile) ? VEH_TRAIN : VEH_ROAD);
	return true;
}


static void TileLoop_Misc(TileIndex tile)
{
	if (IsRailDepotTile(tile)) {
		RailGroundType ground;

		switch (_settings_game.game_creation.landscape) {
			case LT_ARCTIC: {
				int z;
				Slope slope = GetTileSlope(tile, &z);

				/* is the depot on a non-flat tile? */
				if (slope != SLOPE_FLAT) z++;

				ground = (z > GetSnowLine()) ? RAIL_GROUND_ICE_DESERT : RAIL_GROUND_GRASS;
				break;
			}

			case LT_TROPIC:
				ground = (GetTropicZone(tile) == TROPICZONE_DESERT) ? RAIL_GROUND_ICE_DESERT : RAIL_GROUND_GRASS;
				break;

			default:
				ground = RAIL_GROUND_GRASS;
				break;
		}

		if (ground != GetRailGroundType(tile)) {
			SetRailGroundType(tile, ground);
			MarkTileDirtyByTile(tile);
		}
	} else {
		assert(IsLevelCrossingTile(tile) || IsRoadDepotTile(tile));

		switch (_settings_game.game_creation.landscape) {
			case LT_ARCTIC:
				if (IsOnSnow(tile) != (GetTileZ(tile) > GetSnowLine())) {
					ToggleSnow(tile);
					MarkTileDirtyByTile(tile);
				}
				break;

			case LT_TROPIC:
				if (GetTropicZone(tile) == TROPICZONE_DESERT && !IsOnDesert(tile)) {
					ToggleDesert(tile);
					MarkTileDirtyByTile(tile);
				}
				break;
		}

		if (IsLevelCrossingTile(tile)) {
			const Town *t = ClosestTownFromTile(tile, UINT_MAX);
			UpdateRoadSide(tile, t != NULL ? GetTownRadiusGroup(t, tile) : HZB_TOWN_EDGE);
		}
	}
}


static void ChangeTileOwner_Misc(TileIndex tile, Owner old_owner, Owner new_owner)
{
	switch (GetTileSubtype(tile)) {
		default: NOT_REACHED();

		case TT_MISC_CROSSING:
			for (RoadType rt = ROADTYPE_ROAD; rt < ROADTYPE_END; rt++) {
				/* Update all roadtypes, no matter if they are present */
				if (GetRoadOwner(tile, rt) == old_owner) {
					if (HasTileRoadType(tile, rt)) {
						/* A level crossing has two road bits. No need to dirty windows here, we'll redraw the whole screen anyway. */
						Company::Get(old_owner)->infrastructure.road[rt] -= 2;
						if (new_owner != INVALID_OWNER) Company::Get(new_owner)->infrastructure.road[rt] += 2;
					}

					SetRoadOwner(tile, rt, new_owner == INVALID_OWNER ? OWNER_NONE : new_owner);
				}
			}

			if (GetTileOwner(tile) == old_owner) {
				if (new_owner == INVALID_OWNER) {
					DoCommand(tile, 0, GetCrossingRailTrack(tile), DC_EXEC | DC_BANKRUPT, CMD_REMOVE_SINGLE_RAIL);
				} else {
					/* Update infrastructure counts. No need to dirty windows here, we'll redraw the whole screen anyway. */
					RailType rt = GetRailType(tile);
					Company::Get(old_owner)->infrastructure.rail[rt] -= LEVELCROSSING_TRACKBIT_FACTOR;
					Company::Get(new_owner)->infrastructure.rail[rt] += LEVELCROSSING_TRACKBIT_FACTOR;

					SetTileOwner(tile, new_owner);
				}
			}

			break;

		case TT_MISC_DEPOT:
			if (!IsTileOwner(tile, old_owner)) return;

			if (new_owner != INVALID_OWNER) {
				/* Update company infrastructure counts. No need to dirty windows here, we'll redraw the whole screen anyway. */
				if (IsRailDepot(tile)) {
					RailType rt = GetRailType(tile);
					Company::Get(old_owner)->infrastructure.rail[rt]--;
					Company::Get(new_owner)->infrastructure.rail[rt]++;
				} else {
					/* A road depot has two road bits. */
					RoadType rt = (RoadType)FIND_FIRST_BIT(GetRoadTypes(tile));
					Company::Get(old_owner)->infrastructure.road[rt] -= 2;
					Company::Get(new_owner)->infrastructure.road[rt] += 2;
				}

				SetTileOwner(tile, new_owner);
			} else {
				DoCommand(tile, 0, 0, DC_EXEC | DC_BANKRUPT, CMD_LANDSCAPE_CLEAR);
			}
			break;
	}
}


static const byte _fractcoords_enter_x[4] = { 0xA, 0x8, 0x4, 0x8 };
static const byte _fractcoords_enter_y[4] = { 0x8, 0x4, 0x8, 0xA };
static const int8 _deltacoord_leaveoffset_x[4] = { -1,  0,  1,  0 };
static const int8 _deltacoord_leaveoffset_y[4] = {  0,  1,  0, -1 };

/**
 * Compute number of ticks when next wagon will leave a depot.
 * Negative means next wagon should have left depot n ticks before.
 * @param v vehicle outside (leaving) the depot
 * @return number of ticks when the next wagon will leave
 */
int TicksToLeaveDepot(const Train *v)
{
	DiagDirection dir = GetRailDepotDirection(v->tile);
	int length = v->CalcNextVehicleOffset();

	switch (dir) {
		case DIAGDIR_NE: return  ((int)(v->x_pos & 0x0F) - (_fractcoords_enter_x[dir] - (length + 1)));
		case DIAGDIR_SE: return -((int)(v->y_pos & 0x0F) - (_fractcoords_enter_y[dir] + (length + 1)));
		case DIAGDIR_SW: return -((int)(v->x_pos & 0x0F) - (_fractcoords_enter_x[dir] + (length + 1)));
		default:
		case DIAGDIR_NW: return  ((int)(v->y_pos & 0x0F) - (_fractcoords_enter_y[dir] - (length + 1)));
	}

	return 0; // make compilers happy
}

/**
 * Given the direction the road depot is pointing, this is the direction the
 * vehicle should be travelling in in order to enter the depot.
 */
static const byte _roadveh_enter_depot_dir[4] = {
	TRACKDIR_X_SW, TRACKDIR_Y_NW, TRACKDIR_X_NE, TRACKDIR_Y_SE
};

static VehicleEnterTileStatus VehicleEnter_Misc(Vehicle *u, TileIndex tile, int x, int y)
{
	if (!IsGroundDepotTile(tile)) return VETSB_CONTINUE;

	if (IsRailDepot(tile)) {
		if (u->type != VEH_TRAIN) return VETSB_CONTINUE;

		Train *v = Train::From(u);

		/* depot direction */
		DiagDirection dir = GetRailDepotDirection(tile);

		byte fract_coord_x = x & 0xF;
		byte fract_coord_y = y & 0xF;

		/* make sure a train is not entering the tile from behind */
		assert(DistanceFromTileEdge(ReverseDiagDir(dir), fract_coord_x, fract_coord_y) != 0);

		if (v->direction == DiagDirToDir(ReverseDiagDir(dir))) {
			if (fract_coord_x == _fractcoords_enter_x[dir] && fract_coord_y == _fractcoords_enter_y[dir]) {
				/* enter the depot */
				v->track = TRACK_BIT_DEPOT,
				v->vehstatus |= VS_HIDDEN; // hide it
				v->direction = ReverseDir(v->direction);
				if (v->Next() == NULL) VehicleEnterDepot(v->First());
				v->tile = tile;

				InvalidateWindowData(WC_VEHICLE_DEPOT, v->tile);
				return VETSB_ENTERED_WORMHOLE;
			}
		} else if (v->direction == DiagDirToDir(dir)) {
			/* Calculate the point where the following wagon should be activated. */
			int length = v->CalcNextVehicleOffset();

			byte fract_coord_leave_x = _fractcoords_enter_x[dir] +
				(length + 1) * _deltacoord_leaveoffset_x[dir];
			byte fract_coord_leave_y = _fractcoords_enter_y[dir] +
				(length + 1) * _deltacoord_leaveoffset_y[dir];

			if (fract_coord_x == fract_coord_leave_x && fract_coord_y == fract_coord_leave_y) {
				/* leave the depot? */
				if ((v = v->Next()) != NULL) {
					v->vehstatus &= ~VS_HIDDEN;
					v->track = (DiagDirToAxis(dir) == AXIS_X ? TRACK_BIT_X : TRACK_BIT_Y);
				}
			}
		}
	} else {
		if (u->type != VEH_ROAD) return VETSB_CONTINUE;

		RoadVehicle *rv = RoadVehicle::From(u);
		if (rv->frame == RVC_DEPOT_STOP_FRAME &&
				_roadveh_enter_depot_dir[GetRoadDepotDirection(tile)] == rv->state) {
			rv->state = RVSB_IN_DEPOT;
			rv->vehstatus |= VS_HIDDEN;
			rv->direction = ReverseDir(rv->direction);
			if (rv->Next() == NULL) VehicleEnterDepot(rv->First());
			rv->tile = tile;

			InvalidateWindowData(WC_VEHICLE_DEPOT, rv->tile);
			return VETSB_ENTERED_WORMHOLE;
		}
	}

	return VETSB_CONTINUE;
}


static Foundation GetFoundation_Misc(TileIndex tile, Slope tileh)
{
	return FlatteningFoundation(tileh);
}


static CommandCost TerraformTile_Misc(TileIndex tile, DoCommandFlag flags, int z_new, Slope tileh_new)
{
	if (_settings_game.construction.build_on_slopes && AutoslopeEnabled()) {
		switch (GetTileSubtype(tile)) {
			default: NOT_REACHED();

			case TT_MISC_CROSSING:
				if (!IsSteepSlope(tileh_new) && (GetTileMaxZ(tile) == z_new + GetSlopeMaxZ(tileh_new)) && HasBit(VALID_LEVEL_CROSSING_SLOPES, tileh_new)) return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
				break;

			case TT_MISC_DEPOT: {
				DiagDirection dir = IsRailDepot(tile) ? GetRailDepotDirection(tile) : GetRoadDepotDirection(tile);
				if (AutoslopeCheckForEntranceEdge(tile, z_new, tileh_new, dir)) {
					return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
				}
				break;
			}
		}
	}

	return DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
}


extern const TileTypeProcs _tile_type_misc_procs = {
	DrawTile_Misc,           // draw_tile_proc
	GetSlopePixelZ_Misc,     // get_slope_z_proc
	ClearTile_Misc,          // clear_tile_proc
	NULL,                    // add_accepted_cargo_proc
	GetTileDesc_Misc,        // get_tile_desc_proc
	GetTileTrackStatus_Misc, // get_tile_track_status_proc
	ClickTile_Misc,          // click_tile_proc
	NULL,                    // animate_tile_proc
	TileLoop_Misc,           // tile_loop_proc
	ChangeTileOwner_Misc,    // change_tile_owner_proc
	NULL,                    // add_produced_cargo_proc
	VehicleEnter_Misc,       // vehicle_enter_tile_proc
	GetFoundation_Misc,      // get_foundation_proc
	TerraformTile_Misc,      // terraform_tile_proc
};
