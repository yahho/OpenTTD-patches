/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file subsidy.cpp Handling of subsidies. */

#include "stdafx.h"
#include "company_func.h"
#include "industry.h"
#include "town.h"
#include "news_func.h"
#include "ai/ai.hpp"
#include "station_base.h"
#include "strings_func.h"
#include "window_func.h"
#include "subsidy_base.h"
#include "subsidy_func.h"
#include "core/pool_func.hpp"
#include "core/random_func.hpp"
#include "game/game.hpp"
#include "command_func.h"

#include "table/strings.h"

template<> Subsidy::Pool Subsidy::PoolItem::pool ("Subsidy");
INSTANTIATE_POOL_METHODS(Subsidy)

/**
 * Marks subsidy as awarded, creates news and AI event
 * @param company awarded company
 */
void Subsidy::AwardTo(CompanyID company)
{
	assert(!this->IsAwarded());

	this->awarded = company;
	this->remaining = SUBSIDY_CONTRACT_MONTHS;

	char company_name[MAX_LENGTH_COMPANY_NAME_CHARS * MAX_CHAR_LENGTH];
	SetDParam(0, company);
	GetString (company_name, STR_COMPANY_NAME);

	char *cn = xstrdup(company_name);

	/* Add a news item */
	std::pair <NewsReferenceType, NewsReferenceType> reftype =
			SetupSubsidyDecodeParams (this, false, 1);

	SetDParamStr(0, cn);
	AddNewsItem(
		STR_NEWS_SERVICE_SUBSIDY_AWARDED_HALF + _settings_game.difficulty.subsidy_multiplier,
		NT_SUBSIDIES, NF_NORMAL,
		reftype.first, this->src.id, reftype.second, this->dst.id,
		cn
	);
	AI::BroadcastNewEvent(new ScriptEventSubsidyAwarded(this->index));
	Game::NewEvent(new ScriptEventSubsidyAwarded(this->index));

	InvalidateWindowData(WC_SUBSIDIES_LIST, 0);
}

/**
 * Setup the string parameters for printing an end of a subsidy at the screen,
 * and compute the news reference for it.
 * @param i Param index to use for the type (id will be next).
 * @param src %CargoSource being printed.
 * @return Reference of the cargo source in the news system.
 */
static NewsReferenceType SetupSubsidyDecodeParam (uint i, const CargoSource &src)
{
	NewsReferenceType reftype;

	switch (src.type) {
		case ST_INDUSTRY:
			reftype = NR_INDUSTRY;
			SetDParam (i, STR_INDUSTRY_NAME);
			break;
		case ST_TOWN:
			reftype = NR_TOWN;
			SetDParam (i, STR_TOWN_NAME);
			break;
		default: NOT_REACHED();
	}

	SetDParam (i + 1, src.id);

	return reftype;
}

/**
 * Setup the string parameters for printing the subsidy at the screen, and compute the news reference for the subsidy.
 * @param s %Subsidy being printed.
 * @param mode Unit of cargo used, \c true means general name, \c false means singular form.
 * @param offset First param index to use.
 * @return Reference of the subsidy in the news system.
 */
std::pair <NewsReferenceType, NewsReferenceType>
SetupSubsidyDecodeParams (const Subsidy *s, bool mode, uint offset)
{
	/* if mode is false, use the singular form */
	const CargoSpec *cs = CargoSpec::Get(s->cargo_type);
	SetDParam (offset, mode ? cs->name : cs->name_single);

	NewsReferenceType a = SetupSubsidyDecodeParam (offset + 1, s->src);
	NewsReferenceType b = SetupSubsidyDecodeParam (offset + 4, s->dst);
	return std::make_pair (a, b);
}

/**
 * Sets a flag indicating that given town/industry is part of subsidised route.
 * @param src cargo source
 * @param flag flag to set
 */
static inline void SetPartOfSubsidyFlag (const CargoSource &src, PartOfSubsidy flag)
{
	switch (src.type) {
		case ST_INDUSTRY: Industry::Get(src.id)->part_of_subsidy |= flag; return;
		case ST_TOWN:   Town::Get(src.id)->cache.part_of_subsidy |= flag; return;
		default: NOT_REACHED();
	}
}

/**
 * Sets the subsidised flag on both ends of a subsidy route.
 * @param s The subsidy.
 */
static void SetPartOfSubsidyFlags (const Subsidy *s)
{
	SetPartOfSubsidyFlag (s->src, POS_SRC);
	SetPartOfSubsidyFlag (s->dst, POS_DST);
}

/** Perform a full rebuild of the subsidies cache. */
void RebuildSubsidisedSourceAndDestinationCache()
{
	Town *t;
	FOR_ALL_TOWNS(t) t->cache.part_of_subsidy = POS_NONE;

	Industry *i;
	FOR_ALL_INDUSTRIES(i) i->part_of_subsidy = POS_NONE;

	const Subsidy *s;
	FOR_ALL_SUBSIDIES(s) {
		SetPartOfSubsidyFlags (s);
	}
}

/**
 * Delete the subsidies associated with a given cargo source type and id.
 * @param type  Cargo source type of the id.
 * @param index Id to remove.
 */
void DeleteSubsidyWith(SourceType type, SourceID index)
{
	bool dirty = false;

	Subsidy *s;
	FOR_ALL_SUBSIDIES(s) {
		if ((s->src.type == type && s->src.id == index) || (s->dst.type == type && s->dst.id == index)) {
			delete s;
			dirty = true;
		}
	}

	if (dirty) {
		InvalidateWindowData(WC_SUBSIDIES_LIST, 0);
		RebuildSubsidisedSourceAndDestinationCache();
	}
}

/**
 * Check whether a specific subsidy already exists.
 * @param cargo Cargo type.
 * @param src_type Type of source of the cargo, affects interpretation of \a src.
 * @param src Id of the source.
 * @param dst_type Type of the destination of the cargo, affects interpretation of \a dst.
 * @param dst Id of the destination.
 * @return \c true if the subsidy already exists, \c false if not.
 */
static bool CheckSubsidyDuplicate(CargoID cargo, SourceType src_type, SourceID src, SourceType dst_type, SourceID dst)
{
	const Subsidy *s;
	FOR_ALL_SUBSIDIES(s) {
		if (s->cargo_type == cargo &&
				s->src.type == src_type && s->src.id == src &&
				s->dst.type == dst_type && s->dst.id == dst) {
			return true;
		}
	}
	return false;
}

/**
 * Checks if the source and destination of a subsidy are inside the distance limit.
 * @param src_type Type of \a src.
 * @param src      Index of source.
 * @param dst_type Type of \a dst.
 * @param dst      Index of destination.
 * @return True if they are inside the distance limit.
 */
static bool CheckSubsidyDistance(SourceType src_type, SourceID src, SourceType dst_type, SourceID dst)
{
	TileIndex tile_src = (src_type == ST_TOWN) ? Town::Get(src)->xy : Industry::Get(src)->location.tile;
	TileIndex tile_dst = (dst_type == ST_TOWN) ? Town::Get(dst)->xy : Industry::Get(dst)->location.tile;

	return (DistanceManhattan(tile_src, tile_dst) <= SUBSIDY_MAX_DISTANCE);
}

/**
 * Creates a subsidy with the given parameters.
 * @param cid      Subsidised cargo.
 * @param src_type Type of \a src.
 * @param src      Index of source.
 * @param dst_type Type of \a dst.
 * @param dst      Index of destination.
 */
void CreateSubsidy(CargoID cid, SourceType src_type, SourceID src, SourceType dst_type, SourceID dst)
{
	Subsidy *s = new Subsidy();
	s->cargo_type = cid;
	s->src.type = src_type;
	s->src.id = src;
	s->dst.type = dst_type;
	s->dst.id = dst;
	s->remaining = SUBSIDY_OFFER_MONTHS;
	s->awarded = INVALID_COMPANY;

	std::pair <NewsReferenceType, NewsReferenceType> reftype =
			SetupSubsidyDecodeParams (s, false);
	AddNewsItem (STR_NEWS_SERVICE_SUBSIDY_OFFERED, NT_SUBSIDIES, NF_NORMAL,
			reftype.first, s->src.id, reftype.second, s->dst.id);
	SetPartOfSubsidyFlags (s);
	AI::BroadcastNewEvent(new ScriptEventSubsidyOffer(s->index));
	Game::NewEvent(new ScriptEventSubsidyOffer(s->index));

	InvalidateWindowData(WC_SUBSIDIES_LIST, 0);
}

/**
 * Create a new subsidy.
 * @param tile unused.
 * @param flags type of operation
 * @param p1 various bitstuffed elements
 * - p1 = (bit  0 -  7) - SourceType of source.
 * - p1 = (bit  8 - 23) - SourceID of source.
 * - p1 = (bit 24 - 31) - CargoID of subsidy.
 * @param p2 various bitstuffed elements
 * - p2 = (bit  0 -  7) - SourceType of destination.
 * - p2 = (bit  8 - 23) - SourceID of destination.
 * @param text unused.
 * @return the cost of this operation or an error
 */
CommandCost CmdCreateSubsidy(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	if (!Subsidy::CanAllocateItem()) return CMD_ERROR;

	CargoID cid = GB(p1, 24, 8);
	SourceType src_type = (SourceType)GB(p1, 0, 8);
	SourceID src = GB(p1, 8, 16);
	SourceType dst_type = (SourceType)GB(p2, 0, 8);
	SourceID dst = GB(p2, 8, 16);

	if (_current_company != OWNER_DEITY) return CMD_ERROR;

	if (cid >= NUM_CARGO || !::CargoSpec::Get(cid)->IsValid()) return CMD_ERROR;

	switch (src_type) {
		case ST_TOWN:
			if (!Town::IsValidID(src)) return CMD_ERROR;
			break;
		case ST_INDUSTRY:
			if (!Industry::IsValidID(src)) return CMD_ERROR;
			break;
		default:
			return CMD_ERROR;
	}
	switch (dst_type) {
		case ST_TOWN:
			if (!Town::IsValidID(dst)) return CMD_ERROR;
			break;
		case ST_INDUSTRY:
			if (!Industry::IsValidID(dst)) return CMD_ERROR;
			break;
		default:
			return CMD_ERROR;
	}

	if (flags & DC_EXEC) {
		CreateSubsidy(cid, src_type, src, dst_type, dst);
	}

	return CommandCost();
}

/**
 * Tries to create a passenger subsidy between two towns.
 * @return True iff the subsidy was created.
 */
bool FindSubsidyPassengerRoute()
{
	if (!Subsidy::CanAllocateItem()) return false;

	const Town *src = Town::GetRandom();
	if (src->cache.population < SUBSIDY_PAX_MIN_POPULATION ||
			src->GetPercentTransported(CT_PASSENGERS) > SUBSIDY_MAX_PCT_TRANSPORTED) {
		return false;
	}

	const Town *dst = Town::GetRandom();
	if (dst->cache.population < SUBSIDY_PAX_MIN_POPULATION || src == dst) {
		return false;
	}

	if (DistanceManhattan(src->xy, dst->xy) > SUBSIDY_MAX_DISTANCE) return false;
	if (CheckSubsidyDuplicate(CT_PASSENGERS, ST_TOWN, src->index, ST_TOWN, dst->index)) return false;

	CreateSubsidy(CT_PASSENGERS, ST_TOWN, src->index, ST_TOWN, dst->index);

	return true;
}

bool FindSubsidyCargoDestination(CargoID cid, SourceType src_type, SourceID src);


/**
 * Tries to create a cargo subsidy with a town as source.
 * @return True iff the subsidy was created.
 */
bool FindSubsidyTownCargoRoute()
{
	if (!Subsidy::CanAllocateItem()) return false;

	SourceType src_type = ST_TOWN;

	/* Select a random town. */
	const Town *src_town = Town::GetRandom();

	uint32 town_cargo_produced = src_town->cargo_produced;

	/* Passenger subsidies are not handled here. */
	ClrBit(town_cargo_produced, CT_PASSENGERS);

	/* No cargo produced at all? */
	if (town_cargo_produced == 0) return false;

	/* Choose a random cargo that is produced in the town. */
	uint8 cargo_number = RandomRange(CountBits(town_cargo_produced));
	CargoID cid;
	FOR_EACH_SET_CARGO_ID(cid, town_cargo_produced) {
		if (cargo_number == 0) break;
		cargo_number--;
	}

	/* Avoid using invalid NewGRF cargoes. */
	if (!CargoSpec::Get(cid)->IsValid() ||
			_settings_game.linkgraph.GetDistributionType(cid) != DT_MANUAL) {
		return false;
	}

	/* Quit if the percentage transported is large enough. */
	if (src_town->GetPercentTransported(cid) > SUBSIDY_MAX_PCT_TRANSPORTED) return false;

	SourceID src = src_town->index;

	return FindSubsidyCargoDestination(cid, src_type, src);
}

/**
 * Tries to create a cargo subsidy with an industry as source.
 * @return True iff the subsidy was created.
 */
bool FindSubsidyIndustryCargoRoute()
{
	if (!Subsidy::CanAllocateItem()) return false;

	SourceType src_type = ST_INDUSTRY;

	/* Select a random industry. */
	const Industry *src_ind = Industry::GetRandom();
	if (src_ind == NULL) return false;

	uint trans, total;

	CargoID cid;

	/* Randomize cargo type */
	if (src_ind->produced_cargo[1] != CT_INVALID && HasBit(Random(), 0)) {
		cid = src_ind->produced_cargo[1];
		trans = src_ind->last_month_pct_transported[1];
		total = src_ind->last_month_production[1];
	} else {
		cid = src_ind->produced_cargo[0];
		trans = src_ind->last_month_pct_transported[0];
		total = src_ind->last_month_production[0];
	}

	/* Quit if no production in this industry
	 * or if the pct transported is already large enough
	 * or if the cargo is automatically distributed */
	if (total == 0 || trans > SUBSIDY_MAX_PCT_TRANSPORTED ||
			cid == CT_INVALID ||
			_settings_game.linkgraph.GetDistributionType(cid) != DT_MANUAL) {
		return false;
	}

	SourceID src = src_ind->index;

	return FindSubsidyCargoDestination(cid, src_type, src);
}

/**
 * Tries to find a suitable destination for the given source and cargo.
 * @param cid      Subsidized cargo.
 * @param src_type Type of \a src.
 * @param src      Index of source.
 * @return True iff the subsidy was created.
 */
bool FindSubsidyCargoDestination(CargoID cid, SourceType src_type, SourceID src)
{
	/* Choose a random destination. Only consider towns if they can accept the cargo. */
	SourceType dst_type = (HasBit(_town_cargoes_accepted, cid) && Chance16(1, 2)) ? ST_TOWN : ST_INDUSTRY;

	SourceID dst;
	switch (dst_type) {
		case ST_TOWN: {
			/* Select a random town. */
			const Town *dst_town = Town::GetRandom();

			/* Check if the town can accept this cargo. */
			if (!HasBit(dst_town->cargo_accepted_total, cid)) return false;

			dst = dst_town->index;
			break;
		}

		case ST_INDUSTRY: {
			/* Select a random industry. */
			const Industry *dst_ind = Industry::GetRandom();

			/* The industry must accept the cargo */
			if (dst_ind == NULL ||
					(cid != dst_ind->accepts_cargo[0] &&
					 cid != dst_ind->accepts_cargo[1] &&
					 cid != dst_ind->accepts_cargo[2])) {
				return false;
			}

			dst = dst_ind->index;
			break;
		}

		default: NOT_REACHED();
	}

	/* Check that the source and the destination are not the same. */
	if (src_type == dst_type && src == dst) return false;

	/* Check distance between source and destination. */
	if (!CheckSubsidyDistance(src_type, src, dst_type, dst)) return false;

	/* Avoid duplicate subsidies. */
	if (CheckSubsidyDuplicate(cid, src_type, src, dst_type, dst)) return false;

	CreateSubsidy(cid, src_type, src, dst_type, dst);

	return true;
}

/** Perform the monthly update of open subsidies, and try to create a new one. */
void SubsidyMonthlyLoop()
{
	bool modified = false;

	Subsidy *s;
	FOR_ALL_SUBSIDIES(s) {
		if (--s->remaining == 0) {
			if (!s->IsAwarded()) {
				std::pair <NewsReferenceType, NewsReferenceType> reftype =
						SetupSubsidyDecodeParams (s, true);
				AddNewsItem (STR_NEWS_OFFER_OF_SUBSIDY_EXPIRED,
						NT_SUBSIDIES, NF_NORMAL,
						reftype.first, s->src.id,
						reftype.second, s->dst.id);
				AI::BroadcastNewEvent(new ScriptEventSubsidyOfferExpired(s->index));
				Game::NewEvent(new ScriptEventSubsidyOfferExpired(s->index));
			} else {
				if (s->awarded == _local_company) {
					std::pair <NewsReferenceType, NewsReferenceType> reftype =
							SetupSubsidyDecodeParams (s, true);
					AddNewsItem (STR_NEWS_SUBSIDY_WITHDRAWN_SERVICE,
							NT_SUBSIDIES, NF_NORMAL,
							reftype.first, s->src.id,
							reftype.second, s->dst.id);
				}
				AI::BroadcastNewEvent(new ScriptEventSubsidyExpired(s->index));
				Game::NewEvent(new ScriptEventSubsidyExpired(s->index));
			}
			delete s;
			modified = true;
		}
	}

	if (modified) {
		RebuildSubsidisedSourceAndDestinationCache();
	} else if (_settings_game.linkgraph.distribution_pax != DT_MANUAL &&
			   _settings_game.linkgraph.distribution_mail != DT_MANUAL &&
			   _settings_game.linkgraph.distribution_armoured != DT_MANUAL &&
			   _settings_game.linkgraph.distribution_default != DT_MANUAL) {
		/* Return early if there are no manually distributed cargoes and if we
		 * don't need to invalidate the subsidies window. */
		return;
	}

	bool passenger_subsidy = false;
	bool town_subsidy = false;
	bool industry_subsidy = false;

	int random_chance = RandomRange(16);

	if (random_chance < 2 && _settings_game.linkgraph.distribution_pax == DT_MANUAL) {
		/* There is a 1/8 chance each month of generating a passenger subsidy. */
		int n = 1000;

		do {
			passenger_subsidy = FindSubsidyPassengerRoute();
		} while (!passenger_subsidy && n--);
	} else if (random_chance == 2) {
		/* Cargo subsidies with a town as a source have a 1/16 chance. */
		int n = 1000;

		do {
			town_subsidy = FindSubsidyTownCargoRoute();
		} while (!town_subsidy && n--);
	} else if (random_chance == 3) {
		/* Cargo subsidies with an industry as a source have a 1/16 chance. */
		int n = 1000;

		do {
			industry_subsidy = FindSubsidyIndustryCargoRoute();
		} while (!industry_subsidy && n--);
	}

	modified |= passenger_subsidy || town_subsidy || industry_subsidy;

	if (modified) InvalidateWindowData(WC_SUBSIDIES_LIST, 0);
}

/**
 * Tests whether given delivery is subsidised and possibly awards the subsidy to delivering company
 * @param cargo_type type of cargo
 * @param company company delivering the cargo
 * @param src Source of cargo
 * @param st station where the cargo is delivered to
 * @return is the delivery subsidised?
 */
bool CheckSubsidised (CargoID cargo_type, CompanyID company, const CargoSource &src, const Station *st)
{
	/* If the source isn't subsidised, don't continue */
	if (src.id == INVALID_SOURCE) return false;
	switch (src.type) {
		case ST_INDUSTRY:
			if (!(Industry::Get(src.id)->part_of_subsidy & POS_SRC)) return false;
			break;
		case ST_TOWN:
			if (!(Town::Get(src.id)->cache.part_of_subsidy & POS_SRC)) return false;
			break;
		default: return false;
	}

	/* Remember all towns near this station (at least one house in its catchment radius)
	 * which are destination of subsidised path. Do that only if needed */
	SmallVector<const Town *, 2> towns_near;
	if (!st->rect.empty()) {
		Subsidy *s;
		FOR_ALL_SUBSIDIES(s) {
			/* Don't create the cache if there is no applicable subsidy with town as destination */
			if (s->dst.type != ST_TOWN) continue;
			if (s->cargo_type != cargo_type || s->src != src) continue;
			if (s->IsAwarded() && s->awarded != company) continue;

			TileArea ta = st->GetCatchmentArea();
			TILE_AREA_LOOP(tile, ta) {
				if (!IsHouseTile(tile)) continue;
				const Town *t = Town::GetByTile(tile);
				if (t->cache.part_of_subsidy & POS_DST) towns_near.Include(t);
			}
			break;
		}
	}

	bool subsidised = false;

	/* Check if there's a (new) subsidy that applies. There can be more subsidies triggered by this delivery!
	 * Think about the case that subsidies are A->B and A->C and station has both B and C in its catchment area */
	Subsidy *s;
	FOR_ALL_SUBSIDIES(s) {
		if (s->cargo_type == cargo_type && s->src == src && (!s->IsAwarded() || s->awarded == company)) {
			switch (s->dst.type) {
				case ST_INDUSTRY:
					for (const Industry * const *ip = st->industries_near.Begin(); ip != st->industries_near.End(); ip++) {
						if (s->dst.id == (*ip)->index) {
							assert((*ip)->part_of_subsidy & POS_DST);
							subsidised = true;
							if (!s->IsAwarded()) s->AwardTo(company);
						}
					}
					break;
				case ST_TOWN:
					for (const Town * const *tp = towns_near.Begin(); tp != towns_near.End(); tp++) {
						if (s->dst.id == (*tp)->index) {
							assert((*tp)->cache.part_of_subsidy & POS_DST);
							subsidised = true;
							if (!s->IsAwarded()) s->AwardTo(company);
						}
					}
					break;
				default:
					NOT_REACHED();
			}
		}
	}

	return subsidised;
}
