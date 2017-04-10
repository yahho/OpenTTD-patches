/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file null_v.cpp The videio driver that doesn't blit. */

#include "../stdafx.h"
#include "../debug.h"
#include "../gfx_func.h"
#include "../blitter/blitter.h"
#include "null_v.h"

/** Factory for the null video driver. */
static VideoDriverFactory <VideoDriver_Null>
		iFVideoDriver_Null (0, "null", "Null Video Driver");

const char *VideoDriver_Null::Start(const char * const *parm)
{
#ifdef _MSC_VER
	/* Disable the MSVC assertion message box. */
	_set_error_mode(_OUT_TO_STDERR);
#endif

	this->ticks = GetDriverParamInt(parm, "ticks", 1000);

	/* Do not render, nor blit */
	DEBUG(misc, 1, "Forcing blitter 'null'...");
	Blitter *blitter = Blitter::select ("null");
	/* The null blitter should always be available. */
	assert (blitter != NULL);
	_screen_surface.reset (blitter->create (NULL, _cur_resolution.width,
				_cur_resolution.height, _cur_resolution.width));
	_screen_width  = _cur_resolution.width;
	_screen_height = _cur_resolution.height;
	ScreenSizeChanged();

	return NULL;
}

void VideoDriver_Null::Stop() { }

void VideoDriver_Null::MainLoop()
{
	uint i;

	for (i = 0; i < this->ticks; i++) {
		GameLoop();
		UpdateWindows();
	}
}
