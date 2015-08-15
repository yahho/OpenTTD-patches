/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ai_info.hpp AIInfo keeps track of all information of an AI, like Author, Description, ... */

#ifndef AI_INFO_HPP
#define AI_INFO_HPP

#include "../script/script_info.hpp"

/** All static information from an AI like name, version, etc. */
class AIInfo : public ScriptVersionedInfo {
public:
	AIInfo();

	/** Gather all the information on registration. */
	SQInteger construct (class ScriptScanner *scanner) OVERRIDE;

	/**
	 * Create a dummy-AI.
	 */
	AIInfo (bool ignored);

	/**
	 * Use this AI as a random AI.
	 */
	bool UseAsRandomAI() const { return this->use == USE_RANDOM; }

	friend class AIInstance;

private:
	enum {
		USE_RANDOM,       ///< This AI can be used as a random AI.
		USE_MANUAL,       ///< Only use this AI when manually selected
		USE_DUMMY,        ///< This is the dummy AI
	};

	int use;                  ///< Use of this AI (manual, random, dummy)
};

/** All static information from an AI library like name, version, etc. */
class AILibrary : public ScriptLibraryInfo {
};

#endif /* AI_INFO_HPP */
