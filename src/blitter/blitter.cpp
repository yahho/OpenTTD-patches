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

#ifdef WITH_COCOA
bool QZ_CanDisplay8bpp();
#endif

/** Blitter creation function template. */
template <typename B>
static Blitter *create_blitter (void)
{
	return new B;
}

/** Static per-blitter data. */
struct BlitterInfo {
	const char *name;           ///< The name of the blitter.
	const char *desc;           ///< Description of the blitter.
	bool (*usable) (void);      ///< Usability check function.
	Blitter* (*create) (void);  ///< Instance creation function.
};

/** Static blitter data. */
static const BlitterInfo blitter_data[] = {
#define BLITTER(B) { B::name, B::desc, &B::usable, &create_blitter<B> }
	BLITTER (Blitter_Null),
#ifndef DEDICATED
	BLITTER (Blitter_8bppSimple),
	BLITTER (Blitter_8bppOptimized),
	BLITTER (Blitter_32bppSimple),
	BLITTER (Blitter_32bppOptimized),
	BLITTER (Blitter_32bppAnim),
#ifdef WITH_SSE
	BLITTER (Blitter_32bppSSE2),
	BLITTER (Blitter_32bppSSSE3),
	BLITTER (Blitter_32bppSSE4),
	BLITTER (Blitter_32bppSSE4_Anim),
#endif /* WITH_SSE */
#endif /* DEDICATED */
#undef BLITTER
};

/** Usable blitter set type. */
typedef std::bitset <lengthof(blitter_data)> BlitterSet;

/** Blitter usability test function. */
static BlitterSet get_usable_blitters (void)
{
	BlitterSet set;

	for (uint i = 0; i < set.size(); i++) {
		const BlitterInfo *data = &blitter_data[i];
		bool usable = data->usable();
		set.set (i, usable);
		DEBUG(driver, 1, "Blitter %s%s registered", data->name,
				usable ? "" : " not");
	}

	return set;
}

/** Set of usable blitters. */
BlitterSet usable_blitters = get_usable_blitters();

/** Current blitter info. */
static const BlitterInfo *current_info;

/** Current blitter instance. */
ttd_unique_ptr<Blitter> current_blitter;

/**
 * Get the blitter data with the given name.
 * @param name The blitter to select.
 * @return The blitter data, or NULL when there isn't one with the wanted name.
 */
static const BlitterInfo *find_blitter (const char *name)
{
	if (StrEmpty (name)) {
#if defined(DEDICATED)
		name = "null";
#else
		name = "8bpp-optimized";

#if defined(WITH_COCOA)
		/* Some people reported lack of fullscreen support in 8 bpp
		 * mode. While we prefer 8 bpp since it's faster, we will
		 * still have to test for support. */
		if (!QZ_CanDisplay8bpp()) {
			/* The main display can't go to 8 bpp fullscreen mode.
			 * We will have to switch to 32 bpp by default. */
			name = "32bpp-anim";
		}
#endif /* defined(WITH_COCOA) */
#endif /* defined(DEDICATED) */
	}

	for (uint i = 0; i < lengthof(blitter_data); i++) {
		if (usable_blitters.test (i)) {
			const BlitterInfo *data = &blitter_data[i];
			if (strcasecmp (name, data->name) == 0) return data;
		}
	}

	return NULL;
}

/**
 * Find the requested blitter and return his class.
 * @param name the blitter to select.
 * @post Sets the blitter so GetCurrentBlitter() returns it too.
 */
Blitter *SelectBlitter (const char *name)
{
	const BlitterInfo *data = find_blitter (name);
	if (data == NULL) return NULL;

	Blitter *blitter = data->create();
	current_blitter.reset (blitter);
	current_info = data;

	DEBUG(driver, 1, "Successfully %s blitter %s", StrEmpty(name) ? "probed" : "loaded", data->name);
	return blitter;
}

/** Get the name of the current blitter. */
const char *GetCurrentBlitterName (void)
{
	return current_info == NULL ? "none" : current_info->name;
}

/**
 * Fill a buffer with information about the blitters.
 * @param buf The buffer to fill.
 */
void GetBlittersInfo (stringb *buf)
{
	buf->append ("List of blitters:\n");
	for (uint i = 0; i < lengthof(blitter_data); i++) {
		if (usable_blitters.test (i)) {
			const BlitterInfo *data = &blitter_data[i];
			buf->append_fmt ("%18s: %s\n", data->name, data->desc);
		}
	}
	buf->append ('\n');
}
