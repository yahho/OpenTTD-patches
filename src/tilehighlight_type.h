/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tilehighlight_type.h Types related to highlighting tiles. */

#ifndef TILEHIGHLIGHT_TYPE_H
#define TILEHIGHLIGHT_TYPE_H

#include "core/geometry_type.hpp"
#include "map/coord.h"
#include "track_type.h"
#include "window_type.h"
#include "viewport_type.h"
#include "town.h"

/** Highlighting draw styles */
enum HighLightStyle {
	HT_NONE,       ///< default
	HT_RECT,       ///< rectangle (stations, depots, ...)
	HT_POINT,      ///< point (lower land, raise land, level land, ...)
	HT_RAIL = 0x8, ///< used for autorail highlighting, lower bits: direction
	HT_RAIL_X  = HT_RAIL | TRACK_X,     ///< X direction
	HT_RAIL_Y  = HT_RAIL | TRACK_Y,     ///< Y direction
	HT_RAIL_HU = HT_RAIL | TRACK_UPPER, ///< horizontal upper
	HT_RAIL_HL = HT_RAIL | TRACK_LOWER, ///< horizontal lower
	HT_RAIL_VL = HT_RAIL | TRACK_LEFT,  ///< vertical left
	HT_RAIL_VR = HT_RAIL | TRACK_RIGHT, ///< vertical right
	HT_TRACK_MASK = 0x7, ///< masks the drag-direction
};
DECLARE_ENUM_AS_BIT_SET(HighLightStyle)


/** Metadata about the current highlighting. */
struct TileHighlightData {
	Point pos;           ///< Location, in tile "units", of the northern tile of the selected area.
	Point size;          ///< Size, in tile "units", of the white/red selection area.
	Point offs;          ///< Offset, in tile "units", for the blue coverage area from the selected area's northern tile.
	Point outersize;     ///< Size, in tile "units", of the blue coverage area excluding the side of the selected area.
	bool diagonal;       ///< Whether the dragged area is a 45 degrees rotated rectangle.

	Point new_pos;       ///< New value for \a pos; used to determine whether to redraw the selection.
	Point new_size;      ///< New value for \a size; used to determine whether to redraw the selection.
	Point new_outersize; ///< New value for \a outersize; used to determine whether to redraw the selection.
	byte dirty;          ///< Whether the build station window needs to redraw due to the changed selection.

	Point selstart;      ///< The location where the dragging started.
	Point selend;        ///< The location where the drag currently ends.
	byte sizelimit;      ///< Whether the selection is limited in length, and what the maximum length is.

	HighLightStyle drawstyle;      ///< Lower bits 0-3 are reserved for detailed highlight information.
	HighLightStyle next_drawstyle; ///< Queued, but not yet drawn style.

	WindowClass window_class;      ///< The \c WindowClass of the window that is responsible for the selection mode.
	WindowNumber window_number;    ///< The \c WindowNumber of the window that is responsible for the selection mode.

	bool make_square_red;          ///< Whether to give a tile a red selection.
	TileIndex redsq;               ///< The tile that has to get a red selection.

	ViewportPlaceMethod select_method; ///< The method which governs how tiles are selected.
	int                 select_data;   ///< Custom data set by the function that started the selection.

	void Reset();

	bool IsDraggingDiagonal();
	Window *GetCallbackWnd();

	TownID town;         ///< Town area to highlight
};

#endif /* TILEHIGHLIGHT_TYPE_H */
