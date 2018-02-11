/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file dock_gui.cpp GUI to create amazing water objects. */

#include "stdafx.h"
#include "map/slope.h"
#include "terraform_gui.h"
#include "window_gui.h"
#include "station_gui.h"
#include "command_func.h"
#include "water.h"
#include "window_func.h"
#include "vehicle_func.h"
#include "sound_func.h"
#include "viewport_func.h"
#include "gfx_func.h"
#include "company_func.h"
#include "slope_func.h"
#include "tilehighlight_func.h"
#include "company_base.h"
#include "hotkeys.h"
#include "gui.h"
#include "zoom_func.h"

#include "widgets/dock_widget.h"

#include "table/sprites.h"
#include "table/strings.h"

static void ShowBuildDockStationPicker(Window *parent);
static void ShowBuildDocksDepotPicker(Window *parent);

static Axis _ship_depot_direction;

void CcBuildDocks(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Failed()) return;

	if (_settings_client.sound.confirm) SndPlayTileFx(SND_02_SPLAT_WATER, tile);
	if (!_settings_client.gui.persistent_buildingtools) ResetPointerMode();
}

void CcPlaySound_SPLAT_WATER(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Succeeded() && _settings_client.sound.confirm) SndPlayTileFx(SND_02_SPLAT_WATER, tile);
}

StringID GetErrBuildCanal (TileIndex tile, uint32 p1, uint32 p2, const char *text)
{
	return p2 == WATER_CLASS_RIVER ? STR_ERROR_CAN_T_PLACE_RIVERS : STR_ERROR_CAN_T_BUILD_CANALS;
}


/**
 * Gets the other end of the aqueduct, if possible.
 * @param tile_from     The begin tile for the aqueduct.
 * @param [out] tile_to The tile till where to show a selection for the aqueduct.
 * @return The other end of the aqueduct, or otherwise a tile in line with the aqueduct to cause the right error message.
 */
static TileIndex GetOtherAqueductEnd(TileIndex tile_from, TileIndex *tile_to = NULL)
{
	int z;
	DiagDirection dir = GetInclinedSlopeDirection(GetTileSlope(tile_from, &z));

	/* If the direction isn't right, just return the next tile so the command
	 * complains about the wrong slope instead of the ends not matching up.
	 * Make sure the coordinate is always a valid tile within the map, so we
	 * don't go "off" the map. That would cause the wrong error message. */
	if (!IsValidDiagDirection(dir)) return TILE_ADDXY(tile_from, TileX(tile_from) > 2 ? -1 : 1, 0);

	/* Direction the aqueduct is built to. */
	TileIndexDiff offset = TileOffsByDiagDir(ReverseDiagDir(dir));
	/* The maximum length of the aqueduct. */
	int max_length = min(_settings_game.construction.max_bridge_length, DistanceFromEdgeDir(tile_from, ReverseDiagDir(dir)) - 1);

	TileIndex endtile = tile_from;
	for (int length = 0; IsValidTile(endtile) && TileX(endtile) != 0 && TileY(endtile) != 0; length++) {
		endtile = TILE_ADD(endtile, offset);

		if (length > max_length) break;

		if (GetTileMaxZ(endtile) > z) {
			if (tile_to != NULL) *tile_to = endtile;
			break;
		}
	}

	return endtile;
}

/** Toolbar window for constructing water infrastructure. */
struct BuildDocksToolbarWindow : Window {
	/** Dragging actions for this window. */
	enum {
		DRAG_DEMOLISH_AREA,     ///< Clear area
		DRAG_CREATE_WATER,      ///< Create a canal
		DRAG_CREATE_RIVER,      ///< Create rivers
	};

	DockToolbarWidgets last_clicked_widget; ///< Contains the last widget that has been clicked on this toolbar.

	BuildDocksToolbarWindow (const WindowDesc *desc, WindowNumber window_number)
		: Window (desc), last_clicked_widget (WID_DT_INVALID)
	{
		this->InitNested(window_number);
		this->OnInvalidateData();
		if (_settings_client.gui.link_terraform_toolbar) ShowTerraformToolbar(this);
	}

	void OnDelete (void) FINAL_OVERRIDE
	{
		if (_settings_client.gui.link_terraform_toolbar) DeleteWindowById(WC_SCEN_LAND_GEN, 0, false);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;

		bool can_build = CanBuildVehicleInfrastructure(VEH_SHIP);
		this->SetWidgetsDisabledState(!can_build,
			WID_DT_DEPOT,
			WID_DT_STATION,
			WID_DT_BUOY,
			WIDGET_LIST_END);
		if (!can_build) {
			DeleteWindowById(WC_BUILD_STATION, TRANSPORT_WATER);
			DeleteWindowById(WC_BUILD_DEPOT, TRANSPORT_WATER);
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_DT_CANAL: // Build canal button
				HandlePlacePushButton (this, WID_DT_CANAL, SPR_CURSOR_CANAL, POINTER_TILE);
				break;

			case WID_DT_LOCK: // Build lock button
				HandlePlacePushButton (this, WID_DT_LOCK, SPR_CURSOR_LOCK, POINTER_AREA);
				break;

			case WID_DT_DEMOLISH: // Demolish aka dynamite button
				HandlePlacePushButton (this, WID_DT_DEMOLISH, ANIMCURSOR_DEMOLISH, POINTER_TILE);
				break;

			case WID_DT_DEPOT: // Build depot button
				if (!CanBuildVehicleInfrastructure(VEH_SHIP)) return;
				if (HandlePlacePushButton (this, WID_DT_DEPOT, SPR_CURSOR_SHIP_DEPOT, POINTER_TILE)) ShowBuildDocksDepotPicker(this);
				break;

			case WID_DT_STATION: // Build station button
				if (!CanBuildVehicleInfrastructure(VEH_SHIP)) return;
				if (HandlePlacePushButton (this, WID_DT_STATION, SPR_CURSOR_DOCK, POINTER_AREA)) ShowBuildDockStationPicker(this);
				break;

			case WID_DT_BUOY: // Build buoy button
				if (!CanBuildVehicleInfrastructure(VEH_SHIP)) return;
				HandlePlacePushButton (this, WID_DT_BUOY, SPR_CURSOR_BUOY, POINTER_TILE);
				break;

			case WID_DT_RIVER: // Build river button (in scenario editor)
				if (_game_mode != GM_EDITOR) return;
				HandlePlacePushButton (this, WID_DT_RIVER, SPR_CURSOR_RIVER, POINTER_TILE);
				break;

			case WID_DT_BUILD_AQUEDUCT: // Build aqueduct button
				HandlePlacePushButton (this, WID_DT_BUILD_AQUEDUCT, SPR_CURSOR_AQUEDUCT, POINTER_AREA);
				break;

			default: return;
		}
		this->last_clicked_widget = (DockToolbarWidgets)widget;
	}

	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		switch (this->last_clicked_widget) {
			case WID_DT_CANAL: // Build canal button
				VpStartPlaceSizing(tile, (_game_mode == GM_EDITOR) ? VPM_X_AND_Y : VPM_X_OR_Y, DRAG_CREATE_WATER);
				break;

			case WID_DT_LOCK: // Build lock button
				DoCommandP(tile, 0, 0, CMD_BUILD_LOCK);
				break;

			case WID_DT_DEMOLISH: // Demolish aka dynamite button
				VpStartPlaceSizing (tile, VPM_X_AND_Y_ROTATED, DRAG_DEMOLISH_AREA);
				break;

			case WID_DT_DEPOT: // Build depot button
				DoCommandP(tile, _ship_depot_direction, 0, CMD_BUILD_SHIP_DEPOT);
				break;

			case WID_DT_STATION: { // Build station button
				uint32 p2 = (uint32)INVALID_STATION << 16; // no station to join

				/* tile is always the land tile, so need to evaluate _thd.pos */
				Command cmdcont (tile, _ctrl_pressed, p2, CMD_BUILD_DOCK);

				/* Determine the watery part of the dock. */
				DiagDirection dir = GetInclinedSlopeDirection(GetTileSlope(tile));
				TileIndex tile_to = (dir != INVALID_DIAGDIR ? TileAddByDiagDir(tile, ReverseDiagDir(dir)) : tile);

				ShowSelectStationIfNeeded (&cmdcont, TileArea(tile, tile_to));
				break;
			}

			case WID_DT_BUOY: // Build buoy button
				DoCommandP(tile, 0, 0, CMD_BUILD_BUOY);
				break;

			case WID_DT_RIVER: // Build river button (in scenario editor)
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DRAG_CREATE_RIVER);
				break;

			case WID_DT_BUILD_AQUEDUCT: // Build aqueduct button
				DoCommandP(tile, GetOtherAqueductEnd(tile), TRANSPORT_WATER << 12, CMD_BUILD_BRIDGE);
				break;

			default: NOT_REACHED();
		}
	}

	void OnPlaceMouseUp (int userdata, Point pt, TileIndex start_tile, TileIndex end_tile) OVERRIDE
	{
		if (pt.x != -1) {
			switch (userdata) {
				case DRAG_DEMOLISH_AREA:
					HandleDemolishMouseUp (start_tile, end_tile);
					break;
				case DRAG_CREATE_WATER:
					DoCommandP(end_tile, start_tile, (_game_mode == GM_EDITOR && _ctrl_pressed) ? WATER_CLASS_SEA : WATER_CLASS_CANAL, CMD_BUILD_CANAL);
					break;
				case DRAG_CREATE_RIVER:
					DoCommandP(end_tile, start_tile, WATER_CLASS_RIVER, CMD_BUILD_CANAL);
					break;

				default: break;
			}
		}
	}

	virtual void OnPlaceObjectAbort()
	{
		this->RaiseButtons();

		DeleteWindowById(WC_BUILD_STATION, TRANSPORT_WATER);
		DeleteWindowById(WC_BUILD_DEPOT, TRANSPORT_WATER);
		DeleteWindowById(WC_SELECT_STATION, 0);
		DeleteWindowByClass(WC_BUILD_BRIDGE);
	}

	void OnPlacePresize (TileIndex *tile1, TileIndex *tile2) OVERRIDE
	{
		if (this->last_clicked_widget == WID_DT_BUILD_AQUEDUCT) {
			GetOtherAqueductEnd (*tile1, tile2);
		} else {
			TileIndex tile_from = *tile1;
			DiagDirection dir = GetInclinedSlopeDirection(GetTileSlope(tile_from));
			if (IsValidDiagDirection(dir)) {
				/* Locks and docks always select the tile "down" the slope. */
				*tile2 = TileAddByDiagDir (tile_from, ReverseDiagDir(dir));
				/* Locks also select the tile "up" the slope. */
				if (this->last_clicked_widget == WID_DT_LOCK) *tile1 = TileAddByDiagDir (tile_from, dir);
			}
		}
	}

	static HotkeyList hotkeys;
};

/**
 * Handler for global hotkeys of the BuildDocksToolbarWindow.
 * @param hotkey Hotkey
 * @return ES_HANDLED if hotkey was accepted.
 */
static EventState DockToolbarGlobalHotkeys(int hotkey)
{
	if (_game_mode != GM_NORMAL) return ES_NOT_HANDLED;
	Window *w = ShowBuildDocksToolbar();
	if (w == NULL) return ES_NOT_HANDLED;
	return w->OnHotkey(hotkey) ? ES_HANDLED : ES_NOT_HANDLED;
}

static const Hotkey dockstoolbar_hotkeys[] = {
	Hotkey ("canal",    WID_DT_CANAL,    '1'),
	Hotkey ("lock",     WID_DT_LOCK,     '2'),
	Hotkey ("demolish", WID_DT_DEMOLISH, '3'),
	Hotkey ("depot",    WID_DT_DEPOT,    '4'),
	Hotkey ("dock",     WID_DT_STATION,  '5'),
	Hotkey ("buoy",     WID_DT_BUOY,     '6'),
	Hotkey ("river",    WID_DT_RIVER,    '7'),
	Hotkey ("aqueduct", WID_DT_BUILD_AQUEDUCT, '8', 'B'),
};
HotkeyList BuildDocksToolbarWindow::hotkeys("dockstoolbar", dockstoolbar_hotkeys, DockToolbarGlobalHotkeys);

/**
 * Nested widget parts of docks toolbar, game version.
 * Position of #WID_DT_RIVER widget has changed.
 */
static const NWidgetPart _nested_build_docks_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_WATERWAYS_TOOLBAR_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL_LTR),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_CANAL), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_BUILD_CANAL, STR_WATERWAYS_TOOLBAR_BUILD_CANALS_TOOLTIP),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_LOCK), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_BUILD_LOCK, STR_WATERWAYS_TOOLBAR_BUILD_LOCKS_TOOLTIP),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(5, 22), SetFill(1, 1), EndContainer(),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_DEMOLISH), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_DEPOT), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_SHIP_DEPOT, STR_WATERWAYS_TOOLBAR_BUILD_DEPOT_TOOLTIP),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_STATION), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_SHIP_DOCK, STR_WATERWAYS_TOOLBAR_BUILD_DOCK_TOOLTIP),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_BUOY), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_BUOY, STR_WATERWAYS_TOOLBAR_BUOY_TOOLTIP),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_BUILD_AQUEDUCT), SetMinimalSize(23, 22), SetFill(0, 1), SetDataTip(SPR_IMG_AQUEDUCT, STR_WATERWAYS_TOOLBAR_BUILD_AQUEDUCT_TOOLTIP),
	EndContainer(),
};

static WindowDesc::Prefs _build_docks_toolbar_prefs ("toolbar_water");

static const WindowDesc _build_docks_toolbar_desc(
	WDP_ALIGN_TOOLBAR, 0, 0,
	WC_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_build_docks_toolbar_widgets, lengthof(_nested_build_docks_toolbar_widgets),
	&_build_docks_toolbar_prefs, &BuildDocksToolbarWindow::hotkeys
);

/**
 * Open the build water toolbar window
 *
 * If the terraform toolbar is linked to the toolbar, that window is also opened.
 *
 * @return newly opened water toolbar, or NULL if the toolbar could not be opened.
 */
Window *ShowBuildDocksToolbar()
{
	if (!Company::IsValidID(_local_company)) return NULL;

	DeleteWindowByClass(WC_BUILD_TOOLBAR);
	return AllocateWindowDescFront<BuildDocksToolbarWindow>(&_build_docks_toolbar_desc, TRANSPORT_WATER);
}

/**
 * Nested widget parts of docks toolbar, scenario editor version.
 * Positions of #WID_DT_DEPOT, #WID_DT_STATION, and #WID_DT_BUOY widgets have changed.
 */
static const NWidgetPart _nested_build_docks_scen_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_WATERWAYS_TOOLBAR_CAPTION_SE, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_CANAL), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_BUILD_CANAL, STR_WATERWAYS_TOOLBAR_CREATE_LAKE_TOOLTIP),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_LOCK), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_BUILD_LOCK, STR_WATERWAYS_TOOLBAR_BUILD_LOCKS_TOOLTIP),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(5, 22), SetFill(1, 1), EndContainer(),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_DEMOLISH), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_DYNAMITE, STR_TOOLTIP_DEMOLISH_BUILDINGS_ETC),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_RIVER), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_BUILD_RIVER, STR_WATERWAYS_TOOLBAR_CREATE_RIVER_TOOLTIP),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_DT_BUILD_AQUEDUCT), SetMinimalSize(22, 22), SetFill(0, 1), SetDataTip(SPR_IMG_AQUEDUCT, STR_WATERWAYS_TOOLBAR_BUILD_AQUEDUCT_TOOLTIP),
	EndContainer(),
};

/** Window preferences for the build docks in scenario editor window. */
static WindowDesc::Prefs _build_docks_scen_toolbar_prefs ("toolbar_water_scen");

/** Window definition for the build docks in scenario editor window. */
static const WindowDesc _build_docks_scen_toolbar_desc(
	WDP_AUTO, 0, 0,
	WC_SCEN_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_build_docks_scen_toolbar_widgets, lengthof(_nested_build_docks_scen_toolbar_widgets),
	&_build_docks_scen_toolbar_prefs
);

/**
 * Open the build water toolbar window for the scenario editor.
 *
 * @return newly opened water toolbar, or NULL if the toolbar could not be opened.
 */
Window *ShowBuildDocksScenToolbar()
{
	return AllocateWindowDescFront<BuildDocksToolbarWindow>(&_build_docks_scen_toolbar_desc, TRANSPORT_WATER);
}

/** Widget numbers of the build-dock GUI. */
enum BuildDockStationWidgets {
	BDSW_BACKGROUND, ///< Background panel.
	BDSW_LT_OFF,     ///< 'Off' button of coverage high light.
	BDSW_LT_ON,      ///< 'On' button of coverage high light.
	BDSW_INFO,       ///< 'Coverage highlight' label.
};

struct BuildDocksStationWindow : public PickerWindowBase {
public:
	BuildDocksStationWindow (const WindowDesc *desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->InitNested(TRANSPORT_WATER);
		this->LowerWidget(_settings_client.gui.station_show_coverage + BDSW_LT_OFF);
	}

	void OnDelete (void) FINAL_OVERRIDE
	{
		DeleteWindowById(WC_SELECT_STATION, 0);
		this->PickerWindowBase::OnDelete();
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		int rad = (_settings_game.station.modified_catchment) ? CA_DOCK : CA_UNMODIFIED;

		this->DrawWidgets (dpi);

		if (_settings_client.gui.station_show_coverage) {
			SetTileSelectBigSize(-rad, -rad, 2 * rad, 2 * rad);
		} else {
			SetTileSelectSize(1, 1);
		}

		/* strings such as 'Size' and 'Coverage Area' */
		int top = this->GetWidget<NWidgetBase>(BDSW_LT_OFF)->pos_y + this->GetWidget<NWidgetBase>(BDSW_LT_OFF)->current_y + WD_PAR_VSEP_NORMAL;
		NWidgetBase *back_nwi = this->GetWidget<NWidgetBase>(BDSW_BACKGROUND);
		int right  = back_nwi->pos_x + back_nwi->current_x;
		int bottom = back_nwi->pos_y + back_nwi->current_y;
		top = DrawStationCoverageAreaText (dpi, back_nwi->pos_x + WD_FRAMERECT_LEFT, right - WD_FRAMERECT_RIGHT, top, rad);
		/* Resize background if the window is too small.
		 * Never make the window smaller to avoid oscillating if the size change affects the acceptance.
		 * (This is the case, if making the window bigger moves the mouse into the window.) */
		if (top > bottom) {
			ResizeWindow(this, 0, top - bottom, false);
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case BDSW_LT_OFF:
			case BDSW_LT_ON:
				this->RaiseWidget(_settings_client.gui.station_show_coverage + BDSW_LT_OFF);
				_settings_client.gui.station_show_coverage = (widget != BDSW_LT_OFF);
				this->LowerWidget(_settings_client.gui.station_show_coverage + BDSW_LT_OFF);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				this->SetDirty();
				break;
		}
	}

	virtual void OnTick()
	{
		CheckRedrawStationCoverage(this);
	}
};

/** Nested widget parts of a build dock station window. */
static const NWidgetPart _nested_build_dock_station_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_STATION_BUILD_DOCK_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN, BDSW_BACKGROUND),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3),
		NWidget(WWT_LABEL, COLOUR_DARK_GREEN, BDSW_INFO), SetMinimalSize(148, 14), SetDataTip(STR_STATION_BUILD_COVERAGE_AREA_TITLE, STR_NULL),
		NWidget(NWID_HORIZONTAL), SetPIP(14, 0, 14),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, BDSW_LT_OFF), SetMinimalSize(40, 12), SetFill(1, 0), SetDataTip(STR_STATION_BUILD_COVERAGE_OFF, STR_STATION_BUILD_COVERAGE_AREA_OFF_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, BDSW_LT_ON), SetMinimalSize(40, 12), SetFill(1, 0), SetDataTip(STR_STATION_BUILD_COVERAGE_ON, STR_STATION_BUILD_COVERAGE_AREA_ON_TOOLTIP),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 20), SetResize(0, 1),
	EndContainer(),
};

static const WindowDesc _build_dock_station_desc(
	WDP_AUTO, 0, 0,
	WC_BUILD_STATION, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_dock_station_widgets, lengthof(_nested_build_dock_station_widgets)
);

static void ShowBuildDockStationPicker(Window *parent)
{
	new BuildDocksStationWindow(&_build_dock_station_desc, parent);
}

struct BuildDocksDepotWindow : public PickerWindowBase {
private:
	static void UpdateDocksDirection()
	{
		if (_ship_depot_direction != AXIS_X) {
			SetTileSelectSize(1, 2);
		} else {
			SetTileSelectSize(2, 1);
		}
	}

public:
	BuildDocksDepotWindow (const WindowDesc *desc, Window *parent) : PickerWindowBase(desc, parent)
	{
		this->InitNested(TRANSPORT_WATER);
		this->LowerWidget(_ship_depot_direction + WID_BDD_X);
		UpdateDocksDirection();
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_BDD_X:
			case WID_BDD_Y:
				size->width  = ScaleGUITrad(96) + 2;
				size->height = ScaleGUITrad(64) + 2;
				break;
		}
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		this->DrawWidgets (dpi);

		int x1 = ScaleGUITrad(63) + 1;
		int x2 = ScaleGUITrad(31) + 1;
		int y1 = ScaleGUITrad(17) + 1;
		int y2 = ScaleGUITrad(33) + 1;

		NWidgetBase *wid_x = this->GetWidget<NWidgetBase> (WID_BDD_X);
		DrawShipDepotSprite (dpi, wid_x->pos_x + x1, wid_x->pos_y + y1, DIAGDIR_NE);
		DrawShipDepotSprite (dpi, wid_x->pos_x + x2, wid_x->pos_y + y2, DIAGDIR_SW);

		NWidgetBase *wid_y = this->GetWidget<NWidgetBase> (WID_BDD_Y);
		DrawShipDepotSprite (dpi, wid_y->pos_x + x2, wid_y->pos_y + y1, DIAGDIR_NW);
		DrawShipDepotSprite (dpi, wid_y->pos_x + x1, wid_y->pos_y + y2, DIAGDIR_SE);
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_BDD_X:
			case WID_BDD_Y:
				this->RaiseWidget(_ship_depot_direction + WID_BDD_X);
				_ship_depot_direction = (widget == WID_BDD_X ? AXIS_X : AXIS_Y);
				this->LowerWidget(_ship_depot_direction + WID_BDD_X);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				UpdateDocksDirection();
				this->SetDirty();
				break;
		}
	}
};

static const NWidgetPart _nested_build_docks_depot_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_DEPOT_BUILD_SHIP_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN, WID_BDD_BACKGROUND),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3),
		NWidget(NWID_HORIZONTAL_LTR),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0),
			NWidget(WWT_PANEL, COLOUR_GREY, WID_BDD_X), SetMinimalSize(98, 66), SetDataTip(0x0, STR_DEPOT_BUILD_SHIP_ORIENTATION_TOOLTIP),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(2, 0),
			NWidget(WWT_PANEL, COLOUR_GREY, WID_BDD_Y), SetMinimalSize(98, 66), SetDataTip(0x0, STR_DEPOT_BUILD_SHIP_ORIENTATION_TOOLTIP),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(3, 0),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3),
	EndContainer(),
};

static const WindowDesc _build_docks_depot_desc(
	WDP_AUTO, 0, 0,
	WC_BUILD_DEPOT, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_build_docks_depot_widgets, lengthof(_nested_build_docks_depot_widgets)
);


static void ShowBuildDocksDepotPicker(Window *parent)
{
	new BuildDocksDepotWindow(&_build_docks_depot_desc, parent);
}


void InitializeDockGui()
{
	_ship_depot_direction = AXIS_X;
}
