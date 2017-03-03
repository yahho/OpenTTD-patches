/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gfx_layout.cpp Handling of laying out text. */

#include "stdafx.h"

#include <vector>
#include <map>
#include <string>

#include "gfx_layout.h"
#include "string.h"
#include "strings_func.h"
#include "debug.h"
#include "core/pointer.h"
#include "core/flexarray.h"
#include "core/smallmap_type.hpp"
#include "gfx_func.h"

#include "table/control_codes.h"

#ifdef WITH_ICU_LAYOUT
#include <unicode/ustring.h>
#include "layout/ParagraphLayout.h"
#endif /* WITH_ICU_LAYOUT */


#ifdef WITH_ICU_LAYOUT

/** Container with information about a font. */
class ICUFont :
	public LEFontInstance,
	public FontBase {
public:
	ICUFont (FontSize size, TextColour colour) : FontBase (size, colour)
	{
	}

	/* Implementation details of LEFontInstance */

	le_int32 getUnitsPerEM() const OVERRIDE
	{
		return this->fc->GetUnitsPerEM();
	}

	le_int32 getAscent() const OVERRIDE
	{
		return this->fc->GetAscender();
	}

	le_int32 getDescent() const OVERRIDE
	{
		return -this->fc->GetDescender();
	}

	le_int32 getLeading() const OVERRIDE
	{
		return this->fc->GetHeight();
	}

	float getXPixelsPerEm() const OVERRIDE
	{
		return (float)this->fc->GetHeight();
	}

	float getYPixelsPerEm() const OVERRIDE
	{
		return (float)this->fc->GetHeight();
	}

	float getScaleFactorX() const OVERRIDE
	{
		return 1.0f;
	}

	float getScaleFactorY() const OVERRIDE
	{
		return 1.0f;
	}

	const void *getFontTable (LETag tableTag) const
	{
		size_t length;
		return this->getFontTable (tableTag, length);
	}

	const void *getFontTable (LETag tableTag, size_t &length) const OVERRIDE
	{
		return this->fc->GetFontTable(tableTag, length);
	}

	LEGlyphID mapCharToGlyph (LEUnicode32 ch) const OVERRIDE
	{
		if (IsTextDirectionChar (ch)) return 0;
		return this->fc->MapCharToGlyph (ch);
	}

	void getGlyphAdvance (LEGlyphID glyph, LEPoint &advance) const OVERRIDE
	{
		advance.fX = glyph == 0xFFFF ? 0 : this->fc->GetGlyphWidth (glyph);
		advance.fY = 0;
	}

	le_bool getGlyphPoint (LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const OVERRIDE
	{
		return FALSE;
	}
};

typedef ICUFont Font;

#else /* !WITH_ICU_LAYOUT */

typedef FontBase Font;

#endif /* WITH_ICU_LAYOUT */


/** Mapping from index to font. */
typedef std::vector <std::pair <int, Font *> > FontMap;


typedef SmallMap<TextColour, Font *> FontColourMap;

/** Cache of Font instances. */
static FontColourMap fonts[FS_END];

/** Get a static font instance. */
static Font *GetFont (FontSize size, TextColour colour)
{
	FontColourMap::iterator it = fonts[size].Find(colour);
	if (it != fonts[size].End()) return it->second;

	Font *f = new Font(size, colour);
	*fonts[size].Append() = FontColourMap::Pair (colour, f);
	return f;
}


/**
 * Construct a new font.
 * @param size   The font size to use for this font.
 * @param colour The colour to draw this font in.
 */
FontBase::FontBase (FontSize size, TextColour colour) :
		fc(FontCache::Get(size)), colour(colour)
{
	assert(size < FS_END);
}


/** Interface to glue fallback and normal layouter into one. */
class ParagraphBuilder {
public:
	typedef std::vector <ttd_unique_ptr <const ParagraphLayouter::Line> > LineVector;

	virtual ~ParagraphBuilder() {}

	virtual void build (LineVector *v, int maxw, bool reflow) = 0;
};

/**
 * Get the position of a character in a layout.
 * @tparam Line Type of the layout line.
 * @param line Layout line.
 * @param str Layout string.
 * @param ch Character to get the position of.
 * @return Left position of the character relative to the start of the string.
 * @note Will only work right for single-line strings.
 */
template <typename Line>
static int GetCharPosition (const Line *line, const char *str, const char *ch)
{
	/* Find the code point index which corresponds to the char
	 * pointer into our UTF-8 source string. */
	size_t index = 0;
	while (str < ch) {
		WChar c;
		size_t len = Utf8Decode (&c, str);
		if (c == '\0' || c == '\n') return 0;
		str += len;
		index += line->GetInternalCharLength (c);
	}

	/* Valid character. */

	/* Pointer to the end-of-string/line marker? Return total line width. */
	if (*ch == '\0' || *ch == '\n') {
		return line->GetWidth();
	}

	/* Scan all runs until we've found our code point index. */
	for (int run_index = 0; run_index < line->CountRuns(); run_index++) {
		const typename Line::VisualRun *run = line->GetVisualRun (run_index);

		for (int i = 0; i < run->GetGlyphCount(); i++) {
			/* Matching glyph? Return position. */
			if ((size_t)run->GetGlyphToChar(i) == index) {
				return run->GetPositions()[i * 2];
			}
		}
	}

	return 0;
}

/**
 * Get the character that is at a position in a layout.
 * @tparam Line Type of the layout line.
 * @param line Layout line.
 * @param str Layout string.
 * @param x Position in the string.
 * @return Pointer to the character at the position or NULL if no character is at the position.
 */
template <typename Line>
static const char *GetCharAtPosition (const Line *line, const char *str, int x)
{
	for (int run_index = 0; run_index < line->CountRuns(); run_index++) {
		const typename Line::VisualRun *run = line->GetVisualRun (run_index);

		for (int i = 0; i < run->GetGlyphCount(); i++) {
			/* Not a valid glyph (empty). */
			if (run->GetGlyphs()[i] == 0xFFFF) continue;

			int begin_x = (int)run->GetPositions()[i * 2];
			int end_x   = (int)run->GetPositions()[i * 2 + 2];

			if (IsInsideMM(x, begin_x, end_x)) {
				/* Found our glyph, now convert to UTF-8 string index. */
				size_t index = run->GetGlyphToChar(i);

				while (*str != '\0') {
					if (index == 0) return str;

					WChar c = Utf8Consume(&str);
					index -= line->GetInternalCharLength (c);
				}
			}
		}
	}

	return NULL;
}


#ifdef WITH_ICU_LAYOUT
/** Visual run contains data about the bit of text with the same font. */
class ICUVisualRun : public ParagraphLayouter::VisualRun {
	const ParagraphLayout::VisualRun *vr; ///< The actual ICU vr.

public:
	ICUVisualRun (const ParagraphLayout::VisualRun *vr) : vr(vr)
	{
	}

	const FontBase *GetFont (void) const OVERRIDE
	{
		return (const Font*)vr->getFont();
	}

	int GetGlyphCount (void) const OVERRIDE
	{
		return vr->getGlyphCount();
	}

	const GlyphID *GetGlyphs (void) const OVERRIDE
	{
		return vr->getGlyphs();
	}

	const float *GetPositions (void) const OVERRIDE
	{
		return vr->getPositions();
	}

	/**
	 * Get the character index for a glyph index for this visual run.
	 * @param i The glyph index.
	 * @return The character index.
	 */
	int GetGlyphToChar (int i) const
	{
		return vr->getGlyphToCharMap()[i];
	}

	bool GetGlyphPos (ParagraphLayouter::GlyphPos *gp, int i) const OVERRIDE;
};

/**
 * Get the glyph and position for a glyph.
 * @param gp Struct to receive the data.
 * @param i Index of the glyph whose data to get.
 * @return Whether the glyph is valid (non-empty).
 */
bool ICUVisualRun::GetGlyphPos (ParagraphLayouter::GlyphPos *gp, int i) const
{
	GlyphID glyph = this->vr->getGlyphs()[i];
	if (glyph == 0xFFFF) return false;

	gp->glyph = glyph;
	const float *pos = this->GetPositions();
	gp->x0 = pos[i * 2];
	gp->x1 = pos[i * 2 + 2];
	gp->y  = pos[i * 2 + 1];
	return true;
}

/** A single line worth of VisualRuns. */
class ICULine : public ParagraphLayouter::Line {
	ttd_unique_ptr <ParagraphLayout::Line> l; ///< The actual ICU line.
	std::vector <ttd_unique_ptr <ICUVisualRun> > runs;

public:
	typedef ICUVisualRun VisualRun;

	ICULine (ParagraphLayout::Line *l) : l (l), runs (l->countRuns())
	{
		for (int i = 0; i < l->countRuns(); i++) {
			this->runs[i].reset (new ICUVisualRun (l->getVisualRun (i)));
		}
	}

	int GetLeading (void) const OVERRIDE
	{
		return l->getLeading();
	}

	int GetWidth (void) const OVERRIDE
	{
		return l->getWidth();
	}

	int CountRuns (void) const OVERRIDE
	{
		return l->countRuns();
	}

	const ICUVisualRun *GetVisualRun (int run) const OVERRIDE
	{
		return this->runs[run].get();
	}

	static int GetInternalCharLength (WChar c)
	{
		/* ICU uses UTF-16 internally which means we need to account for surrogate pairs. */
		return Utf8CharLen(c) < 4 ? 1 : 2;
	}

	/**
	 * Get the position of a character in the layout.
	 * @param str Layout string.
	 * @param ch Character to get the position of.
	 * @return Left position of the character relative to the start of the string.
	 * @note Will only work right for single-line strings.
	 */
	int GetCharPosition (const char *str, const char *ch) const OVERRIDE
	{
		return ::GetCharPosition (this, str, ch);
	}

	/**
	 * Get the character that is at a position.
	 * @param str Layout string.
	 * @param x Position in the string.
	 * @return Pointer to the character at the position or NULL if no character is at the position.
	 */
	const char *GetCharAtPosition (const char *str, int x) const OVERRIDE
	{
		return ::GetCharAtPosition (this, str, x);
	}
};

/**
 * Wrapper for doing layouts with ICU.
 */
class ICUParagraphLayout : public ParagraphBuilder {
	ttd_unique_free_ptr <UChar> buffer; ///< The buffer.
	ttd_unique_ptr <ParagraphLayout> p; ///< The actual ICU paragraph layout.

public:
	/** Helper for GetLayouter, to get the right type. */
	typedef UChar CharType;

	ICUParagraphLayout (UChar *b, ParagraphLayout *p) : buffer(b), p(p)
	{
	}

	void build (LineVector *v, int max_width, bool reflow) OVERRIDE
	{
		if (reflow) this->p->reflow();

		ParagraphLayout::Line *l;
		while ((l = this->p->nextLine (max_width)) != NULL) {
			v->push_back (ttd_unique_ptr <const ParagraphLayouter::Line> (new ICULine (l)));
		}
	}

	static size_t append_char (UChar *buff, const UChar *buffer_last, WChar c)
	{
		/* Transform from UTF-32 to internal ICU format of UTF-16. */
		int32 length = 0;
		UErrorCode err = U_ZERO_ERROR;
		u_strFromUTF32 (buff, buffer_last - buff, &length, (UChar32*)&c, 1, &err);
		return length;
	}
};

static ICUParagraphLayout *GetParagraphLayout (const UChar *buffer, int32 length, FontMap &fontMapping)
{
	if (length == 0) {
		/* ICU's ParagraphLayout cannot handle empty strings, so fake one. */
		static const UChar empty[2] = { ' ', '\0' };
		buffer = empty;
		length = 1;
		fontMapping.back().first++;
	}

	/* Include trailing null char. */
	UChar *buff = xmemdupt <UChar> (buffer, length + 1);

	/* Fill ICU's FontRuns with the right data. */
	FontRuns runs (fontMapping.size());
	for (FontMap::iterator iter = fontMapping.begin(); iter != fontMapping.end(); iter++) {
		runs.add(iter->second, iter->first);
	}

	LEErrorCode status = LE_NO_ERROR;
	/* ParagraphLayout does not copy "buff", so it must stay valid.
	 * "runs" is copied according to the ICU source, but the documentation does not specify anything, so this might break somewhen. */
	ParagraphLayout *p = new ParagraphLayout(buff, length, &runs, NULL, NULL, NULL, _current_text_dir == TD_RTL ? UBIDI_DEFAULT_RTL : UBIDI_DEFAULT_LTR, false, status);
	if (status != LE_NO_ERROR) {
		delete p;
		free (buff);
		return NULL;
	}

	return new ICUParagraphLayout (buff, p);
}

#endif /* WITH_ICU_LAYOUT */

/*** Paragraph layout ***/

/** Visual run contains data about the bit of text with the same font. */
class FallbackVisualRun : public ParagraphLayouter::VisualRun {
	Font *font;       ///< The font used to layout these.
	GlyphID *glyphs;  ///< The glyphs we're drawing.
	float *positions; ///< The positions of the glyphs.
	int *glyph_to_char; ///< The char index of the glyphs.
	int glyph_count;  ///< The number of glyphs.

public:
	FallbackVisualRun(Font *font, const WChar *chars, int glyph_count, int x);
	~FallbackVisualRun();

	/**
	 * Get the font associated with this run.
	 * @return The font.
	 */
	const FontBase *GetFont (void) const OVERRIDE
	{
		return this->font;
	}

	/**
	 * Get the number of glyphs in this run.
	 * @return The number of glyphs.
	 */
	int GetGlyphCount (void) const OVERRIDE
	{
		return this->glyph_count;
	}

	/**
	 * Get the glyphs of this run.
	 * @return The glyphs.
	 */
	const GlyphID *GetGlyphs() const OVERRIDE
	{
		return this->glyphs;
	}

	/**
	 * Get the positions of this run.
	 * @return The positions.
	 */
	const float *GetPositions() const OVERRIDE
	{
		return this->positions;
	}

	/**
	 * Get the character index for a glyph index for this visual run.
	 * @param i The glyph index.
	 * @return The character index.
	 */
	int GetGlyphToChar (int i) const
	{
		return this->glyph_to_char[i];
	}

	bool GetGlyphPos (ParagraphLayouter::GlyphPos *gp, int i) const OVERRIDE;
};

/**
 * Create the visual run.
 * @param font       The font to use for this run.
 * @param chars      The characters to use for this run.
 * @param char_count The number of characters in this run.
 * @param x          The initial x position for this run.
 */
FallbackVisualRun::FallbackVisualRun(Font *font, const WChar *chars, int char_count, int x) :
		font(font), glyph_count(char_count)
{
	this->glyphs = xmalloct<GlyphID>(this->glyph_count);
	this->glyph_to_char = xmalloct<int>(this->glyph_count);

	/* Positions contains the location of the begin of each of the glyphs, and the end of the last one. */
	this->positions = xmalloct<float>(this->glyph_count * 2 + 2);
	this->positions[0] = x;
	this->positions[1] = 0;

	for (int i = 0; i < this->glyph_count; i++) {
		this->glyphs[i] = font->fc->MapCharToGlyph(chars[i]);
		this->positions[2 * i + 2] = this->positions[2 * i] + font->fc->GetGlyphWidth(this->glyphs[i]);
		this->positions[2 * i + 3] = 0;
		this->glyph_to_char[i] = i;
	}
}

/** Free all data. */
FallbackVisualRun::~FallbackVisualRun()
{
	free(this->positions);
	free(this->glyph_to_char);
	free(this->glyphs);
}

/**
 * Get the glyph and position for a glyph.
 * @param gp Struct to receive the data.
 * @param i Index of the glyph whose data to get.
 * @return Whether the glyph is valid (non-empty).
 */
bool FallbackVisualRun::GetGlyphPos (ParagraphLayouter::GlyphPos *gp, int i) const
{
	GlyphID glyph = this->glyphs[i];
	if (glyph == 0xFFFF) return false;

	gp->glyph = glyph;
	const float *pos = this->GetPositions();
	gp->x0 = pos[i * 2];
	gp->x1 = pos[i * 2 + 2];
	gp->y  = pos[i * 2 + 1];
	return true;
}

/** A single line worth of VisualRuns. */
class FallbackLine : public ParagraphLayouter::Line {
	std::vector <ttd_unique_ptr <FallbackVisualRun> > runs;

public:
	typedef FallbackVisualRun VisualRun;

	void append (Font *font, const WChar *chars, int glyph_count, int x)
	{
		FallbackVisualRun *run = new FallbackVisualRun (font, chars, glyph_count, x);
		this->runs.push_back (ttd_unique_ptr <FallbackVisualRun> (run));
	}

	int GetLeading (void) const OVERRIDE;
	int GetWidth (void) const OVERRIDE;

	/**
	 * Get the number of runs in this line.
	 * @return The number of runs.
	 */
	int CountRuns (void) const OVERRIDE
	{
		return this->runs.size();
	}

	/**
	 * Get a specific visual run.
	 * @return The visual run.
	 */
	const FallbackVisualRun *GetVisualRun (int run) const OVERRIDE
	{
		return this->runs[run].get();
	}

	static int GetInternalCharLength (WChar c)
	{
		return 1;
	}

	/**
	 * Get the position of a character in the layout.
	 * @param str Layout string.
	 * @param ch Character to get the position of.
	 * @return Left position of the character relative to the start of the string.
	 * @note Will only work right for single-line strings.
	 */
	int GetCharPosition (const char *str, const char *ch) const OVERRIDE
	{
		return ::GetCharPosition (this, str, ch);
	}

	/**
	 * Get the character that is at a position.
	 * @param str Layout string.
	 * @param x Position in the string.
	 * @return Pointer to the character at the position or NULL if no character is at the position.
	 */
	const char *GetCharAtPosition (const char *str, int x) const OVERRIDE
	{
		return ::GetCharAtPosition (this, str, x);
	}
};

/**
 * Get the height of the line.
 * @return The maximum height of the line.
 */
int FallbackLine::GetLeading (void) const
{
	int leading = 0;
	std::vector <ttd_unique_ptr <FallbackVisualRun> >::const_iterator
			iter (this->runs.begin());
	while (iter != this->runs.end()) {
		leading = max (leading, (*iter++)->GetFont()->fc->GetHeight());
	}
	return leading;
}

/**
 * Get the width of this line.
 * @return The width of the line.
 */
int FallbackLine::GetWidth (void) const
{
	if (this->runs.empty()) return 0;

	/*
	 * The last X position of a run contains is the end of that run.
	 * Since there is no left-to-right support, taking this value of
	 * the last run gives us the end of the line and thus the width.
	 */
	const FallbackVisualRun *run = this->runs.back().get();
	return (int)run->GetPositions()[run->GetGlyphCount() * 2];
}

/**
 * Class handling the splitting of a paragraph of text into lines and
 * visual runs.
 *
 * One constructs this class with the text that needs to be split into
 * lines. Then nextLine is called with the maximum width until NULL is
 * returned. Each nextLine call creates VisualRuns which contain the
 * length of text that are to be drawn with the same font. In other
 * words, the result of this class is a list of sub strings with their
 * font. The sub strings are then already fully laid out, and only
 * need actual drawing.
 *
 * The positions in a visual run are sequential pairs of X,Y of the
 * begin of each of the glyphs plus an extra pair to mark the end.
 *
 * @note This variant does not handle left-to-right properly. This
 *       is supported in the one ParagraphLayout coming from ICU.
 */
class FallbackParagraphLayout : public ParagraphBuilder, FlexArray<WChar> {
public:
	/** Helper for GetLayouter, to get the right type. */
	typedef WChar CharType;

	FontMap runs;              ///< The fonts we have to use for this paragraph.
	WChar data[];              ///< The buffer.

private:
	/**
	 * Construct a new paragraph layout.
	 * @param buffer The characters of the paragraph.
	 * @param length The length of the paragraph.
	 * @param runs   The font mapping of this paragraph.
	 */
	FallbackParagraphLayout (const WChar *buffer, int length, FontMap &runs)
	{
		/* Include trailing null char. */
		assert (buffer[length] == '\0');
		memcpy (this->data, buffer, (length + 1) * sizeof(WChar));
		assert (!runs.empty());
		assert (runs.back().first == length);
		this->runs.swap (runs);
	}

public:
	/**
	 * Create a new paragraph layout.
	 * @param buffer The characters of the paragraph.
	 * @param length The length of the paragraph.
	 * @param runs   The font mapping of this paragraph.
	 */
	static FallbackParagraphLayout *create (const WChar *buffer, size_t length, FontMap &runs)
	{
		return new (length + 1) FallbackParagraphLayout (buffer, length, runs);
	}

	void build (LineVector *v, int max_width, bool reflow) OVERRIDE;

	/**
	 * Append a wide character to the internal buffer.
	 * @param buff        The buffer to append to.
	 * @param buffer_last The end of the buffer.
	 * @param c           The character to add.
	 * @return The number of buffer spaces that were used.
	 */
	static size_t append_char (WChar *buff, const WChar *buffer_last, WChar c)
	{
		/* Filter out text direction characters that shouldn't be drawn, and
		 * will not be handled in the fallback non ICU case because they are
		 * mostly needed for RTL languages which need more ICU support. */
		if (IsTextDirectionChar (c)) return 0;

		*buff = c;
		return 1;
	}
};

/**
 * Build the paragraph with a maximum width.
 * @param v The line vector where to store the resulting paragraph.
 * @param max_width The maximum width of the string.
 * @return A Line, or NULL when at the end of the paragraph.
 */
void FallbackParagraphLayout::build (LineVector *v, int max_width, bool)
{
	const WChar *buffer = this->data;

	if (*buffer == '\0') {
		/* Only a newline. */
		FallbackLine *l = new FallbackLine();
		l->append (this->runs.front().second, buffer, 0, 0);
		v->push_back (ttd_unique_ptr <const ParagraphLayouter::Line> (l));
		return;
	}

	while (buffer != NULL) {
		/* Simple idea:
		 *  - split a line at a newline character, or at a space where we can break a line.
		 *  - split for a visual run whenever a new line happens, or the font changes.
		 */
		FallbackLine *l = new FallbackLine();

		const WChar *begin = buffer;
		const WChar *last_space = NULL;
		const WChar *last_char = begin;
		int width = 0;

		int offset = buffer - this->data;
		FontMap::const_iterator iter = this->runs.begin();
		while (iter->first <= offset) {
			iter++;
			assert (iter != this->runs.end());
		}

		FontCache *fc = iter->second->fc;
		const WChar *next_run = this->data + iter->first;

		for (;; buffer++) {
			WChar c = *buffer;
			last_char = buffer;

			if (c == '\0') {
				buffer = NULL;
				break;
			}

			if (buffer == next_run) {
				int w = l->GetWidth();
				l->append (iter->second, begin, buffer - begin, w);
				iter++;
				assert (iter != this->runs.end());

				next_run = this->data + iter->first;
				begin = buffer;

				last_space = NULL;
			}

			if (IsWhitespace(c)) last_space = buffer;

			if (!IsPrintable(c) || IsTextDirectionChar(c)) continue;

			int char_width = fc->GetCharacterWidth (c);
			width += char_width;
			if (width <= max_width) continue;

			/* The string is longer than maximum width so we need
			 * to decide what to do with it. */
			if (width == char_width) {
				/* The character is wider than allowed width; don't know
				 * what to do with this case... bail out! */
				v->push_back (ttd_unique_ptr <const ParagraphLayouter::Line> (l));
				return;
			}

			if (last_space == NULL) {
				/* No space has been found. Just terminate at our current
				 * location. This usually happens for languages that do not
				 * require spaces in strings, like Chinese, Japanese and
				 * Korean. For other languages terminating mid-word might
				 * not be the best, but terminating the whole string instead
				 * of continuing the word at the next line is worse. */
			} else {
				/* A space is found; perfect place to terminate */
				buffer = last_space + 1;
				last_char = last_space;
			}

			break;
		}

		if (l->CountRuns() == 0 || last_char - begin != 0) {
			int w = l->GetWidth();
			l->append (iter->second, begin, last_char - begin, w);
		}

		v->push_back (ttd_unique_ptr <const ParagraphLayouter::Line> (l));
	}
}

/**
 * Get the actual ParagraphLayout for the given buffer.
 * @param buff The begin of the buffer.
 * @param length The length of the buffer.
 * @param fontMapping THe mapping of the fonts.
 * @return The ParagraphLayout instance.
 */
static FallbackParagraphLayout *GetParagraphLayout (WChar *buff, int length, FontMap &fontMapping)
{
	return FallbackParagraphLayout::create (buff, length, fontMapping);
}


/**
 * Text drawing parameters, which can change while drawing a line, but are
 * kept between multiple parts of the same text, e.g. on line breaks.
 */
struct FontState {
	FontSize fontsize;       ///< Current font size.
	TextColour cur_colour;   ///< Current text colour.
	TextColour prev_colour;  ///< Text colour from before the last colour switch.

	FontState() : fontsize(FS_END), cur_colour(TC_INVALID), prev_colour(TC_INVALID) {}
	FontState(TextColour colour, FontSize fontsize) : fontsize(fontsize), cur_colour(colour), prev_colour(colour) {}

	/**
	 * Switch to new colour \a c.
	 * @param c New colour to use.
	 */
	inline void SetColour(TextColour c)
	{
		assert(c >= TC_BLUE && c <= TC_BLACK);
		this->prev_colour = this->cur_colour;
		this->cur_colour = c;
	}

	/** Switch to previous colour. */
	inline void SetPreviousColour()
	{
		Swap(this->cur_colour, this->prev_colour);
	}

	/**
	 * Switch to using a new font \a f.
	 * @param f New font to use.
	 */
	inline void SetFontSize(FontSize f)
	{
		this->fontsize = f;
	}
};

/** Key into the linecache */
struct LineCacheKey {
	FontState state_before; ///< Font state at the beginning of the line.
	std::string str;        ///< Source string of the line (including colour and font size codes).

	/** Comparison operator for std::map */
	bool operator < (const LineCacheKey &other) const
	{
		if (this->state_before.fontsize    != other.state_before.fontsize)    return this->state_before.fontsize    < other.state_before.fontsize;
		if (this->state_before.cur_colour  != other.state_before.cur_colour)  return this->state_before.cur_colour  < other.state_before.cur_colour;
		if (this->state_before.prev_colour != other.state_before.prev_colour) return this->state_before.prev_colour < other.state_before.prev_colour;
		return this->str < other.str;
	}
};

/** Item in the linecache */
struct LineCacheItem {
	FontState state_after;     ///< Font state after the line.
	ParagraphBuilder *layout;  ///< Layout of the line.

	LineCacheItem() : layout(NULL) {}
	~LineCacheItem() { delete layout; }
};

typedef std::map <LineCacheKey, LineCacheItem> LineCache;

/** Cache of ParagraphLayout lines. */
static LineCache *linecache;

/** Clear line cache. */
static void ResetLineCache (void)
{
	if (linecache != NULL) linecache->clear();
}

/**
 * Helper for getting a ParagraphBuilder of the given type.
 *
 * @param str The string to create a layouter for.
 * @param state The state of the font and color.
 * @tparam T The type of layouter we want.
 * @return The ParagraphBuilder constructed, or NULL on error.
 */
template <typename T>
static inline ParagraphBuilder *GetLayouter (const char *&str, FontState &state)
{
	typename T::CharType buffer [DRAW_STRING_BUFFER + 1];
	const typename T::CharType *const buffer_last = lastof(buffer);

	typename T::CharType *buff = buffer;
	FontMap fontMapping;
	Font *f = GetFont (state.fontsize, state.cur_colour);

	/*
	 * Go through the whole string while adding Font instances to the font map
	 * whenever the font changes, and convert the wide characters into a format
	 * usable by ParagraphLayout.
	 */
	bool just_inserted = false;
	for (; buff < buffer_last;) {
		WChar c = Utf8Consume(const_cast<const char **>(&str));
		if (c == '\0' || c == '\n') {
			break;
		} else if (c >= SCC_BLUE && c <= SCC_BLACK) {
			state.SetColour((TextColour)(c - SCC_BLUE));
		} else if (c == SCC_PREVIOUS_COLOUR) { // Revert to the previous colour.
			state.SetPreviousColour();
		} else if (c == SCC_TINYFONT) {
			state.SetFontSize(FS_SMALL);
		} else if (c == SCC_BIGFONT) {
			state.SetFontSize(FS_LARGE);
		} else {
			size_t length = T::append_char (buff, buffer_last, c);
			if (length > 0) {
				buff += length;
				just_inserted = false;
			}
			continue;
		}

		if (!just_inserted) {
			fontMapping.push_back (std::make_pair (buff - buffer, f));
			just_inserted = true;
		}
		f = GetFont (state.fontsize, state.cur_colour);
	}

	/* Better safe than sorry. */
	*buff = '\0';

	if (!just_inserted) {
		fontMapping.push_back (std::make_pair (buff - buffer, f));
	} else {
		assert (!fontMapping.empty());
		assert (fontMapping.back().first == buff - buffer);
	}

	return GetParagraphLayout (buffer, buff - buffer, fontMapping);
}

/**
 * Create a new layouter.
 * @param str      The string to create the layout for.
 * @param maxw     The maximum width.
 * @param colour   The colour of the font.
 * @param fontsize The size of font to use.
 */
Layouter::Layouter(const char *str, int maxw, TextColour colour, FontSize fontsize)
{
	FontState state(colour, fontsize);
	WChar c = 0;

	do {
		/* Scan string for end of line */
		const char *lineend = str;
		for (;;) {
			size_t len = Utf8Decode(&c, lineend);
			if (c == '\0' || c == '\n') break;
			lineend += len;
		}

		/* Create linecache on first access to avoid trouble with initialisation order of static variables. */
		if (linecache == NULL) linecache = new LineCache();

		LineCacheKey key;
		key.state_before = state;
		key.str.assign (str, lineend - str);
		LineCacheItem& line = (*linecache)[key];

		bool reflow;
		if (line.layout != NULL) {
			/* Line is in cache */
			str = lineend + 1;
			state = line.state_after;
			reflow = true;
		} else {
			/* Line is new, layout it */
			ParagraphBuilder *layout;
#ifdef WITH_ICU_LAYOUT
			FontState old_state = state;
			const char *old_str = str;

			layout = GetLayouter<ICUParagraphLayout> (str, state);
			if (layout == NULL) {
				static bool warned = false;
				if (!warned) {
					DEBUG(misc, 0, "ICU layouter bailed on the font. Falling back to the fallback layouter");
					warned = true;
				}

				state = old_state;
				str = old_str;
				layout = GetLayouter<FallbackParagraphLayout> (str, state);
			}
#else
			layout = GetLayouter<FallbackParagraphLayout> (str, state);
#endif
			/* The fallback layout should never fail. */
			assert (layout != NULL);
			line.layout = layout;
			line.state_after = state;

			reflow = false;
		}

		/* Copy all lines into a local cache so we can reuse them later on more easily. */
		line.layout->build (this, maxw, reflow);

	} while (c != '\0');
}

/**
 * Get the boundaries of this paragraph.
 * @return The boundaries.
 */
Dimension Layouter::GetBounds()
{
	Dimension d = { 0, 0 };
	for (Layouter::const_iterator l (this->begin()); l != this->end(); l++) {
		d.width = max<uint>(d.width, (*l)->GetWidth());
		d.height += (*l)->GetLeading();
	}
	return d;
}

/**
 * Reset cached font information.
 * @param size Font size to reset.
 */
void Layouter::ResetFontCache(FontSize size)
{
	for (FontColourMap::iterator it = fonts[size].Begin(); it != fonts[size].End(); ++it) {
		delete it->second;
	}
	fonts[size].Clear();

	/* We must reset the linecache since it references the just freed fonts */
	ResetLineCache();
}

/**
 * Reduce the size of linecache if necessary to prevent infinite growth.
 */
void Layouter::ReduceLineCache()
{
	if (linecache != NULL) {
		/* TODO LRU cache would be fancy, but not exactly necessary */
		if (linecache->size() > 4096) ResetLineCache();
	}
}
