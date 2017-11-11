/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file textbuf_type.h Stuff related to text buffers. */

#ifndef TEXTBUF_TYPE_H
#define TEXTBUF_TYPE_H

#include "string.h"
#include "strings_type.h"
#include "core/pointer.h"
#include "core/smallvec_type.hpp"

#ifdef WITH_ICU_SORT
#include <unicode/utext.h>
#include <unicode/brkiter.h>
#endif

/**
 * Valid filter types for Textbuf::IsValidChar.
 */
enum CharSetFilter {
	CS_ALPHANUMERAL,      ///< Both numeric and alphabetic and spaces and stuff
	CS_NUMERAL,           ///< Only numeric ones
	CS_HEXADECIMAL,       ///< Only hexadecimal characters
};

/**
 * Return values for Textbuf::HandleKeypress
 */
enum HandleKeyPressResult
{
	HKPR_EDITING,     ///< Textbuf content changed.
	HKPR_CURSOR,      ///< Non-text change, e.g. cursor position.
	HKPR_CONFIRM,     ///< Return or enter key pressed.
	HKPR_CANCEL,      ///< Escape key pressed.
	HKPR_NOT_HANDLED, ///< Key does not affect editboxes.
};

/** Helper/buffer for input fields. */
struct Textbuf : stringb {
	CharSetFilter afilter;    ///< Allowed characters
	const uint16 max_chars;   ///< the maximum size of the buffer in characters (including terminating '\0')
	uint16 chars;             ///< the current size of the string in characters (including terminating '\0')
	uint16 pixels;            ///< the current size of the string in pixels
	bool caret;               ///< is the caret ("_") visible or not
	uint16 caretpos;          ///< the current position of the caret in the buffer, in bytes
	uint16 caretxoffs;        ///< the current position of the caret in pixels
	uint16 markpos;           ///< the start position of the marked area in the buffer, in bytes
	uint16 markend;           ///< the end position of the marked area in the buffer, in bytes
	uint16 markxoffs;         ///< the start position of the marked area in pixels
	uint16 marklength;        ///< the length of the marked area in pixels

	explicit Textbuf (uint16 max_bytes, char *buf, uint16 max_chars = UINT16_MAX);

	bool IsValidChar (WChar key) const;

	void Assign(StringID string);
	void Assign(const char *text);
	void CDECL Print(const char *format, ...) WARN_FORMAT(2, 3);

	void DeleteAll();
	bool InsertClipboard();

	bool InsertChar(uint32 key);
	bool InsertString(const char *str, bool marked, const char *caret = NULL, const char *insert_location = NULL, const char *replacement_end = NULL);

	bool DeleteChar (bool backspace = true, bool word = false);
	bool MoveLeft (bool word = false);
	bool MoveRight (bool word = false);
	bool MoveEnd (void);

	HandleKeyPressResult HandleKeyPress(WChar key, uint16 keycode);

	bool HandleCaret();
	void UpdateSize();

	/**
	 * Get the current text.
	 * @return Current text.
	 */
	const char *GetText() const
	{
		return this->c_str();
	}

	/**
	 * Get the position of the caret in the text buffer.
	 * @return Pointer to the caret in the text buffer.
	 */
	const char *GetCaret() const
	{
		return this->c_str() + this->caretpos;
	}

	/**
	 * Get the currently marked text.
	 * @param[out] length Length of the marked text.
	 * @return Begining of the marked area or NULL if no text is marked.
	 */
	const char *GetMarkedText (size_t *length) const
	{
		if (this->markend == 0) return NULL;

		*length = this->markend - this->markpos;
		return this->c_str() + this->markpos;
	}

	void GetCharPositions (const char *c1, int *x1, const char *c2, int *x2) const;

	const char *GetCharAtPosition (int x) const;

private:
#ifdef WITH_ICU_SORT
	ttd_unique_ptr<icu::BreakIterator> char_itr; ///< ICU iterator for characters.
	ttd_unique_ptr<icu::BreakIterator> word_itr; ///< ICU iterator for words.
	SmallVector<UChar, 32> utf16_str;      ///< UTF-16 copy of the string.
	SmallVector<size_t, 32> utf16_to_utf8; ///< Mapping from UTF-16 code point position to index in the UTF-8 source string.
#else
	size_t cur_pos;     ///< Current iteration position.
#endif

	void SetString (const char *s);
	size_t SetCurPosition (size_t pos);
	size_t Next (bool word);
	size_t Prev (bool word);

	void DeleteText(uint16 from, uint16 to, bool update);

	void UpdateStringIter();
	void UpdateWidth();
	void UpdateCaretPosition();
	void UpdateMarkedText();
};

#endif /* TEXTBUF_TYPE_H */
