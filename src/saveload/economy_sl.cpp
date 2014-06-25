/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file economy_sl.cpp Code handling saving and loading of economy data */

#include "../stdafx.h"
#include "../economy_func.h"
#include "../economy_base.h"

#include "saveload_buffer.h"

/** Prices in pre 126 savegames */
static void Load_PRIC(LoadBuffer *reader)
{
	/* Old games store 49 base prices, very old games store them as int32 */
	reader->Skip (49 * ((reader->IsOTTDVersionBefore(65) ? 4 : 8) + 2));
}

/** Cargo payment rates in pre 126 savegames */
static void Load_CAPR(LoadBuffer *reader)
{
	uint num_cargo = reader->IsOTTDVersionBefore(55) ? 12 : NUM_CARGO;
	reader->Skip (num_cargo * ((reader->IsOTTDVersionBefore(65) ? 4 : 8) + 2));
}

static const SaveLoad _economy_desc[] = {
	SLE_NULL(4,                                                                 , ,   0,  64), // max_loan
	SLE_NULL(8,                                                                 , ,  65, 143), // max_loan
	SLE_VAR(Economy, old_max_loan_unround,          SLE_FILE_I32 | SLE_VAR_I64, , ,   0,  64),
	SLE_VAR(Economy, old_max_loan_unround,          SLE_INT64,                  , ,  65, 125),
	SLE_VAR(Economy, old_max_loan_unround_fract,    SLE_UINT16,                 , ,  70, 125),
	SLE_VAR(Economy, inflation_prices,              SLE_UINT64,                0, , 126,    ),
	SLE_VAR(Economy, inflation_payment,             SLE_UINT64,                0, , 126,    ),
	SLE_VAR(Economy, fluct,                         SLE_INT16),
	SLE_VAR(Economy, interest_rate,                 SLE_UINT8),
	SLE_VAR(Economy, infl_amount,                   SLE_UINT8),
	SLE_VAR(Economy, infl_amount_pr,                SLE_UINT8),
	SLE_VAR(Economy, industry_daily_change_counter, SLE_UINT32,                0, , 102,    ),
	SLE_END()
};

/** Economy variables */
static void Save_ECMY(SaveDumper *dumper)
{
	dumper->WriteRIFFObject(&_economy, _economy_desc);
}

/** Economy variables */
static void Load_ECMY(LoadBuffer *reader)
{
	reader->ReadObject(&_economy, _economy_desc);
	StartupIndustryDailyChanges(reader->IsOTTDVersionBefore(102));  // old savegames will need to be initialized
}

static const SaveLoad _cargopayment_desc[] = {
	SLE_REF(CargoPayment, front,           REF_VEHICLE),
	SLE_VAR(CargoPayment, route_profit,    SLE_INT64),
	SLE_VAR(CargoPayment, visual_profit,   SLE_INT64),
	SLE_VAR(CargoPayment, visual_transfer, SLE_INT64, 0, , 181, ),
	SLE_END()
};

static void Save_CAPY(SaveDumper *dumper)
{
	CargoPayment *cp;
	FOR_ALL_CARGO_PAYMENTS(cp) {
		dumper->WriteElement(cp->index, cp, _cargopayment_desc);
	}
}

static void Load_CAPY(LoadBuffer *reader)
{
	int index;

	while ((index = reader->IterateChunk()) != -1) {
		CargoPayment *cp = new (index) CargoPayment();
		reader->ReadObject(cp, _cargopayment_desc);
	}
}

static void Ptrs_CAPY(const SavegameTypeVersion *stv)
{
	CargoPayment *cp;
	FOR_ALL_CARGO_PAYMENTS(cp) {
		SlObjectPtrs(cp, _cargopayment_desc, stv);
	}
}


extern const ChunkHandler _economy_chunk_handlers[] = {
	{ 'CAPY', Save_CAPY,     Load_CAPY,     Ptrs_CAPY, NULL, CH_ARRAY},
	{ 'PRIC', NULL,          Load_PRIC,     NULL,      NULL, CH_RIFF },
	{ 'CAPR', NULL,          Load_CAPR,     NULL,      NULL, CH_RIFF },
	{ 'ECMY', Save_ECMY,     Load_ECMY,     NULL,      NULL, CH_RIFF | CH_LAST},
};
