/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file extmidi.cpp Playing music via an external player. */

#include "../stdafx.h"
#include "../debug.h"
#include "../string.h"
#include "../core/alloc_func.hpp"
#include "../sound/sound_driver.hpp"
#include "../video/video_driver.hpp"
#include "../gfx_func.h"
#include "extmidi.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef EXTERNAL_PLAYER
/** The default external midi player. */
#define EXTERNAL_PLAYER "timidity"
#endif

/** Factory for the midi player that uses external players. */
static MusicDriverFactory <MusicDriver_ExtMidi>
		iFMusicDriver_ExtMidi (3, "extmidi", "External MIDI Driver");

/** Song currently playing. */
static char extmidi_song[MAX_PATH];

/** extmidi process parameters. */
static char **extmidi_params;

/** Pid of the running extmidi process. */
static pid_t extmidi_pid = -1;

const char *MusicDriver_ExtMidi::Start(const char * const * parm)
{
	if (strcmp (VideoDriver::GetActiveDriverName(), "allegro") == 0 ||
			strcmp (SoundDriver::GetActiveDriverName(), "allegro") == 0) {
		return "the extmidi driver does not work when Allegro is loaded.";
	}

	const char *command = GetDriverParam(parm, "cmd");
#ifndef MIDI_ARG
	if (StrEmpty(command)) command = EXTERNAL_PLAYER;
#else
	if (StrEmpty(command)) command = EXTERNAL_PLAYER " " MIDI_ARG;
#endif

	/* Count number of arguments, but include 3 extra slots: 1st for command, 2nd for song title, and 3rd for terminating NULL. */
	uint num_args = 3;
	for (const char *t = command; *t != '\0'; t++) if (*t == ' ') num_args++;

	extmidi_params = xcalloct<char *>(num_args);
	extmidi_params[0] = xstrdup(command);

	/* Replace space with \0 and add next arg to params */
	uint p = 1;
	while (true) {
		extmidi_params[p] = strchr(extmidi_params[p - 1], ' ');
		if (extmidi_params[p] == NULL) break;

		extmidi_params[p][0] = '\0';
		extmidi_params[p]++;
		p++;
	}

	/* Last parameter is the song file. */
	extmidi_params[p] = extmidi_song;

	return NULL;
}

static void DoStop (void)
{
	if (extmidi_pid <= 0) return;

	/* First try to gracefully stop for about five seconds;
	 * 5 seconds = 5000 milliseconds, 10 ms per cycle => 500 cycles. */
	for (int i = 0; i < 500; i++) {
		kill (extmidi_pid, SIGTERM);
		if (waitpid (extmidi_pid, NULL, WNOHANG) == extmidi_pid) {
			/* It has shut down, so we are done */
			extmidi_pid = -1;
			return;
		}
		/* Wait 10 milliseconds. */
		CSleep (10);
	}

	DEBUG(driver, 0, "extmidi: gracefully stopping failed, trying the hard way");
	/* Gracefully stopping failed. Do it the hard way
	 * and wait till the process finally died. */
	kill (extmidi_pid, SIGKILL);
	waitpid (extmidi_pid, NULL, 0);
	extmidi_pid = -1;
}

void MusicDriver_ExtMidi::Stop()
{
	free (extmidi_params[0]);
	free (extmidi_params);
	extmidi_song[0] = '\0';
	DoStop();
}

void MusicDriver_ExtMidi::PlaySong(const char *filename)
{
	bstrcpy (extmidi_song, filename);
	DoStop();
}

void MusicDriver_ExtMidi::StopSong()
{
	extmidi_song[0] = '\0';
	DoStop();
}

bool MusicDriver_ExtMidi::IsSongPlaying()
{
	if (extmidi_pid != -1) {
		if (waitpid (extmidi_pid, NULL, WNOHANG) == extmidi_pid) {
			extmidi_pid = -1;
		} else {
			return true;
		}
	}

	if (extmidi_song[0] == '\0') return false;

	extmidi_pid = fork();
	switch (extmidi_pid) {
		case 0: {
			close(0);
			int d = open("/dev/null", O_RDONLY);
			if (d != -1 && dup2(d, 1) != -1 && dup2(d, 2) != -1) {
				execvp (extmidi_params[0], extmidi_params);
			}
			_exit(1);
		}

		case -1:
			DEBUG(driver, 0, "extmidi: couldn't fork: %s", strerror(errno));
			/* FALL THROUGH */

		default:
			extmidi_song[0] = '\0';
			break;
	}

	return extmidi_pid != -1;
}

void MusicDriver_ExtMidi::SetVolume(byte vol)
{
	DEBUG(driver, 1, "extmidi: set volume not implemented");
}
