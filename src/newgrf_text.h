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

#include "string.h"
#include "strings_type.h"
#include "core/smallvec_type.hpp"
#include "misc/countedptr.hpp"
#include "table/control_codes.h"

/** This character, the thorn ('þ'), indicates a unicode string to NFO. */
static const WChar NFO_UTF8_IDENTIFIER = 0x00DE;


/**
 * Element of the linked list.
 * Each of those elements represent the string,
 * but according to a different lang.
 */
struct GRFText {
public:
	/**
	 * Allocate and assign a new GRFText with the given text.
	 * As these strings can have string terminations in them, e.g.
	 * due to "choice lists" we (sometimes) cannot rely on detecting
	 * the length by means of strlen. Also, if the length is already
	 * known not scanning the whole string is more efficient.
	 * @param langid The language of the text.
	 * @param text   The text to store in the new GRFText.
	 * @param len    The length of the text.
	 */
	static GRFText *create (byte langid, const char *text, size_t len)
	{
		return new (len) GRFText (langid, text, len);
	}

	/** Create a GRFText for a given string. */
	static GRFText *create (byte langid, const char *text)
	{
		return create (langid, text, strlen(text) + 1);
	}

	/**
	 * Create a copy of this GRFText.
	 * @return an exact copy of the given text.
	 */
	GRFText *clone (void) const
	{
		return GRFText::create (this->langid, this->text, this->len);
	}

	/**
	 * Free the memory we allocated.
	 * @param p memory to free.
	 */
	void operator delete (void *p)
	{
		free (p);
	}

private:
	/**
	 * Actually construct the GRFText.
	 * @param langid_ The language of the text.
	 * @param text_   The text to store in this GRFText.
	 * @param len_    The length of the text to store.
	 */
	GRFText (byte langid_, const char *text_, size_t len_)
		: next(NULL), len(len_), langid(langid_)
	{
		/* We need to use memcpy instead of strcpy due to
		 * the possibility of "choice lists" and therefore
		 * intermediate string terminators. */
		memcpy (this->text, text_, len);
	}

	/**
	 * Allocate memory for this class.
	 * @param size the size of the instance
	 * @param extra the extra memory for the text
	 * @return the requested amount of memory for both the instance and the text
	 */
	void *operator new (size_t size, size_t extra)
	{
		return xmalloc (size + extra);
	}

	/**
	 * Helper allocation function to disallow something.
	 * Don't allow simple 'news'; they wouldn't have enough memory.
	 * @param size the amount of space not to allocate.
	 */
	void *operator new (size_t size) DELETED;

public:
	GRFText *next; ///< The next GRFText in this chain.
	size_t len;    ///< The length of the stored string, used for copying.
	byte langid;   ///< The language associated with this GRFText.
	char text[];   ///< The actual (translated) text.
};


StringID AddGRFString(uint32 grfid, uint16 stringid, byte langid, bool new_scheme, bool allow_newlines, const char *text_to_add, StringID def_string);
StringID GetGRFStringID(uint32 grfid, uint16 stringid);
const char *GetGRFStringFromGRFText(const struct GRFText *text);
const char *GetGRFStringPtr(uint16 stringid);
void CleanUpStrings();
void SetCurrentGrfLangID(byte language_id);
char *TranslateTTDPatchCodes(uint32 grfid, uint8 language_id, bool allow_newlines, const char *str, int *olen = NULL, StringControlCode byte80 = SCC_NEWGRF_PRINT_WORD_STRING_ID);
struct GRFText *DuplicateGRFText(struct GRFText *orig);
void AddGRFTextToList(struct GRFText **list, struct GRFText *text_to_add);
void AddGRFTextToList(struct GRFText **list, byte langid, uint32 grfid, bool allow_newlines, const char *text_to_add);
void AddGRFTextToList(struct GRFText **list, const char *text_to_add);
void CleanUpGRFText(struct GRFText *grftext);

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

/** Reference counted wrapper around a GRFText pointer. */
struct GRFTextWrapper : public SimpleCountedObject {
	struct GRFText *text; ///< The actual text

	/** Create a new GRFTextWrapper. */
	GRFTextWrapper() : text(NULL)
	{
	}

	/** Cleanup a GRFTextWrapper object. */
	~GRFTextWrapper()
	{
		CleanUpGRFText (this->text);
	}
};

#endif /* NEWGRF_TEXT_H */
