/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file game_info.hpp GameInfo keeps track of all information of an Game, like Author, Description, ... */

#ifndef GAME_INFO_HPP
#define GAME_INFO_HPP

#include "../script/script_info.hpp"

/** All static information from an Game like name, version, etc. */
class GameInfo : public ScriptVersionedInfo {
public:
	/** Gather all the information on registration. */
	SQInteger construct (class ScriptScanner *scanner) OVERRIDE;
};

/** All static information from an Game library like name, version, etc. */
class GameLibrary : public ScriptLibraryInfo {
};

#endif /* GAME_INFO_HPP */
