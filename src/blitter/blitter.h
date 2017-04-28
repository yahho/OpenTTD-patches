/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file blitter.h Blitter code interface. */

#ifndef BLITTER_H
#define BLITTER_H

#include <vector>

#include "../core/pointer.h"
#include "../string.h"
#include "../gfx_type.h"

/** Data structure describing a sprite. */
struct Sprite {
	uint16 height; ///< Height of the sprite.
	uint16 width;  ///< Width of the sprite.
	int16 x_offs;  ///< Number of pixels to shift the sprite to the right.
	int16 y_offs;  ///< Number of pixels to shift the sprite downwards.
};

/** The modes of blitting we can do. */
enum BlitterMode {
	BM_NORMAL,       ///< Perform the simple blitting.
	BM_COLOUR_REMAP, ///< Perform a colour remapping.
	BM_TRANSPARENT,  ///< Perform transparency colour remapping.
	BM_CRASH_REMAP,  ///< Perform a crash remapping.
	BM_BLACK_REMAP,  ///< Perform remapping to a completely blackened sprite
};

/**
 * How all blitters should look like. Extend this class to make your own.
 */
class Blitter {
public:
	typedef void *AllocatorProc (size_t size);

	/** Structure for a raw sprite to encode. */
	struct RawSprite {
		/** Definition of a raw pixel. */
		struct Pixel {
			uint8 r;  ///< Red-channel
			uint8 g;  ///< Green-channel
			uint8 b;  ///< Blue-channel
			uint8 a;  ///< Alpha-channel
			uint8 m;  ///< Remap-channel
		};

		Pixel *data;    ///< The sprite itself
		uint16 height;  ///< Height of the sprite
		uint16 width;   ///< Width of the sprite
		int16  x_offs;  ///< The x-offset of where the sprite will be drawn
		int16  y_offs;  ///< The y-offset of where the sprite will be drawn
	};

	/** Parameters related to blitting. */
	struct BlitterParams {
		const Sprite *sprite; ///< Pointer to the sprite how ever the encoder stored it
		const byte *remap;  ///< XXX -- Temporary storage for remap array

		int skip_left;      ///< How much pixels of the source to skip on the left (based on zoom of dst)
		int skip_top;       ///< How much pixels of the source to skip on the top (based on zoom of dst)
		int width;          ///< The width in pixels that needs to be drawn to dst
		int height;         ///< The height in pixels that needs to be drawn to dst
		int left;           ///< The left offset in the 'dst' in pixels to start drawing
		int top;            ///< The top offset in the 'dst' in pixels to start drawing

		void *dst;          ///< Destination buffer
		int pitch;          ///< The pitch of the destination buffer
	};

	/** Types of palette animation. */
	enum PaletteAnimation {
		PALETTE_ANIMATION_NONE,           ///< No palette animation
		PALETTE_ANIMATION_VIDEO_BACKEND,  ///< Palette animation should be done by video backend (8bpp only!)
		PALETTE_ANIMATION_BLITTER,        ///< The blitter takes care of the palette animation
	};

	/** Buffer to keep a copy of a part of a surface. */
	struct Buffer {
		std::vector<byte> data;
		uint width, height;

		Buffer (void) : data(), width(0), height(0)
		{
		}

		void resize (uint width, uint height, uint n)
		{
			uint count = width * height * n;
			if (count > this->data.size()) {
				this->data.resize (count);
			}
		}
	};

	/** Check if this blitter is usable. */
	static bool usable (void)
	{
		return true;
	}

	/** Helper function to allocate a sprite in Encode. */
	template <typename T>
	static T *AllocateSprite (const RawSprite *sprite,
		AllocatorProc *allocator, size_t extra = 0)
	{
		T *s = (T *) allocator (sizeof(T) + extra);

		s->height = sprite->height;
		s->width  = sprite->width;
		s->x_offs = sprite->x_offs;
		s->y_offs = sprite->y_offs;

		return s;
	}

	/** Blitting surface. */
	struct Surface {
		void *const ptr;    ///< Pixel data
		const uint width;   ///< Surface width
		const uint height;  ///< Surface height
		const uint pitch;   ///< Surface pitch

		Surface (void *ptr, uint width, uint height, uint pitch)
			: ptr(ptr), width(width), height(height), pitch(pitch)
		{
		}

		virtual ~Surface()
		{
		}

		/** Helper function to implement move below. */
		template <typename T, typename P, typename X, typename Y>
		T *movep (P *p, X x, Y y)
		{
			return (T*) p + x + y * (int) this->pitch;
		}

		/**
		 * Move the destination pointer the requested amount x and y,
		 * keeping in mind any pitch and bpp of the renderer.
		 * @param video The destination pointer (video-buffer) to scroll.
		 * @param x How much you want to scroll to the right.
		 * @param y How much you want to scroll to the bottom.
		 * @return A new destination pointer moved to the requested place.
		 */
		virtual void *move (void *video, int x, int y) = 0;

		/**
		 * Draw a pixel with a given colour on the video-buffer.
		 * @param video The destination pointer (video-buffer).
		 * @param x The x position within video-buffer.
		 * @param y The y position within video-buffer.
		 * @param colour A 8bpp mapping colour.
		 */
		virtual void set_pixel (void *video, int x, int y, uint8 colour) = 0;

		/**
		 * Draw a line with a given colour.
		 * @param video The destination pointer (video-buffer).
		 * @param x The x coordinate from where the line starts.
		 * @param y The y coordinate from where the line starts.
		 * @param x2 The x coordinate to where the line goes.
		 * @param y2 The y coordinate to where the lines goes.
		 * @param screen_width The width of the screen you are drawing in (to avoid buffer-overflows).
		 * @param screen_height The height of the screen you are drawing in (to avoid buffer-overflows).
		 * @param colour A 8bpp mapping colour.
		 * @param width Line width.
		 * @param dash Length of dashes for dashed lines. 0 means solid line.
		 */
		virtual void draw_line (void *video, int x, int y, int x2, int y2, int screen_width, int screen_height, uint8 colour, int width, int dash = 0);

		/**
		 * Make a single horizontal line in a single colour on the video-buffer.
		 * @param video The destination pointer (video-buffer).
		 * @param width The length of the line.
		 * @param height The height of the line.
		 * @param colour A 8bpp mapping colour.
		 */
		virtual void draw_rect (void *video, int width, int height, uint8 colour) = 0;

		/**
		 * Draw a colourtable to the screen. This is: the colour of
		 * the screen is read and is looked-up in the palette to match
		 * a new colour, which then is put on the screen again.
		 * @param dst the destination pointer (video-buffer).
		 * @param width the width of the buffer.
		 * @param height the height of the buffer.
		 * @param pal the palette to use.
		 */
		virtual void recolour_rect (void *video, int width, int height, PaletteID pal) = 0;

		/**
		 * Draw a checker pattern over a rectangle.
		 * @param video The destination pointer (video-buffer).
		 * @param width The width of the rectangle to fill.
		 * @param height The height of the rectangle to fill.
		 * @param colour A 8bpp mapping colour.
		 * @param bo Whether the pattern covers the top left corner (0 or 1).
		 */
		virtual void draw_checker (void *video, uint width, uint height, uint8 colour, byte bo) = 0;

		/**
		 * Scroll the videobuffer some 'x' and 'y' value.
		 * @param left The left value of the screen to scroll.
		 * @param top The top value of the screen to scroll.
		 * @param width The width of the screen to scroll.
		 * @param height The height of the screen to scroll.
		 * @param dx How much to scroll in X.
		 * @param dy How much to scroll in Y.
		 */
		virtual void scroll (int left, int top, int width, int height, int dx, int dy) = 0;

		/**
		 * Called when the 8bpp palette is changed; you should redraw
		 * all pixels on the screen that are equal to the 8bpp palette
		 * indices 'first_dirty' to 'first_dirty + count_dirty'.
		 * @param palette The new palette.
		 * @return Whether the screen should be invalidated.
		 */
		virtual bool palette_animate (const Palette &palette);

		/** Draw an image to the screen, given an amount of params defined above. */
		virtual void draw (const BlitterParams *bp, BlitterMode mode, ZoomLevel zoom) = 0;

		/**
		 * Copy from the screen to a buffer.
		 * @param dst The buffer in which the data will be stored.
		 * @param x The x position within video-buffer.
		 * @param y The y position within video-buffer.
		 * @param width The width of the buffer.
		 * @param height The height of the buffer.
		 * @note You can not do anything with the content of the buffer, as the blitter can store non-pixel data in it too!
		 */
		virtual void copy (Buffer *dst, int x, int y, uint width, uint height) = 0;

		/**
		 * Copy from a buffer to the screen.
		 * @param src The buffer from which the data will be read.
		 * @param x The x position within video-buffer.
		 * @param y The y position within video-buffer.
		 * @note You can not do anything with the content of the buffer, as the blitter can store non-pixel data in it too!
		 */
		virtual void paste (const Buffer *src, int x, int y) = 0;

		/**
		 * Copy from the screen to a buffer in a palette format for 8bpp and RGBA format for 32bpp.
		 * @param dst The buffer in which the data will be stored.
		 * @param dst_pitch The pitch (byte per line) of the destination buffer.
		 * @param y The first line to copy.
		 * @param height The number of lines to copy.
		 */
		virtual void export_lines (void *dst, uint dst_pitch, uint y, uint height) = 0;
	};

	/** Static per-blitter data. */
	struct Info {
		const char *name;           ///< The name of the blitter.
		const char *desc;           ///< Description of the blitter.
		bool (*usable) (void);      ///< Usability check function.
		Surface* (*create) (void *ptr, uint width, uint height,
			uint pitch, bool anim); ///< Surface creation function.
		Sprite* (*encode) (const RawSprite *sprite, bool is_font,
			AllocatorProc *allocator);  ///< Encoding function.
		uint screen_depth;          ///< Screen depth (0, 8 or 32).
		PaletteAnimation palette_animation; ///< Palette animation.
	};

	/* Static stuff (active blitter). */
	static char *ini;
	static bool autodetected;

	/* Find the blitter information for a given blitter name. */
	static const Info *find (const char *name);

	/* Find a replacement blitter. */
	static const Info *choose (bool anim, bool base_32bpp, bool grf_32bpp);

	/* Select a blitter. */
	static void select (const Info *blitter);

	/** Get the current active blitter (always set by calling select). */
	static const Info *get (void)
	{
		extern const Info *current_blitter;
		return current_blitter;
	}

	/* Fill a buffer with information about available blitters. */
	static void list (stringb *buf);
};

#endif /* BLITTER_H */
