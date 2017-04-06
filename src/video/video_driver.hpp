/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file video_driver.hpp Base of all video drivers. */

#ifndef VIDEO_VIDEO_DRIVER_HPP
#define VIDEO_VIDEO_DRIVER_HPP

#include "../driver.h"
#include "../core/geometry_type.hpp"
#include "../blitter/blitter.h"
#include "../gfx_func.h"

/** The base of all video drivers. */
class VideoDriver : public Driver, public SharedDriverSystem <VideoDriver> {
public:
	static char *ini; ///< The video driver as stored in the configuration file.

	/** Get the name of this type of driver. */
	static CONSTEXPR const char *GetSystemName (void)
	{
		return "video";
	}

	/**
	 * Mark a particular area dirty.
	 * @param left   The left most line of the dirty area.
	 * @param top    The top most line of the dirty area.
	 * @param width  The width of the dirty area.
	 * @param height The height of the dirty area.
	 */
	virtual void MakeDirty(int left, int top, int width, int height) = 0;

	/**
	 * Perform the actual drawing.
	 */
	virtual void MainLoop() = 0;

	/** Helper function to handle palette animation. */
	static void PaletteAnimate (const Colour (&palette) [256])
	{
		if (_screen_surface->palette_animate (palette)) {
			GetActiveDriver()->MakeDirty (0, 0, _screen_width, _screen_height);
		}
	}

	/**
	 * Change the resolution of the window.
	 * @param w The new width.
	 * @param h The new height.
	 * @return True if the change succeeded.
	 */
	virtual bool ChangeResolution(int w, int h) = 0;

	/**
	 * Change the full screen setting.
	 * @param fullscreen The new setting.
	 * @return True if the change succeeded.
	 */
	virtual bool ToggleFullscreen(bool fullscreen) = 0;

	/**
	 * Callback invoked after the blitter was changed.
	 * @return True if no error.
	 */
	virtual bool AfterBlitterChange()
	{
		return true;
	}

	virtual bool ClaimMousePointer()
	{
		return true;
	}

	/**
	 * Whether the driver has a graphical user interface with the end user.
	 * Or in other words, whether we should spawn a thread for world generation
	 * and NewGRF scanning so the graphical updates can keep coming. Otherwise
	 * progress has to be shown on the console, which uses by definition another
	 * thread/process for display purposes.
	 * @return True for all drivers except null and dedicated.
	 */
	virtual bool HasGUI() const
	{
		return true;
	}

	/**
	 * An edit box lost the input focus. Abort character compositing if necessary.
	 */
	virtual void EditBoxLostFocus() {}
};

/** Video driver factory. */
template <class D>
class VideoDriverFactory : DriverFactory <VideoDriver, D> {
public:
	/**
	 * Construct a new VideoDriverFactory.
	 * @param priority    The priority within the driver class.
	 * @param name        The name of the driver.
	 * @param description A long-ish description of the driver.
	 */
	VideoDriverFactory (int priority, const char *name, const char *description)
		: DriverFactory <VideoDriver, D> (priority, name, description)
	{
	}
};

extern int _num_resolutions;
extern Dimension _resolutions[32];
extern Dimension _cur_resolution;
extern bool _rightclick_emulate;

#endif /* VIDEO_VIDEO_DRIVER_HPP */
