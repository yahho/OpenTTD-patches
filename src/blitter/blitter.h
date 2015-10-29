/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file blitter.h Blitter code interface. */

#ifndef BLITTER_H
#define BLITTER_H

#include "../core/pointer.h"
#include "../string.h"
#include "base.hpp"

Blitter *SelectBlitter (const char *name);

/** Get the current active blitter (always set by calling SelectBlitter). */
static inline Blitter *GetCurrentBlitter()
{
	extern ttd_unique_ptr<Blitter> current_blitter;
	return current_blitter.get();
}

const char *GetCurrentBlitterName (void);

void GetBlittersInfo (stringb *buf);

extern char *_ini_blitter;
extern bool _blitter_autodetected;

#endif /* BLITTER_H */
