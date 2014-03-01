/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file dbg_helpers.cpp Helpers for outputting debug information. */

#include "../stdafx.h"
#include <stdarg.h>
#include "dbg_helpers.h"

/** Trackdir & TrackdirBits short names. */
static const char * const trackdir_names[] = {
	"NE", "SE", "UE", "LE", "LS", "RS", "rne", "rse",
	"SW", "NW", "UW", "LW", "LN", "RN", "rsw", "rnw",
};

/** Write name of given Trackdir. */
void WriteValueStr(Trackdir td, FILE *f)
{
	fprintf (f, "%d (%s)", td, ItemAtT(td, trackdir_names, "UNK", INVALID_TRACKDIR, "INV"));
}

/** Write composed name of given TrackdirBits. */
void WriteValueStr(TrackdirBits td_bits, FILE *f)
{
	fprintf (f, "%d (", td_bits);
	ComposeNameT (f, td_bits, trackdir_names, "UNK", INVALID_TRACKDIR_BIT, "INV");
	putc (')', f);
}


/** DiagDirection short names. */
static const char * const diagdir_names[] = {
	"NE", "SE", "SW", "NW",
};

/** Write name of given DiagDirection. */
void WriteValueStr(DiagDirection dd, FILE *f)
{
	fprintf (f, "%d (%s)", dd, ItemAtT(dd, diagdir_names, "UNK", INVALID_DIAGDIR, "INV"));
}


/** SignalType short names. */
static const char * const signal_type_names[] = {
	"NORMAL", "ENTRY", "EXIT", "COMBO", "PBS", "NOENTRY",
};

/** Write name of given SignalType. */
void WriteValueStr(SignalType t, FILE *f)
{
	fprintf (f, "%d (%s)", t, ItemAtT(t, signal_type_names, "UNK"));
}


/**
 * Keep track of the last assigned type_id. Used for anti-recursion.
 *static*/ size_t& DumpTarget::LastTypeId()
{
	static size_t last_type_id = 0;
	return last_type_id;
}

/** Return structured name of the current class/structure. */
std::string DumpTarget::GetCurrentStructName()
{
	std::string out;
	if (!m_cur_struct.empty()) {
		/* we are inside some named struct, return its name */
		out = m_cur_struct.top();
	}
	return out;
}

/**
 * Find the given instance in our anti-recursion repository.
 * Return a pointer to the name if found, else NULL.
 */
const std::string *DumpTarget::FindKnownName(size_t type_id, const void *ptr)
{
	KNOWN_NAMES::const_iterator it = m_known_names.find(KnownStructKey(type_id, ptr));
	return it != m_known_names.end() ? &it->second : NULL;
}

/** Write some leading spaces into the output. */
void DumpTarget::WriteIndent()
{
	int num_spaces = 2 * m_indent;
	while (num_spaces > 0) {
		putc (' ', f);
		num_spaces--;
	}
}

/** Write a line with indent at the beginning and <LF> at the end. */
void DumpTarget::WriteLine(const char *format, ...)
{
	WriteIndent();
	va_list args;
	va_start(args, format);
	vfprintf (f, format, args);
	va_end(args);
	putc ('\n', f);
}

/** Write 'name = ' with indent. */
void DumpTarget::WriteValue(const char *name)
{
	WriteIndent();
	fprintf (f, "%s = ", name);
}

/** Write name & TileIndex to the output. */
void DumpTarget::WriteTile(const char *name, TileIndex tile)
{
	WriteIndent();
	fputs (name, f);
	if (tile == INVALID_TILE) {
		fputs (" = INVALID_TILE\n", f);
	} else {
		fprintf (f, " = 0x%04X (%d, %d)\n", tile, TileX(tile), TileY(tile));
	}
}

/**
 * Open new structure (one level deeper than the current one) 'name = {<LF>'.
 */
void DumpTarget::BeginStruct(size_t type_id, const char *name, const void *ptr)
{
	/* make composite name */
	std::string cur_name = GetCurrentStructName();
	if (cur_name.size() > 0) {
		/* add name delimiter (we use structured names) */
		cur_name.append(".");
	}
	cur_name.append(name);

	/* put the name onto stack (as current struct name) */
	m_cur_struct.push(cur_name);

	/* put it also to the map of known structures */
	m_known_names.insert(KNOWN_NAMES::value_type(KnownStructKey(type_id, ptr), cur_name));

	WriteIndent();
	fprintf (f, "%s = {\n", name);
	m_indent++;
}

/**
 * Close structure '}<LF>'.
 */
void DumpTarget::EndStruct()
{
	m_indent--;
	WriteIndent();
	fputs ("}\n", f);

	/* remove current struct name from the stack */
	m_cur_struct.pop();
}
