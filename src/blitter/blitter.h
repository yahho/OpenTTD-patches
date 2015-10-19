/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file blitter.h Blitter code interface. */

#ifndef BLITTER_H
#define BLITTER_H

#include "factory.hpp"

static inline Blitter *SelectBlitter (const char *name)
{
	return BlitterFactory::SelectBlitter (name);
}

static inline Blitter *GetCurrentBlitter()
{
	return BlitterFactory::GetCurrentBlitter();
}

static inline const char *GetCurrentBlitterName (void)
{
	return GetCurrentBlitter()->GetName();
}

static inline void GetBlittersInfo (stringb *buf)
{
	BlitterFactory::GetBlittersInfo (buf);
}

extern char *_ini_blitter;
extern bool _blitter_autodetected;

#endif /* BLITTER_H */
