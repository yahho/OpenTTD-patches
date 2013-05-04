/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file bridge_cmd.cpp
 * This file deals with bridges (non-gui stuff)
 */

#include "stdafx.h"
#include "bridge.h"
#include "tunnelbridge.h"
#include "landscape.h"
#include "slope_func.h"
#include "command_func.h"
#include "date_func.h"
#include "economy_func.h"
#include "map/ground.h"
#include "map/water.h"
#include "map/rail.h"
#include "map/road.h"
#include "map/slope.h"
#include "map/tunnelbridge.h"
#include "viewport_func.h"
#include "transparency.h"
#include "rail.h"
#include "elrail_func.h"
#include "clear_func.h"
#include "water.h"

#include "newgrf_commons.h"
#include "newgrf_railtype.h"
#include "newgrf_object.h"

#include "table/sprites.h"
#include "table/strings.h"
#include "table/bridge_land.h"


/** Data for CheckExtendedBridgeHead; see the function for details */
extern const Slope bridgehead_valid_slopes[DIAGDIR_END][2] = {
	{ SLOPE_W, SLOPE_S },
	{ SLOPE_N, SLOPE_W },
	{ SLOPE_E, SLOPE_N },
	{ SLOPE_S, SLOPE_E },
};


/** Z position of the bridge sprites relative to bridge height (downwards) */
static const int BRIDGE_Z_START = 3;

BridgeSpec _bridge[MAX_BRIDGES]; ///< The specification of all bridges.

/** Reset the data been eventually changed by the grf loaded. */
void ResetBridges()
{
	/* First, free sprite table data */
	for (BridgeType i = 0; i < MAX_BRIDGES; i++) {
		if (_bridge[i].sprite_table != NULL) {
			for (BridgePieces j = BRIDGE_PIECE_NORTH; j < BRIDGE_PIECE_INVALID; j++) free(_bridge[i].sprite_table[j]);
			free(_bridge[i].sprite_table);
		}
	}

	/* Then, wipe out current bridges */
	memset(&_bridge, 0, sizeof(_bridge));
	/* And finally, reinstall default data */
	memcpy(&_bridge, &_orig_bridge, sizeof(_orig_bridge));
}

/**
 * Calculate the price factor for building a long bridge.
 * Basically the cost delta is 1,1, 1, 2,2, 3,3,3, 4,4,4,4, 5,5,5,5,5, 6,6,6,6,6,6,  7,7,7,7,7,7,7,  8,8,8,8,8,8,8,8,
 * @param length Length of the bridge.
 * @return Price factor for the bridge.
 */
int CalcBridgeLenCostFactor(int length)
{
	if (length <= 2) return length;

	length -= 2;
	int sum = 2;

	int delta;
	for (delta = 1; delta < length; delta++) {
		sum += delta * delta;
		length -= delta;
	}
	sum += delta * length;
	return sum;
}

/**
 * Get the foundation for a bridge.
 * @param tileh The slope to build the bridge on.
 * @param axis The axis of the bridge entrance.
 * @return The foundation required.
 */
Foundation GetBridgeFoundation(Slope tileh, Axis axis)
{
	if (tileh == SLOPE_FLAT ||
			((tileh == SLOPE_NE || tileh == SLOPE_SW) && axis == AXIS_X) ||
			((tileh == SLOPE_NW || tileh == SLOPE_SE) && axis == AXIS_Y)) return FOUNDATION_NONE;

	return (HasSlopeHighestCorner(tileh) ? InclinedFoundation(axis) : FlatteningFoundation(tileh));
}

/**
 * Get the height ('z') of a bridge.
 * @param tile the bridge ramp tile to get the bridge height from
 * @return the height of the bridge.
 */
int GetBridgeHeight(TileIndex t)
{
	int h;
	Slope tileh = GetTileSlope(t, &h);
	Foundation f = GetBridgeFoundation(tileh, DiagDirToAxis(GetTunnelBridgeDirection(t)));

	/* one height level extra for the ramp */
	return h + 1 + ApplyFoundationToSlope(f, &tileh);
}

/**
 * Determines if the track on a bridge ramp is flat or goes up/down.
 *
 * @param tileh Slope of the tile under the bridge head
 * @param axis Orientation of bridge
 * @return true iff the track is flat.
 */
bool HasBridgeFlatRamp(Slope tileh, Axis axis)
{
	ApplyFoundationToSlope(GetBridgeFoundation(tileh, axis), &tileh);
	/* If the foundation slope is flat the bridge has a non-flat ramp and vice versa. */
	return (tileh != SLOPE_FLAT);
}

/**
 * Check tiles validity for a bridge.
 *
 * @param tile1 Start tile
 * @param tile2 End tile
 * @param axis Pointer to receive bridge axis, or NULL
 * @return Null cost for success or an error
 */
CommandCost CheckBridgeTiles(TileIndex tile1, TileIndex tile2, Axis *axis)
{
	if (!IsValidTile(tile1) || !IsValidTile(tile2)) return_cmd_error(STR_ERROR_BRIDGE_THROUGH_MAP_BORDER);

	if (tile1 == tile2) {
		return_cmd_error(STR_ERROR_CAN_T_START_AND_END_ON);
	} else if (TileX(tile1) == TileX(tile2)) {
		if (axis != NULL) *axis = AXIS_Y;
	} else if (TileY(tile1) == TileY(tile2)) {
		if (axis != NULL) *axis = AXIS_X;
	} else {
		return_cmd_error(STR_ERROR_START_AND_END_MUST_BE_IN);
	}

	return CommandCost();
}

/**
 * Check if a bridge can be built.
 *
 * @param tile1 Start tile
 * @param tile2 End tile
 * @param flags type of operation
 * @param clear1 Try to clear start tile
 * @param clear2 Try to clear end tile
 * @param restricted Force flat ramp (for aqueducts)
 * @return Terraforming cost for success or an error
 */
CommandCost CheckBridgeBuildable(TileIndex tile1, TileIndex tile2, DoCommandFlag flags, bool clear1, bool clear2, bool restricted)
{
	DiagDirection dir = DiagdirBetweenTiles(tile1, tile2);
	assert(IsValidDiagDirection(dir));

	int z1;
	int z2;
	Slope tileh1 = GetTileSlope(tile1, &z1);
	Slope tileh2 = GetTileSlope(tile2, &z2);

	CommandCost terraform1 = CheckBridgeSlope(dir, &tileh1, &z1);
	CommandCost terraform2 = CheckBridgeSlope(ReverseDiagDir(dir), &tileh2, &z2);

	if (restricted && (tileh1 == SLOPE_FLAT || tileh2 == SLOPE_FLAT)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
	if (z1 != z2) return_cmd_error(STR_ERROR_BRIDGEHEADS_NOT_SAME_HEIGHT);

	bool allow_on_slopes = (_settings_game.construction.build_on_slopes && !restricted);

	CommandCost cost;

	if (clear1) {
		/* Try and clear the start landscape */
		CommandCost ret = DoCommand(tile1, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);

		if (terraform1.Failed() || (terraform1.GetCost() != 0 && !allow_on_slopes)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
		cost.AddCost(terraform1);
	} else {
		assert(terraform1.Succeeded());
	}

	if (clear2) {
		/* Try and clear the end landscape */
		CommandCost ret = DoCommand(tile2, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);

		if (terraform2.Failed() || (terraform2.GetCost() != 0 && !allow_on_slopes)) return_cmd_error(STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION);
		cost.AddCost(terraform2);
	} else {
		assert(terraform2.Succeeded());
	}

	const TileIndex heads[] = {tile1, tile2};
	for (int i = 0; i < 2; i++) {
		if (HasBridgeAbove(heads[i])) {
			if (DiagDirToAxis(dir) == GetBridgeAxis(heads[i])) return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);

			if (z1 + 1 == GetBridgeHeight(GetNorthernBridgeEnd(heads[i]))) {
				return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
			}
		}
	}

	TileIndexDiff delta = TileOffsByDiagDir(dir);

	for (TileIndex tile = tile1 + delta; tile != tile2; tile += delta) {
		if (GetTileMaxZ(tile) > z1) return_cmd_error(STR_ERROR_BRIDGE_TOO_LOW_FOR_TERRAIN);

		if (HasBridgeAbove(tile)) {
			/* Disallow crossing bridges for the time being */
			return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
		}

		switch (GetTileType(tile)) {
			case TT_WATER:
				if (IsPlainWater(tile) || IsCoast(tile)) continue;
				break;

			case TT_MISC:
				if (IsTileSubtype(tile, TT_MISC_TUNNEL)) continue;
				if (IsTileSubtype(tile, TT_MISC_DEPOT)) break;
				assert(TT_BRIDGE == TT_MISC_AQUEDUCT);
				/* fall through */
			case TT_RAILWAY:
			case TT_ROAD:
				if (!IsTileSubtype(tile, TT_BRIDGE)) continue;
				if (DiagDirToAxis(dir) == DiagDirToAxis(GetTunnelBridgeDirection(tile))) break;
				if (z1 >= GetBridgeHeight(tile)) continue;
				break;

			case TT_OBJECT: {
				const ObjectSpec *spec = ObjectSpec::GetByTile(tile);
				if ((spec->flags & OBJECT_FLAG_ALLOW_UNDER_BRIDGE) == 0) break;
				if (z1 >= GetTileMaxZ(tile) + spec->height) continue;
				break;
			}

			case TT_GROUND:
				assert(IsGroundTile(tile));
				if (!IsTileSubtype(tile, TT_GROUND_TREES)) continue;
				break;

			default:
				break;
		}

		/* try and clear the middle landscape */
		CommandCost ret = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
		if (ret.Failed()) return ret;
		cost.AddCost(ret);
	}

	return cost;
}

/**
 * Is a bridge of the specified type and length available?
 * @param bridge_type Wanted type of bridge.
 * @param bridge_len  Wanted length of the bridge.
 * @return A succeeded (the requested bridge is available) or failed (it cannot be built) command.
 */
CommandCost CheckBridgeAvailability(BridgeType bridge_type, uint bridge_len, DoCommandFlag flags)
{
	if (flags & DC_QUERY_COST) {
		if (bridge_len <= _settings_game.construction.max_bridge_length) return CommandCost();
		return_cmd_error(STR_ERROR_BRIDGE_TOO_LONG);
	}

	if (bridge_type >= MAX_BRIDGES) return CMD_ERROR;

	const BridgeSpec *b = GetBridgeSpec(bridge_type);
	if (b->avail_year > _cur_year) return CMD_ERROR;

	uint max = min(b->max_length, _settings_game.construction.max_bridge_length);

	if (b->min_length > bridge_len) return CMD_ERROR;
	if (bridge_len <= max) return CommandCost();
	return_cmd_error(STR_ERROR_BRIDGE_TOO_LONG);
}

/**
 * Determines the foundation for a bridge head, and tests if the resulting slope is valid.
 *
 * @param dir Diagonal direction the bridge ramp will be facing
 * @param tileh Slope of the tile under the bridge head; returns slope on top of foundation
 * @param z TileZ corresponding to tileh, gets modified as well
 * @return Error or cost for bridge foundation
 */
CommandCost CheckBridgeSlope(DiagDirection dir, Slope *tileh, int *z)
{
	static const Slope inclined[DIAGDIR_END] = {
		SLOPE_SW,     ///< DIAGDIR_NE
		SLOPE_NW,     ///< DIAGDIR_SE
		SLOPE_NE,     ///< DIAGDIR_SW
		SLOPE_SE,     ///< DIAGDIR_NW
	};

	Foundation f = GetBridgeFoundation(*tileh, DiagDirToAxis(dir));
	*z += ApplyFoundationToSlope(f, tileh);

	if ((*tileh != SLOPE_FLAT) && (*tileh != inclined[dir])) return CMD_ERROR;

	if (f == FOUNDATION_NONE) return CommandCost();

	return CommandCost(EXPENSES_CONSTRUCTION, _price[PR_BUILD_FOUNDATION]);
}

/**
 * Set bridge axis on a new bridge middle tiles, and mark them dirty
 *
 * @param tile1 Bridge start tile
 * @param tile2 Bridge end tile
 * @param direction Bridge axis
 */
void SetBridgeMiddleTiles(TileIndex tile1, TileIndex tile2, Axis direction)
{
	assert(tile1 < tile2);

	MarkTileDirtyByTile(tile1);
	MarkTileDirtyByTile(tile2);

	TileIndexDiff delta = TileOffsByDiagDir(AxisToDiagDir(direction));
	for (TileIndex tile = tile1 + delta; tile < tile2; tile += delta) {
		SetBridgeMiddle(tile, direction);
		MarkTileDirtyByTile(tile);
	}
}

/**
 * Clear middle bridge tiles
 *
 * @param tile1 Bridge start tile
 * @param tile2 Bridge end tile
 */
void RemoveBridgeMiddleTiles(TileIndex tile1, TileIndex tile2)
{
	/* Call this function before clearing the endpoints. */
	assert(IsBridgeHeadTile(tile1));
	assert(IsBridgeHeadTile(tile2));

	TileIndexDiff delta = TileOffsByDiagDir(GetTunnelBridgeDirection(tile1));
	int height = GetBridgeHeight(tile1);

	for (TileIndex t = tile1 + delta; t != tile2; t += delta) {
		/* do not let trees appear from 'nowhere' after removing bridge */
		if (IsNormalRoadTile(t) && GetRoadside(t) == ROADSIDE_TREES) {
			int minz = GetTileMaxZ(t) + 3;
			if (height < minz) SetRoadside(t, ROADSIDE_PAVED);
		}
		ClearBridgeMiddle(t);
		MarkTileDirtyByTile(t);
	}
}


static inline const PalSpriteID *GetBridgeSpriteTable(int index, BridgePieces table)
{
	const BridgeSpec *bridge = GetBridgeSpec(index);
	assert(table < BRIDGE_PIECE_INVALID);
	if (bridge->sprite_table == NULL || bridge->sprite_table[table] == NULL) {
		return _bridge_sprite_table[index][table];
	} else {
		return bridge->sprite_table[table];
	}
}

/**
 * Draw a single pillar sprite.
 * @param psid      Pillarsprite
 * @param x         Pillar X
 * @param y         Pillar Y
 * @param z         Pillar Z
 * @param w         Bounding box size in X direction
 * @param h         Bounding box size in Y direction
 * @param subsprite Optional subsprite for drawing halfpillars
 */
static inline void DrawPillar(const PalSpriteID *psid, int x, int y, int z, int w, int h, const SubSprite *subsprite)
{
	static const int PILLAR_Z_OFFSET = TILE_HEIGHT - BRIDGE_Z_START; ///< Start offset of pillar wrt. bridge (downwards)
	AddSortableSpriteToDraw(psid->sprite, psid->pal, x, y, w, h, BB_HEIGHT_UNDER_BRIDGE - PILLAR_Z_OFFSET, z, IsTransparencySet(TO_BRIDGES), 0, 0, -PILLAR_Z_OFFSET, subsprite);
}

/**
 * Draw two bridge pillars (north and south).
 * @param z_bottom Bottom Z
 * @param z_top    Top Z
 * @param psid     Pillarsprite
 * @param x        Pillar X
 * @param y        Pillar Y
 * @param w        Bounding box size in X direction
 * @param h        Bounding box size in Y direction
 * @return Reached Z at the bottom
 */
static int DrawPillarColumn(int z_bottom, int z_top, const PalSpriteID *psid, int x, int y, int w, int h)
{
	int cur_z;
	for (cur_z = z_top; cur_z >= z_bottom; cur_z -= TILE_HEIGHT) {
		DrawPillar(psid, x, y, cur_z, w, h, NULL);
	}
	return cur_z;
}

/**
 * Draws the pillars under high bridges.
 *
 * @param psid Image and palette of a bridge pillar.
 * @param ti #TileInfo of current bridge-middle-tile.
 * @param axis Orientation of bridge.
 * @param drawfarpillar Whether to draw the pillar at the back
 * @param x Sprite X position of front pillar.
 * @param y Sprite Y position of front pillar.
 * @param z_bridge Absolute height of bridge bottom.
 */
static void DrawBridgePillars(const PalSpriteID *psid, const TileInfo *ti, Axis axis, bool drawfarpillar, int x, int y, int z_bridge)
{
	static const int bounding_box_size[2]  = {16, 2}; ///< bounding box size of pillars along bridge direction
	static const int back_pillar_offset[2] = { 0, 9}; ///< sprite position offset of back facing pillar

	static const int INF = 1000; ///< big number compared to sprite size
	static const SubSprite half_pillar_sub_sprite[2][2] = {
		{ {  -14, -INF, INF, INF }, { -INF, -INF, -15, INF } }, // X axis, north and south
		{ { -INF, -INF,  15, INF }, {   16, -INF, INF, INF } }, // Y axis, north and south
	};

	if (psid->sprite == 0) return;

	/* Determine ground height under pillars */
	DiagDirection south_dir = AxisToDiagDir(axis);
	int z_front_north = ti->z;
	int z_back_north = ti->z;
	int z_front_south = ti->z;
	int z_back_south = ti->z;
	GetSlopePixelZOnEdge(ti->tileh, south_dir, &z_front_south, &z_back_south);
	GetSlopePixelZOnEdge(ti->tileh, ReverseDiagDir(south_dir), &z_front_north, &z_back_north);

	/* Shared height of pillars */
	int z_front = max(z_front_north, z_front_south);
	int z_back = max(z_back_north, z_back_south);

	/* x and y size of bounding-box of pillars */
	int w = bounding_box_size[axis];
	int h = bounding_box_size[OtherAxis(axis)];
	/* sprite position of back facing pillar */
	int x_back = x - back_pillar_offset[axis];
	int y_back = y - back_pillar_offset[OtherAxis(axis)];

	/* Draw front pillars */
	int bottom_z = DrawPillarColumn(z_front, z_bridge, psid, x, y, w, h);
	if (z_front_north < z_front) DrawPillar(psid, x, y, bottom_z, w, h, &half_pillar_sub_sprite[axis][0]);
	if (z_front_south < z_front) DrawPillar(psid, x, y, bottom_z, w, h, &half_pillar_sub_sprite[axis][1]);

	/* Draw back pillars, skip top two parts, which are hidden by the bridge */
	int z_bridge_back = z_bridge - 2 * (int)TILE_HEIGHT;
	if (drawfarpillar && (z_back_north <= z_bridge_back || z_back_south <= z_bridge_back)) {
		bottom_z = DrawPillarColumn(z_back, z_bridge_back, psid, x_back, y_back, w, h);
		if (z_back_north < z_back) DrawPillar(psid, x_back, y_back, bottom_z, w, h, &half_pillar_sub_sprite[axis][0]);
		if (z_back_south < z_back) DrawPillar(psid, x_back, y_back, bottom_z, w, h, &half_pillar_sub_sprite[axis][1]);
	}
}

/**
 * Compute bridge piece. Computes the bridge piece to display depending on the position inside the bridge.
 * bridges pieces sequence (middle parts).
 * Note that it is not covering the bridge heads, which are always referenced by the same sprite table.
 * bridge len 1: BRIDGE_PIECE_NORTH
 * bridge len 2: BRIDGE_PIECE_NORTH  BRIDGE_PIECE_SOUTH
 * bridge len 3: BRIDGE_PIECE_NORTH  BRIDGE_PIECE_MIDDLE_ODD   BRIDGE_PIECE_SOUTH
 * bridge len 4: BRIDGE_PIECE_NORTH  BRIDGE_PIECE_INNER_NORTH  BRIDGE_PIECE_INNER_SOUTH  BRIDGE_PIECE_SOUTH
 * bridge len 5: BRIDGE_PIECE_NORTH  BRIDGE_PIECE_INNER_NORTH  BRIDGE_PIECE_MIDDLE_EVEN  BRIDGE_PIECE_INNER_SOUTH  BRIDGE_PIECE_SOUTH
 * bridge len 6: BRIDGE_PIECE_NORTH  BRIDGE_PIECE_INNER_NORTH  BRIDGE_PIECE_INNER_SOUTH  BRIDGE_PIECE_INNER_NORTH  BRIDGE_PIECE_INNER_SOUTH  BRIDGE_PIECE_SOUTH
 * bridge len 7: BRIDGE_PIECE_NORTH  BRIDGE_PIECE_INNER_NORTH  BRIDGE_PIECE_INNER_SOUTH  BRIDGE_PIECE_MIDDLE_ODD   BRIDGE_PIECE_INNER_NORTH  BRIDGE_PIECE_INNER_SOUTH  BRIDGE_PIECE_SOUTH
 * #0 - always as first, #1 - always as last (if len>1)
 * #2,#3 are to pair in order
 * for odd bridges: #5 is going in the bridge middle if on even position, #4 on odd (counting from 0)
 * @param north Northernmost tile of bridge
 * @param south Southernmost tile of bridge
 * @return Index of bridge piece
 */
static BridgePieces CalcBridgePiece(uint north, uint south)
{
	if (north == 1) {
		return BRIDGE_PIECE_NORTH;
	} else if (south == 1) {
		return BRIDGE_PIECE_SOUTH;
	} else if (north < south) {
		return north & 1 ? BRIDGE_PIECE_INNER_SOUTH : BRIDGE_PIECE_INNER_NORTH;
	} else if (north > south) {
		return south & 1 ? BRIDGE_PIECE_INNER_NORTH : BRIDGE_PIECE_INNER_SOUTH;
	} else {
		return north & 1 ? BRIDGE_PIECE_MIDDLE_EVEN : BRIDGE_PIECE_MIDDLE_ODD;
	}
}

/**
 * Draws the trambits over an already drawn (lower end) of a bridge.
 * @param x       the x of the bridge
 * @param y       the y of the bridge
 * @param z       the z of the bridge
 * @param offset  number representing whether to level or sloped and the direction
 * @param overlay do we want to still see the road?
 * @param head    are we drawing bridge head?
 */
void DrawBridgeTramBits(int x, int y, int z, int offset, bool overlay, bool head)
{
	static const SpriteID tram_offsets[2][6] = { { 107, 108, 109, 110, 111, 112 }, { 4, 5, 15, 16, 17, 18 } };
	static const SpriteID back_offsets[6]    =   {  95,  96,  99, 102, 100, 101 };
	static const SpriteID front_offsets[6]   =   {  97,  98, 103, 106, 104, 105 };

	static const uint size_x[6] = {  1, 16, 16,  1, 16,  1 };
	static const uint size_y[6] = { 16,  1,  1, 16,  1, 16 };
	static const uint front_bb_offset_x[6] = { 15,  0,  0, 15,  0, 15 };
	static const uint front_bb_offset_y[6] = {  0, 15, 15,  0, 15,  0 };

	/* The sprites under the vehicles are drawn as SpriteCombine. StartSpriteCombine() has already been called
	 * The bounding boxes here are the same as for bridge front/roof */
	if (head || !IsInvisibilitySet(TO_BRIDGES)) {
		AddSortableSpriteToDraw(SPR_TRAMWAY_BASE + tram_offsets[overlay][offset], PAL_NONE,
			x, y, size_x[offset], size_y[offset], 0x28, z,
			!head && IsTransparencySet(TO_BRIDGES));
	}

	/* Do not draw catenary if it is set invisible */
	if (!IsInvisibilitySet(TO_CATENARY)) {
		AddSortableSpriteToDraw(SPR_TRAMWAY_BASE + back_offsets[offset], PAL_NONE,
			x, y, size_x[offset], size_y[offset], 0x28, z,
			IsTransparencySet(TO_CATENARY));
	}

	/* Start a new SpriteCombine for the front part */
	EndSpriteCombine();
	StartSpriteCombine();

	/* For sloped sprites the bounding box needs to be higher, as the pylons stop on a higher point */
	if (!IsInvisibilitySet(TO_CATENARY)) {
		AddSortableSpriteToDraw(SPR_TRAMWAY_BASE + front_offsets[offset], PAL_NONE,
			x, y, size_x[offset] + front_bb_offset_x[offset], size_y[offset] + front_bb_offset_y[offset], 0x28, z,
			IsTransparencySet(TO_CATENARY), front_bb_offset_x[offset], front_bb_offset_y[offset]);
	}
}

/**
 * Draw the middle bits of a bridge.
 * @param ti Tile information of the tile to draw it on.
 */
void DrawBridgeMiddle(const TileInfo *ti)
{
	/* Sectional view of bridge bounding boxes:
	 *
	 *  1           2                                1,2 = SpriteCombine of Bridge front/(back&floor) and TramCatenary
	 *  1           2                                  3 = empty helper BB
	 *  1     7     2                                4,5 = pillars under higher bridges
	 *  1 6 88888 6 2                                  6 = elrail-pylons
	 *  1 6 88888 6 2                                  7 = elrail-wire
	 *  1 6 88888 6 2  <- TILE_HEIGHT                  8 = rail-vehicle on bridge
	 *  3333333333333  <- BB_Z_SEPARATOR
	 *                 <- unused
	 *    4       5    <- BB_HEIGHT_UNDER_BRIDGE
	 *    4       5
	 *    4       5
	 *
	 */

	if (!IsBridgeAbove(ti->tile)) return;

	TileIndex rampnorth = GetNorthernBridgeEnd(ti->tile);
	TileIndex rampsouth = GetSouthernBridgeEnd(ti->tile);

	Axis axis = GetBridgeAxis(ti->tile);
	BridgePieces piece = CalcBridgePiece(
		GetTunnelBridgeLength(ti->tile, rampnorth) + 1,
		GetTunnelBridgeLength(ti->tile, rampsouth) + 1
	);

	TransportType transport_type;
	const PalSpriteID *psid;
	bool drawfarpillar;

	if (IsTileType(rampsouth, TT_MISC)) {
		assert(IsAqueductTile(rampsouth));
		transport_type = TRANSPORT_WATER;
		psid = _aqueduct_sprites;
		drawfarpillar = true;
	} else {
		assert(IsTileSubtype(rampsouth, TT_BRIDGE));

		BridgeType type;
		uint base_offset;

		if (IsRailwayTile(rampsouth)) {
			transport_type = TRANSPORT_RAIL;
			type = GetRailBridgeType(rampsouth);
			base_offset = GetRailTypeInfo(GetBridgeRailType(rampsouth))->bridge_offset;
		} else {
			transport_type = TRANSPORT_ROAD;
			type = GetRoadBridgeType(rampsouth);
			base_offset = 8;
		}

		psid = base_offset + GetBridgeSpriteTable(type, piece);
		drawfarpillar = !HasBit(GetBridgeSpec(type)->flags, 0);
	}

	if (axis != AXIS_X) psid += 4;

	int x = ti->x;
	int y = ti->y;
	uint bridge_z = GetBridgePixelHeight(rampsouth);
	int z = bridge_z - BRIDGE_Z_START;

	/* Add a bounding box that separates the bridge from things below it. */
	AddSortableSpriteToDraw(SPR_EMPTY_BOUNDING_BOX, PAL_NONE, x, y, 16, 16, 1, bridge_z - TILE_HEIGHT + BB_Z_SEPARATOR);

	/* Draw Trambits as SpriteCombine */
	if (transport_type == TRANSPORT_ROAD || transport_type == TRANSPORT_RAIL) StartSpriteCombine();

	/* Draw floor and far part of bridge*/
	if (!IsInvisibilitySet(TO_BRIDGES)) {
		if (axis == AXIS_X) {
			AddSortableSpriteToDraw(psid->sprite, psid->pal, x, y, 16, 1, 0x28, z, IsTransparencySet(TO_BRIDGES), 0, 0, BRIDGE_Z_START);
		} else {
			AddSortableSpriteToDraw(psid->sprite, psid->pal, x, y, 1, 16, 0x28, z, IsTransparencySet(TO_BRIDGES), 0, 0, BRIDGE_Z_START);
		}
	}

	psid++;

	if (transport_type == TRANSPORT_ROAD) {
		RoadBits bits = DiagDirToRoadBits(axis == AXIS_X ? DIAGDIR_NE : DIAGDIR_NW);

		if ((GetRoadBits(rampsouth, ROADTYPE_TRAM) & bits) != 0) {
			/* DrawBridgeTramBits() calls EndSpriteCombine() and StartSpriteCombine() */
			DrawBridgeTramBits(x, y, bridge_z, axis ^ 1, (GetRoadBits(rampsouth, ROADTYPE_ROAD) & bits) != 0, false);
		} else {
			EndSpriteCombine();
			StartSpriteCombine();
		}
	} else if (transport_type == TRANSPORT_RAIL) {
		const RailtypeInfo *rti = GetRailTypeInfo(GetBridgeRailType(rampsouth));
		if (rti->UsesOverlay() && !IsInvisibilitySet(TO_BRIDGES)) {
			SpriteID surface = GetCustomRailSprite(rti, rampsouth, RTSG_BRIDGE, TCX_ON_BRIDGE);
			if (surface != 0) {
				AddSortableSpriteToDraw(surface + axis, PAL_NONE, x, y, 16, 16, 0, bridge_z, IsTransparencySet(TO_BRIDGES));
			}
		}
		EndSpriteCombine();

		if (HasCatenaryDrawn(GetBridgeRailType(rampsouth))) {
			DrawCatenaryOnBridge(ti);
		}
	}

	/* draw roof, the component of the bridge which is logically between the vehicle and the camera */
	if (!IsInvisibilitySet(TO_BRIDGES)) {
		if (axis == AXIS_X) {
			y += 12;
			if (psid->sprite & SPRITE_MASK) AddSortableSpriteToDraw(psid->sprite, psid->pal, x, y, 16, 4, 0x28, z, IsTransparencySet(TO_BRIDGES), 0, 3, BRIDGE_Z_START);
		} else {
			x += 12;
			if (psid->sprite & SPRITE_MASK) AddSortableSpriteToDraw(psid->sprite, psid->pal, x, y, 4, 16, 0x28, z, IsTransparencySet(TO_BRIDGES), 3, 0, BRIDGE_Z_START);
		}
	}

	/* Draw TramFront as SpriteCombine */
	if (transport_type == TRANSPORT_ROAD) EndSpriteCombine();

	/* Do not draw anything more if bridges are invisible */
	if (IsInvisibilitySet(TO_BRIDGES)) return;

	psid++;
	if (ti->z + 5 == z) {
		/* draw poles below for small bridges */
		if (psid->sprite != 0) {
			SpriteID image = psid->sprite;
			SpriteID pal   = psid->pal;
			if (IsTransparencySet(TO_BRIDGES)) {
				SetBit(image, PALETTE_MODIFIER_TRANSPARENT);
				pal = PALETTE_TO_TRANSPARENT;
			}

			DrawGroundSpriteAt(image, pal, x - ti->x, y - ti->y, z - ti->z);
		}
	} else {
		/* draw pillars below for high bridges */
		DrawBridgePillars(psid, ti, axis, drawfarpillar, x, y, z);
	}
}

void DrawBridgeGround(TileInfo *ti)
{
	DiagDirection dir = GetTunnelBridgeDirection(ti->tile);

	DrawFoundation(ti, GetBridgeFoundation(ti->tileh, DiagDirToAxis(dir)));

	if (IsOnSnow(ti->tile)) {
		DrawGroundSprite(SPR_FLAT_SNOW_DESERT_TILE + SlopeToSpriteOffset(ti->tileh), PAL_NONE);
	} else {
		TileIndex next = ti->tile + TileOffsByDiagDir(dir);
		if (ti->tileh != SLOPE_FLAT && ti->z == 0 && HasTileWaterClass(next) && GetWaterClass(next) == WATER_CLASS_SEA) {
			DrawShoreTile(ti->tileh);
		} else {
			DrawClearLandTile(ti, 3);
		}
	}
}

const PalSpriteID *GetBridgeRampSprite(int index, int offset, Slope slope, DiagDirection dir)
{
	/* as the lower 3 bits are used for other stuff, make sure they are clear */
	assert((offset & 0x07) == 0x00);

	if (slope == SLOPE_FLAT) offset += 4; // sloped bridge head

	/* HACK Wizardry to convert the bridge ramp direction into a sprite offset */
	offset += (6 - dir) % 4;

	/* Table number BRIDGE_PIECE_HEAD always refers to the bridge heads for any bridge type */
	return offset + GetBridgeSpriteTable(index, BRIDGE_PIECE_HEAD);
}

void DrawAqueductRamp(TileInfo *ti)
{
	DrawBridgeGround(ti);

	assert(ti->tileh != SLOPE_FLAT);

	DiagDirection dir = GetTunnelBridgeDirection(ti->tile);

	/* HACK Wizardry to convert the bridge ramp direction into a sprite offset */
	const PalSpriteID *psid = _aqueduct_sprites + 8 + (6 - dir) % 4;

	/* HACK set the height of the BB of a sloped ramp to 1 so a vehicle on
	 * it doesn't disappear behind it
	 */
	/* Bridge heads are drawn solid no matter how invisibility/transparency is set */
	AddSortableSpriteToDraw(psid->sprite, psid->pal, ti->x, ti->y, 16, 16, 8, ti->z);
}
