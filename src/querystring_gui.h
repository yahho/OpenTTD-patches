/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file querystring_gui.h Base for the GUIs that have an edit box in them. */

#ifndef QUERYSTRING_GUI_H
#define QUERYSTRING_GUI_H

#include "core/pointer.h"
#include "textbuf_type.h"
#include "textbuf_gui.h"
#include "window_gui.h"

/**
 * Data stored about a string that can be modified in the GUI
 */
struct QueryString : Textbuf {
	/* Special actions when hitting ENTER or ESC. (only keyboard, not OSK) */
	static const int ACTION_NOTHING  = -1; ///< Nothing.
	static const int ACTION_DESELECT = -2; ///< Deselect editbox.
	static const int ACTION_CLEAR    = -3; ///< Clear editbox.

	StringID caption;
	int ok_button;      ///< Widget button of parent window to simulate when pressing OK in OSK.
	int cancel_button;  ///< Widget button of parent window to simulate when pressing CANCEL in OSK.
	ttd_unique_free_ptr<char> orig;
	bool handled;

	/**
	 * Initialize string.
	 * @param size Maximum size in bytes.
	 * @param buffer String buffer.
	 * @param chars Maximum size in chars.
	 */
	QueryString (uint16 size, char *buffer, uint16 chars = UINT16_MAX) :
		Textbuf (size, buffer, chars),
		ok_button (ACTION_NOTHING), cancel_button (ACTION_DESELECT)
	{
	}

public:
	void DrawEditBox (BlitArea *area, const Window *w, int wid) const;
	void ClickEditBox(Window *w, Point pt, int wid, int click_count, bool focus_changed);
	void HandleEditBox(Window *w, int wid);

	Point GetCaretPosition(const Window *w, int wid) const;
	Rect GetBoundingRect(const Window *w, int wid, const char *from, const char *to) const;
	const char *GetCharAtPosition(const Window *w, int wid, const Point &pt) const;
};

/** QueryString with integrated buffer. */
template <uint16 N, uint16 M = UINT16_MAX>
struct QueryStringN : QueryString {
	char data [N];

	QueryStringN (void) : QueryString (N, this->data, M)
	{
	}
};

/** QueryString with integrated buffer in chars. */
template <uint16 C>
struct QueryStringC : QueryStringN <C * MAX_CHAR_LENGTH, C> {
};

void ShowOnScreenKeyboard(Window *parent, int button);
void UpdateOSKOriginalText(const Window *parent, int button);
bool IsOSKOpenedFor(const Window *w, int button);

#endif /* QUERYSTRING_GUI_H */
