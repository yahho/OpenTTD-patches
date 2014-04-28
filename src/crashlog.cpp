/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file crashlog.cpp Implementation of generic function to be called to log a crash */

#include "stdafx.h"
#include "crashlog.h"
#include "gamelog.h"
#include "date_func.h"
#include "rev.h"
#include "strings_func.h"
#include "blitter/factory.hpp"
#include "base_media_base.h"
#include "music/music_driver.hpp"
#include "sound/sound_driver.hpp"
#include "video/video_driver.hpp"
#include "saveload/saveload.h"
#include "screenshot.h"
#include "gfx_func.h"
#include "network/network.h"
#include "language.h"
#include "fontcache.h"

#include "ai/ai_info.hpp"
#include "game/game.hpp"
#include "game/game_info.hpp"
#include "company_base.h"
#include "company_func.h"

#include <time.h>

/* static */ const char *CrashLog::message = NULL;

/**
 * Writes compiler (and its version, if available) to the buffer.
 * @param buffer The string where to write.
 */
static void LogCompiler (stringb *buffer)
{
	buffer->append_fmt (" Compiler: "
#if defined(_MSC_VER)
			"MSVC %d", _MSC_VER
#elif defined(__ICC) && defined(__GNUC__)
			"ICC %d (GCC %d.%d.%d mode)", __ICC,  __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
#elif defined(__ICC)
			"ICC %d", __ICC
#elif defined(__GNUC__)
			"GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
#elif defined(__WATCOMC__)
			"WatcomC %d", __WATCOMC__
#else
			"<unknown>"
#endif
			);
#if defined(__VERSION__)
	buffer->append (" \"" __VERSION__ "\"\n\n");
#else
	buffer->append ("\n\n");
#endif
}

/* virtual */ void CrashLog::LogRegisters (stringb *buffer) const
{
	/* Stub implementation; not all OSes support this. */
}

/* virtual */ void CrashLog::LogModules (stringb *buffer) const
{
	/* Stub implementation; not all OSes support this. */
}

/**
 * Writes version and compilation information to the buffer.
 * @param buffer The string where to write.
 */
static void LogVersion (stringb *buffer)
{
	buffer->append_fmt (
			"Binary:\n"
			" Version:    %s (%d)\n"
			" NewGRF ver: %08x\n"
			" Build date: %s\n\n"
			" Flags:     "
#ifdef _SQ64
			" 64-bit"
#else
			" 32-bit"
#endif
#if (TTD_ENDIAN == TTD_LITTLE_ENDIAN)
			" little-endian"
#else
			" big-endian"
#endif
#ifdef DEDICATED
			" dedicated"
#endif
			,
			_openttd_revision, _openttd_revision_modified,
			_openttd_newgrf_version,
			_openttd_build_date
	);
}

/**
 * Writes the (important) configuration settings to the buffer.
 * E.g. graphics set, sound set, blitter and AIs.
 * @param buffer The string where to write.
 */
static void LogConfiguration (stringb *buffer)
{
	buffer->append_fmt (
			"Configuration:\n"
			" Blitter:      %s\n"
			" Graphics set: %s (%u)\n"
			" Language:     %s\n"
			" Music driver: %s\n"
			" Music set:    %s (%u)\n"
			" Network:      %s\n"
			" Sound driver: %s\n"
			" Sound set:    %s (%u)\n"
			" Video driver: %s\n\n",
			BlitterFactory::GetCurrentBlitter() == NULL ? "none" : BlitterFactory::GetCurrentBlitter()->GetName(),
			BaseGraphics::GetUsedSet() == NULL ? "none" : BaseGraphics::GetUsedSet()->name,
			BaseGraphics::GetUsedSet() == NULL ? UINT32_MAX : BaseGraphics::GetUsedSet()->version,
			_current_language == NULL ? "none" : _current_language->file,
			MusicDriver::GetInstance() == NULL ? "none" : MusicDriver::GetInstance()->GetName(),
			BaseMusic::GetUsedSet() == NULL ? "none" : BaseMusic::GetUsedSet()->name,
			BaseMusic::GetUsedSet() == NULL ? UINT32_MAX : BaseMusic::GetUsedSet()->version,
			_networking ? (_network_server ? "server" : "client") : "no",
			SoundDriver::GetInstance() == NULL ? "none" : SoundDriver::GetInstance()->GetName(),
			BaseSounds::GetUsedSet() == NULL ? "none" : BaseSounds::GetUsedSet()->name,
			BaseSounds::GetUsedSet() == NULL ? UINT32_MAX : BaseSounds::GetUsedSet()->version,
			VideoDriver::GetInstance() == NULL ? "none" : VideoDriver::GetInstance()->GetName()
	);

	buffer->append_fmt (
			"Fonts:\n"
			" Small:  %s\n"
			" Medium: %s\n"
			" Large:  %s\n"
			" Mono:   %s\n\n",
			FontCache::Get(FS_SMALL)->GetFontName(),
			FontCache::Get(FS_NORMAL)->GetFontName(),
			FontCache::Get(FS_LARGE)->GetFontName(),
			FontCache::Get(FS_MONO)->GetFontName()
	);

	buffer->append_fmt ("AI Configuration (local: %i):\n", (int)_local_company);
	const Company *c;
	FOR_ALL_COMPANIES(c) {
		if (c->ai_info == NULL) {
			buffer->append_fmt (" %2i: Human\n", (int)c->index);
		} else {
			buffer->append_fmt (" %2i: %s (v%d)\n", (int)c->index, c->ai_info->GetName(), c->ai_info->GetVersion());
		}
	}

	if (Game::GetInfo() != NULL) {
		buffer->append_fmt (" GS: %s (v%d)\n", Game::GetInfo()->GetName(), Game::GetInfo()->GetVersion());
	}
	buffer->append ('\n');
}

/* Include these here so it's close to where it's actually used. */
#ifdef WITH_ALLEGRO
#	include <allegro.h>
#endif /* WITH_ALLEGRO */
#ifdef WITH_FONTCONFIG
#	include <fontconfig/fontconfig.h>
#endif /* WITH_FONTCONFIG */
#ifdef WITH_PNG
	/* pngconf.h, included by png.h doesn't like something in the
	 * freetype headers. As such it's not alphabetically sorted. */
#	include <png.h>
#endif /* WITH_PNG */
#ifdef WITH_FREETYPE
#	include <ft2build.h>
#	include FT_FREETYPE_H
#endif /* WITH_FREETYPE */
#ifdef WITH_ICU
#	include <unicode/uversion.h>
#endif /* WITH_ICU */
#ifdef WITH_LZMA
#	include <lzma.h>
#endif
#ifdef WITH_LZO
#include <lzo/lzo1x.h>
#endif
#ifdef WITH_SDL
#	include "sdl.h"
#	include <SDL.h>
#endif /* WITH_SDL */
#ifdef WITH_ZLIB
# include <zlib.h>
#endif

/**
 * Writes information (versions) of the used libraries.
 * @param buffer The string where to write.
 */
static void LogLibraries (stringb *buffer)
{
	buffer->append ("Libraries:\n");

#ifdef WITH_ALLEGRO
	buffer->append_fmt (" Allegro:    %s\n", allegro_id);
#endif /* WITH_ALLEGRO */

#ifdef WITH_FONTCONFIG
	int version = FcGetVersion();
	buffer->append_fmt (" FontConfig: %d.%d.%d\n", version / 10000, (version / 100) % 100, version % 100);
#endif /* WITH_FONTCONFIG */

#ifdef WITH_FREETYPE
	FT_Library library;
	int major, minor, patch;
	FT_Init_FreeType(&library);
	FT_Library_Version(library, &major, &minor, &patch);
	FT_Done_FreeType(library);
	buffer->append_fmt (" FreeType:   %d.%d.%d\n", major, minor, patch);
#endif /* WITH_FREETYPE */

#ifdef WITH_ICU
	/* 4 times 0-255, separated by dots (.) and a trailing '\0' */
	char buf[4 * 3 + 3 + 1];
	UVersionInfo ver;
	u_getVersion(ver);
	u_versionToString(ver, buf);
	buffer->append_fmt (" ICU:        %s\n", buf);
#endif /* WITH_ICU */

#ifdef WITH_LZMA
	buffer->append_fmt (" LZMA:       %s\n", lzma_version_string());
#endif

#ifdef WITH_LZO
	buffer->append_fmt (" LZO:        %s\n", lzo_version_string());
#endif

#ifdef WITH_PNG
	buffer->append_fmt (" PNG:        %s\n", png_get_libpng_ver(NULL));
#endif /* WITH_PNG */

#ifdef WITH_SDL
#ifdef DYNAMICALLY_LOADED_SDL
	if (SDL_CALL SDL_Linked_Version != NULL) {
#else
	{
#endif
		const SDL_version *v = SDL_CALL SDL_Linked_Version();
		buffer->append_fmt (" SDL:        %d.%d.%d\n", v->major, v->minor, v->patch);
	}
#endif /* WITH_SDL */

#ifdef WITH_ZLIB
	buffer->append_fmt (" Zlib:       %s\n", zlibVersion());
#endif

	buffer->append ('\n');
}

/**
 * Helper function for printing the gamelog.
 * @param s the string to print.
 * @param buffer the buffer where to print.
 */
static void GamelogFillCrashLog (const char *s, void *buffer)
{
	((stringb*)buffer)->append_fmt ("%s\n", s);
}

/**
 * Fill the crash log buffer with all data of a crash log.
 * @param buffer The string where to write.
 */
void CrashLog::FillCrashLog (stringb *buffer) const
{
	time_t cur_time = time(NULL);
	buffer->append ("*** OpenTTD Crash Report ***\n\n");
	buffer->append_fmt ("Crash at: %s", asctime(gmtime(&cur_time)));

	YearMonthDay ymd;
	ConvertDateToYMD(_date, &ymd);
	buffer->append_fmt ("In game date: %i-%02i-%02i (%i)\n\n", ymd.year, ymd.month + 1, ymd.day, _date_fract);

	this->LogError(buffer, CrashLog::message);
	LogVersion(buffer);
	this->LogRegisters(buffer);
	this->LogStacktrace(buffer);
	this->LogOSVersion(buffer);
	LogCompiler(buffer);
	LogConfiguration(buffer);
	LogLibraries(buffer);
	this->LogModules(buffer);

	/* Write the gamelog data to the buffer. */
	GamelogPrint (&GamelogFillCrashLog, buffer);

	buffer->append ("\n*** End of OpenTTD Crash Report ***\n");
}

/**
 * Write the crash log to a file.
 * @note On success the filename will be filled with the full path of the
 *       crash log file. Make sure filename is at least \c MAX_PATH big.
 * @param buffer The begin of the buffer to write to the disk.
 * @param filename Output for the filename of the written file.
 * @return true when the crash log was successfully written.
 */
bool CrashLog::WriteCrashLog (const char *buffer, stringb *filename) const
{
	filename->fmt ("%scrash.log", _personal_dir);

	FILE *file = FioFOpenFile (filename->c_str(), "w", NO_DIRECTORY);
	if (file == NULL) return false;

	size_t len = strlen(buffer);
	size_t written = fwrite(buffer, 1, len, file);

	FioFCloseFile(file);
	return len == written;
}

/* virtual */ int CrashLog::WriteCrashDump (stringb *filename) const
{
	/* Stub implementation; not all OSes support this. */
	return 0;
}

/**
 * Write the (crash) savegame to a file.
 * @note On success the filename will be filled with the full path of the
 *       crash save file. Make sure filename is at least \c MAX_PATH big.
 * @param filename Output for the filename of the written file.
 * @return true when the crash save was successfully made.
 */
bool CrashLog::WriteSavegame (stringb *filename) const
{
	/* If the map array doesn't exist, saving will fail too. If the map got
	 * initialised, there is a big chance the rest is initialised too. */
	if (_mth == NULL) return false;

	try {
		GamelogEmergency();

		filename->fmt ("%scrash.sav", _personal_dir);

		/* Don't do a threaded saveload. */
		return SaveGame(filename->c_str(), NO_DIRECTORY, false);
	} catch (...) {
		return false;
	}
}

/**
 * Write the (crash) screenshot to a file.
 * @note On success the filename will be filled with the full path of the
 *       screenshot. Make sure filename is at least \c MAX_PATH big.
 * @param filename Output for the filename of the written file.
 * @return true when the crash screenshot was successfully made.
 */
bool CrashLog::WriteScreenshot (stringb *filename) const
{
	/* Don't draw when we have invalid screen size */
	if (_screen.width < 1 || _screen.height < 1 || _screen.dst_ptr == NULL) return false;

	bool res = MakeScreenshot(SC_CRASHLOG, "crash");
	if (res) filename->copy (_full_screenshot_name);
	return res;
}

/**
 * Makes the crash log, writes it to a file and then subsequently tries
 * to make a crash dump and crash savegame. It uses DEBUG to write
 * information like paths to the console.
 * @return true when everything is made successfully.
 */
bool CrashLog::MakeCrashLog() const
{
	/* Don't keep looping logging crashes. */
	static bool crashlogged = false;
	if (crashlogged) return false;
	crashlogged = true;

	sstring<MAX_PATH> filename;
	sstring<65536> buffer;
	bool ret = true;

	printf("Crash encountered, generating crash log...\n");
	this->FillCrashLog (&buffer);
	printf("%s\n", buffer.c_str());
	printf("Crash log generated.\n\n");

	printf("Writing crash log to disk...\n");
	bool bret = this->WriteCrashLog (buffer.c_str(), &filename);
	if (bret) {
		printf("Crash log written to %s. Please add this file to any bug reports.\n\n", filename.c_str());
	} else {
		printf("Writing crash log failed. Please attach the output above to any bug reports.\n\n");
		ret = false;
	}

	/* Don't mention writing crash dumps because not all platforms support it. */
	int dret = this->WriteCrashDump (&filename);
	if (dret < 0) {
		printf("Writing crash dump failed.\n\n");
		ret = false;
	} else if (dret > 0) {
		printf("Crash dump written to %s. Please add this file to any bug reports.\n\n", filename.c_str());
	}

	printf("Writing crash savegame...\n");
	bret = this->WriteSavegame (&filename);
	if (bret) {
		printf("Crash savegame written to %s. Please add this file and the last (auto)save to any bug reports.\n\n", filename.c_str());
	} else {
		ret = false;
		printf("Writing crash savegame failed. Please attach the last (auto)save to any bug reports.\n\n");
	}

	printf("Writing crash screenshot...\n");
	bret = this->WriteScreenshot (&filename);
	if (bret) {
		printf("Crash screenshot written to %s. Please add this file to any bug reports.\n\n", filename.c_str());
	} else {
		ret = false;
		printf("Writing crash screenshot failed.\n\n");
	}

	return ret;
}

/**
 * Sets a message for the error message handler.
 * @param message The error message of the error.
 */
/* static */ void CrashLog::SetErrorMessage(const char *message)
{
	CrashLog::message = message;
}

/**
 * Try to close the sound/video stuff so it doesn't keep lingering around
 * incorrect video states or so, e.g. keeping dpmi disabled.
 */
/* static */ void CrashLog::AfterCrashLogCleanup()
{
	if (MusicDriver::GetInstance() != NULL) MusicDriver::GetInstance()->Stop();
	if (SoundDriver::GetInstance() != NULL) SoundDriver::GetInstance()->Stop();
	if (VideoDriver::GetInstance() != NULL) VideoDriver::GetInstance()->Stop();
}
