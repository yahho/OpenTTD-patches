/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file crashlog.h Functions to be called to log a crash */

#ifndef CRASHLOG_H
#define CRASHLOG_H

#include "string.h"

/**
 * Helper class for creating crash logs.
 */
class CrashLog {
private:
	/** Pointer to the error message. */
	static const char *message;

	/** Temporary 'local' location of the buffer. */
	static stringb *gamelog_buffer;

	static void GamelogFillCrashLog(const char *s);
protected:
	/**
	 * Writes OS' version to the buffer.
	 * @param buffer The string where to write.
	 */
	virtual void LogOSVersion (stringb *buffer) const = 0;

	/**
	 * Writes actually encountered error to the buffer.
	 * @param buffer The string where to write.
	 * @param message Message passed to use for possible errors. Can be NULL.
	 */
	virtual void LogError (stringb *buffer, const char *message) const = 0;

	/**
	 * Writes the stack trace to the buffer, if there is information about it
	 * available.
	 * @param buffer The string where to write.
	 */
	virtual void LogStacktrace (stringb *buffer) const = 0;

	/**
	 * Writes information about the data in the registers, if there is
	 * information about it available.
	 * @param buffer The string where to write.
	 */
	virtual void LogRegisters (stringb *buffer) const;

	/**
	 * Writes the dynamically linked libraries/modules to the buffer, if there
	 * is information about it available.
	 * @param buffer The string where to write.
	 */
	virtual void LogModules (stringb *buffer) const;

public:
	/** Stub destructor to silence some compilers. */
	virtual ~CrashLog() {}

	void FillCrashLog (stringb *buffer) const;
	bool WriteCrashLog (const char *buffer, stringb *filename) const;

	/**
	 * Write the (crash) dump to a file.
	 * @note On success the filename will be filled with the full path of the
	 *       crash dump file. Make sure filename is at least \c MAX_PATH big.
	 * @param filename Output for the filename of the written file.
	 * @return if less than 0, error. If 0 no dump is made, otherwise the dump
	 *         was successful (not all OSes support dumping files).
	 */
	virtual int WriteCrashDump (stringb *filename) const;
	bool WriteSavegame (stringb *filename) const;
	bool WriteScreenshot (stringb *filename) const;

	bool MakeCrashLog() const;

	/**
	 * Initialiser for crash logs; do the appropriate things so crashes are
	 * handled by our crash handler instead of returning straight to the OS.
	 * @note must be implemented by all implementers of CrashLog.
	 */
	static void InitialiseCrashLog();

	static void SetErrorMessage(const char *message);
	static void AfterCrashLogCleanup();
};

#endif /* CRASHLOG_H */
