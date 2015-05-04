/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file game_text.cpp Implementation of handling translated strings. */

#include "../stdafx.h"
#include "../strgen/strgen.h"
#include "../string.h"
#include "../debug.h"
#include "../fileio_func.h"
#include "../tar_type.h"
#include "../script/squirrel_class.hpp"
#include "../strings_func.h"
#include "game_text.hpp"
#include "game.hpp"
#include "game_info.hpp"

#include "table/strings.h"

#include <stdarg.h>

void CDECL strgen_warning(const char *s, ...)
{
	char buf[1024];
	va_list va;
	va_start(va, s);
	bstrvfmt (buf, s, va);
	va_end(va);
	DEBUG(script, 0, "%s:%d: warning: %s", _file, _cur_line, buf);
	_warnings++;
}

void CDECL strgen_error(const char *s, ...)
{
	char buf[1024];
	va_list va;
	va_start(va, s);
	bstrvfmt (buf, s, va);
	va_end(va);
	DEBUG(script, 0, "%s:%d: error: %s", _file, _cur_line, buf);
	_errors++;
}

void NORETURN CDECL strgen_fatal(const char *s, ...)
{
	char buf[1024];
	va_list va;
	va_start(va, s);
	bstrvfmt (buf, s, va);
	va_end(va);
	DEBUG(script, 0, "%s:%d: FATAL: %s", _file, _cur_line, buf);
	throw std::exception();
}


/**
 * Read all the raw language strings from the given file.
 * @param file The file to read from.
 * @return The raw strings, or NULL upon error.
 */
LanguageStrings *ReadRawLanguageStrings(const char *file)
{
	LanguageStrings *ret = NULL;
	FILE *fh = NULL;
	try {
		size_t to_read;
		fh = FioFOpenFile(file, "rb", GAME_DIR, &to_read);
		if (fh == NULL) {
			return NULL;
		}

		const char *langname = strrchr(file, PATHSEPCHAR);
		if (langname == NULL) {
			langname = file;
		} else {
			langname++;
		}

		/* Check for invalid empty filename */
		if (*langname == '.' || *langname == 0) {
			fclose(fh);
			return NULL;
		}

		const char *dot = strchr (langname, '.');
		ret = new LanguageStrings ((dot == NULL) ? xstrdup (langname) :
				xstrmemdup (langname, dot - langname));

		char buffer[2048];
		while (to_read != 0 && fgets(buffer, sizeof(buffer), fh) != NULL) {
			size_t len = strlen(buffer);

			/* Remove trailing spaces/newlines from the string. */
			size_t i = len;
			while (i > 0 && (buffer[i - 1] == '\r' || buffer[i - 1] == '\n' || buffer[i - 1] == ' ')) i--;
			buffer[i] = '\0';

			ret->raw.append (xstrndup (buffer, to_read));

			if (len > to_read) {
				to_read = 0;
			} else {
				to_read -= len;
			}
		}

		fclose(fh);
		return ret;
	} catch (...) {
		if (fh != NULL) fclose(fh);
		delete ret;
		return NULL;
	}
}


/** A reader that simply reads using fopen. */
struct StringListReader : StringReader {
	LanguageStrings::StringVector::const_iterator iter; ///< The current location of the iteration.
	LanguageStrings::StringVector::const_iterator end;  ///< The end of the iteration.

	/**
	 * Create the reader.
	 * @param data        The data to fill during reading.
	 * @param file        The file we are reading.
	 * @param master      Are we reading the master file?
	 * @param translation Are we reading a translation?
	 */
	StringListReader(StringData &data, const LanguageStrings *strings, bool master, bool translation) :
			StringReader(data, strings->language.get(), master, translation),
			iter(strings->raw.begin()), end(strings->raw.end())
	{
	}

	/* virtual */ char *ReadLine(char *buffer, size_t size)
	{
		if (this->iter == this->end) return NULL;

		strncpy (buffer, this->iter->get(), size);
		this->iter++;

		return buffer;
	}
};

/** Class for writing an encoded language. */
struct TranslationWriter : LanguageWriter {
	LanguageStrings::StringVector *strings; ///< The encoded strings.

	/**
	 * Writer for the encoded data.
	 * @param strings The string table to add the strings to.
	 */
	TranslationWriter (LanguageStrings::StringVector *strings) : strings(strings)
	{
	}

	void WriteHeader(const LanguagePackHeader *header)
	{
		/* We don't use the header. */
	}

	void WriteNullString (void) OVERRIDE
	{
	}

	void WriteString (size_t length, const byte *buffer) OVERRIDE
	{
		this->strings->append (xstrmemdup ((const char*) buffer, length));
	}
};

/** Class for writing the string IDs. */
struct StringNameWriter : HeaderWriter {
	GameStrings::StringVector *strings; ///< The string names.

	/**
	 * Writer for the string names.
	 * @param strings The string table to add the strings to.
	 */
	StringNameWriter (GameStrings::StringVector *strings) : strings(strings)
	{
	}

	void WriteStringID(const char *name, int stringid)
	{
		if (stringid == (int)this->strings->size()) {
			this->strings->append (xstrdup (name));
		}
	}
};

/**
 * Scanner to find language files in a GameScript directory.
 */
class LanguageScanner : protected FileScanner {
private:
	GameStrings *gs;
	char *exclude;

public:
	/** Initialise */
	LanguageScanner(GameStrings *gs, const char *exclude) : gs(gs), exclude(xstrdup(exclude)) {}
	~LanguageScanner() { free(exclude); }

	/**
	 * Scan.
	 */
	void Scan (const char *directory, const char *dirend)
	{
		this->FileScanner::Scan (".txt", directory, dirend, false);
	}

	/* virtual */ bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename)
	{
		if (strcmp(filename, exclude) == 0) return true;

		gs->strings.append (ReadRawLanguageStrings (filename));
		return true;
	}
};

/**
 * Load all translations that we know of.
 * @return Container with all (compiled) translations.
 */
GameStrings *LoadTranslations()
{
	const GameInfo *info = Game::GetInfo();
	const char *script = info->GetMainScript();
	const char *e = strrchr (script, PATHSEPCHAR);
	if (e == NULL) return NULL;
	size_t base_length = e - script + 1; // point after the path separator

	sstring<512> filename;
	filename.fmt ("%.*s" "lang" PATHSEP, (int)base_length, script);
	base_length = filename.length();
	filename.append ("english.txt");
	if (!FioCheckFileExists (filename.c_str(), GAME_DIR)) return NULL;

	GameStrings *gs = new GameStrings();
	try {
		gs->strings.append (ReadRawLanguageStrings (filename.c_str()));

		const char *tar_filename = info->GetTarFile();
		TarList::iterator iter;
		if (tar_filename != NULL && (iter = TarCache::cache[GAME_DIR].tars.find(tar_filename)) != TarCache::cache[GAME_DIR].tars.end()) {
			/* The main script is in a tar file, so find all files
			 * that are in the same tar and add them to gs. */
			TarFileList::iterator tar;
			FOR_ALL_TARS(tar, GAME_DIR) {
				/* Not in the same tar. */
				if (tar->second.tar_filename != iter->first) continue;

				/* Check the path and extension. */
				if (tar->first.size() <= base_length) continue;
				if (tar->first.compare(0, base_length, filename.c_str(), base_length) != 0) continue;
				if (tar->first.size() < 4) continue;
				if (tar->first.compare(tar->first.size() - 4, 4, ".txt") != 0) continue;
				/* Exclude master file. */
				if (tar->first.compare(base_length, std::string::npos, filename.c_str() + base_length) == 0) continue;

				gs->strings.append (ReadRawLanguageStrings (tar->first.c_str()));
			}
		} else {
			/* Scan filesystem for other language files. */
			const char *p = filename.c_str();
			LanguageScanner scanner (gs, p);
			scanner.Scan (p, p + base_length);
		}

		gs->Compile();
		return gs;
	} catch (...) {
		delete gs;
		return NULL;
	}
}

/** Compile the language. */
void GameStrings::Compile()
{
	StringData data(1);
	StringListReader master_reader (data, this->strings[0].get(), true, false);
	master_reader.ParseFile();
	if (_errors != 0) throw std::exception();

	this->version = data.Version();

	StringNameWriter id_writer(&this->string_names);
	id_writer.WriteHeader(data);

	for (LanguageVector::iterator iter = this->strings.begin(); iter != this->strings.end(); iter++) {
		LanguageStrings *ls = iter->get();

		data.FreeTranslation();
		StringListReader translation_reader (data, ls, false, strcmp (ls->language.get(), "english") != 0);
		translation_reader.ParseFile();
		if (_errors != 0) throw std::exception();

		TranslationWriter writer (&ls->compiled);
		writer.WriteLang(data);
	}
}

/** The currently loaded game strings. */
GameStrings *GameStrings::current = NULL;

/**
 * Get the string pointer of a particular game string.
 * @param id The ID of the game string.
 * @return The encoded string.
 */
const char *GetGameStringPtr(uint id)
{
	if (id >= GameStrings::current->cur_language->compiled.size()) return GetStringPtr(STR_UNDEFINED);
	return GameStrings::current->cur_language->compiled[id].get();
}

/**
 * Register the current translation to the Squirrel engine.
 * @param engine The engine to update/
 */
void RegisterGameTranslation(Squirrel *engine)
{
	delete GameStrings::current;
	GameStrings::current = LoadTranslations();
	if (GameStrings::current == NULL) return;

	HSQUIRRELVM vm = engine->GetVM();
	sq_pushroottable(vm);
	sq_pushstring(vm, "GSText", -1);
	if (SQ_FAILED(sq_get(vm, -2))) return;

	int idx = 0;
	for (GameStrings::StringVector::iterator iter = GameStrings::current->string_names.begin(); iter != GameStrings::current->string_names.end(); iter++, idx++) {
		sq_pushstring(vm, iter->get(), -1);
		sq_pushinteger(vm, idx);
		sq_rawset(vm, -3);
	}

	sq_pop(vm, 2);

	ReconsiderGameScriptLanguage();
}

/**
 * Reconsider the game script language, so we use the right one.
 */
void ReconsiderGameScriptLanguage()
{
	if (GameStrings::current == NULL) return;

	const char *language = strrchr (_current_language->file, PATHSEPCHAR);
	assert (language != NULL);
	language++;

	const char *dot = strrchr (language, '.');
	assert (dot != NULL);
	size_t len = dot - language;

	for (GameStrings::LanguageVector::iterator iter = GameStrings::current->strings.begin(); iter != GameStrings::current->strings.end(); iter++) {
		LanguageStrings *ls = iter->get();
		if (memcmp (ls->language.get(), language, len) == 0
				&& ls->language.get()[len] == '\0') {
			GameStrings::current->cur_language = ls;
			return;
		}
	}

	GameStrings::current->cur_language = GameStrings::current->strings[0].get();
}
