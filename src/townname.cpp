/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file townname.cpp %Town name generators. */

#include "stdafx.h"
#include "string.h"
#include "townname_type.h"
#include "town.h"
#include "strings_func.h"
#include "core/random_func.hpp"
#include "genworld.h"
#include "gfx_layout.h"


/**
 * Initializes this struct from town data
 * @param t town for which we will be printing name later
 */
TownNameParams::TownNameParams(const Town *t) :
		grfid(t->townnamegrfid), // by default, use supplied data
		type(t->townnametype)
{
	if (t->townnamegrfid != 0 && GetGRFTownName(t->townnamegrfid) == NULL) {
		/* Fallback to english original */
		this->grfid = 0;
		this->type = SPECSTR_TOWNNAME_ENGLISH;
		return;
	}
}


/**
 * Fills buffer with specified town name
 * @param buf buffer
 * @param par town name parameters
 * @param townnameparts 'encoded' town name
 */
void AppendTownName (stringb *buf, const TownNameParams *par, uint32 townnameparts)
{
	if (par->grfid == 0) {
		int64 args_array[1] = { townnameparts };
		StringParameters tmp_params(args_array);
		AppendStringWithArgs (buf, par->type, &tmp_params);
	} else {
		GRFTownNameGenerate (buf, par->grfid, par->type, townnameparts);
	}
}


/**
 * Fills buffer with town's name
 * @param buff buffer start
 * @param t we want to get name of this town
 */
void AppendTownName (stringb *buf, const Town *t)
{
	TownNameParams par(t);
	AppendTownName (buf, &par, t->townnameparts);
}


/**
 * Verifies the town name is valid and unique.
 * @param r random bits
 * @param par town name parameters
 * @param town_names if a name is generated, check its uniqueness with the set
 * @return true iff name is valid and unique
 */
bool VerifyTownName(uint32 r, const TownNameParams *par, TownNames *town_names)
{
	/* reserve space for extra unicode character and terminating '\0' */
	sstring <(MAX_LENGTH_TOWN_NAME_CHARS + 1) * MAX_CHAR_LENGTH> buf1, buf2;

	AppendTownName (&buf1, par, r);

	/* Check size and width */
	if (buf1.utf8length() >= MAX_LENGTH_TOWN_NAME_CHARS) return false;

	if (town_names != NULL) {
		if (town_names->find(buf1.c_str()) != town_names->end()) return false;
		town_names->insert(buf1.c_str());
	} else {
		const Town *t;
		FOR_ALL_TOWNS(t) {
			/* We can't just compare the numbers since
			 * several numbers may map to a single name. */
			const char *buf = t->name;
			if (buf == NULL) {
				buf2.clear();
				AppendTownName (&buf2, t);
				buf = buf2.c_str();
			}
			if (strcmp(buf1.c_str(), buf) == 0) return false;
		}
	}

	return true;
}


/**
 * Generates valid town name.
 * @param townnameparts if a name is generated, it's stored there
 * @param town_names if a name is generated, check its uniqueness with the set
 * @return true iff a name was generated
 */
bool GenerateTownName(uint32 *townnameparts, TownNames *town_names)
{
	/* Do not set too low tries, since when we run out of names, we loop
	 * for #tries only one time anyway - then we stop generating more
	 * towns. Do not show it too high neither, since looping through all
	 * the other towns may take considerable amount of time (10000 is
	 * too much). */
	TownNameParams par(_settings_game.game_creation.town_name);

	/* This function is called very often without entering the gameloop
	 * inbetween. So reset layout cache to prevent it from growing too big. */
	Layouter::ReduceLineCache();

	for (int i = 1000; i != 0; i--) {
		uint32 r = _generating_world ? Random() : InteractiveRandom();
		if (!VerifyTownName(r, &par, town_names)) continue;

		*townnameparts = r;
		return true;
	}

	return false;
}
