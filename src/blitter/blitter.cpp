/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file blitter.cpp Common blitter code. */

#include "../stdafx.h"

#include <bitset>

#include "../debug.h"
#include "../string.h"
#include "blitter.h"
#include "../core/math_func.hpp"

#include "null.hpp"

#ifndef DEDICATED
#include "8bpp_simple.hpp"
#include "8bpp_optimized.hpp"
#include "32bpp_simple.hpp"
#include "32bpp_optimized.hpp"
#include "32bpp_anim.hpp"
#ifdef WITH_SSE
#include "32bpp_sse2.hpp"
#include "32bpp_ssse3.hpp"
#include "32bpp_sse4.hpp"
#include "32bpp_anim_sse4.hpp"
#endif
#endif

#ifdef DEDICATED
#define IF_BLITTER_GUI(...)
#else
#define IF_BLITTER_GUI(...) __VA_ARGS__
#endif

#ifdef WITH_SSE
#define IF_BLITTER_SSE IF_BLITTER_GUI
#else
#define IF_BLITTER_SSE(...)
#endif

#define BLITTER_LIST(p) p(Blitter_Null),            \
	IF_BLITTER_GUI (p(Blitter_8bppSimple),)     \
	IF_BLITTER_GUI (p(Blitter_8bppOptimized),)  \
	IF_BLITTER_GUI (p(Blitter_32bppSimple),)    \
	IF_BLITTER_GUI (p(Blitter_32bppOptimized),) \
	IF_BLITTER_GUI (p(Blitter_32bppAnim),)      \
	IF_BLITTER_SSE (p(Blitter_32bppSSE2),)      \
	IF_BLITTER_SSE (p(Blitter_32bppSSSE3),)     \
	IF_BLITTER_SSE (p(Blitter_32bppSSE4),)      \
	IF_BLITTER_SSE (p(Blitter_32bppSSE4_Anim),)

/** Static blitter data. */
static const Blitter::Info blitter_data[] = {
#define BLITTER(B) { B::name, B::desc, &B::usable, &B::create, &B::Encode, B::screen_depth, B::palette_animation }
BLITTER_LIST(BLITTER)
#undef BLITTER
};

/** Usable blitter set type. */
typedef std::bitset <lengthof(blitter_data)> BlitterSet;

/** Blitter usability test function. */
static BlitterSet get_usable_blitters (void)
{
	BlitterSet set;

	for (uint i = 0; i < set.size(); i++) {
		const Blitter::Info *data = &blitter_data[i];
		bool usable = data->usable();
		set.set (i, usable);
		DEBUG(driver, 1, "Blitter %s%s registered", data->name,
				usable ? "" : " not");
	}

	return set;
}

/** Set of usable blitters. */
static const BlitterSet usable_blitters (get_usable_blitters());


/** The blitter as stored in the configuration file. */
char *Blitter::ini;

/** Current blitter info. */
const Blitter::Info *current_blitter;

/** Whether the current blitter was autodetected or specified by the user. */
bool Blitter::autodetected;

/**
 * Get the blitter data with the given name.
 * @param name The blitter to select.
 * @return The blitter data, or NULL when there isn't one with the wanted name.
 */
const Blitter::Info *Blitter::find (const char *name)
{
	for (uint i = 0; i < lengthof(blitter_data); i++) {
		if (usable_blitters.test (i)) {
			const Blitter::Info *data = &blitter_data[i];
			if (strcasecmp (name, data->name) == 0) return data;
		}
	}

	return NULL;
}

/**
 * Find a replacement blitter given some requirements.
 * @param anim       Whether animation is wanted.
 * @param base_32bpp Whether the baseset requires 32 bpp.
 * @param grf_32bpp  Whether a NewGRF requires 32 bpp.
 * @return A suitable replacement blitter.
 */
const Blitter::Info *Blitter::choose (bool anim, bool base_32bpp, bool grf_32bpp)
{
	static const struct {
		const char *name;
		byte animation;   ///< 0: no support, 1: do support, 2: both
		byte base_depth;  ///< 0: 8bpp, 1: 32bpp, 2: both
		byte grf_depth;   ///< 0: 8bpp, 1: 32bpp, 2: both
	} replacement_blitters[] = {
#ifdef WITH_SSE
		{ "32bpp-sse4",       0,  1,  2 },
		{ "32bpp-ssse3",      0,  1,  2 },
		{ "32bpp-sse2",       0,  1,  2 },
		{ "32bpp-sse4-anim",  1,  1,  2 },
#endif
		{ "8bpp-optimized",   2,  0,  0 },
		{ "32bpp-optimized",  0,  2,  2 },
		{ "32bpp-anim",       1,  2,  2 },
	};

	for (uint i = 0; ; i++) {
		/* One of the last two blitters should always match. */
		assert (i < lengthof(replacement_blitters));

		if (replacement_blitters[i].animation  == (anim       ? 0 : 1)) continue;
		if (replacement_blitters[i].base_depth == (base_32bpp ? 0 : 1)) continue;
		if (replacement_blitters[i].grf_depth  == (grf_32bpp  ? 0 : 1)) continue;

		return find (replacement_blitters[i].name);
	}
}

/**
 * Make the given blitter current.
 * @param blitter Blitter to set.
 * @post Sets the blitter so Blitter::get() returns it too.
 */
void Blitter::select (const Blitter::Info *blitter)
{
	current_blitter = blitter;
	DEBUG(driver, 1, "Successfully loaded blitter %s", blitter->name);
}

/**
 * Fill a buffer with information about the blitters.
 * @param buf The buffer to fill.
 */
void Blitter::list (stringb *buf)
{
	buf->append ("List of blitters:\n");
	for (uint i = 0; i < lengthof(blitter_data); i++) {
		if (usable_blitters.test (i)) {
			const Blitter::Info *data = &blitter_data[i];
			buf->append_fmt ("%18s: %s\n", data->name, data->desc);
		}
	}
	buf->append ('\n');
}


bool Blitter::Surface::palette_animate (const Palette &palette)
{
	/* The null driver does not need to animate anything, for the 8bpp
	 * blitters the video backend takes care of the palette animation
	 * and 32bpp blitters do not have palette animation by default,
	 * so this provides a suitable default for most blitters. */
	return false;
}

void Blitter::Surface::draw_line (void *video, int x, int y, int x2, int y2, int screen_width, int screen_height, uint8 colour, int width, int dash)
{
	int dy;
	int dx;
	int stepx;
	int stepy;

	dy = (y2 - y) * 2;
	if (dy < 0) {
		dy = -dy;
		stepy = -1;
	} else {
		stepy = 1;
	}

	dx = (x2 - x) * 2;
	if (dx < 0) {
		dx = -dx;
		stepx = -1;
	} else {
		stepx = 1;
	}

	if (dx == 0 && dy == 0) {
		/* The algorithm below cannot handle this special case; make it work at least for line width 1 */
		if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) this->set_pixel (video, x, y, colour);
		return;
	}

	int frac_diff = width * max(dx, dy);
	if (width > 1) {
		/* compute frac_diff = width * sqrt(dx*dx + dy*dy)
		 * Start interval:
		 *    max(dx, dy) <= sqrt(dx*dx + dy*dy) <= sqrt(2) * max(dx, dy) <= 3/2 * max(dx, dy) */
		int frac_sq = width * width * (dx * dx + dy * dy);
		int frac_max = 3 * frac_diff / 2;
		while (frac_diff < frac_max) {
			int frac_test = (frac_diff + frac_max) / 2;
			if (frac_test * frac_test < frac_sq) {
				frac_diff = frac_test + 1;
			} else {
				frac_max = frac_test - 1;
			}
		}
	}

	int gap = dash;
	if (dash == 0) dash = 1;
	int dash_count = 0;
	if (dx > dy) {
		int y_low     = y;
		int y_high    = y;
		int frac_low  = dy - frac_diff / 2;
		int frac_high = dy + frac_diff / 2;

		while (frac_low + dx / 2 < 0) {
			frac_low += dx;
			y_low -= stepy;
		}
		while (frac_high - dx / 2 >= 0) {
			frac_high -= dx;
			y_high += stepy;
		}
		x2 += stepx;

		while (x != x2) {
			if (dash_count < dash && x >= 0 && x < screen_width) {
				for (int y = y_low; y != y_high; y += stepy) {
					if (y >= 0 && y < screen_height) this->set_pixel (video, x, y, colour);
				}
			}
			if (frac_low >= 0) {
				y_low += stepy;
				frac_low -= dx;
			}
			if (frac_high >= 0) {
				y_high += stepy;
				frac_high -= dx;
			}
			x += stepx;
			frac_low += dy;
			frac_high += dy;
			if (++dash_count >= dash + gap) dash_count = 0;
		}
	} else {
		int x_low     = x;
		int x_high    = x;
		int frac_low  = dx - frac_diff / 2;
		int frac_high = dx + frac_diff / 2;

		while (frac_low + dy / 2 < 0) {
			frac_low += dy;
			x_low -= stepx;
		}
		while (frac_high - dy / 2 >= 0) {
			frac_high -= dy;
			x_high += stepx;
		}
		y2 += stepy;

		while (y != y2) {
			if (dash_count < dash && y >= 0 && y < screen_height) {
				for (int x = x_low; x != x_high; x += stepx) {
					if (x >= 0 && x < screen_width) this->set_pixel (video, x, y, colour);
				}
			}
			if (frac_low >= 0) {
				x_low += stepx;
				frac_low -= dy;
			}
			if (frac_high >= 0) {
				x_high += stepx;
				frac_high -= dy;
			}
			y += stepy;
			frac_low += dx;
			frac_high += dx;
			if (++dash_count >= dash + gap) dash_count = 0;
		}
	}
}
