/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tree_cmd.cpp Handling of tree tiles. */

#include "stdafx.h"
#include "map/ground.h"
#include "map/slope.h"
#include "landscape.h"
#include "viewport_func.h"
#include "command_func.h"
#include "town.h"
#include "genworld.h"
#include "clear_func.h"
#include "company_func.h"
#include "water.h"
#include "company_base.h"
#include "core/random_func.hpp"
#include "newgrf_generic.h"

#include "table/strings.h"
#include "table/tree_land.h"

/**
 * List of tree placer algorithm.
 *
 * This enumeration defines all possible tree placer algorithm in the game.
 */
enum TreePlacer {
	TP_NONE,     ///< No tree placer algorithm
	TP_ORIGINAL, ///< The original algorithm
	TP_IMPROVED, ///< A 'improved' algorithm
};

/** Where to place trees while in-game? */
enum ExtraTreePlacement {
	ETP_NONE,       ///< Place trees on no tiles
	ETP_RAINFOREST, ///< Place trees only on rainforest tiles
	ETP_ALL,        ///< Place trees on all tiles
};

/** Determines when to consider building more trees. */
byte _trees_tick_ctr;

static const uint16 DEFAULT_TREE_STEPS = 1000;             ///< Default number of attempts for placing trees.
static const uint16 DEFAULT_RAINFOREST_TREE_STEPS = 15000; ///< Default number of attempts for placing extra trees at rainforest in tropic.
static const uint16 EDITOR_TREE_DIV = 5;                   ///< Game editor tree generation divisor factor.

/**
 * Tests if a tile can be converted to have trees
 * This is true for clear ground without farms or rocks.
 *
 * @param tile the tile of interest
 * @param allow_desert Allow planting trees on GROUND_DESERT?
 * @return true if trees can be built.
 */
static bool CanPlantTreesOnTile(TileIndex tile, bool allow_desert)
{
	switch (GetTileType(tile)) {
		case TT_WATER:
			return !IsBridgeAbove(tile) && IsCoast(tile) && !IsSlopeWithOneCornerRaised(GetTileSlope(tile));

		case TT_GROUND:
			return IsTileSubtype(tile, TT_GROUND_CLEAR) && !IsBridgeAbove(tile) && GetRawClearGround(tile) != GROUND_ROCKS &&
			       (allow_desert || !IsClearGround(tile, GROUND_DESERT));

		default: return false;
	}
}

/**
 * Creates a tree tile
 * Ground type and density is preserved.
 *
 * @pre the tile must be suitable for trees.
 *
 * @param tile where to plant the trees.
 * @param treetype The type of the tree
 * @param count the number of trees (minus 1)
 * @param growth the growth status
 */
static void PlantTreesOnTile(TileIndex tile, TreeType treetype, uint count, uint growth)
{
	assert(treetype != TREE_INVALID);
	assert(CanPlantTreesOnTile(tile, true));

	Ground ground;
	uint density;

	switch (GetTileType(tile)) {
		case TT_WATER:
			ground = GROUND_SHORE;
			density = 3;
			break;

		case TT_GROUND:
			assert(IsTileSubtype(tile, TT_GROUND_CLEAR));
			ground = GetFullClearGround(tile);
			density = GetClearDensity(tile);
			break;

		default: NOT_REACHED();
	}

	MakeTree(tile, treetype, count, growth, ground, density);
}

void AddNeighbouringTree(TileIndex tile)
{
	/* Don't plant extra trees if that's not allowed. */
	if ((_settings_game.game_creation.landscape == LT_TROPIC && GetTropicZone(tile) == TROPICZONE_RAINFOREST) ?
			_settings_game.construction.extra_tree_placement == ETP_NONE :
			_settings_game.construction.extra_tree_placement != ETP_ALL) {
		return;
	}

	TreeType treetype = GetTreeType(tile);

	tile += TileOffsByDir((Direction)(Random() & 7));

	/* Cacti don't spread */
	if (!CanPlantTreesOnTile(tile, false)) return;

	/* Don't plant trees, if ground was freshly cleared */
	if (IsClearTile(tile) && GetClearGround(tile) == GROUND_GRASS && GetClearDensity(tile) != 3) return;

	PlantTreesOnTile(tile, treetype, 0, 0);
}

/**
 * Get a random TreeType for the given tile based on a given seed
 *
 * This function returns a random TreeType which can be placed on the given tile.
 * The seed for randomness must be less or equal 256, use #GB on the value of Random()
 * to get such a value.
 *
 * @param tile The tile to get a random TreeType from
 * @param seed The seed for randomness, must be less or equal 256
 * @return The random tree type
 */
static TreeType GetRandomTreeType(TileIndex tile, uint seed)
{
	switch (_settings_game.game_creation.landscape) {
		case LT_TEMPERATE:
			return (TreeType)(seed * TREE_COUNT_TEMPERATE / 256 + TREE_TEMPERATE);

		case LT_ARCTIC:
			return (TreeType)(seed * TREE_COUNT_SUB_ARCTIC / 256 + TREE_SUB_ARCTIC);

		case LT_TROPIC:
			switch (GetTropicZone(tile)) {
				case TROPICZONE_NORMAL:  return (TreeType)(seed * TREE_COUNT_SUB_TROPICAL / 256 + TREE_SUB_TROPICAL);
				case TROPICZONE_DESERT:  return (TreeType)((seed > 12) ? TREE_INVALID : TREE_CACTUS);
				default:                 return (TreeType)(seed * TREE_COUNT_RAINFOREST / 256 + TREE_RAINFOREST);
			}

		default:
			return (TreeType)(seed * TREE_COUNT_TOYLAND / 256 + TREE_TOYLAND);
	}
}

/**
 * Make a random tree tile of the given tile
 *
 * Create a new tree-tile for the given tile. The second parameter is used for
 * randomness like type and number of trees.
 *
 * @param tile The tile to make a tree-tile from
 * @param r The randomness value from a Random() value
 */
static void PlaceTree(TileIndex tile, uint32 r)
{
	TreeType tree = GetRandomTreeType(tile, GB(r, 24, 8));

	if (tree != TREE_INVALID) {
		PlantTreesOnTile(tile, tree, GB(r, 22, 2), min(GB(r, 16, 3), 6));

		/* Rerandomize ground, if neither snow nor shore */
		Ground ground = GetClearGround(tile);
		if (ground == GROUND_GRASS || ground == GROUND_ROUGH ) {
			SetClearGroundDensity(tile, GB(r, 28, 1) ? GROUND_ROUGH : GROUND_GRASS, 3);
		}

		/* Set the counter to a random start value */
		SetClearCounter(tile, (Ground)GB(r, 24, 4));
	}
}

/**
 * Creates a number of tree groups.
 * The number of trees in each group depends on how many trees are actually placed around the given tile.
 *
 * @param num_groups Number of tree groups to place.
 */
static void PlaceTreeGroups(uint num_groups)
{
	do {
		TileIndex center_tile = RandomTile();

		for (uint i = 0; i < DEFAULT_TREE_STEPS; i++) {
			uint32 r = Random();
			int x = GB(r, 0, 5) - 16;
			int y = GB(r, 8, 5) - 16;
			uint dist = abs(x) + abs(y);
			TileIndex cur_tile = TileAddWrap(center_tile, x, y);

			IncreaseGeneratingWorldProgress(GWP_TREE);

			if (cur_tile != INVALID_TILE && dist <= 13 && CanPlantTreesOnTile(cur_tile, true)) {
				PlaceTree(cur_tile, r);
			}
		}

	} while (--num_groups);
}

/**
 * Place a tree at the same height as an existing tree.
 *
 * Add a new tree around the given tile which is at the same
 * height or at some offset (2 units) of it.
 *
 * @param tile The base tile to add a new tree somewhere around
 * @param height The height (like the one from the tile)
 */
static void PlaceTreeAtSameHeight(TileIndex tile, int height)
{
	for (uint i = 0; i < DEFAULT_TREE_STEPS; i++) {
		uint32 r = Random();
		int x = GB(r, 0, 5) - 16;
		int y = GB(r, 8, 5) - 16;
		TileIndex cur_tile = TileAddWrap(tile, x, y);
		if (cur_tile == INVALID_TILE) continue;

		/* Keep in range of the existing tree */
		if (abs(x) + abs(y) > 16) continue;

		/* Clear tile, no farm-tiles or rocks */
		if (!CanPlantTreesOnTile(cur_tile, true)) continue;

		/* Not too much height difference */
		if (Delta(GetTileZ(cur_tile), height) > 2) continue;

		/* Place one tree and quit */
		PlaceTree(cur_tile, r);
		break;
	}
}

/**
 * Place some trees randomly
 *
 * This function just place some trees randomly on the map.
 */
void PlaceTreesRandomly()
{
	int i, j, ht;

	i = ScaleByMapSize(DEFAULT_TREE_STEPS);
	if (_game_mode == GM_EDITOR) i /= EDITOR_TREE_DIV;
	do {
		uint32 r = Random();
		TileIndex tile = RandomTileSeed(r);

		IncreaseGeneratingWorldProgress(GWP_TREE);

		if (CanPlantTreesOnTile(tile, true)) {
			PlaceTree(tile, r);
			if (_settings_game.game_creation.tree_placer != TP_IMPROVED) continue;

			/* Place a number of trees based on the tile height.
			 *  This gives a cool effect of multiple trees close together.
			 *  It is almost real life ;) */
			ht = GetTileZ(tile);
			/* The higher we get, the more trees we plant */
			j = GetTileZ(tile) * 2;
			/* Above snowline more trees! */
			if (_settings_game.game_creation.landscape == LT_ARCTIC && ht > GetSnowLine()) j *= 3;
			while (j--) {
				PlaceTreeAtSameHeight(tile, ht);
			}
		}
	} while (--i);

	/* place extra trees at rainforest area */
	if (_settings_game.game_creation.landscape == LT_TROPIC) {
		i = ScaleByMapSize(DEFAULT_RAINFOREST_TREE_STEPS);
		if (_game_mode == GM_EDITOR) i /= EDITOR_TREE_DIV;

		do {
			uint32 r = Random();
			TileIndex tile = RandomTileSeed(r);

			IncreaseGeneratingWorldProgress(GWP_TREE);

			if (GetTropicZone(tile) == TROPICZONE_RAINFOREST && CanPlantTreesOnTile(tile, false)) {
				PlaceTree(tile, r);
			}
		} while (--i);
	}
}

/**
 * Place new trees.
 *
 * This function takes care of the selected tree placer algorithm and
 * place randomly the trees for a new game.
 */
void GenerateTrees()
{
	uint i, total;

	if (_settings_game.game_creation.tree_placer == TP_NONE) return;

	switch (_settings_game.game_creation.tree_placer) {
		case TP_ORIGINAL: i = _settings_game.game_creation.landscape == LT_ARCTIC ? 15 : 6; break;
		case TP_IMPROVED: i = _settings_game.game_creation.landscape == LT_ARCTIC ?  4 : 2; break;
		default: NOT_REACHED();
	}

	total = ScaleByMapSize(DEFAULT_TREE_STEPS);
	if (_settings_game.game_creation.landscape == LT_TROPIC) total += ScaleByMapSize(DEFAULT_RAINFOREST_TREE_STEPS);
	total *= i;
	uint num_groups = (_settings_game.game_creation.landscape != LT_TOYLAND) ? ScaleByMapSize(GB(Random(), 0, 5) + 25) : 0;
	total += num_groups * DEFAULT_TREE_STEPS;
	SetGeneratingWorldProgress(GWP_TREE, total);

	if (num_groups != 0) PlaceTreeGroups(num_groups);

	for (; i != 0; i--) {
		PlaceTreesRandomly();
	}
}

/**
 * Plant a tree.
 * @param tile start tile of area-drag of tree plantation
 * @param flags type of operation
 * @param p1 tree type, TREE_INVALID means random.
 * @param p2 end tile of area-drag
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdPlantTree(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	StringID msg = INVALID_STRING_ID;
	CommandCost cost(EXPENSES_OTHER);
	const byte tree_to_plant = GB(p1, 0, 8); // We cannot use Extract as min and max are climate specific.

	if (p2 >= MapSize()) return CMD_ERROR;
	/* Check the tree type within the current climate */
	if (tree_to_plant != TREE_INVALID && !IsInsideBS(tree_to_plant, _tree_base_by_landscape[_settings_game.game_creation.landscape], _tree_count_by_landscape[_settings_game.game_creation.landscape])) return CMD_ERROR;

	Company *c = (_game_mode != GM_EDITOR) ? Company::GetIfValid(_current_company) : NULL;
	int limit = (c == NULL ? INT32_MAX : GB(c->tree_limit, 16, 16));

	TileArea ta(tile, p2);
	TILE_AREA_LOOP(tile, ta) {
		switch (GetTileType(tile)) {
			case TT_GROUND:
				if (!IsTreeTile(tile)) break;

				/* no more space for trees? */
				if (_game_mode != GM_EDITOR && GetTreeCount(tile) == 4) {
					msg = STR_ERROR_TREE_ALREADY_HERE;
					continue;
				}

				/* Test tree limit. */
				if (--limit < 1) {
					msg = STR_ERROR_TREE_PLANT_LIMIT_REACHED;
					break;
				}

				if (flags & DC_EXEC) {
					AddTreeCount(tile, 1);
					MarkTileDirtyByTile(tile);
					if (c != NULL) c->tree_limit -= 1 << 16;
				}
				/* 2x as expensive to add more trees to an existing tile */
				cost.AddCost(_price[PR_BUILD_TREES] * 2);
				continue;

			case TT_WATER:
				if (!IsCoast(tile) || IsSlopeWithOneCornerRaised(GetTileSlope(tile))) {
					msg = STR_ERROR_CAN_T_BUILD_ON_WATER;
					continue;
				}
				break;

			default:
				msg = STR_ERROR_SITE_UNSUITABLE;
				continue;
		}

		if (IsBridgeAbove(tile)) {
			msg = STR_ERROR_SITE_UNSUITABLE;
			continue;
		}

		TreeType treetype = (TreeType)tree_to_plant;
		/* Be a bit picky about which trees go where. */
		if (_settings_game.game_creation.landscape == LT_TROPIC && treetype != TREE_INVALID && (
				/* No cacti outside the desert */
				(treetype == TREE_CACTUS && GetTropicZone(tile) != TROPICZONE_DESERT) ||
				/* No rain forest trees outside the rain forest, except in the editor mode where it makes those tiles rain forest tile */
				(IsInsideMM(treetype, TREE_RAINFOREST, TREE_CACTUS) && GetTropicZone(tile) != TROPICZONE_RAINFOREST && _game_mode != GM_EDITOR) ||
				/* And no subtropical trees in the desert/rain forest */
				(IsInsideMM(treetype, TREE_SUB_TROPICAL, TREE_TOYLAND) && GetTropicZone(tile) != TROPICZONE_NORMAL))) {
			msg = STR_ERROR_TREE_WRONG_TERRAIN_FOR_TREE_TYPE;
			continue;
		}

		/* Test tree limit. */
		if (--limit < 1) {
			msg = STR_ERROR_TREE_PLANT_LIMIT_REACHED;
			break;
		}

		if (IsTileType(tile, TT_GROUND)) {
			/* Remove fields or rocks. Note that the ground will get barrened */
			if (IsTileSubtype(tile, TT_GROUND_FIELDS) || GetRawClearGround(tile) == GROUND_ROCKS) {
				CommandCost ret = DoCommand(tile, 0, 0, flags, CMD_LANDSCAPE_CLEAR);
				if (ret.Failed()) return ret;
				cost.AddCost(ret);
			}
		}

		if (_game_mode != GM_EDITOR && Company::IsValidID(_current_company)) {
			Town *t = ClosestTownFromTile(tile, _settings_game.economy.dist_local_authority);
			if (t != NULL) ChangeTownRating(t, RATING_TREE_UP_STEP, RATING_TREE_MAXIMUM, flags);
		}

		if (flags & DC_EXEC) {
			if (treetype == TREE_INVALID) {
				treetype = GetRandomTreeType(tile, GB(Random(), 24, 8));
				if (treetype == TREE_INVALID) treetype = TREE_CACTUS;
			}

			/* Plant full grown trees in scenario editor */
			PlantTreesOnTile(tile, treetype, 0, _game_mode == GM_EDITOR ? 3 : 0);
			MarkTileDirtyByTile(tile);
			if (c != NULL) c->tree_limit -= 1 << 16;

			/* When planting rainforest-trees, set tropiczone to rainforest in editor. */
			if (_game_mode == GM_EDITOR && IsInsideMM(treetype, TREE_RAINFOREST, TREE_CACTUS)) {
				SetTropicZone(tile, TROPICZONE_RAINFOREST);
			}
		}

		cost.AddCost(_price[PR_BUILD_TREES]);

		/* Tree limit used up? No need to check more. */
		if (limit < 0) break;
	}

	if (cost.GetCost() == 0) {
		return_cmd_error(msg);
	} else {
		return cost;
	}
}

void OnTick_Trees()
{
	/* Don't place trees if that's not allowed */
	if (_settings_game.construction.extra_tree_placement == ETP_NONE) return;

	uint32 r;
	TileIndex tile;
	TreeType tree;

	/* place a tree at a random rainforest spot */
	if (_settings_game.game_creation.landscape == LT_TROPIC &&
			(r = Random(), tile = RandomTileSeed(r), GetTropicZone(tile) == TROPICZONE_RAINFOREST) &&
			CanPlantTreesOnTile(tile, false) &&
			(tree = GetRandomTreeType(tile, GB(r, 24, 8))) != TREE_INVALID) {
		PlantTreesOnTile(tile, tree, 0, 0);
	}

	/* byte underflow */
	if (--_trees_tick_ctr != 0 || _settings_game.construction.extra_tree_placement != ETP_ALL) return;

	/* place a tree at a random spot */
	r = Random();
	tile = RandomTileSeed(r);
	if (CanPlantTreesOnTile(tile, false) && (tree = GetRandomTreeType(tile, GB(r, 24, 8))) != TREE_INVALID) {
		PlantTreesOnTile(tile, tree, 0, 0);
	}
}

void InitializeTrees()
{
	_trees_tick_ctr = 0;
}
