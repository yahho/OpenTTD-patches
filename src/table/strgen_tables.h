/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file table/strgen_tables.h Tables of commands for strgen */

#include "../core/enum_type.hpp"

enum CmdFlags {
	C_NONE      = 0x0, ///< Nothing special about this command
	C_DONTCOUNT = 0x1, ///< These commands aren't counted for comparison
	C_CASE      = 0x2, ///< These commands support cases
	C_GENDER    = 0x4, ///< These commands support genders
};
DECLARE_ENUM_AS_BIT_SET(CmdFlags)

struct CmdStruct {
	const char *cmd;
	long value;
	uint8 consumes;
	int8 default_plural_offset;
	CmdFlags flags;
};

static const CmdStruct _cmd_structs[] = {
	/* Font size */
	{"TINY_FONT",         SCC_TINYFONT,           0, -1, C_NONE},
	{"BIG_FONT",          SCC_BIGFONT,            0, -1, C_NONE},

	/* Colours */
	{"BLUE",              SCC_BLUE,               0, -1, C_DONTCOUNT},
	{"SILVER",            SCC_SILVER,             0, -1, C_DONTCOUNT},
	{"GOLD",              SCC_GOLD,               0, -1, C_DONTCOUNT},
	{"RED",               SCC_RED,                0, -1, C_DONTCOUNT},
	{"PURPLE",            SCC_PURPLE,             0, -1, C_DONTCOUNT},
	{"LTBROWN",           SCC_LTBROWN,            0, -1, C_DONTCOUNT},
	{"ORANGE",            SCC_ORANGE,             0, -1, C_DONTCOUNT},
	{"GREEN",             SCC_GREEN,              0, -1, C_DONTCOUNT},
	{"YELLOW",            SCC_YELLOW,             0, -1, C_DONTCOUNT},
	{"DKGREEN",           SCC_DKGREEN,            0, -1, C_DONTCOUNT},
	{"CREAM",             SCC_CREAM,              0, -1, C_DONTCOUNT},
	{"BROWN",             SCC_BROWN,              0, -1, C_DONTCOUNT},
	{"WHITE",             SCC_WHITE,              0, -1, C_DONTCOUNT},
	{"LTBLUE",            SCC_LTBLUE,             0, -1, C_DONTCOUNT},
	{"GRAY",              SCC_GRAY,               0, -1, C_DONTCOUNT},
	{"DKBLUE",            SCC_DKBLUE,             0, -1, C_DONTCOUNT},
	{"BLACK",             SCC_BLACK,              0, -1, C_DONTCOUNT},

	{"REV",               SCC_REVISION,           0, -1, C_NONE}, // openttd revision string

	{"STRING1",           SCC_STRING1,            2, -1, C_CASE | C_GENDER}, // included string that consumes the string id and ONE argument
	{"STRING2",           SCC_STRING2,            3, -1, C_CASE | C_GENDER}, // included string that consumes the string id and TWO arguments
	{"STRING3",           SCC_STRING3,            4, -1, C_CASE | C_GENDER}, // included string that consumes the string id and THREE arguments
	{"STRING4",           SCC_STRING4,            5, -1, C_CASE | C_GENDER}, // included string that consumes the string id and FOUR arguments
	{"STRING5",           SCC_STRING5,            6, -1, C_CASE | C_GENDER}, // included string that consumes the string id and FIVE arguments
	{"STRING6",           SCC_STRING6,            7, -1, C_CASE | C_GENDER}, // included string that consumes the string id and SIX arguments
	{"STRING7",           SCC_STRING7,            8, -1, C_CASE | C_GENDER}, // included string that consumes the string id and SEVEN arguments

	{"STATION_FEATURES",  SCC_STATION_FEATURES,   1, -1, C_NONE}, // station features string, icons of the features
	{"INDUSTRY",          SCC_INDUSTRY_NAME,      1, -1, C_CASE | C_GENDER}, // industry, takes an industry #, can have cases
	{"CARGO_LONG",        SCC_CARGO_LONG,         2,  1, C_NONE | C_GENDER},
	{"CARGO_SHORT",       SCC_CARGO_SHORT,        2,  1, C_NONE}, // short cargo description, only ### tons, or ### litres
	{"CARGO_TINY",        SCC_CARGO_TINY,         2,  1, C_NONE}, // tiny cargo description with only the amount, not a specifier for the amount or the actual cargo name
	{"CARGO_LIST",        SCC_CARGO_LIST,         1, -1, C_CASE},
	{"POWER",             SCC_POWER,              1,  0, C_NONE},
	{"VOLUME_LONG",       SCC_VOLUME_LONG,        1,  0, C_NONE},
	{"VOLUME_SHORT",      SCC_VOLUME_SHORT,       1,  0, C_NONE},
	{"WEIGHT_LONG",       SCC_WEIGHT_LONG,        1,  0, C_NONE},
	{"WEIGHT_SHORT",      SCC_WEIGHT_SHORT,       1,  0, C_NONE},
	{"FORCE",             SCC_FORCE,              1,  0, C_NONE},
	{"VELOCITY",          SCC_VELOCITY,           1,  0, C_NONE},
	{"HEIGHT",            SCC_HEIGHT,             1,  0, C_NONE},

	{"P",                 SCC_PLURAL_LIST,        0, -1, C_DONTCOUNT}, // plural specifier
	{"G",                 SCC_GENDER_LIST,        0, -1, C_DONTCOUNT}, // gender specifier

	{"DATE_TINY",         SCC_DATE_TINY,          1, -1, C_NONE},
	{"DATE_SHORT",        SCC_DATE_SHORT,         1, -1, C_CASE},
	{"DATE_LONG",         SCC_DATE_LONG,          1, -1, C_CASE},
	{"DATE_ISO",          SCC_DATE_ISO,           1, -1, C_NONE},

	{"STRING",            SCC_STRING,             1, -1, C_CASE | C_GENDER},
	{"RAW_STRING",        SCC_RAW_STRING_POINTER, 1, -1, C_NONE | C_GENDER},

	/* Numbers */
	{"COMMA",             SCC_COMMA,              1,  0, C_NONE}, // Number with comma
	{"DECIMAL",           SCC_DECIMAL,            2,  0, C_NONE}, // Number with comma and fractional part. Second parameter is number of fractional digits, first parameter is number times 10**(second parameter).
	{"NUM",               SCC_NUM,                1,  0, C_NONE}, // Signed number
	{"ZEROFILL_NUM",      SCC_ZEROFILL_NUM,       2,  0, C_NONE}, // Unsigned number with zero fill, e.g. "02". First parameter is number, second minimum length
	{"BYTES",             SCC_BYTES,              1,  0, C_NONE}, // Unsigned number with "bytes", i.e. "1.02 MiB or 123 KiB"
	{"HEX",               SCC_HEX,                1,  0, C_NONE}, // Hexadecimally printed number

	{"CURRENCY_LONG",     SCC_CURRENCY_LONG,      1,  0, C_NONE},
	{"CURRENCY_SHORT",    SCC_CURRENCY_SHORT,     1,  0, C_NONE}, // compact currency

	{"WAYPOINT",          SCC_WAYPOINT_NAME,      1, -1, C_NONE | C_GENDER}, // waypoint name
	{"STATION",           SCC_STATION_NAME,       1, -1, C_NONE | C_GENDER},
	{"DEPOT",             SCC_DEPOT_NAME,         2, -1, C_NONE | C_GENDER},
	{"TOWN",              SCC_TOWN_NAME,          1, -1, C_NONE | C_GENDER},
	{"GROUP",             SCC_GROUP_NAME,         1, -1, C_NONE | C_GENDER},
	{"SIGN",              SCC_SIGN_NAME,          1, -1, C_NONE | C_GENDER},
	{"ENGINE",            SCC_ENGINE_NAME,        1, -1, C_NONE | C_GENDER},
	{"VEHICLE",           SCC_VEHICLE_NAME,       1, -1, C_NONE | C_GENDER},
	{"COMPANY",           SCC_COMPANY_NAME,       1, -1, C_NONE | C_GENDER},
	{"COMPANY_NUM",       SCC_COMPANY_NUM,        1, -1, C_NONE},
	{"PRESIDENT_NAME",    SCC_PRESIDENT_NAME,     1, -1, C_NONE | C_GENDER},

	{"",                  '\n',                   0, -1, C_DONTCOUNT},
	{"{",                 '{',                    0, -1, C_DONTCOUNT},
	{"UP_ARROW",          SCC_UP_ARROW,           0, -1, C_DONTCOUNT},
	{"SMALL_UP_ARROW",    SCC_SMALL_UP_ARROW,     0, -1, C_DONTCOUNT},
	{"SMALL_DOWN_ARROW",  SCC_SMALL_DOWN_ARROW,   0, -1, C_DONTCOUNT},
	{"TRAIN",             SCC_TRAIN,              0, -1, C_DONTCOUNT},
	{"LORRY",             SCC_LORRY,              0, -1, C_DONTCOUNT},
	{"BUS",               SCC_BUS,                0, -1, C_DONTCOUNT},
	{"PLANE",             SCC_PLANE,              0, -1, C_DONTCOUNT},
	{"SHIP",              SCC_SHIP,               0, -1, C_DONTCOUNT},
	{"NBSP",              0xA0,                   0, -1, C_DONTCOUNT},
	{"COPYRIGHT",         0xA9,                   0, -1, C_DONTCOUNT},
	{"DOWN_ARROW",        SCC_DOWN_ARROW,         0, -1, C_DONTCOUNT},
	{"CHECKMARK",         SCC_CHECKMARK,          0, -1, C_DONTCOUNT},
	{"CROSS",             SCC_CROSS,              0, -1, C_DONTCOUNT},
	{"RIGHT_ARROW",       SCC_RIGHT_ARROW,        0, -1, C_DONTCOUNT},
	{"SMALL_LEFT_ARROW",  SCC_LESS_THAN,          0, -1, C_DONTCOUNT},
	{"SMALL_RIGHT_ARROW", SCC_GREATER_THAN,       0, -1, C_DONTCOUNT},

	/* The following are directional formatting codes used to get the RTL strings right:
	 * http://www.unicode.org/unicode/reports/tr9/#Directional_Formatting_Codes */
	{"LRM",               CHAR_TD_LRM,            0, -1, C_DONTCOUNT},
	{"RLM",               CHAR_TD_RLM,            0, -1, C_DONTCOUNT},
	{"LRE",               CHAR_TD_LRE,            0, -1, C_DONTCOUNT},
	{"RLE",               CHAR_TD_RLE,            0, -1, C_DONTCOUNT},
	{"LRO",               CHAR_TD_LRO,            0, -1, C_DONTCOUNT},
	{"RLO",               CHAR_TD_RLO,            0, -1, C_DONTCOUNT},
	{"PDF",               CHAR_TD_PDF,            0, -1, C_DONTCOUNT},
};

/** Description of a plural form */
struct PluralForm {
	int plural_count;        ///< The number of plural forms
	const char *description; ///< Human readable description of the form
	const char *names;       ///< Plural names
};

/** The maximum number of plurals. */
static const int MAX_PLURALS = 5;

/** All plural forms used */
static const PluralForm _plural_forms[] = {
	{ 2, "Two forms: special case for 1.", "\"1\" \"other\"" },
	{ 1, "Only one form.", "\"other\"" },
	{ 2, "Two forms: special case for 0 to 1.", "\"0..1\" \"other\"" },
	{ 3, "Three forms: special cases for 0, and numbers ending in 1 except when ending in 11.", "\"1,21,31,...\" \"other\" \"0\"" },
	{ 5, "Five forms: special cases for 1, 2, 3 to 6, and 7 to 10.", "\"1\" \"2\" \"3..6\" \"7..10\" \"other\"" },
	{ 3, "Three forms: special cases for numbers ending in 1 except when ending in 11, and 2 to 9 except when ending in 12 to 19.", "\"1,21,31,...\" \"2..9,22..29,32..39,...\" \"other\"" },
	{ 3, "Three forms: special cases for numbers ending in 1 except when ending in 11, and 2 to 4 except when ending in 12 to 14.", "\"1,21,31,...\" \"2..4,22..24,32..34,...\" \"other\"" },
	{ 3, "Three forms: special cases for 1, and numbers ending in 2 to 4 except when ending in 12 to 14.", "\"1\" \"2..4,22..24,32..34,...\" \"other\"" },
	{ 4, "Four forms: special cases for numbers ending in 01, 02, and 03 to 04.", "\"1,101,201,...\" \"2,102,202,...\" \"3..4,103..104,203..204,...\" \"other\"" },
	{ 2, "Two forms: special case for numbers ending in 1 except when ending in 11.", "\"1,21,31,...\" \"other\"" },
	{ 3, "Three forms: special cases for 1, and 2 to 4.", "\"1\" \"2..4\" \"other\"" },
	{ 2, "Two forms: cases for numbers ending with a consonant, and with a vowel.", "\"yeong,il,sam,yuk,chil,pal\" \"i,sa,o,gu\"" },
	{ 4, "Four forms: special cases for 1, 0 and numbers ending in 02 to 10, and numbers ending in 11 to 19.", "\"1\" \"0,2..10,102..110,202..210,...\" \"11..19,111..119,211..219,...\" \"other\"" },
	{ 4, "Four forms: special cases for 1 and 11, 2 and 12, 3..10 and 13..19.", "\"1,11\" \"2,12\" \"3..10,13..19\" \"other\"" },
};

/* Flags:
 * 0 = nothing
 * t = translator editable
 * l = ltr/rtl choice
 * p = plural choice
 * d = separator char (replace spaces with {NBSP})
 * x1 = hexadecimal number of 1 byte
 * x2 = hexadecimal number of 2 bytes
 * g = gender
 * c = cases
 * a = array, i.e. list of strings
 */
 /** All pragmas used */
static const char * const _pragmas[][4] = {
	/*  name         flags  default   description */
	{ "name",        "0",   "",       "English name for the language" },
	{ "ownname",     "t",   "",       "Localised name for the language" },
	{ "isocode",     "0",   "",       "ISO code for the language" },
	{ "plural",      "tp",  "0",      "Plural form to use" },
	{ "textdir",     "tl",  "ltr",    "Text direction. Either ltr (left-to-right) or rtl (right-to-left)" },
	{ "digitsep",    "td",  ",",      "Digit grouping separator for non-currency numbers" },
	{ "digitsepcur", "td",  ",",      "Digit grouping separator for currency numbers" },
	{ "decimalsep",  "td",  ".",      "Decimal separator" },
	{ "winlangid",   "x2",  "0x0000", "Language ID for Windows" },
	{ "grflangid",   "x1",  "0x00",   "Language ID for NewGRFs" },
	{ "gender",      "tag", "",       "List of genders" },
	{ "case",        "tac", "",       "List of cases" },
};
