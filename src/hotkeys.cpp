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

/**
 * List of all HotkeyLists.
 * This is a pointer to ensure initialisation order with the various static HotkeyList instances.
 */
static SmallVector<HotkeyList*, 16> *_hotkey_lists = NULL;


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
 * Parse a string to the keycodes it represents
 * @param hotkey The hotkey object to add the keycodes to
 * @param value The string to parse
 */
static void ParseHotkeys(Hotkey *hotkey, const char *value)
{
	const char *start = value;
	while (*start != '\0') {
		const char *end = start;
		while (*end != '\0' && *end != ',') end++;
		uint16 keycode = ParseKeycode(start, end);
		if (keycode != 0) hotkey->AddKeycode(keycode);
		start = (*end == ',') ? end + 1: end;
	}
}

/**
 * Convert a hotkey to its string representation so it can be written to the
 * config file. Separate parts of the keycode (like "CTRL" and "F1" are split
 * by a '+'.
 * @param keycode The keycode to convert to a string.
 * @return A string representation of this keycode.
 * @note The return value is a static buffer, strdup the result before calling
 *  this function again.
 */
static const char *KeycodeToString(uint16 keycode)
{
	static char buf[32];
	stringb sb (buf);

	if (keycode & WKC_GLOBAL_HOTKEY) sb.append ("GLOBAL+");
	if (keycode & WKC_SHIFT) sb.append ("SHIFT+");
	if (keycode & WKC_CTRL)  sb.append ("CTRL+");
	if (keycode & WKC_ALT)   sb.append ("ALT+");
	if (keycode & WKC_META)  sb.append ("META+");

	keycode = keycode & ~WKC_SPECIAL_KEYS;
	const KeycodeName *key = find_special_key_by_keycode (keycode);
	if (key != NULL) {
		sb.append (key->name);
		return buf;
	}
	assert(keycode < 128);
	sb.append (keycode);
	return buf;
}

/**
 * Convert all keycodes attached to a hotkey to a single string. If multiple
 * keycodes are attached to the hotkey they are split by a comma.
 * @param hotkey The keycodes of this hotkey need to be converted to a string.
 * @return A string representation of all keycodes.
 * @note The return value is a static buffer, strdup the result before calling
 *  this function again.
 */
const char *SaveKeycodes(const Hotkey *hotkey)
{
	static char buf[128];
	stringb sb (buf);
	for (uint i = 0; i < hotkey->keycodes.Length(); i++) {
		const char *str = KeycodeToString(hotkey->keycodes[i]);
		if (i > 0) sb.append (',');
		sb.append (str);
	}
	return buf;
}

/**
 * Create a new Hotkey object with a single default keycode.
 * @param default_keycode The default keycode for this hotkey.
 * @param name The name of this hotkey.
 * @param num Number of this hotkey, should be unique within the hotkey list.
 */
Hotkey::Hotkey(uint16 default_keycode, const char *name, int num) :
	name(name),
	num(num)
{
	if (default_keycode != 0) this->AddKeycode(default_keycode);
}

/**
 * Create a new Hotkey object with multiple default keycodes.
 * @param default_keycodes An array of default keycodes terminated with 0.
 * @param name The name of this hotkey.
 * @param num Number of this hotkey, should be unique within the hotkey list.
 */
Hotkey::Hotkey(const uint16 *default_keycodes, const char *name, int num) :
	name(name),
	num(num)
{
	const uint16 *keycode = default_keycodes;
	while (*keycode != 0) {
		this->AddKeycode(*keycode);
		keycode++;
	}
}

/**
 * Add a keycode to this hotkey, from now that keycode will be matched
 * in addition to any previously added keycodes.
 * @param keycode The keycode to add.
 */
void Hotkey::AddKeycode(uint16 keycode)
{
	this->keycodes.Include(keycode);
}

HotkeyList::HotkeyList(const char *ini_group, Hotkey *items, GlobalHotkeyHandlerFunc global_hotkey_handler) :
	global_hotkey_handler(global_hotkey_handler), ini_group(ini_group), items(items)
{
	if (_hotkey_lists == NULL) _hotkey_lists = new SmallVector<HotkeyList*, 16>();
	*_hotkey_lists->Append() = this;
}

HotkeyList::~HotkeyList()
{
	_hotkey_lists->Erase(_hotkey_lists->Find(this));
}

/**
 * Load HotkeyList from IniFile.
 * @param ini IniFile to load from.
 */
void HotkeyList::Load(IniFile *ini)
{
	IniGroup *group = ini->GetGroup(this->ini_group);
	for (Hotkey *hotkey = this->items; hotkey->name != NULL; ++hotkey) {
		IniItem *item = group->GetItem(hotkey->name, false);
		if (item != NULL) {
			hotkey->keycodes.Clear();
			if (item->value != NULL) ParseHotkeys(hotkey, item->value);
		}
	}
}

/**
 * Save HotkeyList to IniFile.
 * @param ini IniFile to save to.
 */
void HotkeyList::Save(IniFile *ini) const
{
	IniGroup *group = ini->GetGroup(this->ini_group);
	for (const Hotkey *hotkey = this->items; hotkey->name != NULL; ++hotkey) {
		IniItem *item = group->GetItem(hotkey->name, true);
		item->SetValue(SaveKeycodes(hotkey));
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
	for (const Hotkey *list = this->items; list->name != NULL; ++list) {
		if (list->keycodes.Contains(keycode | WKC_GLOBAL_HOTKEY) || (!global_only && list->keycodes.Contains(keycode))) {
			return list->num;
		}
	}
	return -1;
}


static void SaveLoadHotkeys(bool save)
{
	IniFile *ini = new IniFile();
	ini->LoadFromDisk(_hotkeys_file, BASE_DIR);

	for (HotkeyList **list = _hotkey_lists->Begin(); list != _hotkey_lists->End(); ++list) {
		if (save) {
			(*list)->Save(ini);
		} else {
			(*list)->Load(ini);
		}
	}

	if (save) ini->SaveToDisk(_hotkeys_file);
	delete ini;
}


/** Load the hotkeys from the config file */
void LoadHotkeysFromConfig()
{
	SaveLoadHotkeys(false);
}

/** Save the hotkeys to the config file */
void SaveHotkeysToConfig()
{
	SaveLoadHotkeys(true);
}

void HandleGlobalHotkeys(WChar key, uint16 keycode)
{
	for (HotkeyList **list = _hotkey_lists->Begin(); list != _hotkey_lists->End(); ++list) {
		if ((*list)->global_hotkey_handler == NULL) continue;

		int hotkey = (*list)->CheckMatch(keycode, true);
		if (hotkey >= 0 && ((*list)->global_hotkey_handler(hotkey) == ES_HANDLED)) return;
	}
}

