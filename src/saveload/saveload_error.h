/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload.h Types related to saveload errors. */

#ifndef SAVELOAD_ERROR_H
#define SAVELOAD_ERROR_H

#include "../strings_type.h"

/** Saveload error struct. */
struct SlErrorData {
	StringID str;      ///< error message
	const char *data;  ///< extra data for str
};

#endif /* SAVELOAD_ERROR_H */
