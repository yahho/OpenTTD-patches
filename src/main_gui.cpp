/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file main_gui.cpp Handling of the main viewport. */

#include "stdafx.h"
#include "currency.h"
#include "spritecache.h"
#include "window_gui.h"
#include "window_func.h"
#include "textbuf_gui.h"
#include "viewport_func.h"
#include "command_func.h"
#include "console_gui.h"
#include "progress.h"
#include "transparency_gui.h"
#include "sound_func.h"
#include "transparency.h"
#include "strings_func.h"
#include "zoom_func.h"
#include "company_base.h"
#include "company_func.h"
#include "toolbar_gui.h"
#include "statusbar_gui.h"
#include "linkgraph/linkgraph_gui.h"
#include "tilehighlight_func.h"
#include "hotkeys.h"

#include "saveload/saveload.h"

#include "widgets/main_widget.h"

#include "network/network.h"
#include "network/network_func.h"
#include "network/network_gui.h"
#include "network/network_base.h"

#include "table/sprites.h"
#include "table/strings.h"

static int _rename_id = 1;
static int _rename_what = -1;

void CcGiveMoney(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
#ifdef ENABLE_NETWORK
	if (result.Failed() || !_settings_game.economy.give_money) return;

	/* Inform the company of the action of one of its clients (controllers). */
	char msg[64];
	SetDParam(0, p2);
	GetString (msg, STR_COMPANY_NAME);

	if (!_network_server) {
		NetworkClientSendChat(NETWORK_ACTION_GIVE_MONEY, DESTTYPE_TEAM, p2, msg, p1);
	} else {
		NetworkServerSendChat(NETWORK_ACTION_GIVE_MONEY, DESTTYPE_TEAM, p2, msg, CLIENT_ID_SERVER, p1);
	}
#endif /* ENABLE_NETWORK */
}

void HandleOnEditText(const char *str)
{
	switch (_rename_what) {
#ifdef ENABLE_NETWORK
	case 3: { // Give money, you can only give money in excess of loan
		const Company *c = Company::GetIfValid(_local_company);
		if (c == NULL) break;
		Money money = min(c->money - c->current_loan, (Money)(atoi(str) / _currency->rate));

		uint32 money_c = Clamp(ClampToI32(money), 0, 20000000); // Clamp between 20 million and 0

		/* Give 'id' the money, and subtract it from ourself */
		DoCommandP(0, money_c, _rename_id, CMD_GIVE_MONEY, str);
		break;
	}
#endif /* ENABLE_NETWORK */
		default: NOT_REACHED();
	}

	_rename_id = _rename_what = -1;
}

/**
 * This code is shared for the majority of the pushbuttons.
 * Handles e.g. the pressing of a button (to build things), playing of click sound and sets certain parameters
 *
 * @param w Window which called the function
 * @param widget ID of the widget (=button) that called this function
 * @param cursor How should the cursor image change? E.g. cursor with depot image in it
 * @return true if the button is clicked, false if it's unclicked
 * @param mode Pointer mode to set.
 */
bool HandlePlacePushButton (Window *w, int widget, CursorID cursor, PointerMode mode)
{
	if (w->IsWidgetDisabled(widget)) return false;

	if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
	w->SetDirty();

	if (w->IsWidgetLowered(widget)) {
		ResetPointerMode();
		return false;
	}

	SetPointerMode (mode, w->window_class, w->window_number, cursor);
	w->LowerWidget(widget);
	return true;
}


void CcPlaySound_EXPLOSION(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2)
{
	if (result.Succeeded() && _settings_client.sound.confirm) SndPlayTileFx(SND_12_EXPLOSION, tile);
}

#ifdef ENABLE_NETWORK
void ShowNetworkGiveMoneyWindow(CompanyID company)
{
	_rename_id = company;
	_rename_what = 3;
	ShowQueryString(STR_EMPTY, STR_NETWORK_GIVE_MONEY_CAPTION, 30, NULL, CS_NUMERAL, QSF_NONE);
}
#endif /* ENABLE_NETWORK */


/**
 * Zoom a viewport in or out.
 * @param vp  Viewport to zoom.
 * @param in  Whether to zoom in, else out.
 */
void DoZoomInOutViewport (ViewportData *vp, bool in)
{
	if (in) {
		vp->zoom = (ZoomLevel)((int)vp->zoom - 1);
		vp->virtual_width >>= 1;
		vp->virtual_height >>= 1;

		vp->scrollpos_x += vp->virtual_width >> 1;
		vp->scrollpos_y += vp->virtual_height >> 1;
	} else {
		vp->zoom = (ZoomLevel)((int)vp->zoom + 1);

		vp->scrollpos_x -= vp->virtual_width >> 1;
		vp->scrollpos_y -= vp->virtual_height >> 1;

		vp->virtual_width <<= 1;
		vp->virtual_height <<= 1;
	}

	vp->dest_scrollpos_x = vp->scrollpos_x;
	vp->dest_scrollpos_y = vp->scrollpos_y;

	vp->follow_vehicle = INVALID_VEHICLE;

	vp->virtual_left = vp->scrollpos_x;
	vp->virtual_top  = vp->scrollpos_y;
}

void ClampViewportZoom (ViewportData *vp)
{
	if (vp->zoom < _settings_client.gui.zoom_min) {
		do DoZoomInOutViewport (vp, false);
		while (vp->zoom < _settings_client.gui.zoom_min);
	} else if (vp->zoom > _settings_client.gui.zoom_max) {
		do DoZoomInOutViewport (vp, true);
		while (vp->zoom > _settings_client.gui.zoom_max);
	}
}

static const struct NWidgetPart _nested_main_window_widgets[] = {
	NWidget(NWID_VIEWPORT, INVALID_COLOUR, WID_M_VIEWPORT), SetResize(1, 1),
};

enum {
	GHK_QUIT,
	GHK_ABANDON,
	GHK_CONSOLE,
	GHK_BOUNDING_BOXES,
	GHK_DIRTY_BLOCKS,
	GHK_CENTER,
	GHK_CENTER_ZOOM,
	GHK_RESET_OBJECT_TO_PLACE,
	GHK_DELETE_WINDOWS,
	GHK_DELETE_NONVITAL_WINDOWS,
	GHK_REFRESH_SCREEN,
	GHK_CRASH,
	GHK_MONEY,
	GHK_UPDATE_COORDS,
	GHK_TOGGLE_TRANSPARENCY,
	GHK_TOGGLE_INVISIBILITY = GHK_TOGGLE_TRANSPARENCY + 9,
	GHK_TRANSPARENCY_TOOLBAR = GHK_TOGGLE_INVISIBILITY + 8,
	GHK_TRANSPARANCY,
	GHK_CHAT,
	GHK_CHAT_ALL,
	GHK_CHAT_COMPANY,
	GHK_CHAT_SERVER,
};

struct MainWindow : Window
{
	LinkGraphOverlay overlay;
	uint refresh;

	static const uint LINKGRAPH_REFRESH_PERIOD = 0xff;
	static const uint LINKGRAPH_DELAY = 0xf;

	MainWindow (const WindowDesc *desc) : Window (desc),
		overlay (this, WID_M_VIEWPORT, 0, 0, 3), refresh (0)
	{
		this->InitNested(0);
		CLRBITS(this->flags, WF_WHITE_BORDER);
		ResizeWindow (this, _screen_width, _screen_height);

		NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_M_VIEWPORT);
		nvp->InitializeViewport(this, TileXY(32, 32), ZOOM_LVL_VIEWPORT);

		this->viewport->overlay = &this->overlay;
		this->refresh = LINKGRAPH_DELAY;
	}

	virtual void OnTick()
	{
		if (--this->refresh > 0) return;

		this->refresh = LINKGRAPH_REFRESH_PERIOD;

		if (this->overlay.GetCargoMask() == 0 ||
				this->overlay.GetCompanyMask() == 0) {
			return;
		}

		this->overlay.RebuildCache();
		this->GetWidget<NWidgetBase>(WID_M_VIEWPORT)->SetDirty(this);
	}

	void OnPaint (BlitArea *dpi) OVERRIDE
	{
		this->DrawWidgets (dpi);
		if (_game_mode == GM_MENU) {
			static const SpriteID title_sprites[] = {SPR_OTTD_O, SPR_OTTD_P, SPR_OTTD_E, SPR_OTTD_N, SPR_OTTD_T, SPR_OTTD_T, SPR_OTTD_D};
			static const uint LETTER_SPACING = 10;
			int name_width = (lengthof(title_sprites) - 1) * LETTER_SPACING;

			for (uint i = 0; i < lengthof(title_sprites); i++) {
				name_width += GetSpriteSize(title_sprites[i]).width;
			}
			int off_x = (this->width - name_width) / 2;

			for (uint i = 0; i < lengthof(title_sprites); i++) {
				DrawSprite (dpi, title_sprites[i], PAL_NONE, off_x, 50);
				off_x += GetSpriteSize(title_sprites[i]).width + LETTER_SPACING;
			}
		}
	}

	virtual EventState OnHotkey(int hotkey)
	{
		if (hotkey == GHK_QUIT) {
			HandleExitGameRequest();
			return ES_HANDLED;
		}

		/* Disable all key shortcuts, except quit shortcuts when
		 * generating the world, otherwise they create threading
		 * problem during the generating, resulting in random
		 * assertions that are hard to trigger and debug */
		if (HasModalProgress()) return ES_NOT_HANDLED;

		switch (hotkey) {
			case GHK_ABANDON:
				/* No point returning from the main menu to itself */
				if (_game_mode == GM_MENU) return ES_HANDLED;
				if (_settings_client.gui.autosave_on_exit) {
					DoExitSave();
					_switch_mode = SM_MENU;
				} else {
					AskExitToGameMenu();
				}
				return ES_HANDLED;

			case GHK_CONSOLE:
				IConsoleSwitch();
				return ES_HANDLED;

			case GHK_BOUNDING_BOXES:
				ToggleBoundingBoxes();
				return ES_HANDLED;

			case GHK_DIRTY_BLOCKS:
				ToggleDirtyBlocks();
				return ES_HANDLED;
		}

		if (_game_mode == GM_MENU) return ES_NOT_HANDLED;

		switch (hotkey) {
			case GHK_CENTER:
			case GHK_CENTER_ZOOM: {
				Point pt = GetTileBelowCursor();
				if (pt.x != -1) {
					bool instant = (hotkey == GHK_CENTER_ZOOM && this->viewport->zoom != _settings_client.gui.zoom_min);
					if (instant) {
						do DoZoomInOutViewport (this->viewport, true);
						while (this->viewport->zoom > _settings_client.gui.zoom_min);
						this->InvalidateData();
					}
					ScrollMainWindowTo(pt.x, pt.y, -1, instant);
				}
				break;
			}

			case GHK_RESET_OBJECT_TO_PLACE: ResetPointerMode(); break;
			case GHK_DELETE_WINDOWS: DeleteNonVitalWindows(); break;
			case GHK_DELETE_NONVITAL_WINDOWS: DeleteAllNonVitalWindows(); break;
			case GHK_REFRESH_SCREEN: MarkWholeScreenDirty(); break;

			case GHK_CRASH: // Crash the game
				*(volatile byte *)0 = 0;
				break;

			case GHK_MONEY: // Gimme money
				/* You can only cheat for money in single player. */
				if (!_networking) DoCommandP(0, 10000000, 0, CMD_MONEY_CHEAT);
				break;

			case GHK_UPDATE_COORDS: // Update the coordinates of all station signs
				UpdateAllVirtCoords();
				break;

			case GHK_TOGGLE_TRANSPARENCY:
			case GHK_TOGGLE_TRANSPARENCY + 1:
			case GHK_TOGGLE_TRANSPARENCY + 2:
			case GHK_TOGGLE_TRANSPARENCY + 3:
			case GHK_TOGGLE_TRANSPARENCY + 4:
			case GHK_TOGGLE_TRANSPARENCY + 5:
			case GHK_TOGGLE_TRANSPARENCY + 6:
			case GHK_TOGGLE_TRANSPARENCY + 7:
			case GHK_TOGGLE_TRANSPARENCY + 8:
				/* Transparency toggle hot keys */
				ToggleTransparency((TransparencyOption)(hotkey - GHK_TOGGLE_TRANSPARENCY));
				MarkWholeScreenDirty();
				break;

			case GHK_TOGGLE_INVISIBILITY:
			case GHK_TOGGLE_INVISIBILITY + 1:
			case GHK_TOGGLE_INVISIBILITY + 2:
			case GHK_TOGGLE_INVISIBILITY + 3:
			case GHK_TOGGLE_INVISIBILITY + 4:
			case GHK_TOGGLE_INVISIBILITY + 5:
			case GHK_TOGGLE_INVISIBILITY + 6:
			case GHK_TOGGLE_INVISIBILITY + 7:
				/* Invisibility toggle hot keys */
				ToggleInvisibilityWithTransparency((TransparencyOption)(hotkey - GHK_TOGGLE_INVISIBILITY));
				MarkWholeScreenDirty();
				break;

			case GHK_TRANSPARENCY_TOOLBAR:
				ShowTransparencyToolbar();
				break;

			case GHK_TRANSPARANCY:
				ResetRestoreAllTransparency();
				break;

#ifdef ENABLE_NETWORK
			case GHK_CHAT: // smart chat; send to team if any, otherwise to all
				if (_networking) {
					const NetworkClientInfo *cio = NetworkClientInfo::GetByClientID(_network_own_client_id);
					if (cio == NULL) break;

					ShowNetworkChatQueryWindow(NetworkClientPreferTeamChat(cio) ? DESTTYPE_TEAM : DESTTYPE_BROADCAST, cio->client_playas);
				}
				break;

			case GHK_CHAT_ALL: // send text message to all clients
				if (_networking) ShowNetworkChatQueryWindow(DESTTYPE_BROADCAST, 0);
				break;

			case GHK_CHAT_COMPANY: // send text to all team mates
				if (_networking) {
					const NetworkClientInfo *cio = NetworkClientInfo::GetByClientID(_network_own_client_id);
					if (cio == NULL) break;

					ShowNetworkChatQueryWindow(DESTTYPE_TEAM, cio->client_playas);
				}
				break;

			case GHK_CHAT_SERVER: // send text to the server
				if (_networking && !_network_server) {
					ShowNetworkChatQueryWindow(DESTTYPE_CLIENT, CLIENT_ID_SERVER);
				}
				break;
#endif

			default: return ES_NOT_HANDLED;
		}
		return ES_HANDLED;
	}

	virtual void OnScroll(Point delta)
	{
		this->viewport->scrollpos_x += ScaleByZoom(delta.x, this->viewport->zoom);
		this->viewport->scrollpos_y += ScaleByZoom(delta.y, this->viewport->zoom);
		this->viewport->dest_scrollpos_x = this->viewport->scrollpos_x;
		this->viewport->dest_scrollpos_y = this->viewport->scrollpos_y;
		this->refresh = LINKGRAPH_DELAY;
	}

	virtual void OnMouseWheel(int wheel)
	{
		if (_settings_client.gui.scrollwheel_scrolling == 0) {
			ZoomInOrOutToCursorWindow(wheel < 0, this);
		}
	}

	virtual void OnResize()
	{
		if (this->viewport != NULL) {
			NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_M_VIEWPORT);
			nvp->UpdateViewportCoordinates(this);
			this->refresh = LINKGRAPH_DELAY;
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
		/* Forward the message to the appropriate toolbar (ingame or scenario editor) */
		InvalidateWindowData(WC_MAIN_TOOLBAR, 0, data, true);
	}

	static HotkeyList hotkeys;
};

static const Hotkey global_hotkeys[] = {
	Hotkey ("quit",         GHK_QUIT,    'Q' | WKC_CTRL, 'Q' | WKC_META),
	Hotkey ("abandon",      GHK_ABANDON, 'W' | WKC_CTRL, 'W' | WKC_META),
	Hotkey ("console",                 GHK_CONSOLE,        WKC_BACKQUOTE),
	Hotkey ("bounding_boxes",          GHK_BOUNDING_BOXES, 'B' | WKC_CTRL),
	Hotkey ("dirty_blocks",            GHK_DIRTY_BLOCKS,   'I' | WKC_CTRL),
	Hotkey ("center",                  GHK_CENTER,         'C'),
	Hotkey ("center_zoom",             GHK_CENTER_ZOOM,    'Z'),
	Hotkey ("reset_object_to_place",   GHK_RESET_OBJECT_TO_PLACE,   WKC_ESC),
	Hotkey ("delete_windows",          GHK_DELETE_WINDOWS,          WKC_DELETE),
	Hotkey ("delete_all_windows",      GHK_DELETE_NONVITAL_WINDOWS, WKC_DELETE | WKC_SHIFT),
	Hotkey ("refresh_screen",          GHK_REFRESH_SCREEN, 'R' | WKC_CTRL),
#if defined(_DEBUG)
	Hotkey ("crash_game",              GHK_CRASH,          '0' | WKC_ALT),
	Hotkey ("money",                   GHK_MONEY,          '1' | WKC_ALT),
	Hotkey ("update_coordinates",      GHK_UPDATE_COORDS,  '2' | WKC_ALT),
#endif
	Hotkey ("transparency_signs",      GHK_TOGGLE_TRANSPARENCY,     '1' | WKC_CTRL),
	Hotkey ("transparency_trees",      GHK_TOGGLE_TRANSPARENCY + 1, '2' | WKC_CTRL),
	Hotkey ("transparency_houses",     GHK_TOGGLE_TRANSPARENCY + 2, '3' | WKC_CTRL),
	Hotkey ("transparency_industries", GHK_TOGGLE_TRANSPARENCY + 3, '4' | WKC_CTRL),
	Hotkey ("transparency_buildings",  GHK_TOGGLE_TRANSPARENCY + 4, '5' | WKC_CTRL),
	Hotkey ("transparency_bridges",    GHK_TOGGLE_TRANSPARENCY + 5, '6' | WKC_CTRL),
	Hotkey ("transparency_structures", GHK_TOGGLE_TRANSPARENCY + 6, '7' | WKC_CTRL),
	Hotkey ("transparency_catenary",   GHK_TOGGLE_TRANSPARENCY + 7, '8' | WKC_CTRL),
	Hotkey ("transparency_loading",    GHK_TOGGLE_TRANSPARENCY + 8, '9' | WKC_CTRL),
	Hotkey ("invisibility_signs",      GHK_TOGGLE_INVISIBILITY,     '1' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("invisibility_trees",      GHK_TOGGLE_INVISIBILITY + 1, '2' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("invisibility_houses",     GHK_TOGGLE_INVISIBILITY + 2, '3' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("invisibility_industries", GHK_TOGGLE_INVISIBILITY + 3, '4' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("invisibility_buildings",  GHK_TOGGLE_INVISIBILITY + 4, '5' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("invisibility_bridges",    GHK_TOGGLE_INVISIBILITY + 5, '6' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("invisibility_structures", GHK_TOGGLE_INVISIBILITY + 6, '7' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("invisibility_catenary",   GHK_TOGGLE_INVISIBILITY + 7, '8' | WKC_CTRL | WKC_SHIFT),
	Hotkey ("transparency_toolbar",    GHK_TRANSPARENCY_TOOLBAR,    'X' | WKC_CTRL),
	Hotkey ("toggle_transparency",     GHK_TRANSPARANCY,            'X'),
#ifdef ENABLE_NETWORK
	Hotkey ("chat",         GHK_CHAT,         WKC_RETURN, 'T'),
	Hotkey ("chat_all",     GHK_CHAT_ALL,     WKC_RETURN | WKC_SHIFT, 'T' | WKC_SHIFT),
	Hotkey ("chat_company", GHK_CHAT_COMPANY, WKC_RETURN | WKC_CTRL,  'T' | WKC_CTRL),
	Hotkey ("chat_server",  GHK_CHAT_SERVER,  WKC_RETURN | WKC_CTRL | WKC_SHIFT, 'T' | WKC_CTRL | WKC_SHIFT),
#endif
};
HotkeyList MainWindow::hotkeys("global", global_hotkeys);

static const WindowDesc _main_window_desc(
	WDP_MANUAL, 0, 0,
	WC_MAIN_WINDOW, WC_NONE,
	0,
	_nested_main_window_widgets, lengthof(_nested_main_window_widgets),
	NULL, &MainWindow::hotkeys
);

/**
 * Does the given keycode match one of the keycodes bound to 'quit game'?
 * @param keycode The keycode that was pressed by the user.
 * @return True iff the keycode matches one of the hotkeys for 'quit'.
 */
bool IsQuitKey(uint16 keycode)
{
	int num = MainWindow::hotkeys.CheckMatch(keycode);
	return num == GHK_QUIT;
}


void ShowSelectGameWindow();

/**
 * Initialise the default colours (remaps and the likes), and load the main windows.
 */
void SetupColoursAndInitialWindow()
{
	for (uint i = 0; i != 16; i++) {
		const byte *b = GetNonSprite(PALETTE_RECOLOUR_START + i, ST_RECOLOUR);

		assert(b);
		memcpy(_colour_gradient[i], b + 0xC6, sizeof(_colour_gradient[i]));
	}

	new MainWindow(&_main_window_desc);

	/* XXX: these are not done */
	switch (_game_mode) {
		default: NOT_REACHED();
		case GM_MENU:
			ShowSelectGameWindow();
			break;

		case GM_NORMAL:
		case GM_EDITOR:
			ShowVitalWindows();
			break;
	}
}

/**
 * Show the vital in-game windows.
 */
void ShowVitalWindows()
{
	AllocateToolbar();

	/* Status bad only for normal games */
	if (_game_mode == GM_EDITOR) return;

	ShowStatusBar();
}

/**
 * Size of the application screen changed.
 * Adapt the game screen-size, re-allocate the open windows, and repaint everything
 */
void GameSizeChanged()
{
	_cur_resolution.width  = _screen_width;
	_cur_resolution.height = _screen_height;
	ScreenSizeChanged();
	RelocateAllWindows (_screen_width, _screen_height);
	MarkWholeScreenDirty();
}
