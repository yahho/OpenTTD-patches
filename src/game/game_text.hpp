/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file game_text.hpp Base functions regarding game texts. */

#ifndef GAME_TEXT_HPP
#define GAME_TEXT_HPP

#include <vector>
#include "../core/pointer.h"
#include "../core/smallvec_type.hpp"

/** The tab we place our strings in. */
static const uint GAME_TEXT_TAB = 18;

const char *GetGameStringPtr(uint id);
void RegisterGameTranslation(class Squirrel *engine);
void ReconsiderGameScriptLanguage();

/** Container for the raw (unencoded) language strings of a language. */
struct LanguageStrings {
	const char *language; ///< Name of the language (base filename).
	StringList raw;       ///< The raw strings of the language.
	StringList compiled;  ///< The compiled strings of the language.

	LanguageStrings(const char *language, const char *end = NULL);
	~LanguageStrings();
};

/** Container for all the game strings. */
struct GameStrings {
	uint version;                  ///< The version of the language strings.
	LanguageStrings *cur_language; ///< The current (compiled) language.

	/** Helper class to store a vector of LanguageStrings. */
	struct LanguageVector : std::vector <ttd_unique_ptr <LanguageStrings> > {
		/** Append a newly constructed LanguageStrings to the vector. */
		void append (LanguageStrings *ls)
		{
			this->push_back (ttd_unique_ptr <LanguageStrings> (ls));
		}
	};

	LanguageVector strings;    ///< The strings per language, first is master.

	/** Helper class to store a vector of unique strings. */
	struct StringVector : std::vector <ttd_unique_free_ptr <char> > {
		/** Append a newly allocated string to the vector. */
		void append (char *s)
		{
			this->push_back (ttd_unique_free_ptr<char> (s));
		}
	};

	StringVector string_names; ///< The names of the compiled strings.

	void Compile();

	static GameStrings *current;
};

#endif /* GAME_TEXT_HPP */
