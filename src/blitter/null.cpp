/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file null.cpp A blitter that doesn't blit. */

#include "../stdafx.h"
#include "null.hpp"

const char Blitter_Null::name[] = "null";
const char Blitter_Null::desc[] = "Null Blitter (does nothing)";

Sprite *Blitter_Null::Encode (const RawSprite *sprite, bool is_font, AllocatorProc *allocator)
{
	return AllocateSprite<Sprite> (sprite, allocator);
}
