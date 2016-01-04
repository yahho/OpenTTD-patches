/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport_type.h Types related to viewports. */

#ifndef VIEWPORT_TYPE_H
#define VIEWPORT_TYPE_H

#include "zoom_type.h"
#include "strings_type.h"
#include "table/strings.h"

class LinkGraphOverlay;

/**
 * Data structure for viewport, display of a part of the world
 */
struct ViewPort {
	int left;    ///< Screen coordinate left egde of the viewport
	int top;     ///< Screen coordinate top edge of the viewport
	int width;   ///< Screen width of the viewport
	int height;  ///< Screen height of the viewport

	int virtual_left;    ///< Virtual left coordinate
	int virtual_top;     ///< Virtual top coordinate
	int virtual_width;   ///< width << zoom
	int virtual_height;  ///< height << zoom

	ZoomLevel zoom; ///< The zoom level of the viewport.
	LinkGraphOverlay *overlay;
};

/** Margins for the viewport sign */
enum ViewportSignMargin {
	VPSM_LEFT   = 1, ///< Left margin
	VPSM_RIGHT  = 1, ///< Right margin
	VPSM_TOP    = 1, ///< Top margin
	VPSM_BOTTOM = 1, ///< Bottom margin
};

/** Location information about a sign as seen on the viewport */
struct ViewportSign {
	int32 center;        ///< The center position of the sign
	int32 top;           ///< The top of the sign
	uint16 width_normal; ///< The width when not zoomed out (normal font)
	uint16 width_small;  ///< The width when zoomed out (small font)

	void UpdatePosition(int center, int top, StringID str, StringID str_small = STR_NULL);
	void MarkDirty(ZoomLevel maxzoom = ZOOM_LVL_MAX) const;
};

/**
 * Some values for constructing bounding boxes (BB). The Z positions under bridges are:
 * z=0..5  Everything that can be built under low bridges.
 * z=6     reserved, currently unused.
 * z=7     Z separator between bridge/tunnel and the things under/above it.
 */
static const uint BB_HEIGHT_UNDER_BRIDGE = 6; ///< Everything that can be built under low bridges, must not exceed this Z height.
static const uint BB_Z_SEPARATOR         = 7; ///< Separates the bridge/tunnel from the things under/above it.

/** Viewport place method (type of highlighted area and placed objects) */
enum ViewportPlaceMethod {
	VPM_NONE            =    0, ///< no selection currently in progress
	VPM_X_OR_Y          =    1, ///< drag in X or Y direction
	VPM_FIX_X           =    2, ///< drag only in X axis
	VPM_FIX_Y           =    3, ///< drag only in Y axis
	VPM_X_AND_Y         =    4, ///< area of land in X and Y directions
	VPM_X_AND_Y_ROTATED =    5, ///< area of land, allow rotation
	VPM_X_AND_Y_LIMITED =    6, ///< area of land of limited size
	VPM_FIX_HORIZONTAL  =    7, ///< drag only in horizontal direction
	VPM_FIX_VERTICAL    =    8, ///< drag only in vertical direction
	VPM_X_LIMITED       =    9, ///< Drag only in X axis with limited size
	VPM_Y_LIMITED       =   10, ///< Drag only in Y axis with limited size
	VPM_RAILDIRS        = 0x40, ///< all rail directions
	VPM_SIGNALDIRS      = 0x80, ///< similar to VMP_RAILDIRS, but with different cursor
};
DECLARE_ENUM_AS_BIT_SET(ViewportPlaceMethod)

#endif /* VIEWPORT_TYPE_H */
