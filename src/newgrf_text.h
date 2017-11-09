/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_text.h Header of Action 04 "universal holder" structure and functions */

#ifndef NEWGRF_TEXT_H
#define NEWGRF_TEXT_H

#include <map>

#include "string.h"
#include "strings_type.h"
#include "core/flexarray.h"
#include "core/smallvec_type.hpp"
#include "table/control_codes.h"

/** Skip the NFO unicode string marker, if present. */
static inline bool skip_nfo_utf8_identifier (const char **str)
{
	/* This character, the thorn, indicates a unicode string to NFO. */
	if ((*str)[0] != '\xC3' || (*str)[1] != '\x9E') return false;
	*str += 2;
	return true;
}


/**
 * Element of the linked list.
 * Each of those elements represent the string,
 * but according to a different lang.
 */
struct GRFText : FlexArray<char> {
public:
	const size_t len; ///< The length of the stored string, used for copying.
	char text[];      ///< The actual (translated) text.

private:
	/**
	 * Actually construct the GRFText.
	 * @param text The text to store in this GRFText.
	 * @param len  The length of the text to store.
	 */
	GRFText (const char *text, size_t len) : len (len)
	{
		/* We need to use memcpy instead of strcpy due to
		 * the possibility of "choice lists" and therefore
		 * intermediate string terminators. */
		memcpy (this->text, text, len);
	}

	/**
	 * Allocate and assign a new GRFText with the given text.
	 * As these strings can have string terminations in them, e.g.
	 * due to "choice lists" we (sometimes) cannot rely on detecting
	 * the length by means of strlen. Also, if the length is already
	 * known not scanning the whole string is more efficient.
	 * @param text   The text to store in the new GRFText.
	 * @param len    The length of the text.
	 */
	static GRFText *create (const char *text, size_t len)
	{
		return new (len) GRFText (text, len);
	}

public:
	static GRFText *create (const char *text, uint32 grfid, byte langid,
				bool allow_newlines);

	/** Create a GRFText for a given string. */
	static GRFText *create (const char *text)
	{
		return create (text, strlen(text) + 1);
	}

	/**
	 * Create a copy of this GRFText.
	 * @return an exact copy of the given text.
	 */
	GRFText *clone (void) const
	{
		return GRFText::create (this->text, this->len);
	}
};


StringID AddGRFString(uint32 grfid, uint16 stringid, byte langid, bool new_scheme, bool allow_newlines, const char *text_to_add, StringID def_string);
StringID GetGRFStringID(uint32 grfid, StringID stringid);
const char *GetGRFStringPtr(uint16 stringid);
void CleanUpStrings();
void SetCurrentGrfLangID(byte language_id);

char *TranslateTTDPatchCodes (uint32 grfid, uint8 language_id,
	bool allow_newlines, const char *str,
	StringControlCode byte80 = SCC_NEWGRF_PRINT_WORD_STRING_ID);

bool CheckGrfLangID(byte lang_id, byte grf_version);

void StartTextRefStackUsage(const GRFFile *grffile, byte numEntries, const uint32 *values = NULL);
void StopTextRefStackUsage();
void RewindTextRefStack();
bool UsingNewGRFTextStack();
struct TextRefStack *CreateTextRefStackBackup();
void RestoreTextRefStackBackup(struct TextRefStack *backup);
uint RemapNewGRFStringControlCode (uint scc, stringb *buf, const char **str, int64 *argv, uint argv_size, bool modify_argv);

/** Mapping of language data between a NewGRF and OpenTTD. */
struct LanguageMap {
	/** Mapping between NewGRF and OpenTTD IDs. */
	struct Mapping {
		byte newgrf_id;  ///< NewGRF's internal ID for a case/gender.
		byte openttd_id; ///< OpenTTD's internal ID for a case/gender.
	};

	/* We need a vector and can't use SmallMap due to the fact that for "setting" a
	 * gender of a string or requesting a case for a substring we want to map from
	 * the NewGRF's internal ID to OpenTTD's ID whereas for the choice lists we map
	 * the genders/cases/plural OpenTTD IDs to the NewGRF's internal IDs. In this
	 * case a NewGRF developer/translator might want a different translation for
	 * both cases. Thus we are basically implementing a multi-map. */
	SmallVector<Mapping, 1> gender_map; ///< Mapping of NewGRF and OpenTTD IDs for genders.
	SmallVector<Mapping, 1> case_map;   ///< Mapping of NewGRF and OpenTTD IDs for cases.
	int plural_form;                    ///< The plural form used for this language.

	int GetMapping(int newgrf_id, bool gender) const;
	int GetReverseMapping(int openttd_id, bool gender) const;
	static const LanguageMap *GetLanguageMap(uint32 grfid, uint8 language_id);
};

/** Map of GRFText objects by langid. */
struct GRFTextMap : private std::map <byte, GRFText *> {
	/** Default constructor. */
	GRFTextMap() : std::map <byte, GRFText *> ()
	{
	}

	GRFTextMap (const GRFTextMap &other);

	~GRFTextMap();

	const GRFText *get_current (void) const;

	/**
	 * Get a C-string from this GRFText-list. If there is a translation
	 * for the current language it is returned, otherwise the default
	 * translation is returned. If there is neither a default nor a
	 * translation for the current language NULL is returned.
	 */
	const char *get_string (void) const
	{
		const GRFText *t = this->get_current();
		return (t != NULL) ? t->text : NULL;
	}

	void add (byte langid, GRFText *text);
	void add (byte langid, uint32 grfid, bool allow_newlines, const char *text);

	/**
	 * Add a GRFText to this list. The text should not contain any
	 * text-codes. The text will be added as a 'default language'-text.
	 * @param text The text to add to the list.
	 */
	void add_default (const char *text)
	{
		this->add (0x7F, GRFText::create (text));
	}
};

#endif /* NEWGRF_TEXT_H */
