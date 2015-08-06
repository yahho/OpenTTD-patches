/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file script_mode.hpp Switch the script to Test or Execute Mode. */

#ifndef SCRIPT_MODE_HPP
#define SCRIPT_MODE_HPP

#include "script_object.hpp"

/**
 * Base class to switch current mode.
 * If you create an instance of this class, the mode will be switched.
 * @api none
 */
class BaseScriptMode : public ScriptObject {
private:
	const bool test; ///< This instance represents a switch to test mode.

public:
	/**
	 * Creating an instance of this class switches the build mode.
	 * @note When the instance is destroyed, the mode that was
	 *   current when the instance was created is restored!
	 */
	BaseScriptMode (bool t);

	/**
	 * Destroying this instance will reset the building mode to the mode
	 * it was active when the instance was created.
	 */
	~BaseScriptMode();

	/** Check if this instance represents a switch to test mode. */
	bool operator() (void) const
	{
		return this->test;
	}
};

/**
 * Class to switch current mode to Execute Mode.
 * If you create an instance of this class, the mode will be switched to
 *   Execute. The original mode is stored and recovered from when ever the
 *   instance is destroyed.
 * In Execute mode all commands you do are executed for real.
 * @api ai game
 */
class ScriptExecMode : public BaseScriptMode {
public:
	/**
	 * Creating instance of this class switches the build mode to Execute.
	 * @note When the instance is destroyed, he restores the mode that was
	 *   current when the instance was created!
	 */
	ScriptExecMode();
};

/**
 * Class to switch current mode to Test Mode.
 * If you create an instance of this class, the mode will be switched to
 *   Testing. The original mode is stored and recovered from when ever the
 *   instance is destroyed.
 * In Test mode all the commands you execute aren't really executed. The
 *   system only checks if it would be able to execute your requests, and what
 *   the cost would be.
 * @api ai game
 */
class ScriptTestMode : public BaseScriptMode {
public:
	/**
	 * Creating instance of this class switches the build mode to Testing.
	 * @note When the instance is destroyed, he restores the mode that was
	 *   current when the instance was created!
	 */
	ScriptTestMode();
};

#endif /* SCRIPT_MODE_HPP */
