/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_cargo.cpp Implementation of NewGRF cargoes. */

#include "stdafx.h"
#include "debug.h"
#include "newgrf_spritegroup.h"

/** Resolver of cargo. */
struct CargoResolverObject : public ResolverObject {
	/**
	 * Constructor of the cargo resolver.
	 * @param cs Cargo being resolved.
	 * @param callback Callback ID.
	 * @param param1 First parameter (var 10) of the callback.
	 * @param param2 Second parameter (var 18) of the callback.
	 */
	CargoResolverObject (const CargoSpec *cs, CallbackID callback = CBID_NO_CALLBACK, uint32 param1 = 0, uint32 param2 = 0)
		: ResolverObject (cs->grffile, callback, param1, param2)
	{
	}

	/* virtual */ const SpriteGroup *ResolveReal(const RealSpriteGroup *group) const;
};

/* virtual */ const SpriteGroup *CargoResolverObject::ResolveReal(const RealSpriteGroup *group) const
{
	/* Cargo action 2s should always have only 1 "loaded" state, but some
	 * times things don't follow the spec... */
	return group->get_first (false);
}

static inline const SpriteGroup *CargoResolve (const CargoSpec *cs,
	CallbackID callback = CBID_NO_CALLBACK, uint32 param1 = 0, uint32 param2 = 0)
{
	CargoResolverObject object (cs, callback, param1, param2);
	return SpriteGroup::Resolve (cs->group, object);
}

/**
 * Get the custom sprite for the given cargo type.
 * @param cs Cargo being queried.
 * @return Custom sprite to draw, or \c 0 if not available.
 */
SpriteID GetCustomCargoSprite(const CargoSpec *cs)
{
	const SpriteGroup *group = CargoResolve (cs);
	if (group == NULL) return 0;

	return group->GetResult();
}


uint16 GetCargoCallback(CallbackID callback, uint32 param1, uint32 param2, const CargoSpec *cs)
{
	return SpriteGroup::CallbackResult (CargoResolve (cs, callback, param1, param2));
}

/**
 * Translate a GRF-local cargo slot/bitnum into a CargoID.
 * @param cargo   GRF-local cargo slot/bitnum.
 * @param grffile Originating GRF file.
 * @param usebit  Defines the meaning of \a cargo for GRF version < 7.
 *                If true, then \a cargo is a bitnum. If false, then \a cargo is a cargoslot.
 *                For GRF version >= 7 \a cargo is always a translated cargo bit.
 * @return CargoID or CT_INVALID if the cargo is not available.
 */
CargoID GetCargoTranslation(uint8 cargo, const GRFFile *grffile, bool usebit)
{
	/* Pre-version 7 uses the 'climate dependent' ID in callbacks and properties, i.e. cargo is the cargo ID */
	if (grffile->grf_version < 7 && !usebit) return cargo;

	/* Other cases use (possibly translated) cargobits */

	if (grffile->cargo_list.Length() > 0) {
		/* ...and the cargo is in bounds, then get the cargo ID for
		 * the label */
		if (cargo < grffile->cargo_list.Length()) return GetCargoIDByLabel(grffile->cargo_list[cargo]);
	} else {
		/* Else the cargo value is a 'climate independent' 'bitnum' */
		return GetCargoIDByBitnum(cargo);
	}
	return CT_INVALID;
}
