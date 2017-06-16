/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file town_land.h Sprites to use and how to display them for town tiles. */

/** House graphics data struct. */
struct DrawBuildingsTileStruct {
	PalSpriteID ground;
	PalSpriteID building;
	byte subtile_x;
	byte subtile_y;
	byte width;
	byte height;
	byte dz;
	byte draw_proc;  // this allows to specify a special drawing procedure.
};

/** House graphics data. */

static const DrawBuildingsTileStruct dbts_bare        = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_bare_u      = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_058d   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x58d,                   PAL_NONE },   0,  0, 14, 14,   8, 0 };
static const DrawBuildingsTileStruct dbts_bare_058e   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x58e,                   PAL_NONE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_058f   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x58f,                   PAL_NONE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_0590_058f   = { {  0x590,                   PAL_NONE }, {  0x58f,                   PAL_NONE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_0591   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x591,                   PAL_NONE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_0590_0591   = { {  0x590,                   PAL_NONE }, {  0x591,                   PAL_NONE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_058d_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x58d,    PALETTE_TO_STRUCT_WHITE },   0,  0, 14, 14,   8, 0 };
static const DrawBuildingsTileStruct dbts_bare_058e_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x58e,    PALETTE_TO_STRUCT_WHITE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_0591_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x591,    PALETTE_TO_STRUCT_WHITE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_0590_0591_w = { {  0x590,                   PAL_NONE }, {  0x591,    PALETTE_TO_STRUCT_WHITE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_058d_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x58d, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 14, 14,   8, 0 };
static const DrawBuildingsTileStruct dbts_bare_058e_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x58e, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_0591_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x591, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 14, 14,  60, 0 };
static const DrawBuildingsTileStruct dbts_0590_0591_c = { {  0x590,                   PAL_NONE }, {  0x591, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 14, 14,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_0592   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x592,                   PAL_NONE },   0,  0, 14, 16,  11, 0 };
static const DrawBuildingsTileStruct dbts_bare_0593   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x593,                   PAL_NONE },   0,  0, 14, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_bare_0594   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x594,                   PAL_NONE },   0,  0, 14, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_0595_0594   = { {  0x595,                   PAL_NONE }, {  0x594,                   PAL_NONE },   0,  0, 14, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_bare_0592_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x592,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  11, 0 };
static const DrawBuildingsTileStruct dbts_bare_0593_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x593,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_bare_0594_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x594,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_0595_0594_w = { {  0x595,                   PAL_NONE }, {  0x594,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_bare_0592_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x592, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 16, 16,  11, 0 };
static const DrawBuildingsTileStruct dbts_bare_0593_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x593, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_bare_0594_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x594, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_0595_0594_c = { {  0x595,                   PAL_NONE }, {  0x594, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_bare_0592_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x592,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  11, 0 };
static const DrawBuildingsTileStruct dbts_bare_0593_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x593,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_bare_0594_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x594,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  33, 0 };
static const DrawBuildingsTileStruct dbts_0595_0594_n = { {  0x595,                   PAL_NONE }, {  0x594,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  33, 0 };

static const DrawBuildingsTileStruct dbts_bare_0596   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x596,                   PAL_NONE },   0,  0, 12, 12,   6, 0 };
static const DrawBuildingsTileStruct dbts_bare_0597   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x597,                   PAL_NONE },   0,  0, 12, 12,  17, 0 };
static const DrawBuildingsTileStruct dbts_bare_0598   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x598,                   PAL_NONE },   0,  0, 12, 12,  17, 0 };
static const DrawBuildingsTileStruct dbts_0599_0598   = { {  0x599,                   PAL_NONE }, {  0x598,                   PAL_NONE },   0,  0, 12, 12,  17, 0 };

static const DrawBuildingsTileStruct dbts_bare_059a   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59a,                   PAL_NONE },   0,  0, 14, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_bare_059b   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59b,                   PAL_NONE },   0,  0, 14, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_bare_059c   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59c,                   PAL_NONE },   0,  0, 14, 16,  35, 0 };
static const DrawBuildingsTileStruct dbts_059d_059c   = { {  0x59d,                   PAL_NONE }, {  0x59c,                   PAL_NONE },   0,  0, 14, 16,  35, 0 };
static const DrawBuildingsTileStruct dbts_bare_059a_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59a,         PALETTE_CHURCH_RED },   0,  0, 14, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_bare_059b_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59b,         PALETTE_CHURCH_RED },   0,  0, 14, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_bare_059c_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59c,         PALETTE_CHURCH_RED },   0,  0, 14, 16,  35, 0 };
static const DrawBuildingsTileStruct dbts_059d_059c_r = { {  0x59d,                   PAL_NONE }, {  0x59c,         PALETTE_CHURCH_RED },   0,  0, 14, 16,  35, 0 };
static const DrawBuildingsTileStruct dbts_bare_059a_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59a,       PALETTE_CHURCH_CREAM },   0,  0, 14, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_bare_059b_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59b,       PALETTE_CHURCH_CREAM },   0,  0, 14, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_bare_059c_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x59c,       PALETTE_CHURCH_CREAM },   0,  0, 14, 16,  35, 0 };
static const DrawBuildingsTileStruct dbts_059d_059c_c = { {  0x59d,                   PAL_NONE }, {  0x59c,       PALETTE_CHURCH_CREAM },   0,  0, 14, 16,  35, 0 };

static const DrawBuildingsTileStruct dbts_bare_05a0   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a0,                   PAL_NONE },   0,  0, 15, 15,   5, 0 };
static const DrawBuildingsTileStruct dbts_bare_05a1   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a1,                   PAL_NONE },   0,  0, 15, 15,  53, 0 };

static const DrawBuildingsTileStruct dbts_bare_05a2   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a2,                   PAL_NONE },   0,  0, 15, 15,  53, 0 };
static const DrawBuildingsTileStruct dbts_conc_05a2   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5a2,                   PAL_NONE },   0,  0, 15, 15,  53, 1 };

static const DrawBuildingsTileStruct dbts_bare_11d9   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11d9,                   PAL_NONE },   0,  0, 15, 15,  53, 0 };
static const DrawBuildingsTileStruct dbts_conc_11d9   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, { 0x11d9,                   PAL_NONE },   0,  0, 15, 15,  53, 1 };

static const DrawBuildingsTileStruct dbts_bare_05a4   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a4,                   PAL_NONE },   0,  0, 16, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_bare_05a5   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a5,                   PAL_NONE },   0,  0, 16, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_bare_05a6   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a6,                   PAL_NONE },   0,  0, 16, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_05a7_05a6   = { {  0x5a7,                   PAL_NONE }, {  0x5a6,                   PAL_NONE },   0,  0, 16, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_05dd_05de   = { {  0x5dd,                   PAL_NONE }, {  0x5de,                   PAL_NONE },   0,  0, 16, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_05df_05e0   = { {  0x5df,                   PAL_NONE }, {  0x5e0,                   PAL_NONE },   0,  0, 16, 16,  16, 0 };
static const DrawBuildingsTileStruct dbts_05e1_05e2   = { {  0x5e1,                   PAL_NONE }, {  0x5e2,                   PAL_NONE },   0,  0, 16, 16,  16, 0 };

static const DrawBuildingsTileStruct dbts_bare_05a8   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a8,                   PAL_NONE },   0,  0, 16, 16,  15, 0 };
static const DrawBuildingsTileStruct dbts_bare_05a9   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5a9,                   PAL_NONE },   0,  0, 16, 16,  38, 0 };
static const DrawBuildingsTileStruct dbts_bare_05aa   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5aa,                   PAL_NONE },   0,  0, 16, 16,  45, 0 };
static const DrawBuildingsTileStruct dbts_conc_05aa   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5aa,                   PAL_NONE },   0,  0, 16, 16,  45, 0 };

static const DrawBuildingsTileStruct dbts_bare_05ab   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5ab,                   PAL_NONE },   0,  0, 16, 16,  15, 0 };
static const DrawBuildingsTileStruct dbts_bare_05ac   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5ac,                   PAL_NONE },   0,  0, 16, 16,  38, 0 };
static const DrawBuildingsTileStruct dbts_bare_05ad   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5ad,                   PAL_NONE },   0,  0, 16, 16,  45, 0 };
static const DrawBuildingsTileStruct dbts_conc_05ad   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5ad,                   PAL_NONE },   0,  0, 16, 16,  45, 0 };

static const DrawBuildingsTileStruct dbts_bare_1      = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {    0x0,                   PAL_NONE },   6,  5,  3,  6,   8, 0 };
static const DrawBuildingsTileStruct dbts_conc_1      = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {    0x0,                   PAL_NONE },   6,  5,  3,  6,   8, 0 };
static const DrawBuildingsTileStruct dbts_conc_05ae   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5ae,                   PAL_NONE },   6,  5,  3,  6,   8, 0 };

static const DrawBuildingsTileStruct dbts_bare_2      = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {    0x0,                   PAL_NONE },   3,  3,  9,  9,   8, 0 };
static const DrawBuildingsTileStruct dbts_conc_2      = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {    0x0,                   PAL_NONE },   3,  3,  9,  9,   8, 0 };
static const DrawBuildingsTileStruct dbts_conc_05af   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5af,                   PAL_NONE },   3,  3,  9,  9,   8, 0 };

static const DrawBuildingsTileStruct dbts_bare_3      = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  15, 0 };
static const DrawBuildingsTileStruct dbts_grss_05b0   = { { SPR_FLAT_GRASS_TILE,      PAL_NONE }, {  0x5b0,                   PAL_NONE },   0,  0, 16, 16,  15, 0 };
static const DrawBuildingsTileStruct dbts_grss_05b1   = { { SPR_FLAT_GRASS_TILE,      PAL_NONE }, {  0x5b1,                   PAL_NONE },   0,  0, 16, 16,  15, 0 };

static const DrawBuildingsTileStruct dbts_bare_05b2   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b2,                   PAL_NONE },   0,  0, 16, 16,   8, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b3   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b3,                   PAL_NONE },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b4   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b4,                   PAL_NONE },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_conc_05b4   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5b4,                   PAL_NONE },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b2_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b2,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,   8, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b3_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b3,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b4_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b4,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_conc_05b4_w = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5b4,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b2_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b2,      PALETTE_TO_STRUCT_RED },   0,  0, 16, 16,   8, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b3_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b3,      PALETTE_TO_STRUCT_RED },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b4_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b4,      PALETTE_TO_STRUCT_RED },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_conc_05b4_r = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5b4,      PALETTE_TO_STRUCT_RED },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b2_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b2,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,   8, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b3_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b3,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b4_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b4,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_conc_05b4_n = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5b4,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  42, 0 };

static const DrawBuildingsTileStruct dbts_bare_05b5   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b5,                   PAL_NONE },   1,  3, 14, 11,   7, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b6   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b6,                   PAL_NONE },   1,  3, 14, 11,  53, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b7   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b7,                   PAL_NONE },   1,  3, 14, 11,  53, 0 };
static const DrawBuildingsTileStruct dbts_conc_05b7   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5b7,                   PAL_NONE },   1,  3, 14, 11,  53, 0 };

static const DrawBuildingsTileStruct dbts_bare_05b8   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b8,                   PAL_NONE },   3,  1, 11, 14,   6, 0 };
static const DrawBuildingsTileStruct dbts_bare_05b9   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5b9,                   PAL_NONE },   3,  1, 11, 14,  28, 0 };
static const DrawBuildingsTileStruct dbts_bare_05ba   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5ba,                   PAL_NONE },   3,  1, 11, 14,  28, 0 };
static const DrawBuildingsTileStruct dbts_conc_05ba   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5ba,                   PAL_NONE },   3,  1, 11, 14,  28, 0 };

static const DrawBuildingsTileStruct dbts_bare_05bb   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bb,                   PAL_NONE },   2,  0, 13, 16,   6, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bc   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bc,                   PAL_NONE },   2,  0, 13, 16,  45, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bd   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bd,                   PAL_NONE },   2,  0, 13, 16,  46, 0 };
static const DrawBuildingsTileStruct dbts_conc_05bd   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5bd,                   PAL_NONE },   2,  0, 13, 16,  46, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bb_b = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bb,     PALETTE_TO_STRUCT_BLUE },   2,  0, 13, 16,   6, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bc_b = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bc,     PALETTE_TO_STRUCT_BLUE },   2,  0, 13, 16,  45, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bd_b = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bd,     PALETTE_TO_STRUCT_BLUE },   2,  0, 13, 16,  46, 0 };
static const DrawBuildingsTileStruct dbts_conc_05bd_b = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5bd,     PALETTE_TO_STRUCT_BLUE },   2,  0, 13, 16,  46, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bb_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bb,    PALETTE_TO_STRUCT_WHITE },   2,  0, 13, 16,   6, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bc_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bc,    PALETTE_TO_STRUCT_WHITE },   2,  0, 13, 16,  45, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bd_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bd,    PALETTE_TO_STRUCT_WHITE },   2,  0, 13, 16,  46, 0 };
static const DrawBuildingsTileStruct dbts_conc_05bd_w = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5bd,    PALETTE_TO_STRUCT_WHITE },   2,  0, 13, 16,  46, 0 };

static const DrawBuildingsTileStruct dbts_bare_05be_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5be,             PALETTE_TO_RED },   2,  0, 13, 16,  13, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bf_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bf,             PALETTE_TO_RED },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c0_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c0,             PALETTE_TO_RED },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_conc_05c0_r = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5c0,             PALETTE_TO_RED },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_bare_05be_b = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5be,            PALETTE_TO_BLUE },   2,  0, 13, 16,  13, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bf_b = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bf,            PALETTE_TO_BLUE },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c0_b = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c0,            PALETTE_TO_BLUE },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_conc_05c0_b = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5c0,            PALETTE_TO_BLUE },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_bare_05be_o = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5be,          PALETTE_TO_ORANGE },   2,  0, 13, 16,  13, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bf_o = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bf,          PALETTE_TO_ORANGE },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c0_o = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c0,          PALETTE_TO_ORANGE },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_conc_05c0_o = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5c0,          PALETTE_TO_ORANGE },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_bare_05be_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5be,           PALETTE_TO_GREEN },   2,  0, 13, 16,  13, 0 };
static const DrawBuildingsTileStruct dbts_bare_05bf_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5bf,           PALETTE_TO_GREEN },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c0_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c0,           PALETTE_TO_GREEN },   2,  0, 13, 16, 110, 0 };
static const DrawBuildingsTileStruct dbts_conc_05c0_g = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5c0,           PALETTE_TO_GREEN },   2,  0, 13, 16, 110, 0 };

static const DrawBuildingsTileStruct dbts_bare_05c1   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c1,                   PAL_NONE },   1,  2, 15, 12,   4, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c2   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c2,                   PAL_NONE },   1,  2, 15, 12,  24, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c3   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c3,                   PAL_NONE },   1,  2, 15, 12,  31, 0 };
static const DrawBuildingsTileStruct dbts_conc_05c3   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5c3,                   PAL_NONE },   1,  2, 15, 12,  31, 0 };

static const DrawBuildingsTileStruct dbts_bare_05c4   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c4,                   PAL_NONE },   1,  0, 14, 15,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c5   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c5,                   PAL_NONE },   1,  0, 14, 15,  42, 0 };
static const DrawBuildingsTileStruct dbts_bare_05c6   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5c6,                   PAL_NONE },   1,  0, 14, 15,  42, 0 };
static const DrawBuildingsTileStruct dbts_conc_05c6   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, {  0x5c6,                   PAL_NONE },   1,  0, 14, 15,  42, 0 };

static const DrawBuildingsTileStruct dbts_stdn        = { { SPR_GRND_STADIUM_N,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_stdn_05cb   = { { SPR_GRND_STADIUM_N,       PAL_NONE }, {  0x5cb,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_stde        = { { SPR_GRND_STADIUM_E,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_stde_05cc   = { { SPR_GRND_STADIUM_E,       PAL_NONE }, {  0x5cc,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_stdw        = { { SPR_GRND_STADIUM_W,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_stdw_05cd   = { { SPR_GRND_STADIUM_W,       PAL_NONE }, {  0x5cd,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_stds        = { { SPR_GRND_STADIUM_S,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_stds_05ce   = { { SPR_GRND_STADIUM_S,       PAL_NONE }, {  0x5ce,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_bare_05d4   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5d4,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05d3_05d4   = { {  0x5d3,                   PAL_NONE }, {  0x5d4,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_bare_05d6   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5d6,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05d5_05d6   = { {  0x5d5,                   PAL_NONE }, {  0x5d6,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_bare_05d0   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5d0,                   PAL_NONE },   0,  0, 16, 16,  21, 0 };
static const DrawBuildingsTileStruct dbts_05cf_05d0   = { {  0x5cf,                   PAL_NONE }, {  0x5d0,                   PAL_NONE },   0,  0, 16, 16,  21, 0 };
static const DrawBuildingsTileStruct dbts_bare_05d2   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {  0x5d2,                   PAL_NONE },   0,  0, 16, 16,  11, 0 };
static const DrawBuildingsTileStruct dbts_05d1_05d2   = { {  0x5d1,                   PAL_NONE }, {  0x5d2,                   PAL_NONE },   0,  0, 16, 16,  11, 0 };

static const DrawBuildingsTileStruct dbts_05d7_05d8   = { {  0x5d7,                   PAL_NONE }, {  0x5d8,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };

static const DrawBuildingsTileStruct dbts_05d9        = { {  0x5d9,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05da        = { {  0x5da,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05db_05dc   = { {  0x5db,                   PAL_NONE }, {  0x5dc,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_0622_0623   = { {  0x622,                   PAL_NONE }, {  0x623,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_0624_0625   = { {  0x624,                   PAL_NONE }, {  0x625,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_0626_0627   = { {  0x626,                   PAL_NONE }, {  0x627,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05e3_05e4   = { {  0x5e3,                   PAL_NONE }, {  0x5e4,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05e5_05e6   = { {  0x5e5,                   PAL_NONE }, {  0x5e6,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05e7_05e8   = { {  0x5e7,                   PAL_NONE }, {  0x5e8,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05e9_05ea   = { {  0x5e9,                   PAL_NONE }, {  0x5ea,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05eb_05ec   = { {  0x5eb,                   PAL_NONE }, {  0x5ec,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05ed_05ee   = { {  0x5ed,                   PAL_NONE }, {  0x5ee,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };

static const DrawBuildingsTileStruct dbts_05ef        = { {  0x5ef,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05f0_05f1   = { {  0x5f0,                   PAL_NONE }, {  0x5f1,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05f2_05f3   = { {  0x5f2,                   PAL_NONE }, {  0x5f3,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05f4_05f5   = { {  0x5f4,                   PAL_NONE }, {  0x5f5,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05f6_05f7   = { {  0x5f6,                   PAL_NONE }, {  0x5f7,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };
static const DrawBuildingsTileStruct dbts_05f8_05f9   = { {  0x5f8,                   PAL_NONE }, {  0x5f9,                   PAL_NONE },   0,  0, 16, 16,  22, 0 };

static const DrawBuildingsTileStruct dbts_05fa_05fb   = { {  0x5fa,                   PAL_NONE }, {  0x5fb,                   PAL_NONE },   0,  0, 16, 16,  85, 0 };
static const DrawBuildingsTileStruct dbts_05fc_05fd   = { {  0x5fc,                   PAL_NONE }, {  0x5fd,                   PAL_NONE },   0,  0, 16, 16,  85, 0 };
static const DrawBuildingsTileStruct dbts_05fe_05ff   = { {  0x5fe,                   PAL_NONE }, {  0x5ff,                   PAL_NONE },   0,  0, 16, 16,  85, 0 };
static const DrawBuildingsTileStruct dbts_060a_060b   = { {  0x60a,                   PAL_NONE }, {  0x60b,                   PAL_NONE },   0,  0, 16, 16,  95, 0 };
static const DrawBuildingsTileStruct dbts_060c_060d   = { {  0x60c,                   PAL_NONE }, {  0x60d,                   PAL_NONE },   0,  0, 16, 16,  95, 0 };
static const DrawBuildingsTileStruct dbts_060e_060f   = { {  0x60e,                   PAL_NONE }, {  0x60f,                   PAL_NONE },   0,  0, 16, 16,  95, 0 };

static const DrawBuildingsTileStruct dbts_0600_0601   = { {  0x600,                   PAL_NONE }, {  0x601,                   PAL_NONE },   0,  0, 16, 16,  55, 0 };
static const DrawBuildingsTileStruct dbts_0600_0601_w = { {  0x600,    PALETTE_TO_STRUCT_WHITE }, {  0x601,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  55, 0 };
static const DrawBuildingsTileStruct dbts_0602_0603   = { {  0x602,                   PAL_NONE }, {  0x603,                   PAL_NONE },   0,  0, 16, 16,  42, 0 };
static const DrawBuildingsTileStruct dbts_0602_0603_c = { {  0x602, PALETTE_TO_STRUCT_CONCRETE }, {  0x603, PALETTE_TO_STRUCT_CONCRETE },   0,  0, 16, 16,  42, 0 };

static const DrawBuildingsTileStruct dbts_0604_0605   = { {  0x604,                   PAL_NONE }, {  0x605,                   PAL_NONE },   0,  0, 16, 16,  88, 0 };
static const DrawBuildingsTileStruct dbts_0606_0607   = { {  0x606,                   PAL_NONE }, {  0x607,                   PAL_NONE },   0,  0, 16, 16,  88, 0 };
static const DrawBuildingsTileStruct dbts_0608_0609   = { {  0x608,                   PAL_NONE }, {  0x609,                   PAL_NONE },   0,  0, 16, 16,  88, 0 };

static const DrawBuildingsTileStruct dbts_0610_0611   = { {  0x610,                   PAL_NONE }, {  0x611,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_0612        = { {  0x612,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_0612_0616   = { {  0x612,                   PAL_NONE }, {  0x616,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_0613        = { {  0x613,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_0613_0617   = { {  0x613,                   PAL_NONE }, {  0x617,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_0614        = { {  0x614,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_0614_0618   = { {  0x614,                   PAL_NONE }, {  0x618,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_0615        = { {  0x615,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_0615_0619   = { {  0x615,                   PAL_NONE }, {  0x619,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_061a_061b   = { {  0x61a,                   PAL_NONE }, {  0x61b,                   PAL_NONE },   0,  0, 16, 16, 100, 0 };
static const DrawBuildingsTileStruct dbts_061c_061d   = { {  0x61c,                   PAL_NONE }, {  0x61d,                   PAL_NONE },   0,  0, 16, 16, 100, 0 };

static const DrawBuildingsTileStruct dbts_061e_061f   = { {  0x61e,                   PAL_NONE }, {  0x61f,                   PAL_NONE },   0,  0, 16, 16,  25, 0 };
static const DrawBuildingsTileStruct dbts_061e_061f_w = { {  0x61e,    PALETTE_TO_STRUCT_WHITE }, {  0x61f,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  25, 0 };
static const DrawBuildingsTileStruct dbts_0620_0621   = { {  0x620,                   PAL_NONE }, {  0x621,                   PAL_NONE },   0,  0, 16, 16,  25, 0 };
static const DrawBuildingsTileStruct dbts_0620_0621_c = { {  0x620,           PALETTE_TO_CREAM }, {  0x621,           PALETTE_TO_CREAM },   0,  0, 16, 16,  25, 0 };

static const DrawBuildingsTileStruct dbts_11da_11db   = { { 0x11da,                   PAL_NONE }, { 0x11db,                   PAL_NONE },   0,  0, 16, 16,  25, 0 };
static const DrawBuildingsTileStruct dbts_11da_11db_w = { { 0x11da,    PALETTE_TO_STRUCT_WHITE }, { 0x11db,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  25, 0 };
static const DrawBuildingsTileStruct dbts_11dc_11dd   = { { 0x11dc,                   PAL_NONE }, { 0x11dd,                   PAL_NONE },   0,  0, 16, 16,  25, 0 };
static const DrawBuildingsTileStruct dbts_11dc_11dd_c = { { 0x11dc,           PALETTE_TO_CREAM }, { 0x11dd,           PALETTE_TO_CREAM },   0,  0, 16, 16,  25, 0 };

static const DrawBuildingsTileStruct dbts_1134_1135   = { { 0x1134,                   PAL_NONE }, { 0x1135,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_1136_1137   = { { 0x1136,                   PAL_NONE }, { 0x1137,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_113b_113c   = { { 0x113b,                   PAL_NONE }, { 0x113c,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_1138        = { { 0x1138,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_113d_113e   = { { 0x113d,                   PAL_NONE }, { 0x113e,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_1139        = { { 0x1139,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_113f_1140   = { { 0x113f,                   PAL_NONE }, { 0x1140,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_113a        = { { 0x113a,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_1141        = { { 0x1141,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_bare_1144   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1144,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1145   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1145,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1146   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1146,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1142_1146   = { { 0x1142,                   PAL_NONE }, { 0x1146,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1143_1147   = { { 0x1143,                   PAL_NONE }, { 0x1147,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_1148   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1148,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1149   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1149,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114a   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114a,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1142_114a   = { { 0x1142,                   PAL_NONE }, { 0x114a,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1143_114b   = { { 0x1143,                   PAL_NONE }, { 0x114b,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_114e   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114e,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114f   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114f,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1150   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1150,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114c_1150   = { { 0x114c,                   PAL_NONE }, { 0x1150,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114d_1151   = { { 0x114d,                   PAL_NONE }, { 0x1151,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114e_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114e,            PALETTE_TO_PINK },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114f_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114f,            PALETTE_TO_PINK },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1150_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1150,            PALETTE_TO_PINK },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114c_1150_p = { { 0x114c,                   PAL_NONE }, { 0x1150,            PALETTE_TO_PINK },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114d_1151_p = { { 0x114d,                   PAL_NONE }, { 0x1151,            PALETTE_TO_PINK },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114e_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114e,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114f_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114f,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1150_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1150,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114c_1150_c = { { 0x114c,                   PAL_NONE }, { 0x1150,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114d_1151_c = { { 0x114d,                   PAL_NONE }, { 0x1151,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114e_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114e,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_114f_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x114f,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1150_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1150,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114c_1150_g = { { 0x114c,                   PAL_NONE }, { 0x1150,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_114d_1151_g = { { 0x114d,                   PAL_NONE }, { 0x1151,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_1153   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1153,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1154   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1154,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1155   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1155,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_1155   = { { 0x1152,                   PAL_NONE }, { 0x1155,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11de   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11de,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_11de   = { { 0x1152,                   PAL_NONE }, { 0x11de,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1153_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1153,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1154_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1154,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1155_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1155,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_1155_r = { { 0x1152,                   PAL_NONE }, { 0x1155,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11de_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11de,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_11de_r = { { 0x1152,                   PAL_NONE }, { 0x11de,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1153_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1153,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1154_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1154,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1155_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1155,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_1155_g = { { 0x1152,                   PAL_NONE }, { 0x1155,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11de_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11de,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_11de_g = { { 0x1152,                   PAL_NONE }, { 0x11de,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1153_x = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1153,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1154_x = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1154,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1155_x = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1155,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_1155_x = { { 0x1152,                   PAL_NONE }, { 0x1155,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11de_x = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11de,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1152_11de_x = { { 0x1152,                   PAL_NONE }, { 0x11de,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_1157   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1157,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1158   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1158,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1159   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1159,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_1159   = { { 0x1156,                   PAL_NONE }, { 0x1159,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11df   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11df,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_11df   = { { 0x1156,                   PAL_NONE }, { 0x11df,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1157_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1157,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1158_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1158,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1159_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1159,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_1159_g = { { 0x1156,                   PAL_NONE }, { 0x1159,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11df_g = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11df,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_11df_g = { { 0x1156,                   PAL_NONE }, { 0x11df,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1157_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1157,           PALETTE_TO_CREAM },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1158_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1158,           PALETTE_TO_CREAM },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1159_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1159,           PALETTE_TO_CREAM },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_1159_c = { { 0x1156,                   PAL_NONE }, { 0x1159,           PALETTE_TO_CREAM },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11df_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11df,           PALETTE_TO_CREAM },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_11df_c = { { 0x1156,                   PAL_NONE }, { 0x11df,           PALETTE_TO_CREAM },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1157_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1157,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1158_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1158,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1159_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1159,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_1159_y = { { 0x1156,                   PAL_NONE }, { 0x1159,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11df_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11df,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1156_11df_y = { { 0x1156,                   PAL_NONE }, { 0x11df,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_115b   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115b,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115c   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115c,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115d   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115d,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_115d   = { { 0x115a,                   PAL_NONE }, { 0x115d,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e0   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e0,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_11e0   = { { 0x115a,                   PAL_NONE }, { 0x11e0,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115b_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115b,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115c_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115c,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115d_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115d,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_115d_r = { { 0x115a,                   PAL_NONE }, { 0x115d,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e0_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e0,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_11e0_r = { { 0x115a,                   PAL_NONE }, { 0x11e0,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115b_o = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115b,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115c_o = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115c,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115d_o = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115d,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_115d_o = { { 0x115a,                   PAL_NONE }, { 0x115d,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e0_o = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e0,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_11e0_o = { { 0x115a,                   PAL_NONE }, { 0x11e0,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115b_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115b,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115c_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115c,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_115d_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x115d,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_115d_n = { { 0x115a,                   PAL_NONE }, { 0x115d,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e0_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e0,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_115a_11e0_n = { { 0x115a,                   PAL_NONE }, { 0x11e0,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_1160   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1160,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_1161   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1161,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_1162   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1162,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_115e_1162   = { {             0x115e,       PAL_NONE }, { 0x1162,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_115f_1163   = { {             0x115f,       PAL_NONE }, { 0x1163,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_1166   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1166,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_1167   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1167,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_1168   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1168,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_1164_1168   = { {             0x1164,       PAL_NONE }, { 0x1168,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_1165_1169   = { {             0x1165,       PAL_NONE }, { 0x1169,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_116b        = { { 0x116b,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_116c   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x116c,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_116d   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x116d,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_116a_116d   = { { 0x116a,                   PAL_NONE }, { 0x116d,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e2   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e2,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_116a_11e2   = { { 0x116a,                   PAL_NONE }, { 0x11e2,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_116f        = { { 0x116f,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1170   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1170,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1171   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1171,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_116e_1171   = { { 0x116e,                   PAL_NONE }, { 0x1171,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e3   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e3,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_116e_11e3   = { { 0x116e,                   PAL_NONE }, { 0x11e3,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_1173   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1173,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1174   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1174,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1175   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1175,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1172_1175   = { { 0x1172,                   PAL_NONE }, { 0x1175,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_11e4_11e5   = { { 0x11e4,                   PAL_NONE }, { 0x11e5,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1173_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1173,           PALETTE_TO_BROWN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1174_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1174,           PALETTE_TO_BROWN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1175_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1175,           PALETTE_TO_BROWN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1172_1175_n = { { 0x1172,                   PAL_NONE }, { 0x1175,           PALETTE_TO_BROWN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_11e4_11e5_n = { { 0x11e4,                   PAL_NONE }, { 0x11e5,           PALETTE_TO_BROWN },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1173_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1173,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1174_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1174,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1175_c = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1175,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1172_1175_c = { { 0x1172,                   PAL_NONE }, { 0x1175,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_11e4_11e5_c = { { 0x11e4,                   PAL_NONE }, { 0x11e5,           PALETTE_TO_CREAM },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1173_x = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1173,            PALETTE_TO_GREY },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1174_x = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1174,            PALETTE_TO_GREY },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1175_x = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1175,            PALETTE_TO_GREY },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1172_1175_x = { { 0x1172,                   PAL_NONE }, { 0x1175,            PALETTE_TO_GREY },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_11e4_11e5_x = { { 0x11e4,                   PAL_NONE }, { 0x11e5,            PALETTE_TO_GREY },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_1176   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1176,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1176_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1176,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_11e6   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e6,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e6_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e6,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_1177        = { { 0x1177,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_1178        = { { 0x1178,                   PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_1179   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1179,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_117a   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117a,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_117b   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117b,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_117c   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117c,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e7   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e7,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e8   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e8,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_117d   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117d,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117e   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117e,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117f   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117f,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_conc_117f   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, { 0x117f,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e1   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e1,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_conc_11e1   = { { SPR_CONCRETE_GROUND,      PAL_NONE }, { 0x11e1,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117d_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117d,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117e_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117e,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117f_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117f,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_conc_117f_n = { { SPR_CONCRETE_GROUND,      PAL_NONE }, { 0x117f,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e1_n = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e1,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_conc_11e1_n = { { SPR_CONCRETE_GROUND,      PAL_NONE }, { 0x11e1,    PALETTE_TO_STRUCT_BROWN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117d_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117d,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117e_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117e,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_117f_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x117f,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_conc_117f_w = { { SPR_CONCRETE_GROUND,      PAL_NONE }, { 0x117f,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_11e1_w = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11e1,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_conc_11e1_w = { { SPR_CONCRETE_GROUND,      PAL_NONE }, { 0x11e1,    PALETTE_TO_STRUCT_WHITE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_1180_1181   = { { 0x1180,                   PAL_NONE }, { 0x1181,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1183_1182_g = { { 0x1183,                   PAL_NONE }, { 0x1182,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_1184_g = { { 0x1183,                   PAL_NONE }, { 0x1184,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_11e9_g = { { 0x1183,                   PAL_NONE }, { 0x11e9,      PALETTE_TO_DARK_GREEN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_1182_o = { { 0x1183,                   PAL_NONE }, { 0x1182,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_1184_o = { { 0x1183,                   PAL_NONE }, { 0x1184,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_11e9_o = { { 0x1183,                   PAL_NONE }, { 0x11e9,          PALETTE_TO_ORANGE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_1182_n = { { 0x1183,                   PAL_NONE }, { 0x1182,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_1184_n = { { 0x1183,                   PAL_NONE }, { 0x1184,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_11e9_n = { { 0x1183,                   PAL_NONE }, { 0x11e9,           PALETTE_TO_BROWN },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_1182_x = { { 0x1183,                   PAL_NONE }, { 0x1182,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_1184_x = { { 0x1183,                   PAL_NONE }, { 0x1184,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_1183_11e9_x = { { 0x1183,                   PAL_NONE }, { 0x11e9,            PALETTE_TO_GREY },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_1185_1187   = { { 0x1185,                   PAL_NONE }, { 0x1187,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1185_1189   = { { 0x1185,                   PAL_NONE }, { 0x1189,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1185_118b   = { { 0x1185,                   PAL_NONE }, { 0x118b,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1185_11ea   = { { 0x1185,                   PAL_NONE }, { 0x11ea,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1186_1188   = { { 0x1186,                   PAL_NONE }, { 0x1188,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1186_118a   = { { 0x1186,                   PAL_NONE }, { 0x118a,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1186_118c   = { { 0x1186,                   PAL_NONE }, { 0x118c,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };
static const DrawBuildingsTileStruct dbts_1186_11eb   = { { 0x1186,                   PAL_NONE }, { 0x11eb,                   PAL_NONE },   0,  0, 16, 16,  10, 0 };

static const DrawBuildingsTileStruct dbts_bare_11ec   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11ec,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11ed   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11ed,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11ee   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11ee,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_11ef   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11ef,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f0   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f0,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f1   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f1,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_11f2   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f2,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f3   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f3,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f4   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f4,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_4      = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f5   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f5,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f6   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f6,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f7   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f7,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };
static const DrawBuildingsTileStruct dbts_bare_11f8   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f8,                   PAL_NONE },   0,  0, 16, 16,  20, 0 };

static const DrawBuildingsTileStruct dbts_bare_11f9   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11f9,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11fa   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11fa,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_11fb   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11fb,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_11fc   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11fc,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_11fd   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11fd,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11fe   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11fe,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };
static const DrawBuildingsTileStruct dbts_bare_11ff   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x11ff,                   PAL_NONE },   0,  0, 16, 16,  40, 0 };

static const DrawBuildingsTileStruct dbts_bare_1200   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1200,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1201   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1201,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1202   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1202,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1200_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1200,            PALETTE_TO_PINK },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1201_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1201,            PALETTE_TO_PINK },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1202_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1202,            PALETTE_TO_PINK },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1200_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1200,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1201_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1201,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1202_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1202,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1200_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1200,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1201_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1201,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1202_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1202,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_1203   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1203,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1204   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1204,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1205   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1205,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1203_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1203,            PALETTE_TO_PINK },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1204_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1204,            PALETTE_TO_PINK },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1205_p = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1205,            PALETTE_TO_PINK },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1203_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1203,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1204_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1204,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1205_y = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1205,          PALETTE_TO_YELLOW },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1203_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1203,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1204_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1204,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_1205_r = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1205,             PALETTE_TO_RED },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_1206   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1206,                   PAL_NONE },   0,  0, 16, 16,  80, 0 };
static const DrawBuildingsTileStruct dbts_bare_1208   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1208,                   PAL_NONE },   0,  0, 16, 16,  80, 0 };
static const DrawBuildingsTileStruct dbts_bare_120a   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x120a,                   PAL_NONE },   0,  0, 16, 16,  80, 0 };

static const DrawBuildingsTileStruct dbts_bare_1207   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1207,                   PAL_NONE },   0,  0, 16, 16,  80, 0 };
static const DrawBuildingsTileStruct dbts_bare_1209   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1209,                   PAL_NONE },   0,  0, 16, 16,  80, 0 };
static const DrawBuildingsTileStruct dbts_bare_120b   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x120b,                   PAL_NONE },   0,  0, 16, 16,  80, 0 };

static const DrawBuildingsTileStruct dbts_bare_120c   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x120c,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_120d   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x120d,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };
static const DrawBuildingsTileStruct dbts_bare_120e   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x120e,                   PAL_NONE },   0,  0, 16, 16,  60, 0 };

static const DrawBuildingsTileStruct dbts_bare_120f   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x120f,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_1210   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1210,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_bare_1211   = { { SPR_FLAT_BARE_LAND,       PAL_NONE }, { 0x1211,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_1213   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1213,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1214   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1214,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1215   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1215,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1213_r = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1213,             PALETTE_TO_RED },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1214_r = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1214,             PALETTE_TO_RED },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1215_r = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1215,             PALETTE_TO_RED },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1213_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1213,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1214_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1214,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1215_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1215,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1213_g = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1213,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1214_g = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1214,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1215_g = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1215,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_1216   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1216,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1217   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1217,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1218   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1218,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy2_1219   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1219,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_121a   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x121a,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_121b   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x121b,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_121c   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x121c,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_121d   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x121d,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_121e   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x121e,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy2_121f   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x121f,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1220   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1220,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1221   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1221,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_121f_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x121f,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1220_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1220,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1221_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1221,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_121f_m = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x121f,           PALETTE_TO_MAUVE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1220_m = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1220,           PALETTE_TO_MAUVE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1221_m = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1221,           PALETTE_TO_MAUVE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_121f_c = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x121f,           PALETTE_TO_CREAM },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1220_c = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1220,           PALETTE_TO_CREAM },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1221_c = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1221,           PALETTE_TO_CREAM },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy2_1222   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1222,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1223   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1223,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1224   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1224,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1222_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1222,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1223_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1223,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1224_p = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1224,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1222_c = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1222,           PALETTE_TO_CREAM },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1223_c = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1223,           PALETTE_TO_CREAM },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1224_c = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1224,           PALETTE_TO_CREAM },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1222_g = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1222,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1223_g = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1223,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1224_g = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1224,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_1225   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1225,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1226   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1226,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1227   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1227,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1225_p = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1225,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1226_p = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1226,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1227_p = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1227,            PALETTE_TO_PINK },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1225_r = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1225,             PALETTE_TO_RED },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1226_r = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1226,             PALETTE_TO_RED },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1227_r = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1227,             PALETTE_TO_RED },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1225_g = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1225,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1226_g = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1226,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1227_g = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1227,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_1228   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1228,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1229   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1229,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_122a   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x122a,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy2_122b   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x122b,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_122d   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x122d,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_122f   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x122f,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_122c   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x122c,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_122e   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x122e,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1230   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1230,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy2_1231   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1231,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1232   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1232,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1233   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1233,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_1234   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1234,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1235   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1235,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1236   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1236,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy2_1237   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1237,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1238   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1238,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1239   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1239,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_123a   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x123a,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_123b   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x123b,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_123c   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x123c,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy2_123d   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x123d,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_123e   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x123e,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_123f   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x123f,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_1240   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1240,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1241   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1241,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1242   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1242,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1        = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2        = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, {    0x0,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1256   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1256,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1256_g = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1256,      PALETTE_TO_PALE_GREEN },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1256_r = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1256,             PALETTE_TO_RED },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_1256_c = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x1256,           PALETTE_TO_CREAM },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy2_125a   = { { SPR_GRND_HOUSE_TOY2,      PAL_NONE }, { 0x125a,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

static const DrawBuildingsTileStruct dbts_toy1_1257   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1257,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1258   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1258,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };
static const DrawBuildingsTileStruct dbts_toy1_1259   = { { SPR_GRND_HOUSE_TOY1,      PAL_NONE }, { 0x1259,                   PAL_NONE },   0,  0, 16, 16,  50, 0 };

/** House graphics data array. */
static const DrawBuildingsTileStruct *const town_draw_tile_data[][4][4] = {
	{
		{ &dbts_bare_058d,   &dbts_bare_058e,   &dbts_bare_058f,   &dbts_0590_058f   },
		{ &dbts_bare_058d,   &dbts_bare_058e,   &dbts_bare_0591,   &dbts_0590_0591   },
		{ &dbts_bare_058d_w, &dbts_bare_058e_w, &dbts_bare_0591_w, &dbts_0590_0591_w },
		{ &dbts_bare_058d_c, &dbts_bare_058e_c, &dbts_bare_0591_c, &dbts_0590_0591_c },
	}, {
		{ &dbts_bare_0592,   &dbts_bare_0593,   &dbts_bare_0594,   &dbts_0595_0594   },
		{ &dbts_bare_0592_w, &dbts_bare_0593_w, &dbts_bare_0594_w, &dbts_0595_0594_w },
		{ &dbts_bare_0592_c, &dbts_bare_0593_c, &dbts_bare_0594_c, &dbts_0595_0594_c },
		{ &dbts_bare_0592_n, &dbts_bare_0593_n, &dbts_bare_0594_n, &dbts_0595_0594_n },
	}, {
		{ &dbts_bare_0596, &dbts_bare_0597, &dbts_bare_0598, &dbts_0599_0598 },
		{ &dbts_bare_0596, &dbts_bare_0597, &dbts_bare_0598, &dbts_0599_0598 },
		{ &dbts_bare_0596, &dbts_bare_0597, &dbts_bare_0598, &dbts_0599_0598 },
		{ &dbts_bare_0596, &dbts_bare_0597, &dbts_bare_0598, &dbts_0599_0598 },
	}, {
		{ &dbts_bare_059a,   &dbts_bare_059b,   &dbts_bare_059c,   &dbts_059d_059c   },
		{ &dbts_bare_059a_r, &dbts_bare_059b_r, &dbts_bare_059c_r, &dbts_059d_059c_r },
		{ &dbts_bare_059a_c, &dbts_bare_059b_c, &dbts_bare_059c_c, &dbts_059d_059c_c },
		{ &dbts_bare_059a_c, &dbts_bare_059b_c, &dbts_bare_059c_c, &dbts_059d_059c_c },
	}, {
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_05a2, &dbts_conc_05a2 },
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_05a2, &dbts_conc_05a2 },
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_05a2, &dbts_conc_05a2 },
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_05a2, &dbts_conc_05a2 },
	}, {
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_11d9, &dbts_conc_11d9 },
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_11d9, &dbts_conc_11d9 },
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_11d9, &dbts_conc_11d9 },
		{ &dbts_bare_05a0, &dbts_bare_05a1, &dbts_bare_11d9, &dbts_conc_11d9 },
	}, {
		{ &dbts_bare_05a4, &dbts_bare_05a5, &dbts_bare_05a6, &dbts_05a7_05a6 },
		{ &dbts_bare_05a4, &dbts_bare_05a5, &dbts_bare_05a6, &dbts_05a7_05a6 },
		{ &dbts_05dd_05de, &dbts_05df_05e0, &dbts_05e1_05e2, &dbts_05e1_05e2 },
		{ &dbts_05dd_05de, &dbts_05df_05e0, &dbts_05e1_05e2, &dbts_05e1_05e2 },
	}, {
		{ &dbts_bare_05a8, &dbts_bare_05a9, &dbts_bare_05aa, &dbts_conc_05aa },
		{ &dbts_bare_05a8, &dbts_bare_05a9, &dbts_bare_05aa, &dbts_conc_05aa },
		{ &dbts_bare_05a8, &dbts_bare_05a9, &dbts_bare_05aa, &dbts_conc_05aa },
		{ &dbts_bare_05a8, &dbts_bare_05a9, &dbts_bare_05aa, &dbts_conc_05aa },
	}, {
		{ &dbts_bare_05ab, &dbts_bare_05ac, &dbts_bare_05ad, &dbts_conc_05ad },
		{ &dbts_bare_05ab, &dbts_bare_05ac, &dbts_bare_05ad, &dbts_conc_05ad },
		{ &dbts_bare_05ab, &dbts_bare_05ac, &dbts_bare_05ad, &dbts_conc_05ad },
		{ &dbts_bare_05ab, &dbts_bare_05ac, &dbts_bare_05ad, &dbts_conc_05ad },
	}, {
		{ &dbts_bare_1, &dbts_conc_1, &dbts_conc_05ae, &dbts_conc_05ae },
		{ &dbts_bare_1, &dbts_conc_1, &dbts_conc_05ae, &dbts_conc_05ae },
		{ &dbts_bare_1, &dbts_conc_1, &dbts_conc_05ae, &dbts_conc_05ae },
		{ &dbts_bare_1, &dbts_conc_1, &dbts_conc_05ae, &dbts_conc_05ae },
	}, {
		{ &dbts_bare_2, &dbts_conc_2, &dbts_conc_05af, &dbts_conc_05af },
		{ &dbts_bare_2, &dbts_conc_2, &dbts_conc_05af, &dbts_conc_05af },
		{ &dbts_bare_2, &dbts_conc_2, &dbts_conc_05af, &dbts_conc_05af },
		{ &dbts_bare_2, &dbts_conc_2, &dbts_conc_05af, &dbts_conc_05af },
	}, {
		{ &dbts_bare_3, &dbts_grss_05b0, &dbts_grss_05b0, &dbts_grss_05b0 },
		{ &dbts_bare_3, &dbts_grss_05b0, &dbts_grss_05b0, &dbts_grss_05b0 },
		{ &dbts_bare_3, &dbts_grss_05b0, &dbts_grss_05b0, &dbts_grss_05b0 },
		{ &dbts_bare_3, &dbts_grss_05b0, &dbts_grss_05b0, &dbts_grss_05b0 },
	}, {
		{ &dbts_bare_3, &dbts_grss_05b1, &dbts_grss_05b1, &dbts_grss_05b1 },
		{ &dbts_bare_3, &dbts_grss_05b1, &dbts_grss_05b1, &dbts_grss_05b1 },
		{ &dbts_bare_3, &dbts_grss_05b1, &dbts_grss_05b1, &dbts_grss_05b1 },
		{ &dbts_bare_3, &dbts_grss_05b1, &dbts_grss_05b1, &dbts_grss_05b1 },
	}, {
		{ &dbts_bare_05b2,   &dbts_bare_05b3,   &dbts_bare_05b4,   &dbts_conc_05b4   },
		{ &dbts_bare_05b2_w, &dbts_bare_05b3_w, &dbts_bare_05b4_w, &dbts_conc_05b4_w },
		{ &dbts_bare_05b2_r, &dbts_bare_05b3_r, &dbts_bare_05b4_r, &dbts_conc_05b4_r },
		{ &dbts_bare_05b2_n, &dbts_bare_05b3_n, &dbts_bare_05b4_n, &dbts_conc_05b4_n },
	}, {
		{ &dbts_bare_05b5, &dbts_bare_05b6, &dbts_bare_05b7, &dbts_conc_05b7 },
		{ &dbts_bare_05b5, &dbts_bare_05b6, &dbts_bare_05b7, &dbts_conc_05b7 },
		{ &dbts_bare_05b5, &dbts_bare_05b6, &dbts_bare_05b7, &dbts_conc_05b7 },
		{ &dbts_bare_05b5, &dbts_bare_05b6, &dbts_bare_05b7, &dbts_conc_05b7 },
	}, {
		{ &dbts_bare_05b8, &dbts_bare_05b9, &dbts_bare_05ba, &dbts_conc_05ba },
		{ &dbts_bare_05b8, &dbts_bare_05b9, &dbts_bare_05ba, &dbts_conc_05ba },
		{ &dbts_bare_05b8, &dbts_bare_05b9, &dbts_bare_05ba, &dbts_conc_05ba },
		{ &dbts_bare_05b8, &dbts_bare_05b9, &dbts_bare_05ba, &dbts_conc_05ba },
	}, {
		{ &dbts_bare_05bb,   &dbts_bare_05bc,   &dbts_bare_05bd,   &dbts_conc_05bd   },
		{ &dbts_bare_05bb,   &dbts_bare_05bc,   &dbts_bare_05bd,   &dbts_conc_05bd   },
		{ &dbts_bare_05bb_b, &dbts_bare_05bc_b, &dbts_bare_05bd_b, &dbts_conc_05bd_b },
		{ &dbts_bare_05bb_w, &dbts_bare_05bc_w, &dbts_bare_05bd_w, &dbts_conc_05bd_w },
	}, {
		{ &dbts_bare_05be_r, &dbts_bare_05bf_r, &dbts_bare_05c0_r, &dbts_conc_05c0_r },
		{ &dbts_bare_05be_b, &dbts_bare_05bf_b, &dbts_bare_05c0_b, &dbts_conc_05c0_b },
		{ &dbts_bare_05be_o, &dbts_bare_05bf_o, &dbts_bare_05c0_o, &dbts_conc_05c0_o },
		{ &dbts_bare_05be_g, &dbts_bare_05bf_g, &dbts_bare_05c0_g, &dbts_conc_05c0_g },
	}, {
		{ &dbts_bare_05c1, &dbts_bare_05c2, &dbts_bare_05c3, &dbts_conc_05c3 },
		{ &dbts_bare_05c1, &dbts_bare_05c2, &dbts_bare_05c3, &dbts_conc_05c3 },
		{ &dbts_bare_05c1, &dbts_bare_05c2, &dbts_bare_05c3, &dbts_conc_05c3 },
		{ &dbts_bare_05c1, &dbts_bare_05c2, &dbts_bare_05c3, &dbts_conc_05c3 },
	}, {
		{ &dbts_bare_05c4, &dbts_bare_05c5, &dbts_bare_05c6, &dbts_conc_05c6 },
		{ &dbts_bare_05c4, &dbts_bare_05c5, &dbts_bare_05c6, &dbts_conc_05c6 },
		{ &dbts_bare_05c4, &dbts_bare_05c5, &dbts_bare_05c6, &dbts_conc_05c6 },
		{ &dbts_bare_05c4, &dbts_bare_05c5, &dbts_bare_05c6, &dbts_conc_05c6 },
	}, {
		{ &dbts_stdn, &dbts_stdn_05cb, &dbts_stdn_05cb, &dbts_stdn_05cb },
		{ &dbts_stdn, &dbts_stdn_05cb, &dbts_stdn_05cb, &dbts_stdn_05cb },
		{ &dbts_stdn, &dbts_stdn_05cb, &dbts_stdn_05cb, &dbts_stdn_05cb },
		{ &dbts_stdn, &dbts_stdn_05cb, &dbts_stdn_05cb, &dbts_stdn_05cb },
	}, {
		{ &dbts_stde, &dbts_stde_05cc, &dbts_stde_05cc, &dbts_stde_05cc },
		{ &dbts_stde, &dbts_stde_05cc, &dbts_stde_05cc, &dbts_stde_05cc },
		{ &dbts_stde, &dbts_stde_05cc, &dbts_stde_05cc, &dbts_stde_05cc },
		{ &dbts_stde, &dbts_stde_05cc, &dbts_stde_05cc, &dbts_stde_05cc },
	}, {
		{ &dbts_stdw, &dbts_stdw_05cd, &dbts_stdw_05cd, &dbts_stdw_05cd },
		{ &dbts_stdw, &dbts_stdw_05cd, &dbts_stdw_05cd, &dbts_stdw_05cd },
		{ &dbts_stdw, &dbts_stdw_05cd, &dbts_stdw_05cd, &dbts_stdw_05cd },
		{ &dbts_stdw, &dbts_stdw_05cd, &dbts_stdw_05cd, &dbts_stdw_05cd },
	}, {
		{ &dbts_stds, &dbts_stds_05ce, &dbts_stds_05ce, &dbts_stds_05ce },
		{ &dbts_stds, &dbts_stds_05ce, &dbts_stds_05ce, &dbts_stds_05ce },
		{ &dbts_stds, &dbts_stds_05ce, &dbts_stds_05ce, &dbts_stds_05ce },
		{ &dbts_stds, &dbts_stds_05ce, &dbts_stds_05ce, &dbts_stds_05ce },
	}, {
		{ &dbts_bare_05d4, &dbts_05d3_05d4, &dbts_05d3_05d4, &dbts_05d3_05d4 },
		{ &dbts_bare_05d6, &dbts_05d5_05d6, &dbts_05d5_05d6, &dbts_05d5_05d6 },
		{ &dbts_bare_05d0, &dbts_05cf_05d0, &dbts_05cf_05d0, &dbts_05cf_05d0 },
		{ &dbts_bare_05d2, &dbts_05d1_05d2, &dbts_05d1_05d2, &dbts_05d1_05d2 },
	}, {
		{ &dbts_bare, &dbts_05d7_05d8, &dbts_05d7_05d8, &dbts_05d7_05d8 },
		{ &dbts_bare, &dbts_05d7_05d8, &dbts_05d7_05d8, &dbts_05d7_05d8 },
		{ &dbts_bare, &dbts_05d7_05d8, &dbts_05d7_05d8, &dbts_05d7_05d8 },
		{ &dbts_bare, &dbts_05d7_05d8, &dbts_05d7_05d8, &dbts_05d7_05d8 },
	}, {
		{ &dbts_05d9,      &dbts_05da,      &dbts_05db_05dc, &dbts_05db_05dc },
		{ &dbts_0622_0623, &dbts_0624_0625, &dbts_0626_0627, &dbts_0626_0627 },
		{ &dbts_05e3_05e4, &dbts_05e5_05e6, &dbts_05e7_05e8, &dbts_05e7_05e8 },
		{ &dbts_05e9_05ea, &dbts_05eb_05ec, &dbts_05ed_05ee, &dbts_05ed_05ee },
	}, {
		{ &dbts_05ef,      &dbts_05f0_05f1, &dbts_05f2_05f3, &dbts_05f2_05f3 },
		{ &dbts_05ef,      &dbts_05f0_05f1, &dbts_05f2_05f3, &dbts_05f2_05f3 },
		{ &dbts_05f4_05f5, &dbts_05f6_05f7, &dbts_05f8_05f9, &dbts_05f8_05f9 },
		{ &dbts_05f4_05f5, &dbts_05f6_05f7, &dbts_05f8_05f9, &dbts_05f8_05f9 },
	}, {
		{ &dbts_05fa_05fb, &dbts_05fc_05fd, &dbts_05fe_05ff, &dbts_05fe_05ff },
		{ &dbts_05fa_05fb, &dbts_05fc_05fd, &dbts_05fe_05ff, &dbts_05fe_05ff },
		{ &dbts_060a_060b, &dbts_060c_060d, &dbts_060e_060f, &dbts_060e_060f },
		{ &dbts_060a_060b, &dbts_060c_060d, &dbts_060e_060f, &dbts_060e_060f },
	}, {
		{ &dbts_bare, &dbts_0600_0601,   &dbts_0600_0601,   &dbts_0600_0601   },
		{ &dbts_bare, &dbts_0600_0601_w, &dbts_0600_0601_w, &dbts_0600_0601_w },
		{ &dbts_bare, &dbts_0602_0603,   &dbts_0602_0603,   &dbts_0602_0603   },
		{ &dbts_bare, &dbts_0602_0603_c, &dbts_0602_0603_c, &dbts_0602_0603_c },
	}, {
		{ &dbts_0604_0605, &dbts_0606_0607, &dbts_0608_0609, &dbts_0608_0609 },
		{ &dbts_0604_0605, &dbts_0606_0607, &dbts_0608_0609, &dbts_0608_0609 },
		{ &dbts_0604_0605, &dbts_0606_0607, &dbts_0608_0609, &dbts_0608_0609 },
		{ &dbts_0604_0605, &dbts_0606_0607, &dbts_0608_0609, &dbts_0608_0609 },
	}, {
		{ &dbts_bare, &dbts_0610_0611, &dbts_0610_0611, &dbts_0610_0611 },
		{ &dbts_bare, &dbts_0610_0611, &dbts_0610_0611, &dbts_0610_0611 },
		{ &dbts_bare, &dbts_0610_0611, &dbts_0610_0611, &dbts_0610_0611 },
		{ &dbts_bare, &dbts_0610_0611, &dbts_0610_0611, &dbts_0610_0611 },
	}, {
		{ &dbts_0612, &dbts_0612_0616, &dbts_0612_0616, &dbts_0612_0616 },
		{ &dbts_0612, &dbts_0612_0616, &dbts_0612_0616, &dbts_0612_0616 },
		{ &dbts_0612, &dbts_0612_0616, &dbts_0612_0616, &dbts_0612_0616 },
		{ &dbts_0612, &dbts_0612_0616, &dbts_0612_0616, &dbts_0612_0616 },
	}, {
		{ &dbts_0613, &dbts_0613_0617, &dbts_0613_0617, &dbts_0613_0617 },
		{ &dbts_0613, &dbts_0613_0617, &dbts_0613_0617, &dbts_0613_0617 },
		{ &dbts_0613, &dbts_0613_0617, &dbts_0613_0617, &dbts_0613_0617 },
		{ &dbts_0613, &dbts_0613_0617, &dbts_0613_0617, &dbts_0613_0617 },
	}, {
		{ &dbts_0614, &dbts_0614_0618, &dbts_0614_0618, &dbts_0614_0618 },
		{ &dbts_0614, &dbts_0614_0618, &dbts_0614_0618, &dbts_0614_0618 },
		{ &dbts_0614, &dbts_0614_0618, &dbts_0614_0618, &dbts_0614_0618 },
		{ &dbts_0614, &dbts_0614_0618, &dbts_0614_0618, &dbts_0614_0618 },
	}, {
		{ &dbts_0615, &dbts_0615_0619, &dbts_0615_0619, &dbts_0615_0619 },
		{ &dbts_0615, &dbts_0615_0619, &dbts_0615_0619, &dbts_0615_0619 },
		{ &dbts_0615, &dbts_0615_0619, &dbts_0615_0619, &dbts_0615_0619 },
		{ &dbts_0615, &dbts_0615_0619, &dbts_0615_0619, &dbts_0615_0619 },
	}, {
		{ &dbts_bare, &dbts_061a_061b, &dbts_061a_061b, &dbts_061c_061d },
		{ &dbts_bare, &dbts_061a_061b, &dbts_061a_061b, &dbts_061c_061d },
		{ &dbts_bare, &dbts_061a_061b, &dbts_061a_061b, &dbts_061c_061d },
		{ &dbts_bare, &dbts_061a_061b, &dbts_061a_061b, &dbts_061c_061d },
	}, {
		{ &dbts_bare, &dbts_061e_061f,   &dbts_061e_061f,   &dbts_061e_061f   },
		{ &dbts_bare, &dbts_061e_061f_w, &dbts_061e_061f_w, &dbts_061e_061f_w },
		{ &dbts_bare, &dbts_0620_0621,   &dbts_0620_0621,   &dbts_0620_0621   },
		{ &dbts_bare, &dbts_0620_0621_c, &dbts_0620_0621_c, &dbts_0620_0621_c },
	}, {
		{ &dbts_bare, &dbts_11da_11db,   &dbts_11da_11db,   &dbts_11da_11db   },
		{ &dbts_bare, &dbts_11da_11db_w, &dbts_11da_11db_w, &dbts_11da_11db_w },
		{ &dbts_bare, &dbts_11dc_11dd,   &dbts_11dc_11dd,   &dbts_11dc_11dd   },
		{ &dbts_bare, &dbts_11dc_11dd_c, &dbts_11dc_11dd_c, &dbts_11dc_11dd_c },
	}, {
		{ &dbts_bare, &dbts_1134_1135, &dbts_1134_1135, &dbts_1134_1135 },
		{ &dbts_bare, &dbts_1134_1135, &dbts_1134_1135, &dbts_1134_1135 },
		{ &dbts_bare, &dbts_1134_1135, &dbts_1134_1135, &dbts_1134_1135 },
		{ &dbts_bare, &dbts_1134_1135, &dbts_1134_1135, &dbts_1134_1135 },
	}, {
		{ &dbts_1136_1137, &dbts_1136_1137, &dbts_113b_113c, &dbts_113b_113c },
		{ &dbts_1136_1137, &dbts_1136_1137, &dbts_113b_113c, &dbts_113b_113c },
		{ &dbts_1136_1137, &dbts_1136_1137, &dbts_113b_113c, &dbts_113b_113c },
		{ &dbts_1136_1137, &dbts_1136_1137, &dbts_113b_113c, &dbts_113b_113c },
	}, {
		{ &dbts_1138, &dbts_1138, &dbts_113d_113e, &dbts_113d_113e },
		{ &dbts_1138, &dbts_1138, &dbts_113d_113e, &dbts_113d_113e },
		{ &dbts_1138, &dbts_1138, &dbts_113d_113e, &dbts_113d_113e },
		{ &dbts_1138, &dbts_1138, &dbts_113d_113e, &dbts_113d_113e },
	}, {
		{ &dbts_1139, &dbts_1139, &dbts_113f_1140, &dbts_113f_1140 },
		{ &dbts_1139, &dbts_1139, &dbts_113f_1140, &dbts_113f_1140 },
		{ &dbts_1139, &dbts_1139, &dbts_113f_1140, &dbts_113f_1140 },
		{ &dbts_1139, &dbts_1139, &dbts_113f_1140, &dbts_113f_1140 },
	}, {
		{ &dbts_113a, &dbts_113a, &dbts_1141, &dbts_1141 },
		{ &dbts_113a, &dbts_113a, &dbts_1141, &dbts_1141 },
		{ &dbts_113a, &dbts_113a, &dbts_1141, &dbts_1141 },
		{ &dbts_113a, &dbts_113a, &dbts_1141, &dbts_1141 },
	}, {
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1142_1146 },
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1142_1146 },
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1142_1146 },
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1142_1146 },
	}, {
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1143_1147 },
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1143_1147 },
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1143_1147 },
		{ &dbts_bare_1144, &dbts_bare_1145, &dbts_bare_1146, &dbts_1143_1147 },
	}, {
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1142_114a },
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1142_114a },
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1142_114a },
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1142_114a },
	}, {
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1143_114b },
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1143_114b },
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1143_114b },
		{ &dbts_bare_1148, &dbts_bare_1149, &dbts_bare_114a, &dbts_1143_114b },
	}, {
		{ &dbts_bare_114e,   &dbts_bare_114f,   &dbts_bare_1150,   &dbts_114c_1150   },
		{ &dbts_bare_114e_p, &dbts_bare_114f_p, &dbts_bare_1150_p, &dbts_114c_1150_p },
		{ &dbts_bare_114e_c, &dbts_bare_114f_c, &dbts_bare_1150_c, &dbts_114c_1150_c },
		{ &dbts_bare_114e_g, &dbts_bare_114f_g, &dbts_bare_1150_g, &dbts_114c_1150_g },
	}, {
		{ &dbts_bare_114e,   &dbts_bare_114f,   &dbts_bare_1150,   &dbts_114d_1151   },
		{ &dbts_bare_114e_p, &dbts_bare_114f_p, &dbts_bare_1150_p, &dbts_114d_1151_p },
		{ &dbts_bare_114e_c, &dbts_bare_114f_c, &dbts_bare_1150_c, &dbts_114d_1151_c },
		{ &dbts_bare_114e_g, &dbts_bare_114f_g, &dbts_bare_1150_g, &dbts_114d_1151_g },
	}, {
		{ &dbts_bare_1153,   &dbts_bare_1154,   &dbts_bare_1155,   &dbts_1152_1155   },
		{ &dbts_bare_1153_r, &dbts_bare_1154_r, &dbts_bare_1155_r, &dbts_1152_1155_r },
		{ &dbts_bare_1153_g, &dbts_bare_1154_g, &dbts_bare_1155_g, &dbts_1152_1155_g },
		{ &dbts_bare_1153_x, &dbts_bare_1154_x, &dbts_bare_1155_x, &dbts_1152_1155_x },
	}, {
		{ &dbts_bare_1153,   &dbts_bare_1154,   &dbts_bare_11de,   &dbts_1152_11de   },
		{ &dbts_bare_1153_r, &dbts_bare_1154_r, &dbts_bare_11de_r, &dbts_1152_11de_r },
		{ &dbts_bare_1153_g, &dbts_bare_1154_g, &dbts_bare_11de_g, &dbts_1152_11de_g },
		{ &dbts_bare_1153_x, &dbts_bare_1154_x, &dbts_bare_11de_x, &dbts_1152_11de_x },
	}, {
		{ &dbts_bare_1157,   &dbts_bare_1158,   &dbts_bare_1159,   &dbts_1156_1159   },
		{ &dbts_bare_1157_g, &dbts_bare_1158_g, &dbts_bare_1159_g, &dbts_1156_1159_g },
		{ &dbts_bare_1157_c, &dbts_bare_1158_c, &dbts_bare_1159_c, &dbts_1156_1159_c },
		{ &dbts_bare_1157_y, &dbts_bare_1158_y, &dbts_bare_1159_y, &dbts_1156_1159_y },
	}, {
		{ &dbts_bare_1157,   &dbts_bare_1158,   &dbts_bare_11df,   &dbts_1156_11df   },
		{ &dbts_bare_1157_g, &dbts_bare_1158_g, &dbts_bare_11df_g, &dbts_1156_11df_g },
		{ &dbts_bare_1157_c, &dbts_bare_1158_c, &dbts_bare_11df_c, &dbts_1156_11df_c },
		{ &dbts_bare_1157_y, &dbts_bare_1158_y, &dbts_bare_11df_y, &dbts_1156_11df_y },
	}, {
		{ &dbts_bare_115b,   &dbts_bare_115c,   &dbts_bare_115d,   &dbts_115a_115d   },
		{ &dbts_bare_115b_r, &dbts_bare_115c_r, &dbts_bare_115d_r, &dbts_115a_115d_r },
		{ &dbts_bare_115b_o, &dbts_bare_115c_o, &dbts_bare_115d_o, &dbts_115a_115d_o },
		{ &dbts_bare_115b_n, &dbts_bare_115c_n, &dbts_bare_115d_n, &dbts_115a_115d_n },
	}, {
		{ &dbts_bare_115b,   &dbts_bare_115c,   &dbts_bare_11e0,   &dbts_115a_11e0   },
		{ &dbts_bare_115b_r, &dbts_bare_115c_r, &dbts_bare_11e0_r, &dbts_115a_11e0_r },
		{ &dbts_bare_115b_o, &dbts_bare_115c_o, &dbts_bare_11e0_o, &dbts_115a_11e0_o },
		{ &dbts_bare_115b_n, &dbts_bare_115c_n, &dbts_bare_11e0_n, &dbts_115a_11e0_n },
	}, {
		{ &dbts_bare_1160, &dbts_bare_1161, &dbts_bare_1162, &dbts_115e_1162 },
		{ &dbts_bare_1160, &dbts_bare_1161, &dbts_bare_1162, &dbts_115e_1162 },
		{ &dbts_bare_1166, &dbts_bare_1167, &dbts_bare_1168, &dbts_1164_1168 },
		{ &dbts_bare_1166, &dbts_bare_1167, &dbts_bare_1168, &dbts_1164_1168 },
	}, {
		{ &dbts_bare_1160, &dbts_bare_1161, &dbts_bare_1162, &dbts_115f_1163 },
		{ &dbts_bare_1160, &dbts_bare_1161, &dbts_bare_1162, &dbts_115f_1163 },
		{ &dbts_bare_1166, &dbts_bare_1167, &dbts_bare_1168, &dbts_1165_1169 },
		{ &dbts_bare_1166, &dbts_bare_1167, &dbts_bare_1168, &dbts_1165_1169 },
	}, {
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_116d, &dbts_116a_116d },
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_116d, &dbts_116a_116d },
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_116d, &dbts_116a_116d },
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_116d, &dbts_116a_116d },
	}, {
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_11e2, &dbts_116a_11e2 },
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_11e2, &dbts_116a_11e2 },
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_11e2, &dbts_116a_11e2 },
		{ &dbts_116b, &dbts_bare_116c, &dbts_bare_11e2, &dbts_116a_11e2 },
	}, {
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_1171, &dbts_116e_1171 },
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_1171, &dbts_116e_1171 },
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_1171, &dbts_116e_1171 },
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_1171, &dbts_116e_1171 },
	}, {
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_11e3, &dbts_116e_11e3 },
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_11e3, &dbts_116e_11e3 },
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_11e3, &dbts_116e_11e3 },
		{ &dbts_116f, &dbts_bare_1170, &dbts_bare_11e3, &dbts_116e_11e3 },
	}, {
		{ &dbts_bare_1173,   &dbts_bare_1174,   &dbts_bare_1175,   &dbts_1172_1175   },
		{ &dbts_bare_1173_n, &dbts_bare_1174_n, &dbts_bare_1175_n, &dbts_1172_1175_n },
		{ &dbts_bare_1173_c, &dbts_bare_1174_c, &dbts_bare_1175_c, &dbts_1172_1175_c },
		{ &dbts_bare_1173_x, &dbts_bare_1174_x, &dbts_bare_1175_x, &dbts_1172_1175_x },
	}, {
		{ &dbts_bare_1173,   &dbts_bare_1174,   &dbts_bare_1175,   &dbts_11e4_11e5   },
		{ &dbts_bare_1173_n, &dbts_bare_1174_n, &dbts_bare_1175_n, &dbts_11e4_11e5_n },
		{ &dbts_bare_1173_c, &dbts_bare_1174_c, &dbts_bare_1175_c, &dbts_11e4_11e5_c },
		{ &dbts_bare_1173_x, &dbts_bare_1174_x, &dbts_bare_1175_x, &dbts_11e4_11e5_x },
	}, {
		{ &dbts_bare_u, &dbts_bare_1176,   &dbts_bare_1176,   &dbts_bare_1176   },
		{ &dbts_bare_u, &dbts_bare_1176,   &dbts_bare_1176,   &dbts_bare_1176   },
		{ &dbts_bare_u, &dbts_bare_1176_n, &dbts_bare_1176_n, &dbts_bare_1176_n },
		{ &dbts_bare_u, &dbts_bare_1176_n, &dbts_bare_1176_n, &dbts_bare_1176_n },
	}, {
		{ &dbts_bare_u, &dbts_bare_11e6,   &dbts_bare_11e6,   &dbts_bare_11e6   },
		{ &dbts_bare_u, &dbts_bare_11e6,   &dbts_bare_11e6,   &dbts_bare_11e6   },
		{ &dbts_bare_u, &dbts_bare_11e6_n, &dbts_bare_11e6_n, &dbts_bare_11e6_n },
		{ &dbts_bare_u, &dbts_bare_11e6_n, &dbts_bare_11e6_n, &dbts_bare_11e6_n },
	}, {
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_117b, &dbts_bare_117b },
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_117b, &dbts_bare_117b },
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_117b, &dbts_bare_117b },
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_117b, &dbts_bare_117b },
	}, {
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_117c, &dbts_bare_117c },
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_117c, &dbts_bare_117c },
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_117c, &dbts_bare_117c },
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_117c, &dbts_bare_117c },
	}, {
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_11e7, &dbts_bare_11e7 },
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_11e7, &dbts_bare_11e7 },
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_11e7, &dbts_bare_11e7 },
		{ &dbts_1177, &dbts_bare_1179, &dbts_bare_11e7, &dbts_bare_11e7 },
	}, {
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_11e8, &dbts_bare_11e8 },
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_11e8, &dbts_bare_11e8 },
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_11e8, &dbts_bare_11e8 },
		{ &dbts_1178, &dbts_bare_117a, &dbts_bare_11e8, &dbts_bare_11e8 },
	}, {
		{ &dbts_bare_117d,   &dbts_bare_117e,   &dbts_bare_117f,   &dbts_conc_117f   },
		{ &dbts_bare_117d,   &dbts_bare_117e,   &dbts_bare_117f,   &dbts_conc_117f   },
		{ &dbts_bare_117d_n, &dbts_bare_117e_n, &dbts_bare_117f_n, &dbts_conc_117f_n },
		{ &dbts_bare_117d_w, &dbts_bare_117e_w, &dbts_bare_117f_w, &dbts_conc_117f_w },
	}, {
		{ &dbts_bare_117d,   &dbts_bare_117e,   &dbts_bare_11e1,   &dbts_conc_11e1   },
		{ &dbts_bare_117d,   &dbts_bare_117e,   &dbts_bare_11e1,   &dbts_conc_11e1   },
		{ &dbts_bare_117d_n, &dbts_bare_117e_n, &dbts_bare_11e1_n, &dbts_conc_11e1_n },
		{ &dbts_bare_117d_w, &dbts_bare_117e_w, &dbts_bare_11e1_w, &dbts_conc_11e1_w },
	}, {
		{ &dbts_1180_1181, &dbts_1183_1182_g, &dbts_1183_1182_g, &dbts_1183_1184_g },
		{ &dbts_1180_1181, &dbts_1183_1182_o, &dbts_1183_1182_o, &dbts_1183_1184_o },
		{ &dbts_1180_1181, &dbts_1183_1182_n, &dbts_1183_1182_n, &dbts_1183_1184_n },
		{ &dbts_1180_1181, &dbts_1183_1182_x, &dbts_1183_1182_x, &dbts_1183_1184_x },
	}, {
		{ &dbts_1180_1181, &dbts_1183_1182_g, &dbts_1183_1182_g, &dbts_1183_11e9_g },
		{ &dbts_1180_1181, &dbts_1183_1182_o, &dbts_1183_1182_o, &dbts_1183_11e9_o },
		{ &dbts_1180_1181, &dbts_1183_1182_n, &dbts_1183_1182_n, &dbts_1183_11e9_n },
		{ &dbts_1180_1181, &dbts_1183_1182_x, &dbts_1183_1182_x, &dbts_1183_11e9_x },
	}, {
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_118b },
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_118b },
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_118b },
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_118b },
	}, {
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_118c },
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_118c },
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_118c },
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_118c },
	}, {
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_11ea },
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_11ea },
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_11ea },
		{ &dbts_1185_1187, &dbts_1185_1189, &dbts_1185_1189, &dbts_1185_11ea },
	}, {
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_11eb },
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_11eb },
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_11eb },
		{ &dbts_1186_1188, &dbts_1186_118a, &dbts_1186_118a, &dbts_1186_11eb },
	}, {
		{ &dbts_bare_11ec, &dbts_bare_11ed, &dbts_bare_11ed, &dbts_bare_11ee },
		{ &dbts_bare_11ec, &dbts_bare_11ed, &dbts_bare_11ed, &dbts_bare_11ee },
		{ &dbts_bare_11ec, &dbts_bare_11ed, &dbts_bare_11ed, &dbts_bare_11ee },
		{ &dbts_bare_11ec, &dbts_bare_11ed, &dbts_bare_11ed, &dbts_bare_11ee },
	}, {
		{ &dbts_bare_11ef, &dbts_bare_11f0, &dbts_bare_11f0, &dbts_bare_11f1 },
		{ &dbts_bare_11ef, &dbts_bare_11f0, &dbts_bare_11f0, &dbts_bare_11f1 },
		{ &dbts_bare_11ef, &dbts_bare_11f0, &dbts_bare_11f0, &dbts_bare_11f1 },
		{ &dbts_bare_11ef, &dbts_bare_11f0, &dbts_bare_11f0, &dbts_bare_11f1 },
	}, {
		{ &dbts_bare_11f2, &dbts_bare_11f3, &dbts_bare_11f3, &dbts_bare_11f4 },
		{ &dbts_bare_11f2, &dbts_bare_11f3, &dbts_bare_11f3, &dbts_bare_11f4 },
		{ &dbts_bare_11f2, &dbts_bare_11f3, &dbts_bare_11f3, &dbts_bare_11f4 },
		{ &dbts_bare_11f2, &dbts_bare_11f3, &dbts_bare_11f3, &dbts_bare_11f4 },
	}, {
		{ &dbts_bare_4, &dbts_bare_11f5, &dbts_bare_11f5, &dbts_bare_11f5 },
		{ &dbts_bare_4, &dbts_bare_11f6, &dbts_bare_11f6, &dbts_bare_11f6 },
		{ &dbts_bare_4, &dbts_bare_11f7, &dbts_bare_11f7, &dbts_bare_11f7 },
		{ &dbts_bare_4, &dbts_bare_11f8, &dbts_bare_11f8, &dbts_bare_11f8 },
	}, {
		{ &dbts_bare_11f9, &dbts_bare_11fa, &dbts_bare_11fa, &dbts_bare_11fb },
		{ &dbts_bare_11f9, &dbts_bare_11fa, &dbts_bare_11fa, &dbts_bare_11fb },
		{ &dbts_bare_11f9, &dbts_bare_11fa, &dbts_bare_11fa, &dbts_bare_11fb },
		{ &dbts_bare_11f9, &dbts_bare_11fa, &dbts_bare_11fa, &dbts_bare_11fb },
	}, {
		{ &dbts_bare_u, &dbts_bare_11fc, &dbts_bare_11fc, &dbts_bare_11fc },
		{ &dbts_bare_u, &dbts_bare_11fc, &dbts_bare_11fc, &dbts_bare_11fc },
		{ &dbts_bare_u, &dbts_bare_11fc, &dbts_bare_11fc, &dbts_bare_11fc },
		{ &dbts_bare_u, &dbts_bare_11fc, &dbts_bare_11fc, &dbts_bare_11fc },
	}, {
		{ &dbts_bare_11fd, &dbts_bare_11fe, &dbts_bare_11fe, &dbts_bare_11ff },
		{ &dbts_bare_11fd, &dbts_bare_11fe, &dbts_bare_11fe, &dbts_bare_11ff },
		{ &dbts_bare_11fd, &dbts_bare_11fe, &dbts_bare_11fe, &dbts_bare_11ff },
		{ &dbts_bare_11fd, &dbts_bare_11fe, &dbts_bare_11fe, &dbts_bare_11ff },
	}, {
		{ &dbts_bare_1200,   &dbts_bare_1201,   &dbts_bare_1201,   &dbts_bare_1202   },
		{ &dbts_bare_1200_p, &dbts_bare_1201_p, &dbts_bare_1201_p, &dbts_bare_1202_p },
		{ &dbts_bare_1200_y, &dbts_bare_1201_y, &dbts_bare_1201_y, &dbts_bare_1202_y },
		{ &dbts_bare_1200_r, &dbts_bare_1201_r, &dbts_bare_1201_r, &dbts_bare_1202_r },
	}, {
		{ &dbts_bare_1203,   &dbts_bare_1204,   &dbts_bare_1204,   &dbts_bare_1205   },
		{ &dbts_bare_1203_p, &dbts_bare_1204_p, &dbts_bare_1204_p, &dbts_bare_1205_p },
		{ &dbts_bare_1203_y, &dbts_bare_1204_y, &dbts_bare_1204_y, &dbts_bare_1205_y },
		{ &dbts_bare_1203_r, &dbts_bare_1204_r, &dbts_bare_1204_r, &dbts_bare_1205_r },
	}, {
		{ &dbts_bare_1206, &dbts_bare_1208, &dbts_bare_1208, &dbts_bare_120a },
		{ &dbts_bare_1206, &dbts_bare_1208, &dbts_bare_1208, &dbts_bare_120a },
		{ &dbts_bare_1206, &dbts_bare_1208, &dbts_bare_1208, &dbts_bare_120a },
		{ &dbts_bare_1206, &dbts_bare_1208, &dbts_bare_1208, &dbts_bare_120a },
	}, {
		{ &dbts_bare_1207, &dbts_bare_1209, &dbts_bare_1209, &dbts_bare_120b },
		{ &dbts_bare_1207, &dbts_bare_1209, &dbts_bare_1209, &dbts_bare_120b },
		{ &dbts_bare_1207, &dbts_bare_1209, &dbts_bare_1209, &dbts_bare_120b },
		{ &dbts_bare_1207, &dbts_bare_1209, &dbts_bare_1209, &dbts_bare_120b },
	}, {
		{ &dbts_bare_120c, &dbts_bare_120d, &dbts_bare_120d, &dbts_bare_120e },
		{ &dbts_bare_120c, &dbts_bare_120d, &dbts_bare_120d, &dbts_bare_120e },
		{ &dbts_bare_120c, &dbts_bare_120d, &dbts_bare_120d, &dbts_bare_120e },
		{ &dbts_bare_120c, &dbts_bare_120d, &dbts_bare_120d, &dbts_bare_120e },
	}, {
		{ &dbts_bare_120f, &dbts_bare_1210, &dbts_bare_1210, &dbts_bare_1211 },
		{ &dbts_bare_120f, &dbts_bare_1210, &dbts_bare_1210, &dbts_bare_1211 },
		{ &dbts_bare_120f, &dbts_bare_1210, &dbts_bare_1210, &dbts_bare_1211 },
		{ &dbts_bare_120f, &dbts_bare_1210, &dbts_bare_1210, &dbts_bare_1211 },
	}, {
		{ &dbts_toy1_1213,   &dbts_toy1_1214,   &dbts_toy1_1214,   &dbts_toy1_1215   },
		{ &dbts_toy2_1213_r, &dbts_toy2_1214_r, &dbts_toy2_1214_r, &dbts_toy2_1215_r },
		{ &dbts_toy1_1213_p, &dbts_toy1_1214_p, &dbts_toy1_1214_p, &dbts_toy1_1215_p },
		{ &dbts_toy2_1213_g, &dbts_toy2_1214_g, &dbts_toy2_1214_g, &dbts_toy2_1215_g },
	}, {
		{ &dbts_toy1_1216, &dbts_toy1_1217, &dbts_toy1_1217, &dbts_toy1_1218 },
		{ &dbts_toy1_1216, &dbts_toy1_1217, &dbts_toy1_1217, &dbts_toy1_1218 },
		{ &dbts_toy1_1216, &dbts_toy1_1217, &dbts_toy1_1217, &dbts_toy1_1218 },
		{ &dbts_toy1_1216, &dbts_toy1_1217, &dbts_toy1_1217, &dbts_toy1_1218 },
	}, {
		{ &dbts_toy2_1219, &dbts_toy2_121a, &dbts_toy2_121a, &dbts_toy2_121b },
		{ &dbts_toy2_1219, &dbts_toy2_121a, &dbts_toy2_121a, &dbts_toy2_121b },
		{ &dbts_toy2_1219, &dbts_toy2_121a, &dbts_toy2_121a, &dbts_toy2_121b },
		{ &dbts_toy2_1219, &dbts_toy2_121a, &dbts_toy2_121a, &dbts_toy2_121b },
	}, {
		{ &dbts_toy1_121c, &dbts_toy1_121d, &dbts_toy1_121d, &dbts_toy1_121e },
		{ &dbts_toy1_121c, &dbts_toy1_121d, &dbts_toy1_121d, &dbts_toy1_121e },
		{ &dbts_toy1_121c, &dbts_toy1_121d, &dbts_toy1_121d, &dbts_toy1_121e },
		{ &dbts_toy1_121c, &dbts_toy1_121d, &dbts_toy1_121d, &dbts_toy1_121e },
	}, {
		{ &dbts_toy2_121f,   &dbts_toy2_1220,   &dbts_toy2_1220,   &dbts_toy2_1221   },
		{ &dbts_toy1_121f_p, &dbts_toy1_1220_p, &dbts_toy1_1220_p, &dbts_toy1_1221_p },
		{ &dbts_toy2_121f_m, &dbts_toy2_1220_m, &dbts_toy2_1220_m, &dbts_toy2_1221_m },
		{ &dbts_toy1_121f_c, &dbts_toy1_1220_c, &dbts_toy1_1220_c, &dbts_toy1_1221_c },
	}, {
		{ &dbts_toy2_1222,   &dbts_toy2_1223,   &dbts_toy2_1223,   &dbts_toy2_1224   },
		{ &dbts_toy1_1222_p, &dbts_toy1_1223_p, &dbts_toy1_1223_p, &dbts_toy1_1224_p },
		{ &dbts_toy2_1222_c, &dbts_toy2_1223_c, &dbts_toy2_1223_c, &dbts_toy2_1224_c },
		{ &dbts_toy1_1222_g, &dbts_toy1_1223_g, &dbts_toy1_1223_g, &dbts_toy1_1224_g },
	}, {
		{ &dbts_toy1_1225,   &dbts_toy1_1226,   &dbts_toy1_1226,   &dbts_toy1_1227   },
		{ &dbts_toy2_1225_p, &dbts_toy2_1226_p, &dbts_toy2_1226_p, &dbts_toy2_1227_p },
		{ &dbts_toy2_1225_r, &dbts_toy2_1226_r, &dbts_toy2_1226_r, &dbts_toy2_1227_r },
		{ &dbts_toy1_1225_g, &dbts_toy1_1226_g, &dbts_toy1_1226_g, &dbts_toy1_1227_g },
	}, {
		{ &dbts_toy1_1228, &dbts_toy1_1229, &dbts_toy1_1229, &dbts_toy1_122a },
		{ &dbts_toy1_1228, &dbts_toy1_1229, &dbts_toy1_1229, &dbts_toy1_122a },
		{ &dbts_toy1_1228, &dbts_toy1_1229, &dbts_toy1_1229, &dbts_toy1_122a },
		{ &dbts_toy1_1228, &dbts_toy1_1229, &dbts_toy1_1229, &dbts_toy1_122a },
	}, {
		{ &dbts_toy2_122b, &dbts_toy2_122d, &dbts_toy2_122d, &dbts_toy2_122f },
		{ &dbts_toy2_122b, &dbts_toy2_122d, &dbts_toy2_122d, &dbts_toy2_122f },
		{ &dbts_toy2_122b, &dbts_toy2_122d, &dbts_toy2_122d, &dbts_toy2_122f },
		{ &dbts_toy2_122b, &dbts_toy2_122d, &dbts_toy2_122d, &dbts_toy2_122f },
	}, {
		{ &dbts_toy1_122c, &dbts_toy1_122e, &dbts_toy1_122e, &dbts_toy1_1230 },
		{ &dbts_toy1_122c, &dbts_toy1_122e, &dbts_toy1_122e, &dbts_toy1_1230 },
		{ &dbts_toy1_122c, &dbts_toy1_122e, &dbts_toy1_122e, &dbts_toy1_1230 },
		{ &dbts_toy1_122c, &dbts_toy1_122e, &dbts_toy1_122e, &dbts_toy1_1230 },
	}, {
		{ &dbts_toy2_1231, &dbts_toy2_1232, &dbts_toy2_1232, &dbts_toy2_1233 },
		{ &dbts_toy2_1231, &dbts_toy2_1232, &dbts_toy2_1232, &dbts_toy2_1233 },
		{ &dbts_toy2_1231, &dbts_toy2_1232, &dbts_toy2_1232, &dbts_toy2_1233 },
		{ &dbts_toy2_1231, &dbts_toy2_1232, &dbts_toy2_1232, &dbts_toy2_1233 },
	}, {
		{ &dbts_toy1_1234, &dbts_toy1_1235, &dbts_toy1_1235, &dbts_toy1_1236 },
		{ &dbts_toy1_1234, &dbts_toy1_1235, &dbts_toy1_1235, &dbts_toy1_1236 },
		{ &dbts_toy1_1234, &dbts_toy1_1235, &dbts_toy1_1235, &dbts_toy1_1236 },
		{ &dbts_toy1_1234, &dbts_toy1_1235, &dbts_toy1_1235, &dbts_toy1_1236 },
	}, {
		{ &dbts_toy2_1237, &dbts_toy2_1238, &dbts_toy2_1238, &dbts_toy2_1239 },
		{ &dbts_toy2_1237, &dbts_toy2_1238, &dbts_toy2_1238, &dbts_toy2_1239 },
		{ &dbts_toy2_1237, &dbts_toy2_1238, &dbts_toy2_1238, &dbts_toy2_1239 },
		{ &dbts_toy2_1237, &dbts_toy2_1238, &dbts_toy2_1238, &dbts_toy2_1239 },
	}, {
		{ &dbts_toy1_123a, &dbts_toy1_123b, &dbts_toy1_123b, &dbts_toy1_123c },
		{ &dbts_toy1_123a, &dbts_toy1_123b, &dbts_toy1_123b, &dbts_toy1_123c },
		{ &dbts_toy1_123a, &dbts_toy1_123b, &dbts_toy1_123b, &dbts_toy1_123c },
		{ &dbts_toy1_123a, &dbts_toy1_123b, &dbts_toy1_123b, &dbts_toy1_123c },
	}, {
		{ &dbts_toy2_123d, &dbts_toy2_123e, &dbts_toy2_123e, &dbts_toy2_123f },
		{ &dbts_toy2_123d, &dbts_toy2_123e, &dbts_toy2_123e, &dbts_toy2_123f },
		{ &dbts_toy2_123d, &dbts_toy2_123e, &dbts_toy2_123e, &dbts_toy2_123f },
		{ &dbts_toy2_123d, &dbts_toy2_123e, &dbts_toy2_123e, &dbts_toy2_123f },
	}, {
		{ &dbts_toy1_1240, &dbts_toy1_1241, &dbts_toy1_1241, &dbts_toy1_1242 },
		{ &dbts_toy1_1240, &dbts_toy1_1241, &dbts_toy1_1241, &dbts_toy1_1242 },
		{ &dbts_toy1_1240, &dbts_toy1_1241, &dbts_toy1_1241, &dbts_toy1_1242 },
		{ &dbts_toy1_1240, &dbts_toy1_1241, &dbts_toy1_1241, &dbts_toy1_1242 },
	}, {
		{ &dbts_toy1, &dbts_toy1_1256,   &dbts_toy1_1256,   &dbts_toy1_1256   },
		{ &dbts_toy2, &dbts_toy2_1256_g, &dbts_toy2_1256_g, &dbts_toy2_1256_g },
		{ &dbts_toy1, &dbts_toy1_1256_r, &dbts_toy1_1256_r, &dbts_toy1_1256_r },
		{ &dbts_toy2, &dbts_toy2_1256_c, &dbts_toy2_1256_c, &dbts_toy2_1256_c },
	}, {
		{ &dbts_toy1_1257, &dbts_toy1_1258, &dbts_toy1_1258, &dbts_toy1_1259 },
		{ &dbts_toy1_1257, &dbts_toy1_1258, &dbts_toy1_1258, &dbts_toy1_1259 },
		{ &dbts_toy1_1257, &dbts_toy1_1258, &dbts_toy1_1258, &dbts_toy1_1259 },
		{ &dbts_toy1_1257, &dbts_toy1_1258, &dbts_toy1_1258, &dbts_toy1_1259 },
	}, {
		{ &dbts_toy2, &dbts_toy2_125a, &dbts_toy2_125a, &dbts_toy2_125a },
		{ &dbts_toy2, &dbts_toy2_125a, &dbts_toy2_125a, &dbts_toy2_125a },
		{ &dbts_toy2, &dbts_toy2_125a, &dbts_toy2_125a, &dbts_toy2_125a },
		{ &dbts_toy2, &dbts_toy2_125a, &dbts_toy2_125a, &dbts_toy2_125a },
	}
};

/** Make sure we have the right number of elements. */
assert_compile (lengthof(town_draw_tile_data) == NEW_HOUSE_OFFSET);

/**
 * Describes the data that defines each house in the game
 * @param mnd introduction year of the house
 * @param mxd last year it can be built
 * @param p   population
 * @param rc  cost multiplier for removing it
 * @param bn  building name
 * @param rr  rating decrease if removed
 * @param mg  mail generation multiplier
 * @param ca1 acceptance for 1st CargoID
 * @param ca2 acceptance for 2nd CargoID
 * @param ca3 acceptance for 3rd CargoID
 * @param bf  building flags (size, stadium etc...)
 * @param ba  building availability (zone, climate...)
 * @param cg1 1st CargoID available
 * @param cg2 2nd CargoID available
 * @param cg3 3rd CargoID available
 * @see HouseSpec
 */
#define MS(mnd, mxd, p, rc, bn, rr, mg, ca1, ca2, ca3, bf, ba, cg1, cg2, cg3) \
	{mnd, mxd, p, rc, bn, rr, mg, {ca1, ca2, ca3}, {cg1, cg2, cg3}, bf, ba, true, \
	 GRFFileProps(INVALID_HOUSE_ID), 0, {0, 0, 0, 0}, 16, NO_EXTRA_FLAG, HOUSE_NO_CLASS, {0, 2, 0, 0}, 0, 0, 0}
/** House specifications from original data */
static const HouseSpec _original_house_specs[] = {
	/**
	 *                                                                              remove_rating_decrease
	 *                                                                              |    mail_generation
	 *     min_year                                                                 |    |    1st CargoID acceptance
	 *     |         max_year                                                       |    |    |    2nd CargoID acceptance
	 *     |         |    population                                                |    |    |    |    3th CargoID acceptance
	 *     |         |    |    removal_cost                                         |    |    |    |    |
	 *     |         |    |    |    building_name                                   |    |    |    |    |
	 *     |         |    |    |    |                                               |    |    |    |    |
	 *     |         |    |    |    |                                               |    |    |    |    |
	 * +-building_flags   |    |    |                                               |    |    |    |    |
	 * +-building_availability |    |                                               |    |    |    |    |
	 * +-cargoID accepted |    |    |                                               |    |    |    |    |
	 * |   |         |    |    |    |                                               |    |    |    |    |
	 */
	MS(1963, MAX_YEAR, 187, 150, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      140,  70,   8,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 00
	MS(1957, MAX_YEAR,  85, 140, STR_TOWN_BUILDING_NAME_OFFICE_BLOCK_1,           130,  55,   8,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 01
	MS(1968, MAX_YEAR,  40, 100, STR_TOWN_BUILDING_NAME_SMALL_BLOCK_OF_FLATS_1,    90,  20,   8,   3,   1,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 02
	MS(   0, MAX_YEAR,   5,  90, STR_TOWN_BUILDING_NAME_CHURCH_1,                 230,   2,   2,   0,   0,
	   BUILDING_IS_CHURCH | TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 03
	MS(1975, MAX_YEAR, 220, 160, STR_TOWN_BUILDING_NAME_LARGE_OFFICE_BLOCK_1,     160,  85,  10,   4,   6,
	   BUILDING_IS_ANIMATED | TILE_SIZE_1x1,
	   HZ_TEMP | HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 04
	MS(1975, MAX_YEAR, 220, 160, STR_TOWN_BUILDING_NAME_LARGE_OFFICE_BLOCK_1,     160,  85,  10,   4,   6,
	   BUILDING_IS_ANIMATED | TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE  | HZ_ZON5,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 05
	MS(   0, MAX_YEAR,  30,  80, STR_TOWN_BUILDING_NAME_TOWN_HOUSES_1,             80,  12,   4,   1,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 06
	MS(1959, MAX_YEAR, 140, 180, STR_TOWN_BUILDING_NAME_HOTEL_1,                  150,  22,   6,   1,   2,
	   TILE_SIZE_1x2,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 07
	MS(1959, MAX_YEAR,   0, 180, STR_TOWN_BUILDING_NAME_HOTEL_1,                  150,  22,   6,   1,   2,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 08
	MS(1945, MAX_YEAR,   0,  65, STR_TOWN_BUILDING_NAME_STATUE_1,                  40,   0,   2,   0,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 09
	MS(1945, MAX_YEAR,   0,  65, STR_TOWN_BUILDING_NAME_FOUNTAIN_1,                40,   0,   2,   0,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 0A
	MS(   0, MAX_YEAR,   0,  60, STR_TOWN_BUILDING_NAME_PARK_1,                    75,   0,   2,   0,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 0B
	MS(1935, MAX_YEAR,   0,  60, STR_TOWN_BUILDING_NAME_PARK_1,                    75,   0,   2,   0,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 0C
	MS(1951, MAX_YEAR, 150, 130, STR_TOWN_BUILDING_NAME_OFFICE_BLOCK_2,           110,  65,   8,   2,   4,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 0D
	MS(1930, 1960,      95, 110, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      100,  48,   6,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 0E
	MS(1930, 1960,      95, 105, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      100,  48,   6,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 0F
	MS(1930, 1960,      95, 107, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      100,  48,   6,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 10
	MS(1977, MAX_YEAR, 130, 200, STR_TOWN_BUILDING_NAME_MODERN_OFFICE_BUILDING_1, 150,  50,  10,   3,   6,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 11
	MS(1983, MAX_YEAR,   6, 145, STR_TOWN_BUILDING_NAME_WAREHOUSE_1,              110,  10,   6,   3,   8,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 12
	MS(1985, MAX_YEAR, 110, 155, STR_TOWN_BUILDING_NAME_OFFICE_BLOCK_3,           110,  55,   6,   2,   6,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 13
	MS(   0, MAX_YEAR,  65, 250, STR_TOWN_BUILDING_NAME_STADIUM_1,                300,   5,   4,   0,   0,
	   BUILDING_IS_STADIUM | TILE_SIZE_2x2,
	   HZ_TEMP | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 14
	MS(   0, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_STADIUM_1,                300,   5,   4,   0,   0,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 15
	MS(   0, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_STADIUM_1,                300,   5,   4,   0,   0,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 16
	MS(   0, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_STADIUM_1,                300,   5,   4,   0,   0,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 17
	MS(   0, 1951,      15,  70, STR_TOWN_BUILDING_NAME_OLD_HOUSES_1,              75,   6,   3,   1,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 18
	MS(   0, 1952,      12,  75, STR_TOWN_BUILDING_NAME_COTTAGES_1,                75,   7,   3,   1,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 19
	MS(1931, MAX_YEAR,  13,  71, STR_TOWN_BUILDING_NAME_HOUSES_1,                  75,   8,   3,   1,   0,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 1A
	MS(1935, MAX_YEAR, 100, 135, STR_TOWN_BUILDING_NAME_FLATS_1,                  100,  35,   7,   2,   2,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 1B
	MS(1963, MAX_YEAR, 170, 145, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_2,      170,  50,   8,   3,   3,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 1C
	MS(   0, 1955,     100, 132, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_2,      135,  40,   6,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 1D
	MS(1973, MAX_YEAR, 180, 155, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_3,      180,  64,   8,   3,   3,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 1E
	MS(   0, MAX_YEAR,  35, 220, STR_TOWN_BUILDING_NAME_THEATER_1,                230,  23,   8,   2,   2,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 1F
	MS(1958, MAX_YEAR,  65, 250, STR_TOWN_BUILDING_NAME_STADIUM_2,                300,   5,   4,   0,   0,
	   BUILDING_IS_STADIUM | TILE_SIZE_2x2,
	   HZ_TEMP | HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 20
	MS(1958, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_STADIUM_2,                300,   5,   4,   0,   0,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 21
	MS(1958, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_STADIUM_2,                300,   5,   4,   0,   0,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 22
	MS(1958, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_STADIUM_2,                300,   5,   4,   0,   0,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 23
	MS(2000, MAX_YEAR, 140, 170, STR_TOWN_BUILDING_NAME_OFFICES_1,                250,  65,   8,   3,   2,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 24
	MS(   0, 1960,      15,  70, STR_TOWN_BUILDING_NAME_HOUSES_2,                  75,   6,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 25
	MS(   0, 1960,      15,  70, STR_TOWN_BUILDING_NAME_HOUSES_2,                  75,   6,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 26
	MS(1945, MAX_YEAR,  35, 210, STR_TOWN_BUILDING_NAME_CINEMA_1,                 230,  23,   8,   2,   2,
	   TILE_SIZE_1x1,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 27
	MS(1983, MAX_YEAR, 180, 250, STR_TOWN_BUILDING_NAME_SHOPPING_MALL_1,          300,   5,   8,   2,   3,
	   TILE_SIZE_2x2,
	   HZ_TEMP | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 |HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 28
	MS(1983, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_SHOPPING_MALL_1,          300,   5,   8,   2,   3,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 29
	MS(1983, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_SHOPPING_MALL_1,          300,   5,   8,   2,   3,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 2A
	MS(1983, MAX_YEAR,   0, 250, STR_TOWN_BUILDING_NAME_SHOPPING_MALL_1,          300,   5,   8,   2,   3,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 2B
	MS(   0, MAX_YEAR,  80, 100, STR_TOWN_BUILDING_NAME_FLATS_1,                   90,  20,   5,   2,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 2C
	MS(   0, MAX_YEAR,  80, 100, STR_TOWN_BUILDING_NAME_FLATS_1,                   90,  20,   5,   2,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE  | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 2D
	MS(   0, MAX_YEAR,  16,  70, STR_TOWN_BUILDING_NAME_HOUSES_2,                  70,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 2E
	MS(   0, MAX_YEAR,  16,  70, STR_TOWN_BUILDING_NAME_HOUSES_2,                  70,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 2F
	MS(   0, 1963,      14,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  70,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 30
	MS(   0, 1963,      14,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  70,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 31
	MS(1966, MAX_YEAR, 135, 150, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      120,  60,   8,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 32
	MS(1966, MAX_YEAR, 135, 150, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      120,  60,   8,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 33
	MS(1970, MAX_YEAR, 170, 170, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      130,  70,   9,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 34
	MS(1970, MAX_YEAR, 170, 170, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      130,  70,   9,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 35
	MS(1974, MAX_YEAR, 210, 200, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      140,  80,  10,   3,   5,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 36
	MS(1974, MAX_YEAR, 210, 200, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      140,  80,  10,   3,   5,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 37
	MS(   0, MAX_YEAR,  10,  60, STR_TOWN_BUILDING_NAME_HOUSES_2,                  60,   5,   2,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 38
	MS(   0, MAX_YEAR,  10,  60, STR_TOWN_BUILDING_NAME_HOUSES_2,                  60,   5,   2,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 39
	MS(   0, MAX_YEAR,  25, 100, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,       80,  20,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 3A
	MS(   0, MAX_YEAR,  25, 100, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,       80,  20,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 3B
	MS(   0, MAX_YEAR,   6,  85, STR_TOWN_BUILDING_NAME_CHURCH_1,                 230,   2,   2,   0,   0,
	   BUILDING_IS_CHURCH | TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 3C
	MS(   0, MAX_YEAR,   6,  85, STR_TOWN_BUILDING_NAME_CHURCH_1,                 230,   2,   2,   0,   0,
	   BUILDING_IS_CHURCH | TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 3D
	MS(   0, MAX_YEAR,  17,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   7,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 3E
	MS(   0, MAX_YEAR,  17,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   7,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 3F
	MS(   0, 1960,      90, 140, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      110,  45,   6,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW| HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 40
	MS(   0, 1960,      90, 140, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      110,  45,   6,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE| HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 41
	MS(1972, MAX_YEAR, 140, 160, STR_TOWN_BUILDING_NAME_HOTEL_1,                  160,  25,   6,   1,   3,
	   TILE_SIZE_1x2,
	   HZ_SUBARTC_BELOW| HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 42
	MS(1972, MAX_YEAR,   0, 160, STR_TOWN_BUILDING_NAME_HOTEL_1,                  160,  25,   6,   1,   2,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 43
	MS(1972, MAX_YEAR, 140, 160, STR_TOWN_BUILDING_NAME_HOTEL_1,                  160,  25,   6,   1,   3,
	   TILE_SIZE_1x2,
	   HZ_SUBARTC_ABOVE| HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 44
	MS(1972, MAX_YEAR,   0, 160, STR_TOWN_BUILDING_NAME_HOTEL_1,                  160,  25,   6,   1,   2,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 45
	MS(1963, MAX_YEAR, 105, 130, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      105,  50,   7,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 46
	MS(1963, MAX_YEAR, 105, 130, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      105,  50,   7,   2,   3,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE| HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 47
	MS(1978, MAX_YEAR, 190, 190, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      135,  75,   9,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_BELOW | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 48
	MS(1978, MAX_YEAR, 190, 190, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      135,  75,   9,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 49
	MS(1967, MAX_YEAR, 250, 140, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      200,  60,   7,   2,   2,
	   TILE_SIZE_2x1,
	   HZ_SUBARTC_BELOW| HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 4A
	MS(1967, MAX_YEAR,   0, 140, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      200,  60,   7,   2,   2,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 4B
	MS(1967, MAX_YEAR, 250, 140, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      200,  60,   7,   2,   2,
	   TILE_SIZE_2x1,
	   HZ_SUBARTC_ABOVE | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 4C
	MS(1967, MAX_YEAR,   0, 140, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      200,  60,   7,   2,   2,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 4D
	MS(   0, MAX_YEAR,  16,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 4E
	MS(   0, MAX_YEAR,  16,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 4F
	MS(   0, MAX_YEAR,  16,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   5,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 50
	MS(   0, MAX_YEAR,   7,  30, STR_TOWN_BUILDING_NAME_HOUSES_2,                  30,   4,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 51
	MS(   0, MAX_YEAR,  45, 130, STR_TOWN_BUILDING_NAME_FLATS_1,                   95,  15,   6,   2,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 52
	MS(   0, MAX_YEAR,   8,  90, STR_TOWN_BUILDING_NAME_CHURCH_1,                 200,   3,   2,   0,   0,
	   BUILDING_IS_CHURCH | TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 53
	MS(   0, MAX_YEAR,  18,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   7,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2,
	   CT_PASSENGERS, CT_MAIL, CT_FOOD), // 54
	MS(1973, MAX_YEAR,  90, 110, STR_TOWN_BUILDING_NAME_FLATS_1,                   95,  24,   6,   2,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 55
	MS(1962, MAX_YEAR, 120, 120, STR_TOWN_BUILDING_NAME_FLATS_1,                   95,  25,   6,   2,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 56
	MS(1984, MAX_YEAR, 250, 190, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      140,  80,   8,   3,   4,
	   TILE_SIZE_2x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 57
	MS(1984, MAX_YEAR,   0, 190, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      140,  80,   8,   3,   4,
	   TILE_NO_FLAG,
	   HZ_SUBTROPIC,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 58
	MS(   0, MAX_YEAR,  80, 110, STR_TOWN_BUILDING_NAME_FLATS_1,                   95,  23,   6,   2,   1,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 59
	MS(1993, MAX_YEAR, 180, 180, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      150,  90,   8,   3,   4,
	   TILE_SIZE_1x1,
	   HZ_SUBTROPIC | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_GOODS), // 5A
	MS(   0, MAX_YEAR,   8,  90, STR_TOWN_BUILDING_NAME_CHURCH_1,                 200,   3,   2,   0,   0,
	   BUILDING_IS_CHURCH | TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 5B
	MS(   0, MAX_YEAR,  18,  90, STR_TOWN_BUILDING_NAME_HOUSES_2,                  90,   5,   6,   2,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 5C
	MS(   0, MAX_YEAR,   7,  70, STR_TOWN_BUILDING_NAME_HOUSES_2,                  50,   3,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 5D
	MS(   0, MAX_YEAR,  15,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  75,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 5E
	MS(   0, MAX_YEAR,  17,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  75,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 5F
	MS(   0, MAX_YEAR,  19,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  75,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 60
	MS(   0, MAX_YEAR,  21,  80, STR_TOWN_BUILDING_NAME_HOUSES_2,                  75,   6,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 61
	MS(   0, MAX_YEAR,  75, 160, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      130,  20,   8,   4,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 62
	MS(   0, MAX_YEAR,  35,  90, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   9,   4,   1,   2,
	   TILE_SIZE_1x2,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 63
	MS(   0, MAX_YEAR,   0,  90, STR_TOWN_BUILDING_NAME_HOUSES_2,                  80,   0,   4,   1,   2,
	   TILE_NO_FLAG,
	   HZ_NOZNS,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 64
	MS(   0, MAX_YEAR,  85, 150, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      130,  18,   8,   4,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 65
	MS(   0, MAX_YEAR,  11,  60, STR_TOWN_BUILDING_NAME_IGLOO_1,                   45,   3,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 66
	MS(   0, MAX_YEAR,  10,  60, STR_TOWN_BUILDING_NAME_TEPEES_1,                  45,   3,   3,   1,   1,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 67
	MS(   0, MAX_YEAR,  67, 140, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      130,  22,   8,   4,   4,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_FIZZY_DRINKS), // 68
	MS(   0, MAX_YEAR,  86, 145, STR_TOWN_BUILDING_NAME_SHOPS_AND_OFFICES_1,      130,  23,   8,   4,   4,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_FIZZY_DRINKS), // 69
	MS(   0, MAX_YEAR,  95, 165, STR_TOWN_BUILDING_NAME_TALL_OFFICE_BLOCK_1,      130,  28,   8,   4,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 6A
	MS(   0, MAX_YEAR,  30,  90, STR_TOWN_BUILDING_NAME_STATUE_1,                  70,  10,   4,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 6B
	MS(   0, MAX_YEAR,  25,  75, STR_TOWN_BUILDING_NAME_TEAPOT_HOUSE_1,            65,   8,   3,   1,   2,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_CANDY), // 6C
	MS(   0, MAX_YEAR,  18,  85, STR_TOWN_BUILDING_NAME_PIGGY_BANK_1,              95,   7,   3,   2,   4,
	   TILE_SIZE_1x1,
	   HZ_TOYLND | HZ_ZON5 | HZ_ZON4 | HZ_ZON3 | HZ_ZON2 | HZ_ZON1,
	   CT_PASSENGERS, CT_MAIL, CT_FIZZY_DRINKS), // 6D
};
#undef MS

/** Make sure we have the right number of elements: one entry for each house */
assert_compile(lengthof(_original_house_specs) == NEW_HOUSE_OFFSET);
