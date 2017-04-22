/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file font.h Functions to read fonts from files and cache them. */

#ifndef FONT_H
#define FONT_H

#ifdef WITH_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H
#endif /* WITH_FREETYPE */

#include "core/smallmap_type.hpp"
#include "string.h"
#include "spritecache.h"

/** Glyphs are characters from a font. */
typedef uint32 GlyphID;
static const GlyphID SPRITE_GLYPH = 1U << 30;

/** Font cache for basic fonts. */
class FontCache {
private:
	static FontCache cache [FS_END]; ///< All the font caches.

	SpriteID *spriteid_map[0x100]; ///< Mapping of glyphs to sprite IDs.

	byte glyph_widths [256 - 32]; ///< Glyph widths of all ASCII characters.

#ifdef WITH_FREETYPE

	/** Container for information about a FreeType glyph. */
	struct GlyphEntry {
		Sprite *sprite; ///< The loaded sprite.
		byte width;     ///< The width of the glyph.
	};

	/**
	 * The glyph cache. This is structured to reduce memory consumption.
	 * 1) There is a 'segment' table for each font size.
	 * 2) Each segment table is a discrete block of characters.
	 * 3) Each block contains 256 (aligned) characters sequential characters.
	 *
	 * The cache is accessed in the following way:
	 * For character 0x0041  ('A'): sprite_map[0x00][0x41]
	 * For character 0x20AC (Euro): sprite_map[0x20][0xAC]
	 *
	 * Currently only 256 segments are allocated, "limiting" us to 65536 characters.
	 * This can be simply changed in the function GetGlyphPtr.
	 */
	GlyphEntry *sprite_map[0x100];

	const GlyphEntry *missing_sprite; ///< Sprite for missing glyphs (question mark)

	typedef SmallMap<uint32, SmallPair<size_t, const void*> > FontTable; ///< Table with font table cache

	FontTable font_tables; ///< Cached font tables.

	FT_Face face;  ///< The font face associated with this font.

#endif /* WITH_FREETYPE */

	const FontSize fs;                ///< The size of the font.
	int height;                       ///< The height of the font.
	int ascender;                     ///< The ascender value of the font.
	int descender;                    ///< The descender value of the font.
	int units_per_em;                 ///< The units per EM value of the font.

	byte widest_digit;          ///< Widest digit.
	byte widest_digit_nonnull;  ///< Widest leading (non-null) digit.
	byte digit_width;           ///< Width of the widest digit.

	void ResetFontMetrics (void);

	void ClearGlyphToSpriteMap (void);

	SpriteID GetGlyphSprite (GlyphID key) const;

#ifdef WITH_FREETYPE

	const GlyphEntry *GetGlyphPtr (GlyphID key);

#endif /* WITH_FREETYPE */

	FontCache(FontSize fs);
	~FontCache();

public:
	/**
	 * Get the height of the font.
	 * @return The height of the font.
	 */
	inline int GetHeight() const { return this->height; }

	/**
	 * Get the ascender value of the font.
	 * @return The ascender value of the font.
	 */
	inline int GetAscender() const { return this->ascender; }

	/**
	 * Get the descender value of the font.
	 * @return The descender value of the font.
	 */
	inline int GetDescender() const{ return this->descender; }

	/**
	 * Get the units per EM value of the font.
	 * @return The units per EM value of the font.
	 */
	inline int GetUnitsPerEM() const { return this->units_per_em; }

	/**
	 * Get the SpriteID mapped to the given key
	 * @param key The key to get the sprite for.
	 * @return The sprite.
	 */
	SpriteID GetUnicodeGlyph (WChar key) const;

	/**
	 * Map a SpriteID to the key
	 * @param key The key to map to.
	 * @param sprite The sprite that is being mapped.
	 */
	void SetUnicodeGlyph (WChar key, SpriteID sprite);

	/** Initialize the glyph map */
	void InitializeUnicodeGlyphMap (void);

#ifdef WITH_FREETYPE

	/* Load the freetype font. */
	void LoadFreeTypeFont (void);

	/* Unload the freetype font. */
	void UnloadFreeTypeFont (void);

#endif /* WITH_FREETYPE */

	/** Clear the font cache. */
	void ClearFontCache (void);

	/**
	 * Get the glyph (sprite) of the given key.
	 * @param key The key to look up.
	 * @return The sprite.
	 */
	const Sprite *GetGlyph (GlyphID key);

	/**
	 * Get the width of the glyph with the given key.
	 * @param key The key to look up.
	 * @return The width.
	 */
	uint GetGlyphWidth (GlyphID key);

	/**
	 * Do we need to draw a glyph shadow?
	 * @return True if it has to be done, otherwise false.
	 */
	bool GetDrawGlyphShadow (void) const;

	/**
	 * Map a character into a glyph.
	 * @param key The character.
	 * @return The glyph ID used to draw the character.
	 */
	GlyphID MapCharToGlyph (WChar key) const;

	/**
	 * Get the glyph (sprite) for a given character.
	 * @param c The character to look up.
	 * @return The sprite.
	 */
	const Sprite *GetCharGlyph (WChar c)
	{
		return this->GetGlyph (this->MapCharToGlyph (c));
	}

	/**
	 * Read a font table from the font.
	 * @param tag The of the table to load.
	 * @param length The length of the read data.
	 * @return The loaded table data.
	 */
	const void *GetFontTable (uint32 tag, size_t &length);

	/**
	 * Get the name of this font.
	 * @return The name of the font.
	 */
	const char *GetFontName (void) const;

	/**
	 * Return width of character glyph.
	 * @param size  Font of the character
	 * @param key   Character code glyph
	 * @return Width of the character glyph
	 */
	byte GetCharacterWidth (WChar key)
	{
		/* Use cache if possible */
		if (key >= 32 && key < 256) return this->glyph_widths [key - 32];

		return this->GetGlyphWidth (this->MapCharToGlyph (key));
	}

	/**
	 * Return the maximum width of single digit.
	 * @param size  Font of the digit
	 * @return Width of the digit.
	 */
	byte GetDigitWidth (void) const
	{
		return this->digit_width;
	}

	/* Compute the broadest n-digit value in a given font size. */
	uint64 GetBroadestValue (uint n) const;

	/**
	 * Get the font cache of a given font size.
	 * @param fs The font size to look up.
	 * @return The font cache.
	 */
	static inline FontCache *Get(FontSize fs)
	{
		assert(fs < FS_END);
		return &FontCache::cache[fs];
	}
};

/** Initialize the glyph map */
static inline void InitializeUnicodeGlyphMap()
{
	for (FontSize fs = FS_BEGIN; fs < FS_END; fs++) {
		FontCache::Get(fs)->InitializeUnicodeGlyphMap();
	}
}

static inline void ClearFontCache()
{
	for (FontSize fs = FS_BEGIN; fs < FS_END; fs++) {
		FontCache::Get(fs)->ClearFontCache();
	}
}

/**
 * Return the maximum width of single digit.
 * @param size  Font of the digit
 * @return Width of the digit.
 */
static inline byte GetDigitWidth (FontSize size = FS_NORMAL)
{
	return FontCache::Get(size)->GetDigitWidth();
}

#ifdef WITH_FREETYPE

/** Settings for a single freetype font. */
struct FreeTypeSubSetting {
	char font[MAX_PATH]; ///< The name of the font, or path to the font.
	uint size;           ///< The (requested) size of the font.
	bool aa;             ///< Whether to do anti aliasing or not.
};

/** Settings for the freetype fonts. */
struct FreeTypeSettings {
	FreeTypeSubSetting small;  ///< The smallest font; mostly used for zoomed out view.
	FreeTypeSubSetting medium; ///< The normal font size.
	FreeTypeSubSetting large;  ///< The largest font; mostly used for newspapers.
	FreeTypeSubSetting mono;   ///< The mono space font used for license/readme viewers.
};

extern FreeTypeSettings _freetype;

void InitFreeType(bool monospace);
void UninitFreeType();

/**
 * We would like to have a fallback font as the current one
 * doesn't contain all characters we need.
 * This function must set all fonts of settings.
 * @param settings the settings to overwrite the fontname of.
 * @param language_isocode the language, e.g. en_GB.
 * @param winlangid the language ID windows style.
 * @param callback The function to call to check for missing glyphs.
 * @return true if a font has been set, false otherwise.
 */
bool SetFallbackFont (FreeTypeSettings *settings, const char *language_isocode, int winlangid, class MissingGlyphSearcher *callback);

#else /* WITH_FREETYPE */

static inline void InitFreeType (bool monospace)
{
}

static inline void UninitFreeType (void)
{
}

#endif /* WITH_FREETYPE */

#endif /* FONT_H */
