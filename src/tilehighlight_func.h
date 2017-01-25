/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tilehighlight_func.h Functions related to tile highlights. */

#ifndef TILEHIGHLIGHT_FUNC_H
#define TILEHIGHLIGHT_FUNC_H

#include "gfx_type.h"
#include "window_gui.h"
#include "tilehighlight_type.h"

void SetPointerMode (PointerMode mode, WindowClass window_class,
	WindowNumber window_num, CursorID icon);

/**
 * Change the cursor and mouse click/drag handling to a mode for performing special operations like tile area selection, object placement, etc.
 * @param mode Mode to perform.
 * @param w %Window requesting the mode change.
 * @param icon New shape of the mouse cursor.
 */
static inline void SetPointerMode (PointerMode mode, Window *w, CursorID icon)
{
	SetPointerMode (mode, w->window_class, w->window_number, icon);
}

void ResetPointerMode (void);

bool HandlePlacePushButton (Window *w, int widget, CursorID cursor, PointerMode mode);

void HandleDemolishMouseUp (TileIndex start_tile, TileIndex end_tile);

void VpStartPlaceSizing (TileIndex tile, ViewportPlaceMethod method,
	int userdata = 0, uint limit = 0);
void VpSetPlaceSizingLimit (uint limit);
void VpStopPlaceSizing (void);

void UpdateTileSelection();

extern TileHighlightData _thd;

#endif /* TILEHIGHLIGHT_FUNC_H */
