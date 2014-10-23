/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file textfile.cpp Code related to textfiles. */

#include "stdafx.h"
#include "fileio_func.h"
#include "fontcache.h"
#include "gfx_type.h"
#include "gfx_func.h"
#include "string.h"
#include "textfile.h"

#include "widgets/misc_widget.h"

#include "table/strings.h"

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

/** Window definition for the textfile window */
static WindowDesc _textfile_desc(
	WDP_CENTER, "textfile", 630, 460,
	WC_TEXTFILE, WC_NONE,
	0,
	_nested_textfile_widgets, lengthof(_nested_textfile_widgets)
);

TextfileWindow::TextfileWindow (const TextfileDesc &txt)
	: Window(&_textfile_desc), file_type(txt.type)
{
	this->CreateNestedTree();
	this->vscroll = this->GetScrollbar(WID_TF_VSCROLLBAR);
	this->hscroll = this->GetScrollbar(WID_TF_HSCROLLBAR);
	this->FinishInitNested();
	this->GetWidget<NWidgetCore>(WID_TF_CAPTION)->SetDataTip(STR_TEXTFILE_README_CAPTION + txt.type, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS);

	this->hscroll->SetStepSize(10); // Speed up horizontal scrollbar
	this->vscroll->SetStepSize(FONT_HEIGHT_MONO);

	if (!txt.valid()) return;

	this->lines.clear();

	/* Get text from file */
	size_t filesize;
	FILE *handle = FioFOpenFile (txt.path, "rb", txt.dir, &filesize);
	if (handle == NULL) return;

	this->text = xmalloc (filesize + 1);
	size_t read = fread (this->text, 1, filesize, handle);
	fclose (handle);

	if (read != filesize) return;

	this->text[filesize] = '\0';

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

/* virtual */ void TextfileWindow::DrawWidget(const Rect &r, int widget) const
{
	if (widget != WID_TF_BACKGROUND) return;

	const int x = r.left + WD_FRAMETEXT_LEFT;
	const int y = r.top + WD_FRAMETEXT_TOP;
	const int right = r.right - WD_FRAMETEXT_RIGHT;
	const int bottom = r.bottom - WD_FRAMETEXT_BOTTOM;

	DrawPixelInfo new_dpi;
	if (!FillDrawPixelInfo(&new_dpi, x, y, right - x + 1, bottom - y + 1)) return;
	DrawPixelInfo *old_dpi = _cur_dpi;
	_cur_dpi = &new_dpi;

	/* Draw content (now coordinates given to DrawString* are local to the new clipping region). */
	int line_height = FONT_HEIGHT_MONO;
	int y_offset = -this->vscroll->GetPosition();

	for (uint i = 0; i < this->lines.size(); i++) {
		if (IsWidgetLowered(WID_TF_WRAPTEXT)) {
			y_offset = DrawStringMultiLine(0, right - x, y_offset, bottom - y, this->lines[i], TC_WHITE, SA_TOP | SA_LEFT, false, FS_MONO);
		} else {
			DrawString(-this->hscroll->GetPosition(), right - x, y_offset, this->lines[i], TC_WHITE, SA_TOP | SA_LEFT, false, FS_MONO);
			y_offset += line_height; // margin to previous element
		}
	}

	_cur_dpi = old_dpi;
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
		this->path = NULL;
		return;
	}

	const char *slash = strrchr (filename, PATHSEPCHAR);
	if (slash == NULL) {
		this->path = NULL;
		return;
	}

	size_t alloc_size = (slash - filename)
		+ 1     // slash
		+ 9     // longest prefix length ("changelog")
		+ 1     // underscore
		+ 7     // longest possible language length
		+ 9;    // ".txt.ext" and null terminator

	this->path = xmalloc (alloc_size);
	stringb buf (alloc_size, this->path);
	buf.fmt ("%.*s%s", (int)(slash - filename + 1), filename, prefixes[type]);
	size_t base_length = buf.length();

	static const char * const exts[] = {
		"txt",
	};

	for (size_t i = 0; i < lengthof(exts); i++) {
		buf.truncate (base_length);
		buf.append_fmt ("_%s.%s", GetCurrentLanguageIsoCode(), exts[i]);
		if (FioCheckFileExists (buf.c_str(), dir)) return;

		buf.truncate (base_length);
		buf.append_fmt ("_%.2s.%s", GetCurrentLanguageIsoCode(), exts[i]);
		if (FioCheckFileExists (buf.c_str(), dir)) return;

		buf.truncate (base_length);
		buf.append_fmt (".%s", exts[i]);
		if (FioCheckFileExists (buf.c_str(), dir)) return;
	}

	free (this->path);
	this->path = NULL;
}
