/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file viewport.cpp Handling of all viewports.
 *
 * \verbatim
 * The in-game coordinate system looks like this *
 *                                               *
 *                    ^ Z                        *
 *                    |                          *
 *                    |                          *
 *                    |                          *
 *                    |                          *
 *                 /     \                       *
 *              /           \                    *
 *           /                 \                 *
 *        /                       \              *
 *   X <                             > Y         *
 * \endverbatim
 */

#include "stdafx.h"

#include <vector>

#include "debug.h"
#include "cpu.h"
#include "map/zoneheight.h"
#include "map/slope.h"
#include "map/bridge.h"
#include "landscape.h"
#include "viewport_func.h"
#include "station_base.h"
#include "waypoint_base.h"
#include "town.h"
#include "signs_base.h"
#include "signs_func.h"
#include "vehicle_base.h"
#include "vehicle_gui.h"
#include "blitter/blitter.h"
#include "strings_func.h"
#include "zoom_func.h"
#include "vehicle_func.h"
#include "company_func.h"
#include "waypoint_func.h"
#include "window_func.h"
#include "tilehighlight_func.h"
#include "window_gui.h"
#include "linkgraph/linkgraph_gui.h"
#include "clear_func.h"
#include "station_func.h"
#include "viewport_sprite_sorter.h"
#include "spritecache.h"

#include "table/strings.h"
#include "table/string_colours.h"

Point _tile_fract_coords;

static const int MAX_BUILDING_HEIGHT = 25; ///< Maximum height of a building in tile heights

static const int MAX_TILE_EXTENT_LEFT   = ZOOM_LVL_BASE * TILE_PIXELS;                       ///< Maximum left   extent of tile relative to north corner.
static const int MAX_TILE_EXTENT_RIGHT  = ZOOM_LVL_BASE * TILE_PIXELS;                       ///< Maximum right  extent of tile relative to north corner.
static const int MAX_TILE_EXTENT_TOP    = ZOOM_LVL_BASE * MAX_BUILDING_HEIGHT * TILE_HEIGHT; ///< Maximum top    extent of tile relative to north corner (not considering bridges).
static const int MAX_TILE_EXTENT_BOTTOM = ZOOM_LVL_BASE * (TILE_PIXELS + 2 * TILE_HEIGHT);   ///< Maximum bottom extent of tile relative to north corner (worst case: #SLOPE_STEEP_N).

struct TileSpriteToDraw {
	SpriteID image;
	PaletteID pal;
	const SubSprite *sub;           ///< only draw a rectangular part of the sprite
	int32 x;                        ///< screen X coordinate of sprite
	int32 y;                        ///< screen Y coordinate of sprite
};

struct ChildScreenSpriteToDraw {
	SpriteID image;
	PaletteID pal;
	const SubSprite *sub;           ///< only draw a rectangular part of the sprite
	int32 x;
	int32 y;
	int next;                       ///< next child to draw (-1 at the end)
};

/** Enumeration of multi-part foundations */
enum FoundationPart {
	FOUNDATION_PART_NORMAL   = 0,     ///< First part (normal foundation or no foundation)
	FOUNDATION_PART_HALFTILE = 1,     ///< Second part (halftile foundation)
	FOUNDATION_PART_END
};

/** Foundation part data */
struct FoundationData {
	Point offset;    ///< Pixel offset for ground sprites on the foundations.
	int *last_child; ///< Tail of ChildSprite list of the foundations. (index into child_screen_sprites_to_draw)
	int index;       ///< Foundation sprites (index into parent_sprites_to_draw).
};

/**
 * Mode of "sprite combining"
 * @see StartSpriteCombine
 */
enum SpriteCombineMode {
	SPRITE_COMBINE_NONE,     ///< Every #AddSortableSpriteToDraw start its own bounding box
	SPRITE_COMBINE_PENDING,  ///< %Sprite combining will start with the next unclipped sprite.
	SPRITE_COMBINE_ACTIVE,   ///< %Sprite combining is active. #AddSortableSpriteToDraw outputs child sprites.
};

typedef SmallVector<TileSpriteToDraw, 64> TileSpriteToDrawVector;
typedef SmallVector<ParentSpriteToDraw, 64> ParentSpriteToDrawVector;
typedef SmallVector<ChildScreenSpriteToDraw, 16> ChildScreenSpriteToDrawVector;

/** Data structure storing rendering information */
struct ViewportDrawer {
	DrawPixelInfo dpi;

	TileSpriteToDrawVector tile_sprites_to_draw;
	ParentSpriteToDrawVector parent_sprites_to_draw;
	ChildScreenSpriteToDrawVector child_screen_sprites_to_draw;

	int *last_child;

	SpriteCombineMode combine_sprites;               ///< Current mode of "sprite combining". @see StartSpriteCombine

	FoundationData foundation[FOUNDATION_PART_END];  ///< Foundation data.
	FoundationData *foundation_part;                 ///< Currently active foundation for ground sprite drawing.
};

bool IsViewportDrawerDetailed (const ViewportDrawer *vd)
{
	return vd->dpi.zoom <= ZOOM_LVL_DETAIL;
}

static void MarkViewportDirty(const ViewPort *vp, int left, int top, int right, int bottom);

TileHighlightData _thd;
bool _draw_bounding_boxes = false;
bool _draw_dirty_blocks = false;
uint _dirty_block_colour = 0;


/** Compare two parent sprites for sorting. */
static bool CompareParentSprites (const ParentSpriteToDraw *ps1,
	const ParentSpriteToDraw *ps2)
{
	if (ps1->xmax < ps2->xmin || ps1->ymax < ps2->ymin
			|| ps1->zmax < ps2->zmin) {
		/* First sprite goes before second one in some axis. */
		return true;
	}

	if (ps1->xmin > ps2->xmax || ps1->ymin > ps2->ymax
			|| ps1->zmin > ps2->zmax) {
		/* No overlap, so second sprite goes before first one. */
		return false;
	}

	/* Use X+Y+Z as the sorting order, so sprites closer to the bottom of
	 * the screen and with higher Z elevation, are drawn in front. Here
	 * X,Y,Z are the coordinates of the "center of mass" of the sprite,
	 * i.e. X=(left+right)/2, etc. However, since we only care about
	 * order, don't actually divide / 2. */
	return  ps1->xmin + ps1->xmax + ps1->ymin + ps1->ymax + ps1->zmin + ps1->zmax <=
		ps2->xmin + ps2->xmax + ps2->ymin + ps2->ymax + ps2->zmin + ps2->zmax;
}

/** Sort parent sprites pointer array */
static void ViewportSortParentSprites (ParentSpriteToDraw **psd,
	const ParentSpriteToDraw *const *psdvend)
{
	SortParentSprites (CompareParentSprites, psd, psdvend);
}

/** Type for the actual viewport sprite sorter. */
typedef void (*VpSpriteSorter) (ParentSpriteToDraw **psd, const ParentSpriteToDraw *const *psdvend);

static const VpSpriteSorter _vp_sprite_sorter =
#ifdef WITH_SSE
		HasCPUIDFlag (1, 2, 19) ? &ViewportSortParentSpritesSSE41 :
#endif
		&ViewportSortParentSprites;


static Point MapXYZToViewport(const ViewPort *vp, int x, int y, int z)
{
	Point p = RemapCoords(x, y, z);
	p.x -= vp->virtual_width / 2;
	p.y -= vp->virtual_height / 2;
	return p;
}

void DeleteWindowViewport(Window *w)
{
	if (w->viewport == NULL) return;

	free(w->viewport);
	w->viewport = NULL;
}

/**
 * Initialize viewport of the window for use.
 * @param w Window to use/display the viewport in
 * @param x Offset of left edge of viewport with respect to left edge window \a w
 * @param y Offset of top edge of viewport with respect to top edge window \a w
 * @param width Width of the viewport
 * @param height Height of the viewport
 * @param follow_flags Flags controlling the viewport.
 *        - If bit 31 is set, the lower 20 bits are the vehicle that the viewport should follow.
 *        - If bit 31 is clear, it is a #TileIndex.
 * @param zoom Zoomlevel to display
 */
void InitializeWindowViewport(Window *w, int x, int y,
	int width, int height, uint32 follow_flags, ZoomLevel zoom)
{
	assert(w->viewport == NULL);

	ViewportData *vp = xcalloct<ViewportData>();

	vp->left = x + w->left;
	vp->top = y + w->top;
	vp->width = width;
	vp->height = height;

	vp->zoom = static_cast<ZoomLevel>(Clamp(zoom, _settings_client.gui.zoom_min, _settings_client.gui.zoom_max));

	vp->virtual_width = ScaleByZoom(width, zoom);
	vp->virtual_height = ScaleByZoom(height, zoom);

	Point pt;

	if (follow_flags & 0x80000000) {
		const Vehicle *veh;

		vp->follow_vehicle = (VehicleID)(follow_flags & 0xFFFFF);
		veh = Vehicle::Get(vp->follow_vehicle);
		pt = MapXYZToViewport(vp, veh->x_pos, veh->y_pos, veh->z_pos);
	} else {
		uint x = TileX(follow_flags) * TILE_SIZE;
		uint y = TileY(follow_flags) * TILE_SIZE;

		vp->follow_vehicle = INVALID_VEHICLE;
		pt = MapXYZToViewport(vp, x, y, GetSlopePixelZ(x, y));
	}

	vp->scrollpos_x = pt.x;
	vp->scrollpos_y = pt.y;
	vp->dest_scrollpos_x = pt.x;
	vp->dest_scrollpos_y = pt.y;

	vp->overlay = NULL;

	w->viewport = vp;
	vp->virtual_left = 0; // pt.x;
	vp->virtual_top = 0;  // pt.y;
}

static Point _vp_move_offs;

static void DoSetViewportPosition(const Window *w, int left, int top, int width, int height)
{
	FOR_ALL_WINDOWS_FROM_BACK_FROM(w, w) {
		if (left + width > w->left &&
				w->left + w->width > left &&
				top + height > w->top &&
				w->top + w->height > top) {

			if (left < w->left) {
				DoSetViewportPosition(w, left, top, w->left - left, height);
				DoSetViewportPosition(w, left + (w->left - left), top, width - (w->left - left), height);
				return;
			}

			if (left + width > w->left + w->width) {
				DoSetViewportPosition(w, left, top, (w->left + w->width - left), height);
				DoSetViewportPosition(w, left + (w->left + w->width - left), top, width - (w->left + w->width - left), height);
				return;
			}

			if (top < w->top) {
				DoSetViewportPosition(w, left, top, width, (w->top - top));
				DoSetViewportPosition(w, left, top + (w->top - top), width, height - (w->top - top));
				return;
			}

			if (top + height > w->top + w->height) {
				DoSetViewportPosition(w, left, top, width, (w->top + w->height - top));
				DoSetViewportPosition(w, left, top + (w->top + w->height - top), width, height - (w->top + w->height - top));
				return;
			}

			return;
		}
	}

	ScrollScreenRect (left, top, width, height, _vp_move_offs.x, _vp_move_offs.y);
}

static void SetViewportPosition(Window *w, int x, int y)
{
	ViewPort *vp = w->viewport;
	int old_left = vp->virtual_left;
	int old_top = vp->virtual_top;
	int i;
	int left, top, width, height;

	vp->virtual_left = x;
	vp->virtual_top = y;

	/* Viewport is bound to its left top corner, so it must be rounded down (UnScaleByZoomLower)
	 * else glitch described in FS#1412 will happen (offset by 1 pixel with zoom level > NORMAL)
	 */
	old_left = UnScaleByZoomLower(old_left, vp->zoom);
	old_top = UnScaleByZoomLower(old_top, vp->zoom);
	x = UnScaleByZoomLower(x, vp->zoom);
	y = UnScaleByZoomLower(y, vp->zoom);

	old_left -= x;
	old_top -= y;

	if (old_top == 0 && old_left == 0) return;

	_vp_move_offs.x = old_left;
	_vp_move_offs.y = old_top;

	left = vp->left;
	top = vp->top;
	width = vp->width;
	height = vp->height;

	if (left < 0) {
		width += left;
		left = 0;
	}

	i = left + width - _screen_width;
	if (i >= 0) width -= i;

	if (width > 0) {
		if (top < 0) {
			height += top;
			top = 0;
		}

		i = top + height - _screen_height;
		if (i >= 0) height -= i;

		if (height > 0) DoSetViewportPosition(w->z_front, left, top, width, height);
	}
}

/**
 * Is a xy position inside the viewport of the window?
 * @param w Window to examine its viewport
 * @param x X coordinate of the xy position
 * @param y Y coordinate of the xy position
 * @return Pointer to the viewport if the xy position is in the viewport of the window,
 *         otherwise \c NULL is returned.
 */
ViewPort *IsPtInWindowViewport(const Window *w, int x, int y)
{
	ViewPort *vp = w->viewport;

	if (vp != NULL &&
			IsInsideMM(x, vp->left, vp->left + vp->width) &&
			IsInsideMM(y, vp->top, vp->top + vp->height))
		return vp;

	return NULL;
}

/**
 * Translate coordinates in a viewport to a tile coordinate
 * @param x Viewport x coordinate
 * @param y Viewport y coordinate
 * @return Tile coordinate
 */
static Point TranslateXYToTileCoord (int x, int y)
{
	Point pt;
	int a, b;
	int z;

	x = x >> (2 + ZOOM_LVL_SHIFT);
	y = y >> (1 + ZOOM_LVL_SHIFT);

	a = y - x;
	b = y + x;

	/* Bring the coordinates near to a valid range. This is mostly due to the
	 * tiles on the north side of the map possibly being drawn too high due to
	 * the extra height levels. So at the top we allow a number of extra tiles.
	 * This number is based on the tile height and pixels. */
	int extra_tiles = CeilDiv(_settings_game.construction.max_heightlevel * TILE_HEIGHT, TILE_PIXELS);
	a = Clamp(a, -extra_tiles * TILE_SIZE, MapMaxX() * TILE_SIZE - 1);
	b = Clamp(b, -extra_tiles * TILE_SIZE, MapMaxY() * TILE_SIZE - 1);

	/* (a, b) is the X/Y-world coordinate that belongs to (x,y) if the landscape would be completely flat on height 0.
	 * Now find the Z-world coordinate by fix point iteration.
	 * This is a bit tricky because the tile height is non-continuous at foundations.
	 * The clicked point should be approached from the back, otherwise there are regions that are not clickable.
	 * (FOUNDATION_HALFTILE_LOWER on SLOPE_STEEP_S hides north halftile completely)
	 * So give it a z-malus of 4 in the first iterations.
	 */
	z = 0;

	int min_coord = _settings_game.construction.freeform_edges ? TILE_SIZE : 0;

	for (int i = 0; i < 5; i++) z = GetSlopePixelZ(Clamp(a + max(z, 4) - 4, min_coord, MapMaxX() * TILE_SIZE - 1), Clamp(b + max(z, 4) - 4, min_coord, MapMaxY() * TILE_SIZE - 1)) / 2;
	for (int malus = 3; malus > 0; malus--) z = GetSlopePixelZ(Clamp(a + max(z, malus) - malus, min_coord, MapMaxX() * TILE_SIZE - 1), Clamp(b + max(z, malus) - malus, min_coord, MapMaxY() * TILE_SIZE - 1)) / 2;
	for (int i = 0; i < 5; i++) z = GetSlopePixelZ(Clamp(a + z, min_coord, MapMaxX() * TILE_SIZE - 1), Clamp(b + z, min_coord, MapMaxY() * TILE_SIZE - 1)) / 2;

	pt.x = Clamp(a + z, min_coord, MapMaxX() * TILE_SIZE - 1);
	pt.y = Clamp(b + z, min_coord, MapMaxY() * TILE_SIZE - 1);

	return pt;
}

Point GetTileBelowCursor()
{
	int x = _cursor.pos.x;
	int y = _cursor.pos.y;

	Window *w = FindWindowFromPt (x, y);

	if (w != NULL) {
		ViewPort *vp = w->viewport;
		if (vp != NULL) {
			x -= vp->left;
			y -= vp->top;

			if ((uint)x < (uint)vp->width && (uint)y < (uint)vp->height) {
				x = ScaleByZoom (x, vp->zoom) + vp->virtual_left;
				y = ScaleByZoom (y, vp->zoom) + vp->virtual_top;
				return TranslateXYToTileCoord (x, y);
			}
		}
	}

	Point pt;
	pt.y = pt.x = -1;
	return pt;
}


void ZoomInOrOutToCursorWindow (bool in, Window *w)
{
	assert (w != NULL);

	if (_game_mode != GM_MENU) {
		ViewPort *vp = w->viewport;
		if ((in && vp->zoom <= _settings_client.gui.zoom_min) || (!in && vp->zoom >= _settings_client.gui.zoom_max)) return;

		uint x = _cursor.pos.x - vp->left;
		uint y = _cursor.pos.y - vp->top;

		if (x >= (uint)vp->width || y >= (uint)vp->height) return;

		if (in) {
			x = (x >> 1) + (vp->width  >> 2);
			y = (y >> 1) + (vp->height >> 2);
		} else {
			x = vp->width  - x;
			y = vp->height - y;
		}

		/* Get the tile below the cursor and center on the zoomed-out center */
		x = ScaleByZoom (x, vp->zoom) + vp->virtual_left;
		y = ScaleByZoom (y, vp->zoom) + vp->virtual_top;
		Point pt = TranslateXYToTileCoord (x, y);
		ScrollWindowTo (pt.x, pt.y, -1, w, true);
		DoZoomInOutViewport (w->viewport, in);
		w->InvalidateData();
	}
}

/**
 * Update the status of the zoom-buttons according to the zoom-level
 * of the viewport. This will update their status and invalidate accordingly
 * @param w Window pointer to the window that has the zoom buttons
 * @param vp pointer to the viewport whose zoom-level the buttons represent
 * @param widget_zoom_in widget index for window with zoom-in button
 * @param widget_zoom_out widget index for window with zoom-out button
 */
void HandleZoomMessage(Window *w, const ViewPort *vp, byte widget_zoom_in, byte widget_zoom_out)
{
	w->SetWidgetDisabledState(widget_zoom_in, vp->zoom <= _settings_client.gui.zoom_min);
	w->SetWidgetDirty(widget_zoom_in);

	w->SetWidgetDisabledState(widget_zoom_out, vp->zoom >= _settings_client.gui.zoom_max);
	w->SetWidgetDirty(widget_zoom_out);
}

/**
 * Schedules a tile sprite for drawing.
 *
 * @param vd the viewport drawer to use.
 * @param image the image to draw.
 * @param pal the provided palette.
 * @param x position x (world coordinates) of the sprite.
 * @param y position y (world coordinates) of the sprite.
 * @param z position z (world coordinates) of the sprite.
 * @param sub Only draw a part of the sprite.
 * @param extra_offs_x Pixel X offset for the sprite position.
 * @param extra_offs_y Pixel Y offset for the sprite position.
 */
static void AddTileSpriteToDraw (ViewportDrawer *vd, SpriteID image,
	PaletteID pal, int32 x, int32 y, int z, const SubSprite *sub = NULL,
	int extra_offs_x = 0, int extra_offs_y = 0)
{
	assert((image & SPRITE_MASK) < MAX_SPRITES);

	TileSpriteToDraw *ts = vd->tile_sprites_to_draw.Append();
	ts->image = image;
	ts->pal = pal;
	ts->sub = sub;
	Point pt = RemapCoords(x, y, z);
	ts->x = pt.x + extra_offs_x;
	ts->y = pt.y + extra_offs_y;
}

/**
 * Adds a child sprite to the active foundation.
 *
 * The pixel offset of the sprite relative to the ParentSprite is the sum of the offset passed to OffsetGroundSprite() and extra_offs_?.
 *
 * @param vd the viewport drawer to use.
 * @param image the image to draw.
 * @param pal the provided palette.
 * @param sub Only draw a part of the sprite.
 * @param foundation_part Foundation part.
 * @param extra_offs_x Pixel X offset for the sprite position.
 * @param extra_offs_y Pixel Y offset for the sprite position.
 */
static void AddChildSpriteToFoundation (ViewportDrawer *vd, SpriteID image,
	PaletteID pal, const SubSprite *sub, FoundationData *foundation_part,
	int extra_offs_x, int extra_offs_y)
{
	assert (foundation_part >= vd->foundation);
	assert (foundation_part < endof(vd->foundation));
	assert (foundation_part->index != -1);
	const Point &offs = foundation_part->offset;

	/* Change the active ChildSprite list to the one of the foundation */
	int *old_child = vd->last_child;
	vd->last_child = foundation_part->last_child;

	AddChildSpriteScreen (vd, image, pal, offs.x + extra_offs_x, offs.y + extra_offs_y, false, sub, false);

	/* Switch back to last ChildSprite list */
	vd->last_child = old_child;
}

/**
 * Draws a ground sprite at a specific world-coordinate relative to the current tile.
 * If the current tile is drawn on top of a foundation the sprite is added as child sprite to the "foundation"-ParentSprite.
 *
 * @param ti TileInfo of the tile to draw
 * @param image the image to draw.
 * @param pal the provided palette.
 * @param x position x (world coordinates) of the sprite relative to current tile.
 * @param y position y (world coordinates) of the sprite relative to current tile.
 * @param z position z (world coordinates) of the sprite relative to current tile.
 * @param sub Only draw a part of the sprite.
 * @param extra_offs_x Pixel X offset for the sprite position.
 * @param extra_offs_y Pixel Y offset for the sprite position.
 */
void DrawGroundSpriteAt (const TileInfo *ti, SpriteID image, PaletteID pal,
	int32 x, int32 y, int z, const SubSprite *sub,
	int extra_offs_x, int extra_offs_y)
{
	/* Switch to first foundation part, if no foundation was drawn */
	if (ti->vd->foundation_part == NULL) ti->vd->foundation_part = ti->vd->foundation;

	if (ti->vd->foundation_part->index != -1) {
		Point pt = RemapCoords(x, y, z);
		AddChildSpriteToFoundation (ti->vd, image, pal, sub, ti->vd->foundation_part, pt.x + extra_offs_x * ZOOM_LVL_BASE, pt.y + extra_offs_y * ZOOM_LVL_BASE);
	} else {
		AddTileSpriteToDraw (ti->vd, image, pal, ti->x + x, ti->y + y, ti->z + z, sub, extra_offs_x * ZOOM_LVL_BASE, extra_offs_y * ZOOM_LVL_BASE);
	}
}

/**
 * Draws a ground sprite for the current tile.
 * If the current tile is drawn on top of a foundation the sprite is added as child sprite to the "foundation"-ParentSprite.
 *
 * @param ti TileInfo of the tile to draw
 * @param image the image to draw.
 * @param pal the provided palette.
 * @param sub Only draw a part of the sprite.
 * @param extra_offs_x Pixel X offset for the sprite position.
 * @param extra_offs_y Pixel Y offset for the sprite position.
 */
void DrawGroundSprite (const TileInfo *ti, SpriteID image, PaletteID pal,
	const SubSprite *sub, int extra_offs_x, int extra_offs_y)
{
	DrawGroundSpriteAt (ti, image, pal, 0, 0, 0, sub, extra_offs_x, extra_offs_y);
}

/**
 * Called when a foundation has been drawn for the current tile.
 * Successive ground sprites for the current tile will be drawn as child sprites of the "foundation"-ParentSprite, not as TileSprites.
 *
 * @param vd the viewport drawer to use.
 * @param x sprite x-offset (screen coordinates) of ground sprites relative to the "foundation"-ParentSprite.
 * @param y sprite y-offset (screen coordinates) of ground sprites relative to the "foundation"-ParentSprite.
 */
void OffsetGroundSprite (ViewportDrawer *vd, int x, int y)
{
	/* Switch to next foundation part */
	if (vd->foundation_part == NULL) {
		vd->foundation_part = vd->foundation;
	} else {
		assert (vd->foundation_part == vd->foundation);
		vd->foundation_part++;
	}

	/* vd->last_child == NULL if foundation sprite was clipped by the viewport bounds */
	if (vd->last_child != NULL) vd->foundation_part->index = vd->parent_sprites_to_draw.Length() - 1;

	vd->foundation_part->offset.x = x * ZOOM_LVL_BASE;
	vd->foundation_part->offset.y = y * ZOOM_LVL_BASE;
	vd->foundation_part->last_child = vd->last_child;
}

/**
 * Adds a child sprite to a parent sprite.
 * In contrast to "AddChildSpriteScreen()" the sprite position is in world coordinates
 *
 * @param vd the viewport drawer to use.
 * @param image the image to draw.
 * @param pal the provided palette.
 * @param pt position of the sprite.
 * @param sub Only draw a part of the sprite.
 */
static void AddCombinedSprite (ViewportDrawer *vd, SpriteID image,
	PaletteID pal, const Point &pt, const SubSprite *sub)
{
	const Sprite *spr = GetSprite(image & SPRITE_MASK, ST_NORMAL);

	if (pt.x + spr->x_offs >= vd->dpi.left + vd->dpi.width ||
			pt.x + spr->x_offs + spr->width <= vd->dpi.left ||
			pt.y + spr->y_offs >= vd->dpi.top + vd->dpi.height ||
			pt.y + spr->y_offs + spr->height <= vd->dpi.top)
		return;

	const ParentSpriteToDraw *pstd = vd->parent_sprites_to_draw.End() - 1;
	AddChildSpriteScreen (vd, image, pal, pt.x - pstd->left, pt.y - pstd->top, false, sub, false);
}

/**
 * Draw a (transparent) sprite at given coordinates with a given bounding box.
 * The bounding box extends from (x + bb_offset_x, y + bb_offset_y, z + bb_offset_z) to (x + w - 1, y + h - 1, z + dz - 1), both corners included.
 * Bounding boxes with bb_offset_x == w or bb_offset_y == h or bb_offset_z == dz are allowed and produce thin slices.
 *
 * @note Bounding boxes are normally specified with bb_offset_x = bb_offset_y = bb_offset_z = 0. The extent of the bounding box in negative direction is
 *       defined by the sprite offset in the grf file.
 *       However if modifying the sprite offsets is not suitable (e.g. when using existing graphics), the bounding box can be tuned by bb_offset.
 *
 * @pre w >= bb_offset_x, h >= bb_offset_y, dz >= bb_offset_z. Else w, h or dz are ignored.
 *
 * @param vd the viewport drawer to use.
 * @param image the image to combine and draw,
 * @param pal the provided palette,
 * @param x position X (world) of the sprite,
 * @param y position Y (world) of the sprite,
 * @param w bounding box extent towards positive X (world),
 * @param h bounding box extent towards positive Y (world),
 * @param dz bounding box extent towards positive Z (world),
 * @param z position Z (world) of the sprite,
 * @param transparent if true, switch the palette between the provided palette and the transparent palette,
 * @param bb_offset_x bounding box extent towards negative X (world),
 * @param bb_offset_y bounding box extent towards negative Y (world),
 * @param bb_offset_z bounding box extent towards negative Z (world)
 * @param sub Only draw a part of the sprite.
 */
void AddSortableSpriteToDraw (ViewportDrawer *vd, SpriteID image, PaletteID pal,
	int x, int y, int w, int h, int dz, int z, bool transparent,
	int bb_offset_x, int bb_offset_y, int bb_offset_z, const SubSprite *sub)
{
	int32 left, right, top, bottom;

	assert((image & SPRITE_MASK) < MAX_SPRITES);

	/* make the sprites transparent with the right palette */
	if (transparent) {
		SetBit(image, PALETTE_MODIFIER_TRANSPARENT);
		pal = PALETTE_TO_TRANSPARENT;
	}

	Point pt = RemapCoords (x, y, z);

	if (vd->combine_sprites == SPRITE_COMBINE_ACTIVE) {
		AddCombinedSprite (vd, image, pal, pt, sub);
		return;
	}

	vd->last_child = NULL;

	int tmp_left, tmp_top, tmp_x = pt.x, tmp_y = pt.y;

	/* Compute screen extents of sprite */
	if (image == SPR_EMPTY_BOUNDING_BOX) {
		left = tmp_left = RemapCoords(x + w          , y + bb_offset_y, z + bb_offset_z).x;
		right           = RemapCoords(x + bb_offset_x, y + h          , z + bb_offset_z).x + 1;
		top  = tmp_top  = RemapCoords(x + bb_offset_x, y + bb_offset_y, z + dz         ).y;
		bottom          = RemapCoords(x + w          , y + h          , z + bb_offset_z).y + 1;
	} else {
		const Sprite *spr = GetSprite(image & SPRITE_MASK, ST_NORMAL);
		left = tmp_left = (pt.x += spr->x_offs);
		right           = (pt.x +  spr->width );
		top  = tmp_top  = (pt.y += spr->y_offs);
		bottom          = (pt.y +  spr->height);
	}

	if (_draw_bounding_boxes && (image != SPR_EMPTY_BOUNDING_BOX)) {
		/* Compute maximal extents of sprite and its bounding box */
		left   = min(left  , RemapCoords(x + w          , y + bb_offset_y, z + bb_offset_z).x);
		right  = max(right , RemapCoords(x + bb_offset_x, y + h          , z + bb_offset_z).x + 1);
		top    = min(top   , RemapCoords(x + bb_offset_x, y + bb_offset_y, z + dz         ).y);
		bottom = max(bottom, RemapCoords(x + w          , y + h          , z + bb_offset_z).y + 1);
	}

	/* Do not add the sprite to the viewport, if it is outside */
	if (left   >= vd->dpi.left + vd->dpi.width ||
	    right  <= vd->dpi.left                 ||
	    top    >= vd->dpi.top + vd->dpi.height ||
	    bottom <= vd->dpi.top) {
		return;
	}

	ParentSpriteToDraw *ps = vd->parent_sprites_to_draw.Append();
	ps->x = tmp_x;
	ps->y = tmp_y;

	ps->left = tmp_left;
	ps->top  = tmp_top;

	ps->image = image;
	ps->pal = pal;
	ps->sub = sub;
	ps->xmin = x + bb_offset_x;
	ps->xmax = x + max(bb_offset_x, w) - 1;

	ps->ymin = y + bb_offset_y;
	ps->ymax = y + max(bb_offset_y, h) - 1;

	ps->zmin = z + bb_offset_z;
	ps->zmax = z + max(bb_offset_z, dz) - 1;

	ps->comparison_done = false;
	ps->first_child = -1;

	vd->last_child = &ps->first_child;

	if (vd->combine_sprites == SPRITE_COMBINE_PENDING) vd->combine_sprites = SPRITE_COMBINE_ACTIVE;
}

/**
 * Starts a block of sprites, which are "combined" into a single bounding box.
 *
 * Subsequent calls to #AddSortableSpriteToDraw will be drawn into the same bounding box.
 * That is: The first sprite that is not clipped by the viewport defines the bounding box, and
 * the following sprites will be child sprites to that one.
 *
 * That implies:
 *  - The drawing order is definite. No other sprites will be sorted between those of the block.
 *  - You have to provide a valid bounding box for all sprites,
 *    as you won't know which one is the first non-clipped one.
 *    Preferable you use the same bounding box for all.
 *  - You cannot use #AddChildSpriteScreen inside the block, as its result will be indefinite.
 *
 * The block is terminated by #EndSpriteCombine.
 *
 * You cannot nest "combined" blocks.
 */
void StartSpriteCombine (ViewportDrawer *vd)
{
	assert (vd->combine_sprites == SPRITE_COMBINE_NONE);
	vd->combine_sprites = SPRITE_COMBINE_PENDING;
}

/**
 * Terminates a block of sprites started by #StartSpriteCombine.
 * Take a look there for details.
 */
void EndSpriteCombine (ViewportDrawer *vd)
{
	assert (vd->combine_sprites != SPRITE_COMBINE_NONE);
	vd->combine_sprites = SPRITE_COMBINE_NONE;
}

/**
 * Check if the parameter "check" is inside the interval between
 * begin and end, including both begin and end.
 * @note Whether \c begin or \c end is the biggest does not matter.
 *       This method will account for that.
 * @param begin The begin of the interval.
 * @param end   The end of the interval.
 * @param check The value to check.
 */
static bool IsInRangeInclusive(int begin, int end, int check)
{
	if (begin > end) Swap(begin, end);
	return begin <= check && check <= end;
}

/**
 * Checks whether a point is inside the selected a diagonal rectangle given by _thd.size and _thd.pos
 * @param x The x coordinate of the point to be checked.
 * @param y The y coordinate of the point to be checked.
 * @return True if the point is inside the rectangle, else false.
 */
bool IsInsideRotatedRectangle(int x, int y)
{
	int dist_a = (_thd.size.x + _thd.size.y);      // Rotated coordinate system for selected rectangle.
	int dist_b = (_thd.size.x - _thd.size.y);      // We don't have to divide by 2. It's all relative!
	int a = ((x - _thd.pos.x) + (y - _thd.pos.y)); // Rotated coordinate system for the point under scrutiny.
	int b = ((x - _thd.pos.x) - (y - _thd.pos.y));

	/* Check if a and b are between 0 and dist_a or dist_b respectively. */
	return IsInRangeInclusive(dist_a, 0, a) && IsInRangeInclusive(dist_b, 0, b);
}

/**
 * Add a child sprite to a parent sprite.
 *
 * @param vd the viewport drawer to use.
 * @param image the image to draw.
 * @param pal the provided palette.
 * @param x sprite x-offset (screen coordinates) relative to parent sprite.
 * @param y sprite y-offset (screen coordinates) relative to parent sprite.
 * @param transparent if true, switch the palette between the provided palette and the transparent palette,
 * @param sub Only draw a part of the sprite.
 */
void AddChildSpriteScreen (ViewportDrawer *vd, SpriteID image, PaletteID pal,
	int x, int y, bool transparent, const SubSprite *sub, bool scale)
{
	assert((image & SPRITE_MASK) < MAX_SPRITES);

	/* If the ParentSprite was clipped by the viewport bounds, do not draw the ChildSprites either */
	if (vd->last_child == NULL) return;

	/* make the sprites transparent with the right palette */
	if (transparent) {
		SetBit(image, PALETTE_MODIFIER_TRANSPARENT);
		pal = PALETTE_TO_TRANSPARENT;
	}

	*vd->last_child = vd->child_screen_sprites_to_draw.Length();

	ChildScreenSpriteToDraw *cs = vd->child_screen_sprites_to_draw.Append();
	cs->image = image;
	cs->pal = pal;
	cs->sub = sub;
	cs->x = scale ? x * ZOOM_LVL_BASE : x;
	cs->y = scale ? y * ZOOM_LVL_BASE : y;
	cs->next = -1;

	/* Append the sprite to the active ChildSprite list.
	 * If the active ParentSprite is a foundation, update last_foundation_child as well.
	 * Note: ChildSprites of foundations are NOT sequential in the vector, as selection sprites are added at last. */
	if (vd->foundation[0].last_child == vd->last_child) vd->foundation[0].last_child = &cs->next;
	if (vd->foundation[1].last_child == vd->last_child) vd->foundation[1].last_child = &cs->next;
	vd->last_child = &cs->next;
}


/**
 * Draws sprites between ground sprite and everything above.
 *
 * The sprite is either drawn as TileSprite or as ChildSprite of the active foundation.
 *
 * @param image the image to draw.
 * @param pal the provided palette.
 * @param ti TileInfo Tile that is being drawn
 * @param z_offset Z offset relative to the groundsprite. Only used for the sprite position, not for sprite sorting.
 * @param foundation_part Foundation part the sprite belongs to.
 */
static void DrawSelectionSprite (SpriteID image, PaletteID pal, const TileInfo *ti, int z_offset, FoundationData *foundation_part)
{
	/* FIXME: This is not totally valid for some autorail highlights that extend over the edges of the tile. */
	if (foundation_part->index == -1) {
		/* draw on real ground */
		AddTileSpriteToDraw (ti->vd, image, pal, ti->x, ti->y, ti->z + z_offset);
	} else {
		/* draw on top of foundation */
		AddChildSpriteToFoundation (ti->vd, image, pal, NULL, foundation_part, 0, -z_offset * ZOOM_LVL_BASE);
	}
}

/**
 * Draws a selection rectangle on a tile.
 *
 * @param ti TileInfo Tile that is being drawn
 * @param pal Palette to apply.
 */
static void DrawTileSelectionRect(const TileInfo *ti, PaletteID pal)
{
	if (!IsValidTile(ti->tile)) return;

	SpriteID sel;
	if (IsHalftileSlope(ti->tileh)) {
		Corner halftile_corner = GetHalftileSlopeCorner(ti->tileh);
		SpriteID sel2 = SPR_HALFTILE_SELECTION_FLAT + halftile_corner;
		DrawSelectionSprite (sel2, pal, ti, 7 + TILE_HEIGHT, &ti->vd->foundation[FOUNDATION_PART_HALFTILE]);

		Corner opposite_corner = OppositeCorner(halftile_corner);
		if (IsSteepSlope(ti->tileh)) {
			sel = SPR_HALFTILE_SELECTION_DOWN;
		} else {
			sel = ((ti->tileh & SlopeWithOneCornerRaised(opposite_corner)) != 0 ? SPR_HALFTILE_SELECTION_UP : SPR_HALFTILE_SELECTION_FLAT);
		}
		sel += opposite_corner;
	} else {
		sel = SPR_SELECT_TILE + SlopeToSpriteOffset(ti->tileh);
	}
	DrawSelectionSprite (sel, pal, ti, 7, &ti->vd->foundation[FOUNDATION_PART_NORMAL]);
}

#include "table/autorail.h"

/**
 * Draws autorail highlights.
 *
 * @param *ti TileInfo Tile that is being drawn
 * @param track Track that is being drawn
 */
static void DrawAutorailSelection (const TileInfo *ti, Track track)
{
	static const byte RED = 0x80; // flag for invalid tracks

	/* table maps each of the six rail directions and tileh combinations to a sprite
	 * invalid entries are required to make sure that this array can be quickly accessed */
	static const byte autorail_sprites[][6] = {
		{  0,   8,     16,     25,     34,     42 }, // tileh = 0
		{  5,  13, RED|22, RED|31,     35,     42 }, // tileh = 1
		{  5,  10,     16,     26, RED|38, RED|46 }, // tileh = 2
		{  5,   9, RED|23,     26,     35, RED|46 }, // tileh = 3
		{  2,  10, RED|19, RED|28,     34,     43 }, // tileh = 4
		{  1,   9,     17,     26,     35,     43 }, // tileh = 5
		{  1,  10, RED|20,     26, RED|38,     43 }, // tileh = 6
		{  1,   9,     17,     26,     35,     43 }, // tileh = 7
		{  2,  13,     17,     25, RED|40, RED|48 }, // tileh = 8
		{  1,  13,     17, RED|32,     35, RED|48 }, // tileh = 9
		{  1,   9,     17,     26,     35,     43 }, // tileh = 10
		{  1,   9,     17,     26,     35,     43 }, // tileh = 11
		{  2,   9,     17, RED|29, RED|40,     43 }, // tileh = 12
		{  1,   9,     17,     26,     35,     43 }, // tileh = 13
		{  1,   9,     17,     26,     35,     43 }, // tileh = 14
		{  0,   1,      2,      3,      4,      5 }, // invalid (15)
		{  0,   1,      2,      3,      4,      5 }, // invalid (16)
		{  0,   1,      2,      3,      4,      5 }, // invalid (17)
		{  0,   1,      2,      3,      4,      5 }, // invalid (18)
		{  0,   1,      2,      3,      4,      5 }, // invalid (19)
		{  0,   1,      2,      3,      4,      5 }, // invalid (20)
		{  0,   1,      2,      3,      4,      5 }, // invalid (21)
		{  0,   1,      2,      3,      4,      5 }, // invalid (22)
		{  6,  11,     17,     27, RED|39, RED|47 }, // tileh = 23
		{  0,   1,      2,      3,      4,      5 }, // invalid (24)
		{  0,   1,      2,      3,      4,      5 }, // invalid (25)
		{  0,   1,      2,      3,      4,      5 }, // invalid (26)
		{  7,  15, RED|24, RED|33,     36,     44 }, // tileh = 27
		{  0,   1,      2,      3,      4,      5 }, // invalid (28)
		{  3,  14,     18,     26, RED|41, RED|49 }, // tileh = 29
		{  4,  12, RED|21, RED|30,     37,     45 }  // tileh = 30
	};

	FoundationData *foundation_part = ti->vd->foundation;
	Slope autorail_tileh = RemoveHalftileSlope(ti->tileh);
	if (IsHalftileSlope(ti->tileh)) {
		static const uint _lower_rail[4] = { 5U, 2U, 4U, 3U };
		Corner halftile_corner = GetHalftileSlopeCorner(ti->tileh);
		if (track != _lower_rail[halftile_corner]) {
			foundation_part++;
			/* Here we draw the highlights of the "three-corners-raised"-slope. That looks ok to me. */
			autorail_tileh = SlopeWithThreeCornersRaised(OppositeCorner(halftile_corner));
		}
	}

	uint offset = autorail_sprites[autorail_tileh][track];

	DrawSelectionSprite (SPR_AUTORAIL_BASE + (offset & 0x7F),
			(_thd.make_square_red || (offset & 0x80) != 0) ?
					PALETTE_SEL_TILE_RED : PAL_NONE,
			ti, 7, foundation_part);
}

/**
 * Checks if the specified tile is selected and if so draws selection using correct selectionstyle.
 * @param *ti TileInfo Tile that is being drawn
 * @param zoom Zoom level to draw at
 */
static void DrawTileSelection (const TileInfo *ti, ZoomLevel zoom)
{
	/* Draw a red error square? */
	bool is_redsq = _thd.redsq == ti->tile;
	if (is_redsq) DrawTileSelectionRect(ti, PALETTE_TILE_RED_PULSATING);

	/* No tile selection active? */
	if (_thd.drawstyle == HT_NONE) return;

	if (_thd.diagonal) { // We're drawing a 45 degrees rotated (diagonal) rectangle
		if (!IsInsideRotatedRectangle ((int)ti->x, (int)ti->y)) return;
	} else if (!IsInsideBS (ti->x, _thd.pos.x, _thd.size.x) ||
			!IsInsideBS (ti->y, _thd.pos.y, _thd.size.y)) {
		/* Check if it's inside the outer area? */
		if (!is_redsq && _thd.outersize.x > 0 &&
				IsInsideBS (ti->x, _thd.pos.x + _thd.offs.x, _thd.size.x + _thd.outersize.x) &&
				IsInsideBS (ti->y, _thd.pos.y + _thd.offs.y, _thd.size.y + _thd.outersize.y)) {
			/* Draw a blue rect. */
			DrawTileSelectionRect (ti, PALETTE_SEL_TILE_BLUE);
		}
		return;
	}

	/* Inside the inner area. */

	if (_thd.drawstyle == HT_RECT) {
		if (!is_redsq) DrawTileSelectionRect(ti, _thd.make_square_red ? PALETTE_SEL_TILE_RED : PAL_NONE);
	} else if (_thd.drawstyle == HT_POINT) {
		/* Figure out the Z coordinate for the single dot. */
		int z = 0;
		FoundationData *foundation_part = ti->vd->foundation;
		if (ti->tileh & SLOPE_N) {
			z += TILE_HEIGHT;
			if (RemoveHalftileSlope(ti->tileh) == SLOPE_STEEP_N) z += TILE_HEIGHT;
		}
		if (IsHalftileSlope(ti->tileh)) {
			Corner halftile_corner = GetHalftileSlopeCorner(ti->tileh);
			if ((halftile_corner == CORNER_W) || (halftile_corner == CORNER_E)) z += TILE_HEIGHT;
			if (halftile_corner != CORNER_S) {
				foundation_part++;
				if (IsSteepSlope(ti->tileh)) z -= TILE_HEIGHT;
			}
		}
		DrawSelectionSprite (zoom <= ZOOM_LVL_DETAIL ? SPR_DOT : SPR_DOT_SMALL, PAL_NONE, ti, z, foundation_part);
	} else {
		/* autorail highlighting */
		assert ((_thd.drawstyle & HT_RAIL) != 0);

		Track track = (Track)(_thd.drawstyle & HT_TRACK_MASK);
		assert (IsValidTrack (track));

		if (_thd.select_method != VPM_NONE) {
			assert (_thd.select_method == VPM_RAILDIRS);

			int px = ti->x - _thd.selstart.x;
			int py = ti->y - _thd.selstart.y;

			switch (track) {
				case TRACK_X: // x direction
					track = (py == 0) ? TRACK_X : INVALID_TRACK;
					break;
				case TRACK_Y: // y direction
					track = (px == 0) ? TRACK_Y : INVALID_TRACK;
					break;
				case TRACK_UPPER: // horizontal upper
					track = (px == -py)      ? TRACK_UPPER :
						(px == -py - 16) ? TRACK_LOWER :
						INVALID_TRACK;
					break;
				case TRACK_LOWER: // horizontal lower
					track = (px == -py)      ? TRACK_LOWER :
						(px == -py + 16) ? TRACK_UPPER :
						INVALID_TRACK;
					break;
				case TRACK_LEFT: // vertical left
					track = (px == py)      ? TRACK_LEFT  :
						(px == py + 16) ? TRACK_RIGHT :
						INVALID_TRACK;
					break;
				case TRACK_RIGHT: // vertical right
					track = (px == py)      ? TRACK_RIGHT :
						(px == py - 16) ? TRACK_LEFT  :
						INVALID_TRACK;
					break;
				default: NOT_REACHED();
			}
		}

		if (track != INVALID_TRACK) DrawAutorailSelection (ti, track);
	}
}

static void DrawTownArea (const TileInfo *ti)
{
	if (_thd.town == INVALID_TOWN) return;

	Town *t = Town::Get (_thd.town);

	if (DistanceSquare (ti->tile, t->xy) < t->cache.squared_town_zone_radius[HZB_TOWN_EDGE]) {
		DrawTileSelectionRect (ti, PALETTE_SEL_TILE_BLUE);
	}
}

int GetVirtualHeight (int x, int y)
{
	/* Assume a decreasing slope to 0 outside the map. */
	int correction = 0;

	if (x < 0) {
		correction += x;
		x = 0;
	} else if ((uint)x >= MapSizeX()) {
		correction += MapMaxX() - x;
		x = MapMaxX();
	}

	if (y < 0) {
		correction += y;
		y = 0;
	} else if ((uint)y >= MapSizeY()) {
		correction += MapMaxY() - y;
		y = MapMaxY();
	}

	return max ((int)(TileHeight (TileXY (x, y)) + correction), 0);
}

static inline void GetVirtualSlope_Corner (TileInfo *ti, uint refx, uint refy,
	int dx, int dy, Slope limit_slope, Slope steep_slope)
{
	int h = TileHeight (TileXY (refx, refy)) + dx + dy;

	if (h >= 0) {
		ti->tileh = steep_slope;
		ti->z = h * TILE_HEIGHT;
	} else {
		ti->tileh = (h == -1) ? limit_slope : SLOPE_FLAT;
		ti->z = 0;
	}
}

static inline void GetVirtualSlope_Side (TileInfo *ti, uint refx0, uint refy0,
	uint refx1, uint refy1, int diff,
	Slope inclined_slope, Slope inclined_slope0, Slope inclined_slope1)
{
	int h0 = TileHeight (TileXY (refx0, refy0));
	int h1 = TileHeight (TileXY (refx1, refy1));

	if (h0 > h1) {
		int h = h1 + diff;
		if (h >= 0) {
			ti->tileh = inclined_slope | inclined_slope0 | SLOPE_STEEP;
			ti->z = h * TILE_HEIGHT;
		} else {
			ti->tileh = (h == -1) ? (inclined_slope & inclined_slope0) : SLOPE_FLAT;
			ti->z = 0;
		}
	} else if (h0 < h1) {
		int h = h0 + diff;
		if (h >= 0) {
			ti->tileh = inclined_slope | inclined_slope1 | SLOPE_STEEP;
			ti->z = h * TILE_HEIGHT;
		} else {
			ti->tileh = (h == -1) ? (inclined_slope & inclined_slope1) : SLOPE_FLAT;
			ti->z = 0;
		}
	} else {
		int h = h0 + diff;
		if (h >= 0) {
			ti->tileh = inclined_slope;
			ti->z = h * TILE_HEIGHT;
		} else {
			ti->tileh = SLOPE_FLAT;
			ti->z = 0;
		}
	}
}

static void GetVirtualSlope (int x, int y, TileInfo *ti, DrawTileProc **dtp)
{
	/* Assume a decreasing slope to 0 outside the map. */
	if (x < 0) {
		if (y < 0) {
			/* We are north of the map. */
			GetVirtualSlope_Corner (ti, 0, 0, x, y, SLOPE_S, SLOPE_STEEP_S);
		} else if ((uint)y < MapMaxY()) {
			/* We are north-east of the map. */
			GetVirtualSlope_Side (ti, 0, y, 0, y + 1, x,
				SLOPE_SW, SLOPE_NW, SLOPE_SE);
		} else {
			/* We are east of the map. */
			GetVirtualSlope_Corner (ti, 0, MapMaxY(), x, MapMaxY() - y - 1, SLOPE_W, SLOPE_STEEP_W);
		}
	} else if ((uint)x < MapMaxX()) {
		if (y < 0) {
			/* We are north-west of the map. */
			GetVirtualSlope_Side (ti, x, 0, x + 1, 0, y,
				SLOPE_SE, SLOPE_NE, SLOPE_SW);
		} else if ((uint)y < MapMaxY()) {
			/* We are on the map. */
			TileIndex tile = TileXY (x, y);
			ti->tile = tile;
			ti->tileh = GetTilePixelSlope (tile, &ti->z);
			*dtp = GetTileProcs(tile)->draw_tile_proc;
			return;
		} else {
			/* We are south-east of the map. */
			GetVirtualSlope_Side (ti, x, MapMaxY(), x + 1, MapMaxY(), MapMaxY() - y - 1,
				SLOPE_NW, SLOPE_NE, SLOPE_SW);
		}
	} else {
		if (y < 0) {
			/* We are west of the map. */
			GetVirtualSlope_Corner (ti, MapMaxX(), 0, MapMaxX() - x - 1, y, SLOPE_E, SLOPE_STEEP_E);
		} else if ((uint)y < MapMaxY()) {
			/* We are south-west of the map. */
			GetVirtualSlope_Side (ti, MapMaxX(), y, MapMaxX(), y + 1, MapMaxX() - x - 1,
				SLOPE_NE, SLOPE_NW, SLOPE_SE);
		} else {
			/* We are south of the map. */
			GetVirtualSlope_Corner (ti, MapMaxX(), MapMaxY(), MapMaxX() - x - 1, MapMaxY() - y - 1, SLOPE_N, SLOPE_STEEP_N);
		}
	}

	ti->tile = INVALID_TILE;
	*dtp = DrawVoidTile;
}

static void ViewportAddLandscape (ViewportDrawer *vd, ZoomLevel zoom)
{
	static const uint HEIGHT_SHIFT = ZOOM_LVL_SHIFT + 3;
	static const uint WIDTH_SHIFT  = ZOOM_LVL_SHIFT + 5;

	int htop = (vd->dpi.top  - 1) >> HEIGHT_SHIFT;
	int top  = htop >> 1;
	int left = (vd->dpi.left - 1) >> WIDTH_SHIFT;
	int x = (top - left) >> 1;
	int y = (top + left) >> 1;
	bool direction = ((top ^ left) & 1) != 0;
	if (!direction) x--;
	assert (((2 * (x + y)) == (htop - 3)) || ((2 * (x + y)) == (htop - 2)));

	int hbot = (vd->dpi.top + vd->dpi.height) >> HEIGHT_SHIFT;
	int bottom = hbot >> 1;
	int right = (vd->dpi.left + vd->dpi.width) >> WIDTH_SHIFT;
	int width = (((bottom + right) >> 1) - y) - (((bottom - right) >> 1) - x) + !direction + 1;

	assert (width > 0);

	/* Now (x,y) is the tile that would be drawn at the top left corner of
	 * the viewport _if_ it were at sea level. But perhaps it is not. */
	for (;;) {
		uint w = ((uint)width - !direction) / 2;
		int x_cur = x;
		int y_cur = y;

		int h = GetVirtualHeight (x_cur + 1, y_cur + 1);
		while (w--) {
			h = min (h, GetVirtualHeight (--x_cur + 1, ++y_cur + 1));
		}

		/* So now h is the minimum height of the southern corners
		 * of all tiles in the row. Check if any of them is visible. */
		int hmin = 2 * (x + y + 2) - h;
		if (hmin > htop) {
			assert ((hmin - htop) <= 3);
			break;
		}

		/* No tile in this row needs to be drawn.
		 * See how many rows we can skip. */
		uint n = 1 + (uint)(htop - hmin) / 3;
		y += n / 2;
		x += n / 2;
		if ((n % 2) != 0) {
			direction ? y++ : x++;
			direction = !direction;
		}
	}

	TileInfo ti;
	ti.vd = vd;

	enum {
		STATE_GROUND,    ///< ground in the current row is visible
		STATE_BUILDINGS, ///< buildings in the current row may be visible, but ground is not
		STATE_BRIDGES,   ///< only high enough bridges in the current row may be visible
	} state = STATE_GROUND;

	for (;;) {
		uint w = ((uint)width - !direction) / 2;
		int x_cur = x;
		int y_cur = y;

		int h = GetVirtualHeight (x_cur + 1, y_cur);

		do {
			ti.x = x_cur * TILE_SIZE;
			ti.y = y_cur * TILE_SIZE;

			DrawTileProc *dtp;
			GetVirtualSlope (x_cur, y_cur, &ti, &dtp);

			if (state == STATE_GROUND || (ti.tile != INVALID_TILE &&
					(state == STATE_BUILDINGS || HasBridgeAbove(ti.tile)))) {
				vd->foundation_part = NULL;
				vd->foundation[0].index = -1;
				vd->foundation[1].index = -1;
				vd->foundation[0].last_child = NULL;
				vd->foundation[1].last_child = NULL;
				dtp(&ti);
			}

			if (state == STATE_GROUND) {
				if (((uint)x_cur == MapMaxX() && (uint)y_cur < MapSizeY()) ||
						((uint)y_cur == MapMaxY() && (uint)x_cur < MapSizeX())) {
					TileIndex tile = TileXY (x_cur, y_cur);
					ti.tile = tile;
					ti.tileh = GetTilePixelSlope(tile, &ti.z);
				}
				if (ti.tile != INVALID_TILE) {
					DrawTownArea(&ti);
					DrawTileSelection (&ti, zoom);
				}
			}

			y_cur++;
			h = max (h, GetVirtualHeight (x_cur, y_cur));
			x_cur--;
		} while (w--);

		int hnew = 2 * (x + y + 1) - h;
		switch (state) {
			case STATE_GROUND:
				/* Is ground still visible? */
				if (hnew <= hbot + 1) break;
				state = STATE_BUILDINGS;
				/* fall through */
			case STATE_BUILDINGS:
				/* Are buildings still possibly visible? */
				if (hnew <= hbot + MAX_BUILDING_HEIGHT) break;
				state = STATE_BRIDGES;
				/* fall through */
			case STATE_BRIDGES:
				/* Are bridges still possibly visible? */
				if (hnew <= hbot + _settings_game.construction.max_bridge_height + 4) break;
				return;
		}

		direction ? y++ : x++;
		direction = !direction;
	}
}

static inline void ViewportDrawString (BlitArea *area, ZoomLevel zoom,
	int x, int y, StringID string, uint64 params_1, uint64 params_2,
	Colours colour, int width, bool small)
{
	TextColour tc = TC_BLACK;
	int x0 = UnScaleByZoom (x, zoom);
	int x1 = x0 + width;
	int y0 = UnScaleByZoom (y, zoom);

	SetDParam (0, params_1);
	SetDParam (1, params_2);

	if (colour != INVALID_COLOUR) {
		if (IsTransparencySet(TO_SIGNS) && string != STR_WHITE_SIGN) {
			/* Don't draw the rectangle.
			 * Real colours need the TC_IS_PALETTE_COLOUR flag.
			 * Otherwise colours from _string_colourmap are assumed. */
			tc = (TextColour)_colour_gradient[colour][6] | TC_IS_PALETTE_COLOUR;
		} else {
			/* Draw the rectangle if 'transparent station signs' is off,
			 * or if we are drawing a general text sign (STR_WHITE_SIGN). */
			DrawFrameRect (area,
				x0, y0, x1, y0 + VPSM_TOP + (small ? FONT_HEIGHT_SMALL : FONT_HEIGHT_NORMAL) + VPSM_BOTTOM, colour,
				IsTransparencySet(TO_SIGNS) ? FR_TRANSPARENT : FR_NONE
			);
		}
	}

	DrawString (area, x0 + VPSM_LEFT, x1 - 1 - VPSM_RIGHT, y0 + VPSM_TOP, string, tc, SA_HOR_CENTER);
}

/**
 * Add a string to draw in the viewport
 * @param area current viewport area
 * @param dpi current viewport area
 * @param small_from Zoomlevel from when the small font should be used
 * @param sign sign position and dimension
 * @param string_normal String for normal and 2x zoom level
 * @param string_small String for 4x and 8x zoom level
 * @param string_small_shadow Shadow string for 4x and 8x zoom level; or #STR_NULL if no shadow
 * @param colour colour of the sign background; or INVALID_COLOUR if transparent
 */
void ViewportAddString (BlitArea *area, const DrawPixelInfo *dpi,
	ZoomLevel small_from, const ViewportSign *sign, StringID string_normal,
	StringID string_small, StringID string_small_shadow,
	uint64 params_1, uint64 params_2, Colours colour)
{
	bool small = dpi->zoom >= small_from;

	int left   = dpi->left;
	int top    = dpi->top;
	int right  = left + dpi->width;
	int bottom = top + dpi->height;

	int sign_height     = ScaleByZoom(VPSM_TOP + FONT_HEIGHT_NORMAL + VPSM_BOTTOM, dpi->zoom);
	int sign_width      = small ? sign->width_small : sign->width_normal;
	int sign_half_width = ScaleByZoom(sign_width / 2, dpi->zoom);

	if (bottom < sign->top ||
			top   > sign->top + sign_height ||
			right < sign->center - sign_half_width ||
			left  > sign->center + sign_half_width) {
		return;
	}

	assert (sign_width != 0);

	int x = sign->center - sign_half_width;
	int y = sign->top;
	if (small && (string_small_shadow != STR_NULL)) {
		ViewportDrawString (area, dpi->zoom, x + 4, y,
				string_small_shadow, params_1, params_2,
				INVALID_COLOUR, sign_width, false);
		y -= 4;
	}

	StringID str = small ? string_small : string_normal;
	ViewportDrawString (area, dpi->zoom, x, y, str, params_1, params_2,
			colour, sign_width, small);
}

static void ViewportAddTownNames (BlitArea *area, DrawPixelInfo *dpi)
{
	if (!HasBit(_display_opt, DO_SHOW_TOWN_NAMES) || _game_mode == GM_MENU) return;

	const Town *t;
	FOR_ALL_TOWNS(t) {
		ViewportAddString (area, dpi, ZOOM_LVL_OUT_16X, &t->cache.sign,
				_settings_client.gui.population_in_label ? STR_VIEWPORT_TOWN_POP : STR_VIEWPORT_TOWN,
				STR_VIEWPORT_TOWN_TINY_WHITE, STR_VIEWPORT_TOWN_TINY_BLACK,
				t->index, t->cache.population);
	}
}


static void ViewportAddStationNames (BlitArea *area, DrawPixelInfo *dpi)
{
	if (!(HasBit(_display_opt, DO_SHOW_STATION_NAMES) || HasBit(_display_opt, DO_SHOW_WAYPOINT_NAMES)) || IsInvisibilitySet(TO_SIGNS) || _game_mode == GM_MENU) return;

	const BaseStation *st;
	FOR_ALL_BASE_STATIONS(st) {
		/* Check whether the base station is a station or a waypoint */
		bool is_station = !st->IsWaypoint();

		/* Don't draw if the display options are disabled */
		if (!HasBit(_display_opt, is_station ? DO_SHOW_STATION_NAMES : DO_SHOW_WAYPOINT_NAMES)) continue;

		/* Don't draw if station is owned by another company and competitor station names are hidden. Stations owned by none are never ignored. */
		if (!HasBit(_display_opt, DO_SHOW_COMPETITOR_SIGNS) && _local_company != st->owner && st->owner != OWNER_NONE) continue;

		ViewportAddString (area, dpi, ZOOM_LVL_OUT_16X, &st->sign,
				is_station ? STR_VIEWPORT_STATION : STR_VIEWPORT_WAYPOINT,
				(is_station ? STR_VIEWPORT_STATION : STR_VIEWPORT_WAYPOINT) + 1, STR_NULL,
				st->index, st->facilities, (st->owner == OWNER_NONE || !st->IsInUse()) ? COLOUR_GREY : _company_colours[st->owner]);
	}
}


static void ViewportAddSigns (BlitArea *area, DrawPixelInfo *dpi)
{
	/* Signs are turned off or are invisible */
	if (!HasBit(_display_opt, DO_SHOW_SIGNS) || IsInvisibilitySet(TO_SIGNS)) return;

	const Sign *si;
	FOR_ALL_SIGNS(si) {
		/* Don't draw if sign is owned by another company and competitor signs should be hidden.
		 * Note: It is intentional that also signs owned by OWNER_NONE are hidden. Bankrupt
		 * companies can leave OWNER_NONE signs after them. */
		if (!HasBit(_display_opt, DO_SHOW_COMPETITOR_SIGNS) && _local_company != si->owner && si->owner != OWNER_DEITY) continue;

		ViewportAddString (area, dpi, ZOOM_LVL_OUT_16X, &si->sign,
				STR_WHITE_SIGN,
				(IsTransparencySet(TO_SIGNS) || si->owner == OWNER_DEITY) ? STR_VIEWPORT_SIGN_SMALL_WHITE : STR_VIEWPORT_SIGN_SMALL_BLACK, STR_NULL,
				si->index, 0, (si->owner == OWNER_NONE) ? COLOUR_GREY : (si->owner == OWNER_DEITY ? INVALID_COLOUR : _company_colours[si->owner]));
	}
}

/**
 * Update the position of the viewport sign.
 * @param center the (preferred) center of the viewport sign
 * @param top    the new top of the sign
 * @param str    the string to show in the sign
 * @param str_small the string to show when zoomed out. STR_NULL means same as \a str
 */
void ViewportSign::UpdatePosition(int center, int top, StringID str, StringID str_small)
{
	if (this->width_normal != 0) this->MarkDirty();

	this->top = top;

	char buffer[DRAW_STRING_BUFFER];

	GetString (buffer, str);
	this->width_normal = VPSM_LEFT + Align(GetStringBoundingBox(buffer).width, 2) + VPSM_RIGHT;
	this->center = center;

	/* zoomed out version */
	if (str_small != STR_NULL) {
		GetString (buffer, str_small);
	}
	this->width_small = VPSM_LEFT + Align(GetStringBoundingBox(buffer, FS_SMALL).width, 2) + VPSM_RIGHT;

	this->MarkDirty();
}

/**
 * Mark the sign dirty in all viewports.
 * @param maxzoom Maximum %ZoomLevel at which the text is visible.
 *
 * @ingroup dirty
 */
void ViewportSign::MarkDirty(ZoomLevel maxzoom) const
{
	Rect zoomlevels[ZOOM_LVL_COUNT];

	for (ZoomLevel zoom = ZOOM_LVL_BEGIN; zoom != ZOOM_LVL_END; zoom++) {
		/* FIXME: This doesn't switch to width_small when appropriate. */
		zoomlevels[zoom].left   = this->center - ScaleByZoom(this->width_normal / 2 + 1, zoom);
		zoomlevels[zoom].top    = this->top    - ScaleByZoom(1, zoom);
		zoomlevels[zoom].right  = this->center + ScaleByZoom(this->width_normal / 2 + 1, zoom);
		zoomlevels[zoom].bottom = this->top    + ScaleByZoom(VPSM_TOP + FONT_HEIGHT_NORMAL + VPSM_BOTTOM + 1, zoom);
	}

	Window *w;
	FOR_ALL_WINDOWS_FROM_BACK(w) {
		ViewPort *vp = w->viewport;
		if (vp != NULL && vp->zoom <= maxzoom) {
			assert(vp->width != 0);
			Rect &zl = zoomlevels[vp->zoom];
			MarkViewportDirty(vp, zl.left, zl.top, zl.right, zl.bottom);
		}
	}
}

static void ViewportDrawTileSprites (DrawPixelInfo *dpi, const TileSpriteToDrawVector *tstdv)
{
	const TileSpriteToDraw *tsend = tstdv->End();
	for (const TileSpriteToDraw *ts = tstdv->Begin(); ts != tsend; ++ts) {
		DrawSpriteViewport (dpi, ts->image, ts->pal, ts->x, ts->y, ts->sub);
	}
}

static void ViewportDrawParentSprites (DrawPixelInfo *dpi,
	const ParentSpriteToDraw *const *it,
	const ParentSpriteToDraw *const *psd_end,
	const ChildScreenSpriteToDraw *csstdv)
{
	for (; it != psd_end; it++) {
		const ParentSpriteToDraw *ps = *it;
		if (ps->image != SPR_EMPTY_BOUNDING_BOX) DrawSpriteViewport (dpi, ps->image, ps->pal, ps->x, ps->y, ps->sub);

		int child_idx = ps->first_child;
		while (child_idx >= 0) {
			const ChildScreenSpriteToDraw *cs = &csstdv[child_idx];
			child_idx = cs->next;
			DrawSpriteViewport (dpi, cs->image, cs->pal, ps->left + cs->x, ps->top + cs->y, cs->sub);
		}
	}
}

/**
 * Draws the bounding boxes of all ParentSprites
 * @param psd Array of ParentSprites
 */
static void ViewportDrawBoundingBoxes (DrawPixelInfo *dpi,
	const ParentSpriteToDraw *const *it,
	const ParentSpriteToDraw *const *const psd_end)
{
	for (; it != psd_end; it++) {
		const ParentSpriteToDraw *ps = *it;
		Point pt1 = RemapCoords(ps->xmax + 1, ps->ymax + 1, ps->zmax + 1); // top front corner
		Point pt2 = RemapCoords(ps->xmin    , ps->ymax + 1, ps->zmax + 1); // top left corner
		Point pt3 = RemapCoords(ps->xmax + 1, ps->ymin    , ps->zmax + 1); // top right corner
		Point pt4 = RemapCoords(ps->xmax + 1, ps->ymax + 1, ps->zmin    ); // bottom front corner

		DrawBox (dpi,         pt1.x,         pt1.y,
		              pt2.x - pt1.x, pt2.y - pt1.y,
		              pt3.x - pt1.x, pt3.y - pt1.y,
		              pt4.x - pt1.x, pt4.y - pt1.y);
	}
}

/**
 * Draw/colour the blocks that have been redrawn.
 */
static void ViewportDrawDirtyBlocks (const DrawPixelInfo *dpi)
{
	dpi->surface->draw_checker (dpi->dst_ptr,
			UnScaleByZoom (dpi->width,  dpi->zoom),
			UnScaleByZoom (dpi->height, dpi->zoom),
			_string_colourmap[_dirty_block_colour & 0xF],
			UnScaleByZoom (dpi->left + dpi->top, dpi->zoom) & 1);
}

void ViewportDoDraw (Blitter::Surface *surface, void *dst_ptr,
	const ViewPort *vp, int left, int top, int width, int height)
{
	ViewportDrawer vd;

	vd.dpi.zoom = vp->zoom;
	int mask = ScaleByZoom(-1, vp->zoom);

	vd.combine_sprites = SPRITE_COMBINE_NONE;

	vd.dpi.width  = ScaleByZoom (width,  vp->zoom);
	vd.dpi.height = ScaleByZoom (height, vp->zoom);
	vd.dpi.left = (ScaleByZoom (left, vp->zoom) + vp->virtual_left) & mask;
	vd.dpi.top  = (ScaleByZoom (top,  vp->zoom) + vp->virtual_top)  & mask;
	vd.dpi.surface = surface;
	vd.last_child = NULL;

	int x = left + vp->left;
	int y = top + vp->top;

	vd.dpi.dst_ptr = vd.dpi.surface->move (dst_ptr, x, y);

	ViewportAddLandscape (&vd, vp->zoom);
	ViewportAddVehicles (&vd, &vd.dpi);

	if (vd.tile_sprites_to_draw.Length() != 0) ViewportDrawTileSprites (&vd.dpi, &vd.tile_sprites_to_draw);

	uint nsprites = vd.parent_sprites_to_draw.Length();
	if (nsprites > 0) {
		std::vector <ParentSpriteToDraw*> sorted_parent_sprites (nsprites); ///< Parent sprite pointer array used for sorting

		ParentSpriteToDraw **sprites = &sorted_parent_sprites.front();
		ParentSpriteToDraw *psd_end = vd.parent_sprites_to_draw.End();
		for (ParentSpriteToDraw *it = vd.parent_sprites_to_draw.Begin(); it != psd_end; it++) {
			*sprites++ = it;
		}

		sprites = &sorted_parent_sprites.front();
		const ParentSpriteToDraw *const *end = sprites + nsprites;
		_vp_sprite_sorter (sprites, end);
		ViewportDrawParentSprites (&vd.dpi, sprites, end,
				vd.child_screen_sprites_to_draw.Begin());

		if (_draw_bounding_boxes) ViewportDrawBoundingBoxes (&vd.dpi, sprites, end);
	}

	if (_draw_dirty_blocks) ViewportDrawDirtyBlocks (&vd.dpi);

	BlitArea dp = vd.dpi;
	ZoomLevel zoom = vd.dpi.zoom;
	dp.width  = width;
	dp.height = height;

	if (vp->overlay != NULL && vp->overlay->GetCargoMask() != 0 && vp->overlay->GetCompanyMask() != 0) {
		/* translate to window coordinates */
		dp.left = x;
		dp.top = y;
		vp->overlay->Draw(&dp);
	}

	/* translate to world coordinates */
	dp.left = UnScaleByZoom (vd.dpi.left, zoom);
	dp.top  = UnScaleByZoom (vd.dpi.top,  zoom);

	ViewportAddTownNames (&dp, &vd.dpi);
	ViewportAddStationNames (&dp, &vd.dpi);
	ViewportAddSigns (&dp, &vd.dpi);

	DrawTextEffects (&dp, &vd.dpi);

	vd.tile_sprites_to_draw.Clear();
	vd.parent_sprites_to_draw.Clear();
	vd.child_screen_sprites_to_draw.Clear();
}

/**
 * Make sure we don't draw a too big area at a time.
 * If we do, the sprite memory will overflow.
 */
static void ViewportDrawChk (Blitter::Surface *surface,
	const ViewPort *vp, int left, int top, int width, int height)
{
	if (ScaleByZoom (height, vp->zoom) * ScaleByZoom (width, vp->zoom) > 180000 * ZOOM_LVL_BASE * ZOOM_LVL_BASE) {
		if (height > width) {
			int half = height >> 1;
			ViewportDrawChk (surface, vp, left, top, width, half);
			ViewportDrawChk (surface, vp, left, top + half, width, height - half);
		} else {
			int half = width >> 1;
			ViewportDrawChk (surface, vp, left, top, half, height);
			ViewportDrawChk (surface, vp, left + half, top, width - half, height);
		}
	} else {
		ViewportDoDraw (surface, surface->ptr, vp,
				left - vp->left, top - vp->top, width, height);
	}
}

/**
 * Draw the viewport of this window.
 */
void Window::DrawViewport (BlitArea *dpi) const
{
	const ViewPort *vp = this->viewport;

	int left = dpi->left + this->left;
	int top  = dpi->top  + this->top;

	assert (dpi->dst_ptr == dpi->surface->move (dpi->surface->ptr, left, top));

	int right  = left + dpi->width;
	int bottom = top  + dpi->height;

	if (right <= vp->left || bottom <= vp->top) return;

	if (left >= vp->left + vp->width) return;

	if (left < vp->left) left = vp->left;
	if (right > vp->left + vp->width) right = vp->left + vp->width;

	if (top >= vp->top + vp->height) return;

	if (top < vp->top) top = vp->top;
	if (bottom > vp->top + vp->height) bottom = vp->top + vp->height;

	ViewportDrawChk (dpi->surface, vp, left, top, right - left, bottom - top);
}

static int GetNearestHeight (int x, int y)
{
	if (x < 0) {
		x = 0;
	} else if ((uint)x >= MapSizeX()) {
		x = MapMaxX();
	}

	if (y < 0) {
		y = 0;
	} else if ((uint)y >= MapSizeY()) {
		y = MapMaxY();
	}

	return TileHeight (TileXY (x, y));
}

static inline void ClampViewportToMap(const ViewPort *vp, int &x, int &y)
{
	/* Centre of the viewport is hot spot */
	x += vp->virtual_width / 2;
	y += vp->virtual_height / 2;

	/* Convert viewport coordinates to map coordinates
	 * Calculation is scaled by 4 to avoid rounding errors */
	int vx = -x + y * 2;
	int vy =  x + y * 2;

	/* Compute the tile at that spot if it were at sea level. */
	int tx = vx >> (ZOOM_LVL_SHIFT + 6);
	int ty = vy >> (ZOOM_LVL_SHIFT + 6);

	/* Correct for tile height. */
	int h = 0;
	for (;;) {
		int hh = GetNearestHeight (tx, ty);
		if (hh < h + 4) {
			h = hh;
			break;
		}
		int d = 1 + (uint)(hh - h - 4) / 6;
		h += 4 * d;
		tx += d;
		ty += d;
	}

	/* Interpolate height. */
	{
		int xp = vx & ((1 << (ZOOM_LVL_SHIFT + 6)) - 1);
		int xq = (1 << (ZOOM_LVL_SHIFT + 6)) - xp;
		int yp = vy & ((1 << (ZOOM_LVL_SHIFT + 6)) - 1);
		int yq = (1 << (ZOOM_LVL_SHIFT + 6)) - yp;
		int c = xp * yq * (GetNearestHeight (tx + 1, ty) - h) +
			xq * yp * (GetNearestHeight (tx, ty + 1) - h) +
			xp * yp * (GetNearestHeight (tx + 1, ty + 1) - h);
		h *= ZOOM_LVL_BASE * TILE_SIZE;
		h += c >> (ZOOM_LVL_SHIFT + 8);
	}

	vx += h;
	vy += h;

	/* clamp to size of map */
	vx = Clamp(vx, 0, MapMaxX() * TILE_SIZE * 4 * ZOOM_LVL_BASE);
	vy = Clamp(vy, 0, MapMaxY() * TILE_SIZE * 4 * ZOOM_LVL_BASE);

	vx -= h;
	vy -= h;

	/* Convert map coordinates to viewport coordinates */
	x = (-vx + vy) / 2;
	y = ( vx + vy) / 4;

	/* Remove centering */
	x -= vp->virtual_width / 2;
	y -= vp->virtual_height / 2;
}

/**
 * Update the viewport position being displayed.
 * @param w %Window owning the viewport.
 */
void UpdateViewportPosition(Window *w)
{
	const ViewPort *vp = w->viewport;

	if (w->viewport->follow_vehicle != INVALID_VEHICLE) {
		const Vehicle *veh = Vehicle::Get(w->viewport->follow_vehicle);
		Point pt = MapXYZToViewport(vp, veh->x_pos, veh->y_pos, veh->z_pos);

		w->viewport->scrollpos_x = pt.x;
		w->viewport->scrollpos_y = pt.y;
		SetViewportPosition(w, pt.x, pt.y);
	} else {
		/* Ensure the destination location is within the map */
		ClampViewportToMap(vp, w->viewport->dest_scrollpos_x, w->viewport->dest_scrollpos_y);

		int delta_x = w->viewport->dest_scrollpos_x - w->viewport->scrollpos_x;
		int delta_y = w->viewport->dest_scrollpos_y - w->viewport->scrollpos_y;

		bool update_overlay = false;
		if (delta_x != 0 || delta_y != 0) {
			if (_settings_client.gui.smooth_scroll) {
				int max_scroll = ScaleByMapPerimeter(512 * ZOOM_LVL_BASE);
				/* Not at our desired position yet... */
				w->viewport->scrollpos_x += Clamp(delta_x / 4, -max_scroll, max_scroll);
				w->viewport->scrollpos_y += Clamp(delta_y / 4, -max_scroll, max_scroll);
			} else {
				w->viewport->scrollpos_x = w->viewport->dest_scrollpos_x;
				w->viewport->scrollpos_y = w->viewport->dest_scrollpos_y;
			}
			update_overlay = (w->viewport->scrollpos_x == w->viewport->dest_scrollpos_x &&
								w->viewport->scrollpos_y == w->viewport->dest_scrollpos_y);
		}

		ClampViewportToMap(vp, w->viewport->scrollpos_x, w->viewport->scrollpos_y);

		SetViewportPosition(w, w->viewport->scrollpos_x, w->viewport->scrollpos_y);
		if (update_overlay) RebuildViewportOverlay(w);
	}
}

/**
 * Marks a viewport as dirty for repaint if it displays (a part of) the area the needs to be repainted.
 * @param vp     The viewport to mark as dirty
 * @param left   Left edge of area to repaint
 * @param top    Top edge of area to repaint
 * @param right  Right edge of area to repaint
 * @param bottom Bottom edge of area to repaint
 * @ingroup dirty
 */
static void MarkViewportDirty(const ViewPort *vp, int left, int top, int right, int bottom)
{
	/* Rounding wrt. zoom-out level */
	right  += (1 << vp->zoom) - 1;
	bottom += (1 << vp->zoom) - 1;

	right -= vp->virtual_left;
	if (right <= 0) return;

	bottom -= vp->virtual_top;
	if (bottom <= 0) return;

	left = max(0, left - vp->virtual_left);

	if (left >= vp->virtual_width) return;

	top = max(0, top - vp->virtual_top);

	if (top >= vp->virtual_height) return;

	SetDirtyBlocks(
		UnScaleByZoomLower(left, vp->zoom) + vp->left,
		UnScaleByZoomLower(top, vp->zoom) + vp->top,
		UnScaleByZoom(right, vp->zoom) + vp->left + 1,
		UnScaleByZoom(bottom, vp->zoom) + vp->top + 1
	);
}

/**
 * Mark all viewports that display an area as dirty (in need of repaint).
 * @param left   Left   edge of area to repaint. (viewport coordinates, that is wrt. #ZOOM_LVL_NORMAL)
 * @param top    Top    edge of area to repaint. (viewport coordinates, that is wrt. #ZOOM_LVL_NORMAL)
 * @param right  Right  edge of area to repaint. (viewport coordinates, that is wrt. #ZOOM_LVL_NORMAL)
 * @param bottom Bottom edge of area to repaint. (viewport coordinates, that is wrt. #ZOOM_LVL_NORMAL)
 * @ingroup dirty
 */
void MarkAllViewportsDirty(int left, int top, int right, int bottom)
{
	Window *w;
	FOR_ALL_WINDOWS_FROM_BACK(w) {
		ViewPort *vp = w->viewport;
		if (vp != NULL) {
			assert(vp->width != 0);
			MarkViewportDirty(vp, left, top, right, bottom);
		}
	}
}

void ConstrainAllViewportsZoom()
{
	Window *w;
	FOR_ALL_WINDOWS_FROM_FRONT(w) {
		if (w->viewport == NULL) continue;
		ClampViewportZoom (w->viewport);
		/* Update the windows that have zoom-buttons to perhaps disable their buttons */
		w->InvalidateData();
	}
}

/**
 * Mark a tile given by its index dirty for repaint.
 * @param tile The tile to mark dirty.
 * @ingroup dirty
 */
void MarkTileDirtyByTile(TileIndex tile)
{
	Point pt = RemapCoords(TileX(tile) * TILE_SIZE, TileY(tile) * TILE_SIZE, TilePixelHeight(tile));
	MarkAllViewportsDirty(
			pt.x - MAX_TILE_EXTENT_LEFT,
			pt.y - MAX_TILE_EXTENT_TOP,
			pt.x + MAX_TILE_EXTENT_RIGHT,
			pt.y + MAX_TILE_EXTENT_BOTTOM);
}

/**
 * Mark a tile that has (or had) a bridge dirty for repaint.
 * @param tile The tile to mark dirty.
 * @param height The height of the bridge that is (or was) here.
 * @ingroup dirty
 */
void MarkBridgeTileDirtyByTile (TileIndex tile, uint bridge_height)
{
	Point pt = RemapCoords (TileX(tile) * TILE_SIZE, TileY(tile) * TILE_SIZE, bridge_height * TILE_HEIGHT);
	MarkAllViewportsDirty(
			pt.x - MAX_TILE_EXTENT_LEFT,
			pt.y - MAX_TILE_EXTENT_TOP,
			pt.x + MAX_TILE_EXTENT_RIGHT,
			pt.y + MAX_TILE_EXTENT_BOTTOM + (bridge_height - TileHeight(tile)) * ZOOM_LVL_BASE * TILE_HEIGHT);
}

/**
 * Mark a (virtual) tile outside the map dirty for repaint.
 * @param x Tile X position.
 * @param y Tile Y position.
 * @ingroup dirty
 */
void MarkTileDirtyByTileOutsideMap(int x, int y)
{
	Point pt = RemapCoords(x * TILE_SIZE, y * TILE_SIZE, GetVirtualHeight (x, y) * TILE_HEIGHT);
	MarkAllViewportsDirty(
			pt.x - MAX_TILE_EXTENT_LEFT,
			pt.y, // no buildings outside of map
			pt.x + MAX_TILE_EXTENT_RIGHT,
			pt.y + MAX_TILE_EXTENT_BOTTOM);
}

static void MarkTilesDirty (int x_start, int y_start, int x_end, int y_end)
{
	/* make sure everything is multiple of TILE_SIZE */
	assert ((x_end | y_end | x_start | y_start) % TILE_SIZE == 0);

	/* How it works:
	 * Suppose we have to mark dirty rectangle of 3x4 tiles:
	 *   x
	 *  xxx
	 * xxxxx
	 *  xxxxx
	 *   xxx
	 *    x
	 * This algorithm marks dirty columns of tiles, so it is done in 3+4-1 steps:
	 * 1)  x     2)  x
	 *    xxx       Oxx
	 *   Oxxxx     xOxxx
	 *    xxxxx     Oxxxx
	 *     xxx       xxx
	 *      x         x
	 * And so forth...
	 */

	int top_x = x_end; // coordinates of top dirty tile
	int top_y = y_start;
	int bot_x = top_x; // coordinates of bottom dirty tile
	int bot_y = top_y;

	do {
		/* topmost dirty point */
		TileIndex top_tile = TileVirtXY (top_x, top_y);
		Point top = RemapCoords (top_x, top_y, GetTileMaxPixelZ (top_tile));

		/* bottommost point */
		TileIndex bottom_tile = TileVirtXY (bot_x, bot_y);
		Point bot = RemapCoords (bot_x + TILE_SIZE, bot_y + TILE_SIZE, GetTilePixelZ(bottom_tile)); // bottommost point

		/* the 'x' coordinate of 'top' and 'bot' is the same (and always in the same distance from tile middle),
		 * tile height/slope affects only the 'y' on-screen coordinate! */

		int l = top.x - TILE_PIXELS * ZOOM_LVL_BASE; // 'x' coordinate of left   side of the dirty rectangle
		int t = top.y;                               // 'y' coordinate of top    side of the dirty rectangle
		int r = top.x + TILE_PIXELS * ZOOM_LVL_BASE; // 'x' coordinate of right  side of the dirty rectangle
		int b = bot.y;                               // 'y' coordinate of bottom side of the dirty rectangle

		static const int OVERLAY_WIDTH = 4 * ZOOM_LVL_BASE; // part of selection sprites is drawn outside the selected area (in particular: terraforming)

		/* For halftile foundations on SLOPE_STEEP_S the sprite extents some more towards the top */
		MarkAllViewportsDirty (l - OVERLAY_WIDTH, t - OVERLAY_WIDTH - TILE_HEIGHT * ZOOM_LVL_BASE, r + OVERLAY_WIDTH, b + OVERLAY_WIDTH);

		/* haven't we reached the topmost tile yet? */
		if (top_x != x_start) {
			top_x -= TILE_SIZE;
		} else {
			top_y += TILE_SIZE;
		}

		/* the way the bottom tile changes is different when we reach the bottommost tile */
		if (bot_y != y_end) {
			bot_y += TILE_SIZE;
		} else {
			bot_x -= TILE_SIZE;
		}
	} while (bot_x >= top_x);
}

static void MarkSquaredRadiusDirty (TileIndex xy, uint rr)
{
	uint r = 0;
	while (r*r < rr) r++;

	uint x = TileX (xy);
	int x0 = (r < x) ? x - r : 0;
	int x1 = (r < MapMaxX() - x) ? x + r : MapMaxX();

	uint y = TileY (xy);
	int y0 = (r < y) ? y - r : 0;
	int y1 = (r < MapMaxY() - y) ? y + r : MapMaxY();

	MarkTilesDirty (x0 * TILE_SIZE, y0 * TILE_SIZE, x1 * TILE_SIZE, y1 * TILE_SIZE);
}

void MarkTownAreaDirty (TownID town)
{
	Town *t = Town::Get (town);

	MarkSquaredRadiusDirty (t->xy, t->cache.squared_town_zone_radius[0]);
}

/**
 * Marks the selected tiles as dirty.
 *
 * This function marks the selected tiles as dirty for repaint
 *
 * @ingroup dirty
 */
static void SetSelectionTilesDirty()
{
	int x_size = _thd.size.x;
	int y_size = _thd.size.y;

	if (!_thd.diagonal) { // Selecting in a straight rectangle (or a single square)
		int x_start = _thd.pos.x;
		int y_start = _thd.pos.y;

		if (_thd.outersize.x != 0) {
			x_size  += _thd.outersize.x;
			x_start += _thd.offs.x;
			y_size  += _thd.outersize.y;
			y_start += _thd.offs.y;
		}

		x_size -= TILE_SIZE;
		y_size -= TILE_SIZE;

		assert(x_size >= 0);
		assert(y_size >= 0);

		int x_clamp = MapSizeX() * TILE_SIZE - TILE_SIZE;
		int y_clamp = MapSizeY() * TILE_SIZE - TILE_SIZE;

		MarkTilesDirty (Clamp (x_start, 0, x_clamp),
				Clamp (y_start, 0, y_clamp),
				Clamp (x_start + x_size, 0, x_clamp),
				Clamp (y_start + y_size, 0, y_clamp));
	} else { // Selecting in a 45 degrees rotated (diagonal) rectangle.
		/* a_size, b_size describe a rectangle with rotated coordinates */
		int a_size = x_size + y_size, b_size = x_size - y_size;

		int interval_a = a_size < 0 ? -(int)TILE_SIZE : (int)TILE_SIZE;
		int interval_b = b_size < 0 ? -(int)TILE_SIZE : (int)TILE_SIZE;

		for (int a = -interval_a; a != a_size + interval_a; a += interval_a) {
			for (int b = -interval_b; b != b_size + interval_b; b += interval_b) {
				uint x = (_thd.pos.x + (a + b) / 2) / TILE_SIZE;
				uint y = (_thd.pos.y + (a - b) / 2) / TILE_SIZE;

				if (x < MapMaxX() && y < MapMaxY()) {
					MarkTileDirtyByTile(TileXY(x, y));
				}
			}
		}
	}
}


void SetSelectionRed(bool b)
{
	_thd.make_square_red = b;
	SetSelectionTilesDirty();
}

/**
 * Test whether a sign is below the mouse
 * @param vp the clicked viewport
 * @param x X position of click
 * @param y Y position of click
 * @param sign the sign to check
 * @return true if the sign was hit
 */
static bool CheckClickOnViewportSign(const ViewPort *vp, int x, int y, const ViewportSign *sign)
{
	bool small = (vp->zoom >= ZOOM_LVL_OUT_16X);
	int sign_half_width = ScaleByZoom((small ? sign->width_small : sign->width_normal) / 2, vp->zoom);
	int sign_height = ScaleByZoom(VPSM_TOP + (small ? FONT_HEIGHT_SMALL : FONT_HEIGHT_NORMAL) + VPSM_BOTTOM, vp->zoom);

	return y >= sign->top && y < sign->top + sign_height &&
			x >= sign->center - sign_half_width && x < sign->center + sign_half_width;
}

static bool CheckClickOnTown(const ViewPort *vp, int x, int y)
{
	if (!HasBit(_display_opt, DO_SHOW_TOWN_NAMES)) return false;

	const Town *t;
	FOR_ALL_TOWNS(t) {
		if (CheckClickOnViewportSign(vp, x, y, &t->cache.sign)) {
			ShowTownViewWindow(t->index);
			return true;
		}
	}

	return false;
}

static bool CheckClickOnStation(const ViewPort *vp, int x, int y)
{
	if (!(HasBit(_display_opt, DO_SHOW_STATION_NAMES) || HasBit(_display_opt, DO_SHOW_WAYPOINT_NAMES)) || IsInvisibilitySet(TO_SIGNS)) return false;

	const BaseStation *st;
	FOR_ALL_BASE_STATIONS(st) {
		/* Check whether the base station is a station or a waypoint */
		bool is_station = !st->IsWaypoint();

		/* Don't check if the display options are disabled */
		if (!HasBit(_display_opt, is_station ? DO_SHOW_STATION_NAMES : DO_SHOW_WAYPOINT_NAMES)) continue;

		/* Don't check if competitor signs are not shown and the sign isn't owned by the local company */
		if (!HasBit(_display_opt, DO_SHOW_COMPETITOR_SIGNS) && _local_company != st->owner && st->owner != OWNER_NONE) continue;

		if (CheckClickOnViewportSign(vp, x, y, &st->sign)) {
			if (is_station) {
				ShowStationViewWindow(st->index);
			} else {
				ShowWaypointWindow(Waypoint::From(st));
			}
			return true;
		}
	}

	return false;
}


static bool CheckClickOnSign(const ViewPort *vp, int x, int y)
{
	/* Signs are turned off, or they are transparent and invisibility is ON, or company is a spectator */
	if (!HasBit(_display_opt, DO_SHOW_SIGNS) || IsInvisibilitySet(TO_SIGNS) || _local_company == COMPANY_SPECTATOR) return false;

	const Sign *si;
	FOR_ALL_SIGNS(si) {
		/* If competitor signs are hidden, don't check signs that aren't owned by local company */
		if (!HasBit(_display_opt, DO_SHOW_COMPETITOR_SIGNS) && _local_company != si->owner && si->owner != OWNER_DEITY) continue;
		if (si->owner == OWNER_DEITY && _game_mode != GM_EDITOR) continue;

		if (CheckClickOnViewportSign(vp, x, y, &si->sign)) {
			HandleClickOnSign(si);
			return true;
		}
	}

	return false;
}


static void PlaceObject()
{
	Point pt;
	Window *w;

	pt = GetTileBelowCursor();
	if (pt.x == -1) return;

	if (_pointer_mode == POINTER_CORNER) {
		pt.x += TILE_SIZE / 2;
		pt.y += TILE_SIZE / 2;
	}

	_tile_fract_coords.x = pt.x & TILE_UNIT_MASK;
	_tile_fract_coords.y = pt.y & TILE_UNIT_MASK;

	w = _thd.GetCallbackWnd();
	if (w != NULL) w->OnPlaceObject(pt, TileVirtXY(pt.x, pt.y));
}


bool HandleViewportClicked(const ViewPort *vp, int x, int y)
{
	x -= vp->left;
	y -= vp->top;

	assert ((uint)x < (uint)vp->width);
	assert ((uint)y < (uint)vp->height);

	x = ScaleByZoom (x, vp->zoom) + vp->virtual_left;
	y = ScaleByZoom (y, vp->zoom) + vp->virtual_top;

	const Vehicle *v = CheckClickOnVehicle (x, y);

	PointerMode mode = _pointer_mode;
	if (mode >= POINTER_VEHICLE) {
		if (v != NULL && VehicleClicked(v)) return true;
		mode = (PointerMode)(mode - POINTER_VEHICLE);
	}

	/* Vehicle placement mode already handled above. */
	if (mode != POINTER_NONE) {
		PlaceObject();
		return true;
	}

	if (CheckClickOnTown(vp, x, y)) return true;
	if (CheckClickOnStation(vp, x, y)) return true;
	if (CheckClickOnSign(vp, x, y)) return true;

	Point pt = TranslateXYToTileCoord (x, y);
	bool result = ClickTile (TileVirtXY (pt.x, pt.y));

	if (v != NULL) {
		DEBUG(misc, 2, "Vehicle %d (index %d) at %p", v->unitnumber, v->index, v);
		if (IsCompanyBuildableVehicleType(v)) {
			v = v->First();
			if (_ctrl_pressed && v->owner == _local_company) {
				StartStopVehicle(v, true);
			} else {
				ShowVehicleViewWindow(v);
			}
		}
		return true;
	}
	return result;
}

void RebuildViewportOverlay(Window *w)
{
	if (w->viewport->overlay != NULL &&
			w->viewport->overlay->GetCompanyMask() != 0 &&
			w->viewport->overlay->GetCargoMask() != 0) {
		w->viewport->overlay->RebuildCache();
		w->SetDirty();
	}
}

/**
 * Scrolls the viewport in a window to a given location.
 * @param x       Desired x location of the map to scroll to (world coordinate).
 * @param y       Desired y location of the map to scroll to (world coordinate).
 * @param z       Desired z location of the map to scroll to (world coordinate). Use \c -1 to scroll to the height of the map at the \a x, \a y location.
 * @param w       %Window containing the viewport.
 * @param instant Jump to the location instead of slowly moving to it.
 * @return Destination of the viewport was changed (to activate other actions when the viewport is already at the desired position).
 */
bool ScrollWindowTo(int x, int y, int z, Window *w, bool instant)
{
	/* The slope cannot be acquired outside of the map, so make sure we are always within the map. */
	if (z == -1) {
		if ((uint)x < MapSizeX() * TILE_SIZE && (uint)y < MapSizeY() * TILE_SIZE) {
			z = GetSlopePixelZ (x, y);
		} else {
			z = GetVirtualHeight (x / (int)TILE_SIZE, y / (int)TILE_SIZE) * TILE_HEIGHT;
		}
	}

	Point pt = MapXYZToViewport(w->viewport, x, y, z);
	w->viewport->follow_vehicle = INVALID_VEHICLE;

	if (w->viewport->dest_scrollpos_x == pt.x && w->viewport->dest_scrollpos_y == pt.y) return false;

	if (instant) {
		w->viewport->scrollpos_x = pt.x;
		w->viewport->scrollpos_y = pt.y;
		RebuildViewportOverlay(w);
	}

	w->viewport->dest_scrollpos_x = pt.x;
	w->viewport->dest_scrollpos_y = pt.y;
	return true;
}

/**
 * Scrolls the viewport in a window to a given location.
 * @param tile    Desired tile to center on.
 * @param w       %Window containing the viewport.
 * @param instant Jump to the location instead of slowly moving to it.
 * @return Destination of the viewport was changed (to activate other actions when the viewport is already at the desired position).
 */
bool ScrollWindowToTile(TileIndex tile, Window *w, bool instant)
{
	return ScrollWindowTo(TileX(tile) * TILE_SIZE, TileY(tile) * TILE_SIZE, -1, w, instant);
}

/**
 * Scrolls the viewport of the main window to a given location.
 * @param tile    Desired tile to center on.
 * @param instant Jump to the location instead of slowly moving to it.
 * @return Destination of the viewport was changed (to activate other actions when the viewport is already at the desired position).
 */
bool ScrollMainWindowToTile(TileIndex tile, bool instant)
{
	return ScrollMainWindowTo(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, -1, instant);
}

/**
 * Set a tile to display a red error square.
 * @param tile Tile that should show the red error square.
 */
void SetRedErrorSquare(TileIndex tile)
{
	TileIndex old;

	old = _thd.redsq;
	_thd.redsq = tile;

	if (tile != old) {
		if (tile != INVALID_TILE) MarkTileDirtyByTile(tile);
		if (old  != INVALID_TILE) MarkTileDirtyByTile(old);
	}
}

/**
 * Highlight \a w by \a h tiles at the cursor.
 * @param w Width of the highlighted tiles rectangle.
 * @param h Height of the highlighted tiles rectangle.
 */
void SetTileSelectSize(int w, int h)
{
	_thd.new_size.x = w * TILE_SIZE;
	_thd.new_size.y = h * TILE_SIZE;
	_thd.new_outersize.x = 0;
	_thd.new_outersize.y = 0;
}

void SetTileSelectBigSize(int ox, int oy, int sx, int sy)
{
	_thd.offs.x = ox * TILE_SIZE;
	_thd.offs.y = oy * TILE_SIZE;
	_thd.new_outersize.x = sx * TILE_SIZE;
	_thd.new_outersize.y = sy * TILE_SIZE;
}

/** returns the best autorail highlight type from map coordinates */
static HighLightStyle GetAutorailHT(int x, int y)
{
	return (HighLightStyle) _autorail_piece[x & TILE_UNIT_MASK][y & TILE_UNIT_MASK];
}

/**
 * Reset tile highlighting.
 */
void TileHighlightData::Reset()
{
	this->pos.x = 0;
	this->pos.y = 0;
	this->new_pos.x = 0;
	this->new_pos.y = 0;
}

/**
 * Is the user dragging a 'diagonal rectangle'?
 * @return User is dragging a rotated rectangle.
 */
bool TileHighlightData::IsDraggingDiagonal()
{
	return (this->select_method == VPM_X_AND_Y_ROTATED) && _ctrl_pressed && _left_button_down;
}

/**
 * Get the window that started the current highlighting.
 * @return The window that requested the current tile highlighting, or \c NULL if not available.
 */
Window *TileHighlightData::GetCallbackWnd()
{
	return FindWindowById(this->window_class, this->window_number);
}



/**
 * Displays the measurement tooltips when selecting multiple tiles
 * @param str String to be displayed
 * @param paramcount number of params to deal with
 * @param params (optional) up to 5 pieces of additional information that may be added to a tooltip
 * @param close_cond Condition for closing this tooltip.
 */
static inline void ShowMeasurementTooltips(StringID str, uint paramcount, const uint64 params[], TooltipCloseCondition close_cond = TCC_LEFT_CLICK)
{
	if (!_settings_client.gui.measure_tooltip) return;
	GuiShowTooltips(_thd.GetCallbackWnd(), str, paramcount, params, close_cond);
}

/**
 * Highlights all tiles between a set of two tiles. Used in dock and tunnel placement
 * @param from TileIndex of the first tile to highlight
 * @param to TileIndex of the last tile to highlight
 */
static void VpSetPresizeRange (TileIndex from, TileIndex to)
{
	uint64 distance = DistanceManhattan(from, to) + 1;

	_thd.selend.x = TileX(to) * TILE_SIZE;
	_thd.selend.y = TileY(to) * TILE_SIZE;
	_thd.selstart.x = TileX(from) * TILE_SIZE;
	_thd.selstart.y = TileY(from) * TILE_SIZE;
	_thd.next_drawstyle = HT_RECT;

	/* show measurement only if there is any length to speak of */
	if (distance > 1) ShowMeasurementTooltips(STR_MEASURE_LENGTH, 1, &distance, TCC_HOVER);
}

/**
 * Updates tile highlighting for all cases.
 * Uses _thd.selstart and _thd.selend (set elsewhere) to determine _thd.pos and _thd.size
 * Also drawstyle is determined. Uses _thd.new.* as a buffer and calls SetSelectionTilesDirty() twice,
 * Once for the old and once for the new selection.
 * _thd is TileHighlightData, found in viewport.h
 */
void UpdateTileSelection()
{
	HighLightStyle new_drawstyle = HT_NONE;
	bool new_diagonal = false;

	PointerMode mode = _pointer_mode;
	if (mode >= POINTER_VEHICLE) mode = (PointerMode)(mode - POINTER_VEHICLE);

	if ((mode == POINTER_AREA) || (_thd.select_method != VPM_NONE)) {
		if (mode == POINTER_AREA) {
			Window *w = _thd.GetCallbackWnd();
			if (w != NULL) {
				Point pt = GetTileBelowCursor();
				if (pt.x == -1) {
					_thd.selend.x = -1;
				} else {
					TileIndex tile = TileVirtXY (pt.x, pt.y);
					TileIndex tile2 = tile;
					w->OnPlacePresize (&tile, &tile2);
					VpSetPresizeRange (tile, tile2);
				}
			}
		}

		int x1 = _thd.selend.x;
		int y1 = _thd.selend.y;
		if (x1 != -1) {
			int x2 = _thd.selstart.x & ~TILE_UNIT_MASK;
			int y2 = _thd.selstart.y & ~TILE_UNIT_MASK;
			x1 &= ~TILE_UNIT_MASK;
			y1 &= ~TILE_UNIT_MASK;

			if (_thd.IsDraggingDiagonal()) {
				new_diagonal = true;
			} else {
				if (x1 >= x2) Swap(x1, x2);
				if (y1 >= y2) Swap(y1, y2);
			}
			_thd.new_pos.x = x1;
			_thd.new_pos.y = y1;
			_thd.new_size.x = x2 - x1;
			_thd.new_size.y = y2 - y1;
			if (!new_diagonal) {
				_thd.new_size.x += TILE_SIZE;
				_thd.new_size.y += TILE_SIZE;
			}
			new_drawstyle = _thd.next_drawstyle;
		}
	} else if ((mode != POINTER_NONE) && (mode != POINTER_DRAG)) {
		Point pt = GetTileBelowCursor();
		int x1 = pt.x;
		int y1 = pt.y;
		if (x1 != -1) {
			switch (mode) {
				case POINTER_TILE:
					new_drawstyle = HT_RECT;
					break;
				case POINTER_CORNER:
					new_drawstyle = HT_POINT;
					x1 += TILE_SIZE / 2;
					y1 += TILE_SIZE / 2;
					break;
				case POINTER_RAIL_AUTO:
					/* Draw one highlighted tile in any direction */
					new_drawstyle = GetAutorailHT(pt.x, pt.y);
					break;
				default:
					switch (mode) {
						case POINTER_RAIL_X: new_drawstyle = HT_RAIL_X; break;
						case POINTER_RAIL_Y: new_drawstyle = HT_RAIL_Y; break;

						case POINTER_RAIL_H:
							new_drawstyle = (pt.x & TILE_UNIT_MASK) + (pt.y & TILE_UNIT_MASK) <= TILE_SIZE ? HT_RAIL_HU : HT_RAIL_HL;
							break;

						case POINTER_RAIL_V:
							new_drawstyle = (pt.x & TILE_UNIT_MASK) > (pt.y & TILE_UNIT_MASK) ? HT_RAIL_VL : HT_RAIL_VR;
							break;

						default: NOT_REACHED();
					}
					_thd.selstart.x = x1 & ~TILE_UNIT_MASK;
					_thd.selstart.y = y1 & ~TILE_UNIT_MASK;
					break;
			}
			_thd.new_pos.x = x1 & ~TILE_UNIT_MASK;
			_thd.new_pos.y = y1 & ~TILE_UNIT_MASK;
		}
	}

	/* redraw selection */
	if (_thd.drawstyle != new_drawstyle ||
			_thd.pos.x != _thd.new_pos.x || _thd.pos.y != _thd.new_pos.y ||
			_thd.size.x != _thd.new_size.x || _thd.size.y != _thd.new_size.y ||
			_thd.outersize.x != _thd.new_outersize.x ||
			_thd.outersize.y != _thd.new_outersize.y ||
			_thd.diagonal    != new_diagonal) {
		/* Clear the old tile selection? */
		if (_thd.drawstyle != HT_NONE) SetSelectionTilesDirty();

		_thd.drawstyle = new_drawstyle;
		_thd.pos = _thd.new_pos;
		_thd.size = _thd.new_size;
		_thd.outersize = _thd.new_outersize;
		_thd.diagonal = new_diagonal;
		_thd.dirty = 0xff;

		/* Draw the new tile selection? */
		if (new_drawstyle != HT_NONE) SetSelectionTilesDirty();
	}
}

/** highlighting tiles while only going over them with the mouse */
void VpStartPlaceSizing (TileIndex tile, ViewportPlaceMethod method,
	int userdata, uint limit)
{
	assert (method != VPM_NONE);

	_thd.select_method = method;
	_thd.select_data   = userdata;
	_thd.selend.x = TileX(tile) * TILE_SIZE;
	_thd.selstart.x = TileX(tile) * TILE_SIZE;
	_thd.selend.y = TileY(tile) * TILE_SIZE;
	_thd.selstart.y = TileY(tile) * TILE_SIZE;

	/* Needed so several things (road, autoroad, bridges, ...) are placed correctly.
	 * In effect, placement starts from the centre of a tile
	 */
	if (method == VPM_X_OR_Y || method == VPM_X || method == VPM_Y) {
		_thd.selend.x += TILE_SIZE / 2;
		_thd.selend.y += TILE_SIZE / 2;
		_thd.selstart.x += TILE_SIZE / 2;
		_thd.selstart.y += TILE_SIZE / 2;
	}

	switch (_pointer_mode) {
		case POINTER_TILE:
			_thd.next_drawstyle = HT_RECT;
			break;

		case POINTER_CORNER:
			_thd.next_drawstyle = HT_POINT;
			break;

		default:
			assert (_pointer_mode >= POINTER_RAIL_FIRST);
			assert (_pointer_mode <= POINTER_RAIL_LAST);
			_thd.next_drawstyle = _thd.drawstyle;
			break;
	}

	/* You shouldn't be using this function if dragging is not possible. */
	assert (limit != 1);
	_thd.sizelimit = limit;
}

void VpSetPlaceSizingLimit (uint limit)
{
	_thd.sizelimit = limit;
}

/**
 * Check if the direction of start and end tile should be swapped based on
 * the dragging-style. Default directions are:
 * in the case of a line (HT_RAIL):  DIR_NE, DIR_NW, DIR_N, DIR_E
 * in the case of a rect (HT_RECT, HT_POINT): DIR_S, DIR_E
 * For example dragging a rectangle area from south to north should be swapped to
 * north-south (DIR_S) to obtain the same results with less code. This is what
 * the return value signifies.
 * @param style HighLightStyle dragging style
 * @param start_tile start tile of drag
 * @param end_tile end tile of drag
 * @return boolean value which when true means start/end should be swapped
 */
static bool SwapDirection(HighLightStyle style, TileIndex start_tile, TileIndex end_tile)
{
	uint start_x = TileX(start_tile);
	uint start_y = TileY(start_tile);
	uint end_x = TileX(end_tile);
	uint end_y = TileY(end_tile);

	switch (style) {
		case HT_NONE: return false;

		case HT_RECT:
		case HT_POINT: return (end_x != start_x && end_y < start_y);

		default: return (end_x > start_x || (end_x == start_x && end_y > start_y));
	}
}

/**
 * Calculates height difference between one tile and another.
 * Multiplies the result to suit the standard given by #TILE_HEIGHT_STEP.
 *
 * To correctly get the height difference we need the direction we are dragging
 * in, as well as with what kind of tool we are dragging. For example a horizontal
 * autorail tool that starts in bottom and ends at the top of a tile will need the
 * maximum of SW, S and SE, N corners respectively. This is handled by the lookup table below
 * See #_tileoffs_by_dir in map.cpp for the direction enums if you can't figure out the values yourself.
 * @param style      Highlighting style of the drag. This includes direction and style (autorail, rect, etc.)
 * @param distance   Number of tiles dragged, important for horizontal/vertical drags, ignored for others.
 * @param start_tile Start tile of the drag operation.
 * @param end_tile   End tile of the drag operation.
 * @return Height difference between two tiles. The tile measurement tool utilizes this value in its tooltip.
 */
static int CalcHeightdiff(HighLightStyle style, uint distance, TileIndex start_tile, TileIndex end_tile)
{
	bool swap = SwapDirection(style, start_tile, end_tile);
	uint h0, h1; // Start height and end height.

	if (start_tile == end_tile) return 0;
	if (swap) Swap(start_tile, end_tile);

	switch (style) {
		case HT_RECT:
			/* In the case of an area we can determine whether we were dragging south or
			 * east by checking the X-coordinates of the tiles */
			if (TileX(end_tile) > TileX(start_tile)) {
				start_tile = TILE_ADD(start_tile, TileDiffXY(0, 0));
				end_tile   = TILE_ADD(end_tile,   TileDiffXY(1, 1));
			} else {
				start_tile = TILE_ADD(start_tile, TileDiffXY(1, 0));
				end_tile   = TILE_ADD(end_tile,   TileDiffXY(0, 1));
			}
			/* FALL THROUGH */
		case HT_POINT:
			h0 = TileHeight(start_tile);
			h1 = TileHeight(end_tile);
			break;
		default: { // All other types, this is mostly only line/autorail
			static const Track flip_track [TRACK_END] = {
				TRACK_X, TRACK_Y, TRACK_LOWER, TRACK_UPPER, TRACK_RIGHT, TRACK_LEFT,
			};
			static const CoordDiff coorddiff_by_track[TRACK_END][2][2] = {
				/*  Start               End  */
				{ { {1, 0}, {1, 1} }, { {0, 1}, {0, 0} } }, // TRACK_X
				{ { {0, 1}, {1, 1} }, { {1, 0}, {0, 0} } }, // TRACK_Y
				{ { {1, 0}, {0, 0} }, { {0, 1}, {0, 0} } }, // TRACK_UPPER
				{ { {1, 0}, {1, 1} }, { {1, 1}, {0, 1} } }, // TRACK_LOWER
				{ { {1, 0}, {1, 1} }, { {1, 0}, {0, 0} } }, // TRACK_LEFT
				{ { {0, 1}, {1, 1} }, { {0, 0}, {0, 1} } }, // TRACK_RIGHT
			};

			bool even = ((distance % 2) == 0); // we're only interested in whether the distance is even or odd
			Track track = (Track)(style & HT_TRACK_MASK);
			assert (IsValidTrack (track));

			/* To handle autorail, we do some magic to be able to use a lookup table.
			 * Firstly if we drag the other way around, we switch start&end, and if needed
			 * also flip the drag-position. Eg if it was on the left, and the distance is even
			 * that means the end, which is now the start is on the right */
			if (swap && even) track = flip_track[track];

			/* Use lookup table for start-tile based on HighLightStyle direction */
			h0 = max (
				TileHeight (TILE_ADD (start_tile, ToTileIndexDiff (coorddiff_by_track[track][0][0]))),
				TileHeight (TILE_ADD (start_tile, ToTileIndexDiff (coorddiff_by_track[track][0][1])))
			);

			/* Use lookup table for end-tile based on HighLightStyle direction
			 * flip around side (lower/upper, left/right) based on distance */
			if (even) track = flip_track[track];
			h1 = max (
				TileHeight (TILE_ADD (end_tile, ToTileIndexDiff (coorddiff_by_track[track][1][0]))),
				TileHeight (TILE_ADD (end_tile, ToTileIndexDiff (coorddiff_by_track[track][1][1])))
			);
			break;
		}
	}

	if (swap) Swap(h0, h1);
	return (int)(h1 - h0) * TILE_HEIGHT_STEP;
}

static const StringID measure_strings_length[] = {STR_NULL, STR_MEASURE_LENGTH, STR_MEASURE_LENGTH_HEIGHTDIFF};

/**
 * Check for underflowing the map.
 * @param test  the variable to test for underflowing
 * @param other the other variable to update to keep the line
 * @param mult  the constant to multiply the difference by for \c other
 */
static void CheckUnderflow(int &test, int &other, int mult)
{
	if (test >= 0) return;

	other += mult * test;
	test = 0;
}

/**
 * Check for overflowing the map.
 * @param test  the variable to test for overflowing
 * @param other the other variable to update to keep the line
 * @param max   the maximum value for the \c test variable
 * @param mult  the constant to multiply the difference by for \c other
 */
static void CheckOverflow(int &test, int &other, int max, int mult)
{
	if (test <= max) return;

	other += mult * (test - max);
	test = max;
}

/** while dragging */
static void CalcRaildirsDrawstyle (int x, int y)
{
	HighLightStyle b;

	int dx = _thd.selstart.x - (_thd.selend.x & ~TILE_UNIT_MASK);
	int dy = _thd.selstart.y - (_thd.selend.y & ~TILE_UNIT_MASK);

	assert_compile (POINTER_RAIL_LAST == POINTER_RAIL_AUTO);

	switch (_pointer_mode) {
		case POINTER_RAIL_X:
			b = HT_RAIL_X;
			y = _thd.selstart.y;
			break;

		case POINTER_RAIL_Y:
			b = HT_RAIL_Y;
			x = _thd.selstart.x;
			break;

		case POINTER_RAIL_H: {
			int d = dx + dy;
			if (d == 0) {
				/* We are on a straight horizontal line. Determine the 'rail'
				 * to build based the sub tile location. */
				b = (x & TILE_UNIT_MASK) + (y & TILE_UNIT_MASK) >= TILE_SIZE ? HT_RAIL_HL : HT_RAIL_HU;
				break;
			}

			/* We are not on a straight line. Determine the rail to build
			 * based on whether we are above or below it. */
			b = d >= (int)TILE_SIZE ? HT_RAIL_HU : HT_RAIL_HL;

			/* Calculate where a horizontal line through the start point and
			 * a vertical line from the selected end point intersect and
			 * use that point as the end point. */
			int raw_dx = _thd.selstart.x - _thd.selend.x;
			int raw_dy = _thd.selstart.y - _thd.selend.y;
			int offset = (raw_dx - raw_dy) / 2;
			x = _thd.selstart.x - (offset & ~TILE_UNIT_MASK);
			y = _thd.selstart.y + (offset & ~TILE_UNIT_MASK);

			/* 'Build' the last half rail tile if needed */
			if ((offset & TILE_UNIT_MASK) > (TILE_SIZE / 2)) {
				if (d >= (int)TILE_SIZE) {
					x -= TILE_SIZE;
				} else {
					y += (d < 0) ? (int)TILE_SIZE : -(int)TILE_SIZE;
				}
			}

			/* Make sure we do not overflow the map! */
			CheckUnderflow (x, y, 1);
			CheckUnderflow (y, x, 1);
			CheckOverflow (x, y, (MapMaxX() - 1) * TILE_SIZE, 1);
			CheckOverflow (y, x, (MapMaxY() - 1) * TILE_SIZE, 1);
			assert (x >= 0 && y >= 0 && x <= (int)(MapMaxX() * TILE_SIZE) && y <= (int)(MapMaxY() * TILE_SIZE));

			break;
		}

		case POINTER_RAIL_V: {
			int d = dx - dy;
			if (d == 0) {
				/* We are on a straight vertical line. Determine the 'rail'
				 * to build based the sub tile location. */
				b = (x & TILE_UNIT_MASK) > (y & TILE_UNIT_MASK) ? HT_RAIL_VL : HT_RAIL_VR;
				break;
			}

			/* We are not on a straight line. Determine the rail to build
			 * based on whether we are left or right from it. */
			b = d < 0 ? HT_RAIL_VL : HT_RAIL_VR;

			/* Calculate where a vertical line through the start point and
			 * a horizontal line from the selected end point intersect and
			 * use that point as the end point. */
			int raw_dx = _thd.selstart.x - _thd.selend.x;
			int raw_dy = _thd.selstart.y - _thd.selend.y;
			int offset = (raw_dx + raw_dy + (int)TILE_SIZE) / 2;
			x = _thd.selstart.x - (offset & ~TILE_UNIT_MASK);
			y = _thd.selstart.y - (offset & ~TILE_UNIT_MASK);

			/* 'Build' the last half rail tile if needed */
			if ((offset & TILE_UNIT_MASK) > (TILE_SIZE / 2)) {
				if (d < 0) {
					y -= TILE_SIZE;
				} else {
					x -= TILE_SIZE;
				}
			}

			/* Make sure we do not overflow the map! */
			CheckUnderflow (x, y, -1);
			CheckUnderflow (y, x, -1);
			CheckOverflow (x, y, (MapMaxX() - 1) * TILE_SIZE, -1);
			CheckOverflow (y, x, (MapMaxY() - 1) * TILE_SIZE, -1);
			assert (x >= 0 && y >= 0 && x <= (int)(MapMaxX() * TILE_SIZE) && y <= (int)(MapMaxY() * TILE_SIZE));

			break;
		}

		default:
			uint w = abs(dx) + TILE_SIZE;
			uint h = abs(dy) + TILE_SIZE;

			if (dx == 0 && dy == 0) { // check if we're only within one tile
				if (_pointer_mode == POINTER_RAIL_AUTO) {
					b = GetAutorailHT (x, y);
				} else { // rect for autosignals on one tile
					b = HT_RECT;
				}

			} else if (dx == 0 || dy == 0) { // Is this in X or Y direction?
				int fxpy = _tile_fract_coords.x + _tile_fract_coords.y;
				int sxpy = (_thd.selend.x & TILE_UNIT_MASK) + (_thd.selend.y & TILE_UNIT_MASK);
				int fxmy = _tile_fract_coords.x - _tile_fract_coords.y;
				int sxmy = (_thd.selend.x & TILE_UNIT_MASK) - (_thd.selend.y & TILE_UNIT_MASK);

				if (dy == 0) {
					if (dx == (int)TILE_SIZE) { // 2x1 special handling
						b = (fxmy < -3 && sxmy > 3) ? HT_RAIL_VR :
							(fxpy <= 12 && sxpy >= 20) ? HT_RAIL_HU :
							HT_RAIL_X;
					} else if (dx == -(int)TILE_SIZE) {
						b = (fxmy > 3 && sxmy < -3) ? HT_RAIL_VL :
							(fxpy >= 20 && sxpy <= 12) ? HT_RAIL_HL :
							HT_RAIL_X;
					} else {
						b = HT_RAIL_X;
					}
					y = _thd.selstart.y;
				} else {
					if (dy == (int)TILE_SIZE) { // 2x1 special handling
						b = (fxmy > 3 && sxmy < -3) ? HT_RAIL_VL :
							(fxpy <= 12 && sxpy >= 20) ? HT_RAIL_HU :
							HT_RAIL_Y;
					} else if (dy == -(int)TILE_SIZE) { // 2x1 other direction
						b = (fxmy < -3 && sxmy > 3) ? HT_RAIL_VR :
							(fxpy >= 20 && sxpy <= 12) ? HT_RAIL_HL :
							HT_RAIL_Y;
					} else {
						b = HT_RAIL_Y;
					}
					x = _thd.selstart.x;
				}

			} else if (w > h * 2) { // still count as x dir?
				b = HT_RAIL_X;
				y = _thd.selstart.y;

			} else if (h > w * 2) { // still count as y dir?
				b = HT_RAIL_Y;
				x = _thd.selstart.x;

			} else { // complicated direction
				int d = w - h;
				_thd.selend.x = _thd.selend.x & ~TILE_UNIT_MASK;
				_thd.selend.y = _thd.selend.y & ~TILE_UNIT_MASK;

				/* four cases. */
				bool xpos = x > _thd.selstart.x;
				bool ypos = y > _thd.selstart.y;
				if (d == 0) {
					uint xm = x & TILE_UNIT_MASK;
					uint ym = y & TILE_UNIT_MASK;
					b = (xpos == ypos) ?
							(xm > ym ? HT_RAIL_VL : HT_RAIL_VR) :
							(xm + ym >= TILE_SIZE ? HT_RAIL_HL : HT_RAIL_HU);
				} else if (xpos) {
					if (ypos) {
						/* south */
						if (d > 0) {
							x = _thd.selstart.x + h;
							b = HT_RAIL_VL;
						} else {
							y = _thd.selstart.y + w;
							b = HT_RAIL_VR;
						}
					} else {
						/* west */
						if (d > 0) {
							x = _thd.selstart.x + h;
							b = HT_RAIL_HL;
						} else {
							y = _thd.selstart.y - w;
							b = HT_RAIL_HU;
						}
					}
				} else {
					if (ypos) {
						/* east */
						if (d > 0) {
							x = _thd.selstart.x - h;
							b = HT_RAIL_HU;
						} else {
							y = _thd.selstart.y + w;
							b = HT_RAIL_HL;
						}
					} else {
						/* north */
						if (d > 0) {
							x = _thd.selstart.x - h;
							b = HT_RAIL_VR;
						} else {
							y = _thd.selstart.y - w;
							b = HT_RAIL_VL;
						}
					}
				}
			}
	}

	if (_settings_client.gui.measure_tooltip) {
		TileIndex t0 = TileVirtXY(_thd.selstart.x, _thd.selstart.y);
		TileIndex t1 = TileVirtXY(x, y);
		uint distance = DistanceManhattan(t0, t1) + 1;
		byte index = 0;
		uint64 params[2];

		if (distance != 1) {
			int heightdiff = CalcHeightdiff(b, distance, t0, t1);
			/* If we are showing a tooltip for horizontal or vertical drags,
			 * 2 tiles have a length of 1. To bias towards the ceiling we add
			 * one before division. It feels more natural to count 3 lengths as 2 */
			if (!IsDiagonalTrack ((Track)(b & HT_TRACK_MASK))) {
				distance = CeilDiv(distance, 2);
			}

			params[index++] = distance;
			if (heightdiff != 0) params[index++] = heightdiff;
		}

		ShowMeasurementTooltips(measure_strings_length[index], index, params);
	}

	_thd.selend.x = x;
	_thd.selend.y = y;
	_thd.next_drawstyle = b;
}

/**
 * Selects tiles while dragging
 * @param x X coordinate of end of selection
 * @param y Y coordinate of end of selection
 * @param method modifies the way tiles are selected. Possible
 * methods are VPM_* in viewport.h
 */
static void VpSelectTilesWithMethod (int x, int y, ViewportPlaceMethod method)
{
	assert (method != VPM_NONE);

	int sx, sy;
	HighLightStyle style;

	if (x == -1) {
		_thd.selend.x = -1;
		return;
	}

	/* Special handling of drag in any (8-way) direction */
	if (method == VPM_RAILDIRS) {
		_thd.selend.x = x;
		_thd.selend.y = y;
		CalcRaildirsDrawstyle (x, y);
		return;
	}

	/* Needed so level-land is placed correctly */
	if (_thd.next_drawstyle == HT_POINT) {
		x += TILE_SIZE / 2;
		y += TILE_SIZE / 2;
	}

	sx = _thd.selstart.x;
	sy = _thd.selstart.y;

	int limit = (_thd.sizelimit != 0) ? (_thd.sizelimit - 1) * TILE_SIZE : 0;
	/* Limited size does not work with rotation. */
	assert ((limit == 0) || (method != VPM_X_AND_Y_ROTATED));

	switch (method) {
		case VPM_X_OR_Y: // drag in X or Y direction
			if (abs(sy - y) < abs(sx - x)) {
				y = sy;
				style = HT_RAIL_X;
			} else {
				x = sx;
				style = HT_RAIL_Y;
			}
			goto calc_heightdiff_single_direction;

		case VPM_Y: // drag in Y direction
			x = sx;
			style = HT_RAIL_Y;
			goto calc_heightdiff_single_direction;

		case VPM_X: // drag in X direction
			y = sy;
			style = HT_RAIL_X;

calc_heightdiff_single_direction:;
			if (limit > 0) {
				x = sx + Clamp(x - sx, -limit, limit);
				y = sy + Clamp(y - sy, -limit, limit);
			}
			if (_settings_client.gui.measure_tooltip) {
				TileIndex t0 = TileVirtXY(sx, sy);
				TileIndex t1 = TileVirtXY(x, y);
				uint distance = DistanceManhattan(t0, t1) + 1;
				byte index = 0;
				uint64 params[2];

				if (distance != 1) {
					/* With current code passing a HT_RAIL style to calculate the height
					 * difference is enough. However if/when a point-tool is created
					 * with this method, function should be called with new_style (below)
					 * instead of HT_RAIL | style case HT_POINT is handled specially
					 * new_style := (_thd.next_drawstyle & HT_RECT) ? HT_RAIL | style : _thd.next_drawstyle; */
					int heightdiff = CalcHeightdiff (style, 0, t0, t1);

					params[index++] = distance;
					if (heightdiff != 0) params[index++] = heightdiff;
				}

				ShowMeasurementTooltips(measure_strings_length[index], index, params);
			}
			break;

		case VPM_X_AND_Y: // drag an X by Y area
			if (limit > 0) {
				x = sx + Clamp (x - sx, -limit, limit);
				y = sy + Clamp (y - sy, -limit, limit);
			}
			/* fall through */
		case VPM_X_AND_Y_ROTATED:
			if (_settings_client.gui.measure_tooltip) {
				static const StringID measure_strings_area[] = {
					STR_NULL, STR_NULL, STR_MEASURE_AREA, STR_MEASURE_AREA_HEIGHTDIFF
				};

				TileIndex t0 = TileVirtXY(sx, sy);
				TileIndex t1 = TileVirtXY(x, y);
				uint dx = Delta(TileX(t0), TileX(t1)) + 1;
				uint dy = Delta(TileY(t0), TileY(t1)) + 1;
				byte index = 0;
				uint64 params[3];

				/* If dragging an area (eg dynamite tool) and it is actually a single
				 * row/column, change the type to 'line' to get proper calculation for height */
				style = (HighLightStyle)_thd.next_drawstyle;
				if (_thd.IsDraggingDiagonal()) {
					/* Determine the "area" of the diagonal dragged selection.
					 * We assume the area is the number of tiles along the X
					 * edge and the number of tiles along the Y edge. However,
					 * multiplying these two numbers does not give the exact
					 * number of tiles; basically we are counting the black
					 * squares on a chess board and ignore the white ones to
					 * make the tile counts at the edges match up. There is no
					 * other way to make a proper count though.
					 *
					 * First convert to the rotated coordinate system. */
					int dist_x = TileX(t0) - TileX(t1);
					int dist_y = TileY(t0) - TileY(t1);
					int a_max = dist_x + dist_y;
					int b_max = dist_y - dist_x;

					/* Now determine the size along the edge, but due to the
					 * chess board principle this counts double. */
					a_max = abs(a_max + (a_max > 0 ? 2 : -2)) / 2;
					b_max = abs(b_max + (b_max > 0 ? 2 : -2)) / 2;

					/* We get a 1x1 on normal 2x1 rectangles, due to it being
					 * a seen as two sides. As the result for actual building
					 * will be the same as non-diagonal dragging revert to that
					 * behaviour to give it a more normally looking size. */
					if (a_max != 1 || b_max != 1) {
						dx = a_max;
						dy = b_max;
					}
				} else if (style == HT_RECT) {
					if (dx == 1) {
						style = HT_RAIL_Y;
					} else if (dy == 1) {
						style = HT_RAIL_X;
					}
				}

				if (dx != 1 || dy != 1) {
					int heightdiff = CalcHeightdiff(style, 0, t0, t1);

					params[index++] = dx - (style & HT_POINT ? 1 : 0);
					params[index++] = dy - (style & HT_POINT ? 1 : 0);
					if (heightdiff != 0) params[index++] = heightdiff;
				}

				ShowMeasurementTooltips(measure_strings_area[index], index, params);
			}
			break;

		default: NOT_REACHED();
	}

	_thd.selend.x = x;
	_thd.selend.y = y;
}

/** Abort the current dragging operation, if any. */
void VpStopPlaceSizing (void)
{
	_thd.select_method = VPM_NONE;
	SetTileSelectSize(1, 1);
}

/**
 * Handle the mouse while dragging for placement/resizing.
 * @return State of handling the event.
 */
EventState VpHandlePlaceSizingDrag()
{
	if (_thd.select_method == VPM_NONE) return ES_NOT_HANDLED;

	/* stop drag mode if the window has been closed */
	Window *w = _thd.GetCallbackWnd();
	if (w == NULL) {
		ResetPointerMode();
		return ES_HANDLED;
	}

	/* while dragging execute the drag procedure of the corresponding window */
	if (_left_button_down) {
		Point pt = GetTileBelowCursor();
		if (w->OnPlaceDrag (_thd.select_data, pt)) {
			VpSelectTilesWithMethod (pt.x, pt.y, _thd.select_method);
		}
		return ES_HANDLED;
	}

	/* mouse button released..
	 * keep the selected tool, but reset it to the original mode. */
	VpStopPlaceSizing();

	w->OnPlaceMouseUp (_thd.select_data, _thd.selend, TileVirtXY(_thd.selstart.x, _thd.selstart.y), TileVirtXY(_thd.selend.x, _thd.selend.y));

	return ES_HANDLED;
}

#include "table/animcursors.h"

/**
 * Change the cursor and mouse click/drag handling to a mode for performing special operations like tile area selection, object placement, etc.
 * @param mode Mode to perform.
 * @param window_class %Window class of the window requesting the mode change.
 * @param window_num Number of the window in its class requesting the mode change.
 * @param icon New shape of the mouse cursor.
 */
void SetPointerMode (PointerMode mode, WindowClass window_class,
	WindowNumber window_num, CursorID icon)
{
	if (_thd.window_class != WC_INVALID) {
		/* Undo clicking on button and drag & drop */
		Window *w = _thd.GetCallbackWnd();
		/* Call the abort function, but set the window class to something
		 * that will never be used to avoid infinite loops. Setting it to
		 * the 'next' window class must not be done because recursion into
		 * this function might in some cases reset the newly set object to
		 * place or not properly reset the original selection. */
		_thd.window_class = WC_INVALID;
		if (w != NULL) w->OnPlaceObjectAbort();
	}

	/* Mark the old selection dirty, in case the selection shape or colour changes */
	if (_thd.drawstyle != HT_NONE) SetSelectionTilesDirty();

	SetTileSelectSize(1, 1);

	_thd.make_square_red = false;

	_pointer_mode = mode;

	_thd.window_class = window_class;
	_thd.window_number = window_num;

	if (mode == POINTER_AREA) { // special tools, like tunnels or docks start with presizing mode
		_thd.selend.x = -1;
	}

	if ((icon & ANIMCURSOR_FLAG) != 0) {
		SetAnimatedMouseCursor(_animcursors[icon & ~ANIMCURSOR_FLAG]);
	} else {
		SetMouseCursor (icon);
	}

}

/** Reset the cursor and mouse mode handling back to default (normal cursor, only clicking in windows). */
void ResetPointerMode (void)
{
	SetPointerMode (POINTER_NONE, WC_MAIN_WINDOW, 0, SPR_CURSOR_MOUSE);
}

Point GetViewportStationMiddle(const ViewPort *vp, const Station *st)
{
	int x = TileX(st->xy) * TILE_SIZE;
	int y = TileY(st->xy) * TILE_SIZE;
	int z = GetSlopePixelZ(Clamp(x, 0, MapSizeX() * TILE_SIZE - 1), Clamp(y, 0, MapSizeY() * TILE_SIZE - 1));

	Point p = RemapCoords(x, y, z);
	p.x = UnScaleByZoom(p.x - vp->virtual_left, vp->zoom) + vp->left;
	p.y = UnScaleByZoom(p.y - vp->virtual_top, vp->zoom) + vp->top;
	return p;
}
