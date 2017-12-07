/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_airport.cpp NewGRF handling of airports. */

#include "stdafx.h"
#include "debug.h"
#include "date_func.h"
#include "newgrf_spritegroup.h"
#include "newgrf_text.h"
#include "station_base.h"
#include "newgrf_class_func.h"

/** Resolver for the airport scope. */
struct AirportScopeResolver : public ScopeResolver {
	const GRFFile *const grffile; ///< GRFFile the resolved SpriteGroup belongs to.
	struct Station *st; ///< Station of the airport for which the callback is run, or \c NULL for build gui.
	byte layout;        ///< Layout of the airport to build.
	TileIndex tile;     ///< Tile for the callback, only valid for airporttile callbacks.

	/**
	 * Constructor of the scope resolver for an airport.
	 * @param grffile GRFFile the resolved SpriteGroup belongs to.
	 * @param tile %Tile for the callback, only valid for airporttile callbacks.
	 * @param st %Station of the airport for which the callback is run, or \c NULL for build gui.
	 * @param layout Layout of the airport to build.
	 */
	AirportScopeResolver (const GRFFile *grffile, TileIndex tile,
			Station *st, byte layout)
		: ScopeResolver(), grffile(grffile),
		  st(st), layout(layout), tile(tile)
	{
	}

	/* virtual */ uint32 GetRandomBits() const;
	/* virtual */ uint32 GetVariable(byte variable, uint32 parameter, bool *available) const;
	/* virtual */ void StorePSA(uint pos, int32 value);
};

/** Resolver object for airports. */
struct AirportResolverObject : public ResolverObject {
	AirportScopeResolver airport_scope;

	/**
	 * Constructor of the airport resolver.
	 * @param tile %Tile for the callback, only valid for airporttile callbacks.
	 * @param st %Station of the airport for which the callback is run, or \c NULL for build gui.
	 * @param as AirportSpec for which the callback is run.
	 * @param layout Layout of the airport to build.
	 * @param callback Callback ID.
	 * @param param1 First parameter (var 10) of the callback.
	 * @param param2 Second parameter (var 18) of the callback.
	 */
	AirportResolverObject (TileIndex tile, Station *st, const AirportSpec *as, byte layout,
			CallbackID callback = CBID_NO_CALLBACK, uint32 param1 = 0, uint32 param2 = 0)
		: ResolverObject (as->grf_prop.grffile, callback, param1, param2),
		  airport_scope (this->grffile, tile, st, layout)
	{
	}

	/* virtual */ ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, byte relative = 0)
	{
		switch (scope) {
			case VSG_SCOPE_SELF: return &this->airport_scope;
			default: return ResolverObject::GetScope(scope, relative);
		}
	}

	/* virtual */ const SpriteGroup *ResolveReal(const RealSpriteGroup *group) const;
};

/**
 * Reset airport classes to their default state.
 * This includes initialising the defaults classes with an empty
 * entry, for standard airports.
 */
template <>
/* static */ void NewGRFClass <AirportSpec, AirportClassID, APC_MAX>::InsertDefaults()
{
	AirportClass::Get(AirportClass::Allocate('SMAL'))->name = STR_AIRPORT_CLASS_SMALL;
	AirportClass::Get(AirportClass::Allocate('LARG'))->name = STR_AIRPORT_CLASS_LARGE;
	AirportClass::Get(AirportClass::Allocate('HUB_'))->name = STR_AIRPORT_CLASS_HUB;
	AirportClass::Get(AirportClass::Allocate('HELI'))->name = STR_AIRPORT_CLASS_HELIPORTS;
}

template <>
bool NewGRFClass <AirportSpec, AirportClassID, APC_MAX>::IsUIAvailable (uint index) const
{
	return true;
}

INSTANTIATE_NEWGRF_CLASS_METHODS(AirportClass)


AirportOverrideManager _airport_mngr(NEW_AIRPORT_OFFSET, NUM_AIRPORTS, AT_INVALID);

AirportSpec AirportSpec::specs[NUM_AIRPORTS]; ///< Airport specifications.

/**
 * Retrieve airport spec for the given airport. If an override is available
 *  it is returned.
 * @param type index of airport
 * @return A pointer to the corresponding AirportSpec
 */
/* static */ const AirportSpec *AirportSpec::Get(byte type)
{
	assert(type < lengthof(AirportSpec::specs));
	const AirportSpec *as = &AirportSpec::specs[type];
	if (type >= NEW_AIRPORT_OFFSET && !as->enabled) {
		byte subst_id = _airport_mngr.GetSubstituteID(type);
		if (subst_id == AT_INVALID) return as;
		as = &AirportSpec::specs[subst_id];
	}
	if (as->grf_prop.override != AT_INVALID) return &AirportSpec::specs[as->grf_prop.override];
	return as;
}

/**
 * Retrieve airport spec for the given airport. Even if an override is
 *  available the base spec is returned.
 * @param type index of airport
 * @return A pointer to the corresponding AirportSpec
 */
/* static */ AirportSpec *AirportSpec::GetWithoutOverride(byte type)
{
	assert(type < lengthof(AirportSpec::specs));
	return &AirportSpec::specs[type];
}

/** Check whether this airport is available to build. */
bool AirportSpec::IsAvailable() const
{
	if (!this->enabled) return false;
	if (_cur_year < this->min_year) return false;
	if (_settings_game.station.never_expire_airports) return true;
	return _cur_year <= this->max_year;
}

/**
 * This function initializes the airportspec array.
 */
void AirportSpec::ResetAirports()
{
	extern const AirportSpec _origin_airport_specs[];
	memset(&AirportSpec::specs, 0, sizeof(AirportSpec::specs));
	memcpy(&AirportSpec::specs, &_origin_airport_specs, sizeof(AirportSpec) * NEW_AIRPORT_OFFSET);

	_airport_mngr.ResetOverride();
}

/**
 * Tie all airportspecs to their class.
 */
void BindAirportSpecs()
{
	for (int i = 0; i < NUM_AIRPORTS; i++) {
		AirportSpec *as = AirportSpec::GetWithoutOverride(i);
		if (as->enabled) AirportClass::Assign(as);
	}
}


void AirportOverrideManager::SetEntitySpec(AirportSpec *as)
{
	byte airport_id = this->AddEntityID(as->grf_prop.local_id, as->grf_prop.grffile->grfid, as->grf_prop.subst_id);

	if (airport_id == invalid_ID) {
		grfmsg(1, "Airport.SetEntitySpec: Too many airports allocated. Ignoring.");
		return;
	}

	memcpy(AirportSpec::GetWithoutOverride(airport_id), as, sizeof(*as));

	/* Now add the overrides. */
	for (int i = 0; i < max_offset; i++) {
		AirportSpec *overridden_as = AirportSpec::GetWithoutOverride(i);

		if (entity_overrides[i] != as->grf_prop.local_id || grfid_overrides[i] != as->grf_prop.grffile->grfid) continue;

		overridden_as->grf_prop.override = airport_id;
		overridden_as->enabled = false;
		entity_overrides[i] = invalid_ID;
		grfid_overrides[i] = 0;
	}
}

/* virtual */ uint32 AirportScopeResolver::GetVariable(byte variable, uint32 parameter, bool *available) const
{
	switch (variable) {
		case 0x40: return this->layout;
	}

	if (this->st == NULL) {
		*available = false;
		return UINT_MAX;
	}

	switch (variable) {
		/* Get a variable from the persistent storage */
		case 0x7C: return (this->st->airport.psa != NULL) ? this->st->airport.psa->GetValue(parameter) : 0;

		case 0xF0: return this->st->facilities;
		case 0xFA: return Clamp(this->st->build_date - DAYS_TILL_ORIGINAL_BASE_YEAR, 0, 65535);
	}

	return this->st->GetNewGRFVariable (this->grffile, variable, parameter, available);
}

/* virtual */ const SpriteGroup *AirportResolverObject::ResolveReal(const RealSpriteGroup *group) const
{
	/* Airport action 2s should always have only 1 "loaded" state, but some
	 * times things don't follow the spec... */
	return group->get_first (false);
}

/* virtual */ uint32 AirportScopeResolver::GetRandomBits() const
{
	return this->st == NULL ? 0 : this->st->random_bits;
}

/**
 * Store a value into the object's persistent storage.
 * @param object Object that we want to query.
 * @param pos Position in the persistent storage to use.
 * @param value Value to store.
 */
/* virtual */ void AirportScopeResolver::StorePSA(uint pos, int32 value)
{
	if (this->st == NULL) return;

	if (this->st->airport.psa == NULL) {
		/* There is no need to create a storage if the value is zero. */
		if (value == 0) return;

		/* Create storage on first modification. */
		uint32 grfid = (this->grffile != NULL) ? this->grffile->grfid : 0;
		assert(PersistentStorage::CanAllocateItem());
		this->st->airport.psa = new PersistentStorage(grfid, GSF_AIRPORTS, this->st->airport.tile);
	}
	this->st->airport.psa->StoreValue(pos, value);
}

static const SpriteGroup *AirportResolve (TileIndex tile, Station *st, const AirportSpec *as, byte layout,
	CallbackID callback = CBID_NO_CALLBACK, uint32 param1 = 0, uint32 param2 = 0)
{
	AirportResolverObject object (tile, st, as, layout, callback, param1, param2);
	return SpriteGroup::Resolve (as->grf_prop.spritegroup, object);
}

SpriteID GetCustomAirportSprite(const AirportSpec *as, byte layout)
{
	const SpriteGroup *group = AirportResolve (INVALID_TILE, NULL, as, layout);
	if (group == NULL) return as->preview_sprite;

	return group->GetResult();
}

uint16 GetAirportCallback(CallbackID callback, uint32 param1, uint32 param2, Station *st, TileIndex tile)
{
	return SpriteGroup::CallbackResult (AirportResolve (tile, st,
			AirportSpec::Get (st->airport.type), st->airport.layout,
			callback, param1, param2));
}

/**
 * Get a custom text for the airport.
 * @param as The airport type's specification.
 * @param layout The layout index.
 * @param callback The callback to call.
 * @return The custom text.
 */
StringID GetAirportTextCallback(const AirportSpec *as, byte layout, uint16 callback)
{
	uint16 cb_res = SpriteGroup::CallbackResult (AirportResolve (INVALID_TILE,
			NULL, as, layout, (CallbackID)callback));
	if (cb_res == CALLBACK_FAILED || cb_res == 0x400) return STR_UNDEFINED;
	if (cb_res > 0x400) {
		ErrorUnknownCallbackResult(as->grf_prop.grffile->grfid, callback, cb_res);
		return STR_UNDEFINED;
	}

	return GetGRFStringID(as->grf_prop.grffile->grfid, 0xD000 + cb_res);
}
