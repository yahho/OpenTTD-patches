/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform_gui.cpp GUI related to terraforming the map. */

#include "stdafx.h"
#include "map/ground.h"
#include "company_func.h"
#include "company_base.h"
#include "gui.h"
#include "window_gui.h"
#include "window_func.h"
#include "viewport_func.h"
#include "command_func.h"
#include "signs_func.h"
#include "sound_func.h"
#include "base_station_base.h"
#include "textbuf_gui.h"
#include "genworld.h"
#include "landscape_type.h"
#include "tilehighlight_func.h"
#include "strings_func.h"
#include "newgrf_object.h"
#include "object.h"
#include "hotkeys.h"
#include "engine_base.h"
#include "terraform_gui.h"
#include "zoom_func.h"

#include "widgets/terraform_widget.h"

#include "table/strings.h"

void CcTerraform(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Succeeded()) {
		if (_settings_client.sound.confirm) SndPlayTileFx(SND_1F_SPLAT_OTHER, tile);
	} else {
		extern TileIndex _terraform_err_tile;
		SetRedErrorSquare(_terraform_err_tile);
	}
}

void CcTerraformLand(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (HasBit(p2, 31)) return;

	CcTerraform(result, tile, p1, p2);
}

StringID GetErrTerraformLand (TileIndex tile, uint32 p1, uint32 p2, const char *text)
{
	return HasBit(p2, 31) ? 0 : HasBit(p2, 0) ? STR_ERROR_CAN_T_RAISE_LAND_HERE : STR_ERROR_CAN_T_LOWER_LAND_HERE;
}

StringID GetErrLevelLand (TileIndex tile, uint32 p1, uint32 p2, const char *text)
{
	switch (GB(p2, 1, 2)) {
		case LM_LEVEL: return STR_ERROR_CAN_T_LEVEL_LAND_HERE;
		case LM_RAISE: return STR_ERROR_CAN_T_RAISE_LAND_HERE;
		case LM_LOWER: return STR_ERROR_CAN_T_LOWER_LAND_HERE;
		default: return 0;
	}
}


/** Scenario editor command that generates desert areas */
static void GenerateDesertArea(TileIndex end, TileIndex start)
{
	if (_game_mode != GM_EDITOR) return;

	_generating_world = true;

	TileArea ta(start, end);
	TILE_AREA_LOOP(tile, ta) {
		SetTropicZone(tile, (_ctrl_pressed) ? TROPICZONE_NORMAL : TROPICZONE_DESERT);
		DoCommandP(tile, 0, 0, CMD_LANDSCAPE_CLEAR);
		MarkTileDirtyByTile(tile);
	}
	_generating_world = false;
	InvalidateWindowClassesData(WC_TOWN_VIEW, 0);
}

/** Scenario editor command that generates rocky areas */
static void GenerateRockyArea(TileIndex end, TileIndex start)
{
	if (_game_mode != GM_EDITOR) return;

	bool success = false;
	TileArea ta(start, end);

	TILE_AREA_LOOP(tile, ta) {
		if (!IsGroundTile(tile)) continue;
		if (IsTreeTile(tile) && GetClearGround(tile) == GROUND_SHORE) continue;
		MakeClear(tile, GROUND_ROCKS, 3);
		MarkTileDirtyByTile(tile);
		success = true;
	}

	if (success && _settings_client.sound.confirm) SndPlayTileFx(SND_1F_SPLAT_OTHER, end);
}

/** Placing actions in the terraform window. */
enum {
	PLACE_DEMOLISH_AREA,        ///< Clear area
	PLACE_LOWER_AREA,           ///< Lower / level area
	PLACE_RAISE_AREA,           ///< Raise / level area
	PLACE_LEVEL_AREA,           ///< Level area
	PLACE_CREATE_ROCKS,         ///< Fill area with rocks
	PLACE_CREATE_DESERT,        ///< Fill area with desert
	PLACE_BUY_LAND,             ///< Buy land
	PLACE_SIGN,                 ///< Place a sign
};

/**
 * A central place to handle all X_AND_Y dragged GUI functions.
 * @param proc       Procedure related to the dragging
 * @param start_tile Begin of the dragging
 * @param end_tile   End of the dragging
 * @return Returns true if the action was found and handled, and false otherwise. This
 * allows for additional implements that are more local. For example X_Y drag
 * of convertrail which belongs in rail_gui.cpp and not terraform_gui.cpp
 */
static bool GUIPlaceProcDragXY (int proc, TileIndex start_tile, TileIndex end_tile)
{
	if (!_settings_game.construction.freeform_edges) {
		/* When end_tile is void, the error tile will not be visible to the
		 * user. This happens when terraforming at the southern border. */
		if (TileX(end_tile) == MapMaxX()) end_tile += TileDiffXY(-1, 0);
		if (TileY(end_tile) == MapMaxY()) end_tile += TileDiffXY(0, -1);
	}

	switch (proc) {
		case PLACE_DEMOLISH_AREA:
			DoCommandP(end_tile, start_tile, _ctrl_pressed ? 1 : 0, CMD_CLEAR_AREA);
			break;
		case PLACE_LOWER_AREA:
			DoCommandP(end_tile, start_tile, LM_LOWER << 1 | (_ctrl_pressed ? 1 : 0), CMD_LEVEL_LAND);
			break;
		case PLACE_RAISE_AREA:
			DoCommandP(end_tile, start_tile, LM_RAISE << 1 | (_ctrl_pressed ? 1 : 0), CMD_LEVEL_LAND);
			break;
		case PLACE_LEVEL_AREA:
			DoCommandP(end_tile, start_tile, LM_LEVEL << 1 | (_ctrl_pressed ? 1 : 0), CMD_LEVEL_LAND);
			break;
		case PLACE_CREATE_ROCKS:
			GenerateRockyArea(end_tile, start_tile);
			break;
		case PLACE_CREATE_DESERT:
			GenerateDesertArea(end_tile, start_tile);
			break;
		default:
			return false;
	}

	return true;
}

void HandleDemolishMouseUp (TileIndex start_tile, TileIndex end_tile)
{
	GUIPlaceProcDragXY (PLACE_DEMOLISH_AREA, start_tile, end_tile);
}

/** Terra form toolbar managing class. */
struct TerraformToolbarWindow : Window {
	int placing_action; ///< Currently active placing action.

	TerraformToolbarWindow (const WindowDesc *desc, WindowNumber window_number) :
		Window (desc), placing_action (-1)
	{
		/* This is needed as we like to have the tree available on OnInit. */
		this->CreateNestedTree();
		this->InitNested(window_number);
	}

	~TerraformToolbarWindow()
	{
	}

	virtual void OnInit()
	{
		/* Don't show the place object button when there are no objects to place. */
		NWidgetStacked *show_object = this->GetWidget<NWidgetStacked>(WID_TT_SHOW_PLACE_OBJECT);
		show_object->SetDisplayedPlane(ObjectClass::GetUIClassCount() != 0 ? 0 : SZSP_NONE);
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		if (widget < WID_TT_BUTTONS_START) return;

		switch (widget) {
			case WID_TT_LOWER_LAND: // Lower land button
				HandlePlacePushButton (this, WID_TT_LOWER_LAND, ANIMCURSOR_LOWERLAND, POINTER_CORNER);
				this->placing_action = PLACE_LOWER_AREA;
				break;

			case WID_TT_RAISE_LAND: // Raise land button
				HandlePlacePushButton (this, WID_TT_RAISE_LAND, ANIMCURSOR_RAISELAND, POINTER_CORNER);
				this->placing_action = PLACE_RAISE_AREA;
				break;

			case WID_TT_LEVEL_LAND: // Level land button
				HandlePlacePushButton (this, WID_TT_LEVEL_LAND, SPR_CURSOR_LEVEL_LAND, POINTER_CORNER);
				this->placing_action = PLACE_LEVEL_AREA;
				break;

			case WID_TT_DEMOLISH: // Demolish aka dynamite button
				HandlePlacePushButton (this, WID_TT_DEMOLISH, ANIMCURSOR_DEMOLISH, POINTER_TILE);
				this->placing_action = PLACE_DEMOLISH_AREA;
				break;

			case WID_TT_BUY_LAND: // Buy land button
				HandlePlacePushButton (this, WID_TT_BUY_LAND, SPR_CURSOR_BUY_LAND, POINTER_TILE);
				this->placing_action = PLACE_BUY_LAND;
				break;

			case WID_TT_PLANT_TREES: // Plant trees button
				ShowBuildTreesToolbar();
				break;

			case WID_TT_PLACE_SIGN: // Place sign button
				HandlePlacePushButton (this, WID_TT_PLACE_SIGN, SPR_CURSOR_SIGN, POINTER_TILE);
				this->placing_action = PLACE_SIGN;
				break;

			case WID_TT_PLACE_OBJECT: // Place object button
				ShowBuildObjectPicker();
				break;

			default: NOT_REACHED();
		}
	}

	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		switch (this->placing_action) {
			default:
				VpStartPlaceSizing (tile, VPM_X_AND_Y_ROTATED, this->placing_action);
				break;

			case PLACE_BUY_LAND:
				DoCommandP(tile, OBJECT_OWNED_LAND, 0, CMD_BUILD_OBJECT);
				break;

			case PLACE_SIGN:
				PlaceProc_Sign(tile);
				break;
		}
	}

	virtual Point OnInitialPosition(int16 sm_width, int16 sm_height, int window_number)
	{
		Point pt = GetToolbarAlignedWindowPosition(sm_width);
		pt.y += sm_height;
		return pt;
	}

	void OnPlaceMouseUp (int userdata, TileIndex start_tile, TileIndex end_tile) OVERRIDE
	{
		switch (userdata) {
			default: NOT_REACHED();
			case PLACE_DEMOLISH_AREA:
			case PLACE_LOWER_AREA:
			case PLACE_RAISE_AREA:
			case PLACE_LEVEL_AREA:
				GUIPlaceProcDragXY (userdata, start_tile, end_tile);
				break;
		}
	}

	virtual void OnPlaceObjectAbort()
	{
		this->RaiseButtons();
	}

	static HotkeyList hotkeys;
};

/**
 * Handler for global hotkeys of the TerraformToolbarWindow.
 * @param hotkey Hotkey
 * @return Whether the hotkey was handled.
 */
static bool TerraformToolbarGlobalHotkeys (int hotkey)
{
	if (_game_mode != GM_NORMAL) return false;
	Window *w = ShowTerraformToolbar(NULL);
	return (w != NULL) && w->OnHotkey (hotkey);
}

static const Hotkey terraform_hotkeys[] = {
	Hotkey ("lower",       WID_TT_LOWER_LAND,   'Q' | WKC_GLOBAL_HOTKEY),
	Hotkey ("raise",       WID_TT_RAISE_LAND,   'W' | WKC_GLOBAL_HOTKEY),
	Hotkey ("level",       WID_TT_LEVEL_LAND,   'E' | WKC_GLOBAL_HOTKEY),
	Hotkey ("dynamite",    WID_TT_DEMOLISH,     'D' | WKC_GLOBAL_HOTKEY),
	Hotkey ("buyland",     WID_TT_BUY_LAND,     'U'),
	Hotkey ("trees",       WID_TT_PLANT_TREES,  'I'),
	Hotkey ("placesign",   WID_TT_PLACE_SIGN,   'O'),
	Hotkey ("placeobject", WID_TT_PLACE_OBJECT, 'P'),
};
HotkeyList TerraformToolbarWindow::hotkeys("terraform", terraform_hotkeys, TerraformToolbarGlobalHotkeys);

static const NWidgetPart _nested_terraform_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_LANDSCAPING_TOOLBAR, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_LOWER_LAND), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_TERRAFORM_DOWN, STR_LANDSCAPING_TOOLTIP_LOWER_A_CORNER_OF_LAND),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_RAISE_LAND), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_TERRAFORM_UP, STR_LANDSCAPING_TOOLTIP_RAISE_A_CORNER_OF_LAND),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_LEVEL_LAND), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_LEVEL_LAND, STR_LANDSCAPING_LEVEL_LAND_TOOLTIP),

		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(4, 22), EndContainer(),

		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_DEMOLISH), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_BUY_LAND), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_BUY_LAND, STR_LANDSCAPING_TOOLTIP_PURCHASE_LAND),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_TT_PLANT_TREES), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_PLANTTREES, STR_SCENEDIT_TOOLBAR_PLANT_TREES),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_TT_PLACE_SIGN), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_SIGN, STR_SCENEDIT_TOOLBAR_PLACE_SIGN),
		NWidget(NWID_SELECTION, INVALID_COLOUR, WID_TT_SHOW_PLACE_OBJECT),
			NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_TT_PLACE_OBJECT), SetMinimalSize(22, 22),
								SetFill(0, 1), SetDataTip(SPR_IMG_TRANSMITTER, STR_SCENEDIT_TOOLBAR_PLACE_OBJECT),
		EndContainer(),
	EndContainer(),
};

static WindowDesc::Prefs _terraform_prefs ("toolbar_landscape");

static const WindowDesc _terraform_desc(
	WDP_MANUAL, 0, 0,
	WC_SCEN_LAND_GEN, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_terraform_widgets, lengthof(_nested_terraform_widgets),
	&_terraform_prefs, &TerraformToolbarWindow::hotkeys
);

/**
 * Show the toolbar for terraforming in the game.
 * @param link The toolbar we might want to link to.
 * @return The allocated toolbar if the window was newly opened, else \c NULL.
 */
Window *ShowTerraformToolbar(Window *link)
{
	if (!Company::IsValidID(_local_company)) return NULL;

	Window *w;
	if (link == NULL) {
		w = AllocateWindowDescFront<TerraformToolbarWindow>(&_terraform_desc, 0);
		return w;
	}

	/* Delete the terraform toolbar to place it again. */
	DeleteWindowById(WC_SCEN_LAND_GEN, 0, true);
	w = AllocateWindowDescFront<TerraformToolbarWindow>(&_terraform_desc, 0);
	/* Align the terraform toolbar under the main toolbar. */
	w->top -= w->height;
	w->SetDirty();
	/* Put the linked toolbar to the left / right of it. */
	link->left = w->left + (_current_text_dir == TD_RTL ? w->width : -link->width);
	link->top  = w->top;
	link->SetDirty();

	return w;
}

static byte _terraform_size = 1;

/**
 * Raise/Lower a bigger chunk of land at the same time in the editor. When
 * raising get the lowest point, when lowering the highest point, and set all
 * tiles in the selection to that height.
 * @todo : Incorporate into game itself to allow for ingame raising/lowering of
 *         larger chunks at the same time OR remove altogether, as we have 'level land' ?
 * @param tile The top-left tile where the terraforming will start
 * @param mode true for raising, false for lowering land
 */
static void CommonRaiseLowerBigLand (TileIndex tile, bool mode)
{
	assert (_terraform_size != 0);
	assert (_terraform_size != 1);

	TileArea ta (tile, _terraform_size, _terraform_size);
	ta.ClampToMap();

	if (ta.w == 0 || ta.h == 0) return;

	if (_settings_client.sound.confirm) SndPlayTileFx (SND_1F_SPLAT_OTHER, tile);

	uint h;
	if (mode) {
		/* Raise land */
		h = MAX_TILE_HEIGHT;
		TILE_AREA_LOOP(tile2, ta) {
			h = min (h, TileHeight(tile2));
		}
	} else {
		/* Lower land */
		h = 0;
		TILE_AREA_LOOP(tile2, ta) {
			h = max (h, TileHeight(tile2));
		}
	}

	TILE_AREA_LOOP(tile2, ta) {
		if (TileHeight(tile2) == h) {
			DoCommandP (tile2, SLOPE_N, (mode ? 1 : 0) | (1 << 31), CMD_TERRAFORM_LAND);
		}
	}
}

static const int8 _multi_terraform_coords[][2] = {
	{  0, -2},
	{  4,  0}, { -4,  0}, {  0,  2},
	{ -8,  2}, { -4,  4}, {  0,  6}, {  4,  4}, {  8,  2},
	{-12,  0}, { -8, -2}, { -4, -4}, {  0, -6}, {  4, -4}, {  8, -2}, { 12,  0},
	{-16,  2}, {-12,  4}, { -8,  6}, { -4,  8}, {  0, 10}, {  4,  8}, {  8,  6}, { 12,  4}, { 16,  2},
	{-20,  0}, {-16, -2}, {-12, -4}, { -8, -6}, { -4, -8}, {  0,-10}, {  4, -8}, {  8, -6}, { 12, -4}, { 16, -2}, { 20,  0},
	{-24,  2}, {-20,  4}, {-16,  6}, {-12,  8}, { -8, 10}, { -4, 12}, {  0, 14}, {  4, 12}, {  8, 10}, { 12,  8}, { 16,  6}, { 20,  4}, { 24,  2},
	{-28,  0}, {-24, -2}, {-20, -4}, {-16, -6}, {-12, -8}, { -8,-10}, { -4,-12}, {  0,-14}, {  4,-12}, {  8,-10}, { 12, -8}, { 16, -6}, { 20, -4}, { 24, -2}, { 28,  0},
};

static const NWidgetPart _nested_scen_edit_land_gen_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_TERRAFORM_TOOLBAR_LAND_GENERATION_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		NWidget(NWID_HORIZONTAL), SetPadding(2, 2, 7, 2),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_DEMOLISH), SetMinimalSize(22, 22),
										SetFill(0, 1), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_LOWER_LAND), SetMinimalSize(22, 22),
										SetFill(0, 1), SetDataTip(SPR_IMG_TERRAFORM_DOWN, STR_LANDSCAPING_TOOLTIP_LOWER_A_CORNER_OF_LAND),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_RAISE_LAND), SetMinimalSize(22, 22),
										SetFill(0, 1), SetDataTip(SPR_IMG_TERRAFORM_UP, STR_LANDSCAPING_TOOLTIP_RAISE_A_CORNER_OF_LAND),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_LEVEL_LAND), SetMinimalSize(22, 22),
										SetFill(0, 1), SetDataTip(SPR_IMG_LEVEL_LAND, STR_LANDSCAPING_LEVEL_LAND_TOOLTIP),
			NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_PLACE_ROCKS), SetMinimalSize(22, 22),
										SetFill(0, 1), SetDataTip(SPR_IMG_ROCKS, STR_TERRAFORM_TOOLTIP_PLACE_ROCKY_AREAS_ON_LANDSCAPE),
			NWidget(NWID_SELECTION, INVALID_COLOUR, WID_ETT_SHOW_PLACE_DESERT),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_PLACE_DESERT), SetMinimalSize(22, 22),
											SetFill(0, 1), SetDataTip(SPR_IMG_DESERT, STR_TERRAFORM_TOOLTIP_DEFINE_DESERT_AREA),
			EndContainer(),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_ETT_PLACE_OBJECT), SetMinimalSize(23, 22),
										SetFill(0, 1), SetDataTip(SPR_IMG_TRANSMITTER, STR_SCENEDIT_TOOLBAR_PLACE_OBJECT),
			NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_ETT_PLACE_HOUSE), SetMinimalSize(23, 22),
										SetFill(0, 1), SetDataTip(SPR_IMG_TOWN, STR_SCENEDIT_TOOLBAR_PLACE_HOUSE),
			NWidget(NWID_SPACER), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_HORIZONTAL),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(WWT_EMPTY, COLOUR_DARK_GREEN, WID_ETT_DOTS), SetMinimalSize(59, 31), SetDataTip(STR_EMPTY, STR_NULL),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(NWID_VERTICAL),
				NWidget(NWID_SPACER), SetFill(0, 1),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_INCREASE_SIZE), SetMinimalSize(12, 12), SetDataTip(SPR_ARROW_UP, STR_TERRAFORM_TOOLTIP_INCREASE_SIZE_OF_LAND_AREA),
				NWidget(NWID_SPACER), SetMinimalSize(0, 1),
				NWidget(WWT_IMGBTN, COLOUR_GREY, WID_ETT_DECREASE_SIZE), SetMinimalSize(12, 12), SetDataTip(SPR_ARROW_DOWN, STR_TERRAFORM_TOOLTIP_DECREASE_SIZE_OF_LAND_AREA),
				NWidget(NWID_SPACER), SetFill(0, 1),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(2, 0),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 6),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_ETT_NEW_SCENARIO), SetMinimalSize(160, 12),
								SetFill(1, 0), SetDataTip(STR_TERRAFORM_SE_NEW_WORLD, STR_TERRAFORM_TOOLTIP_GENERATE_RANDOM_LAND), SetPadding(0, 2, 0, 2),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_ETT_RESET_LANDSCAPE), SetMinimalSize(160, 12),
								SetFill(1, 0), SetDataTip(STR_TERRAFORM_RESET_LANDSCAPE, STR_TERRAFORM_RESET_LANDSCAPE_TOOLTIP), SetPadding(1, 2, 2, 2),
	EndContainer(),
};

/**
 * Callback function for the scenario editor 'reset landscape' confirmation window
 * @param w Window unused
 * @param confirmed boolean value, true when yes was clicked, false otherwise
 */
static void ResetLandscapeConfirmationCallback(Window *w, bool confirmed)
{
	if (confirmed) {
		/* Set generating_world to true to get instant-green grass after removing
		 * company property. */
		_generating_world = true;

		/* Delete all companies */
		Company *c;
		FOR_ALL_COMPANIES(c) {
			ChangeOwnershipOfCompanyItems(c->index, INVALID_OWNER);
			delete c;
		}

		_generating_world = false;

		/* Delete all station signs */
		BaseStation *st;
		FOR_ALL_BASE_STATIONS(st) {
			/* There can be buoys, remove them */
			if (IsBuoyTile(st->xy)) DoCommand(st->xy, 0, 0, DC_EXEC | DC_BANKRUPT, CMD_LANDSCAPE_CLEAR);
			if (!st->IsInUse()) delete st;
		}

		/* Now that all vehicles are gone, we can reset the engine pool. Maybe it reduces some NewGRF changing-mess */
		EngineOverrideManager::ResetToCurrentNewGRFConfig();

		MarkWholeScreenDirty();
	}
}

/** Landscape generation window handler in the scenario editor. */
struct ScenarioEditorLandscapeGenerationWindow : Window {
	int placing_action; ///< Currently active placing action.

	ScenarioEditorLandscapeGenerationWindow (const WindowDesc *desc, WindowNumber window_number) :
		Window (desc), placing_action (-1)
	{
		this->CreateNestedTree();
		NWidgetStacked *show_desert = this->GetWidget<NWidgetStacked>(WID_ETT_SHOW_PLACE_DESERT);
		show_desert->SetDisplayedPlane(_settings_game.game_creation.landscape == LT_TROPIC ? 0 : SZSP_NONE);
		this->InitNested(window_number);
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		this->DrawWidgets (dpi);

		if (this->IsWidgetLowered(WID_ETT_LOWER_LAND) || this->IsWidgetLowered(WID_ETT_RAISE_LAND)) { // change area-size if raise/lower corner is selected
			SetTileSelectSize(_terraform_size, _terraform_size);
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		if (widget != WID_ETT_DOTS) return;

		size->width  = max<uint>(size->width,  ScaleGUITrad(59));
		size->height = max<uint>(size->height, ScaleGUITrad(31));
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		if (widget != WID_ETT_DOTS) return;

		int center_x = RoundDivSU(r.left + r.right, 2);
		int center_y = RoundDivSU(r.top + r.bottom, 2);

		int n = _terraform_size * _terraform_size;
		const int8 *coords = &_multi_terraform_coords[0][0];

		assert(n != 0);
		do {
			DrawSprite (dpi, SPR_WHITE_POINT, PAL_NONE, center_x + ScaleGUITrad(coords[0]), center_y + ScaleGUITrad(coords[1]));
			coords += 2;
		} while (--n);
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		if (widget < WID_ETT_BUTTONS_START) return;

		switch (widget) {
			case WID_ETT_DEMOLISH: // Demolish aka dynamite button
				HandlePlacePushButton (this, WID_ETT_DEMOLISH, ANIMCURSOR_DEMOLISH, POINTER_TILE);
				this->placing_action = PLACE_DEMOLISH_AREA;
				break;

			case WID_ETT_LOWER_LAND: // Lower land button
				HandlePlacePushButton (this, WID_ETT_LOWER_LAND, ANIMCURSOR_LOWERLAND, POINTER_CORNER);
				this->placing_action = PLACE_LOWER_AREA;
				break;

			case WID_ETT_RAISE_LAND: // Raise land button
				HandlePlacePushButton (this, WID_ETT_RAISE_LAND, ANIMCURSOR_RAISELAND, POINTER_CORNER);
				this->placing_action = PLACE_RAISE_AREA;
				break;

			case WID_ETT_LEVEL_LAND: // Level land button
				HandlePlacePushButton (this, WID_ETT_LEVEL_LAND, SPR_CURSOR_LEVEL_LAND, POINTER_CORNER);
				this->placing_action = PLACE_LEVEL_AREA;
				break;

			case WID_ETT_PLACE_ROCKS: // Place rocks button
				HandlePlacePushButton (this, WID_ETT_PLACE_ROCKS, SPR_CURSOR_ROCKY_AREA, POINTER_TILE);
				this->placing_action = PLACE_CREATE_ROCKS;
				break;

			case WID_ETT_PLACE_DESERT: // Place desert button (in tropical climate)
				HandlePlacePushButton (this, WID_ETT_PLACE_DESERT, SPR_CURSOR_DESERT, POINTER_TILE);
				this->placing_action = PLACE_CREATE_DESERT;
				break;

			case WID_ETT_PLACE_OBJECT: // Place transmitter button
				ShowBuildObjectPicker();
				break;

			case WID_ETT_PLACE_HOUSE: // Place house button
				ShowBuildHousePicker();
				break;

			case WID_ETT_INCREASE_SIZE:
			case WID_ETT_DECREASE_SIZE: { // Increase/Decrease terraform size
				int size = (widget == WID_ETT_INCREASE_SIZE) ? 1 : -1;
				this->HandleButtonClick(widget);
				size += _terraform_size;

				if (!IsInsideMM(size, 1, 8 + 1)) return;
				_terraform_size = size;

				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				this->SetDirty();
				break;
			}

			case WID_ETT_NEW_SCENARIO: // gen random land
				this->HandleButtonClick(widget);
				ShowCreateScenario();
				break;

			case WID_ETT_RESET_LANDSCAPE: // Reset landscape
				ShowQuery(STR_QUERY_RESET_LANDSCAPE_CAPTION, STR_RESET_LANDSCAPE_CONFIRMATION_TEXT, NULL, ResetLandscapeConfirmationCallback);
				break;

			default: NOT_REACHED();
		}
	}

	virtual void OnTimeout()
	{
		for (uint i = WID_ETT_START; i < this->nested_array_size; i++) {
			if (this->IsWidgetLowered(i)) {
				this->RaiseWidget(i);
				this->SetWidgetDirty(i);
			}
		}
	}

	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		switch (this->placing_action) {
			case PLACE_LOWER_AREA:
			case PLACE_RAISE_AREA:
				if (_terraform_size != 1) {
					CommonRaiseLowerBigLand (tile, this->placing_action == PLACE_RAISE_AREA);
					break;
				}
				/* fall through */
			default:
				VpStartPlaceSizing (tile, VPM_X_AND_Y_ROTATED, this->placing_action);
				break;

			case PLACE_CREATE_ROCKS:
			case PLACE_CREATE_DESERT:
				VpStartPlaceSizing (tile, VPM_X_AND_Y, this->placing_action);
				break;
		}
	}

	void OnPlaceMouseUp (int userdata, TileIndex start_tile, TileIndex end_tile) OVERRIDE
	{
		switch (userdata) {
			default: NOT_REACHED();
			case PLACE_DEMOLISH_AREA:
			case PLACE_LOWER_AREA:
			case PLACE_RAISE_AREA:
			case PLACE_LEVEL_AREA:
			case PLACE_CREATE_ROCKS:
			case PLACE_CREATE_DESERT:
				GUIPlaceProcDragXY (userdata, start_tile, end_tile);
				break;
		}
	}

	virtual void OnPlaceObjectAbort()
	{
		this->RaiseButtons();
		this->SetDirty();
	}

	static HotkeyList hotkeys;
};

/**
 * Handler for global hotkeys of the ScenarioEditorLandscapeGenerationWindow.
 * @param hotkey Hotkey
 * @return Whether the hotkey was handled.
 */
static bool TerraformToolbarEditorGlobalHotkeys (int hotkey)
{
	if (_game_mode != GM_EDITOR) return false;
	Window *w = ShowEditorTerraformToolbar();
	return (w != NULL) && w->OnHotkey (hotkey);
}

static const Hotkey terraform_editor_hotkeys[] = {
	Hotkey ("dynamite", WID_ETT_DEMOLISH,     'D' | WKC_GLOBAL_HOTKEY),
	Hotkey ("lower",    WID_ETT_LOWER_LAND,   'Q' | WKC_GLOBAL_HOTKEY),
	Hotkey ("raise",    WID_ETT_RAISE_LAND,   'W' | WKC_GLOBAL_HOTKEY),
	Hotkey ("level",    WID_ETT_LEVEL_LAND,   'E' | WKC_GLOBAL_HOTKEY),
	Hotkey ("rocky",    WID_ETT_PLACE_ROCKS,  'R'),
	Hotkey ("desert",   WID_ETT_PLACE_DESERT, 'T'),
	Hotkey ("object",   WID_ETT_PLACE_OBJECT, 'O'),
	Hotkey ("house",    WID_ETT_PLACE_HOUSE,  'H'),
};

HotkeyList ScenarioEditorLandscapeGenerationWindow::hotkeys("terraform_editor", terraform_editor_hotkeys, TerraformToolbarEditorGlobalHotkeys);

static WindowDesc::Prefs _scen_edit_land_gen_prefs ("toolbar_landscape_scen");

static const WindowDesc _scen_edit_land_gen_desc(
	WDP_AUTO, 0, 0,
	WC_SCEN_LAND_GEN, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_scen_edit_land_gen_widgets, lengthof(_nested_scen_edit_land_gen_widgets),
	&_scen_edit_land_gen_prefs, &ScenarioEditorLandscapeGenerationWindow::hotkeys
);

/**
 * Show the toolbar for terraforming in the scenario editor.
 * @return The allocated toolbar if the window was newly opened, else \c NULL.
 */
Window *ShowEditorTerraformToolbar()
{
	return AllocateWindowDescFront<ScenarioEditorLandscapeGenerationWindow>(&_scen_edit_land_gen_desc, 0);
}
