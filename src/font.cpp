/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file font.cpp Cache for characters from fonts. */

#include "stdafx.h"
#include "debug.h"
#include "font.h"
#include "blitter/blitter.h"
#include "core/math_func.hpp"
#include "string.h"
#include "strings_func.h"
#include "zoom_type.h"
#include "gfx_layout.h"
#include "zoom_func.h"

#include "table/sprites.h"
#include "table/control_codes.h"

static const int MAX_FONT_SIZE     = 72; ///< Maximum font size.

/** Default heights for the different sizes of fonts. */
static const int _default_font_height[FS_END]   = {10, 6, 18, 10};
static const int _default_font_ascender[FS_END] = { 8, 5, 15,  8};

#ifdef WITH_FREETYPE

static FT_Library _library = NULL;

FreeTypeSettings _freetype;

static const byte FACE_COLOUR   = 1;
static const byte SHADOW_COLOUR = 2;

#endif /* WITH_FREETYPE */


/** Reset font metrics of the font. */
void FontCache::ResetFontMetrics (void)
{
	FontSize fs = this->fs;
	int height   = _default_font_height[fs];
	int ascender = _default_font_ascender[fs];
	this->height    = ScaleGUITrad (height);
	this->ascender  = ScaleGUITrad (ascender);
	this->descender = ScaleGUITrad (ascender - height);
	this->units_per_em = 1;

	for (uint i = 0; i != 224; i++) {
		this->glyph_widths[i] = this->GetGlyphWidth (this->MapCharToGlyph (i + 32));
	}

	byte widest_digit = 9;
	byte digit_width = this->glyph_widths['9' - 32];
	for (byte i = 8; i > 0; i--) {
		byte w = this->glyph_widths[i + '0' - 32];
		if (w > digit_width) {
			widest_digit = i;
			digit_width = w;
		}
	}
	this->widest_digit_nonnull = widest_digit;

	byte w = this->glyph_widths['0' - 32];
	if (w > digit_width) {
		widest_digit = 0;
		digit_width = w;
	}
	this->widest_digit = widest_digit;
	this->digit_width = digit_width;
}

/**
 * Create a new font cache.
 * @param fs The size of the font.
 */
FontCache::FontCache (FontSize fs) :
#ifdef WITH_FREETYPE
	missing_sprite (NULL), font_tables(), face (NULL),
#endif /* WITH_FREETYPE */
	fs (fs)
{
	memset (this->spriteid_map, 0, sizeof(this->spriteid_map));
#ifdef WITH_FREETYPE
	memset (this->sprite_map, 0, sizeof(this->sprite_map));
#endif /* WITH_FREETYPE */

	this->ResetFontMetrics();
	this->InitializeUnicodeGlyphMap();
}

/** Clean everything up. */
FontCache::~FontCache()
{
#ifdef WITH_FREETYPE
	this->UnloadFreeTypeFont();
#endif /* WITH_FREETYPE */
	this->ClearGlyphToSpriteMap();
}


/**
 * Get height of a character for a given font size.
 * @param size Font size to get height of
 * @return     Height of characters in the given font (pixels)
 */
int GetCharacterHeight(FontSize size)
{
	return FontCache::Get(size)->GetHeight();
}


SpriteID FontCache::GetUnicodeGlyph (GlyphID key) const
{
	SpriteID *p = this->spriteid_map[GB(key, 8, 8)];
	return (p == NULL) ? 0 : p[GB(key, 0, 8)];
}

void FontCache::SetUnicodeGlyph (GlyphID key, SpriteID sprite)
{
	SpriteID **p = &this->spriteid_map[GB(key, 8, 8)];
	if (*p == NULL) *p = xcalloct<SpriteID>(256);
	(*p)[GB(key, 0, 8)] = sprite;
}

void FontCache::InitializeUnicodeGlyphMap (void)
{
	static const uint ASCII_LETTERSTART = 32; ///< First printable ASCII letter.

	/* Font sprites are contiguous and arranged by font size. */
	static const uint delta = 256 - ASCII_LETTERSTART;
	assert_compile (SPR_ASCII_SPACE_SMALL == SPR_ASCII_SPACE + delta);
	assert_compile (SPR_ASCII_SPACE_BIG   == SPR_ASCII_SPACE + 2 * delta);

	/* Clear out existing glyph map if it exists */
	this->ClearGlyphToSpriteMap();

	assert_compile (FS_NORMAL == 0);
	assert_compile (FS_SMALL  == 1);
	assert_compile (FS_LARGE  == 2);
	assert_compile (FS_MONO   == 3);

	/* (this->fs % 3) maps FS_MONO to FS_NORMAL. */
	SpriteID base = SPR_ASCII_SPACE + (this->fs % 3) * delta - ASCII_LETTERSTART;

	for (uint i = ASCII_LETTERSTART; i < 256; i++) {
		SpriteID sprite = base + i;
		if (!SpriteExists(sprite)) continue;
		this->SetUnicodeGlyph(i, sprite);
		this->SetUnicodeGlyph(i + SCC_SPRITE_START, sprite);
	}

	/* Glyphs to be accessed through an SCC_* enum entry only. */
	static const byte clear_list[] = {
		0xAA, // Feminine ordinal indicator / Down arrow
		0xAC, // Not sign / Tick mark
		0xAF, // Macron / Right arrow
		0xB4, // Acute accent / Train symbol
		0xB5, // Micro sign / Truck symbol
		0xB6, // Pilcrow sign / Bus symbol
		0xB7, // Middle dot / Aircraft symbol
		0xB8, // Cedilla / Ship symbol
		0xB9, // Superscript 1 / Superscript -1
		0xBC, // One quarter / Small up arrow
		0xBD, // One half / Small down arrow
	};

	for (uint i = 0; i < lengthof(clear_list); i++) {
		this->SetUnicodeGlyph (clear_list[i], 0);
	}

	/* Default unicode mapping table for sprite based glyphs.
	 * This table allows us use unicode characters even though the glyphs
	 * don't exist, or are in the wrong place, in the standard sprite
	 * fonts. This is not used for FreeType rendering */
	static const uint16 translation_map[][2] = {
		{ 0x00A0, 0x20 }, // Non-breaking space / Up arrow
		{ 0x00AD, 0x20 }, // Soft hyphen / X mark
		{ 0x0178, 0x9F }, // Capital letter Y with diaeresis
		{ 0x010D, 0x63 }, // Small letter c with caron
	};

	for (uint i = 0; i < lengthof(translation_map); i++) {
		WChar code = translation_map[i][0];
		byte  key  = translation_map[i][1];
		this->SetUnicodeGlyph (code, base + key);
	}
}

/**
 * Clear the glyph to sprite mapping.
 */
void FontCache::ClearGlyphToSpriteMap (void)
{
	for (uint i = 0; i < 256; i++) {
		free (this->spriteid_map[i]);
		this->spriteid_map[i] = NULL;
	}
}

SpriteID FontCache::GetGlyphSprite (GlyphID key) const
{
	SpriteID sprite = this->GetUnicodeGlyph(key);
	if (sprite == 0) sprite = this->GetUnicodeGlyph('?');
	return sprite;
}


#ifdef WITH_FREETYPE

/** Get the FreeType settings struct for a given font size. */
static FreeTypeSubSetting *GetFreeTypeSettings (FontSize fs)
{
	switch (fs) {
		default: NOT_REACHED();
		case FS_NORMAL: return &_freetype.medium;
		case FS_SMALL:  return &_freetype.small;
		case FS_LARGE:  return &_freetype.large;
		case FS_MONO:   return &_freetype.mono;
	}
}

/**
 * Set the right font names.
 * @param settings  The settings to modify.
 * @param font_name The new font name.
 * @param searcher  The searcher that says whether we are dealing with a monospaced font.
 */
static void SetFontNames (FreeTypeSettings *settings, const char *font_name,
	const MissingGlyphSearcher *searcher)
{
	if (searcher->Monospace()) {
		bstrcpy (settings->mono.font, font_name);
	} else {
		bstrcpy (settings->small.font,  font_name);
		bstrcpy (settings->medium.font, font_name);
		bstrcpy (settings->large.font,  font_name);
	}
}

/**
 * Get the font loaded into a Freetype face by using a font-name.
 * If no appropriate font is found, the function returns an error.
 */

#ifdef WIN32

/* ========================================================================================
 * Windows support
 * ======================================================================================== */

#include <vector>
#include "core/pointer.h"
#include "core/alloc_func.hpp"
#include <windows.h>
#include <shlobj.h> /* SHGetFolderPath */
#include "os/windows/win32.h"

/**
 * Get the short DOS 8.3 format for paths.
 * FreeType doesn't support Unicode filenames and Windows' fopen (as used
 * by FreeType) doesn't support UTF-8 filenames. So we have to convert the
 * filename into something that isn't UTF-8 but represents the Unicode file
 * name. This is the short DOS 8.3 format. This does not contain any
 * characters that fopen doesn't support.
 * @param long_path the path in system encoding.
 * @return the short path in ANSI (ASCII).
 */
static const char *GetShortPath (const TCHAR *long_path)
{
	static char short_path[MAX_PATH];
#ifdef UNICODE
	WCHAR short_path_w[MAX_PATH];
	GetShortPathName(long_path, short_path_w, lengthof(short_path_w));
	WideCharToMultiByte(CP_ACP, 0, short_path_w, -1, short_path, lengthof(short_path), NULL, NULL);
#else
	/* Technically not needed, but do it for consistency. */
	GetShortPathName(long_path, short_path, lengthof(short_path));
#endif
	return short_path;
}

/* Get the font file to be loaded into Freetype by looping the registry
 * location where windows lists all installed fonts. Not very nice, will
 * surely break if the registry path changes, but it works. Much better
 * solution would be to use CreateFont, and extract the font data from it
 * by GetFontData. The problem with this is that the font file needs to be
 * kept in memory then until the font is no longer needed. This could mean
 * an additional memory usage of 30MB (just for fonts!) when using an eastern
 * font for all font sizes */
#define FONT_DIR_NT "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"
#define FONT_DIR_9X "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts"
static FT_Error GetFontByFaceName (const char *font_name, const char *alt_name, FT_Face *face)
{
	FT_Error err = FT_Err_Cannot_Open_Resource;
	HKEY hKey;
	LONG ret;
	TCHAR vbuffer[MAX_PATH], dbuffer[256];
	TCHAR *pathbuf;
	const char *font_path;
	uint index;
	size_t path_len;

	/* On windows NT (2000, NT3.5, XP, etc.) the fonts are stored in the
	 * "Windows NT" key, on Windows 9x in the Windows key. To save us having
	 * to retrieve the windows version, we'll just query both */
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T(FONT_DIR_NT), 0, KEY_READ, &hKey);
	if (ret != ERROR_SUCCESS) ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T(FONT_DIR_9X), 0, KEY_READ, &hKey);

	if (ret != ERROR_SUCCESS) {
		DEBUG(freetype, 0, "Cannot open registry key HKLM\\SOFTWARE\\Microsoft\\Windows (NT)\\CurrentVersion\\Fonts");
		return err;
	}

	/* Convert font name to file system encoding. */
	TCHAR *font_namep = _tcsdup(OTTD2FS(font_name));

	for (index = 0;; index++) {
		TCHAR *s;
		DWORD vbuflen = lengthof(vbuffer);
		DWORD dbuflen = lengthof(dbuffer);

		ret = RegEnumValue(hKey, index, vbuffer, &vbuflen, NULL, NULL, (byte*)dbuffer, &dbuflen);
		if (ret != ERROR_SUCCESS) goto registry_no_font_found;

		/* The font names in the registry are of the following 3 forms:
		 * - ADMUI3.fon
		 * - Book Antiqua Bold (TrueType)
		 * - Batang & BatangChe & Gungsuh & GungsuhChe (TrueType)
		 * We will strip the font-type '()' if any and work with the font name
		 * itself, which must match exactly; if...
		 * TTC files, font files which contain more than one font are separated
		 * by '&'. Our best bet will be to do substr match for the fontname
		 * and then let FreeType figure out which index to load */
		s = _tcschr(vbuffer, _T('('));
		if (s != NULL) s[-1] = '\0';

		if (_tcschr(vbuffer, _T('&')) == NULL) {
			if (_tcsicmp(vbuffer, font_namep) == 0) break;
		} else {
			if (_tcsstr(vbuffer, font_namep) != NULL) break;
		}
	}

	if (!SUCCEEDED(OTTDSHGetFolderPath(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, vbuffer))) {
		DEBUG(freetype, 0, "SHGetFolderPath cannot return fonts directory");
		goto folder_error;
	}

	/* Some fonts are contained in .ttc files, TrueType Collection fonts. These
	 * contain multiple fonts inside this single file. GetFontData however
	 * returns the whole file, so we need to check each font inside to get the
	 * proper font. */
	path_len = _tcslen(vbuffer) + _tcslen(dbuffer) + 2; // '\' and terminating nul.
	pathbuf = AllocaM(TCHAR, path_len);
	_sntprintf(pathbuf, path_len, _T("%s\\%s"), vbuffer, dbuffer);

	/* Convert the path into something that FreeType understands. */
	font_path = GetShortPath(pathbuf);

	index = 0;
	do {
		err = FT_New_Face(_library, font_path, index, face);
		if (err != FT_Err_Ok) break;

		if (strncasecmp(font_name, (*face)->family_name, strlen((*face)->family_name)) == 0) break;
		/* Try english name if font name failed */
		if (alt_name != NULL && strncasecmp(alt_name, (*face)->family_name, strlen((*face)->family_name)) == 0) break;
		err = FT_Err_Cannot_Open_Resource;

	} while ((FT_Long)++index != (*face)->num_faces);


folder_error:
registry_no_font_found:
	free(font_namep);
	RegCloseKey(hKey);
	return err;
}

static FT_Error GetFontByFaceName (const char *font_name, FT_Face *face)
{
	return GetFontByFaceName (font_name, NULL, face);
}

/**
 * Get the English font name for a buffer of font data (see below).
 * @param buf the buffer with the font data
 * @param len the length of the buffer
 * @param fontname the buffer where to store the English name
 * @return whether the name was found
 */
static bool GetEnglishFontName (const byte *buf, size_t len,
	char (&fontname) [MAX_PATH])
{
	if (len < 6) return NULL;

	if ((buf[0] != 0) || (buf[1] != 0)) return NULL;

	uint count = (buf[2] << 8) + buf[3];
	if (count > (len - 6) / 12) return NULL;

	size_t data_offset = (buf[4] << 8) + buf[5];
	if (data_offset > len) return NULL;
	const byte *data = buf + data_offset;
	size_t data_len = len - data_offset;

	for (buf += 6; count > 0; count--, buf += 12) {
		uint platform = (buf[0] << 8) + buf[1];
		/* ignore encoding (bytes 2 and 3) */
		uint language = (buf[4] << 8) + buf[5];
		if ((platform != 1 || language != 0) &&      // Macintosh English
				(platform != 3 || language != 0x0409)) { // Microsoft English (US)
			continue;
		}

		uint name = (buf[6] << 8) + buf[7];
		if (name != 1) continue;

		uint offset = (buf[10] << 8) + buf[11];
		if (offset > data_len) continue;

		uint length = (buf[8] << 8) + buf[9];
		if (length > (data_len - offset)) continue;

		bstrfmt (fontname, "%.*s", length, data + offset);
		return true;
	}

	return false;
}

/**
 * Fonts can have localised names and when the system locale is the same as
 * one of those localised names Windows will always return that localised name
 * instead of allowing to get the non-localised (English US) name of the font.
 * This will later on give problems as freetype uses the non-localised name of
 * the font and we need to compare based on that name.
 * Windows furthermore DOES NOT have an API to get the non-localised name nor
 * can we override the system locale. This means that we have to actually read
 * the font itself to gather the font name we want.
 * Based on: http://blogs.msdn.com/michkap/archive/2006/02/13/530814.aspx
 * @param logfont the font information to get the english name of.
 * @param fontname the buffer where to store the English name
 */
static void GetEnglishFontName (const ENUMLOGFONTEX *logfont,
	char (&fontname) [MAX_PATH])
{
	bool found = false;

	HFONT font = CreateFontIndirect(&logfont->elfLogFont);
	if (font != NULL) {
		HDC dc = GetDC (NULL);
		HGDIOBJ oldfont = SelectObject (dc, font);
		DWORD dw = GetFontData (dc, 'eman', 0, NULL, 0);
		if (dw != GDI_ERROR) {
			byte *buf = xmalloct<byte>(dw);
			if (GetFontData (dc, 'eman', 0, buf, dw) != GDI_ERROR) {
				found = GetEnglishFontName (buf, dw, fontname);
			}
			free(buf);
		}
		SelectObject (dc, oldfont);
		ReleaseDC (NULL, dc);
		DeleteObject (font);
	}

	if (!found) {
		bstrcpy (fontname, WIDE_TO_MB((const TCHAR*)logfont->elfFullName));
	}
}

struct EFCParam {
	FreeTypeSettings *settings;
	LOCALESIGNATURE  locale;
	MissingGlyphSearcher *callback;
	std::vector <ttd_unique_free_ptr <TCHAR> > fonts;
};

static int CALLBACK EnumFontCallback(const ENUMLOGFONTEX *logfont, const NEWTEXTMETRICEX *metric, DWORD type, LPARAM lParam)
{
	EFCParam *info = (EFCParam *)lParam;

	/* Skip duplicates */
	const TCHAR *fname = (const TCHAR*)logfont->elfFullName;
	for (uint i = 0; i < info->fonts.size(); i++) {
		if (_tcscmp (info->fonts[i].get(), fname) == 0) return 1;
	}
	info->fonts.push_back (ttd_unique_free_ptr<TCHAR> (_tcsdup (fname)));
	/* Only use TrueType fonts */
	if (!(type & TRUETYPE_FONTTYPE)) return 1;
	/* Don't use SYMBOL fonts */
	if (logfont->elfLogFont.lfCharSet == SYMBOL_CHARSET) return 1;
	/* Use monospaced fonts when asked for it. */
	if (info->callback->Monospace() && (logfont->elfLogFont.lfPitchAndFamily & (FF_MODERN | FIXED_PITCH)) != (FF_MODERN | FIXED_PITCH)) return 1;

	/* The font has to have at least one of the supported locales to be usable. */
	if ((metric->ntmFontSig.fsCsb[0] & info->locale.lsCsbSupported[0]) == 0 && (metric->ntmFontSig.fsCsb[1] & info->locale.lsCsbSupported[1]) == 0) {
		/* On win9x metric->ntmFontSig seems to contain garbage. */
		FONTSIGNATURE fs;
		memset(&fs, 0, sizeof(fs));
		HFONT font = CreateFontIndirect(&logfont->elfLogFont);
		if (font != NULL) {
			HDC dc = GetDC(NULL);
			HGDIOBJ oldfont = SelectObject(dc, font);
			GetTextCharsetInfo(dc, &fs, 0);
			SelectObject(dc, oldfont);
			ReleaseDC(NULL, dc);
			DeleteObject(font);
		}
		if ((fs.fsCsb[0] & info->locale.lsCsbSupported[0]) == 0 && (fs.fsCsb[1] & info->locale.lsCsbSupported[1]) == 0) return 1;
	}

	char font_name[MAX_PATH];
	convert_from_fs (fname, font_name, lengthof(font_name));

	/* Add english name after font name */
	char english_name [MAX_PATH];
	GetEnglishFontName (logfont, english_name);

	/* Check whether we can actually load the font. */
	bool ft_init = _library != NULL;
	bool found = false;
	FT_Face face;
	/* Init FreeType if needed. */
	if ((ft_init || FT_Init_FreeType(&_library) == FT_Err_Ok) && GetFontByFaceName (font_name, english_name, &face) == FT_Err_Ok) {
		FT_Done_Face(face);
		found = true;
	}
	if (!ft_init) {
		/* Uninit FreeType if we did the init. */
		FT_Done_FreeType(_library);
		_library = NULL;
	}

	if (!found) return 1;

	SetFontNames (info->settings, font_name, info->callback);
	if (info->callback->FindMissingGlyphs()) return 1;
	DEBUG(freetype, 1, "Fallback font: %s (%s)", font_name, english_name);
	return 0; // stop enumerating
}

bool SetFallbackFont(FreeTypeSettings *settings, const char *language_isocode, int winlangid, MissingGlyphSearcher *callback)
{
	DEBUG(freetype, 1, "Trying fallback fonts");
	EFCParam langInfo;
	if (GetLocaleInfo(MAKELCID(winlangid, SORT_DEFAULT), LOCALE_FONTSIGNATURE, (LPTSTR)&langInfo.locale, sizeof(langInfo.locale) / sizeof(TCHAR)) == 0) {
		/* Invalid langid or some other mysterious error, can't determine fallback font. */
		DEBUG(freetype, 1, "Can't get locale info for fallback font (langid=0x%x)", winlangid);
		return false;
	}
	langInfo.settings = settings;
	langInfo.callback = callback;

	LOGFONT font;
	/* Enumerate all fonts. */
	font.lfCharSet = DEFAULT_CHARSET;
	font.lfFaceName[0] = '\0';
	font.lfPitchAndFamily = 0;

	HDC dc = GetDC(NULL);
	int ret = EnumFontFamiliesEx(dc, &font, (FONTENUMPROC)&EnumFontCallback, (LPARAM)&langInfo, 0);
	ReleaseDC(NULL, dc);
	return ret == 0;
}

#elif defined(__APPLE__) /* end ifdef Win32 */

/* ========================================================================================
 * OSX support
 * ======================================================================================== */

#include "os/macosx/macos.h"

static FT_Error GetFontByFaceName (const char *font_name, FT_Face *face)
{
	FT_Error err = FT_Err_Cannot_Open_Resource;

	/* Get font reference from name. */
	CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, font_name, kCFStringEncodingUTF8);
	ATSFontRef font = ATSFontFindFromName(name, kATSOptionFlagsDefault);
	CFRelease(name);
	if (font == kInvalidFont) return err;

	/* Get a file system reference for the font. */
	FSRef ref;
	OSStatus os_err = -1;
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
	if (MacOSVersionIsAtLeast(10, 5, 0)) {
		os_err = ATSFontGetFileReference(font, &ref);
	} else
#endif
	{
#if (MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5) && !defined(__LP64__)
		/* This type was introduced with the 10.5 SDK. */
#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
	#define ATSFSSpec FSSpec
#endif
		FSSpec spec;
		os_err = ATSFontGetFileSpecification(font, (ATSFSSpec *)&spec);
		if (os_err == noErr) os_err = FSpMakeFSRef(&spec, &ref);
#endif
	}

	if (os_err == noErr) {
		/* Get unix path for file. */
		UInt8 file_path[PATH_MAX];
		if (FSRefMakePath(&ref, file_path, sizeof(file_path)) == noErr) {
			DEBUG(freetype, 3, "Font path for %s: %s", font_name, file_path);
			err = FT_New_Face(_library, (const char *)file_path, 0, face);
		}
	}

	return err;
}

bool SetFallbackFont(FreeTypeSettings *settings, const char *language_isocode, int winlangid, MissingGlyphSearcher *callback)
{
	bool result = false;

#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
	if (MacOSVersionIsAtLeast(10, 5, 0)) {
		/* Determine fallback font using CoreText. This uses the language isocode
		 * to find a suitable font. CoreText is available from 10.5 onwards. */
		char lang[16];
		if (strcmp(language_isocode, "zh_TW") == 0) {
			/* Traditional Chinese */
			bstrcpy (lang, "zh-Hant");
		} else if (strcmp(language_isocode, "zh_CN") == 0) {
			/* Simplified Chinese */
			bstrcpy (lang, "zh-Hans");
		} else {
			/* Just copy the first part of the isocode. */
			bstrcpy (lang, language_isocode);
			char *sep = strchr(lang, '_');
			if (sep != NULL) *sep = '\0';
		}

		/* Create a font descriptor matching the wanted language and latin (english) glyphs. */
		CFStringRef lang_codes[2];
		lang_codes[0] = CFStringCreateWithCString(kCFAllocatorDefault, lang, kCFStringEncodingUTF8);
		lang_codes[1] = CFSTR("en");
		CFArrayRef lang_arr = CFArrayCreate(kCFAllocatorDefault, (const void **)lang_codes, lengthof(lang_codes), &kCFTypeArrayCallBacks);
		CFDictionaryRef lang_attribs = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&kCTFontLanguagesAttribute, (const void **)&lang_arr, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CTFontDescriptorRef lang_desc = CTFontDescriptorCreateWithAttributes(lang_attribs);
		CFRelease(lang_arr);
		CFRelease(lang_attribs);
		CFRelease(lang_codes[0]);

		/* Get array of all font descriptors for the wanted language. */
		CFSetRef mandatory_attribs = CFSetCreate(kCFAllocatorDefault, (const void **)&kCTFontLanguagesAttribute, 1, &kCFTypeSetCallBacks);
		CFArrayRef descs = CTFontDescriptorCreateMatchingFontDescriptors(lang_desc, mandatory_attribs);
		CFRelease(mandatory_attribs);
		CFRelease(lang_desc);

		for (CFIndex i = 0; descs != NULL && i < CFArrayGetCount(descs); i++) {
			CTFontDescriptorRef font = (CTFontDescriptorRef)CFArrayGetValueAtIndex(descs, i);

			/* Get font traits. */
			CFDictionaryRef traits = (CFDictionaryRef)CTFontDescriptorCopyAttribute(font, kCTFontTraitsAttribute);
			CTFontSymbolicTraits symbolic_traits;
			CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(traits, kCTFontSymbolicTrait), kCFNumberIntType, &symbolic_traits);
			CFRelease(traits);

			/* Skip symbol fonts and vertical fonts. */
			if ((symbolic_traits & kCTFontClassMaskTrait) == (CTFontStylisticClass)kCTFontSymbolicClass || (symbolic_traits & kCTFontVerticalTrait)) continue;
			/* Skip bold fonts (especially Arial Bold, which looks worse than regular Arial). */
			if (symbolic_traits & kCTFontBoldTrait) continue;
			/* Select monospaced fonts if asked for. */
			if (((symbolic_traits & kCTFontMonoSpaceTrait) == kCTFontMonoSpaceTrait) != callback->Monospace()) continue;

			/* Get font name. */
			char name[128];
			CFStringRef font_name = (CFStringRef)CTFontDescriptorCopyAttribute(font, kCTFontDisplayNameAttribute);
			CFStringGetCString(font_name, name, lengthof(name), kCFStringEncodingUTF8);
			CFRelease(font_name);

			/* There are some special fonts starting with an '.' and the last
			 * resort font that aren't usable. Skip them. */
			if (name[0] == '.' || strncmp(name, "LastResort", 10) == 0) continue;

			/* Save result. */
			SetFontNames (settings, name, callback);
			if (!callback->FindMissingGlyphs()) {
				DEBUG(freetype, 2, "CT-Font for %s: %s", language_isocode, name);
				result = true;
				break;
			}
		}
		if (descs != NULL) CFRelease(descs);
	} else
#endif
	{
		/* Create a font iterator and iterate over all fonts that
		 * are available to the application. */
		ATSFontIterator itr;
		ATSFontRef font;
		ATSFontIteratorCreate(kATSFontContextLocal, NULL, NULL, kATSOptionFlagsDefaultScope, &itr);
		while (!result && ATSFontIteratorNext(itr, &font) == noErr) {
			/* Get font name. */
			char name[128];
			CFStringRef font_name;
			ATSFontGetName(font, kATSOptionFlagsDefault, &font_name);
			CFStringGetCString(font_name, name, lengthof(name), kCFStringEncodingUTF8);

			bool monospace = IsMonospaceFont(font_name);
			CFRelease(font_name);

			/* Select monospaced fonts if asked for. */
			if (monospace != callback->Monospace()) continue;

			/* We only want the base font and not bold or italic variants. */
			if (strstr(name, "Italic") != NULL || strstr(name, "Bold")) continue;

			/* Skip some inappropriate or ugly looking fonts that have better alternatives. */
			if (name[0] == '.' || strncmp(name, "Apple Symbols", 13) == 0 || strncmp(name, "LastResort", 10) == 0) continue;

			/* Save result. */
			SetFontNames (settings, name, callback);
			if (!callback->FindMissingGlyphs()) {
				DEBUG(freetype, 2, "ATS-Font for %s: %s", language_isocode, name);
				result = true;
				break;
			}
		}
		ATSFontIteratorRelease(&itr);
	}

	if (!result) {
		/* For some OS versions, the font 'Arial Unicode MS' does not report all languages it
		 * supports. If we didn't find any other font, just try it, maybe we get lucky. */
		SetFontNames (settings, "Arial Unicode MS", callback);
		result = !callback->FindMissingGlyphs();
	}

	callback->FindMissingGlyphs();
	return result;
}

#elif defined(WITH_FONTCONFIG) /* end ifdef __APPLE__ */

#include <fontconfig/fontconfig.h>

/* ========================================================================================
 * FontConfig (unix) support
 * ======================================================================================== */

static FT_Error GetFontByFaceName (const char *font_name, FT_Face *face)
{
	if (!FcInit()) {
		ShowInfoF("Unable to load font configuration");
		return FT_Err_Cannot_Open_Resource;
	}

	/* Split & strip the font's style */
	char *font_family = xstrdup (font_name);
	char *font_style = strchr (font_family, ',');
	if (font_style != NULL) {
		font_style[0] = '\0';
		font_style++;
		while (*font_style == ' ' || *font_style == '\t') font_style++;
	}

	/* Resolve the name and populate the information structure */
	FcPattern *pat = FcNameParse ((FcChar8*)font_family);
	if (font_style != NULL) FcPatternAddString(pat, FC_STYLE, (FcChar8*)font_style);
	FcConfigSubstitute(0, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);
	FcFontSet *fs = FcFontSetCreate();
	FcResult result;
	FcPattern *match = FcFontMatch (0, pat, &result);

	FT_Error err = FT_Err_Cannot_Open_Resource;
	if (fs != NULL && match != NULL) {
		int i;
		FcChar8 *family;
		FcChar8 *style;
		FcChar8 *file;
		FcFontSetAdd(fs, match);

		for (i = 0; err != FT_Err_Ok && i < fs->nfont; i++) {
			/* Try the new filename */
			if (FcPatternGetString(fs->fonts[i], FC_FILE,   0, &file)   == FcResultMatch &&
					FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, &family) == FcResultMatch &&
					FcPatternGetString(fs->fonts[i], FC_STYLE,  0, &style)  == FcResultMatch) {

				/* The correct style? */
				if (font_style != NULL && strcasecmp(font_style, (char*)style) != 0) continue;

				/* Font config takes the best shot, which, if the family name is spelled
				 * wrongly a 'random' font, so check whether the family name is the
				 * same as the supplied name */
				if (strcasecmp(font_family, (char*)family) == 0) {
					err = FT_New_Face(_library, (char *)file, 0, face);
				}
			}
		}
	}

	free(font_family);
	FcPatternDestroy(pat);
	FcFontSetDestroy(fs);
	FcFini();

	return err;
}

bool SetFallbackFont(FreeTypeSettings *settings, const char *language_isocode, int winlangid, MissingGlyphSearcher *callback)
{
	if (!FcInit()) return false;

	bool ret = false;

	/* Fontconfig doesn't handle full language isocodes, only the part
	 * before the _ of e.g. en_GB is used, so "remove" everything after
	 * the _. */
	char lang[16];
	bstrfmt (lang, ":lang=%s", language_isocode);
	char *split = strchr(lang, '_');
	if (split != NULL) *split = '\0';

	/* First create a pattern to match the wanted language. */
	FcPattern *pat = FcNameParse((FcChar8*)lang);
	/* We only want to know the filename. */
	FcObjectSet *os = FcObjectSetBuild(FC_FILE, FC_SPACING, FC_SLANT, FC_WEIGHT, NULL);
	/* Get the list of filenames matching the wanted language. */
	FcFontSet *fs = FcFontList(NULL, pat, os);

	/* We don't need these anymore. */
	FcObjectSetDestroy(os);
	FcPatternDestroy(pat);

	if (fs != NULL) {
		int best_weight = -1;
		const char *best_font = NULL;

		for (int i = 0; i < fs->nfont; i++) {
			FcPattern *font = fs->fonts[i];

			FcChar8 *file = NULL;
			FcResult res = FcPatternGetString(font, FC_FILE, 0, &file);
			if (res != FcResultMatch || file == NULL) {
				continue;
			}

			/* Get a font with the right spacing .*/
			int value = 0;
			FcPatternGetInteger(font, FC_SPACING, 0, &value);
			if (callback->Monospace() != (value == FC_MONO) && value != FC_DUAL) continue;

			/* Do not use those that explicitly say they're slanted. */
			FcPatternGetInteger(font, FC_SLANT, 0, &value);
			if (value != 0) continue;

			/* We want the fatter font as they look better at small sizes. */
			FcPatternGetInteger(font, FC_WEIGHT, 0, &value);
			if (value <= best_weight) continue;

			SetFontNames (settings, (const char*)file, callback);

			bool missing = callback->FindMissingGlyphs();
			DEBUG(freetype, 1, "Font \"%s\" misses%s glyphs", file, missing ? "" : " no");

			if (!missing) {
				best_weight = value;
				best_font   = (const char *)file;
			}
		}

		if (best_font != NULL) {
			ret = true;
			SetFontNames (settings, best_font, callback);
			InitFreeType(callback->Monospace());
		}

		/* Clean up the list of filenames. */
		FcFontSetDestroy(fs);
	}

	FcFini();
	return ret;
}

#else /* without WITH_FONTCONFIG */

static inline FT_Error GetFontByFaceName (const char *font_name, FT_Face *face)
{
	return FT_Err_Cannot_Open_Resource;
}

bool SetFallbackFont (FreeTypeSettings *settings, const char *language_isocode, int winlangid, MissingGlyphSearcher *callback)
{
	return false;
}

#endif /* WITH_FONTCONFIG */

/**
 * Loads the freetype font.
 * First type to load the fontname as if it were a path. If that fails,
 * try to resolve the filename of the font using fontconfig, where the
 * format is 'font family name' or 'font family name, font style'.
 */
void FontCache::LoadFreeTypeFont (void)
{
	this->UnloadFreeTypeFont();

	assert (this->face == NULL);
	assert (this->font_tables.Length() == 0);

	FontSize fs = this->fs;
	FreeTypeSubSetting *settings = GetFreeTypeSettings (fs);

	if (StrEmpty(settings->font)) return;

	if (_library == NULL) {
		if (FT_Init_FreeType(&_library) != FT_Err_Ok) {
			ShowInfoF("Unable to initialize FreeType, using sprite fonts instead");
			return;
		}

		DEBUG(freetype, 2, "Initialized");
	}

	FT_Face face = NULL;
	FT_Error err = FT_New_Face (_library, settings->font, 0, &face);

	if (err != FT_Err_Ok) err = GetFontByFaceName (settings->font, &face);

	if (err == FT_Err_Ok) {
		DEBUG(freetype, 2, "Requested '%s', using '%s %s'", settings->font, face->family_name, face->style_name);

		/* Attempt to select the unicode character map */
		err = FT_Select_Charmap (face, ft_encoding_unicode);
		if (err == FT_Err_Ok) goto found_face; // Success

		if (err == FT_Err_Invalid_CharMap_Handle) {
			/* Try to pick a different character map instead. We default to
			 * the first map, but platform_id 0 encoding_id 0 should also
			 * be unicode (strange system...) */
			FT_CharMap found = face->charmaps[0];
			int i;

			for (i = 0; i < face->num_charmaps; i++) {
				FT_CharMap charmap = face->charmaps[i];
				if (charmap->platform_id == 0 && charmap->encoding_id == 0) {
					found = charmap;
				}
			}

			if (found != NULL) {
				err = FT_Set_Charmap (face, found);
				if (err == FT_Err_Ok) goto found_face;
			}
		}
	}

	FT_Done_Face(face);

	static const char *SIZE_TO_NAME[] = { "medium", "small", "large", "mono" };
	ShowInfoF ("Unable to use '%s' for %s font, FreeType reported error 0x%X, using sprite font instead", settings->font, SIZE_TO_NAME[fs], err);
	return;

found_face:
	assert(face != NULL);
	this->face = face;

	int pixels = settings->size;
	if (pixels == 0) {
		/* Try to determine a good height based on the minimal height recommended by the font. */
		pixels = _default_font_height[fs];

		TT_Header *head = (TT_Header *)FT_Get_Sfnt_Table (face, ft_sfnt_head);
		if (head != NULL) {
			/* Font height is minimum height plus the difference between the default
			 * height for this font size and the small size. */
			int diff = _default_font_height[fs] - _default_font_height[FS_SMALL];
			pixels = Clamp (min (head->Lowest_Rec_PPEM, 20) + diff, _default_font_height[fs], MAX_FONT_SIZE);
		}
	}

	err = FT_Set_Pixel_Sizes (face, 0, pixels);
	if (err != FT_Err_Ok) {

		/* Find nearest size to that requested */
		FT_Bitmap_Size *bs = face->available_sizes;
		int i = face->num_fixed_sizes;
		if (i > 0) { // In pathetic cases one might get no fixed sizes at all.
			int n = bs->height;
			FT_Int chosen = 0;
			for (; --i; bs++) {
				if (abs (pixels - bs->height) >= abs (pixels - n)) continue;
				n = bs->height;
				chosen = face->num_fixed_sizes - i;
			}

			/* Don't use FT_Set_Pixel_Sizes here - it might give us another
			 * error, even though the size is available (FS#5885). */
			err = FT_Select_Size (face, chosen);
		}
	}

	if (err == FT_Err_Ok) {
		this->units_per_em = this->face->units_per_EM;
		this->ascender     = this->face->size->metrics.ascender >> 6;
		this->descender    = this->face->size->metrics.descender >> 6;
		this->height       = this->ascender - this->descender;
	} else {
		/* Both FT_Set_Pixel_Sizes and FT_Select_Size failed. */
		DEBUG(freetype, 0, "Font size selection failed. Using FontCache defaults.");
	}
}

/** Unload the freetype font. */
void FontCache::UnloadFreeTypeFont (void)
{
	if (this->face == NULL) return;

	this->ClearFontCache();

	for (FontTable::iterator iter = this->font_tables.Begin(); iter != this->font_tables.End(); iter++) {
		free(iter->second.second);
	}

	this->font_tables.Clear();

	FT_Done_Face(this->face);
	this->face = NULL;

	this->ResetFontMetrics();
}

#endif /* WITH_FREETYPE */


/** Reset cached glyphs. */
void FontCache::ClearFontCache (void)
{
#ifdef WITH_FREETYPE
	if (this->face == NULL) {
		this->ResetFontMetrics();
	} else {
		this->missing_sprite = NULL;
		for (int i = 0; i < 256; i++) {
			if (this->sprite_map[i] == NULL) continue;

			for (int j = 0; j < 256; j++) {
				free (this->sprite_map[i][j].sprite);
			}

			free (this->sprite_map[i]);
			this->sprite_map[i] = NULL;
		}
	}
#else  /* WITH_FREETYPE */
	this->ResetFontMetrics();
#endif /* WITH_FREETYPE */

	Layouter::ResetFontCache(this->fs);
}


#ifdef WITH_FREETYPE

static void *AllocateFont(size_t size)
{
	return xmalloc (size);
}

/* Check if a glyph should be rendered with antialiasing */
static bool GetFontAAState(FontSize size)
{
	/* AA is only supported for 32 bpp */
	if (Blitter::get()->screen_depth != 32) return false;
	return GetFreeTypeSettings(size)->aa;
}

static Sprite *MakeBuiltinQuestionMark (void)
{
	/* The font misses the '?' character. Use built-in sprite.
	 * Note: We cannot use the baseset as this also has to work
	 * in the bootstrap GUI. */
#define CPSET { 0, 0, 0, 0, 1 }
#define CP___ { 0, 0, 0, 0, 0 }
	static Blitter::RawSprite::Pixel builtin_questionmark_data[10 * 8] = {
		CP___, CP___, CPSET, CPSET, CPSET, CPSET, CP___, CP___,
		CP___, CPSET, CPSET, CP___, CP___, CPSET, CPSET, CP___,
		CP___, CP___, CP___, CP___, CP___, CPSET, CPSET, CP___,
		CP___, CP___, CP___, CP___, CPSET, CPSET, CP___, CP___,
		CP___, CP___, CP___, CPSET, CPSET, CP___, CP___, CP___,
		CP___, CP___, CP___, CPSET, CPSET, CP___, CP___, CP___,
		CP___, CP___, CP___, CPSET, CPSET, CP___, CP___, CP___,
		CP___, CP___, CP___, CP___, CP___, CP___, CP___, CP___,
		CP___, CP___, CP___, CPSET, CPSET, CP___, CP___, CP___,
		CP___, CP___, CP___, CPSET, CPSET, CP___, CP___, CP___,
	};
#undef CPSET
#undef CP___
	static const Blitter::RawSprite builtin_questionmark = {
		builtin_questionmark_data,
		10, // height
		8,  // width
		0,  // x_offs
		0,  // y_offs
	};

	Sprite *spr = Blitter::get()->encode (&builtin_questionmark, true, AllocateFont);
	assert (spr != NULL);
	return spr;
}

/** Copy the pixels from a glyph rendered by FreeType into a RawSprite. */
static inline void CopyGlyphPixels (Blitter::RawSprite::Pixel *data,
	uint width, FT_GlyphSlot slot, byte colour, bool aa = false)
{
	for (uint y = 0; y < (unsigned int)slot->bitmap.rows; y++) {
		for (uint x = 0; x < (unsigned int)slot->bitmap.width; x++) {
			byte a = aa ? slot->bitmap.buffer[x + y * slot->bitmap.pitch] :
					HasBit(slot->bitmap.buffer[(x / 8) + y * slot->bitmap.pitch], 7 - (x % 8)) ? 0xFF : 0;
			if (a > 0) {
				data[x + y * width].m = colour;
				data[x + y * width].a = a;
			}
		}
	}
}

const FontCache::GlyphEntry *FontCache::GetGlyphPtr (GlyphID key)
{
	if (key == 0) {
		const GlyphEntry *glyph = this->missing_sprite;
		if (glyph != NULL) return glyph;

		GlyphID question_glyph = this->MapCharToGlyph ('?');
		if (question_glyph != 0) {
			/* Use '?' for missing characters. */
			glyph = this->GetGlyphPtr (question_glyph);
			this->missing_sprite = glyph;
			return glyph;
		}

		/* The font misses the '?' character. We handle this below. */
	}

	/* Check for the glyph in our cache */
	GlyphEntry *p = this->sprite_map[GB(key, 8, 8)];
	if (p != NULL) {
		p += GB(key, 0, 8);
		if (p->sprite != NULL) return p;
	} else {
		DEBUG(freetype, 3, "Allocating glyph cache for range 0x%02X00, size %u", GB(key, 8, 8), this->fs);
		p = xcalloct<GlyphEntry>(256);
		this->sprite_map[GB(key, 8, 8)] = p;
		p += GB(key, 0, 8);
	}

	DEBUG(freetype, 4, "Set glyph for unicode character 0x%04X, size %u", key, this->fs);

	if (key == 0) {
		/* The font misses the '?' character. Use built-in sprite. */
		this->missing_sprite = p;
		Sprite *spr = MakeBuiltinQuestionMark();
		p->sprite = spr;
		p->width  = spr->width + (this->fs != FS_NORMAL);
		return p;
	}

	FT_GlyphSlot slot = this->face->glyph;

	bool aa = GetFontAAState(this->fs);

	FT_Load_Glyph(this->face, key, FT_LOAD_DEFAULT);
	FT_Render_Glyph(this->face->glyph, aa ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);

	/* Despite requesting a normal glyph, FreeType may have returned a bitmap */
	aa = (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

	/* Add 1 pixel for the shadow on the medium font. Our sprite must be at least 1x1 pixel */
	unsigned int width  = max(1U, (unsigned int)slot->bitmap.width + (this->fs == FS_NORMAL));
	unsigned int height = max(1U, (unsigned int)slot->bitmap.rows  + (this->fs == FS_NORMAL));

	/* Limit glyph size to prevent overflows later on. */
	if (width > 256 || height > 256) usererror("Font glyph is too large");

	/* FreeType has rendered the glyph, now we allocate a sprite and copy the image into it.
	 * Use a static buffer to prevent repeated allocation/deallocation. */
	static ReusableBuffer <Blitter::RawSprite::Pixel> buffer;
	Blitter::RawSprite sprite;
	sprite.data = buffer.ZeroAllocate (width * height);
	sprite.width = width;
	sprite.height = height;
	sprite.x_offs = slot->bitmap_left;
	sprite.y_offs = this->ascender - slot->bitmap_top;

	/* Draw shadow for medium size */
	if (this->fs == FS_NORMAL && !aa) {
		CopyGlyphPixels (sprite.data + width + 1, width, slot, SHADOW_COLOUR);
	}

	CopyGlyphPixels (sprite.data, width, slot, FACE_COLOUR, aa);

	p->sprite = Blitter::get()->encode (&sprite, true, AllocateFont);
	p->width  = slot->advance.x >> 6;
	return p;
}

#endif /* WITH_FREETYPE */

const Sprite *FontCache::GetGlyph (GlyphID key)
{
#ifdef WITH_FREETYPE
	if ((this->face != NULL) && ((key & SPRITE_GLYPH) == 0)) {
		return this->GetGlyphPtr(key)->sprite;
	}
#endif /* WITH_FREETYPE */

	return GetSprite (this->GetGlyphSprite (key), ST_FONT);
}

bool FontCache::GetDrawGlyphShadow (void) const
{
#ifdef WITH_FREETYPE
	return this->fs == FS_NORMAL && GetFontAAState (FS_NORMAL);
#else  /* WITH_FREETYPE */
	return false;
#endif /* WITH_FREETYPE */
}

uint FontCache::GetGlyphWidth (GlyphID key)
{
#ifdef WITH_FREETYPE
	if ((this->face != NULL) && ((key & SPRITE_GLYPH) == 0)) {
		return this->GetGlyphPtr(key)->width;
	}
#endif /* WITH_FREETYPE */

	SpriteID sprite = this->GetGlyphSprite (key);
	return SpriteExists(sprite) ? GetSprite(sprite, ST_FONT)->width + ScaleGUITrad(this->fs != FS_NORMAL ? 1 : 0) : 0;
}

GlyphID FontCache::MapCharToGlyph (WChar key) const
{
	assert(IsPrintable(key));

#ifdef WITH_FREETYPE
	if ((this->face != NULL)
			&& (key < SCC_SPRITE_START || key > SCC_SPRITE_END)) {
		return FT_Get_Char_Index (this->face, key);
	}
#endif /* WITH_FREETYPE */

	return SPRITE_GLYPH | key;
}

const void *FontCache::GetFontTable (uint32 tag, size_t &length)
{
#ifdef WITH_FREETYPE
	if (this->face != NULL) {
		const FontTable::iterator iter = this->font_tables.Find(tag);
		if (iter != this->font_tables.End()) {
			length = iter->second.first;
			return iter->second.second;
		}

		FT_ULong len = 0;
		FT_Byte *result = NULL;

		FT_Load_Sfnt_Table (this->face, tag, 0, NULL, &len);

		if (len > 0) {
			result = xmalloct<FT_Byte>(len);
			FT_Load_Sfnt_Table (this->face, tag, 0, result, &len);
		}
		length = len;

		this->font_tables.Insert (tag, SmallPair<size_t, const void *>(length, result));
		return result;
	}
#endif /* WITH_FREETYPE */

	length = 0;
	return NULL;
}

const char *FontCache::GetFontName (void) const
{
#ifdef WITH_FREETYPE
	if (this->face != NULL) return this->face->family_name;
#endif /* WITH_FREETYPE */

	return "sprite";
}

/** Compute the broadest n-digit value in this font. */
uint64 FontCache::GetBroadestValue (uint n) const
{
	uint d = this->widest_digit;

	if (n <= 1) return d;

	uint64 val = this->widest_digit_nonnull;
	do {
		val = 10 * val + d;
	} while (--n > 1);
	return val;
}


FontCache FontCache::cache [FS_END] = {
	FontCache (FS_NORMAL),
	FontCache (FS_SMALL),
	FontCache (FS_LARGE),
	FontCache (FS_MONO),
};


#ifdef WITH_FREETYPE

/**
 * (Re)initialize the freetype related things, i.e. load the non-sprite fonts.
 * @param monospace Whether to initialise the monospace or regular fonts.
 */
void InitFreeType(bool monospace)
{
	for (FontSize fs = FS_BEGIN; fs < FS_END; fs++) {
		if (monospace != (fs == FS_MONO)) continue;
		FontCache::Get(fs)->LoadFreeTypeFont();
	}
}

/**
 * Free everything allocated w.r.t. fonts.
 */
void UninitFreeType()
{
	for (FontSize fs = FS_BEGIN; fs < FS_END; fs++) {
		FontCache::Get(fs)->UnloadFreeTypeFont();
	}

	FT_Done_FreeType(_library);
	_library = NULL;
}

#endif /* WITH_FREETYPE */
