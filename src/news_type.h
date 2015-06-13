/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file news_type.h Types related to news. */

#ifndef NEWS_TYPE_H
#define NEWS_TYPE_H

#include "core/pointer.h"
#include "core/enum_type.hpp"
#include "date_func.h"
#include "strings_type.h"
#include "sound_type.h"
#include "settings_type.h"
#include "cargo_type.h"
#include "engine_type.h"
#include "vehicle_type.h"
#include "industry_type.h"
#include "map/coord.h"

/** Constants in the message options window. */
enum MessageOptionsSpace {
	MOS_WIDG_PER_SETTING      = 4,  ///< Number of widgets needed for each news category, starting at widget #WID_MO_START_OPTION.

	MOS_LEFT_EDGE             = 6,  ///< Number of pixels between left edge of the window and the options buttons column.
	MOS_COLUMN_SPACING        = 4,  ///< Number of pixels between the buttons and the description columns.
	MOS_RIGHT_EDGE            = 6,  ///< Number of pixels between right edge of the window and the options descriptions column.
	MOS_BUTTON_SPACE          = 10, ///< Additional space in the button with the option value (for better looks).

	MOS_ABOVE_GLOBAL_SETTINGS = 6,  ///< Number of vertical pixels between the categories and the global options.
	MOS_BOTTOM_EDGE           = 6,  ///< Number of pixels between bottom edge of the window and bottom of the global options.
};

/**
 * Type of news.
 */
enum NewsType {
	NT_ARRIVAL_COMPANY, ///< First vehicle arrived for company
	NT_ARRIVAL_OTHER,   ///< First vehicle arrived for competitor
	NT_ACCIDENT,        ///< An accident or disaster has occurred
	NT_COMPANY_INFO,    ///< Company info (new companies, bankruptcy messages)
	NT_INDUSTRY_OPEN,   ///< Opening of industries
	NT_INDUSTRY_CLOSE,  ///< Closing of industries
	NT_ECONOMY,         ///< Economic changes (recession, industry up/dowm)
	NT_INDUSTRY_COMPANY,///< Production changes of industry serviced by local company
	NT_INDUSTRY_OTHER,  ///< Production changes of industry serviced by competitor(s)
	NT_INDUSTRY_NOBODY, ///< Other industry production changes
	NT_ADVICE,          ///< Bits of news about vehicles of the company
	NT_NEW_VEHICLES,    ///< New vehicle has become available
	NT_ACCEPTANCE,      ///< A type of cargo is (no longer) accepted
	NT_SUBSIDIES,       ///< News about subsidies (announcements, expirations, acceptance)
	NT_GENERAL,         ///< General news (from towns)
	NT_END,             ///< end-of-array marker
};

/**
 * References to objects in news.
 *
 * @warning
 * Be careful!
 * Vehicles are a special case, as news are kept when vehicles are autoreplaced/renewed.
 * You have to make sure, #ChangeVehicleNews catches the DParams of your message.
 * This is NOT ensured by the references.
 */
enum NewsReferenceType {
	NR_NONE,      ///< Empty reference
	NR_TILE,      ///< Reference tile.     Scroll to tile when clicking on the news.
	NR_VEHICLE,   ///< Reference vehicle.  Scroll to vehicle when clicking on the news. Delete news when vehicle is deleted.
	NR_STATION,   ///< Reference station.  Scroll to station when clicking on the news. Delete news when station is deleted.
	NR_INDUSTRY,  ///< Reference industry. Scroll to industry when clicking on the news. Delete news when industry is deleted.
	NR_TOWN,      ///< Reference town.     Scroll to town when clicking on the news.
	NR_ENGINE,    ///< Reference engine.
};

/**
 * Various OR-able news-item flags.
 * @note #NF_INCOLOUR is set automatically if needed.
 */
enum NewsFlag {
	NFB_WINDOW_LAYOUT  = 0,                      ///< First bit for window layout.
	NFB_WINDOW_LAYOUT_COUNT = 3,                 ///< Number of bits for window layout.
	NFB_INCOLOUR       = 3,                      ///< News item is shown in colour (otherwise it is shown in black & white).
	NFB_SHADE          = 4,                      ///< Disable transparency in the viewport and shade colours.
	NFB_VEHICLE_PARAM0 = 5,                      ///< String param 0 contains a vehicle ID. (special autoreplace behaviour)

	NF_INCOLOUR       = 1 << NFB_INCOLOUR,       ///< Bit value for coloured news.
	NF_SHADE          = 1 << NFB_SHADE,          ///< Bit value for enabling shading.
	NF_VEHICLE_PARAM0 = 1 << NFB_VEHICLE_PARAM0, ///< Bit value for specifying that string param 0 contains a vehicle ID. (special autoreplace behaviour)

	NF_THIN           = 0 << NFB_WINDOW_LAYOUT,  ///< Thin news item. (Newspaper with headline and viewport)
	NF_SMALL          = 1 << NFB_WINDOW_LAYOUT,  ///< Small news item. (Information window with text and viewport)
	NF_NORMAL         = 2 << NFB_WINDOW_LAYOUT,  ///< Normal news item. (Newspaper with text only)
	NF_VEHICLE        = 3 << NFB_WINDOW_LAYOUT,  ///< Vehicle news item. (new engine available)
	NF_COMPANY        = 4 << NFB_WINDOW_LAYOUT,  ///< Company news item. (Newspaper with face)

	NF_SHADE_THIN     = NF_SHADE | NF_THIN,      ///< Often-used combination
};


/**
 * News display options
 */
enum NewsDisplay {
	ND_OFF,        ///< Only show a reminder in the status bar
	ND_SUMMARY,    ///< Show ticker
	ND_FULL,       ///< Show newspaper
};

/**
 * Per-NewsType data
 */
struct NewsTypeData {
	const char * const name;    ///< Name
	const byte age;             ///< Maximum age of news items (in days)
	const SoundFx sound;        ///< Sound

	/**
	 * Construct this entry.
	 * @param name The name of the type.
	 * @param age The maximum age for these messages.
	 * @param sound The sound to play.
	 */
	CONSTEXPR NewsTypeData(const char *name, byte age, SoundFx sound) :
		name(name),
		age(age),
		sound(sound)
	{
	}

	NewsDisplay GetDisplay() const;
};

/** Information about a single item of news. */
struct NewsItem {
	NewsItem *prev;              ///< Previous news item
	NewsItem *next;              ///< Next news item
	StringID string_id;          ///< Message text
	Date date;                   ///< Date of the news
	NewsType type;               ///< Type of the news
	NewsFlag flags;              ///< NewsFlags bits @see NewsFlag

	NewsReferenceType reftype1;  ///< Type of ref1
	NewsReferenceType reftype2;  ///< Type of ref2
	uint32 ref1;                 ///< Reference 1 to some object: Used for a possible viewport, scrolling after clicking on the news, and for deleteing the news when the object is deleted.
	uint32 ref2;                 ///< Reference 2 to some object: Used for scrolling after clicking on the news, and for deleteing the news when the object is deleted.

	NewsItem (StringID string, NewsType type, NewsFlag flags,
			NewsReferenceType reftype1 = NR_NONE, uint32 ref1 = UINT32_MAX,
			NewsReferenceType reftype2 = NR_NONE, uint32 ref2 = UINT32_MAX)
		: prev(NULL), next(NULL),
		  string_id(string), date(_date), type(type),
		  flags (_cur_year < _settings_client.gui.coloured_news_year ? flags : (NewsFlag)(flags | NF_INCOLOUR)),
		  reftype1(reftype1), reftype2(reftype2),
		  ref1(ref1), ref2(ref2)
	{
	}

	virtual ~NewsItem() { }

	uint64 params[10]; ///< Parameters for string resolving.
};

/** NewsItem derived class template. */
template <NewsFlag f, NewsReferenceType reftype, typename T>
struct RefNewsItem : NewsItem {
	RefNewsItem (StringID string, NewsType type, T ref)
		: NewsItem (string, type, f, reftype, ref)
	{
	}

	template <typename P0>
	RefNewsItem (StringID string, NewsType type, T ref, P0 param0)
		: NewsItem (string, type, f, reftype, ref)
	{
		this->params[0] = param0;
	}

	template <typename P0, typename P1>
	RefNewsItem (StringID string, NewsType type, T ref,
			P0 param0, P1 param1)
		: NewsItem (string, type, f, reftype, ref)
	{
		this->params[0] = param0;
		this->params[1] = param1;
	}

	template <typename P0, typename P1, typename P2>
	RefNewsItem (StringID string, NewsType type, T ref,
			P0 param0, P1 param1, P2 param2)
		: NewsItem (string, type, f, reftype, ref)
	{
		this->params[0] = param0;
		this->params[1] = param1;
		this->params[2] = param2;
	}
};

/** News linked to a tile on the map. */
typedef RefNewsItem <NF_SHADE_THIN, NR_TILE, TileIndex> TileNewsItem;

/** News about an industry. */
typedef RefNewsItem <NF_SHADE_THIN, NR_INDUSTRY, IndustryID> IndustryNewsItem;

/** Base class for VehicleNewsItem. */
struct BaseVehicleNewsItem : NewsItem {
	BaseVehicleNewsItem (StringID string, NewsType type, VehicleID vid,
			const struct Station *st = NULL);
};

/**
 * News involving a vehicle.
 * @warning The params may not reference the vehicle due to autoreplacing.
 * See VehicleAdviceNewsItem below for that.
 */
struct VehicleNewsItem : BaseVehicleNewsItem {
	VehicleNewsItem (StringID string, NewsType type, VehicleID vid)
		: BaseVehicleNewsItem (string, type, vid)
	{
	}

	template <typename P>
	VehicleNewsItem (StringID string, NewsType type, VehicleID vid, P p)
		: BaseVehicleNewsItem (string, type, vid)
	{
		this->params[0] = p;
	}
};

/** News about arrival of first vehicle to a station. */
struct ArrivalNewsItem : BaseVehicleNewsItem {
	ArrivalNewsItem (StringID string, const struct Vehicle *v,
			const struct Station *st);
};

/** News about a plane crash. */
struct PlaneCrashNewsItem : BaseVehicleNewsItem {
	PlaneCrashNewsItem (VehicleID vid, const struct Station *st, uint pass);
};

/** Base class for VehicleAdviceNewsItem. */
typedef RefNewsItem <(NewsFlag)(NF_INCOLOUR | NF_SMALL | NF_VEHICLE_PARAM0), NR_VEHICLE, VehicleID> BaseVehicleAdviceNewsItem;

/** Advice about a vehicle. */
struct VehicleAdviceNewsItem : BaseVehicleAdviceNewsItem {
	VehicleAdviceNewsItem (StringID string, VehicleID vid)
		: BaseVehicleAdviceNewsItem (string, NT_ADVICE, vid, vid)
	{
	}

	template <typename P>
	VehicleAdviceNewsItem (StringID string, VehicleID vid, P param)
		: BaseVehicleAdviceNewsItem (string, NT_ADVICE, vid, vid, param)
	{
	}
};

/** News about change of acceptance at a station. */
struct AcceptanceNewsItem : NewsItem {
	AcceptanceNewsItem (const struct Station *st,
			uint num_items, CargoID *cargo, StringID msg);
};

/** News about founding of a town. */
struct FoundTownNewsItem : TileNewsItem {
	ttd_unique_free_ptr <char> data;

	FoundTownNewsItem (TownID tid, TileIndex tile, const char *company_name);
};

/** News about road rebuilding in a town. */
struct RoadRebuildNewsItem : NewsItem {
	ttd_unique_free_ptr <char> data;

	RoadRebuildNewsItem (TownID tid, const char *company_name);
};

/** News about availability of a new vehicle (engine). */
struct EngineNewsItem : NewsItem {
	EngineNewsItem (EngineID eid);
};

/** News about a subsidy. */
struct SubsidyNewsItem : NewsItem {
	SubsidyNewsItem (StringID string, const struct Subsidy *s,
			bool plural, uint offset = 0);
};

/** News about a subsidy award. */
struct SubsidyAwardNewsItem : SubsidyNewsItem {
	ttd_unique_free_ptr <char> data;

	SubsidyAwardNewsItem (const struct Subsidy *s, const char *company_name);
};

/**
 * Data that needs to be stored for company news messages.
 * The problem with company news messages are the custom name
 * of the companies and the fact that the company data is reset,
 * resulting in wrong names and such.
 */
struct CompanyNewsInformation {
	char company_name[64];       ///< The name of the company
	char president_name[64];     ///< The name of the president
	char other_company_name[64]; ///< The name of the company taking over this one

	uint32 face; ///< The face of the president
	byte colour; ///< The colour related to the company

	void FillData(const struct Company *c, const struct Company *other = NULL);
};

/** Base NewsItem for news about a company. */
struct BaseCompanyNewsItem : NewsItem {
	ttd_unique_free_ptr <CompanyNewsInformation> data;

	BaseCompanyNewsItem (NewsType type, StringID str,
		const struct Company *c, const struct Company *other,
		NewsReferenceType reftype = NR_NONE, uint32 ref = UINT32_MAX);
};

/** Generic news about a company. */
struct CompanyNewsItem : BaseCompanyNewsItem {
	CompanyNewsItem (StringID str1, StringID str2, const struct Company *c);
};

/** News about a company launch. */
struct LaunchNewsItem : BaseCompanyNewsItem {
	LaunchNewsItem (const struct Company *c, TownID tid);
};

/** News about a company merger. */
struct MergerNewsItem : BaseCompanyNewsItem {
	MergerNewsItem (const struct Company *c, const struct Company *merger);
};

/** News about exclusive transport rights. */
struct ExclusiveRightsNewsItem : BaseCompanyNewsItem {
	ExclusiveRightsNewsItem (TownID tid, const struct Company *c);
};

#endif /* NEWS_TYPE_H */
