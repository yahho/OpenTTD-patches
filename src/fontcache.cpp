/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fontcache.cpp Cache for characters from fonts. */

#include "stdafx.h"
#include "debug.h"
#include "fontcache.h"
#include "fontdetection.h"
#include "blitter/blitter.h"
#include "core/math_func.hpp"
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

FT_Library _library = NULL;

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
	font_tables(), face (NULL),
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
		for (int i = 0; i < 256; i++) {
			if (this->sprite_map[i] == NULL) continue;

			for (int j = 0; j < 256; j++) {
				if (this->sprite_map[i][j].duplicate) continue;
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

FontCache::GlyphEntry *FontCache::SetGlyphPtr (GlyphID key,
	Sprite *sprite, byte width, bool duplicate)
{
	if (this->sprite_map[GB(key, 8, 8)] == NULL) {
		DEBUG(freetype, 3, "Allocating glyph cache for range 0x%02X00, size %u", GB(key, 8, 8), this->fs);
		this->sprite_map[GB(key, 8, 8)] = xcalloct<GlyphEntry>(256);
	}

	DEBUG(freetype, 4, "Set glyph for unicode character 0x%04X, size %u", key, this->fs);
	GlyphEntry *p = &this->sprite_map[GB(key, 8, 8)][GB(key, 0, 8)];
	p->sprite    = sprite;
	p->width     = width;
	p->duplicate = duplicate;
	return p;
}

static void *AllocateFont(size_t size)
{
	return xmalloc (size);
}

/* Check if a glyph should be rendered with antialiasing */
static bool GetFontAAState(FontSize size)
{
	/* AA is only supported for 32 bpp */
	if (Blitter::get()->GetScreenDepth() != 32) return false;
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

	Sprite *spr = Blitter::get()->Encode (&builtin_questionmark, true, AllocateFont);
	assert (spr != NULL);
	return spr;
}

FontCache::GlyphEntry *FontCache::GetGlyphPtr (GlyphID key)
{
	/* Check for the glyph in our cache */
	GlyphEntry *p = this->sprite_map[GB(key, 8, 8)];
	if (p != NULL) {
		GlyphEntry *glyph = &p[GB(key, 0, 8)];
		if (glyph->sprite != NULL) return glyph;
	}

	FT_GlyphSlot slot = this->face->glyph;

	bool aa = GetFontAAState(this->fs);

	if (key == 0) {
		GlyphID question_glyph = this->MapCharToGlyph('?');
		if (question_glyph == 0) {
			/* The font misses the '?' character. Use built-in sprite. */
			Sprite *spr = MakeBuiltinQuestionMark();
			return this->SetGlyphPtr (key, spr, spr->width + (this->fs != FS_NORMAL), false);
		} else {
			/* Use '?' for missing characters. */
			GlyphEntry *glyph = this->GetGlyphPtr (question_glyph);
			return this->SetGlyphPtr (key, glyph->sprite, glyph->width, true);
		}
	}
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
		for (unsigned int y = 0; y < (unsigned int)slot->bitmap.rows; y++) {
			for (unsigned int x = 0; x < (unsigned int)slot->bitmap.width; x++) {
				if (aa ? (slot->bitmap.buffer[x + y * slot->bitmap.pitch] > 0) : HasBit(slot->bitmap.buffer[(x / 8) + y * slot->bitmap.pitch], 7 - (x % 8))) {
					sprite.data[1 + x + (1 + y) * sprite.width].m = SHADOW_COLOUR;
					sprite.data[1 + x + (1 + y) * sprite.width].a = aa ? slot->bitmap.buffer[x + y * slot->bitmap.pitch] : 0xFF;
				}
			}
		}
	}

	for (unsigned int y = 0; y < (unsigned int)slot->bitmap.rows; y++) {
		for (unsigned int x = 0; x < (unsigned int)slot->bitmap.width; x++) {
			if (aa ? (slot->bitmap.buffer[x + y * slot->bitmap.pitch] > 0) : HasBit(slot->bitmap.buffer[(x / 8) + y * slot->bitmap.pitch], 7 - (x % 8))) {
				sprite.data[x + y * sprite.width].m = FACE_COLOUR;
				sprite.data[x + y * sprite.width].a = aa ? slot->bitmap.buffer[x + y * slot->bitmap.pitch] : 0xFF;
			}
		}
	}

	Sprite *spr = Blitter::get()->Encode (&sprite, true, AllocateFont);

	return this->SetGlyphPtr (key, spr, slot->advance.x >> 6);
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
