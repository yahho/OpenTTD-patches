/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file newgrf_text.cpp
 * Implementation of  Action 04 "universal holder" structure and functions.
 * This file implements a linked-lists of strings,
 * holding everything that the newgrf action 04 will send over to OpenTTD.
 * One of the biggest problems is that Dynamic lang Array uses ISO codes
 * as way to identifying current user lang, while newgrf uses bit shift codes
 * not related to ISO.  So equivalence functionnality had to be set.
 */

#include "stdafx.h"
#include "newgrf.h"
#include "strings_func.h"
#include "newgrf_storage.h"
#include "newgrf_text.h"
#include "newgrf_cargo.h"
#include "string.h"
#include "date_type.h"
#include "debug.h"
#include "core/pointer.h"
#include "core/flexarray.h"
#include "core/alloc_type.hpp"
#include "core/smallmap_type.hpp"
#include "language.h"

#include "table/strings.h"
#include "table/control_codes.h"

/**
 * Explains the newgrf shift bit positioning.
 * the grf base will not be used in order to find the string, but rather for
 * jumping from standard langID scheme to the new one.
 */
enum GRFBaseLanguages {
	GRFLB_AMERICAN    = 0x01,
	GRFLB_ENGLISH     = 0x02,
	GRFLB_GERMAN      = 0x04,
	GRFLB_FRENCH      = 0x08,
	GRFLB_SPANISH     = 0x10,
	GRFLB_GENERIC     = 0x80,
};

enum GRFExtendedLanguages {
	GRFLX_AMERICAN    = 0x00,
	GRFLX_ENGLISH     = 0x01,
	GRFLX_GERMAN      = 0x02,
	GRFLX_FRENCH      = 0x03,
	GRFLX_SPANISH     = 0x04,
	GRFLX_UNSPECIFIED = 0x7F,
};


/**
 * Holder of a GRFTextMap.
 * Putting both grfid and stringid together allows us to avoid duplicates,
 * since it is NOT SUPPOSED to happen.
 */
struct GRFTextEntry {
	uint32 grfid;
	uint16 stringid;
	StringID def_string;
	GRFTextMap *map;
};


static uint _num_grf_texts = 0;
static GRFTextEntry _grf_text[TAB_SIZE_NEWGRF];
static byte _currentLangID = GRFLX_ENGLISH;  ///< by default, english is used.

/**
 * Get the mapping from the NewGRF supplied ID to OpenTTD's internal ID.
 * @param newgrf_id The NewGRF ID to map.
 * @param gender    Whether to map genders or cases.
 * @return The, to OpenTTD's internal ID, mapped index, or -1 if there is no mapping.
 */
int LanguageMap::GetMapping(int newgrf_id, bool gender) const
{
	const SmallVector<Mapping, 1> &map = gender ? this->gender_map : this->case_map;
	for (const Mapping *m = map.Begin(); m != map.End(); m++) {
		if (m->newgrf_id == newgrf_id) return m->openttd_id;
	}
	return -1;
}

/**
 * Get the mapping from OpenTTD's internal ID to the NewGRF supplied ID.
 * @param openttd_id The OpenTTD ID to map.
 * @param gender     Whether to map genders or cases.
 * @return The, to the NewGRF supplied ID, mapped index, or -1 if there is no mapping.
 */
int LanguageMap::GetReverseMapping(int openttd_id, bool gender) const
{
	const SmallVector<Mapping, 1> &map = gender ? this->gender_map : this->case_map;
	for (const Mapping *m = map.Begin(); m != map.End(); m++) {
		if (m->openttd_id == openttd_id) return m->newgrf_id;
	}
	return -1;
}


/** Allocated string with (some) built-in bounds checking. */
struct mstring : stringb, FlexArray <char> {
	char data[];

private:
	mstring (size_t n) : stringb (n, this->data)
	{
	}

public:
	static mstring *create (size_t n = 1)
	{
		return new (n) mstring (n);
	}
};


/** Dump the representation of a switch case mapping. */
static void DumpSwitchMapping (stringb *buf, const LanguageMap *lm,
	const ttd_unique_free_ptr<mstring> (&mapping) [256])
{
	/*
	 * Format for case switch:
	 * <NUM CASES> <CASE1> <LEN1> <STRING1> <CASE2> <LEN2> <STRING2> <CASE3> <LEN3> <STRING3> <STRINGDEFAULT>
	 * Each LEN is printed using 2 bytes in big endian order.
	 */
	int cases [MAX_NUM_CASES];

	assert (_current_language->num_cases <= lengthof(cases));

	/* "<NUM CASES>" */
	uint count = 0;
	for (uint8 i = 0; i < _current_language->num_cases; i++) {
		/* Count the ones we have a mapped string for. */
		int idx = lm->GetReverseMapping (i, false);
		cases[i] = idx;
		if ((idx >= 0) && mapping[idx]) count++;
	}
	buf->append ((char)count);

	for (uint8 i = 0; i < _current_language->num_cases; i++) {
		/* Resolve the string we're looking for. */
		int idx = cases[i];
		if ((idx < 0) || !mapping[idx]) continue;
		const mstring *m = mapping[idx].get();

		/* "<CASEn>" */
		buf->append ((char)(i + 1));

		/* "<LENn>" */
		size_t len = m->length() + 1;
		buf->append ((char)GB(len, 8, 8));
		buf->append ((char)GB(len, 0, 8));

		/* "<STRINGn>" */
		buf->append (m->c_str());
		buf->append ('\0');
	}

	/* "<STRINGDEFAULT>" */
	buf->append (mapping[0]->c_str());
	buf->append ('\0');
}

/** Dump the representation of a choice list. */
static void DumpChoiceList (stringb *buf, const LanguageMap *lm,
	const ttd_unique_free_ptr<mstring> (&mapping) [256],
	int offset, bool gender)
{
	/*
	 * Format for choice list:
	 * <OFFSET> <NUM CHOICES> <LENs> <STRINGs>
	 */
	const mstring *strs [(MAX_NUM_GENDERS > LANGUAGE_MAX_PLURAL_FORMS) ? MAX_NUM_GENDERS : LANGUAGE_MAX_PLURAL_FORMS];

	/* "<OFFSET>" */
	buf->append ((char)(offset - 0x80));

	/* "<NUM CHOICES>" */
	uint count = gender ? _current_language->num_genders : LANGUAGE_MAX_PLURAL_FORMS;
	buf->append ((char)count);

	assert (count <= lengthof(strs));

	/* "<LENs>" */
	for (uint i = 0; i < count; i++) {
		int idx = gender ? lm->GetReverseMapping (i, true) : i + 1;
		const mstring *m = mapping[(idx >= 0) && mapping[idx] ? idx : 0].get();
		strs[i] = m;
		size_t len = m->length() + 1;
		if (len > 0xFF) {
			grfmsg(1, "choice list string is too long");
			len = 0xFF;
		}
		buf->append ((char)len);
	}

	/* "<STRINGs>" */
	for (uint i = 0; i < count; i++) {
		const mstring *m = strs[i];
		/* Limit the length of the string we copy to 0xFE. The length is written above
		 * as a byte and we need room for the final '\0'. */
		size_t len = min<size_t> (0xFE, m->length());
		buf->append_fmt ("%.*s", (int)len, m->c_str());
		buf->append ('\0');
	}
}

/** Dump the representation of a string mapping. */
static void DumpMapping (stringb *buf, const LanguageMap *lm,
	const ttd_unique_free_ptr<mstring> (&mapping) [256],
	StringControlCode type, int offset)
{
	if (lm == NULL) {
		/* In case there is no mapping, just ignore everything but the default.
		 * A probable cause for this happening is when the language file has
		 * been removed by the user and as such no mapping could be made. */
		buf->append (mapping[0]->c_str());
		return;
	}

	buf->append_utf8 (type);

	switch (type) {
		case SCC_SWITCH_CASE:
			DumpSwitchMapping (buf, lm, mapping);
			break;

		case SCC_PLURAL_LIST:
			buf->append ((char)lm->plural_form);
			/* fall through */
		default:
			DumpChoiceList (buf, lm, mapping, offset, type == SCC_GENDER_LIST);
			break;
	}
}


/** Construct a copy of this text map. */
GRFTextMap::GRFTextMap (const GRFTextMap &other)
	: std::map <byte, GRFText *> (other)
{
	for (iterator iter = this->begin(); iter != this->end(); iter++) {
		iter->second = iter->second->clone();
	}
}

/** Destroy the text map. */
GRFTextMap::~GRFTextMap()
{
	for (iterator iter = this->begin(); iter != this->end(); iter++) {
		delete iter->second;
	}
}

/** Get the GRFText for the current language, or a default. */
const GRFText *GRFTextMap::get_current (void) const
{
	byte langs[4] = { _currentLangID, GRFLX_UNSPECIFIED, GRFLX_ENGLISH, GRFLX_AMERICAN };

	for (uint i = 0; i < lengthof(langs); i++) {
		const_iterator iter = this->find (langs[i]);
		if (iter != this->end()) return iter->second;
	}

	return NULL;
}

/** Add a GRFText to this map. */
void GRFTextMap::add (byte langid, GRFText *text)
{
	std::pair <iterator, bool> pair =
			this->insert (std::make_pair (langid, text));

	if (!pair.second) {
		/* The langid already existed in the map. */
		GRFText *old = pair.first->second;
		pair.first->second = text;
		delete old;
	}
}

/**
 * Add a string to this map.
 * @param langid The language of the new text.
 * @param grfid The grfid where this string is defined.
 * @param allow_newlines Whether newlines are allowed in this string.
 * @param text The text to add to the list.
 * @note All text-codes will be translated.
 */
void GRFTextMap::add (byte langid, uint32 grfid, bool allow_newlines, const char *text)
{
	int len;
	char *translatedtext = TranslateTTDPatchCodes (grfid, langid, allow_newlines, text, &len);
	GRFText *newtext = GRFText::create (translatedtext, len);
	free (translatedtext);

	this->add (langid, newtext);
}


/**
 * Translate TTDPatch string codes into something OpenTTD can handle (better).
 * @param grfid          The (NewGRF) ID associated with this string
 * @param language_id    The (NewGRF) language ID associated with this string.
 * @param allow_newlines Whether newlines are allowed in the string or not.
 * @param str            The string to translate.
 * @param [out] olen     The length of the final string.
 * @param byte80         The control code to use as replacement for the 0x80-value.
 * @return The translated string.
 */
char *TranslateTTDPatchCodes(uint32 grfid, uint8 language_id, bool allow_newlines, const char *str, int *olen, StringControlCode byte80)
{
	enum {
		CTRL_EOF,   ///< End of string.
		CTRL_HSKIP, ///< Variable space.
		CTRL_NOP,   ///< Ignore.
		CTRL_NL,    ///< New line.
		CTRL_SETXY, ///< Set string position.
		CTRL_PRSTK, ///< Print string from stack.
		CTRL_PRSTR, ///< Print immediate string.
		CTRL_EXT,   ///< Extended format code.
	};

	assert_compile (CTRL_EXT < 0x20);

	static const uint16 ctrl [0xBE] = {
		CTRL_EOF, CTRL_HSKIP, '?', '?', '?', '?', '?', '?',
		'?', '?', CTRL_NOP, '?',
			'?', CTRL_NL, SCC_TINYFONT, SCC_BIGFONT,
		'?', '?', '?', '?', '?', '?', '?', '?',
		'?', '?', '?', '?', '?', '?', '?', CTRL_SETXY,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
		0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
		0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
		0x78, 0x79, 0x7A, SCC_NEWGRF_PRINT_DWORD_SIGNED,
			SCC_NEWGRF_PRINT_WORD_SIGNED,           // 0x7C
			SCC_NEWGRF_PRINT_BYTE_SIGNED,           // 0x7D
			SCC_NEWGRF_PRINT_WORD_UNSIGNED,         // 0x7E
			SCC_NEWGRF_PRINT_DWORD_CURRENCY,        // 0x7F
		CTRL_PRSTK, CTRL_PRSTR,                         // 0x80, 0x81
			SCC_NEWGRF_PRINT_WORD_DATE_LONG,        // 0x82
			SCC_NEWGRF_PRINT_WORD_DATE_SHORT,       // 0x83
			SCC_NEWGRF_PRINT_WORD_SPEED,            // 0x84
			SCC_NEWGRF_DISCARD_WORD,                // 0x85
			SCC_NEWGRF_ROTATE_TOP_4_WORDS,          // 0x86
			SCC_NEWGRF_PRINT_WORD_VOLUME_LONG,      // 0x87
		SCC_BLUE, SCC_SILVER, SCC_GOLD, SCC_RED,        // 0x88
			SCC_PURPLE, SCC_LTBROWN, SCC_ORANGE, SCC_GREEN,
		SCC_YELLOW, SCC_DKGREEN, SCC_CREAM, SCC_BROWN,  // 0x90
			SCC_WHITE, SCC_LTBLUE, SCC_GRAY, SCC_DKBLUE,
		SCC_BLACK, 0x99, CTRL_EXT, 0x9B, 0x9C, 0x9D, 0x20AC, 0x0178,
		SCC_UP_ARROW, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
		0xA8, 0xA9, SCC_DOWN_ARROW, 0xAB,
			SCC_CHECKMARK, SCC_CROSS, 0xAE, SCC_RIGHT_ARROW,
		0xB0, 0xB1, 0xB2, 0xB3,
			SCC_TRAIN, SCC_LORRY, SCC_BUS, SCC_PLANE,
		SCC_SHIP, SCC_SUPERSCRIPT_M1, 0xBA, 0xBB,
			SCC_SMALL_UP_ARROW, SCC_SMALL_DOWN_ARROW,
	};

	size_t tmp_len = strlen (str) * 10 + 1; // Allocate space to allow for expansion
	char *tmp = xmalloc (tmp_len);
	stringb tmp_buf (tmp_len, tmp);
	stringb *buf = &tmp_buf;

	bool unicode = false;
	WChar c;
	size_t len = Utf8Decode(&c, str);

	/* Helper variables for a possible (string) mapping. */
	StringControlCode mapping_type = (StringControlCode)0;
	int mapping_offset;

	/* Mapping of NewGRF-supplied ID to the different strings in the choice list. */
	ttd_unique_free_ptr<mstring> mapping_strings[256];

	if (c == NFO_UTF8_IDENTIFIER) {
		unicode = true;
		str += len;
	}

	for (;;) {
		if (unicode && Utf8EncodedCharLen(*str) != 0) {
			c = Utf8Consume(&str);
			/* 'Magic' range of control codes. */
			if (GB(c, 8, 8) == 0xE0) {
				c = GB(c, 0, 8);
			} else if (c >= 0x20) {
				if (!IsValidChar(c, CS_ALPHANUMERAL)) c = '?';
				buf->append_utf8 (c);
				continue;
			}
		} else {
			c = (byte)*str++;
		}

		if (c < lengthof(ctrl)) {
			c = ctrl[c];
		} else if (!IsPrintable (c)) {
			c = '?';
		}

		switch (c) {
			case CTRL_EOF:
				goto string_end;

			case CTRL_HSKIP:
				if (str[0] == '\0') goto string_end;
				buf->append (' ');
				str++;
				break;

			case CTRL_NOP:
				break;

			case CTRL_NL:
				if (allow_newlines) {
					buf->append (0x0A);
				} else {
					grfmsg(1, "Detected newline in string that does not allow one");
				}
				break;

			case CTRL_SETXY:
				if (str[0] == '\0' || str[1] == '\0') goto string_end;
				buf->append (' ');
				str += 2;
				break;

			case CTRL_PRSTK:
				buf->append_utf8 (byte80);
				break;

			case CTRL_PRSTR: {
				if (str[0] == '\0' || str[1] == '\0') goto string_end;
				StringID string;
				string  = ((uint8)*str++);
				string |= ((uint8)*str++) << 8;
				buf->append_utf8 (SCC_NEWGRF_STRINL);
				buf->append_utf8 (MapGRFStringID (grfid, string));
				break;
			}

			case CTRL_EXT: {
				int code = *str++;
				switch (code) {
					case 0x00: goto string_end;
					case 0x01: buf->append_utf8 (SCC_NEWGRF_PRINT_QWORD_CURRENCY); break;
					/* 0x02: ignore next colour byte is not supported. It works on the final
					 * string and as such hooks into the string drawing routine. At that
					 * point many things already happened, such as splitting up of strings
					 * when drawn over multiple lines or right-to-left translations, which
					 * make the behaviour peculiar, e.g. only happening at specific width
					 * of windows. Or we need to add another pass over the string to just
					 * support this. As such it is not implemented in OpenTTD. */
					case 0x03: {
						if (str[0] == '\0' || str[1] == '\0') goto string_end;
						uint16 tmp  = ((uint8)*str++);
						tmp        |= ((uint8)*str++) << 8;
						buf->append_utf8 (SCC_NEWGRF_PUSH_WORD);
						buf->append_utf8 (tmp);
						break;
					}
					case 0x04:
						if (str[0] == '\0') goto string_end;
						buf->append_utf8 (SCC_NEWGRF_UNPRINT);
						buf->append_utf8 (*str++);
						break;
					case 0x06: buf->append_utf8 (SCC_NEWGRF_PRINT_BYTE_HEX);          break;
					case 0x07: buf->append_utf8 (SCC_NEWGRF_PRINT_WORD_HEX);          break;
					case 0x08: buf->append_utf8 (SCC_NEWGRF_PRINT_DWORD_HEX);         break;
					/* 0x09, 0x0A are TTDPatch internal use only string codes. */
					case 0x0B: buf->append_utf8 (SCC_NEWGRF_PRINT_QWORD_HEX);         break;
					case 0x0C: buf->append_utf8 (SCC_NEWGRF_PRINT_WORD_STATION_NAME); break;
					case 0x0D: buf->append_utf8 (SCC_NEWGRF_PRINT_WORD_WEIGHT_LONG);  break;
					case 0x0E:
					case 0x0F: {
						if (str[0] == '\0') goto string_end;
						const LanguageMap *lm = LanguageMap::GetLanguageMap(grfid, language_id);
						int index = *str++;
						int mapped = lm != NULL ? lm->GetMapping(index, code == 0x0E) : -1;
						if (mapped >= 0) {
							buf->append_utf8 (code == 0x0E ? SCC_GENDER_INDEX : SCC_SET_CASE);
							buf->append_utf8 (code == 0x0E ? mapped : mapped + 1);
						}
						break;
					}

					case 0x10:
					case 0x11:
						if (str[0] == '\0') goto string_end;
						if (mapping_type == 0) {
							if (code == 0x10) str++; // Skip the index
							grfmsg(1, "choice list %s marker found when not expected", code == 0x10 ? "next" : "default");
							break;
						} else {
							/* Terminate the previous string. */
							byte index = (code == 0x10 ? *str++ : 0);
							if (mapping_strings[index]) {
								grfmsg(1, "duplicate choice list string, ignoring");
								buf->append ('\0');
							} else {
								mstring *m = mstring::create (strlen(str) * 10 + 1);
								mapping_strings[index].reset (m);
								buf = m;
							}
						}
						break;

					case 0x12:
						if (mapping_type == 0) {
							grfmsg(1, "choice list end marker found when not expected");
						} else {
							/* Terminate the previous string. */
							buf = &tmp_buf;

							if (!mapping_strings[0]) {
								/* In case of a (broken) NewGRF without a default,
								 * assume an empty string. */
								grfmsg(1, "choice list misses default value");
								mapping_strings[0].reset (mstring::create());
							}

							/* Now we can start flushing everything and clean everything up. */
							const LanguageMap *lm = LanguageMap::GetLanguageMap (grfid, language_id);
							DumpMapping (buf, lm, mapping_strings, mapping_type, mapping_offset);

							mapping_type = (StringControlCode)0;
							for (uint i = 0; i < lengthof(mapping_strings); i++) {
								mapping_strings[i].reset();
							}
						}
						break;

					case 0x13:
					case 0x14:
					case 0x15:
						if (str[0] == '\0') goto string_end;
						if (mapping_type != 0) {
							grfmsg(1, "choice lists can't be stacked, it's going to get messy now...");
							if (code != 0x14) str++;
						} else {
							static const StringControlCode mp[] = { SCC_GENDER_LIST, SCC_SWITCH_CASE, SCC_PLURAL_LIST };
							mapping_type = mp[code - 0x13];
							if (code != 0x14) mapping_offset = *str++;
						}
						break;

					case 0x16:
					case 0x17:
					case 0x18:
					case 0x19:
					case 0x1A:
					case 0x1B:
					case 0x1C:
					case 0x1D:
					case 0x1E:
						buf->append_utf8 (SCC_NEWGRF_PRINT_DWORD_DATE_LONG + code - 0x16);
						break;

					default:
						grfmsg(1, "missing handler for extended format code");
						break;
				}
				break;
			}

			default:
				buf->append_utf8 (c);
				break;
		}
	}

string_end:
	if (mapping_type != 0) {
		grfmsg(1, "choice list was incomplete, the whole list is ignored");
	}

	if (olen != NULL) *olen = tmp_buf.length() + 1;
	return xrealloc (tmp, tmp_buf.length() + 1);
}

/**
 * Add the new read string into our structure.
 */
StringID AddGRFString(uint32 grfid, uint16 stringid, byte langid_to_add, bool new_scheme, bool allow_newlines, const char *text_to_add, StringID def_string)
{
	char *translatedtext;
	uint id;

	/* When working with the old language scheme (grf_version is less than 7) and
	 * English or American is among the set bits, simply add it as English in
	 * the new scheme, i.e. as langid = 1.
	 * If English is set, it is pretty safe to assume the translations are not
	 * actually translated.
	 */
	if (!new_scheme) {
		if (langid_to_add & (GRFLB_AMERICAN | GRFLB_ENGLISH)) {
			langid_to_add = GRFLX_ENGLISH;
		} else {
			StringID ret = STR_EMPTY;
			if (langid_to_add & GRFLB_GERMAN)  ret = AddGRFString(grfid, stringid, GRFLX_GERMAN,  true, allow_newlines, text_to_add, def_string);
			if (langid_to_add & GRFLB_FRENCH)  ret = AddGRFString(grfid, stringid, GRFLX_FRENCH,  true, allow_newlines, text_to_add, def_string);
			if (langid_to_add & GRFLB_SPANISH) ret = AddGRFString(grfid, stringid, GRFLX_SPANISH, true, allow_newlines, text_to_add, def_string);
			return ret;
		}
	}

	for (id = 0; id < _num_grf_texts; id++) {
		if (_grf_text[id].grfid == grfid && _grf_text[id].stringid == stringid) {
			break;
		}
	}

	/* Too many strings allocated, return empty */
	if (id == lengthof(_grf_text)) return STR_EMPTY;

	int len;
	translatedtext = TranslateTTDPatchCodes(grfid, langid_to_add, allow_newlines, text_to_add, &len);

	GRFText *newtext = GRFText::create (translatedtext, len);

	free(translatedtext);

	/* If we didn't find our stringid and grfid in the list, allocate a new id */
	if (id == _num_grf_texts) _num_grf_texts++;

	if (_grf_text[id].map == NULL) {
		_grf_text[id].grfid      = grfid;
		_grf_text[id].stringid   = stringid;
		_grf_text[id].def_string = def_string;
		_grf_text[id].map        = new GRFTextMap;
	}
	_grf_text[id].map->add (langid_to_add, newtext);

	grfmsg (3, "Added 0x%X: grfid %08X string 0x%X lang 0x%X string '%s'", id, grfid, stringid, langid_to_add, newtext->text);

	return MakeStringID(TEXT_TAB_NEWGRF_START, id);
}

/**
 * Returns the index for this stringid associated with its grfID
 */
StringID GetGRFStringID(uint32 grfid, uint16 stringid)
{
	for (uint id = 0; id < _num_grf_texts; id++) {
		if (_grf_text[id].grfid == grfid && _grf_text[id].stringid == stringid) {
			return MakeStringID(TEXT_TAB_NEWGRF_START, id);
		}
	}

	return STR_UNDEFINED;
}


/**
 * Get a C-string from a stringid set by a newgrf.
 */
const char *GetGRFStringPtr(uint16 stringid)
{
	assert(_grf_text[stringid].grfid != 0);

	const char *str = _grf_text[stringid].map->get_string();
	if (str != NULL) return str;

	/* Use the default string ID if the fallback string isn't available */
	return GetStringPtr(_grf_text[stringid].def_string);
}

/**
 * Equivalence Setter function between game and newgrf langID.
 * This function will adjust _currentLangID as to what is the LangID
 * of the current language set by the user.
 * This function is called after the user changed language,
 * from strings.cpp:ReadLanguagePack
 * @param language_id iso code of current selection
 */
void SetCurrentGrfLangID(byte language_id)
{
	_currentLangID = language_id;
}

bool CheckGrfLangID(byte lang_id, byte grf_version)
{
	if (grf_version < 7) {
		switch (_currentLangID) {
			case GRFLX_GERMAN:  return (lang_id & GRFLB_GERMAN)  != 0;
			case GRFLX_FRENCH:  return (lang_id & GRFLB_FRENCH)  != 0;
			case GRFLX_SPANISH: return (lang_id & GRFLB_SPANISH) != 0;
			default:            return (lang_id & (GRFLB_ENGLISH | GRFLB_AMERICAN)) != 0;
		}
	}

	return (lang_id == _currentLangID || lang_id == GRFLX_UNSPECIFIED);
}

/**
 * House cleaning.
 * Remove all strings and reset the text counter.
 */
void CleanUpStrings()
{
	uint id;

	for (id = 0; id < _num_grf_texts; id++) {
		delete _grf_text[id].map;
		_grf_text[id].grfid    = 0;
		_grf_text[id].stringid = 0;
		_grf_text[id].map      = NULL;
	}

	_num_grf_texts = 0;
}

struct TextRefStack {
	byte stack[0x30];
	byte position;
	const GRFFile *grffile;
	bool used;

	TextRefStack() : position(0), grffile(NULL), used(false) {}

	TextRefStack(const TextRefStack &stack) :
		position(stack.position),
		grffile(stack.grffile),
		used(stack.used)
	{
		memcpy(this->stack, stack.stack, sizeof(this->stack));
	}

	uint8  PopUnsignedByte()  { assert(this->position < lengthof(this->stack)); return this->stack[this->position++]; }
	int8   PopSignedByte()    { return (int8)this->PopUnsignedByte(); }

	uint16 PopUnsignedWord()
	{
		uint16 val = this->PopUnsignedByte();
		return val | (this->PopUnsignedByte() << 8);
	}
	int16  PopSignedWord()    { return (int32)this->PopUnsignedWord(); }

	uint32 PopUnsignedDWord()
	{
		uint32 val = this->PopUnsignedWord();
		return val | (this->PopUnsignedWord() << 16);
	}
	int32  PopSignedDWord()   { return (int32)this->PopUnsignedDWord(); }

	uint64 PopUnsignedQWord()
	{
		uint64 val = this->PopUnsignedDWord();
		return val | (((uint64)this->PopUnsignedDWord()) << 32);
	}
	int64  PopSignedQWord()   { return (int64)this->PopUnsignedQWord(); }

	/** Rotate the top four words down: W1, W2, W3, W4 -> W4, W1, W2, W3 */
	void RotateTop4Words()
	{
		byte tmp[2];
		for (int i = 0; i  < 2; i++) tmp[i] = this->stack[this->position + i + 6];
		for (int i = 5; i >= 0; i--) this->stack[this->position + i + 2] = this->stack[this->position + i];
		for (int i = 0; i  < 2; i++) this->stack[this->position + i] = tmp[i];
	}

	void PushWord(uint16 word)
	{
		if (this->position >= 2) {
			this->position -= 2;
		} else {
			for (int i = lengthof(stack) - 1; i >= this->position + 2; i--) {
				this->stack[i] = this->stack[i - 2];
			}
		}
		this->stack[this->position]     = GB(word, 0, 8);
		this->stack[this->position + 1] = GB(word, 8, 8);
	}

	void ResetStack(const GRFFile *grffile)
	{
		assert(grffile != NULL);
		this->position = 0;
		this->grffile = grffile;
		this->used = true;
	}

	void RewindStack() { this->position = 0; }
};

/** The stack that is used for TTDP compatible string code parsing */
static TextRefStack _newgrf_textrefstack;

/**
 * Check whether the NewGRF text stack is in use.
 * @return True iff the NewGRF text stack is used.
 */
bool UsingNewGRFTextStack()
{
	return _newgrf_textrefstack.used;
}

/**
 * Create a backup of the current NewGRF text stack.
 * @return A copy of the current text stack.
 */
struct TextRefStack *CreateTextRefStackBackup()
{
	return new TextRefStack(_newgrf_textrefstack);
}

/**
 * Restore a copy of the text stack to the used stack.
 * @param backup The copy to restore.
 */
void RestoreTextRefStackBackup(struct TextRefStack *backup)
{
	_newgrf_textrefstack = *backup;
	delete backup;
}

/**
 * Start using the TTDP compatible string code parsing.
 *
 * On start a number of values is copied on the #TextRefStack.
 * You can then use #GetString() and the normal string drawing functions,
 * and they will use the #TextRefStack for NewGRF string codes.
 *
 * However, when you want to draw a string multiple times using the same stack,
 * you have to call #RewindTextRefStack() between draws.
 *
 * After you are done with drawing, you must disable usage of the #TextRefStack
 * by calling #StopTextRefStackUsage(), so NewGRF string codes operate on the
 * normal string parameters again.
 *
 * @param grffile the NewGRF providing the stack data
 * @param numEntries number of entries to copy from the registers
 * @param values values to copy onto the stack; if NULL the temporary NewGRF registers will be used instead
 */
void StartTextRefStackUsage(const GRFFile *grffile, byte numEntries, const uint32 *values)
{
	extern TemporaryStorageArray<int32, 0x110> _temp_store;

	_newgrf_textrefstack.ResetStack(grffile);

	byte *p = _newgrf_textrefstack.stack;
	for (uint i = 0; i < numEntries; i++) {
		uint32 value = values != NULL ? values[i] : _temp_store.GetValue(0x100 + i);
		for (uint j = 0; j < 32; j += 8) {
			*p = GB(value, j, 8);
			p++;
		}
	}
}

/** Stop using the TTDP compatible string code parsing */
void StopTextRefStackUsage()
{
	_newgrf_textrefstack.used = false;
}

void RewindTextRefStack()
{
	_newgrf_textrefstack.RewindStack();
}

/**
 * FormatString for NewGRF specific "magic" string control codes
 * @param scc   the string control code that has been read
 * @param buf   the buffer we're writing to
 * @param str   the string that we need to write
 * @param argv  the OpenTTD stack of values
 * @param argv_size space on the stack \a argv
 * @param modify_argv When true, modify the OpenTTD stack.
 * @return the string control code to "execute" now
 */
uint RemapNewGRFStringControlCode (uint scc, stringb *buf, const char **str, int64 *argv, uint argv_size, bool modify_argv)
{
	switch (scc) {
		default: break;

		case SCC_NEWGRF_PRINT_DWORD_SIGNED:
		case SCC_NEWGRF_PRINT_WORD_SIGNED:
		case SCC_NEWGRF_PRINT_BYTE_SIGNED:
		case SCC_NEWGRF_PRINT_WORD_UNSIGNED:
		case SCC_NEWGRF_PRINT_BYTE_HEX:
		case SCC_NEWGRF_PRINT_WORD_HEX:
		case SCC_NEWGRF_PRINT_DWORD_HEX:
		case SCC_NEWGRF_PRINT_QWORD_HEX:
		case SCC_NEWGRF_PRINT_DWORD_CURRENCY:
		case SCC_NEWGRF_PRINT_QWORD_CURRENCY:
		case SCC_NEWGRF_PRINT_WORD_STRING_ID:
		case SCC_NEWGRF_PRINT_WORD_DATE_LONG:
		case SCC_NEWGRF_PRINT_DWORD_DATE_LONG:
		case SCC_NEWGRF_PRINT_WORD_DATE_SHORT:
		case SCC_NEWGRF_PRINT_DWORD_DATE_SHORT:
		case SCC_NEWGRF_PRINT_WORD_SPEED:
		case SCC_NEWGRF_PRINT_WORD_VOLUME_LONG:
		case SCC_NEWGRF_PRINT_WORD_VOLUME_SHORT:
		case SCC_NEWGRF_PRINT_WORD_WEIGHT_LONG:
		case SCC_NEWGRF_PRINT_WORD_WEIGHT_SHORT:
		case SCC_NEWGRF_PRINT_WORD_POWER:
		case SCC_NEWGRF_PRINT_WORD_STATION_NAME:
		case SCC_NEWGRF_PRINT_WORD_CARGO_NAME:
			if (argv_size < 1) {
				DEBUG(misc, 0, "Too many NewGRF string parameters.");
				return 0;
			}
			break;

		case SCC_NEWGRF_PRINT_WORD_CARGO_LONG:
		case SCC_NEWGRF_PRINT_WORD_CARGO_SHORT:
		case SCC_NEWGRF_PRINT_WORD_CARGO_TINY:
			if (argv_size < 2) {
				DEBUG(misc, 0, "Too many NewGRF string parameters.");
				return 0;
			}
			break;
	}

	if (_newgrf_textrefstack.used && modify_argv) {
		switch (scc) {
			default: NOT_REACHED();
			case SCC_NEWGRF_PRINT_BYTE_SIGNED:      *argv = _newgrf_textrefstack.PopSignedByte();    break;
			case SCC_NEWGRF_PRINT_QWORD_CURRENCY:   *argv = _newgrf_textrefstack.PopSignedQWord();   break;

			case SCC_NEWGRF_PRINT_DWORD_CURRENCY:
			case SCC_NEWGRF_PRINT_DWORD_SIGNED:     *argv = _newgrf_textrefstack.PopSignedDWord();   break;

			case SCC_NEWGRF_PRINT_BYTE_HEX:         *argv = _newgrf_textrefstack.PopUnsignedByte();  break;
			case SCC_NEWGRF_PRINT_QWORD_HEX:        *argv = _newgrf_textrefstack.PopUnsignedQWord(); break;

			case SCC_NEWGRF_PRINT_WORD_SPEED:
			case SCC_NEWGRF_PRINT_WORD_VOLUME_LONG:
			case SCC_NEWGRF_PRINT_WORD_VOLUME_SHORT:
			case SCC_NEWGRF_PRINT_WORD_SIGNED:      *argv = _newgrf_textrefstack.PopSignedWord();    break;

			case SCC_NEWGRF_PRINT_WORD_HEX:
			case SCC_NEWGRF_PRINT_WORD_WEIGHT_LONG:
			case SCC_NEWGRF_PRINT_WORD_WEIGHT_SHORT:
			case SCC_NEWGRF_PRINT_WORD_POWER:
			case SCC_NEWGRF_PRINT_WORD_STATION_NAME:
			case SCC_NEWGRF_PRINT_WORD_UNSIGNED:    *argv = _newgrf_textrefstack.PopUnsignedWord();  break;

			case SCC_NEWGRF_PRINT_DWORD_DATE_LONG:
			case SCC_NEWGRF_PRINT_DWORD_DATE_SHORT:
			case SCC_NEWGRF_PRINT_DWORD_HEX:        *argv = _newgrf_textrefstack.PopUnsignedDWord(); break;

			case SCC_NEWGRF_PRINT_WORD_DATE_LONG:
			case SCC_NEWGRF_PRINT_WORD_DATE_SHORT:  *argv = _newgrf_textrefstack.PopUnsignedWord() + DAYS_TILL_ORIGINAL_BASE_YEAR; break;

			case SCC_NEWGRF_DISCARD_WORD:           _newgrf_textrefstack.PopUnsignedWord(); break;

			case SCC_NEWGRF_ROTATE_TOP_4_WORDS:     _newgrf_textrefstack.RotateTop4Words(); break;
			case SCC_NEWGRF_PUSH_WORD:              _newgrf_textrefstack.PushWord(Utf8Consume(str)); break;

			case SCC_NEWGRF_UNPRINT: {
				size_t unprint = Utf8Consume(str);
				if (unprint < buf->length()) {
					buf->truncate (buf->length() - unprint);
				} else {
					buf->clear();
				}
				break;
			}

			case SCC_NEWGRF_PRINT_WORD_CARGO_LONG:
			case SCC_NEWGRF_PRINT_WORD_CARGO_SHORT:
			case SCC_NEWGRF_PRINT_WORD_CARGO_TINY:
				argv[0] = GetCargoTranslation(_newgrf_textrefstack.PopUnsignedWord(), _newgrf_textrefstack.grffile);
				argv[1] = _newgrf_textrefstack.PopUnsignedWord();
				break;

			case SCC_NEWGRF_PRINT_WORD_STRING_ID:
				*argv = MapGRFStringID(_newgrf_textrefstack.grffile->grfid, _newgrf_textrefstack.PopUnsignedWord());
				break;

			case SCC_NEWGRF_PRINT_WORD_CARGO_NAME: {
				CargoID cargo = GetCargoTranslation(_newgrf_textrefstack.PopUnsignedWord(), _newgrf_textrefstack.grffile);
				*argv = cargo < NUM_CARGO ? 1 << cargo : 0;
				break;
			}
		}
	} else {
		/* Consume additional parameter characters */
		switch (scc) {
			default: break;

			case SCC_NEWGRF_PUSH_WORD:
			case SCC_NEWGRF_UNPRINT:
				Utf8Consume(str);
				break;
		}
	}

	switch (scc) {
		default: NOT_REACHED();
		case SCC_NEWGRF_PRINT_DWORD_SIGNED:
		case SCC_NEWGRF_PRINT_WORD_SIGNED:
		case SCC_NEWGRF_PRINT_BYTE_SIGNED:
		case SCC_NEWGRF_PRINT_WORD_UNSIGNED:
			return SCC_COMMA;

		case SCC_NEWGRF_PRINT_BYTE_HEX:
		case SCC_NEWGRF_PRINT_WORD_HEX:
		case SCC_NEWGRF_PRINT_DWORD_HEX:
		case SCC_NEWGRF_PRINT_QWORD_HEX:
			return SCC_HEX;

		case SCC_NEWGRF_PRINT_DWORD_CURRENCY:
		case SCC_NEWGRF_PRINT_QWORD_CURRENCY:
			return SCC_CURRENCY_LONG;

		case SCC_NEWGRF_PRINT_WORD_STRING_ID:
			return SCC_NEWGRF_PRINT_WORD_STRING_ID;

		case SCC_NEWGRF_PRINT_WORD_DATE_LONG:
		case SCC_NEWGRF_PRINT_DWORD_DATE_LONG:
			return SCC_DATE_LONG;

		case SCC_NEWGRF_PRINT_WORD_DATE_SHORT:
		case SCC_NEWGRF_PRINT_DWORD_DATE_SHORT:
			return SCC_DATE_SHORT;

		case SCC_NEWGRF_PRINT_WORD_SPEED:
			return SCC_VELOCITY;

		case SCC_NEWGRF_PRINT_WORD_VOLUME_LONG:
			return SCC_VOLUME_LONG;

		case SCC_NEWGRF_PRINT_WORD_VOLUME_SHORT:
			return SCC_VOLUME_SHORT;

		case SCC_NEWGRF_PRINT_WORD_WEIGHT_LONG:
			return SCC_WEIGHT_LONG;

		case SCC_NEWGRF_PRINT_WORD_WEIGHT_SHORT:
			return SCC_WEIGHT_SHORT;

		case SCC_NEWGRF_PRINT_WORD_POWER:
			return SCC_POWER;

		case SCC_NEWGRF_PRINT_WORD_CARGO_LONG:
			return SCC_CARGO_LONG;

		case SCC_NEWGRF_PRINT_WORD_CARGO_SHORT:
			return SCC_CARGO_SHORT;

		case SCC_NEWGRF_PRINT_WORD_CARGO_TINY:
			return SCC_CARGO_TINY;

		case SCC_NEWGRF_PRINT_WORD_CARGO_NAME:
			return SCC_CARGO_LIST;

		case SCC_NEWGRF_PRINT_WORD_STATION_NAME:
			return SCC_STATION_NAME;

		case SCC_NEWGRF_DISCARD_WORD:
		case SCC_NEWGRF_ROTATE_TOP_4_WORDS:
		case SCC_NEWGRF_PUSH_WORD:
		case SCC_NEWGRF_UNPRINT:
			return 0;
	}
}
