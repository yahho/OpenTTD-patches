/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ini_load.cpp Definition of the #IniLoadFile class, related to reading and storing '*.ini' files. */

#include "stdafx.h"

#include "core/alloc_func.hpp"
#include "core/mem_func.hpp"
#include "ini_type.h"
#include "string.h"


/** IniName constructor helper (see IniName::IniName below). */
static inline char *ini_name_helper (const char *name, size_t len)
{
	if (len == 0) len = strlen (name);

	char *s = xstrndup (name, len);
	str_validate (s, s + len);
	return s;
}

/**
 * IniName constructor.
 * @param name the name of the item
 * @param len  the length of the name of the item, or 0 to use the full name length
 */
IniName::IniName (const char *name, size_t len)
	: name (ini_name_helper (name, len))
{
}


/**
 * Construct a new in-memory item of an Ini file.
 * @param name   the name of the item
 * @param len    the length of the name of the item
 */
IniItem::IniItem (const char *name, size_t len)
	: ForwardListLink<IniItem>(), IniName (name, len),
		value(NULL), comment(NULL)
{
}

/** Free everything we loaded. */
IniItem::~IniItem()
{
	free(this->value);
	free(this->comment);

	delete this->next;
}

/**
 * Replace the current value with another value.
 * @param value the value to replace with.
 */
void IniItem::SetValue(const char *value)
{
	free(this->value);
	this->value = xstrdup(value);
}

/**
 * Construct a new in-memory group of an Ini file.
 * @param parent the file we belong to
 * @param name   the name of the group
 * @param len    the length of the name of the group
 */
IniGroup::IniGroup (IniGroupType type, const char *name, size_t len)
	: ForwardListLink<IniGroup>(), IniName (name, len),
		IniList<IniItem>(), type(type), comment(NULL)
{
}

/** Free everything we loaded. */
IniGroup::~IniGroup()
{
	free(this->comment);

	delete this->next;
}

/**
 * Get the item with the given name, and create it if it doesn't exist
 * @param name name of the item to find.
 * @return the requested item
 */
IniItem *IniGroup::get_item (const char *name)
{
	IniItem *item = this->find (name);
	if (item != NULL) return item;

	/* otherwise make a new one */
	return this->append (name);
}


/**
 * Construct a new in-memory Ini file representation.
 * @param list_group_names A \c NULL terminated list with group names that should be loaded as lists instead of variables. @see IGT_LIST
 * @param seq_group_names  A \c NULL terminated list with group names that should be loaded as lists of names. @see IGT_SEQUENCE
 */
IniLoadFile::IniLoadFile(const char * const *list_group_names, const char * const *seq_group_names) :
		IniList<IniGroup>(),
		comment(NULL),
		list_group_names(list_group_names),
		seq_group_names(seq_group_names)
{
}

/** Free everything we loaded. */
IniLoadFile::~IniLoadFile()
{
	free(this->comment);
}

/**
 * Get the type a group in this file is.
 * @param name the name of the group
 * @param len use only this many chars of name
 * @return the type of the group by that name
 */
IniGroupType IniLoadFile::get_group_type (const char *name, size_t len) const
{
	if (this->list_group_names != NULL) {
		for (const char *const *p = this->list_group_names; *p != NULL; p++) {
			if (strncmp (*p, name, len) == 0 && (*p)[len] == 0) return IGT_LIST;
		}
	}

	if (this->seq_group_names != NULL) {
		for (const char *const *p = this->seq_group_names; *p != NULL; p++) {
			if (strncmp (*p, name, len) == 0 && (*p)[len] == 0) return IGT_SEQUENCE;
		}
	}

	return IGT_VARIABLES;
}

/**
 * Get the group with the given name. If it doesn't exist, create a new group.
 * @param name name of the group to find.
 * @param len  the maximum length of said name (\c 0 means length of the string).
 * @return The requested group.
 */
IniGroup *IniLoadFile::get_group (const char *name, size_t len)
{
	if (len == 0) len = strlen(name);

	/* does it exist already? */
	IniGroup *group = this->find (name, len);
	if (group != NULL) return group;

	/* otherwise make a new one */
	group = this->append (name, len);
	group->comment = xstrdup("\n");
	return group;
}

/**
 * Load the Ini file's data from the disk.
 * @param filename the file to load.
 * @param subdir the sub directory to load the file from.
 * @pre nothing has been loaded yet.
 */
void IniLoadFile::LoadFromDisk(const char *filename, Subdirectory subdir)
{
	assert (this->begin() == this->end());

	char buffer[1024];
	IniGroup *group = NULL;

	char *comment = NULL;
	uint comment_size = 0;
	uint comment_alloc = 0;

	size_t end;
	FILE *in = this->OpenFile(filename, subdir, &end);
	if (in == NULL) return;

	end += ftell(in);

	/* for each line in the file */
	while ((size_t)ftell(in) < end && fgets(buffer, sizeof(buffer), in)) {
		char c, *s;
		/* trim whitespace from the left side */
		for (s = buffer; *s == ' ' || *s == '\t'; s++) {}

		/* trim whitespace from right side. */
		char *e = s + strlen(s);
		while (e > s && ((c = e[-1]) == '\n' || c == '\r' || c == ' ' || c == '\t')) e--;
		*e = '\0';

		/* Skip comments and empty lines outside IGT_SEQUENCE groups. */
		if ((group == NULL || group->type != IGT_SEQUENCE) && (*s == '#' || *s == ';' || *s == '\0')) {
			uint ns = comment_size + (e - s + 1);
			uint a = comment_alloc;
			/* add to comment */
			if (ns > a) {
				a = max(a, 128U);
				do a *= 2; while (a < ns);
				comment = xrealloct (comment, comment_alloc = a);
			}
			uint pos = comment_size;
			comment_size += (e - s + 1);
			comment[pos + e - s] = '\n'; // comment newline
			memcpy(comment + pos, s, e - s); // copy comment contents
			continue;
		}

		/* it's a group? */
		if (s[0] == '[') {
			if (e[-1] != ']') {
				this->ReportFileError("ini: invalid group name '", buffer, "'");
			} else {
				e--;
			}
			s++; // skip [
			group = this->append (s, e - s);
			if (comment_size != 0) {
				group->comment = xstrndup(comment, comment_size);
				comment_size = 0;
			}
		} else if (group != NULL) {
			if (group->type == IGT_SEQUENCE) {
				/* A sequence group, use the line as item name without further interpretation. */
				IniItem *item = group->append (buffer, e - buffer);
				if (comment_size) {
					item->comment = xstrndup(comment, comment_size);
					comment_size = 0;
				}
				continue;
			}
			char *t;
			/* find end of keyname */
			if (*s == '\"') {
				s++;
				for (t = s; *t != '\0' && *t != '\"'; t++) {}
				if (*t == '\"') *t = ' ';
			} else {
				for (t = s; *t != '\0' && *t != '=' && *t != '\t' && *t != ' '; t++) {}
			}

			/* it's an item in an existing group */
			IniItem *item = group->append (s, t - s);
			if (comment_size != 0) {
				item->comment = xstrndup(comment, comment_size);
				comment_size = 0;
			}

			/* find start of parameter */
			while (*t == '=' || *t == ' ' || *t == '\t') t++;

			bool quoted = (*t == '\"');
			/* remove starting quotation marks */
			if (*t == '\"') t++;
			/* remove ending quotation marks */
			e = t + strlen(t);
			if (e > t && e[-1] == '\"') e--;
			*e = '\0';

			/* If the value was not quoted and empty, it must be NULL */
			item->value = (!quoted && e == t) ? NULL : xstrndup(t, e - t);
			if (item->value != NULL) str_validate(item->value, item->value + strlen(item->value));
		} else {
			/* it's an orphan item */
			this->ReportFileError("ini: '", buffer, "' outside of group");
		}
	}

	if (comment_size > 0) {
		this->comment = xstrndup(comment, comment_size);
		comment_size = 0;
	}

	free(comment);
	fclose(in);
}

