/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file textfile.cpp Code related to textfiles. */

#include "stdafx.h"
#include "core/math_func.hpp"
#include "fileio_func.h"
#include "font.h"
#include "gfx_type.h"
#include "gfx_func.h"
#include "string.h"
#include "textfile.h"

#include "widgets/misc_widget.h"

#include "table/strings.h"

#if defined(WITH_ZLIB)
#include <zlib.h>
#endif

#if defined(WITH_LZMA)
#include <lzma.h>
#endif

/** Widgets for the textfile window. */
static const NWidgetPart _nested_textfile_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_MAUVE),
		NWidget(WWT_CAPTION, COLOUR_MAUVE, WID_TF_CAPTION), SetDataTip(STR_NULL, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_TEXTBTN, COLOUR_MAUVE, WID_TF_WRAPTEXT), SetDataTip(STR_TEXTFILE_WRAP_TEXT, STR_TEXTFILE_WRAP_TEXT_TOOLTIP),
		NWidget(WWT_DEFSIZEBOX, COLOUR_MAUVE),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, COLOUR_MAUVE, WID_TF_BACKGROUND), SetMinimalSize(200, 125), SetResize(1, 12), SetScrollbar(WID_TF_VSCROLLBAR),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_VSCROLLBAR, COLOUR_MAUVE, WID_TF_VSCROLLBAR),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(NWID_HSCROLLBAR, COLOUR_MAUVE, WID_TF_HSCROLLBAR),
		NWidget(WWT_RESIZEBOX, COLOUR_MAUVE),
	EndContainer(),
};

/** Window preferences for the textfile window */
static WindowDesc::Prefs _textfile_prefs ("textfile");

/** Window definition for the textfile window */
static const WindowDesc _textfile_desc(
	WDP_CENTER, 630, 460,
	WC_TEXTFILE, WC_NONE,
	0,
	_nested_textfile_widgets, lengthof(_nested_textfile_widgets),
	&_textfile_prefs
);


/** Stream template struct. */
template <TextfileDesc::Format FMT>
struct stream;

/** Stream loop function results. */
enum StreamResult {
	STREAM_OK,    ///< success, decoding in progress
	STREAM_END,   ///< success, decoding finished
	STREAM_ERROR, ///< error
};

#ifdef WITH_ZLIB

/** Zlib stream struct. */
template <>
struct stream <TextfileDesc::FORMAT_GZ> {
	typedef z_stream stream_type;

	static inline void construct (z_stream *z)
	{
		memset (z, 0, sizeof(z_stream));
	}

	static inline bool init (z_stream *z)
	{
		/* Window size is 15, plus flag 32 for automatic header detection. */
		return inflateInit2 (z, 15 + 32) == Z_OK;
	}

	static inline StreamResult loop (z_stream *z, bool finish)
	{
		switch (inflate (z, finish ? Z_FINISH : Z_NO_FLUSH)) {
			case Z_OK:
				return STREAM_OK;

			case Z_STREAM_END:
				return STREAM_END;

			case Z_BUF_ERROR:
				if (z->avail_out == 0) return STREAM_OK;
				/* fall through */
			default:
				return STREAM_ERROR;
		}
	}

	static inline void end (z_stream *z)
	{
		int r = inflateEnd (z);
		assert (r == Z_OK);
	}
};

#endif /* WITH_ZLIB */

#ifdef WITH_LZMA

/** LZMA stream struct. */
template <>
struct stream <TextfileDesc::FORMAT_XZ> {
	typedef lzma_stream stream_type;

	static inline void construct (lzma_stream *z)
	{
		memset (z, 0, sizeof(lzma_stream));
	}

	static inline bool init (lzma_stream *z)
	{
		return lzma_auto_decoder (z, UINT64_MAX, LZMA_CONCATENATED) == LZMA_OK;
	}

	static inline StreamResult loop (lzma_stream *z, bool finish)
	{
		switch (lzma_code (z, finish ? LZMA_FINISH : LZMA_RUN)) {
			case LZMA_OK:          return STREAM_OK;
			case LZMA_STREAM_END:  return STREAM_END;
			default:               return STREAM_ERROR;
		}
	}

	static inline void end (lzma_stream *z)
	{
		lzma_end (z);
	}
};

#endif /* WITH_LZMA */

/**
 * Read in data from file and update stream.
 * @param input Buffer to read data into.
 * @param handle Handle of file to read from.
 * @param remaining Remaining size of file to read, updated on return.
 * @param z Stream to update.
 * @return Whether (some) data could be read.
 */
template <uint N, typename Z>
static bool fill_buffer (byte (*input) [N], FILE *handle, size_t *remaining, Z *z)
{
	assert (z->avail_in == 0);

	size_t read = fread (input, 1, min (*remaining, N), handle);
	if (read == 0) return false;

	*remaining -= read;
	z->avail_in = read;
	z->next_in  = &(*input)[0];
	return true;
}

/**
 * Read in a compressed file.
 * @tparam FMT Stream format type.
 * @param handle Handle of file to read.
 * @param filesize Size of file to read.
 * @param len Pointer to store decompressed length on success, excluding
 * appended null char.
 * @return Newly allocated storage with the decompressed contents of the file.
 */
template <TextfileDesc::Format FMT>
static char *stream_unzip (FILE *handle, size_t filesize, size_t *len)
{
	static const uint BLOCKSIZE = 4096;
	byte input [1024];

	/* Initialise stream. */
	typename stream<FMT>::stream_type z;
	stream<FMT>::construct (&z);
	if (!fill_buffer (&input, handle, &filesize, &z)) return NULL;
	if (!stream<FMT>::init (&z)) return NULL;

	/* Assume output will be at least as big as input, and align up. */
	size_t alloc = Align (filesize + z.avail_in,  BLOCKSIZE);
	char *output = xmalloc (alloc);
	z.next_out = (byte*)output;
	z.avail_out = alloc;
	assert (z.avail_out > 0);

	/* Decode loop. */
	int r;
	for (;;) {
		if (z.avail_in == 0 && filesize != 0
				&& !fill_buffer (&input, handle, &filesize, &z)) {
			r = STREAM_ERROR;
			break;
		}

		assert (z.avail_out != 0);
		r = stream<FMT>::loop (&z, filesize == 0);
		if (r != STREAM_OK) break;

		if (z.avail_out == 0) {
			assert (z.next_out == (byte*)(output + alloc));
			size_t new_alloc = alloc +
				Align (filesize + z.avail_in, BLOCKSIZE);
			output = xrealloc (output, new_alloc);
			z.next_out = (byte*)(output + alloc);
			alloc = new_alloc;
			z.avail_out = BLOCKSIZE;
		}
	}

	/* Finish decoding. */
	stream<FMT>::end (&z);

	if (r != STREAM_END) {
		free (output);
		return NULL;
	}

	/* Compute total size. */
	size_t total = alloc - z.avail_out;
	if (total == 0) {
		/* No output? */
		free (output);
		return NULL;
	}
	*len = total;

	/* Append null terminator if required. */
	if (z.avail_out == 0) {
		if (output[total - 1] == '\n') {
			total--;
		} else {
			output = xrealloc (output, alloc + 1);
		}
	}
	output[total] = '\0';
	return output;
}

/*
 * Read in the text file represented by this description.
 * @param len pointer to store file length on success, excluding appended 0
 * @return pointer to newly allocated storage with the contents of the file on success, or NULL on error
 */
char *TextfileDesc::read (size_t *len) const
{
	size_t filesize;
	FILE *handle = FioFOpenFile (this->path.get(), "rb", this->dir, &filesize);
	if (handle == NULL) return NULL;

	switch (this->format) {
		default: NOT_REACHED();

		case FORMAT_RAW: {
			char *text = xmalloc (filesize + 1);
			size_t read = fread (text, 1, filesize, handle);
			fclose (handle);
			if (read != filesize) {
				free (text);
				return NULL;
			}
			text[filesize] = '\0';
			*len = filesize;
			return text;
		}

#if defined(WITH_ZLIB)
		case FORMAT_GZ: {
			char *text = stream_unzip<FORMAT_GZ> (handle, filesize, len);
			fclose (handle);
			return text;
		}
#endif

#if defined(WITH_LZMA)
		case FORMAT_XZ: {
			char *text = stream_unzip<FORMAT_XZ> (handle, filesize, len);
			fclose (handle);
			return text;
		}
#endif
	}
}

TextfileWindow::TextfileWindow (const TextfileDesc &txt) :
	Window (&_textfile_desc), file_type (txt.type),
	vscroll (NULL), hscroll (NULL), text (NULL), lines ()
{
	this->CreateNestedTree();
	this->vscroll = this->GetScrollbar(WID_TF_VSCROLLBAR);
	this->hscroll = this->GetScrollbar(WID_TF_HSCROLLBAR);
	this->InitNested();
	this->GetWidget<NWidgetCore>(WID_TF_CAPTION)->SetDataTip(STR_TEXTFILE_README_CAPTION + txt.type, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS);

	this->hscroll->SetStepSize(10); // Speed up horizontal scrollbar
	this->vscroll->SetStepSize(FONT_HEIGHT_MONO);

	if (!txt.valid()) return;

	this->lines.clear();

	size_t filesize;
	this->text = txt.read (&filesize);
	if (!this->text) return;

	/* Replace tabs and line feeds with a space since str_validate removes those. */
	for (char *p = this->text; *p != '\0'; p++) {
		if (*p == '\t' || *p == '\r') *p = ' ';
	}

	/* Check for the byte-order-mark, and skip it if needed. */
	char *p = this->text;
	if (strncmp ("\xEF\xBB\xBF", p, 3) == 0) p += 3;

	/* Make sure the string is a valid UTF-8 sequence. */
	str_validate (p, this->text + filesize, SVS_REPLACE_WITH_QUESTION_MARK | SVS_ALLOW_NEWLINE);

	/* Split the string on newlines. */
	for (;;) {
		this->lines.push_back (p);
		p = strchr (p, '\n');
		if (p == NULL) break;
		*(p++) = '\0';
		/* Break on last line if the file does end with a newline. */
		if (p == this->text + filesize) break;
	}
}

/* virtual */ TextfileWindow::~TextfileWindow()
{
	free(this->text);
}

/**
 * Get the total height of the content displayed in this window, if wrapping is disabled.
 * @return the height in pixels
 */
uint TextfileWindow::GetContentHeight()
{
	int max_width = this->GetWidget<NWidgetCore>(WID_TF_BACKGROUND)->current_x - WD_FRAMETEXT_LEFT - WD_FRAMERECT_RIGHT;

	uint height = 0;
	for (uint i = 0; i < this->lines.size(); i++) {
		height += GetStringHeight(this->lines[i], max_width, FS_MONO);
	}

	return height;
}

/* virtual */ void TextfileWindow::UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
{
	switch (widget) {
		case WID_TF_BACKGROUND:
			resize->height = 1;

			size->height = 4 * resize->height + TOP_SPACING + BOTTOM_SPACING; // At least 4 lines are visible.
			size->width = max(200u, size->width); // At least 200 pixels wide.
			break;
	}
}

/** Set scrollbars to the right lengths. */
void TextfileWindow::SetupScrollbars()
{
	if (IsWidgetLowered(WID_TF_WRAPTEXT)) {
		this->vscroll->SetCount(this->GetContentHeight());
		this->hscroll->SetCount(0);
	} else {
		uint max_length = 0;
		for (uint i = 0; i < this->lines.size(); i++) {
			max_length = max(max_length, GetStringBoundingBox(this->lines[i], FS_MONO).width);
		}
		this->vscroll->SetCount(this->lines.size() * FONT_HEIGHT_MONO);
		this->hscroll->SetCount(max_length + WD_FRAMETEXT_LEFT + WD_FRAMETEXT_RIGHT);
	}

	this->SetWidgetDisabledState(WID_TF_HSCROLLBAR, IsWidgetLowered(WID_TF_WRAPTEXT));
}

/* virtual */ void TextfileWindow::OnClick(Point pt, int widget, int click_count)
{
	switch (widget) {
		case WID_TF_WRAPTEXT:
			this->ToggleWidgetLoweredState(WID_TF_WRAPTEXT);
			this->SetupScrollbars();
			this->InvalidateData();
			break;
	}
}

void TextfileWindow::DrawWidget (BlitArea *dpi, const Rect &r, int widget) const
{
	if (widget != WID_TF_BACKGROUND) return;

	const int x = r.left + WD_FRAMETEXT_LEFT;
	const int y = r.top + WD_FRAMETEXT_TOP;
	const int right = r.right - WD_FRAMETEXT_RIGHT;
	const int bottom = r.bottom - WD_FRAMETEXT_BOTTOM;

	BlitArea new_dpi;
	if (!InitBlitArea (dpi, &new_dpi, x, y, right - x + 1, bottom - y + 1)) return;

	/* Draw content (now coordinates given to DrawString* are local to the new clipping region). */
	int line_height = FONT_HEIGHT_MONO;
	int y_offset = -this->vscroll->GetPosition();

	for (uint i = 0; i < this->lines.size(); i++) {
		if (IsWidgetLowered(WID_TF_WRAPTEXT)) {
			y_offset = DrawStringMultiLine (&new_dpi, 0, right - x, y_offset, bottom - y, this->lines[i], TC_WHITE, SA_TOP | SA_LEFT, false, FS_MONO);
		} else {
			DrawString (&new_dpi, -this->hscroll->GetPosition(), right - x, y_offset, this->lines[i], TC_WHITE, SA_TOP | SA_LEFT, false, FS_MONO);
			y_offset += line_height; // margin to previous element
		}
	}
}

/* virtual */ void TextfileWindow::OnResize()
{
	this->vscroll->SetCapacityFromWidget(this, WID_TF_BACKGROUND, TOP_SPACING + BOTTOM_SPACING);
	this->hscroll->SetCapacityFromWidget(this, WID_TF_BACKGROUND);

	this->SetupScrollbars();
}

void TextfileWindow::GlyphSearcher::Reset()
{
	this->iter = this->begin;
}

const char *TextfileWindow::GlyphSearcher::NextString()
{
	return (this->iter == this->end) ? NULL : *(this->iter++);
}

/**
 * Search a textfile file next to the given content.
 * @param type The type of the textfile to search for.
 * @param dir The subdirectory to search in.
 * @param filename The filename of the content to look for.
 */
TextfileDesc::TextfileDesc (TextfileType type, Subdirectory dir, const char *filename)
	: type(type), dir(dir)
{
	static const char * const prefixes[] = {
		"readme",
		"changelog",
		"license",
	};
	assert_compile(lengthof(prefixes) == TFT_END);

	if (filename == NULL) {
		this->format = FORMAT_END;
		return;
	}

	const char *slash = strrchr (filename, PATHSEPCHAR);
	if (slash == NULL) {
		this->format = FORMAT_END;
		return;
	}

	size_t alloc_size = (slash - filename)
		+ 1     // slash
		+ 9     // longest prefix length ("changelog")
		+ 1     // underscore
		+ 7     // longest possible language length
		+ 9;    // ".txt.ext" and null terminator

	this->path.reset (xmalloc (alloc_size));
	stringb buf (alloc_size, this->path.get());
	buf.fmt ("%.*s%s", (int)(slash - filename + 1), filename, prefixes[type]);
	size_t base_length = buf.length();

	static const char * const exts[] = {
		"txt",
#if defined(WITH_ZLIB)
		"txt.gz",
#endif
#if defined(WITH_LZMA)
		"txt.xz",
#endif
	};
	assert_compile (lengthof(exts) == FORMAT_END);

	for (size_t i = 0; i < lengthof(exts); i++) {
		buf.truncate (base_length);
		buf.append_fmt ("_%s.%s", GetCurrentLanguageIsoCode(), exts[i]);
		if (FioCheckFileExists (buf.c_str(), dir)) {
			this->format = (Format) i;
			return;
		}

		buf.truncate (base_length);
		buf.append_fmt ("_%.2s.%s", GetCurrentLanguageIsoCode(), exts[i]);
		if (FioCheckFileExists (buf.c_str(), dir)) {
			this->format = (Format) i;
			return;
		}

		buf.truncate (base_length);
		buf.append_fmt (".%s", exts[i]);
		if (FioCheckFileExists (buf.c_str(), dir)) {
			this->format = (Format) i;
			return;
		}
	}

	this->path.reset();
	assert (this->path.get() == NULL);
	this->format = FORMAT_END;
}
