/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file town_gui.cpp GUI for towns. */

#include "stdafx.h"

#include <algorithm>
#include <vector>

#include "town.h"
#include "map/zoneheight.h"
#include "map/slope.h"
#include "map/bridge.h"
#include "viewport_func.h"
#include "error.h"
#include "gui.h"
#include "command_func.h"
#include "company_func.h"
#include "company_base.h"
#include "company_gui.h"
#include "network/network.h"
#include "string.h"
#include "strings_func.h"
#include "sound_func.h"
#include "tilehighlight_func.h"
#include "sortlist_type.h"
#include "road_cmd.h"
#include "landscape.h"
#include "querystring_gui.h"
#include "window_func.h"
#include "townname_func.h"
#include "core/geometry_func.hpp"
#include "core/random_func.hpp"
#include "core/smallvec_type.hpp"
#include "genworld.h"
#include "widgets/dropdown_func.h"
#include "economy_func.h"
#include "newgrf_config.h"
#include "newgrf_house.h"
#include "newgrf_cargo.h"
#include "date_func.h"
#include "zoom_func.h"
#include "slope_func.h"

#include "widgets/town_widget.h"

#include "table/strings.h"

typedef GUIList<const Town*> GUITownList;

static const NWidgetPart _nested_town_authority_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_BROWN),
		NWidget(WWT_CAPTION, COLOUR_BROWN, WID_TA_CAPTION), SetDataTip(STR_LOCAL_AUTHORITY_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_BROWN),
		NWidget(WWT_DEFSIZEBOX, COLOUR_BROWN),
		NWidget(WWT_STICKYBOX, COLOUR_BROWN),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_BROWN, WID_TA_RATING_INFO), SetMinimalSize(317, 92), SetResize(1, 1), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_BROWN, WID_TA_COMMAND_LIST), SetMinimalSize(305, 52), SetResize(1, 0), SetDataTip(0x0, STR_LOCAL_AUTHORITY_ACTIONS_TOOLTIP), SetScrollbar(WID_TA_SCROLLBAR), EndContainer(),
		NWidget(NWID_VSCROLLBAR, COLOUR_BROWN, WID_TA_SCROLLBAR),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_BROWN, WID_TA_ACTION_INFO), SetMinimalSize(317, 52), SetResize(1, 0), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TA_EXECUTE),  SetMinimalSize(317, 12), SetResize(1, 0), SetFill(1, 0), SetDataTip(STR_LOCAL_AUTHORITY_DO_IT_BUTTON, STR_LOCAL_AUTHORITY_DO_IT_TOOLTIP),
		NWidget(WWT_RESIZEBOX, COLOUR_BROWN),
	EndContainer()
};

/** Town authority window. */
struct TownAuthorityWindow : Window {
private:
	Town *town;    ///< Town being displayed.
	int sel_index; ///< Currently selected town action, \c 0 to \c TACT_COUNT-1, \c -1 means no action selected.
	Scrollbar *vscroll;
	uint displayed_actions_on_previous_painting; ///< Actions that were available on the previous call to OnPaint()

	/**
	 * Get the position of the Nth set bit.
	 *
	 * If there is no Nth bit set return -1
	 *
	 * @param bits The value to search in
	 * @param n The Nth set bit from which we want to know the position
	 * @return The position of the Nth set bit
	 */
	static int GetNthSetBit (uint bits, uint n)
	{
		uint i;
		FOR_EACH_SET_BIT(i, bits) {
			n--;
			if (n == 0) return i;
		}
		return -1;
	}

public:
	TownAuthorityWindow (const WindowDesc *desc, WindowNumber window_number) :
		Window (desc), town (NULL), sel_index(-1), vscroll (NULL),
		displayed_actions_on_previous_painting (0)
	{
		this->town = Town::Get(window_number);
		this->InitNested(window_number);
		this->vscroll = this->GetScrollbar(WID_TA_SCROLLBAR);
		this->vscroll->SetCapacity((this->GetWidget<NWidgetBase>(WID_TA_COMMAND_LIST)->current_y - WD_FRAMERECT_TOP - WD_FRAMERECT_BOTTOM) / FONT_HEIGHT_NORMAL);
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		int numact;
		uint buttons = GetMaskOfTownActions(&numact, _local_company, this->town);
		if (buttons != displayed_actions_on_previous_painting) this->SetDirty();
		displayed_actions_on_previous_painting = buttons;

		this->vscroll->SetCount(numact + 1);

		if (this->sel_index != -1 && !HasBit(buttons, this->sel_index)) {
			this->sel_index = -1;
		}

		this->SetWidgetDisabledState(WID_TA_EXECUTE, this->sel_index == -1);

		this->DrawWidgets (dpi);
		if (!this->IsShaded()) this->DrawRatings (dpi);
	}

	/** Draw the contents of the ratings panel. May request a resize of the window if the contents does not fit. */
	void DrawRatings (BlitArea *dpi)
	{
		NWidgetBase *nwid = this->GetWidget<NWidgetBase>(WID_TA_RATING_INFO);
		uint left = nwid->pos_x + WD_FRAMERECT_LEFT;
		uint right = nwid->pos_x + nwid->current_x - 1 - WD_FRAMERECT_RIGHT;

		uint y = nwid->pos_y + WD_FRAMERECT_TOP;

		DrawString (dpi, left, right, y, STR_LOCAL_AUTHORITY_COMPANY_RATINGS);
		y += FONT_HEIGHT_NORMAL;

		Dimension icon_size = GetSpriteSize(SPR_COMPANY_ICON);
		int icon_width      = icon_size.width;
		int icon_y_offset   = (FONT_HEIGHT_NORMAL - icon_size.height) / 2;

		Dimension exclusive_size = GetSpriteSize(SPR_EXCLUSIVE_TRANSPORT);
		int exclusive_width      = exclusive_size.width;
		int exclusive_y_offset   = (FONT_HEIGHT_NORMAL - exclusive_size.height) / 2;

		bool rtl = _current_text_dir == TD_RTL;
		uint text_left      = left  + (rtl ? 0 : icon_width + exclusive_width + 4);
		uint text_right     = right - (rtl ? icon_width + exclusive_width + 4 : 0);
		uint icon_left      = rtl ? right - icon_width : left;
		uint exclusive_left = rtl ? right - icon_width - exclusive_width - 2 : left + icon_width + 2;

		/* Draw list of companies */
		const Company *c;
		FOR_ALL_COMPANIES(c) {
			if ((HasBit(this->town->have_ratings, c->index) || this->town->exclusivity == c->index)) {
				DrawCompanyIcon (dpi, c->index, icon_left, y + icon_y_offset);

				SetDParam(0, c->index);
				SetDParam(1, c->index);

				int r = this->town->ratings[c->index];
				StringID str;
				(str = STR_CARGO_RATING_APPALLING, r <= RATING_APPALLING) || // Apalling
				(str++,                    r <= RATING_VERYPOOR)  || // Very Poor
				(str++,                    r <= RATING_POOR)      || // Poor
				(str++,                    r <= RATING_MEDIOCRE)  || // Mediocore
				(str++,                    r <= RATING_GOOD)      || // Good
				(str++,                    r <= RATING_VERYGOOD)  || // Very Good
				(str++,                    r <= RATING_EXCELLENT) || // Excellent
				(str++,                    true);                    // Outstanding

				SetDParam(2, str);
				if (this->town->exclusivity == c->index) {
					DrawSprite (dpi, SPR_EXCLUSIVE_TRANSPORT, COMPANY_SPRITE_COLOUR(c->index), exclusive_left, y + exclusive_y_offset);
				}

				DrawString (dpi, text_left, text_right, y, STR_LOCAL_AUTHORITY_COMPANY_RATING);
				y += FONT_HEIGHT_NORMAL;
			}
		}

		y = y + WD_FRAMERECT_BOTTOM - nwid->pos_y; // Compute needed size of the widget.
		if (y > nwid->current_y) {
			/* If the company list is too big to fit, mark ourself dirty and draw again. */
			ResizeWindow(this, 0, y - nwid->current_y, false);
		}
	}

	virtual void SetStringParameters(int widget) const
	{
		if (widget == WID_TA_CAPTION) SetDParam(0, this->window_number);
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		switch (widget) {
			case WID_TA_ACTION_INFO:
				if (this->sel_index != -1) {
					SetDParam(0, _price[PR_TOWN_ACTION] * _town_action_costs[this->sel_index] >> 8);
					DrawStringMultiLine (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, r.top + WD_FRAMERECT_TOP, r.bottom - WD_FRAMERECT_BOTTOM,
								STR_LOCAL_AUTHORITY_ACTION_TOOLTIP_SMALL_ADVERTISING + this->sel_index);
				}
				break;
			case WID_TA_COMMAND_LIST: {
				int numact;
				uint buttons = GetMaskOfTownActions(&numact, _local_company, this->town);
				int y = r.top + WD_FRAMERECT_TOP;
				int pos = this->vscroll->GetPosition();

				if (--pos < 0) {
					DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_LOCAL_AUTHORITY_ACTIONS_TITLE);
					y += FONT_HEIGHT_NORMAL;
				}

				for (int i = 0; buttons; i++, buttons >>= 1) {
					if (pos <= -5) break; ///< Draw only the 5 fitting lines

					if ((buttons & 1) && --pos < 0) {
						DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y,
								STR_LOCAL_AUTHORITY_ACTION_SMALL_ADVERTISING_CAMPAIGN + i, this->sel_index == i ? TC_WHITE : TC_ORANGE);
						y += FONT_HEIGHT_NORMAL;
					}
				}
				break;
			}
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_TA_ACTION_INFO: {
				assert(size->width > padding.width && size->height > padding.height);
				const uint width = size->width - (WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT);
				uint hmax = 0;
				for (int i = 0; i < TACT_COUNT; i++) {
					SetDParam(0, _price[PR_TOWN_ACTION] * _town_action_costs[i] >> 8);
					uint h = GetStringHeight (STR_LOCAL_AUTHORITY_ACTION_TOOLTIP_SMALL_ADVERTISING + i, width);
					hmax = max (hmax, h);
				}
				size->height = max (size->height, hmax + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
				break;
			}

			case WID_TA_COMMAND_LIST:
				size->height = WD_FRAMERECT_TOP + 5 * FONT_HEIGHT_NORMAL + WD_FRAMERECT_BOTTOM;
				size->width = GetStringBoundingBox(STR_LOCAL_AUTHORITY_ACTIONS_TITLE).width;
				for (uint i = 0; i < TACT_COUNT; i++ ) {
					size->width = max(size->width, GetStringBoundingBox(STR_LOCAL_AUTHORITY_ACTION_SMALL_ADVERTISING_CAMPAIGN + i).width);
				}
				size->width += WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT;
				break;

			case WID_TA_RATING_INFO:
				resize->height = FONT_HEIGHT_NORMAL;
				size->height = WD_FRAMERECT_TOP + 9 * FONT_HEIGHT_NORMAL + WD_FRAMERECT_BOTTOM;
				break;
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_TA_COMMAND_LIST: {
				int y = this->GetRowFromWidget(pt.y, WID_TA_COMMAND_LIST, 1, FONT_HEIGHT_NORMAL);
				if (!IsInsideMM(y, 0, 5)) return;

				y += this->vscroll->GetPosition();
				if (y == 0) break;

				y = GetNthSetBit (GetMaskOfTownActions (NULL, _local_company, this->town), y);
				if (y < 0) break;

				this->sel_index = y;
				this->SetDirty();
				/* When double-clicking, continue */
				if (click_count == 1) break;
			}
			FALLTHROUGH;

			case WID_TA_EXECUTE:
				DoCommandP(this->town->xy, this->window_number, this->sel_index, CMD_DO_TOWN_ACTION);
				break;
		}
	}

	virtual void OnHundredthTick()
	{
		this->SetDirty();
	}
};

static WindowDesc::Prefs _town_authority_prefs ("view_town_authority");

static const WindowDesc _town_authority_desc(
	WDP_AUTO, 317, 222,
	WC_TOWN_AUTHORITY, WC_NONE,
	0,
	_nested_town_authority_widgets, lengthof(_nested_town_authority_widgets),
	&_town_authority_prefs
);

static void ShowTownAuthorityWindow(uint town)
{
	AllocateWindowDescFront<TownAuthorityWindow>(&_town_authority_desc, town);
}


/* Town view window. */
struct TownViewWindow : Window {
private:
	Town *town; ///< Town displayed by the window.

public:
	static const int WID_TV_HEIGHT_NORMAL = 162;

	TownViewWindow (const WindowDesc *desc, WindowNumber window_number) :
		Window (desc), town (NULL)
	{
		this->CreateNestedTree();

		this->town = Town::Get(window_number);
		if (this->town->larger_town) this->GetWidget<NWidgetCore>(WID_TV_CAPTION)->widget_data = STR_TOWN_VIEW_CITY_CAPTION;

		this->InitNested(window_number);

		this->flags |= WF_DISABLE_VP_SCROLL;
		NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_TV_VIEWPORT);
		nvp->InitializeViewport(this, this->town->xy, ZOOM_LVL_NEWS);

		/* disable renaming town in network games if you are not the server */
		this->SetWidgetDisabledState(WID_TV_CHANGE_NAME, _networking && !_network_server);
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		if (_game_mode != GM_EDITOR) {
			this->SetWidgetLoweredState (WID_TV_SHOW_AREA, _thd.town == this->town->index);
		}

		this->DrawWidgets (dpi);
	}

	virtual void SetStringParameters(int widget) const
	{
		if (widget == WID_TV_CAPTION) SetDParam(0, this->town->index);
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		if (widget != WID_TV_INFO) return;

		uint y = r.top + WD_FRAMERECT_TOP;

		SetDParam(0, this->town->cache.population);
		SetDParam(1, this->town->cache.num_houses);
		DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_LEFT, y, STR_TOWN_VIEW_POPULATION_HOUSES);

		SetDParam(0, this->town->supplied[CT_PASSENGERS].old_act);
		SetDParam(1, this->town->supplied[CT_PASSENGERS].old_max);
		DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_LEFT, y += FONT_HEIGHT_NORMAL, STR_TOWN_VIEW_PASSENGERS_LAST_MONTH_MAX);

		SetDParam(0, this->town->supplied[CT_MAIL].old_act);
		SetDParam(1, this->town->supplied[CT_MAIL].old_max);
		DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_LEFT, y += FONT_HEIGHT_NORMAL, STR_TOWN_VIEW_MAIL_LAST_MONTH_MAX);

		bool first = true;
		for (int i = TE_BEGIN; i < TE_END; i++) {
			if (this->town->goal[i] == 0) continue;
			if (this->town->goal[i] == TOWN_GROWTH_WINTER && (TileHeight(this->town->xy) < LowestSnowLine() || this->town->cache.population <= 90)) continue;
			if (this->town->goal[i] == TOWN_GROWTH_DESERT && (GetTropicZone(this->town->xy) != TROPICZONE_DESERT || this->town->cache.population <= 60)) continue;

			if (first) {
				DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_LEFT, y += FONT_HEIGHT_NORMAL, STR_TOWN_VIEW_CARGO_FOR_TOWNGROWTH);
				first = false;
			}

			bool rtl = _current_text_dir == TD_RTL;
			uint cargo_text_left = r.left + WD_FRAMERECT_LEFT + (rtl ? 0 : 20);
			uint cargo_text_right = r.right - WD_FRAMERECT_RIGHT - (rtl ? 20 : 0);

			const CargoSpec *cargo = FindFirstCargoWithTownEffect((TownEffect)i);
			assert(cargo != NULL);

			StringID string;

			if (this->town->goal[i] == TOWN_GROWTH_DESERT || this->town->goal[i] == TOWN_GROWTH_WINTER) {
				/* For 'original' gameplay, don't show the amount required (you need 1 or more ..) */
				string = STR_TOWN_VIEW_CARGO_FOR_TOWNGROWTH_DELIVERED_GENERAL;
				if (this->town->received[i].old_act == 0) {
					string = STR_TOWN_VIEW_CARGO_FOR_TOWNGROWTH_REQUIRED_GENERAL;

					if (this->town->goal[i] == TOWN_GROWTH_WINTER && TileHeight(this->town->xy) < GetSnowLine()) {
						string = STR_TOWN_VIEW_CARGO_FOR_TOWNGROWTH_REQUIRED_WINTER;
					}
				}

				SetDParam(0, cargo->name);
			} else {
				string = STR_TOWN_VIEW_CARGO_FOR_TOWNGROWTH_DELIVERED;
				if (this->town->received[i].old_act < this->town->goal[i]) {
					string = STR_TOWN_VIEW_CARGO_FOR_TOWNGROWTH_REQUIRED;
				}

				SetDParam(0, cargo->Index());
				SetDParam(1, this->town->received[i].old_act);
				SetDParam(2, cargo->Index());
				SetDParam(3, this->town->goal[i]);
			}
			DrawString (dpi, cargo_text_left, cargo_text_right, y += FONT_HEIGHT_NORMAL, string);
		}

		if (HasBit(this->town->flags, TOWN_IS_GROWING)) {
			SetDParam(0, ((this->town->growth_rate & (~TOWN_GROW_RATE_CUSTOM)) * TOWN_GROWTH_TICKS + DAY_TICKS) / DAY_TICKS);
			DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_LEFT, y += FONT_HEIGHT_NORMAL, this->town->fund_buildings_months == 0 ? STR_TOWN_VIEW_TOWN_GROWS_EVERY : STR_TOWN_VIEW_TOWN_GROWS_EVERY_FUNDED);
		} else {
			DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_LEFT, y += FONT_HEIGHT_NORMAL, STR_TOWN_VIEW_TOWN_GROW_STOPPED);
		}

		/* only show the town noise, if the noise option is activated. */
		if (_settings_game.economy.station_noise_level) {
			SetDParam(0, this->town->noise_reached);
			SetDParam(1, this->town->MaxTownNoise());
			DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_LEFT, y += FONT_HEIGHT_NORMAL, STR_TOWN_VIEW_NOISE_IN_TOWN);
		}

		if (this->town->text != NULL) {
			SetDParamStr(0, this->town->text);
			DrawStringMultiLine (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y += FONT_HEIGHT_NORMAL, UINT16_MAX, STR_JUST_RAW_STRING, TC_BLACK);
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_TV_CENTER_VIEW: // scroll to location
				if (_ctrl_pressed) {
					ShowExtraViewPortWindow(this->town->xy);
				} else {
					ScrollMainWindowToTile(this->town->xy);
				}
				break;

			case WID_TV_SHOW_AUTHORITY: // town authority
				ShowTownAuthorityWindow(this->window_number);
				break;

			case WID_TV_CHANGE_NAME: // rename
				SetDParam(0, this->window_number);
				ShowQueryString(STR_TOWN_NAME, STR_TOWN_VIEW_RENAME_TOWN_BUTTON, MAX_LENGTH_TOWN_NAME_CHARS, this, CS_ALPHANUMERAL, QSF_ENABLE_DEFAULT | QSF_LEN_IN_CHARS);
				break;

			case WID_TV_EXPAND: { // expand town - only available on Scenario editor
				/* Warn the user if towns are not allowed to build roads, but do this only once per OpenTTD run. */
				static bool _warn_town_no_roads = false;

				if (!_settings_game.economy.allow_town_roads && !_warn_town_no_roads) {
					ShowErrorMessage(STR_ERROR_TOWN_EXPAND_WARN_NO_ROADS, INVALID_STRING_ID, WL_WARNING);
					_warn_town_no_roads = true;
				}

				DoCommandP(0, this->window_number, 0, CMD_EXPAND_TOWN);
				break;
			}

			case WID_TV_DELETE: // delete town - only available on Scenario editor
				DoCommandP(0, this->window_number, 0, CMD_DELETE_TOWN);
				break;

			case WID_TV_SHOW_AREA: // show town area
				if (_thd.town == this->town->index) {
					_thd.town = INVALID_TOWN;
				} else {
					if (_thd.town != INVALID_TOWN) {
						SetWindowWidgetDirty (WC_TOWN_VIEW, _thd.town, WID_TV_SHOW_AREA);
						MarkTownAreaDirty (_thd.town);
					}
					_thd.town = this->town->index;
				}
				SetWidgetDirty (WID_TV_SHOW_AREA);
				MarkTownAreaDirty (this->town->index);
				break;
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_TV_INFO:
				size->height = GetDesiredInfoHeight(size->width);
				break;
		}
	}

	/**
	 * Gets the desired height for the information panel.
	 * @return the desired height in pixels.
	 */
	uint GetDesiredInfoHeight(int width) const
	{
		uint aimed_height = 3 * FONT_HEIGHT_NORMAL + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;

		bool first = true;
		for (int i = TE_BEGIN; i < TE_END; i++) {
			if (this->town->goal[i] == 0) continue;
			if (this->town->goal[i] == TOWN_GROWTH_WINTER && (TileHeight(this->town->xy) < LowestSnowLine() || this->town->cache.population <= 90)) continue;
			if (this->town->goal[i] == TOWN_GROWTH_DESERT && (GetTropicZone(this->town->xy) != TROPICZONE_DESERT || this->town->cache.population <= 60)) continue;

			if (first) {
				aimed_height += FONT_HEIGHT_NORMAL;
				first = false;
			}
			aimed_height += FONT_HEIGHT_NORMAL;
		}
		aimed_height += FONT_HEIGHT_NORMAL;

		if (_settings_game.economy.station_noise_level) aimed_height += FONT_HEIGHT_NORMAL;

		if (this->town->text != NULL) {
			SetDParamStr(0, this->town->text);
			aimed_height += GetStringHeight(STR_JUST_RAW_STRING, width - WD_FRAMERECT_LEFT - WD_FRAMERECT_RIGHT);
		}

		return aimed_height;
	}

	void ResizeWindowAsNeeded()
	{
		const NWidgetBase *nwid_info = this->GetWidget<NWidgetBase>(WID_TV_INFO);
		uint aimed_height = GetDesiredInfoHeight(nwid_info->current_x);
		if (aimed_height > nwid_info->current_y || (aimed_height < nwid_info->current_y && nwid_info->current_y > nwid_info->smallest_y)) {
			this->ReInit();
		}
	}

	virtual void OnResize()
	{
		if (this->viewport != NULL) {
			NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_TV_VIEWPORT);
			nvp->UpdateViewportCoordinates(this);

			ScrollWindowToTile(this->town->xy, this, true); // Re-center viewport.
		}
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;
		/* Called when setting station noise or required cargoes have changed, in order to resize the window */
		this->SetDirty(); // refresh display for current size. This will allow to avoid glitches when downgrading
		this->ResizeWindowAsNeeded();
	}

	virtual void OnQueryTextFinished(char *str)
	{
		if (str == NULL) return;

		DoCommandP(0, this->window_number, 0, CMD_RENAME_TOWN, str);
	}
};

static const NWidgetPart _nested_town_game_view_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_BROWN),
		NWidget(WWT_CAPTION, COLOUR_BROWN, WID_TV_CAPTION), SetDataTip(STR_TOWN_VIEW_TOWN_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_BROWN),
		NWidget(WWT_DEFSIZEBOX, COLOUR_BROWN),
		NWidget(WWT_STICKYBOX, COLOUR_BROWN),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_BROWN),
		NWidget(WWT_INSET, COLOUR_BROWN), SetPadding(2, 2, 2, 2),
			NWidget(NWID_VIEWPORT, INVALID_COLOUR, WID_TV_VIEWPORT), SetMinimalSize(254, 86), SetFill(1, 0), SetResize(1, 1), SetPadding(1, 1, 1, 1),
		EndContainer(),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_BROWN, WID_TV_INFO), SetMinimalSize(260, 32), SetResize(1, 0), SetFill(1, 0), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL, NC_EQUALSIZE),
			NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_SHOW_AUTHORITY), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_TOWN_VIEW_LOCAL_AUTHORITY_BUTTON, STR_TOWN_VIEW_LOCAL_AUTHORITY_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_SHOW_AREA), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_SHOW_TOWN_AREA, STR_SHOW_TOWN_AREA_TOOLTIP),
			EndContainer(),
			NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_CENTER_VIEW), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_BUTTON_LOCATION, STR_TOWN_VIEW_CENTER_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_CHANGE_NAME), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_BUTTON_RENAME, STR_TOWN_VIEW_RENAME_TOOLTIP),
			EndContainer(),
		EndContainer(),
		NWidget(WWT_RESIZEBOX, COLOUR_BROWN),
	EndContainer(),
};

static WindowDesc::Prefs _town_game_view_prefs ("view_town");

static const WindowDesc _town_game_view_desc(
	WDP_AUTO, 260, TownViewWindow::WID_TV_HEIGHT_NORMAL,
	WC_TOWN_VIEW, WC_NONE,
	0,
	_nested_town_game_view_widgets, lengthof(_nested_town_game_view_widgets),
	&_town_game_view_prefs
);

static const NWidgetPart _nested_town_editor_view_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_BROWN),
		NWidget(WWT_CAPTION, COLOUR_BROWN, WID_TV_CAPTION), SetDataTip(STR_TOWN_VIEW_TOWN_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_BROWN),
		NWidget(WWT_DEFSIZEBOX, COLOUR_BROWN),
		NWidget(WWT_STICKYBOX, COLOUR_BROWN),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_BROWN),
		NWidget(WWT_INSET, COLOUR_BROWN), SetPadding(2, 2, 2, 2),
			NWidget(NWID_VIEWPORT, INVALID_COLOUR, WID_TV_VIEWPORT), SetMinimalSize(254, 86), SetFill(1, 1), SetResize(1, 1), SetPadding(1, 1, 1, 1),
		EndContainer(),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_BROWN, WID_TV_INFO), SetMinimalSize(260, 32), SetResize(1, 0), SetFill(1, 0), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL, NC_EQUALSIZE),
			NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_EXPAND), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_TOWN_VIEW_EXPAND_BUTTON, STR_TOWN_VIEW_EXPAND_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_DELETE), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_TOWN_VIEW_DELETE_BUTTON, STR_TOWN_VIEW_DELETE_TOOLTIP),
			EndContainer(),
			NWidget(NWID_HORIZONTAL, NC_EQUALSIZE),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_CENTER_VIEW), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_BUTTON_LOCATION, STR_TOWN_VIEW_CENTER_TOOLTIP),
				NWidget(WWT_PUSHTXTBTN, COLOUR_BROWN, WID_TV_CHANGE_NAME), SetMinimalSize(130, 12), SetFill(1, 1), SetResize(1, 0), SetDataTip(STR_BUTTON_RENAME, STR_TOWN_VIEW_RENAME_TOOLTIP),
			EndContainer(),
		EndContainer(),
		NWidget(WWT_RESIZEBOX, COLOUR_BROWN),
	EndContainer(),
};

static WindowDesc::Prefs _town_editor_view_prefs ("view_town_scen");

static const WindowDesc _town_editor_view_desc(
	WDP_AUTO, 260, TownViewWindow::WID_TV_HEIGHT_NORMAL,
	WC_TOWN_VIEW, WC_NONE,
	0,
	_nested_town_editor_view_widgets, lengthof(_nested_town_editor_view_widgets),
	&_town_editor_view_prefs
);

void ShowTownViewWindow(TownID town)
{
	if (_game_mode == GM_EDITOR) {
		AllocateWindowDescFront<TownViewWindow>(&_town_editor_view_desc, town);
	} else {
		AllocateWindowDescFront<TownViewWindow>(&_town_game_view_desc, town);
	}
}

static const NWidgetPart _nested_town_directory_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_BROWN),
		NWidget(WWT_CAPTION, COLOUR_BROWN), SetDataTip(STR_TOWN_DIRECTORY_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_BROWN),
		NWidget(WWT_DEFSIZEBOX, COLOUR_BROWN),
		NWidget(WWT_STICKYBOX, COLOUR_BROWN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_TEXTBTN, COLOUR_BROWN, WID_TD_SORT_ORDER), SetDataTip(STR_BUTTON_SORT_BY, STR_TOOLTIP_SORT_ORDER),
				NWidget(WWT_DROPDOWN, COLOUR_BROWN, WID_TD_SORT_CRITERIA), SetDataTip(STR_JUST_STRING, STR_TOOLTIP_SORT_CRITERIA),
				NWidget(WWT_PANEL, COLOUR_BROWN), SetResize(1, 0), EndContainer(),
			EndContainer(),
			NWidget(WWT_PANEL, COLOUR_BROWN, WID_TD_LIST), SetMinimalSize(196, 0), SetDataTip(0x0, STR_TOWN_DIRECTORY_LIST_TOOLTIP),
							SetFill(1, 0), SetResize(0, 10), SetScrollbar(WID_TD_SCROLLBAR), EndContainer(),
			NWidget(WWT_PANEL, COLOUR_BROWN),
				NWidget(WWT_TEXT, COLOUR_BROWN, WID_TD_WORLD_POPULATION), SetPadding(2, 0, 0, 2), SetMinimalSize(196, 12), SetFill(1, 0), SetDataTip(STR_TOWN_POPULATION, STR_NULL),
			EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_BROWN, WID_TD_SCROLLBAR),
			NWidget(WWT_RESIZEBOX, COLOUR_BROWN),
		EndContainer(),
	EndContainer(),
};

/** Town directory window class. */
struct TownDirectoryWindow : public Window {
private:
	/* Runtime saved values */
	static Listing last_sorting;
	static const Town *last_town;

	/* Constants for sorting towns */
	static const StringID sorter_names[];
	static GUITownList::SortFunction * const sorter_funcs[];

	GUITownList towns;

	Scrollbar *vscroll;

	void BuildSortTownList()
	{
		if (this->towns.NeedRebuild()) {
			this->towns.Clear();

			const Town *t;
			FOR_ALL_TOWNS(t) {
				*this->towns.Append() = t;
			}

			this->towns.Compact();
			this->towns.RebuildDone();
			this->vscroll->SetCount(this->towns.Length()); // Update scrollbar as well.
		}
		/* Always sort the towns. */
		this->last_town = NULL;
		this->towns.Sort();
		this->SetWidgetDirty(WID_TD_LIST); // Force repaint of the displayed towns.
	}

	/** Sort by town name */
	static int CDECL TownNameSorter(const Town * const *a, const Town * const *b)
	{
		static char buf_cache[64];
		const Town *ta = *a;
		const Town *tb = *b;
		char buf[64];

		SetDParam(0, ta->index);
		GetString (buf, STR_TOWN_NAME);

		/* If 'b' is the same town as in the last round, use the cached value
		 * We do this to speed stuff up ('b' is called with the same value a lot of
		 * times after each other) */
		if (tb != last_town) {
			last_town = tb;
			SetDParam(0, tb->index);
			GetString (buf_cache, STR_TOWN_NAME);
		}

		return strnatcmp(buf, buf_cache); // Sort by name (natural sorting).
	}

	/** Sort by population (default descending, as big towns are of the most interest). */
	static int CDECL TownPopulationSorter(const Town * const *a, const Town * const *b)
	{
		uint32 a_population = (*a)->cache.population;
		uint32 b_population = (*b)->cache.population;
		if (a_population == b_population) return TownDirectoryWindow::TownNameSorter(a, b);
		return (a_population < b_population) ? -1 : 1;
	}

	/** Sort by town rating */
	static int CDECL TownRatingSorter(const Town * const *a, const Town * const *b)
	{
		int before = TownDirectoryWindow::last_sorting.order ? 1 : -1; // Value to get 'a' before 'b'.

		/* Towns without rating are always after towns with rating. */
		if (HasBit((*a)->have_ratings, _local_company)) {
			if (HasBit((*b)->have_ratings, _local_company)) {
				int16 a_rating = (*a)->ratings[_local_company];
				int16 b_rating = (*b)->ratings[_local_company];
				if (a_rating == b_rating) return TownDirectoryWindow::TownNameSorter(a, b);
				return (a_rating < b_rating) ? -1 : 1;
			}
			return before;
		}
		if (HasBit((*b)->have_ratings, _local_company)) return -before;
		return -before * TownDirectoryWindow::TownNameSorter(a, b); // Sort unrated towns always on ascending town name.
	}

public:
	TownDirectoryWindow (const WindowDesc *desc) :
		Window (desc), towns(), vscroll (NULL)
	{
		this->CreateNestedTree();

		this->vscroll = this->GetScrollbar(WID_TD_SCROLLBAR);

		this->towns.SetListing(this->last_sorting);
		this->towns.SetSortFuncs(TownDirectoryWindow::sorter_funcs);
		this->towns.ForceRebuild();
		this->BuildSortTownList();

		this->InitNested(0);
	}

	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_TD_WORLD_POPULATION:
				SetDParam(0, GetWorldPopulation());
				break;

			case WID_TD_SORT_CRITERIA:
				SetDParam(0, TownDirectoryWindow::sorter_names[this->towns.SortType()]);
				break;
		}
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		switch (widget) {
			case WID_TD_SORT_ORDER:
				this->DrawSortButtonState (dpi, widget, this->towns.IsDescSortOrder() ? SBS_DOWN : SBS_UP);
				break;

			case WID_TD_LIST: {
				int n = 0;
				int y = r.top + WD_FRAMERECT_TOP;
				if (this->towns.Length() == 0) { // No towns available.
					DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right, y, STR_TOWN_DIRECTORY_NONE);
					break;
				}

				/* At least one town available. */
				bool rtl = _current_text_dir == TD_RTL;
				Dimension icon_size = GetSpriteSize(SPR_TOWN_RATING_GOOD);
				int text_left  = r.left + WD_FRAMERECT_LEFT + (rtl ? 0 : icon_size.width + 2);
				int text_right = r.right - WD_FRAMERECT_RIGHT - (rtl ? icon_size.width + 2 : 0);
				int icon_x = rtl ? r.right - WD_FRAMERECT_RIGHT - icon_size.width : r.left + WD_FRAMERECT_LEFT;

				for (uint i = this->vscroll->GetPosition(); i < this->towns.Length(); i++) {
					const Town *t = this->towns[i];
					assert(t->xy != INVALID_TILE);

					/* Draw rating icon. */
					if (_game_mode == GM_EDITOR || !HasBit(t->have_ratings, _local_company)) {
						DrawSprite (dpi, SPR_TOWN_RATING_NA, PAL_NONE, icon_x, y + (this->resize.step_height - icon_size.height) / 2);
					} else {
						SpriteID icon = SPR_TOWN_RATING_APALLING;
						if (t->ratings[_local_company] > RATING_VERYPOOR) icon = SPR_TOWN_RATING_MEDIOCRE;
						if (t->ratings[_local_company] > RATING_GOOD)     icon = SPR_TOWN_RATING_GOOD;
						DrawSprite (dpi, icon, PAL_NONE, icon_x, y + (this->resize.step_height - icon_size.height) / 2);
					}

					SetDParam(0, t->index);
					SetDParam(1, t->cache.population);
					DrawString (dpi, text_left, text_right, y + (this->resize.step_height - FONT_HEIGHT_NORMAL) / 2, STR_TOWN_DIRECTORY_TOWN);

					y += this->resize.step_height;
					if (++n == this->vscroll->GetCapacity()) break; // max number of towns in 1 window
				}
				break;
			}
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_TD_SORT_ORDER: {
				Dimension d = GetStringBoundingBox(this->GetWidget<NWidgetCore>(widget)->widget_data);
				d.width += padding.width + Window::SortButtonWidth() * 2; // Doubled since the string is centred and it also looks better.
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}
			case WID_TD_SORT_CRITERIA: {
				Dimension d = {0, 0};
				for (uint i = 0; TownDirectoryWindow::sorter_names[i] != INVALID_STRING_ID; i++) {
					d = maxdim(d, GetStringBoundingBox(TownDirectoryWindow::sorter_names[i]));
				}
				d.width += padding.width;
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}
			case WID_TD_LIST: {
				Dimension d = GetStringBoundingBox(STR_TOWN_DIRECTORY_NONE);
				for (uint i = 0; i < this->towns.Length(); i++) {
					const Town *t = this->towns[i];

					assert(t != NULL);

					SetDParam(0, t->index);
					SetDParamMaxDigits(1, 8);
					d = maxdim(d, GetStringBoundingBox(STR_TOWN_DIRECTORY_TOWN));
				}
				Dimension icon_size = GetSpriteSize(SPR_TOWN_RATING_GOOD);
				d.width += icon_size.width + 2;
				d.height = max(d.height, icon_size.height);
				resize->height = d.height;
				d.height *= 5;
				d.width += padding.width + WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT;
				d.height += padding.height + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;
				*size = maxdim(*size, d);
				break;
			}
			case WID_TD_WORLD_POPULATION: {
				SetDParamMaxDigits(0, 10);
				Dimension d = GetStringBoundingBox(STR_TOWN_POPULATION);
				d.width += padding.width;
				d.height += padding.height;
				*size = maxdim(*size, d);
				break;
			}
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_TD_SORT_ORDER: // Click on sort order button
				if (this->towns.SortType() != 2) { // A different sort than by rating.
					this->towns.ToggleSortOrder();
					this->last_sorting = this->towns.GetListing(); // Store new sorting order.
				} else {
					/* Some parts are always sorted ascending on name. */
					this->last_sorting.order = !this->last_sorting.order;
					this->towns.SetListing(this->last_sorting);
					this->towns.ForceResort();
					this->towns.Sort();
				}
				this->SetDirty();
				break;

			case WID_TD_SORT_CRITERIA: // Click on sort criteria dropdown
				ShowDropDownMenu(this, TownDirectoryWindow::sorter_names, this->towns.SortType(), WID_TD_SORT_CRITERIA, 0, 0);
				break;

			case WID_TD_LIST: { // Click on Town Matrix
				uint id_v = this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_TD_LIST, WD_FRAMERECT_TOP);
				if (id_v >= this->towns.Length()) return; // click out of town bounds

				const Town *t = this->towns[id_v];
				assert(t != NULL);
				if (_ctrl_pressed) {
					ShowExtraViewPortWindow(t->xy);
				} else {
					ScrollMainWindowToTile(t->xy);
				}
				break;
			}
		}
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		if (widget != WID_TD_SORT_CRITERIA) return;

		if (this->towns.SortType() != index) {
			this->towns.SetSortType(index);
			this->last_sorting = this->towns.GetListing(); // Store new sorting order.
			this->BuildSortTownList();
		}
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		if (this->towns.NeedRebuild()) this->BuildSortTownList();
		this->DrawWidgets (dpi);
	}

	virtual void OnHundredthTick()
	{
		this->BuildSortTownList();
		this->SetDirty();
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_TD_LIST);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (data == 0) {
			/* This needs to be done in command-scope to enforce rebuilding before resorting invalid data */
			this->towns.ForceRebuild();
		} else {
			this->towns.ForceResort();
		}
	}
};

Listing TownDirectoryWindow::last_sorting = {false, 0};
const Town *TownDirectoryWindow::last_town = NULL;

/** Names of the sorting functions. */
const StringID TownDirectoryWindow::sorter_names[] = {
	STR_SORT_BY_NAME,
	STR_SORT_BY_POPULATION,
	STR_SORT_BY_RATING,
	INVALID_STRING_ID
};

/** Available town directory sorting functions. */
GUITownList::SortFunction * const TownDirectoryWindow::sorter_funcs[] = {
	&TownNameSorter,
	&TownPopulationSorter,
	&TownRatingSorter,
};

static WindowDesc::Prefs _town_directory_prefs ("list_towns");

static const WindowDesc _town_directory_desc(
	WDP_AUTO, 208, 202,
	WC_TOWN_DIRECTORY, WC_NONE,
	0,
	_nested_town_directory_widgets, lengthof(_nested_town_directory_widgets),
	&_town_directory_prefs
);

void ShowTownDirectory()
{
	if (BringWindowToFrontById(WC_TOWN_DIRECTORY, 0)) return;
	new TownDirectoryWindow(&_town_directory_desc);
}

void CcFoundTown(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Failed()) return;

	if (HasBit(p1, 6)) {
		ScrollMainWindowToTile(Town::Get(_new_town_id)->xy);
	} else {
		if (_settings_client.sound.confirm) SndPlayTileFx(SND_1F_SPLAT_OTHER, tile);
		if (!_settings_client.gui.persistent_buildingtools) ResetPointerMode();
	}
}

StringID GetErrFoundTown (TileIndex tile, uint32 p1, uint32 p2, const char *text)
{
	return HasBit(p1, 6) ? STR_ERROR_CAN_T_GENERATE_TOWN : STR_ERROR_CAN_T_FOUND_TOWN_HERE;
}

static const NWidgetPart _nested_found_town_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_FOUND_TOWN_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_SHADEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	/* Construct new town(s) buttons. */
	NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
		NWidget(NWID_SPACER), SetMinimalSize(0, 2),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_NEW_TOWN), SetMinimalSize(156, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_NEW_TOWN_BUTTON, STR_FOUND_TOWN_NEW_TOWN_TOOLTIP), SetPadding(0, 2, 1, 2),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_TF_RANDOM_TOWN), SetMinimalSize(156, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_RANDOM_TOWN_BUTTON, STR_FOUND_TOWN_RANDOM_TOWN_TOOLTIP), SetPadding(0, 2, 1, 2),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_TF_MANY_RANDOM_TOWNS), SetMinimalSize(156, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_MANY_RANDOM_TOWNS, STR_FOUND_TOWN_RANDOM_TOWNS_TOOLTIP), SetPadding(0, 2, 0, 2),
		/* Town name selection. */
		NWidget(WWT_LABEL, COLOUR_DARK_GREEN), SetMinimalSize(156, 14), SetPadding(0, 2, 0, 2), SetDataTip(STR_FOUND_TOWN_NAME_TITLE, STR_NULL),
		NWidget(WWT_EDITBOX, COLOUR_GREY, WID_TF_TOWN_NAME_EDITBOX), SetMinimalSize(156, 12), SetPadding(0, 2, 3, 2),
										SetDataTip(STR_FOUND_TOWN_NAME_EDITOR_TITLE, STR_FOUND_TOWN_NAME_EDITOR_HELP),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_TF_TOWN_NAME_RANDOM), SetMinimalSize(78, 12), SetPadding(0, 2, 0, 2), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_NAME_RANDOM_BUTTON, STR_FOUND_TOWN_NAME_RANDOM_TOOLTIP),
		/* Town size selection. */
		NWidget(NWID_HORIZONTAL), SetPIP(2, 0, 2),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(WWT_LABEL, COLOUR_DARK_GREEN), SetMinimalSize(148, 14), SetDataTip(STR_FOUND_TOWN_INITIAL_SIZE_TITLE, STR_NULL),
			NWidget(NWID_SPACER), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_HORIZONTAL, NC_EQUALSIZE), SetPIP(2, 0, 2),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_SIZE_SMALL), SetMinimalSize(78, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_INITIAL_SIZE_SMALL_BUTTON, STR_FOUND_TOWN_INITIAL_SIZE_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_SIZE_MEDIUM), SetMinimalSize(78, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_INITIAL_SIZE_MEDIUM_BUTTON, STR_FOUND_TOWN_INITIAL_SIZE_TOOLTIP),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 1),
		NWidget(NWID_HORIZONTAL, NC_EQUALSIZE), SetPIP(2, 0, 2),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_SIZE_LARGE), SetMinimalSize(78, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_INITIAL_SIZE_LARGE_BUTTON, STR_FOUND_TOWN_INITIAL_SIZE_TOOLTIP),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_SIZE_RANDOM), SetMinimalSize(78, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_SIZE_RANDOM, STR_FOUND_TOWN_INITIAL_SIZE_TOOLTIP),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 3),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_CITY), SetPadding(0, 2, 0, 2), SetMinimalSize(156, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_CITY, STR_FOUND_TOWN_CITY_TOOLTIP), SetFill(1, 0),
		/* Town roads selection. */
		NWidget(NWID_HORIZONTAL), SetPIP(2, 0, 2),
			NWidget(NWID_SPACER), SetFill(1, 0),
			NWidget(WWT_LABEL, COLOUR_DARK_GREEN), SetMinimalSize(148, 14), SetDataTip(STR_FOUND_TOWN_ROAD_LAYOUT, STR_NULL),
			NWidget(NWID_SPACER), SetFill(1, 0),
		EndContainer(),
		NWidget(NWID_HORIZONTAL, NC_EQUALSIZE), SetPIP(2, 0, 2),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_LAYOUT_ORIGINAL), SetMinimalSize(78, 12), SetFill(1, 0), SetDataTip(STR_FOUND_TOWN_SELECT_LAYOUT_ORIGINAL, STR_FOUND_TOWN_SELECT_TOWN_ROAD_LAYOUT),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_LAYOUT_BETTER), SetMinimalSize(78, 12), SetFill(1, 0), SetDataTip(STR_FOUND_TOWN_SELECT_LAYOUT_BETTER_ROADS, STR_FOUND_TOWN_SELECT_TOWN_ROAD_LAYOUT),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 1),
		NWidget(NWID_HORIZONTAL, NC_EQUALSIZE), SetPIP(2, 0, 2),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_LAYOUT_GRID2), SetMinimalSize(78, 12), SetFill(1, 0), SetDataTip(STR_FOUND_TOWN_SELECT_LAYOUT_2X2_GRID, STR_FOUND_TOWN_SELECT_TOWN_ROAD_LAYOUT),
			NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_LAYOUT_GRID3), SetMinimalSize(78, 12), SetFill(1, 0), SetDataTip(STR_FOUND_TOWN_SELECT_LAYOUT_3X3_GRID, STR_FOUND_TOWN_SELECT_TOWN_ROAD_LAYOUT),
		EndContainer(),
		NWidget(NWID_SPACER), SetMinimalSize(0, 1),
		NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_TF_LAYOUT_RANDOM), SetPadding(0, 2, 0, 2), SetMinimalSize(0, 12), SetFill(1, 0),
										SetDataTip(STR_FOUND_TOWN_SELECT_LAYOUT_RANDOM, STR_FOUND_TOWN_SELECT_TOWN_ROAD_LAYOUT), SetFill(1, 0),
		NWidget(NWID_SPACER), SetMinimalSize(0, 2),
	EndContainer(),
};

/** Found a town window class. */
struct FoundTownWindow : Window {
private:
	TownSize town_size;     ///< Selected town size
	TownLayout town_layout; ///< Selected town layout
	bool city;              ///< Are we building a city?
	QueryStringC <MAX_LENGTH_TOWN_NAME_CHARS> townname_editbox; ///< Townname editbox
	bool townnamevalid;     ///< Is generated town name valid?
	uint32 townnameparts;   ///< Generated town name
	TownNameParams params;  ///< Town name parameters

public:
	FoundTownWindow (const WindowDesc *desc, WindowNumber window_number) :
			Window(desc),
			town_size(TSZ_MEDIUM),
			town_layout(_settings_game.economy.town_layout),
			city(false),
			townname_editbox(),
			townnamevalid(false),
			townnameparts(0),
			params(_settings_game.game_creation.town_name)
	{
		this->InitNested(window_number);
		this->querystrings[WID_TF_TOWN_NAME_EDITBOX] = &this->townname_editbox;
		this->RandomTownName();
		this->UpdateButtons(true);
	}

	void RandomTownName()
	{
		this->townnamevalid = GenerateTownName(&this->townnameparts);

		if (!this->townnamevalid) {
			this->townname_editbox.DeleteAll();
		} else {
			this->townname_editbox.clear();
			AppendTownName (&this->townname_editbox, &this->params, this->townnameparts);
			this->townname_editbox.UpdateSize();
		}
		UpdateOSKOriginalText(this, WID_TF_TOWN_NAME_EDITBOX);

		this->SetWidgetDirty(WID_TF_TOWN_NAME_EDITBOX);
	}

	void UpdateButtons(bool check_availability)
	{
		if (check_availability && _game_mode != GM_EDITOR) {
			this->SetWidgetsDisabledState(true, WID_TF_RANDOM_TOWN, WID_TF_MANY_RANDOM_TOWNS, WID_TF_SIZE_LARGE, WIDGET_LIST_END);
			this->SetWidgetsDisabledState(_settings_game.economy.found_town != TF_CUSTOM_LAYOUT,
					WID_TF_LAYOUT_ORIGINAL, WID_TF_LAYOUT_BETTER, WID_TF_LAYOUT_GRID2, WID_TF_LAYOUT_GRID3, WID_TF_LAYOUT_RANDOM, WIDGET_LIST_END);
			if (_settings_game.economy.found_town != TF_CUSTOM_LAYOUT) town_layout = _settings_game.economy.town_layout;
		}

		for (int i = WID_TF_SIZE_SMALL; i <= WID_TF_SIZE_RANDOM; i++) {
			this->SetWidgetLoweredState(i, i == WID_TF_SIZE_SMALL + this->town_size);
		}

		this->SetWidgetLoweredState(WID_TF_CITY, this->city);

		for (int i = WID_TF_LAYOUT_ORIGINAL; i <= WID_TF_LAYOUT_RANDOM; i++) {
			this->SetWidgetLoweredState(i, i == WID_TF_LAYOUT_ORIGINAL + this->town_layout);
		}

		this->SetDirty();
	}

	void ExecuteFoundTownCommand(TileIndex tile, bool random)
	{
		const char *name = NULL;

		if (!this->townnamevalid) {
			name = this->townname_editbox.GetText();
		} else {
			/* If user changed the name, send it */
			sstring<MAX_LENGTH_TOWN_NAME_CHARS * MAX_CHAR_LENGTH> buf;
			AppendTownName (&buf, &this->params, this->townnameparts);
			if (strcmp (buf.c_str(), this->townname_editbox.GetText()) != 0) name = this->townname_editbox.GetText();
		}

		bool success = DoCommandP(tile, this->town_size | this->city << 2 | this->town_layout << 3 | random << 6,
				townnameparts, CMD_FOUND_TOWN, name);

		/* Rerandomise name, if success and no cost-estimation. */
		if (success && !_shift_pressed) this->RandomTownName();
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_TF_NEW_TOWN:
				HandlePlacePushButton (this, WID_TF_NEW_TOWN, SPR_CURSOR_TOWN, POINTER_TILE);
				break;

			case WID_TF_RANDOM_TOWN:
				this->ExecuteFoundTownCommand(0, true);
				break;

			case WID_TF_TOWN_NAME_RANDOM:
				this->RandomTownName();
				this->SetFocusedWidget(WID_TF_TOWN_NAME_EDITBOX);
				break;

			case WID_TF_MANY_RANDOM_TOWNS:
				_generating_world = true;
				UpdateNearestTownForRoadTiles(true);
				if (!GenerateTowns(this->town_layout)) {
					ShowErrorMessage(STR_ERROR_CAN_T_GENERATE_TOWN, STR_ERROR_NO_SPACE_FOR_TOWN, WL_INFO);
				}
				UpdateNearestTownForRoadTiles(false);
				_generating_world = false;
				break;

			case WID_TF_SIZE_SMALL: case WID_TF_SIZE_MEDIUM: case WID_TF_SIZE_LARGE: case WID_TF_SIZE_RANDOM:
				this->town_size = (TownSize)(widget - WID_TF_SIZE_SMALL);
				this->UpdateButtons(false);
				break;

			case WID_TF_CITY:
				this->city ^= true;
				this->SetWidgetLoweredState(WID_TF_CITY, this->city);
				this->SetDirty();
				break;

			case WID_TF_LAYOUT_ORIGINAL: case WID_TF_LAYOUT_BETTER: case WID_TF_LAYOUT_GRID2:
			case WID_TF_LAYOUT_GRID3: case WID_TF_LAYOUT_RANDOM:
				this->town_layout = (TownLayout)(widget - WID_TF_LAYOUT_ORIGINAL);
				this->UpdateButtons(false);
				break;
		}
	}

	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		this->ExecuteFoundTownCommand(tile, false);
	}

	virtual void OnPlaceObjectAbort()
	{
		this->RaiseButtons();
		this->UpdateButtons(false);
	}

	/**
	 * Some data on this window has become invalid.
	 * @param data Information about the changed data.
	 * @param gui_scope Whether the call is done from GUI scope. You may not do everything when not in GUI scope. See #InvalidateWindowData() for details.
	 */
	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		if (!gui_scope) return;
		this->UpdateButtons(true);
	}
};

static WindowDesc::Prefs _found_town_prefs ("build_town");

static const WindowDesc _found_town_desc(
	WDP_AUTO, 160, 162,
	WC_FOUND_TOWN, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_found_town_widgets, lengthof(_nested_found_town_widgets),
	&_found_town_prefs
);

void ShowFoundTownWindow()
{
	if (_game_mode != GM_EDITOR && !Company::IsValidID(_local_company)) return;
	AllocateWindowDescFront<FoundTownWindow>(&_found_town_desc, 0);
}


/**
 * Window for selecting towns to build a house in.
 */
struct SelectTownWindow : Window {
	const TileIndex tile;       ///< tile where to build the house
	const HouseID house;        ///< house to build
	std::vector <Town *> towns; ///< sorted vector of towns
	Scrollbar *vscroll;         ///< scrollbar for the town list
	bool rebuild;               ///< town vector must be rebuilt

	void RebuildTownList (void);

	SelectTownWindow (const WindowDesc *desc, TileIndex tile, HouseID house)
		: Window(desc), tile(tile), house(house), towns(),
		  vscroll(NULL), rebuild(false)
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_ST_SCROLLBAR);
		this->RebuildTownList();
		this->InitNested();
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		if (widget != WID_ST_PANEL) return;

		/* Determine the widest string */
		Dimension d = { 0, 0 };
		for (uint i = 0; i < Town::pool.items; i++) {
			SetDParam(0, i);
			d = maxdim(d, GetStringBoundingBox(STR_SELECT_TOWN_LIST_ITEM));
		}

		resize->height = d.height;
		d.height *= 5;
		d.width += WD_FRAMERECT_RIGHT + WD_FRAMERECT_LEFT;
		d.height += WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;
		*size = d;
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		if (widget != WID_ST_PANEL) return;

		uint y = r.top + WD_FRAMERECT_TOP;
		uint end = min(this->vscroll->GetCount(), this->vscroll->GetPosition() + this->vscroll->GetCapacity());
		for (uint i = this->vscroll->GetPosition(); i < end; i++) {
			SetDParam(0, this->towns[i]->index);
			DrawString (dpi, r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, y, STR_SELECT_TOWN_LIST_ITEM);
			y += this->resize.step_height;
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		if (widget != WID_ST_PANEL) return;

		uint pos = this->vscroll->GetScrolledRowFromWidget(pt.y, this, WID_ST_PANEL, WD_FRAMERECT_TOP);
		if (pos >= this->towns.size()) return;

		Town *town = this->towns[pos];

		StringID err = IsNewTownHouseAllowed (town, this->house);
		if (err != STR_NULL) {
			ShowErrorMessage (STR_ERROR_CAN_T_BUILD_HOUSE_HERE,
					err, WL_INFO, pt.x, pt.y);
			return;
		}

		/* Place a house */
		DoBuildHouse (town, this->tile, this->house, InteractiveRandom());

		/* Close the window */
		this->Delete();
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		if (this->rebuild) this->RebuildTownList();
		this->DrawWidgets (dpi);
	}

	virtual void OnResize()
	{
		this->vscroll->SetCapacityFromWidget(this, WID_ST_PANEL, WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM);
	}

	virtual void OnInvalidateData(int data = 0, bool gui_scope = true)
	{
		this->rebuild = true;
	}
};

struct TownDistanceSorter {
	const TileIndex tile;

	TownDistanceSorter (TileIndex tile) : tile (tile)
	{
	}

	bool operator() (const Town *t1, const Town *t2)
	{
		return DistanceSquare (tile, t1->xy) < DistanceSquare (tile, t2->xy);
	}
};

void SelectTownWindow::RebuildTownList (void)
{
	this->towns.clear();

	Town *t;
	FOR_ALL_TOWNS(t) {
		this->towns.push_back (t);
	}

	TownDistanceSorter sorter (this->tile);
	std::stable_sort (this->towns.begin(), this->towns.end(), sorter);

	this->rebuild = false;
	this->vscroll->SetCount (this->towns.size());
}

static const NWidgetPart _nested_select_town_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_ST_CAPTION), SetDataTip(STR_SELECT_TOWN_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_DEFSIZEBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN, WID_ST_PANEL), SetResize(1, 0), SetScrollbar(WID_ST_SCROLLBAR), EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_DARK_GREEN, WID_ST_SCROLLBAR),
			NWidget(WWT_RESIZEBOX, COLOUR_DARK_GREEN),
		EndContainer(),
	EndContainer(),
};

static WindowDesc::Prefs _select_town_prefs ("select_town");

static const WindowDesc _select_town_desc(
	WDP_AUTO, 100, 0,
	WC_SELECT_TOWN, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_select_town_widgets, lengthof(_nested_select_town_widgets),
	&_select_town_prefs
);


/** The window used for building houses. */
class HousePickerWindow : public Window {
	std::vector <HouseID> houses; ///< List of available houses.
	std::vector <uint16>  sets;   ///< List of house sets, each item points the first house of the set in the houses array.
	uint sel_set;                 ///< Index of the selected house set.
	uint sel_offset;              ///< Index of the selected house.
	StringID name;                ///< Name of the selected house.
	uint32 supply;                ///< Cargo mask of produced cargo.
	char acceptance[DRAW_STRING_BUFFER]; ///< String representation of accepted cargo.
	uint line_height;             ///< Height of a single line in the list of house sets.

	static HouseID cur_house;     ///< House selected in the house picker window.

	uint GetSetSize (uint set) const
	{
		assert (set < this->sets.size());
		uint next = set + 1;
		return ((next == this->sets.size()) ? this->houses.size() : this->sets[next])
				- this->sets[this->sel_set];
	}

	static void GetAcceptedCargo (CargoArray *acceptance, HouseID house)
	{
		const HouseSpec *hs = HouseSpec::Get (house);
		CargoID accepts[3];

		/* Set the initial accepted cargo types. */
		for (uint i = 0; i < lengthof(accepts); i++) {
			accepts[i] = hs->accepts_cargo[i];
		}

		/* Check for custom accepted cargo types. */
		if (HasBit(hs->callback_mask, CBM_HOUSE_ACCEPT_CARGO)) {
			uint16 callback = GetHouseCallback (CBID_HOUSE_ACCEPT_CARGO, 0, 0, house);
			if (callback != CALLBACK_FAILED) {
				/* Replace accepted cargo types with translated values from callback. */
				accepts[0] = GetCargoTranslation (GB(callback,  0, 5), hs->grf_prop.grffile);
				accepts[1] = GetCargoTranslation (GB(callback,  5, 5), hs->grf_prop.grffile);
				accepts[2] = GetCargoTranslation (GB(callback, 10, 5), hs->grf_prop.grffile);
			}
		}

		/* Check for custom cargo acceptance */
		if (HasBit(hs->callback_mask, CBM_HOUSE_CARGO_ACCEPTANCE)) {
			uint16 callback = GetHouseCallback (CBID_HOUSE_CARGO_ACCEPTANCE, 0, 0, house);
			if (callback != CALLBACK_FAILED) {
				if (accepts[0] != CT_INVALID) (*acceptance)[accepts[0]] += GB(callback, 0, 4);
				if (accepts[1] != CT_INVALID) (*acceptance)[accepts[1]] += GB(callback, 4, 4);
				if (_settings_game.game_creation.landscape != LT_TEMPERATE && HasBit(callback, 12)) {
					/* The 'S' bit indicates food instead of goods. */
					(*acceptance)[CT_FOOD] += GB(callback, 8, 4);
				} else if (accepts[2] != CT_INVALID) {
					(*acceptance)[accepts[2]] += GB(callback, 8, 4);
				}
				return;
			}
		}

		/* No custom acceptance, so fill in with the default values. */
		for (uint i = 0; i < lengthof(accepts); i++) {
			if (accepts[i] != CT_INVALID) (*acceptance)[accepts[i]] += hs->cargo_acceptance[i];
		}
	}

	void BuildSetList (void)
	{
		/* Try to reselect the previous selection. */
		this->sel_set = 0;
		this->sel_offset = 0;

		const GRFFile *cur_grffile = (cur_house != INVALID_HOUSE_ID) ?
				HouseSpec::Get (cur_house)->grf_prop.grffile : NULL;

		assert (!this->houses.empty());

		uint i = 0;
		for (;;) {
			HouseID house = this->houses[i];
			const HouseSpec *hs = HouseSpec::Get (house);
			const GRFFile *grffile = hs->grf_prop.grffile;
			if (grffile == cur_grffile) {
				this->sel_set = this->sets.size();
			}
			this->sets.push_back (i);
			for (;;) {
				if (house == cur_house) {
					this->sel_offset = i;
				}
				i++;
				if (i == this->houses.size()) return;
				house = this->houses[i];
				if (HouseSpec::Get(house)->grf_prop.grffile != grffile) break;
			}
		}
	}

	void SetObjectToPlace (void)
	{
		SetPointerMode (POINTER_TILE, this, SPR_CURSOR_TOWN);
	}

	void UpdateCache (void)
	{
		if (cur_house == INVALID_HOUSE_ID) {
			SetTileSelectSize (1, 1);
			this->name = STR_EMPTY;
			this->supply = 0;
			this->acceptance[0] = '\0';
			return;
		}

		const HouseSpec *hs = HouseSpec::Get (cur_house);

		SetTileSelectSize ((hs->building_flags & BUILDING_2_TILES_X) ? 2 : 1,
				   (hs->building_flags & BUILDING_2_TILES_Y) ? 2 : 1);

		/* Cache house name. */
		this->name = hs->building_name;
		uint16 callback_res = GetHouseCallback (CBID_HOUSE_CUSTOM_NAME, 1, 0, cur_house);
		if (callback_res != CALLBACK_FAILED && callback_res != 0x400) {
			uint32 grfid = hs->grf_prop.grffile->grfid;
			if (callback_res > 0x400) {
				ErrorUnknownCallbackResult (grfid, CBID_HOUSE_CUSTOM_NAME, callback_res);
			} else {
				StringID ret = GetGRFStringID (grfid, 0xD000 + callback_res);
				if (ret != STR_NULL && ret != STR_UNDEFINED) this->name = ret;
			}
		}

		/* Cache house production. */
		if (HasBit(hs->callback_mask, CBM_HOUSE_PRODUCE_CARGO)) {
			CargoArray production;
			for (uint i = 0; i < 256; i++) {
				uint16 callback = GetHouseCallback (CBID_HOUSE_PRODUCE_CARGO, i, 0, cur_house);
				if (callback == CALLBACK_FAILED || callback == CALLBACK_HOUSEPRODCARGO_END) break;
				CargoID c = GetCargoTranslation (GB(callback, 8, 7), hs->grf_prop.grffile);
				if (c != CT_INVALID) production[c]++;
			}
			uint32 mask = 0;
			for (CargoID i = 0; i < NUM_CARGO; i++) if (production[i] != 0) SetBit (mask, i);
			this->supply = mask;
		} else {
			uint32 mask = 0;
			if (hs->population > 0) mask |= (1 << CT_PASSENGERS);
			if (hs->mail_generation > 0) mask |= (1 << CT_MAIL);
			this->supply = mask;
		}

		/* Cache house acceptance. */
		CargoArray cargo;
		GetAcceptedCargo (&cargo, cur_house);

		stringb buf (this->acceptance);

		for (CargoID i = 0; i < NUM_CARGO; ++i) {
			if (cargo[i] == 0) continue;

			/* Add a comma between each item. */
			if (!buf.empty()) buf.append (", ");

			/* If the accepted value is less than 8, show it in 1/8s. */
			if (cargo[i] < 8) {
				SetDParam (0, cargo[i]);
				SetDParam (1, CargoSpec::Get(i)->name);
				AppendString (&buf, STR_LAND_AREA_INFORMATION_CARGO_EIGHTS);
			} else {
				AppendString (&buf, CargoSpec::Get(i)->name);
			}
		}

		if (buf.empty()) AppendString (&buf, STR_JUST_NOTHING);
	}

public:
	HousePickerWindow (const WindowDesc *desc, WindowNumber number) :
		Window (desc), houses(), sets(), sel_set (0), sel_offset (0),
		name (STR_NULL), supply (0), line_height (0)
	{
		memset (this->acceptance, 0, sizeof(this->acceptance));

		this->CreateNestedTree();
		/* there is no shade box but we will shade the window if there is no house to show */
		this->shade_select = this->GetWidget<NWidgetStacked> (WID_HP_MAIN_PANEL_SEL);
		NWidgetMatrix *matrix = this->GetWidget<NWidgetMatrix> (WID_HP_HOUSE_SELECT_MATRIX);
		matrix->SetScrollbar (this->GetScrollbar (WID_HP_HOUSE_SELECT_SCROLL));
		this->InitNested (number);

		if (cur_house != INVALID_HOUSE_ID) {
			matrix->SetClicked (this->sel_offset); // set clicked item again to make it visible
			this->SetObjectToPlace();
		} else {
			ResetPointerMode();
		}
	}

	void OnInit() OVERRIDE
	{
		/* Rebuild house list. */
		this->sets.clear();
		this->houses.clear();

		/* Collect houses. */
		for (HouseID house = 0; house < NUM_HOUSES; house++) {
			const HouseSpec *hs = HouseSpec::Get(house);

			if (!hs->enabled) continue;
			if (hs->grf_prop.override != INVALID_HOUSE_ID) continue;

			uint landscape = _settings_game.game_creation.landscape;
			uint mask = (landscape != LT_ARCTIC) ? (HZ_TEMP << landscape) :
					(HZ_SUBARTC_ABOVE | HZ_SUBARTC_BELOW);
			HouseZones availability = hs->building_availability;
			if ((availability & mask) == 0) continue;
			if ((availability & HZ_ZONALL) == 0) continue;

			this->houses.push_back (house);
		}

		if (this->houses.empty()) {
			cur_house = INVALID_HOUSE_ID;
			/* Hide widgets if we have no houses to show. */
			this->SetShaded (true);
		} else {
			std::sort (this->houses.begin(), this->houses.end());

			this->BuildSetList();

			this->SetShaded (false);

			/* Show the list of house sets if we have at least 2 items to show. */
			this->GetWidget<NWidgetStacked> (WID_HP_HOUSE_SETS_SEL)->SetDisplayedPlane (this->sets.size() > 1 ? 0 : SZSP_NONE);
			/* Set the number of items in the list of house sets. */
			this->GetWidget<NWidgetCore> (WID_HP_HOUSE_SETS)->widget_data = (this->sets.size() << MAT_ROW_START) | (1 << MAT_COL_START);
			/* Show the landscape info only in arctic climate (above/below snowline). */
			this->GetWidget<NWidgetStacked> (WID_HP_HOUSE_LANDSCAPE_SEL)->SetDisplayedPlane (_settings_game.game_creation.landscape == LT_ARCTIC ? 0 : SZSP_NONE);
			/* Update the matrix of houses. */
			NWidgetMatrix *matrix = this->GetWidget<NWidgetMatrix> (WID_HP_HOUSE_SELECT_MATRIX);
			matrix->SetCount (GetSetSize (this->sel_set));
			matrix->SetClicked (this->sel_offset);
			cur_house = this->houses[this->sets[this->sel_set] + this->sel_offset];
		}

		this->UpdateCache();

		/* If we have exactly one set of houses and it is not the default one then display its name in the title bar. */
		this->GetWidget<NWidgetCore> (WID_HP_CAPTION)->widget_data =
				(this->sets.size() == 1 && HouseSpec::Get (this->houses[0])->grf_prop.grffile != NULL) ?
				STR_BUILD_HOUSE_CUSTOM_CAPTION : STR_BUILD_HOUSE_CAPTION;
	}

	const GRFFile *GetGRFFileOfSet (uint set) const
	{
		assert (set < this->sets.size());
		return HouseSpec::Get (this->houses[this->sets[set]])->grf_prop.grffile;
	}

	static const char *GetNameOfSet (const GRFFile *grffile)
	{
		return GetGRFConfig (grffile->grfid)->GetName();
	}

	void SetStringParameters (int widget) const OVERRIDE
	{
		switch (widget) {
			case WID_HP_CAPTION:
				if (this->sets.size() == 1) {
					const GRFFile *grffile = GetGRFFileOfSet (0);
					if (grffile != NULL) {
						SetDParamStr (0, GetNameOfSet (grffile));
					}
				}
				break;

			case WID_HP_HOUSE_NAME: {
				SetDParam (0, this->name);
				break;
			}

			case WID_HP_HISTORICAL_BUILDING:
				SetDParam (0, ((cur_house != INVALID_HOUSE_ID) && (HouseSpec::Get (cur_house)->extra_flags & BUILDING_IS_HISTORICAL) != 0) ? STR_BUILD_HOUSE_HISTORICAL_BUILDING : STR_EMPTY);
				break;

			case WID_HP_HOUSE_POPULATION:
				SetDParam (0, (cur_house != INVALID_HOUSE_ID) ? HouseSpec::Get (cur_house)->population : 0);
				break;

			case WID_HP_HOUSE_ZONES: {
				HouseZones zones = (cur_house != INVALID_HOUSE_ID) ? HouseSpec::Get(cur_house)->building_availability : HZ_NOZNS;
				for (int i = 0; i < HZB_END; i++) {
					SetDParam (i, HasBit (zones, HZB_END - 1 - i) ? STR_BUILD_HOUSE_ZONE_ENABLED : STR_BUILD_HOUSE_ZONE_DISABLED);
				}
				break;
			}

			case WID_HP_HOUSE_LANDSCAPE: {
				StringID info;
				if (cur_house != INVALID_HOUSE_ID) {
					switch (HouseSpec::Get(cur_house)->building_availability & (HZ_SUBARTC_ABOVE | HZ_SUBARTC_BELOW)) {
						case HZ_SUBARTC_ABOVE: info = STR_BUILD_HOUSE_ABOVE_SNOWLINE; break;
						case HZ_SUBARTC_BELOW: info = STR_BUILD_HOUSE_BELOW_SNOWLINE; break;
						default: info = STR_BUILD_HOUSE_ABOVE_OR_BELOW_SNOWLINE; break;
					}
				} else {
					info = STR_EMPTY;
				}
				SetDParam (0, info);
				break;
			}

			case WID_HP_HOUSE_YEARS: {
				if (cur_house != INVALID_HOUSE_ID) {
					const HouseSpec *hs = HouseSpec::Get (cur_house);
					SetDParam (0, hs->min_year <= _cur_year ? STR_BUILD_HOUSE_GOOD_YEAR : STR_BUILD_HOUSE_BAD_YEAR);
					SetDParam (1, hs->min_year);
					SetDParam (2, hs->max_year >= _cur_year ? STR_BUILD_HOUSE_GOOD_YEAR : STR_BUILD_HOUSE_BAD_YEAR);
					SetDParam (3, hs->max_year);
				} else {
					SetDParam (0, STR_EMPTY);
					SetDParam (1, 0);
					SetDParam (2, STR_EMPTY);
					SetDParam (3, 0);
				}
				break;
			}

			case WID_HP_HOUSE_ACCEPTANCE:
				SetDParamStr (0, this->acceptance);
				break;

			case WID_HP_HOUSE_SUPPLY:
				SetDParam (0, this->supply);
				break;

			default: break;
		}
	}

	void UpdateWidgetSize (int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize) OVERRIDE
	{
		switch (widget) {
			case WID_HP_HOUSE_SETS: {
				uint max_w = 0;
				for (uint i = 0; i < this->sets.size(); i++) {
					const GRFFile *grffile = this->GetGRFFileOfSet (i);
					uint w = (grffile != NULL) ?
							GetStringBoundingBox (GetNameOfSet (grffile)).width :
							GetStringBoundingBox (STR_BUILD_HOUSE_ORIGINAL_SET_NAME).width;
					max_w = max (max_w, w);
				}
				size->width = max (size->width, max_w + padding.width);
				this->line_height = FONT_HEIGHT_NORMAL + WD_MATRIX_TOP + WD_MATRIX_BOTTOM;
				size->height = this->sets.size() * this->line_height;
				break;
			}

			case WID_HP_HOUSE_PREVIEW:
				size->width  = ScaleGUITrad (2 * TILE_PIXELS);
				size->height = ScaleGUITrad (142);
				break;

			case WID_HP_HOUSE_NAME:
				size->width = 120; // we do not want this window to get too wide, better clip
				break;

			case WID_HP_HISTORICAL_BUILDING:
				size->width = max (size->width, GetStringBoundingBox (STR_BUILD_HOUSE_HISTORICAL_BUILDING).width + padding.width);
				break;

			case WID_HP_HOUSE_POPULATION:
				/* Max popultion is 255 - 3 digits */
				SetDParamMaxDigits (0, 3);
				size->width = max (size->width, GetStringBoundingBox (STR_BUILD_HOUSE_POPULATION).width + padding.width);
				break;

			case WID_HP_HOUSE_ZONES: {
				for (int i = 0; i < HZB_END; i++) {
					SetDParam (2 * i, STR_BUILD_HOUSE_ZONE_ENABLED); // colour
					SetDParam (2 * i + 1, i + 1); // digit: 1(center)/2/3/4/5(edge)
				}
				size->width = max (size->width, GetStringBoundingBox (STR_BUILD_HOUSE_ZONES).width + padding.width);
				break;
			}

			case WID_HP_HOUSE_LANDSCAPE: {
				SetDParam (0, STR_BUILD_HOUSE_ABOVE_OR_BELOW_SNOWLINE);
				Dimension dim = GetStringBoundingBox (STR_BUILD_HOUSE_LANDSCAPE);
				SetDParam (0, STR_BUILD_HOUSE_ABOVE_SNOWLINE);
				dim = maxdim (dim, GetStringBoundingBox (STR_BUILD_HOUSE_LANDSCAPE));
				SetDParam (0, STR_BUILD_HOUSE_BELOW_SNOWLINE);
				dim = maxdim (dim, GetStringBoundingBox (STR_BUILD_HOUSE_LANDSCAPE));
				dim.width += padding.width;
				dim.height += padding.height;
				*size = maxdim (*size, dim);
				break;
			}

			case WID_HP_HOUSE_YEARS: {
				SetDParam (0, STR_BUILD_HOUSE_GOOD_YEAR);
				SetDParamMaxDigits (1, 8);
				SetDParam (2, STR_BUILD_HOUSE_GOOD_YEAR);
				SetDParamMaxDigits (3, 8);
				Dimension dim = GetStringBoundingBox (STR_BUILD_HOUSE_YEARS);
				dim.width  += padding.width;
				dim.height += padding.height;
				*size = maxdim (*size, dim);
				break;
			}

			case WID_HP_HOUSE_SELECT_MATRIX:
				resize->height = 1; // don't snap to rows of this matrix
				break;

			case WID_HP_HOUSE_SELECT:
				size->width  = ScaleGUITrad(64) + 2;
				size->height = ScaleGUITrad(58) + 2;
				break;

			/* these texts can be long, better clip */
			case WID_HP_HOUSE_ACCEPTANCE:
			case WID_HP_HOUSE_SUPPLY:
				size->width = 0;
				break;

			default: break;
		}
	}

	void DrawWidget (BlitArea *dpi, const Rect &r, int widget) const OVERRIDE
	{
		switch (GB(widget, 0, 16)) {
			case WID_HP_HOUSE_SETS: {
				int y = r.top + WD_MATRIX_TOP;
				for (uint i = 0; i < this->sets.size(); i++) {
					const GRFFile *grffile = this->GetGRFFileOfSet (i);
					StringID str;
					if (grffile != NULL) {
						SetDParamStr (0, GetNameOfSet (grffile));
						str = STR_JUST_RAW_STRING;
					} else {
						str = STR_BUILD_HOUSE_ORIGINAL_SET_NAME;
					}
					DrawString (dpi, r.left + WD_MATRIX_LEFT, r.right - WD_MATRIX_RIGHT, y, str, i == this->sel_set ? TC_WHITE : TC_BLACK);
					y += this->line_height;
				}
				break;
			}

			case WID_HP_HOUSE_PREVIEW:
				if (cur_house != INVALID_HOUSE_ID) {
					DrawHouseImage (cur_house, dpi, r.left, r.top, r.right, r.bottom);
				}
				break;

			case WID_HP_HOUSE_SELECT: {
				HouseID house = this->houses[this->sets[this->sel_set] + GB(widget, 16, 16)];
				DrawHouseImage (house, dpi,
						r.left  + WD_MATRIX_LEFT,  r.top    + WD_MATRIX_TOP,
						r.right - WD_MATRIX_RIGHT, r.bottom - WD_MATRIX_BOTTOM);
				break;
			}
		}
	}

	void OnClick (Point pt, int widget, int click_count) OVERRIDE
	{
		switch (GB(widget, 0, 16)) {
			case WID_HP_HOUSE_SETS: {
				uint index = (uint)(pt.y - this->GetWidget<NWidgetBase>(widget)->pos_y) / this->line_height;
				if (index < this->sets.size() && index != this->sel_set) {
					this->SetObjectToPlace();
					cur_house = this->houses[this->sets[index]];
					this->sel_set = index;
					this->sel_offset = 0;

					NWidgetMatrix *matrix = this->GetWidget<NWidgetMatrix>(WID_HP_HOUSE_SELECT_MATRIX);
					matrix->SetCount (GetSetSize (index));
					matrix->SetClicked (0);
					this->UpdateCache();
					this->SetDirty();
				}
				break;
			}

			case WID_HP_HOUSE_SELECT: {
				uint index = GB(widget, 16, 16);
				if (index != this->sel_offset) {
					this->SetObjectToPlace();
					cur_house = this->houses[this->sets[this->sel_set] + index];
					this->sel_offset = index;

					NWidgetMatrix *matrix = this->GetWidget<NWidgetMatrix>(WID_HP_HOUSE_SELECT_MATRIX);
					matrix->SetClicked (index);
					this->UpdateCache();
					this->SetDirty();
				}
				break;
			}
		}
	}

	void OnPlaceObject (Point pt, TileIndex tile) OVERRIDE;

	void OnPlaceObjectAbort (void) OVERRIDE
	{
		this->sel_offset = -1;
		this->GetWidget<NWidgetMatrix>(WID_HP_HOUSE_SELECT_MATRIX)->SetClicked (-1);
		this->SetDirty();
	}
};

static StringID CheckPlaceHouse (TileIndex tile, HouseID house, Town *town)
{
	const HouseSpec *hs = HouseSpec::Get (house);

	int z = GetTileMaxZ (tile);

	if (_settings_game.game_creation.landscape == LT_ARCTIC) {
		bool above_snowline = z > HighestSnowLine();
		HouseZones mask = above_snowline ? HZ_SUBARTC_ABOVE : HZ_SUBARTC_BELOW;
		if ((hs->building_availability & mask) == 0) {
			return above_snowline ?
				STR_ERROR_BUILDING_NOT_ALLOWED_ABOVE_SNOW_LINE :
				STR_ERROR_BUILDING_NOT_ALLOWED_BELOW_SNOW_LINE;
		}
	}

	if (town != NULL) {
		StringID err = IsNewTownHouseAllowed (town, house);
		if (err != STR_NULL) return err;
	}

	TileArea ta (tile);
	if (hs->building_flags & BUILDING_2_TILES_X) ta.w++;
	if (hs->building_flags & BUILDING_2_TILES_Y) ta.h++;

	bool noslope = (hs->building_flags & TILE_NOT_SLOPED) != 0;

	TILE_AREA_LOOP(test, ta) {
		if (noslope) {
			if (!IsTileFlat (test)) {
				return STR_ERROR_FLAT_LAND_REQUIRED;
			}
		} else {
			if (IsSteepSlope (GetTileSlope (test)) || (GetTileMaxZ (test) != z)) {
				return STR_ERROR_LAND_SLOPED_IN_WRONG_DIRECTION;
			}
		}

		if (HasBridgeAbove (test)) {
			return STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST;
		}

		CommandCost clear = DoCommand (test, 0, 0, DC_AUTO | DC_NO_WATER, CMD_LANDSCAPE_CLEAR);
		if (clear.Failed()) return clear.GetErrorMessage();
	}

	return STR_NULL;
}

void HousePickerWindow::OnPlaceObject (Point pt, TileIndex tile)
{
	HouseID house = cur_house;
	if (house == INVALID_HOUSE_ID) return;

	StringID err;
	if (!_ctrl_pressed) {
		/* Add the house to the closest town. */
		Town *town = CalcClosestTownFromTile (tile);
		if (town == NULL) {
			err = STR_ERROR_MUST_FOUND_TOWN_FIRST;
		} else {
			err = CheckPlaceHouse (tile, house, town);
			if (err == STR_NULL) {
				DoBuildHouse (town, tile, house, InteractiveRandom());
				return;
			}
		}
	} else {
		/* Show a list of towns to join. */
		if (Town::pool.items == 0) {
			err = STR_ERROR_MUST_FOUND_TOWN_FIRST;
		} else {
			err = CheckPlaceHouse (tile, house, NULL);
			if (err == STR_NULL) {
				DeleteWindowByClass (WC_SELECT_TOWN);
				new SelectTownWindow (&_select_town_desc, tile, house);
				return;
			}
		}
	}

	ShowErrorMessage (STR_ERROR_CAN_T_BUILD_HOUSE_HERE, err,
			WL_INFO, pt.x, pt.y);
}

static const NWidgetPart _nested_house_picker_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN, WID_HP_CAPTION), SetDataTip(STR_BUILD_HOUSE_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_DEFSIZEBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_SELECTION, COLOUR_DARK_GREEN, WID_HP_MAIN_PANEL_SEL),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetScrollbar(WID_HP_HOUSE_SELECT_SCROLL),
			NWidget(NWID_HORIZONTAL), SetPIP(5, 0, 0),
				NWidget(NWID_VERTICAL), SetPIP(5, 2, 2),
					/* List of house sets */
					NWidget(NWID_SELECTION, COLOUR_DARK_GREEN, WID_HP_HOUSE_SETS_SEL),
						NWidget(NWID_HORIZONTAL),
							NWidget(WWT_MATRIX, COLOUR_GREY, WID_HP_HOUSE_SETS), SetMinimalSize(0, 60), SetFill(1, 0), SetResize(0, 0),
									SetMatrixDataTip(1, 1, STR_BUILD_HOUSE_HOUSESET_LIST_TOOLTIP),
						EndContainer(),
					EndContainer(),
					/* House picture and label */
					NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_HP_HOUSE_PREVIEW), SetFill(1, 1), SetResize(0, 1), SetMinimalSize(2 * TILE_PIXELS, 142), SetPadding(5, 0, 5, 0),
					NWidget(WWT_LABEL, COLOUR_DARK_GREEN, WID_HP_HOUSE_NAME), SetDataTip(STR_BUILD_HOUSE_NAME, STR_NULL), SetMinimalSize(120, 0),
					NWidget(WWT_LABEL, COLOUR_DARK_GREEN, WID_HP_HISTORICAL_BUILDING), SetDataTip(STR_JUST_STRING, STR_NULL),
					/* House info (short) */
					NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_HP_HOUSE_POPULATION), SetDataTip(STR_BUILD_HOUSE_POPULATION, STR_NULL), SetPadding(5, 0, 0, 0),
					NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_HP_HOUSE_ZONES), SetDataTip(STR_BUILD_HOUSE_ZONES, STR_NULL),
					NWidget(NWID_SELECTION, COLOUR_DARK_GREEN, WID_HP_HOUSE_LANDSCAPE_SEL),
						NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_HP_HOUSE_LANDSCAPE), SetDataTip(STR_BUILD_HOUSE_LANDSCAPE, STR_NULL),
					EndContainer(),
					NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_HP_HOUSE_YEARS), SetDataTip(STR_BUILD_HOUSE_YEARS, STR_NULL),
				EndContainer(),
				/* House matrix */
				NWidget(NWID_MATRIX, COLOUR_DARK_GREEN, WID_HP_HOUSE_SELECT_MATRIX), SetPIP(0, 2, 0), SetPadding(2, 2, 2, 2), SetScrollbar(WID_HP_HOUSE_SELECT_SCROLL),
					NWidget(WWT_PANEL, COLOUR_DARK_GREEN, WID_HP_HOUSE_SELECT), SetMinimalSize(64, 64), SetFill(0, 0), SetResize(0, 0),
							SetDataTip(0x0, STR_BUILD_HOUSE_SELECT_HOUSE_TOOLTIP), SetScrollbar(WID_HP_HOUSE_SELECT_SCROLL),
					EndContainer(),
				EndContainer(),
				NWidget(NWID_VSCROLLBAR, COLOUR_DARK_GREEN, WID_HP_HOUSE_SELECT_SCROLL),
			EndContainer(),
			NWidget(NWID_HORIZONTAL), SetPIP(5, 2, 0),
				/* House info (long) */
				NWidget(NWID_VERTICAL), SetPIP(0, 2, 5),
					NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_HP_HOUSE_ACCEPTANCE), SetDataTip(STR_BUILD_HOUSE_ACCEPTED_CARGO, STR_NULL), SetFill(1, 0), SetResize(1, 0),
					NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_HP_HOUSE_SUPPLY), SetDataTip(STR_BUILD_HOUSE_SUPPLIED_CARGO, STR_NULL), SetFill(1, 0), SetResize(1, 0),
				EndContainer(),
				/* Resize box */
				NWidget(NWID_VERTICAL),
					NWidget(NWID_SPACER), SetFill(0, 1),
					NWidget(WWT_RESIZEBOX, COLOUR_DARK_GREEN),
				EndContainer(),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

static WindowDesc::Prefs _house_picker_prefs ("build_house");

static const WindowDesc _house_picker_desc(
	WDP_AUTO, 0, 0,
	WC_BUILD_HOUSE, WC_BUILD_TOOLBAR,
	WDF_CONSTRUCTION,
	_nested_house_picker_widgets, lengthof(_nested_house_picker_widgets),
	&_house_picker_prefs
);

HouseID HousePickerWindow::cur_house = INVALID_HOUSE_ID;

/**
 * Show our house picker.
 * @param parent The toolbar window we're associated with.
 */
void ShowBuildHousePicker (void)
{
	AllocateWindowDescFront<HousePickerWindow> (&_house_picker_desc, 0);
}
