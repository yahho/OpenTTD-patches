/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file hotkeys.cpp Implementation of hotkey related functions */

#include "stdafx.h"
#include "openttd.h"
#include "hotkeys.h"
#include "ini_type.h"
#include "string.h"
#include "window_gui.h"

char *_hotkeys_file;


/** String representation of a keycode */
struct KeycodeName {
	WindowKeyCodes keycode; ///< Keycode
	uint namelen;           ///< Length of keycode name
	const char *name;       ///< Name of keycode
};

/** Array of non-standard keys that can be used in the hotkeys config file. */
static const KeycodeName special_keys[] = {
#define DEFINE_HOTKEY(keycode,name) { keycode, sizeof name - 1, name }
	DEFINE_HOTKEY (WKC_SHIFT,     "SHIFT"),
	DEFINE_HOTKEY (WKC_CTRL,      "CTRL"),
	DEFINE_HOTKEY (WKC_ALT,       "ALT"),
	DEFINE_HOTKEY (WKC_META,      "META"),
	DEFINE_HOTKEY (WKC_GLOBAL_HOTKEY, "GLOBAL"),
	DEFINE_HOTKEY (WKC_ESC,       "ESC"),
	DEFINE_HOTKEY (WKC_DELETE,    "DEL"),
	DEFINE_HOTKEY (WKC_RETURN,    "RETURN"),
	DEFINE_HOTKEY (WKC_BACKQUOTE, "BACKQUOTE"),
	DEFINE_HOTKEY (WKC_F1,        "F1"),
	DEFINE_HOTKEY (WKC_F2,        "F2"),
	DEFINE_HOTKEY (WKC_F3,        "F3"),
	DEFINE_HOTKEY (WKC_F4,        "F4"),
	DEFINE_HOTKEY (WKC_F5,        "F5"),
	DEFINE_HOTKEY (WKC_F6,        "F6"),
	DEFINE_HOTKEY (WKC_F7,        "F7"),
	DEFINE_HOTKEY (WKC_F8,        "F8"),
	DEFINE_HOTKEY (WKC_F9,        "F9"),
	DEFINE_HOTKEY (WKC_F10,       "F10"),
	DEFINE_HOTKEY (WKC_F11,       "F11"),
	DEFINE_HOTKEY (WKC_F12,       "F12"),
	DEFINE_HOTKEY (WKC_PAUSE,     "PAUSE"),
	DEFINE_HOTKEY (WKC_COMMA,     "COMMA"),
	DEFINE_HOTKEY (WKC_NUM_PLUS,  "NUM_PLUS"),
	DEFINE_HOTKEY (WKC_NUM_MINUS, "NUM_MINUS"),
	DEFINE_HOTKEY (WKC_EQUALS,    "="),
	DEFINE_HOTKEY (WKC_MINUS,     "-"),
#undef DEFINE_HOTKEY
};

static const KeycodeName *find_special_key_by_keycode (uint16 keycode)
{
	for (uint i = 0; i < lengthof(special_keys); i++) {
		if (special_keys[i].keycode == keycode) {
			return &special_keys[i];
		}
	}

	return NULL;
}

static const KeycodeName *find_special_key_by_name (const char *name, size_t len)
{
	for (uint i = 0; i < lengthof(special_keys); i++) {
		if (special_keys[i].namelen == len && strncasecmp (name, special_keys[i].name, len) == 0) {
			return &special_keys[i];
		}
	}

	return NULL;
}


/**
 * Try to parse a single part of a keycode.
 * @param start Start of the string to parse.
 * @param end End of the string to parse.
 * @return A keycode if a match is found or 0.
 */
static uint16 ParseCode(const char *start, const char *end)
{
	assert(start <= end);
	while (start < end && *start == ' ') start++;
	while (end > start && *end == ' ') end--;
	size_t len = end - start;

	const KeycodeName *key = find_special_key_by_name (start, len);
	if (key != NULL) return key->keycode;

	if (len == 1) {
		if (*start >= 'a' && *start <= 'z') return *start - ('a'-'A');
		/* Ignore invalid keycodes */
		if (*(const uint8 *)start < 128) return *start;
	}
	return 0;
}

/**
 * Parse a string representation of a keycode.
 * @param start Start of the input.
 * @param end End of the input.
 * @return A valid keycode or 0.
 */
static uint16 ParseKeycode(const char *start, const char *end)
{
	assert(start <= end);
	uint16 keycode = 0;
	for (;;) {
		const char *cur = start;
		while (*cur != '+' && cur != end) cur++;
		uint16 code = ParseCode(start, cur);
		if (code == 0) return 0;
		if (code & WKC_SPECIAL_KEYS) {
			/* Some completely wrong keycode we don't support. */
			if (code & ~WKC_SPECIAL_KEYS) return 0;
			keycode |= code;
		} else {
			/* Ignore the code if it has more then 1 letter. */
			if (keycode & ~WKC_SPECIAL_KEYS) return 0;
			keycode |= code;
		}
		if (cur == end) break;
		assert(cur < end);
		start = cur + 1;
	}
	return keycode;
}

/**
 * Append the description of a keycode to a string.
 * @param buf The buffer to append the description to.
 * @param keycode The keycode whose description to append
 */
static void AppendKeycodeDescription (stringb *buf, uint16 keycode)
{
	if (keycode & WKC_GLOBAL_HOTKEY) buf->append ("GLOBAL+");
	if (keycode & WKC_SHIFT) buf->append ("SHIFT+");
	if (keycode & WKC_CTRL)  buf->append ("CTRL+");
	if (keycode & WKC_ALT)   buf->append ("ALT+");
	if (keycode & WKC_META)  buf->append ("META+");

	keycode &= ~WKC_SPECIAL_KEYS;
	const KeycodeName *key = find_special_key_by_keycode (keycode);
	if (key != NULL) {
		buf->append (key->name);
	} else {
		assert (keycode < 128);
		buf->append (keycode);
	}
}


/**
 * List of all HotkeyLists.
 * This is a pointer to ensure initialisation order with the various static HotkeyList instances.
 */
static ForwardList<HotkeyList> *hotkey_lists;

void HotkeyList::init (void)
{
	if (hotkey_lists == NULL) hotkey_lists = new ForwardList<HotkeyList>;
	hotkey_lists->append (this);
}

HotkeyList::~HotkeyList()
{
	hotkey_lists->remove (this);
}

/**
 * Load HotkeyList from IniFile.
 * @param ini IniFile to load from.
 */
void HotkeyList::Load(IniFile *ini)
{
	const IniGroup *group = ini->get_group (this->ini_group);

	this->mappings.clear();

	for (uint i = 0; i < this->ndescs; i++) {
		const Hotkey *hotkey = &this->descs[i];

		const IniItem *item = group->find (hotkey->name);
		if (item == NULL) {
			static const uint16 Hotkey::* const defaults [] = { &Hotkey::default0, &Hotkey::default1, &Hotkey::default2, &Hotkey::default3 };
			for (uint i = 0; i < lengthof(defaults); i++) {
				uint16 keycode = hotkey->*defaults[i];
				if (keycode == 0) break;
				this->push_mapping (keycode, hotkey->num);
			}
		} else if (item->value != NULL) {
			const char *start = item->value;
			while (*start != '\0') {
				const char *end = start;
				while (*end != '\0' && *end != ',') end++;
				uint16 keycode = ParseKeycode (start, end);
				if (keycode != 0) this->push_mapping (keycode, hotkey->num);
				start = (*end == ',') ? end + 1: end;
			}
		}
	}
}

/**
 * Save HotkeyList to IniFile.
 * @param ini IniFile to save to.
 */
void HotkeyList::Save(IniFile *ini) const
{
	IniGroup *group = ini->get_group (this->ini_group);

	for (uint i = 0; i < this->ndescs; i++) {
		const Hotkey *hotkey = &this->descs[i];

		sstring<128> buf;
		std::vector<Mapping>::const_iterator it = this->mappings.begin();
		for (; it != this->mappings.end(); it++) {
			if (it->value == hotkey->num) {
				if (!buf.empty()) buf.append (',');
				AppendKeycodeDescription (&buf, it->keycode);
			}
		}

		group->get_item(hotkey->name)->SetValue (buf.c_str());
	}
}

/**
 * Check if a keycode is bound to something.
 * @param keycode The keycode that was pressed
 * @param global_only Limit the search to hotkeys defined as 'global'.
 * @return The number of the matching hotkey or -1.
 */
int HotkeyList::CheckMatch(uint16 keycode, bool global_only) const
{
	std::vector<Mapping>::const_iterator it = this->mappings.begin();
	for (; it != this->mappings.end(); it++) {
		if ((it->keycode == (keycode | WKC_GLOBAL_HOTKEY)) || (!global_only && (it->keycode == keycode))) {
			return it->value;
		}
	}
	return -1;
}


/** Load the hotkeys from the config file */
void LoadHotkeysFromConfig()
{
	IniFile ini (_hotkeys_file, NO_DIRECTORY);

	ForwardList<HotkeyList>::iterator it (hotkey_lists->begin());
	for (; it != hotkey_lists->end(); it++) {
		it->Load (&ini);
	}
}

/** Save the hotkeys to the config file */
void SaveHotkeysToConfig()
{
	IniFile ini (_hotkeys_file, NO_DIRECTORY);

	ForwardList<HotkeyList>::const_iterator it (hotkey_lists->cbegin());
	for (; it != hotkey_lists->cend(); it++) {
		it->Save (&ini);
	}

	ini.SaveToDisk (_hotkeys_file);
}

void HandleGlobalHotkeys(WChar key, uint16 keycode)
{
	ForwardList<HotkeyList>::const_iterator it (hotkey_lists->cbegin());
	for (; it != hotkey_lists->cend(); it++) {
		if (it->global_hotkey_handler == NULL) continue;

		int hotkey = it->CheckMatch (keycode, true);
		if (hotkey >= 0 && it->global_hotkey_handler(hotkey)) return;
	}
}

