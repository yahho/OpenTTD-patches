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

/** Instantiation of the null blitter factory. */
static FBlitter_Null iFBlitter_Null;

Sprite *Blitter_Null::Encode(const SpriteLoader::Sprite *sprite, AllocatorProc *allocator)
{
	return AllocateSprite<Sprite> (sprite, allocator);
}
