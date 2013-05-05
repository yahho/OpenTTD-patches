/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file company_sl.cpp Code handling saving and loading of company data */

#include "../stdafx.h"
#include "../company_func.h"
#include "../company_manager_face.h"
#include "../fios.h"
#include "../map/rail.h"
#include "../map/road.h"
#include "../map/tunnelbridge.h"
#include "../tunnelbridge.h"
#include "../station_base.h"
#include "../station_func.h"

#include "saveload_buffer.h"
#include "saveload_error.h"

#include "table/strings.h"

/**
 * Converts an old company manager's face format to the new company manager's face format
 *
 * Meaning of the bits in the old face (some bits are used in several times):
 * - 4 and 5: chin
 * - 6 to 9: eyebrows
 * - 10 to 13: nose
 * - 13 to 15: lips (also moustache for males)
 * - 16 to 19: hair
 * - 20 to 22: eye colour
 * - 20 to 27: tie, ear rings etc.
 * - 28 to 30: glasses
 * - 19, 26 and 27: race (bit 27 set and bit 19 equal to bit 26 = black, otherwise white)
 * - 31: gender (0 = male, 1 = female)
 *
 * @param face the face in the old format
 * @return the face in the new format
 */
CompanyManagerFace ConvertFromOldCompanyManagerFace(uint32 face)
{
	CompanyManagerFace cmf = 0;
	GenderEthnicity ge = GE_WM;

	if (HasBit(face, 31)) SetBit(ge, GENDER_FEMALE);
	if (HasBit(face, 27) && (HasBit(face, 26) == HasBit(face, 19))) SetBit(ge, ETHNICITY_BLACK);

	SetCompanyManagerFaceBits(cmf, CMFV_GEN_ETHN,    ge, ge);
	SetCompanyManagerFaceBits(cmf, CMFV_HAS_GLASSES, ge, GB(face, 28, 3) <= 1);
	SetCompanyManagerFaceBits(cmf, CMFV_EYE_COLOUR,  ge, HasBit(ge, ETHNICITY_BLACK) ? 0 : ClampU(GB(face, 20, 3), 5, 7) - 5);
	SetCompanyManagerFaceBits(cmf, CMFV_CHIN,        ge, ScaleCompanyManagerFaceValue(CMFV_CHIN,     ge, GB(face,  4, 2)));
	SetCompanyManagerFaceBits(cmf, CMFV_EYEBROWS,    ge, ScaleCompanyManagerFaceValue(CMFV_EYEBROWS, ge, GB(face,  6, 4)));
	SetCompanyManagerFaceBits(cmf, CMFV_HAIR,        ge, ScaleCompanyManagerFaceValue(CMFV_HAIR,     ge, GB(face, 16, 4)));
	SetCompanyManagerFaceBits(cmf, CMFV_JACKET,      ge, ScaleCompanyManagerFaceValue(CMFV_JACKET,   ge, GB(face, 20, 2)));
	SetCompanyManagerFaceBits(cmf, CMFV_COLLAR,      ge, ScaleCompanyManagerFaceValue(CMFV_COLLAR,   ge, GB(face, 22, 2)));
	SetCompanyManagerFaceBits(cmf, CMFV_GLASSES,     ge, GB(face, 28, 1));

	uint lips = GB(face, 10, 4);
	if (!HasBit(ge, GENDER_FEMALE) && lips < 4) {
		SetCompanyManagerFaceBits(cmf, CMFV_HAS_MOUSTACHE, ge, true);
		SetCompanyManagerFaceBits(cmf, CMFV_MOUSTACHE,     ge, max(lips, 1U) - 1);
	} else {
		if (!HasBit(ge, GENDER_FEMALE)) {
			lips = lips * 15 / 16;
			lips -= 3;
			if (HasBit(ge, ETHNICITY_BLACK) && lips > 8) lips = 0;
		} else {
			lips = ScaleCompanyManagerFaceValue(CMFV_LIPS, ge, lips);
		}
		SetCompanyManagerFaceBits(cmf, CMFV_LIPS, ge, lips);

		uint nose = GB(face, 13, 3);
		if (ge == GE_WF) {
			nose = (nose * 3 >> 3) * 3 >> 2; // There is 'hole' in the nose sprites for females
		} else {
			nose = ScaleCompanyManagerFaceValue(CMFV_NOSE, ge, nose);
		}
		SetCompanyManagerFaceBits(cmf, CMFV_NOSE, ge, nose);
	}

	uint tie_earring = GB(face, 24, 4);
	if (!HasBit(ge, GENDER_FEMALE) || tie_earring < 3) { // Not all females have an earring
		if (HasBit(ge, GENDER_FEMALE)) SetCompanyManagerFaceBits(cmf, CMFV_HAS_TIE_EARRING, ge, true);
		SetCompanyManagerFaceBits(cmf, CMFV_TIE_EARRING, ge, HasBit(ge, GENDER_FEMALE) ? tie_earring : ScaleCompanyManagerFaceValue(CMFV_TIE_EARRING, ge, tie_earring / 2));
	}

	return cmf;
}

/** Rebuilding of company statistics after loading a savegame. */
void AfterLoadCompanyStats()
{
	/* Reset infrastructure statistics to zero. */
	Company *c;
	FOR_ALL_COMPANIES(c) MemSetT(&c->infrastructure, 0);

	/* Collect airport count. */
	Station *st;
	FOR_ALL_STATIONS(st) {
		if ((st->facilities & FACIL_AIRPORT) && Company::IsValidID(st->owner)) {
			Company::Get(st->owner)->infrastructure.airport++;
		}
	}

	for (TileIndex tile = 0; tile < MapSize(); tile++) {
		switch (GetTileType(tile)) {
			case TT_RAILWAY: {
				c = Company::GetIfValid(GetTileOwner(tile));
				if (c == NULL) break;

				TrackBits bits = GetTrackBits(tile);
				if (HasExactlyOneBit(bits)) {
					Track track = FindFirstTrack(bits);
					c->infrastructure.rail[GetRailType(tile, track)] += IsTileSubtype(tile, TT_BRIDGE) ? TUNNELBRIDGE_TRACKBIT_FACTOR : 1;
					c->infrastructure.signal += CountBits(GetPresentSignals(tile, track));
				} else if (bits == TRACK_BIT_HORZ || bits == TRACK_BIT_VERT) {
					if (IsTileSubtype(tile, TT_BRIDGE)) {
						DiagDirection dir = GetTunnelBridgeDirection(tile);
						c->infrastructure.rail[GetSideRailType(tile, dir)] += TUNNELBRIDGE_TRACKBIT_FACTOR;
						c->infrastructure.rail[GetSideRailType(tile, ReverseDiagDir(dir))]++;
					} else {
						c->infrastructure.rail[GetRailType(tile, TRACK_UPPER)]++;
						c->infrastructure.rail[GetRailType(tile, TRACK_LOWER)]++;
					}
					c->infrastructure.signal += CountBits(GetPresentSignals(tile, TRACK_UPPER)) + CountBits(GetPresentSignals(tile, TRACK_LOWER));
				} else {
					assert(TracksOverlap(bits));
					uint pieces = CountBits(bits);
					pieces *= pieces;
					if (IsTileSubtype(tile, TT_BRIDGE)) pieces *= TUNNELBRIDGE_TRACKBIT_FACTOR;
					c->infrastructure.rail[GetRailType(tile, FindFirstTrack(bits))] += pieces;
				}

				if (IsTileSubtype(tile, TT_BRIDGE)) {
					/* Only count the bridge if we're on the northern end tile. */
					TileIndex other_end = GetOtherBridgeEnd(tile);
					if (tile < other_end) {
						/* Count each bridge TUNNELBRIDGE_TRACKBIT_FACTOR times to simulate
						 * the higher structural maintenance needs. */
						uint len = GetTunnelBridgeLength(tile, other_end) * TUNNELBRIDGE_TRACKBIT_FACTOR;
						c->infrastructure.rail[GetRailType(tile)] += len;
					}
				}
				break;
			}

			case TT_ROAD:
				if (IsTileSubtype(tile, TT_TRACK)) {
					/* Iterate all present road types as each can have a different owner. */
					RoadType rt;
					FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
						c = Company::GetIfValid(GetRoadOwner(tile, rt));
						if (c != NULL) c->infrastructure.road[rt] += CountBits(GetRoadBits(tile, rt));
					}
				} else {
					/* Only count the bridge if we're on the northern end tile. */
					TileIndex other_end = GetOtherBridgeEnd(tile);
					uint len = (tile < other_end) ? 2 * GetTunnelBridgeLength(tile, other_end) : 0;

					RoadBits bridge_piece = DiagDirToRoadBits(GetTunnelBridgeDirection(tile));

					/* Iterate all present road types as each can have a different owner. */
					RoadType rt;
					FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
						c = Company::GetIfValid(GetRoadOwner(tile, rt));
						if (c != NULL) {
							RoadBits pieces = GetRoadBits(tile, rt);
							uint n = CountBits(pieces);
							c->infrastructure.road[rt] += ((pieces & bridge_piece) != 0) ? (n + len) * TUNNELBRIDGE_TRACKBIT_FACTOR : n;
						}
					}
				}
				break;

			case TT_MISC:
				switch (GetTileSubtype(tile)) {
					case TT_MISC_CROSSING: {
						c = Company::GetIfValid(GetTileOwner(tile));
						if (c != NULL) c->infrastructure.rail[GetRailType(tile)] += LEVELCROSSING_TRACKBIT_FACTOR;

						/* Iterate all present road types as each can have a different owner. */
						RoadType rt;
						FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
							c = Company::GetIfValid(GetRoadOwner(tile, rt));
							/* A level crossing has two road bits. */
							if (c != NULL) c->infrastructure.road[rt] += 2;
						}
						break;
					}

					case TT_MISC_AQUEDUCT:
						c = Company::GetIfValid(GetTileOwner(tile));
						if (c != NULL) {
							/* Only count the bridge if we're on the northern end tile. */
							TileIndex other_end = GetOtherTunnelBridgeEnd(tile);
							if (tile < other_end) {
								/* Count each bridge TUNNELBRIDGE_TRACKBIT_FACTOR times to simulate
								 * the higher structural maintenance needs, and don't forget the end tiles. */
								uint len = (GetTunnelBridgeLength(tile, other_end) + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;
								c->infrastructure.water += len;
							}
						}
						break;

					case TT_MISC_TUNNEL: {
						/* Only count the tunnel if we're on the northern end tile. */
						TileIndex other_end = GetOtherTunnelEnd(tile);
						if (tile < other_end) {
							/* Count each tunnel TUNNELBRIDGE_TRACKBIT_FACTOR times to simulate
							 * the higher structural maintenance needs, and don't forget the end tiles. */
							uint len = (GetTunnelBridgeLength(tile, other_end) + 2) * TUNNELBRIDGE_TRACKBIT_FACTOR;

							if (GetTunnelTransportType(tile) == TRANSPORT_RAIL) {
								c = Company::GetIfValid(GetTileOwner(tile));
								if (c != NULL) c->infrastructure.rail[GetRailType(tile)] += len;
							} else {
								assert(GetTunnelTransportType(tile) == TRANSPORT_ROAD);
								/* Iterate all present road types as each can have a different owner. */
								RoadType rt;
								FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
									c = Company::GetIfValid(GetRoadOwner(tile, rt));
									if (c != NULL) c->infrastructure.road[rt] += len * 2; // A full diagonal road has two road bits.
								}
							}
						}
						break;
					}

					case TT_MISC_DEPOT:
						c = Company::GetIfValid(GetTileOwner(tile));
						if (c != NULL) {
							if (IsRailDepot(tile)) {
								c->infrastructure.rail[GetRailType(tile)]++;
							} else {
								/* Road depots have two road bits. */
								c->infrastructure.road[FIND_FIRST_BIT(GetRoadTypes(tile))] += 2;
							}
						}
						break;
				}
				break;

			case TT_STATION:
				c = Company::GetIfValid(GetTileOwner(tile));
				if (c != NULL && GetStationType(tile) != STATION_AIRPORT && !IsBuoy(tile)) c->infrastructure.station++;

				switch (GetStationType(tile)) {
					case STATION_RAIL:
					case STATION_WAYPOINT:
						if (c != NULL && !IsStationTileBlocked(tile)) c->infrastructure.rail[GetRailType(tile)]++;
						break;

					case STATION_BUS:
					case STATION_TRUCK: {
						/* Iterate all present road types as each can have a different owner. */
						RoadType rt;
						FOR_EACH_SET_ROADTYPE(rt, GetRoadTypes(tile)) {
							c = Company::GetIfValid(GetRoadOwner(tile, rt));
							if (c != NULL) c->infrastructure.road[rt] += 2; // A road stop has two road bits.
						}
						break;
					}

					case STATION_DOCK:
					case STATION_BUOY:
						if (GetWaterClass(tile) == WATER_CLASS_CANAL) {
							if (c != NULL) c->infrastructure.water++;
						}
						break;

					default:
						break;
				}
				break;

			case TT_WATER:
				if (IsShipDepot(tile) || IsLock(tile)) {
					c = Company::GetIfValid(GetTileOwner(tile));
					if (c != NULL) {
						if (IsShipDepot(tile)) c->infrastructure.water += LOCK_DEPOT_TILE_FACTOR;
						if (IsLock(tile) && GetLockPart(tile) == LOCK_PART_MIDDLE) {
							/* The middle tile specifies the owner of the lock. */
							c->infrastructure.water += 3 * LOCK_DEPOT_TILE_FACTOR; // the middle tile specifies the owner of the
							break; // do not count the middle tile as canal
						}
					}
				}
				/* FALL THROUGH */

			case TT_OBJECT:
				if (GetWaterClass(tile) == WATER_CLASS_CANAL) {
					c = Company::GetIfValid(GetTileOwner(tile));
					if (c != NULL) c->infrastructure.water++;
				}
				break;

			default:
				break;
		}
	}
}



/* Save/load of companies */
static const SaveLoad _company_desc[] = {
	SLE_VAR(CompanyProperties, name_2,           SLE_UINT32),
	SLE_VAR(CompanyProperties, name_1,           SLE_STRINGID),
	SLE_STR(CompanyProperties, name,             SLS_STR | SLS_ALLOW_CONTROL, 0, 0, , 84, ),

	SLE_VAR(CompanyProperties, president_name_1, SLE_UINT16),
	SLE_VAR(CompanyProperties, president_name_2, SLE_UINT32),
	SLE_STR(CompanyProperties, president_name,   SLS_STR | SLS_ALLOW_CONTROL, 0, 0, , 84, ),

	SLE_VAR(CompanyProperties, face,                  SLE_UINT32),

	/* money was changed to a 64 bit field in legacy savegame version 1. */
	SLE_VAR(CompanyProperties, money,                 SLE_VAR_I64 | SLE_FILE_I32,  , ,   0,   0),
	SLE_VAR(CompanyProperties, money,                 SLE_INT64,                  0, ,   1,    ),

	SLE_VAR(CompanyProperties, current_loan,          SLE_VAR_I64 | SLE_FILE_I32,  , ,   0,  64),
	SLE_VAR(CompanyProperties, current_loan,          SLE_INT64,                  0, ,  65,    ),

	SLE_VAR(CompanyProperties, colour,                SLE_UINT8),
	SLE_VAR(CompanyProperties, money_fraction,        SLE_UINT8),
	SLE_VAR(CompanyProperties, avail_railtypes,       SLE_UINT8,                   , ,   0,  57),
	SLE_VAR(CompanyProperties, block_preview,         SLE_UINT8),

	SLE_NULL(2, , ,  0,  93), ///< cargo_types
	SLE_NULL(4, , , 94, 169), ///< cargo_types
	SLE_VAR(CompanyProperties, location_of_HQ,        SLE_FILE_U16 | SLE_VAR_U32,  , ,   0,   5),
	SLE_VAR(CompanyProperties, location_of_HQ,        SLE_UINT32,                 0, ,   6,    ),
	SLE_VAR(CompanyProperties, last_build_coordinate, SLE_FILE_U16 | SLE_VAR_U32,  , ,   0,   5),
	SLE_VAR(CompanyProperties, last_build_coordinate, SLE_UINT32,                 0, ,   6,    ),
	SLE_VAR(CompanyProperties, inaugurated_year,      SLE_FILE_U8  | SLE_VAR_I32,  , ,   0,  30),
	SLE_VAR(CompanyProperties, inaugurated_year,      SLE_INT32,                  0, ,  31,    ),

	SLE_ARR(CompanyProperties, share_owners,          SLE_UINT8, 4),

	SLE_VAR(CompanyProperties, num_valid_stat_ent,    SLE_UINT8),

	SLE_VAR(CompanyProperties, months_of_bankruptcy,  SLE_UINT8),
	SLE_VAR(CompanyProperties, bankrupt_asked,        SLE_FILE_U8  | SLE_VAR_U16,  , ,   0, 103),
	SLE_VAR(CompanyProperties, bankrupt_asked,        SLE_UINT16,                 0, , 104,    ),
	SLE_VAR(CompanyProperties, bankrupt_timeout,      SLE_INT16),
	SLE_VAR(CompanyProperties, bankrupt_value,        SLE_VAR_I64 | SLE_FILE_I32,  , ,   0,  64),
	SLE_VAR(CompanyProperties, bankrupt_value,        SLE_INT64,                  0, ,  65,    ),

	/* yearly expenses was changed to 64-bit in legacy savegame version 2. */
	SLE_ARR(CompanyProperties, yearly_expenses,       SLE_FILE_I32 | SLE_VAR_I64, 3 * 13,  , , 0, 1),
	SLE_ARR(CompanyProperties, yearly_expenses,       SLE_INT64,                  3 * 13, 0, , 2,  ),

	SLE_VAR(CompanyProperties, is_ai,                 SLE_BOOL,                   0, ,   2,    ),
	SLE_NULL(1, , , 107, 111), ///< is_noai
	SLE_NULL(1, , ,   4,  99),

	SLE_VAR(CompanyProperties, terraform_limit,       SLE_UINT32,                 0, , 156,    ),
	SLE_VAR(CompanyProperties, clear_limit,           SLE_UINT32,                 0, , 156,    ),
	SLE_VAR(CompanyProperties, tree_limit,            SLE_UINT32,                 0, , 175,    ),

	SLE_END()
};

static const SaveLoad _company_settings_desc[] = {
	/* Engine renewal settings */
	SLE_NULL(512, , , 16, 18),
	SLE_REF(Company, engine_renew_list,            REF_ENGINE_RENEWS,  0, ,  19, ),
	SLE_VAR(Company, settings.engine_renew,        SLE_BOOL,           0, ,  16, ),
	SLE_VAR(Company, settings.engine_renew_months, SLE_INT16,          0, ,  16, ),
	SLE_VAR(Company, settings.engine_renew_money,  SLE_UINT32,         0, ,  16, ),
	SLE_VAR(Company, settings.renew_keep_length,   SLE_BOOL,           0, ,   2, ),

	/* Default vehicle settings */
	SLE_VAR(Company, settings.vehicle.servint_ispercent,   SLE_BOOL,   0, , 120, ),
	SLE_VAR(Company, settings.vehicle.servint_trains,    SLE_UINT16,   0, , 120, ),
	SLE_VAR(Company, settings.vehicle.servint_roadveh,   SLE_UINT16,   0, , 120, ),
	SLE_VAR(Company, settings.vehicle.servint_aircraft,  SLE_UINT16,   0, , 120, ),
	SLE_VAR(Company, settings.vehicle.servint_ships,     SLE_UINT16,   0, , 120, ),

	SLE_NULL(63, , , 2, 143), // old reserved space

	SLE_END()
};

static const SaveLoad _company_settings_skip_desc[] = {
	/* Engine renewal settings */
	SLE_NULL(512, , , 16, 18),
	SLE_NULL(2,  , , 19, 68),  // engine_renew_list
	SLE_NULL(4, 0, , 69,   ),  // engine_renew_list
	SLE_NULL(1, 0, , 16,   ),  // settings.engine_renew
	SLE_NULL(2, 0, , 16,   ),  // settings.engine_renew_months
	SLE_NULL(4, 0, , 16,   ),  // settings.engine_renew_money
	SLE_NULL(1, 0, ,  2,   ),  // settings.renew_keep_length

	/* Default vehicle settings */
	SLE_NULL(1, 0, , 120, ),   // settings.vehicle.servint_ispercent
	SLE_NULL(2, 0, , 120, ),   // settings.vehicle.servint_trains
	SLE_NULL(2, 0, , 120, ),   // settings.vehicle.servint_roadveh
	SLE_NULL(2, 0, , 120, ),   // settings.vehicle.servint_aircraft
	SLE_NULL(2, 0, , 120, ),   // settings.vehicle.servint_ships

	SLE_NULL(63, , , 2, 143),  // old reserved space

	SLE_END()
};

static const SaveLoad _company_economy_desc[] = {
	/* these were changed to 64-bit in legacy savegame format 2 */
	SLE_VAR(CompanyEconomyEntry, income,        SLE_FILE_I32 | SLE_VAR_I64,  , , 0, 1),
	SLE_VAR(CompanyEconomyEntry, income,        SLE_INT64,                  0, , 2,  ),
	SLE_VAR(CompanyEconomyEntry, expenses,      SLE_FILE_I32 | SLE_VAR_I64,  , , 0, 1),
	SLE_VAR(CompanyEconomyEntry, expenses,      SLE_INT64,                  0, , 2,  ),
	SLE_VAR(CompanyEconomyEntry, company_value, SLE_FILE_I32 | SLE_VAR_I64,  , , 0, 1),
	SLE_VAR(CompanyEconomyEntry, company_value, SLE_INT64,                  0, , 2,  ),

	SLE_VAR(CompanyEconomyEntry, delivered_cargo[NUM_CARGO - 1], SLE_INT32,   , ,   0, 169),
	SLE_ARR(CompanyEconomyEntry, delivered_cargo,     SLE_UINT32, NUM_CARGO, 0, , 170,    ),
	SLE_VAR(CompanyEconomyEntry, performance_history, SLE_INT32),

	SLE_END()
};

/* We do need to read this single value, as the bigger it gets, the more data is stored */
struct CompanyOldAI {
	uint8 num_build_rec;
};

static const SaveLoad _company_ai_desc[] = {
	SLE_NULL(2, , ,  0, 106),
	SLE_NULL(2, , ,  0,  12),
	SLE_NULL(4, , , 13, 106),
	SLE_NULL(8, , ,  0, 106),
	 SLE_VAR(CompanyOldAI, num_build_rec, SLE_UINT8, , , 0, 106),
	SLE_NULL(3, , ,  0, 106),

	SLE_NULL(2, , ,  0,   5),
	SLE_NULL(4, , ,  6, 106),
	SLE_NULL(2, , ,  0,   5),
	SLE_NULL(4, , ,  6, 106),
	SLE_NULL(2, , ,  0, 106),

	SLE_NULL(2, , ,  0,   5),
	SLE_NULL(4, , ,  6, 106),
	SLE_NULL(2, , ,  0,   5),
	SLE_NULL(4, , ,  6, 106),
	SLE_NULL(2, , ,  0, 106),

	SLE_NULL(2, , ,  0,  68),
	SLE_NULL(4, , , 69, 106),

	SLE_NULL(18, , , 0, 106),
	SLE_NULL(20, , , 0, 106),
	SLE_NULL(32, , , 0, 106),

	SLE_NULL(64, , , 2, 106),
	SLE_END()
};

static const SaveLoad _company_ai_build_rec_desc[] = {
	SLE_NULL(2, , , 0,   5),
	SLE_NULL(4, , , 6, 106),
	SLE_NULL(2, , , 0,   5),
	SLE_NULL(4, , , 6, 106),
	SLE_NULL(8, , , 0, 106),
	SLE_END()
};

static const SaveLoad _company_livery_desc[] = {
	SLE_VAR(Livery, in_use,  SLE_BOOL,  0, , 34, ),
	SLE_VAR(Livery, colour1, SLE_UINT8, 0, , 34, ),
	SLE_VAR(Livery, colour2, SLE_UINT8, 0, , 34, ),
	SLE_END()
};

static void Load_PLYR_common(LoadBuffer *reader, Company *c, CompanyProperties *cprops)
{
	int i;

	reader->ReadObject(cprops, _company_desc);
	if (c != NULL) {
		reader->ReadObject(c, _company_settings_desc);
	} else {
		reader->ReadObject(NULL, _company_settings_skip_desc);
	}

	/* Keep backwards compatible for savegames, so load the old AI block */
	if (reader->IsOTTDVersionBefore(107) && cprops->is_ai) {
		CompanyOldAI old_ai;

		reader->ReadObject(&old_ai, _company_ai_desc);
		for (i = 0; i != old_ai.num_build_rec; i++) {
			reader->ReadObject(NULL, _company_ai_build_rec_desc);
		}
	}

	/* Read economy */
	reader->ReadObject(&cprops->cur_economy, _company_economy_desc);

	/* Read old economy entries. */
	if (cprops->num_valid_stat_ent > lengthof(cprops->old_economy)) throw SlCorrupt("Too many old economy entries");
	for (i = 0; i < cprops->num_valid_stat_ent; i++) {
		reader->ReadObject(&cprops->old_economy[i], _company_economy_desc);
	}

	/* Read each livery entry. */
	int num_liveries = reader->IsOTTDVersionBefore(63) ? LS_END - 4 : (reader->IsOTTDVersionBefore(85) ? LS_END - 2: LS_END);
	if (c != NULL) {
		for (i = 0; i < num_liveries; i++) {
			reader->ReadObject(&c->livery[i], _company_livery_desc);
		}

		if (num_liveries < LS_END) {
			/* We want to insert some liveries somewhere in between. This means some have to be moved. */
			memmove(&c->livery[LS_FREIGHT_WAGON], &c->livery[LS_PASSENGER_WAGON_MONORAIL], (LS_END - LS_FREIGHT_WAGON) * sizeof(c->livery[0]));
			c->livery[LS_PASSENGER_WAGON_MONORAIL] = c->livery[LS_MONORAIL];
			c->livery[LS_PASSENGER_WAGON_MAGLEV]   = c->livery[LS_MAGLEV];
		}

		if (num_liveries == LS_END - 4) {
			/* Copy bus/truck liveries over to trams */
			c->livery[LS_PASSENGER_TRAM] = c->livery[LS_BUS];
			c->livery[LS_FREIGHT_TRAM]   = c->livery[LS_TRUCK];
		}
	} else {
		/* Skip liveries */
		Livery dummy_livery;
		for (i = 0; i < num_liveries; i++) {
			reader->ReadObject(&dummy_livery, _company_livery_desc);
		}
	}
}

static void Save_PLYR(SaveDumper *dumper)
{
	Company *c;
	FOR_ALL_COMPANIES(c) {
		SaveDumper temp(1024);
		int i;

		temp.WriteObject((CompanyProperties*)c, _company_desc);
		temp.WriteObject(c, _company_settings_desc);

		/* Write economy */
		temp.WriteObject(&c->cur_economy, _company_economy_desc);

		/* Write old economy entries. */
		assert(c->num_valid_stat_ent <= lengthof(c->old_economy));
		for (i = 0; i < c->num_valid_stat_ent; i++) {
			temp.WriteObject(&c->old_economy[i], _company_economy_desc);
		}

		/* Write each livery entry. */
		for (i = 0; i < LS_END; i++) {
			temp.WriteObject(&c->livery[i], _company_livery_desc);
		}

		dumper->WriteElementHeader(c->index, temp.GetSize());
		temp.Dump(dumper);
	}
}

static void Load_PLYR(LoadBuffer *reader)
{
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		Company *c = new (index) Company();
		Load_PLYR_common(reader, c, c);
		_company_colours[index] = (Colours)c->colour;
	}
}

static void Check_PLYR(LoadBuffer *reader)
{
	int index;
	while ((index = reader->IterateChunk()) != -1) {
		CompanyProperties *cprops = new CompanyProperties();
		memset(cprops, 0, sizeof(*cprops));
		Load_PLYR_common(reader, NULL, cprops);

		/* We do not load old custom names */
		if (reader->IsOTTDVersionBefore(84)) {
			if (GB(cprops->name_1, 11, 5) == 15) {
				cprops->name_1 = STR_GAME_SAVELOAD_NOT_AVAILABLE;
			}

			if (GB(cprops->president_name_1, 11, 5) == 15) {
				cprops->president_name_1 = STR_GAME_SAVELOAD_NOT_AVAILABLE;
			}
		}

		if (cprops->name == NULL && !IsInsideMM(cprops->name_1, SPECSTR_COMPANY_NAME_START, SPECSTR_COMPANY_NAME_LAST + 1) &&
				cprops->name_1 != STR_GAME_SAVELOAD_NOT_AVAILABLE && cprops->name_1 != STR_SV_UNNAMED &&
				cprops->name_1 != SPECSTR_ANDCO_NAME && cprops->name_1 != SPECSTR_PRESIDENT_NAME &&
				cprops->name_1 != SPECSTR_SILLY_NAME) {
			cprops->name_1 = STR_GAME_SAVELOAD_NOT_AVAILABLE;
		}

		if (!_load_check_data.companies.Insert(index, cprops)) delete cprops;
	}
}

static void Ptrs_PLYR(const SavegameTypeVersion *stv)
{
	Company *c;
	FOR_ALL_COMPANIES(c) {
		SlObjectPtrs(c, _company_settings_desc, stv);
	}
}


extern const ChunkHandler _company_chunk_handlers[] = {
	{ 'PLYR', Save_PLYR, Load_PLYR, Ptrs_PLYR, Check_PLYR, CH_ARRAY | CH_LAST},
};
