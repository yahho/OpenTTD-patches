/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file game_info.cpp Implementation of GameInfo */

#include "../stdafx.h"

#include "../script/convert.hpp"
#include "../script/script_scanner.hpp"
#include "game_info.hpp"
#include "../debug.h"

static const char *const game_api_versions[] =
	{ "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8" };

SQInteger GameInfo::construct (ScriptScanner *scanner)
{
	return this->ScriptVersionedInfo::construct (scanner,
						game_api_versions, NULL);
}
