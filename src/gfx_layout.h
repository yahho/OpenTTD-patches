/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gfx_layout.h Functions related to laying out the texts. */

#ifndef GFX_LAYOUT_H
#define GFX_LAYOUT_H

#include <vector>

#include "core/pointer.h"
#include "font.h"
#include "gfx_func.h"

/** Common information about a font. */
struct FontBase {
	FontCache *fc;     ///< The font we are using.
	TextColour colour; ///< The colour this font has to be.

	FontBase (FontSize size, TextColour colour);
};

/** Namespace for paragraph layout-related types. */
namespace ParagraphLayouter {
	/** Visual run contains data about the bit of text with the same font. */
	class VisualRun {
	public:
		virtual ~VisualRun() {}
		virtual const FontBase *GetFont() const = 0;
		virtual int GetGlyphCount() const = 0;
		virtual const GlyphID *GetGlyphs() const = 0;
		virtual const float *GetPositions() const = 0;
		virtual int GetLeading() const = 0;
		virtual const int *GetGlyphToCharMap() const = 0;
	};

	/** A single line worth of VisualRuns. */
	class Line {
	public:
		virtual ~Line() {}
		virtual int GetLeading() const = 0;
		virtual int GetWidth() const = 0;
		virtual int CountRuns() const = 0;
		virtual const VisualRun *GetVisualRun(int run) const = 0;
		virtual int GetInternalCharLength(WChar c) const = 0;
		int GetCharPosition (const char *str, const char *ch) const;
		const char *GetCharAtPosition (const char *str, int x) const;
	};
};

/**
 * The layouter performs all the layout work.
 *
 * It also accounts for the memory allocations and frees.
 */
class Layouter : public std::vector <ttd_unique_ptr <const ParagraphLayouter::Line> > {
public:
	Layouter(const char *str, int maxw = INT32_MAX, TextColour colour = TC_FROMSTRING, FontSize fontsize = FS_NORMAL);
	Dimension GetBounds();

	static void ResetFontCache(FontSize size);
	static void ReduceLineCache();
};

#endif /* GFX_LAYOUT_H */
