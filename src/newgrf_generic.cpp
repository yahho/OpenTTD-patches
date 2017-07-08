/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_generic.cpp Handling of generic feature callbacks. */

#include "stdafx.h"
#include "debug.h"
#include "newgrf_spritegroup.h"
#include "industrytype.h"
#include "core/random_func.hpp"
#include "newgrf_sound.h"
#include "map/ground.h"
#include "map/water.h"
#include "map/slope.h"
#include <list>

/** Data used from GenericScopeResolver. */
struct GenericScopeResolverData {
	CargoID cargo_type;
	uint8 default_selection;
	uint8 src_industry;        ///< Source industry substitute type. 0xFF for "town", 0xFE for "unknown".
	uint8 dst_industry;        ///< Destination industry substitute type. 0xFF for "town", 0xFE for "unknown".
	uint8 distance;
	AIConstructionEvent event;
	uint8 count;
	uint8 station_size;
};

/** Scope resolver for generic objects and properties. */
struct GenericScopeResolver : public ScopeResolver {
	const GRFFile *const grffile; ///< GRFFile the resolved SpriteGroup belongs to.
	const GenericScopeResolverData *const data;

	/**
	 * Generic scope resolver.
	 * @param grffile GRFFile the resolved SpriteGroup belongs to.
	 * @param data Callback data.
	 */
	GenericScopeResolver (const GRFFile *grffile, const GenericScopeResolverData *data)
		: ScopeResolver(), grffile(grffile), data(data)
	{
	}

	/* virtual */ uint32 GetVariable(byte variable, uint32 parameter, bool *available) const;
};


/** Resolver object for generic objects/properties. */
struct GenericResolverObject : public ResolverObject {
	GenericScopeResolver generic_scope;

	/**
	 * Generic resolver.
	 * @param grffile GRF file.
	 * @param data Callback data.
	 * @param callback Callback ID.
	 * @param param1 Callback param.
	 */
	GenericResolverObject (const GRFFile *grffile,
			const GenericScopeResolverData *data,
			CallbackID callback, uint32 param1)
		: ResolverObject (grffile, callback, param1),
		  generic_scope (grffile, data)
	{
	}

	/* virtual */ ScopeResolver *GetScope(VarSpriteGroupScope scope = VSG_SCOPE_SELF, byte relative = 0)
	{
		switch (scope) {
			case VSG_SCOPE_SELF: return &this->generic_scope;
			default: return ResolverObject::GetScope(scope, relative);
		}
	}

	/* virtual */ const SpriteGroup *ResolveReal(const RealSpriteGroup *group) const;
};

struct GenericCallback {
	const GRFFile *file;
	const SpriteGroup *group;

	GenericCallback(const GRFFile *file, const SpriteGroup *group) :
		file(file),
		group(group)
	{ }
};

typedef std::list<GenericCallback> GenericCallbackList;

static GenericCallbackList _gcl[GSF_END];


/**
 * Reset all generic feature callback sprite groups.
 */
void ResetGenericCallbacks()
{
	for (uint8 feature = 0; feature < lengthof(_gcl); feature++) {
		_gcl[feature].clear();
	}
}


/**
 * Add a generic feature callback sprite group to the appropriate feature list.
 * @param feature The feature for the callback.
 * @param file The GRF of the callback.
 * @param group The sprite group of the callback.
 */
void AddGenericCallback(uint8 feature, const GRFFile *file, const SpriteGroup *group)
{
	if (feature >= lengthof(_gcl)) {
		grfmsg(5, "AddGenericCallback: Unsupported feature 0x%02X", feature);
		return;
	}

	/* Generic feature callbacks are evaluated in reverse (i.e. the last group
	 * to be added is evaluated first, etc) thus we push the group to the
	 * beginning of the list so a standard iterator will do the right thing. */
	_gcl[feature].push_front(GenericCallback(file, group));
}

/* virtual */ uint32 GenericScopeResolver::GetVariable(byte variable, uint32 parameter, bool *available) const
{
	if (this->data != NULL) {
		switch (variable) {
			case 0x40: return this->grffile->cargo_map[this->data->cargo_type];

			case 0x80: return this->data->cargo_type;
			case 0x81: return CargoSpec::Get(this->data->cargo_type)->bitnum;
			case 0x82: return this->data->default_selection;
			case 0x83: return this->data->src_industry;
			case 0x84: return this->data->dst_industry;
			case 0x85: return this->data->distance;
			case 0x86: return this->data->event;
			case 0x87: return this->data->count;
			case 0x88: return this->data->station_size;

			default: break;
		}
	}

	DEBUG(grf, 1, "Unhandled generic feature variable 0x%02X", variable);

	*available = false;
	return UINT_MAX;
}


/* virtual */ const SpriteGroup *GenericResolverObject::ResolveReal(const RealSpriteGroup *group) const
{
	return group->get_first();
}


/**
 * Follow a generic feature callback list and return the first successful
 * answer
 * @param feature GRF Feature of callback
 * @param callback Callback
 * @param param1_grfv7 callback_param1 for GRFs up to version 7.
 * @param param1_grfv8 callback_param1 for GRFs from version 8 on.
 * @param data Callback data
 * @param [out] file Optionally returns the GRFFile which made the final decision for the callback result.
 *                   May be NULL if not required.
 * @return callback value if successful or CALLBACK_FAILED
 */
static uint16 GetGenericCallbackResult (uint8 feature, CallbackID callback, uint32 param1_grfv7, uint32 param1_grfv8, const GenericScopeResolverData *data, const GRFFile **file)
{
	assert(feature < lengthof(_gcl));

	/* Test each feature callback sprite group. */
	for (GenericCallbackList::const_iterator it = _gcl[feature].begin(); it != _gcl[feature].end(); ++it) {
		/* Set callback param based on GRF version. */
		GenericResolverObject object (it->file, data, callback,
				it->file->grf_version >= 8 ? param1_grfv8 : param1_grfv7);
		uint16 result = SpriteGroup::CallbackResult (SpriteGroup::Resolve (it->group, object));
		if (result == CALLBACK_FAILED) continue;

		/* Return NewGRF file if necessary */
		if (file != NULL) *file = it->file;

		return result;
	}

	/* No callback returned a valid result, so we've failed. */
	return CALLBACK_FAILED;
}


/**
 * 'Execute' an AI purchase selection callback
 *
 * @param feature GRF Feature to call callback for.
 * @param cargo_type Cargotype to pass to callback. (Variable 80)
 * @param default_selection 'Default selection' to pass to callback. (Variable 82)
 * @param src_industry 'Source industry type' to pass to callback. (Variable 83)
 * @param dst_industry 'Destination industry type' to pass to callback. (Variable 84)
 * @param distance 'Distance between source and destination' to pass to callback. (Variable 85)
 * @param event 'AI construction event' to pass to callback. (Variable 86)
 * @param count 'Construction number' to pass to callback. (Variable 87)
 * @param station_size 'Station size' to pass to callback. (Variable 88)
 * @param [out] file Optionally returns the GRFFile which made the final decision for the callback result.
 *                   May be NULL if not required.
 * @return callback value if successful or CALLBACK_FAILED
 */
uint16 GetAiPurchaseCallbackResult(uint8 feature, CargoID cargo_type, uint8 default_selection, IndustryType src_industry, IndustryType dst_industry, uint8 distance, AIConstructionEvent event, uint8 count, uint8 station_size, const GRFFile **file)
{
	if (src_industry != IT_AI_UNKNOWN && src_industry != IT_AI_TOWN) {
		const IndustrySpec *is = GetIndustrySpec(src_industry);
		/* If this is no original industry, use the substitute type */
		if (is->grf_prop.subst_id != INVALID_INDUSTRYTYPE) src_industry = is->grf_prop.subst_id;
	}

	if (dst_industry != IT_AI_UNKNOWN && dst_industry != IT_AI_TOWN) {
		const IndustrySpec *is = GetIndustrySpec(dst_industry);
		/* If this is no original industry, use the substitute type */
		if (is->grf_prop.subst_id != INVALID_INDUSTRYTYPE) dst_industry = is->grf_prop.subst_id;
	}

	GenericScopeResolverData data;
	data.cargo_type        = cargo_type;
	data.default_selection = default_selection;
	data.src_industry      = src_industry;
	data.dst_industry      = dst_industry;
	data.distance          = distance;
	data.event             = event;
	data.count             = count;
	data.station_size      = station_size;

	uint16 callback = GetGenericCallbackResult (feature, CBID_GENERIC_AI_PURCHASE_SELECTION, 0, 0, &data, file);
	if (callback != CALLBACK_FAILED) callback = GB(callback, 0, 8);
	return callback;
}


/**
 * 'Execute' the ambient sound effect callback.
 * @param tile Tile the sound effect should be generated for.
 */
void AmbientSoundEffectCallback(TileIndex tile)
{
	assert(IsGroundTile(tile) || IsWaterTile(tile));

	/* Only run every 1/200-th time. */
	uint32 r; // Save for later
	if (!Chance16R(1, 200, r) || !_settings_client.sound.ambient) return;

	/* Prepare resolver object. */
	int oldtype = IsWaterTile(tile) ? 6 : IsTreeTile(tile) ? 4 : 0;
	uint32 param1_v7 = oldtype << 28 | Clamp(TileHeight(tile), 0, 15) << 24 | GB(r, 16, 8) << 16 | GetTerrainType(tile);
	uint32 param1_v8 = oldtype << 24 | GetTileZ(tile) << 16 | GB(r, 16, 8) << 8 | (HasTileWaterClass(tile) ? GetWaterClass(tile) : 0) << 3 | GetTerrainType(tile);

	/* Run callback. */
	const GRFFile *grf_file;
	uint16 callback = GetGenericCallbackResult (GSF_SOUNDFX, CBID_SOUNDS_AMBIENT_EFFECT, param1_v7, param1_v8, NULL, &grf_file);

	if (callback != CALLBACK_FAILED) PlayTileSound(grf_file, callback, tile);
}
