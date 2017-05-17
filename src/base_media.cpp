/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file base_media.cpp Generic functions for replacing base data (graphics, sounds). */

#include "stdafx.h"

#include <set>

#include "debug.h"
#include "base_media_base.h"

/** Set of isocodes. */
typedef std::set <const char *, StringCompare> StringSet;

/** Keep a set of all isocodes seen, instead of duplicating each one. */
static StringSet isocodes;

/** Get the reference pointer for an isocode. */
static const char *register_isocode (const char *isocode)
{
	std::pair <StringSet::iterator, StringSet::iterator> pair =
			isocodes.equal_range (isocode);

	if (pair.first != pair.second) return *pair.first;

	/* Not found. */
	const char *p = xstrdup (isocode);
	isocodes.insert (pair.first, p);
	return p;
}

/**
 * Add a description of this set for a given language.
 * @param isocode The isocode of the language.
 * @param desc The description for that language.
 */
void BaseSetDesc::add_desc (const char *isocode, const char *desc)
{
	this->description[register_isocode(isocode)].reset (xstrdup (desc));
}

/**
 * Get the description of this set for the given ISO code.
 * It falls back to the first two characters of the ISO code in case
 * no match could be made with the full ISO code. If even then the
 * matching fails the default is returned.
 * @param isocode The isocode to search for.
 * @return The description.
 */
const char *BaseSetDesc::get_desc (const char *isocode) const
{
	assert (isocode != NULL);

	/* First the full ISO code */
	StringMap::const_iterator iter = this->description.find (isocode);
	if (iter != this->description.end()) return iter->second.get();

	/* Then the first two characters */
	const char alt [3] = { isocode[0], isocode[1], '\0' };
	iter = this->description.lower_bound (alt);
	if (iter != this->description.end() && strncmp (iter->first, isocode, 2) == 0) {
		return iter->second.get();
	}

	/* Then fall back */
	return this->get_default_desc();
}

/**
 * Try to read a single piece of metadata from an ini file.
 * @param metadata The metadata group to search in.
 * @param name The name of the item to read.
 * @param type The type of the set for debugging output.
 * @param filename The name of the filename for debugging output.
 * @return The associated item, or NULL if it doesn't exist.
 */
const IniItem *BaseSetDesc::fetch_metadata (const IniGroup *metadata,
	const char *name, const char *type, const char *filename)
{
	const IniItem *item = metadata->find (name);
	if (item == NULL || StrEmpty (item->value)) {
		DEBUG (grf, 0, "Base %sset detail loading: %s field missing in %s.",
				type, name, filename);
		return NULL;
	}
	return item;
}
