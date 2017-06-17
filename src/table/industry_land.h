/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file industry_land.h Information about the behaviour of the default industry tiles. */

/** Industry tile graphics data struct. */
struct DrawIndustryTileStruct {
	SpriteID ground;
	SpriteID building;
	byte subtile_x;
	byte subtile_y;
	byte width;
	byte height;
	byte dz;
	byte draw_proc;  // this allows to specify a special drawing procedure.
};

/**
 * This is used to gather some data about animation
 * drawing in the industry code
 * Image_1-2-3 are in fact only offset in the sprites
 * used by the industry.
 * To specify an invalid one, either 255 or 0 is used,
 * depending of the industry.
 */
struct DrawIndustryAnimationStruct {
	int x;        ///< coordinate x of the first image offset
	byte image_1; ///< image offset 1
	byte image_2; ///< image offset 2
	byte image_3; ///< image offset 3
};

/**
 * Simple structure gathering x,y coordinates for
 * industries animations
 */
struct DrawIndustryCoordinates {
	byte x;  ///< coordinate x of the pair
	byte y;  ///< coordinate y of the pair
};

/** Industry graphics data. */

#define PN(x) (x)
#define PM(x) ((x) | (1 << PALETTE_MODIFIER_COLOUR))

static const DrawIndustryTileStruct dbts_07e6      = {  PN(0x7e6),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_0f54_07db = {  PN(0xf54),  PN(0x7db),   7,  0,  9,  9,  10, 0 };
static const DrawIndustryTileStruct dbts_0f54_07dc = {  PN(0xf54),  PN(0x7dc),   7,  0,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54_07dd = {  PN(0xf54),  PN(0x7dd),   7,  0,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_07e6_07dd = {  PN(0x7e6),  PN(0x7dd),   7,  0,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_07e6_07de = {  PN(0x7e6),  PN(0x7de),   7,  0,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_07e6_07df = {  PN(0x7e6),  PN(0x7df),   7,  0,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54_07e0 = {  PN(0xf54),  PN(0x7e0),   1,  2, 15,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54_07e1 = {  PN(0xf54),  PN(0x7e1),   1,  2, 15,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54_07e2 = {  PN(0xf54),  PN(0x7e2),   1,  2, 15,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_07e6_07e2 = {  PN(0x7e6),  PN(0x7e2),   1,  2, 15,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54_07e3 = {  PN(0xf54),  PN(0x7e3),   4,  4,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54_07e4 = {  PN(0xf54),  PN(0x7e4),   4,  4,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54_07e5 = {  PN(0xf54),  PN(0x7e5),   4,  4,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_07e6_07e5 = {  PN(0x7e6),  PN(0x7e5),   4,  4,  9,  9,  30, 0 };
static const DrawIndustryTileStruct dbts_0f54      = {  PN(0xf54),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_07e9      = {  PN(0x7e9),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_07e7      = {  PN(0x7e7),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_07e8      = {  PN(0x7e8),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_0f54_07fd = {  PN(0xf54),  PN(0x7fd),   1,  1, 14, 14,   5, 0 };
static const DrawIndustryTileStruct dbts_0f54_07fe = {  PN(0xf54),  PN(0x7fe),   1,  1, 14, 14,  44, 0 };
static const DrawIndustryTileStruct dbts_0f54_07ff = {  PN(0xf54),  PN(0x7ff),   1,  1, 14, 14,  44, 0 };
static const DrawIndustryTileStruct dbts_0f54_0800 = {  PN(0xf54),  PN(0x800),   0,  2, 16, 12,   6, 0 };
static const DrawIndustryTileStruct dbts_0f54_0801 = {  PN(0xf54),  PN(0x801),   0,  2, 16, 12,  47, 0 };
static const DrawIndustryTileStruct dbts_0f54_0802 = {  PN(0xf54),  PN(0x802),   0,  2, 16, 12,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0803 = {  PN(0xf54),  PN(0x803),   1,  0, 14, 15,   5, 0 };
static const DrawIndustryTileStruct dbts_0f54_0804 = {  PN(0xf54),  PN(0x804),   1,  0, 14, 15,  19, 0 };
static const DrawIndustryTileStruct dbts_0f54_0805 = {  PN(0xf54),  PN(0x805),   1,  0, 14, 15,  21, 0 };
static const DrawIndustryTileStruct dbts_0f54_0806 = {  PN(0xf54),  PN(0x806),   1,  2, 14, 11,  32, 5 };
static const DrawIndustryTileStruct dbts_0f54_080d = {  PN(0xf54),  PN(0x80d),   1,  0, 13, 16,   8, 0 };
static const DrawIndustryTileStruct dbts_0f54_080e = {  PN(0xf54),  PN(0x80e),   1,  0, 13, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0f54_080f = {  PN(0xf54),  PN(0x80f),   1,  0, 13, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0f54_0810 = {  PN(0xf54),  PN(0x810),   0,  1, 16, 14,   8, 0 };
static const DrawIndustryTileStruct dbts_0f54_0811 = {  PN(0xf54),  PN(0x811),   0,  1, 16, 14,  21, 0 };
static const DrawIndustryTileStruct dbts_0f54_0812 = {  PN(0xf54),  PN(0x812),   0,  1, 16, 14,  21, 0 };
static const DrawIndustryTileStruct dbts_0f54_0813 = {  PN(0xf54),  PN(0x813),   1,  1, 14, 14,  12, 0 };
static const DrawIndustryTileStruct dbts_0f54_0814 = {  PN(0xf54),  PN(0x814),   1,  1, 14, 14,  15, 0 };
static const DrawIndustryTileStruct dbts_0f54_0815 = {  PN(0xf54),  PN(0x815),   1,  1, 14, 14,  22, 0 };
static const DrawIndustryTileStruct dbts_0f54_0816 = {  PN(0xf54),  PN(0x816),   0,  0, 16, 15,  20, 0 };
static const DrawIndustryTileStruct dbts_0f54_0817 = {  PN(0xf54),  PN(0x817),   0,  1, 16, 13,  19, 0 };
static const DrawIndustryTileStruct dbts_081d_0818 = {  PN(0x81d),  PN(0x818),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_0819 = {  PN(0x81d),  PN(0x819),   0,  0, 16, 16,  15, 0 };
static const DrawIndustryTileStruct dbts_081d_081a = {  PN(0x81d),  PN(0x81a),   0,  0, 16, 16,  31, 0 };
static const DrawIndustryTileStruct dbts_081d_081b = {  PN(0x81d),  PN(0x81b),   0,  0, 16, 16,  39, 0 };
static const DrawIndustryTileStruct dbts_081d_081c = {  PN(0x81d),  PN(0x81c),   0,  0, 16, 16,   7, 0 };
static const DrawIndustryTileStruct dbts_0f54_081e = {  PN(0xf54),  PM(0x81e),   1,  1, 14, 14,   4, 0 };
static const DrawIndustryTileStruct dbts_0f54_081f = {  PN(0xf54),  PM(0x81f),   1,  1, 14, 14,  24, 0 };
static const DrawIndustryTileStruct dbts_0f54_0820 = {  PN(0xf54),  PM(0x820),   1,  1, 14, 14,  27, 0 };
static const DrawIndustryTileStruct dbts_058c_0820 = {  PN(0x58c),  PM(0x820),   1,  1, 14, 14,  27, 0 };
static const DrawIndustryTileStruct dbts_0f54_0821 = {  PN(0xf54),  PM(0x821),   3,  3, 10,  9,   3, 0 };
static const DrawIndustryTileStruct dbts_0f54_0822 = {  PN(0xf54),  PM(0x822),   3,  3, 10,  9,  63, 0 };
static const DrawIndustryTileStruct dbts_0f54_0823 = {  PN(0xf54),  PM(0x823),   3,  3, 10,  9,  62, 0 };
static const DrawIndustryTileStruct dbts_058c_0823 = {  PN(0x58c),  PM(0x823),   3,  3, 10,  9,  62, 0 };
static const DrawIndustryTileStruct dbts_0f54_0824 = {  PN(0xf54),  PM(0x824),   4,  4,  7,  7,   3, 0 };
static const DrawIndustryTileStruct dbts_0f54_0825 = {  PN(0xf54),  PM(0x825),   4,  4,  7,  7,  72, 0 };
static const DrawIndustryTileStruct dbts_058c_0826 = {  PN(0x58c),  PM(0x826),   4,  4,  7,  7,  80, 0 };
static const DrawIndustryTileStruct dbts_0f54_0827 = {  PN(0xf54),  PM(0x827),   2,  0, 12, 16,  51, 0 };
static const DrawIndustryTileStruct dbts_0f54_0828 = {  PN(0xf54),  PM(0x828),   2,  0, 12, 16,  51, 0 };
static const DrawIndustryTileStruct dbts_0f54_0829 = {  PN(0xf54),  PM(0x829),   2,  0, 12, 16,  51, 0 };
static const DrawIndustryTileStruct dbts_058c_0829 = {  PN(0x58c),  PM(0x829),   2,  0, 12, 16,  51, 0 };
static const DrawIndustryTileStruct dbts_0f54_082a = {  PN(0xf54),  PM(0x82a),   0,  0, 16, 16,  26, 0 };
static const DrawIndustryTileStruct dbts_0f54_082b = {  PN(0xf54),  PM(0x82b),   0,  0, 16, 16,  44, 0 };
static const DrawIndustryTileStruct dbts_0f54_082c = {  PN(0xf54),  PM(0x82c),   0,  0, 16, 16,  46, 0 };
static const DrawIndustryTileStruct dbts_058c_082c = {  PN(0x58c),  PM(0x82c),   0,  0, 16, 16,  46, 0 };
static const DrawIndustryTileStruct dbts_0f54_082d = {  PN(0xf54),  PN(0x82d),   3,  1, 10, 13,   2, 0 };
static const DrawIndustryTileStruct dbts_0f54_082e = {  PN(0xf54),  PN(0x82e),   3,  1, 10, 13,  11, 0 };
static const DrawIndustryTileStruct dbts_0f54_082f = {  PN(0xf54),  PN(0x82f),   3,  1, 10, 13,  11, 0 };
static const DrawIndustryTileStruct dbts_058c_082f = {  PN(0x58c),  PN(0x82f),   3,  1, 10, 13,  11, 0 };
static const DrawIndustryTileStruct dbts_0fdd      = {  PN(0xfdd),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0833 = {  PN(0xfdd),  PN(0x833),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0837 = {  PN(0xfdd),  PN(0x837),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0834 = {  PN(0xfdd),  PN(0x834),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0830 = {  PN(0xfdd),  PN(0x830),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0838 = {  PN(0xfdd),  PN(0x838),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0835 = {  PN(0xfdd),  PN(0x835),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0831 = {  PN(0xfdd),  PN(0x831),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0839 = {  PN(0xfdd),  PN(0x839),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0836 = {  PN(0xfdd),  PN(0x836),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0fdd_0832 = {  PN(0xfdd),  PN(0x832),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_087d_087e = {  PN(0x87d),  PN(0x87e),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_087d_087f = {  PN(0x87d),  PN(0x87f),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_087d_0880 = {  PN(0x87d),  PN(0x880),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_087d_0881 = {  PN(0x87d),  PN(0x881),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_087d_0882 = {  PN(0x87d),  PN(0x882),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_087d_0883 = {  PN(0x87d),  PN(0x883),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_083a_083c = {  PN(0x83a),  PM(0x83c),   0,  0, 16, 16,  18, 0 };
static const DrawIndustryTileStruct dbts_083b_083d = {  PN(0x83b),  PM(0x83d),   0,  0, 16, 16,  18, 0 };
static const DrawIndustryTileStruct dbts_083e_083f = {  PN(0x83e),  PM(0x83f),   0,  0, 16, 16,  18, 0 };
static const DrawIndustryTileStruct dbts_0840_0841 = {  PN(0x840),  PN(0x841),   0,  0, 16, 16,  18, 0 };
static const DrawIndustryTileStruct dbts_0842_0843 = {  PN(0x842),  PN(0x843),   0,  0, 16, 16,  30, 0 };
static const DrawIndustryTileStruct dbts_0844_0845 = {  PN(0x844),  PN(0x845),   0,  0, 16, 16,  16, 0 };
static const DrawIndustryTileStruct dbts_07e6_0869 = {  PN(0x7e6),  PN(0x869),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_086d = {  PN(0x7e6),  PN(0x86d),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0862_0866 = {  PM(0x862),  PM(0x866),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_086a = {  PN(0x7e6),  PN(0x86a),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_086e = {  PN(0x7e6),  PN(0x86e),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0863_0867 = {  PM(0x863),  PM(0x867),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_086b = {  PN(0x7e6),  PN(0x86b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_086f = {  PN(0x7e6),  PN(0x86f),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0864_0868 = {  PM(0x864),  PM(0x868),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_086c = {  PN(0x7e6),  PN(0x86c),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0870 = {  PN(0x7e6),  PN(0x870),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0865      = {  PM(0x865),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_0f54_0871 = {  PN(0xf54),  PM(0x871),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0875 = {  PN(0xf54),  PM(0x875),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0879 = {  PN(0xf54),  PM(0x879),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0872 = {  PN(0xf54),  PM(0x872),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0876 = {  PN(0xf54),  PM(0x876),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_087a = {  PN(0xf54),  PM(0x87a),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0873 = {  PN(0xf54),  PM(0x873),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0877 = {  PN(0xf54),  PM(0x877),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_087b = {  PN(0xf54),  PM(0x87b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0874 = {  PN(0xf54),  PM(0x874),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_0878 = {  PN(0xf54),  PM(0x878),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_087c = {  PN(0xf54),  PM(0x87c),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f54_07ea = {  PN(0xf54),  PN(0x7ea),   3,  2,  8,  8,  18, 0 };
static const DrawIndustryTileStruct dbts_0f54_07eb = {  PN(0xf54),  PN(0x7eb),   3,  2,  8,  8,  37, 0 };
static const DrawIndustryTileStruct dbts_0f54_07ec = {  PN(0xf54),  PN(0x7ec),   3,  2,  8,  8,  49, 0 };
static const DrawIndustryTileStruct dbts_07e6_07ec = {  PN(0x7e6),  PN(0x7ec),   3,  2,  8,  8,  49, 0 };
static const DrawIndustryTileStruct dbts_07e6_07ed = {  PN(0x7e6),  PN(0x7ed),   3,  2,  8,  8,  49, 0 };
static const DrawIndustryTileStruct dbts_07e6_07ee = {  PN(0x7e6),  PN(0x7ee),   3,  2,  8,  8,  49, 0 };
static const DrawIndustryTileStruct dbts_0f54_07ef = {  PN(0xf54),  PN(0x7ef),   3,  2, 10,  7,  20, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f0 = {  PN(0xf54),  PN(0x7f0),   3,  2, 10,  7,  40, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f1 = {  PN(0xf54),  PN(0x7f1),   3,  2, 10,  7,  40, 0 };
static const DrawIndustryTileStruct dbts_07e6_07f1 = {  PN(0x7e6),  PN(0x7f1),   3,  2, 10,  7,  40, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f2 = {  PN(0xf54),  PN(0x7f2),   4,  4,  7,  8,  22, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f3 = {  PN(0xf54),  PN(0x7f3),   4,  4,  7,  8,  22, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f4 = {  PN(0xf54),  PN(0x7f4),   4,  4,  7,  8,  22, 0 };
static const DrawIndustryTileStruct dbts_07e6_07f4 = {  PN(0x7e6),  PN(0x7f4),   4,  4,  7,  8,  22, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f5 = {  PN(0xf54),  PN(0x7f5),   2,  1, 11, 13,  12, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f6 = {  PN(0xf54),  PN(0x7f6),   2,  1, 11, 13,  12, 0 };
static const DrawIndustryTileStruct dbts_0f54_07f7 = {  PN(0xf54),  PN(0x7f7),   2,  1, 11, 13,  12, 0 };
static const DrawIndustryTileStruct dbts_07e6_07f7 = {  PN(0x7e6),  PN(0x7f7),   2,  1, 11, 13,  12, 0 };
static const DrawIndustryTileStruct dbts_07e6_085c = {  PN(0x7e6),  PN(0x85c),   0,  0,  1,  1,   1, 0 };
static const DrawIndustryTileStruct dbts_0851_0852 = {  PN(0x851),  PN(0x852),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0846_0847 = {  PM(0x846),  PM(0x847),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_07e6_085d = {  PN(0x7e6),  PN(0x85d),   0,  0,  1,  1,   1, 0 };
static const DrawIndustryTileStruct dbts_0853_0854 = {  PN(0x853),  PN(0x854),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_0848_0849 = {  PM(0x848),  PM(0x849),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_07e6_085e = {  PN(0x7e6),  PN(0x85e),   0,  0,  1,  1,   1, 0 };
static const DrawIndustryTileStruct dbts_0855_0856 = {  PN(0x855),  PN(0x856),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_084a_084b = {  PM(0x84a),  PM(0x84b),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_07e6_085f = {  PN(0x7e6),  PN(0x85f),   0,  0,  1,  1,   1, 0 };
static const DrawIndustryTileStruct dbts_0857      = {  PN(0x857),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_084c      = {  PM(0x84c),    PN(0x0),   0,  0,  1,  1,   0, 0 };
static const DrawIndustryTileStruct dbts_07e6_0860 = {  PN(0x7e6),  PN(0x860),   0,  0,  1,  1,   1, 0 };
static const DrawIndustryTileStruct dbts_0858_0859 = {  PN(0x858),  PN(0x859),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_084d_084e = {  PM(0x84d),  PM(0x84e),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_07e6_0861 = {  PN(0x7e6),  PN(0x861),   0,  0,  1,  1,   1, 0 };
static const DrawIndustryTileStruct dbts_085a_085b = {  PN(0x85a),  PN(0x85b),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_084f_0850 = {  PM(0x84f),  PM(0x850),   0,  0, 16, 16,  20, 0 };
static const DrawIndustryTileStruct dbts_07e6_0884 = {  PN(0x7e6),  PN(0x884),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0886_0884 = {  PN(0x886),  PN(0x884),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0885 = {  PN(0x7e6),  PN(0x885),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0887_0885 = {  PN(0x887),  PN(0x885),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_088c = {  PN(0x7e6),  PM(0x88c),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_088d = {  PN(0x7e6),  PM(0x88d),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_088e = {  PN(0x7e6),  PM(0x88e),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_088f = {  PN(0x7e6),  PM(0x88f),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0890 = {  PN(0x7e6),  PM(0x890),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0891 = {  PN(0x7e6),  PM(0x891),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0892 = {  PN(0x7e6),  PM(0x892),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0893 = {  PN(0x7e6),  PM(0x893),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0894 = {  PN(0x7e6),  PM(0x894),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0895 = {  PN(0x7e6),  PM(0x895),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0896 = {  PN(0x7e6),  PM(0x896),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0897 = {  PN(0x7e6),  PM(0x897),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0898 = {  PN(0x7e6),  PN(0x898),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0899 = {  PN(0x7e6),  PN(0x899),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_089a = {  PN(0x7e6),  PN(0x89a),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08a6 = {  PN(0x7e6),  PN(0x8a6),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_089b = {  PN(0x7e6),  PN(0x89b),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_089c = {  PN(0x7e6),  PN(0x89c),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_089d = {  PN(0x7e6),  PN(0x89d),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_089e = {  PN(0x7e6),  PN(0x89e),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_089f = {  PN(0x7e6),  PN(0x89f),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08a0 = {  PN(0x7e6),  PN(0x8a0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08a1 = {  PN(0x7e6),  PN(0x8a1),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08a2 = {  PN(0x7e6),  PN(0x8a2),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08a3 = {  PN(0x7e6),  PN(0x8a3),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08a4 = {  PN(0x7e6),  PN(0x8a4),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08a5 = {  PN(0x7e6),  PN(0x8a5),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08a7      = {  PN(0x8a7),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08b7 = {  PN(0x7e6),  PN(0x8b7),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08c7 = {  PN(0x7e6),  PN(0x8c7),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08a8      = {  PN(0x8a8),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b8      = {  PN(0x8b8),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c8      = {  PN(0x8c8),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08a9      = {  PN(0x8a9),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08b9 = {  PN(0x7e6),  PN(0x8b9),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08c9 = {  PN(0x7e6),  PN(0x8c9),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08aa      = {  PN(0x8aa),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08ba = {  PN(0x7e6),  PN(0x8ba),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08ca = {  PN(0x7e6),  PN(0x8ca),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08ab      = {  PN(0x8ab),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08bb      = {  PN(0x8bb),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08cb      = {  PN(0x8cb),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08ac      = {  PN(0x8ac),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08bc      = {  PN(0x8bc),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08cc      = {  PN(0x8cc),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08ad      = {  PN(0x8ad),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08bd      = {  PN(0x8bd),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08cd      = {  PN(0x8cd),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08ae      = {  PN(0x8ae),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08be      = {  PN(0x8be),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08ce_08d7 = {  PN(0x8ce),  PN(0x8d7),   0,  0, 16, 16,  35, 0 };
static const DrawIndustryTileStruct dbts_08ce_08d8 = {  PN(0x8ce),  PN(0x8d8),   0,  0, 16, 16,  35, 0 };
static const DrawIndustryTileStruct dbts_08ce_08d9 = {  PN(0x8ce),  PN(0x8d9),   0,  0, 16, 16,  35, 0 };
static const DrawIndustryTileStruct dbts_08af      = {  PN(0x8af),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08bf      = {  PN(0x8bf),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08cf      = {  PN(0x8cf),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b0      = {  PN(0x8b0),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c0      = {  PN(0x8c0),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08d0      = {  PN(0x8d0),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b1      = {  PN(0x8b1),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c1      = {  PN(0x8c1),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08d1      = {  PN(0x8d1),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b2      = {  PN(0x8b2),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c2      = {  PN(0x8c2),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08d2      = {  PN(0x8d2),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b3      = {  PN(0x8b3),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c3      = {  PN(0x8c3),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08d3      = {  PN(0x8d3),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b4      = {  PN(0x8b4),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c4      = {  PN(0x8c4),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08d4      = {  PN(0x8d4),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b5      = {  PN(0x8b5),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c5      = {  PN(0x8c5),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08d5      = {  PN(0x8d5),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08b6      = {  PN(0x8b6),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08c6      = {  PN(0x8c6),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08d6      = {  PN(0x8d6),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_088a = {  PN(0x7e6),  PN(0x88a),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0888_088a = {  PN(0x888),  PN(0x88a),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_088b = {  PN(0x7e6),  PN(0x88b),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0889_088b = {  PN(0x889),  PN(0x88b),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08da      = {  PN(0x8da),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08e3 = {  PN(0x7e6),  PN(0x8e3),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08ec = {  PN(0x7e6),  PN(0x8ec),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08db      = {  PN(0x8db),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08e4 = {  PN(0x7e6),  PN(0x8e4),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08ed = {  PN(0x7e6),  PN(0x8ed),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08dc      = {  PN(0x8dc),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08e5 = {  PN(0x7e6),  PN(0x8e5),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08ee = {  PN(0x7e6),  PN(0x8ee),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08dd      = {  PN(0x8dd),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08e6 = {  PN(0x7e6),  PN(0x8e6),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08ef = {  PN(0x7e6),  PN(0x8ef),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08de      = {  PN(0x8de),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08e7      = {  PN(0x8e7),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f0      = {  PN(0x8f0),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08df      = {  PN(0x8df),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08e8      = {  PN(0x8e8),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f1      = {  PN(0x8f1),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08e0      = {  PN(0x8e0),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08e9 = {  PN(0x7e6),  PN(0x8e9),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_08f2 = {  PN(0x7e6),  PN(0x8f2),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08e1      = {  PN(0x8e1),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08ea      = {  PN(0x8ea),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f3      = {  PN(0x8f3),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08e2      = {  PN(0x8e2),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08eb      = {  PN(0x8eb),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f4      = {  PN(0x8f4),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f5      = {  PN(0x8f5),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0905      = {  PN(0x905),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0915      = {  PN(0x915),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f6      = {  PN(0x8f6),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0906      = {  PN(0x906),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0916      = {  PN(0x916),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f7      = {  PN(0x8f7),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0907      = {  PN(0x907),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0917      = {  PN(0x917),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f8      = {  PN(0x8f8),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0908      = {  PN(0x908),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0918      = {  PN(0x918),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08f9      = {  PN(0x8f9),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0909      = {  PN(0x909),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0919      = {  PN(0x919),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08fa      = {  PN(0x8fa),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_090a      = {  PN(0x90a),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_091a      = {  PN(0x91a),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08fb      = {  PN(0x8fb),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_090b      = {  PN(0x90b),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_091b      = {  PN(0x91b),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08fc      = {  PN(0x8fc),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_090c      = {  PN(0x90c),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_091c      = {  PN(0x91c),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08fd      = {  PN(0x8fd),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_090d      = {  PN(0x90d),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_091d      = {  PN(0x91d),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08fe      = {  PN(0x8fe),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_090e      = {  PN(0x90e),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_091e      = {  PN(0x91e),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_08ff      = {  PN(0x8ff),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_090f      = {  PN(0x90f),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_091f      = {  PN(0x91f),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0900      = {  PN(0x900),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0910      = {  PN(0x910),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0920      = {  PN(0x920),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0901      = {  PN(0x901),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0911      = {  PN(0x911),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0921      = {  PN(0x921),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0902      = {  PN(0x902),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0912      = {  PN(0x912),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0922      = {  PN(0x922),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0903      = {  PN(0x903),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0913      = {  PN(0x913),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0923      = {  PN(0x923),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0904      = {  PN(0x904),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0914      = {  PN(0x914),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0924      = {  PN(0x924),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0925      = {  PN(0x925),    PN(0x0),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_0925_0926 = {  PN(0x925),  PN(0x926),   0,  0, 16, 16,  30, 0 };
static const DrawIndustryTileStruct dbts_0925_0927 = {  PN(0x925),  PN(0x927),   0,  0, 16, 16,  30, 0 };
static const DrawIndustryTileStruct dbts_11c6_092b = { PN(0x11c6),  PM(0x92b),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_092c = { PN(0x11c6),  PM(0x92c),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_092d = { PN(0x11c6),  PM(0x92d),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_092e = { PN(0x11c6),  PM(0x92e),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_092f = { PN(0x11c6),  PM(0x92f),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_0930 = { PN(0x11c6),  PM(0x930),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_0928 = { PN(0x11c6),  PM(0x928),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_0929 = { PN(0x11c6),  PM(0x929),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_11c6_092a = { PN(0x11c6),  PM(0x92a),   0,  0, 16, 16,  25, 0 };
static const DrawIndustryTileStruct dbts_07e6_0931 = {  PN(0x7e6),  PN(0x931),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0935 = {  PN(0x7e6),  PN(0x935),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0939 = {  PN(0x7e6),  PN(0x939),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0932 = {  PN(0x7e6),  PN(0x932),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0936 = {  PN(0x7e6),  PN(0x936),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_093a = {  PN(0x7e6),  PN(0x93a),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0933 = {  PN(0x7e6),  PN(0x933),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0937 = {  PN(0x7e6),  PN(0x937),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_093b = {  PN(0x7e6),  PN(0x93b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0934 = {  PN(0x7e6),  PN(0x934),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_0938 = {  PN(0x7e6),  PN(0x938),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_093c = {  PN(0x7e6),  PN(0x93c),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1245 = {  PN(0x7e6), PM(0x1245),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1248 = {  PN(0x7e6), PM(0x1248),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_124b = {  PN(0x7e6), PM(0x124b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1247 = {  PN(0x7e6), PM(0x1247),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_124a = {  PN(0x7e6), PM(0x124a),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_124d = {  PN(0x7e6), PM(0x124d),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1246 = {  PN(0x7e6), PM(0x1246),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1249 = {  PN(0x7e6), PM(0x1249),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_124c = {  PN(0x7e6), PM(0x124c),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_081d_124e = {  PN(0x81d), PN(0x124e),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_124f = {  PN(0x81d), PN(0x124f),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_1250 = {  PN(0x81d), PN(0x1250),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_1251 = {  PN(0x81d), PN(0x1251),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_1252 = {  PN(0x81d), PN(0x1252),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_1253 = {  PN(0x81d), PN(0x1253),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_1254 = {  PN(0x81d), PN(0x1254),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_081d_1255 = {  PN(0x81d), PN(0x1255),   0,  0, 16, 16,  10, 0 };
static const DrawIndustryTileStruct dbts_07e6_125b = {  PN(0x7e6), PN(0x125b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_125e = {  PN(0x7e6), PN(0x125e),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1261 = {  PN(0x7e6), PN(0x1261),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_125c = {  PN(0x7e6), PN(0x125c),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_125f = {  PN(0x7e6), PN(0x125f),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1262 = {  PN(0x7e6), PN(0x1262),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_125d = {  PN(0x7e6), PN(0x125d),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1260 = {  PN(0x7e6), PN(0x1260),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_07e6_1263 = {  PN(0x7e6), PN(0x1263),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243      = { PN(0x1243),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_1264 = { PN(0x1243), PN(0x1264),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_1268 = { PN(0x1243), PN(0x1268),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_1265 = { PN(0x1243), PN(0x1265),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_1269 = { PN(0x1243), PN(0x1269),   0,  0, 16, 16,  50, 4 };
static const DrawIndustryTileStruct dbts_1243_1266 = { PN(0x1243), PN(0x1266),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_126a = { PN(0x1243), PN(0x126a),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_1267 = { PN(0x1243), PN(0x1267),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_126b = { PN(0x1243), PN(0x126b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_126c = { PN(0x1243), PN(0x126c),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1271      = { PN(0x1271),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1271_1279 = { PN(0x1271), PN(0x1279),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1272      = { PN(0x1272),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1272_127a = { PN(0x1272), PN(0x127a),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1273      = { PN(0x1273),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1273_127b = { PN(0x1273), PN(0x127b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1274      = { PN(0x1274),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1274_127c = { PN(0x1274), PN(0x127c),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1275      = { PN(0x1275),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1275_127d = { PN(0x1275), PN(0x127d),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1276      = { PN(0x1276),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1276_127e = { PN(0x1276), PN(0x127e),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1277      = { PN(0x1277),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1277_127f = { PN(0x1277), PN(0x127f),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1278      = { PN(0x1278),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1278_1280 = { PN(0x1278), PN(0x1280),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244      = { PN(0x1244),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244_1284 = { PN(0x1244), PM(0x1284),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244_1283 = { PN(0x1244), PM(0x1283),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244_1286 = { PN(0x1244), PM(0x1286),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244_1281 = { PN(0x1244), PM(0x1281),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244_1282 = { PN(0x1244), PM(0x1282),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244_1285 = { PN(0x1244), PM(0x1285),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_1287 = { PN(0x1243), PM(0x1287),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1244_1288 = { PN(0x1244), PM(0x1288),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_1243_1289 = { PN(0x1243), PM(0x1289),   0,  0, 16, 16,  50, 3 };
static const DrawIndustryTileStruct dbts_0f8d_129b = {  PN(0xf8d), PN(0x129b),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f8d_129c = {  PN(0xf8d), PN(0x129c),   0,  0, 16, 16,  50, 2 };
static const DrawIndustryTileStruct dbts_0f8d_129d = {  PN(0xf8d), PN(0x129d),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_12a0      = { PM(0x12a0),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_12a1      = { PM(0x12a1),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_12a2      = { PM(0x12a2),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_12a3      = { PM(0x12a3),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f8d      = {  PN(0xf8d),    PN(0x0),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f8d_12a4 = {  PN(0xf8d), PM(0x12a4),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f8d_12a6 = {  PN(0xf8d), PM(0x12a6),   0,  0, 16, 16,  50, 0 };
static const DrawIndustryTileStruct dbts_0f8d_12a5 = {  PN(0xf8d), PM(0x12a5),   0,  0, 16, 16,  50, 1 };

#undef PN
#undef PM

/** Structure for industry tiles drawing */
static const DrawIndustryTileStruct *const industry_draw_tile_data[NEW_INDUSTRYTILEOFFSET][4] = {
	{ &dbts_0f54_07db, &dbts_0f54_07dc, &dbts_0f54_07dd, &dbts_07e6_07dd },
	{ &dbts_07e6_07dd, &dbts_07e6_07de, &dbts_07e6_07df, &dbts_07e6_07df },
	{ &dbts_0f54_07e0, &dbts_0f54_07e1, &dbts_0f54_07e2, &dbts_07e6_07e2 },
	{ &dbts_0f54_07e3, &dbts_0f54_07e4, &dbts_0f54_07e5, &dbts_07e6_07e5 },
	{ &dbts_0f54,      &dbts_0f54,      &dbts_07e6,      &dbts_07e9      },
	{ &dbts_0f54,      &dbts_0f54,      &dbts_07e6,      &dbts_07e7      },
	{ &dbts_0f54,      &dbts_0f54,      &dbts_07e6,      &dbts_07e8      },
	{ &dbts_0f54_07fd, &dbts_0f54_07fe, &dbts_0f54_07ff, &dbts_0f54_07ff },
	{ &dbts_0f54_0800, &dbts_0f54_0801, &dbts_0f54_0802, &dbts_0f54_0802 },
	{ &dbts_0f54_0803, &dbts_0f54_0804, &dbts_0f54_0805, &dbts_0f54_0805 },
	{ &dbts_0f54,      &dbts_0f54,      &dbts_0f54,      &dbts_0f54_0806 },
	{ &dbts_0f54_080d, &dbts_0f54_080e, &dbts_0f54_080f, &dbts_0f54_080f },
	{ &dbts_0f54_0810, &dbts_0f54_0811, &dbts_0f54_0812, &dbts_0f54_0812 },
	{ &dbts_0f54_0813, &dbts_0f54_0814, &dbts_0f54_0815, &dbts_0f54_0815 },
	{ &dbts_0f54,      &dbts_0f54,      &dbts_0f54,      &dbts_0f54_0816 },
	{ &dbts_0f54,      &dbts_0f54,      &dbts_0f54,      &dbts_0f54_0817 },
	{ &dbts_081d_0818, &dbts_081d_0819, &dbts_081d_081a, &dbts_081d_081b },
	{ &dbts_081d_081c, &dbts_081d_081c, &dbts_081d_081c, &dbts_081d_081c },
	{ &dbts_0f54_081e, &dbts_0f54_081f, &dbts_0f54_0820, &dbts_058c_0820 },
	{ &dbts_0f54_0821, &dbts_0f54_0822, &dbts_0f54_0823, &dbts_058c_0823 },
	{ &dbts_0f54_0824, &dbts_0f54_0825, &dbts_0f54_0825, &dbts_058c_0826 },
	{ &dbts_0f54_0827, &dbts_0f54_0828, &dbts_0f54_0829, &dbts_058c_0829 },
	{ &dbts_0f54_082a, &dbts_0f54_082b, &dbts_0f54_082c, &dbts_058c_082c },
	{ &dbts_0f54_082d, &dbts_0f54_082e, &dbts_0f54_082f, &dbts_058c_082f },
	{ &dbts_0fdd,      &dbts_0fdd,      &dbts_0fdd,      &dbts_0fdd      },
	{ &dbts_0fdd,      &dbts_0fdd,      &dbts_0fdd,      &dbts_0fdd_0833 },
	{ &dbts_0fdd_0837, &dbts_0fdd_0834, &dbts_0fdd_0834, &dbts_0fdd_0830 },
	{ &dbts_0fdd_0838, &dbts_0fdd_0835, &dbts_0fdd_0835, &dbts_0fdd_0831 },
	{ &dbts_0fdd_0839, &dbts_0fdd_0836, &dbts_0fdd_0836, &dbts_0fdd_0832 },
	{ &dbts_07e6,      &dbts_087d_087e, &dbts_087d_087e, &dbts_087d_087e },
	{ &dbts_087d_087e, &dbts_087d_087f, &dbts_087d_0880, &dbts_087d_0881 },
	{ &dbts_087d_0882, &dbts_087d_0883, &dbts_087d_0883, &dbts_087d_0882 },
	{ &dbts_087d_0881, &dbts_087d_0880, &dbts_087d_087f, &dbts_087d_087e },
	{ &dbts_083a_083c, &dbts_083a_083c, &dbts_083a_083c, &dbts_083a_083c },
	{ &dbts_083b_083d, &dbts_083b_083d, &dbts_083b_083d, &dbts_083b_083d },
	{ &dbts_07e6,      &dbts_083e_083f, &dbts_083e_083f, &dbts_083e_083f },
	{ &dbts_07e6,      &dbts_0840_0841, &dbts_0840_0841, &dbts_0840_0841 },
	{ &dbts_07e6,      &dbts_0842_0843, &dbts_0842_0843, &dbts_0842_0843 },
	{ &dbts_07e6,      &dbts_0844_0845, &dbts_0844_0845, &dbts_0844_0845 },
	{ &dbts_07e6_0869, &dbts_07e6_086d, &dbts_07e6_086d, &dbts_0862_0866 },
	{ &dbts_07e6_086a, &dbts_07e6_086e, &dbts_07e6_086e, &dbts_0863_0867 },
	{ &dbts_07e6_086b, &dbts_07e6_086f, &dbts_07e6_086f, &dbts_0864_0868 },
	{ &dbts_07e6_086c, &dbts_07e6_0870, &dbts_07e6_0870, &dbts_0865      },
	{ &dbts_0f54_0871, &dbts_0f54_0875, &dbts_0f54_0875, &dbts_0f54_0879 },
	{ &dbts_0f54_0872, &dbts_0f54_0876, &dbts_0f54_0876, &dbts_0f54_087a },
	{ &dbts_0f54_0873, &dbts_0f54_0877, &dbts_0f54_0877, &dbts_0f54_087b },
	{ &dbts_0f54_0874, &dbts_0f54_0878, &dbts_0f54_0878, &dbts_0f54_087c },
	{ &dbts_0f54_07ea, &dbts_0f54_07eb, &dbts_0f54_07ec, &dbts_07e6_07ec },
	{ &dbts_07e6_07ec, &dbts_07e6_07ed, &dbts_07e6_07ee, &dbts_07e6_07ee },
	{ &dbts_0f54_07ef, &dbts_0f54_07f0, &dbts_0f54_07f1, &dbts_07e6_07f1 },
	{ &dbts_0f54_07f2, &dbts_0f54_07f3, &dbts_0f54_07f4, &dbts_07e6_07f4 },
	{ &dbts_0f54_07f5, &dbts_0f54_07f6, &dbts_0f54_07f7, &dbts_07e6_07f7 },
	{ &dbts_07e6_085c, &dbts_0851_0852, &dbts_0851_0852, &dbts_0846_0847 },
	{ &dbts_07e6_085d, &dbts_0853_0854, &dbts_0853_0854, &dbts_0848_0849 },
	{ &dbts_07e6_085e, &dbts_0855_0856, &dbts_0855_0856, &dbts_084a_084b },
	{ &dbts_07e6_085f, &dbts_0857,      &dbts_0857,      &dbts_084c      },
	{ &dbts_07e6_0860, &dbts_0858_0859, &dbts_0858_0859, &dbts_084d_084e },
	{ &dbts_07e6_0861, &dbts_085a_085b, &dbts_085a_085b, &dbts_084f_0850 },
	{ &dbts_07e6_0884, &dbts_07e6_0884, &dbts_07e6_0884, &dbts_0886_0884 },
	{ &dbts_07e6_0885, &dbts_07e6_0885, &dbts_07e6_0885, &dbts_0887_0885 },
	{ &dbts_07e6_088c, &dbts_07e6_088d, &dbts_07e6_088d, &dbts_07e6_088e },
	{ &dbts_07e6_088f, &dbts_07e6_0890, &dbts_07e6_0890, &dbts_07e6_0891 },
	{ &dbts_07e6_0892, &dbts_07e6_0893, &dbts_07e6_0893, &dbts_07e6_0894 },
	{ &dbts_07e6_0895, &dbts_07e6_0896, &dbts_07e6_0896, &dbts_07e6_0897 },
	{ &dbts_07e6_0898, &dbts_07e6_0899, &dbts_07e6_0899, &dbts_07e6_089a },
	{ &dbts_07e6,      &dbts_07e6,      &dbts_07e6,      &dbts_07e6_08a6 },
	{ &dbts_07e6_089b, &dbts_07e6_089c, &dbts_07e6_089c, &dbts_07e6_089d },
	{ &dbts_07e6,      &dbts_07e6,      &dbts_07e6,      &dbts_07e6_089e },
	{ &dbts_07e6,      &dbts_07e6_089f, &dbts_07e6_08a0, &dbts_07e6_08a0 },
	{ &dbts_07e6,      &dbts_07e6_089f, &dbts_07e6_08a0, &dbts_07e6_08a1 },
	{ &dbts_07e6,      &dbts_07e6_08a2, &dbts_07e6_08a3, &dbts_07e6_08a4 },
	{ &dbts_07e6,      &dbts_07e6_08a2, &dbts_07e6_08a3, &dbts_07e6_08a5 },
	{ &dbts_08a7,      &dbts_07e6_08b7, &dbts_07e6_08b7, &dbts_07e6_08c7 },
	{ &dbts_08a8,      &dbts_08b8,      &dbts_08b8,      &dbts_08c8      },
	{ &dbts_08a9,      &dbts_07e6_08b9, &dbts_07e6_08b9, &dbts_07e6_08c9 },
	{ &dbts_08aa,      &dbts_07e6_08ba, &dbts_07e6_08ba, &dbts_07e6_08ca },
	{ &dbts_08ab,      &dbts_08bb,      &dbts_08bb,      &dbts_08cb      },
	{ &dbts_08ac,      &dbts_08bc,      &dbts_08bc,      &dbts_08cc      },
	{ &dbts_08ad,      &dbts_08bd,      &dbts_08bd,      &dbts_08cd      },
	{ &dbts_08ae,      &dbts_08be,      &dbts_08be,      &dbts_08ce_08d7 },
	{ &dbts_08af,      &dbts_08bf,      &dbts_08bf,      &dbts_08cf      },
	{ &dbts_08b0,      &dbts_08c0,      &dbts_08c0,      &dbts_08d0      },
	{ &dbts_08b1,      &dbts_08c1,      &dbts_08c1,      &dbts_08d1      },
	{ &dbts_08b2,      &dbts_08c2,      &dbts_08c2,      &dbts_08d2      },
	{ &dbts_08b3,      &dbts_08c3,      &dbts_08c3,      &dbts_08d3      },
	{ &dbts_08b4,      &dbts_08c4,      &dbts_08c4,      &dbts_08d4      },
	{ &dbts_08b5,      &dbts_08c5,      &dbts_08c5,      &dbts_08d5      },
	{ &dbts_08b6,      &dbts_08c6,      &dbts_08c6,      &dbts_08d6      },
	{ &dbts_08ce_08d7, &dbts_08ce_08d8, &dbts_08ce_08d9, &dbts_08ce_08d9 },
	{ &dbts_07e6_088a, &dbts_07e6_088a, &dbts_07e6_088a, &dbts_0888_088a },
	{ &dbts_07e6_088b, &dbts_07e6_088b, &dbts_07e6_088b, &dbts_0889_088b },
	{ &dbts_08da,      &dbts_07e6_08e3, &dbts_07e6_08e3, &dbts_07e6_08ec },
	{ &dbts_08db,      &dbts_07e6_08e4, &dbts_07e6_08e4, &dbts_07e6_08ed },
	{ &dbts_08dc,      &dbts_07e6_08e5, &dbts_07e6_08e5, &dbts_07e6_08ee },
	{ &dbts_08dd,      &dbts_07e6_08e6, &dbts_07e6_08e6, &dbts_07e6_08ef },
	{ &dbts_08de,      &dbts_08e7,      &dbts_08e7,      &dbts_08f0      },
	{ &dbts_08df,      &dbts_08e8,      &dbts_08e8,      &dbts_08f1      },
	{ &dbts_08e0,      &dbts_07e6_08e9, &dbts_07e6_08e9, &dbts_07e6_08f2 },
	{ &dbts_08e1,      &dbts_08ea,      &dbts_08ea,      &dbts_08f3      },
	{ &dbts_08e2,      &dbts_08eb,      &dbts_08eb,      &dbts_08f4      },
	{ &dbts_08f5,      &dbts_0905,      &dbts_0905,      &dbts_0915      },
	{ &dbts_08f6,      &dbts_0906,      &dbts_0906,      &dbts_0916      },
	{ &dbts_08f7,      &dbts_0907,      &dbts_0907,      &dbts_0917      },
	{ &dbts_08f8,      &dbts_0908,      &dbts_0908,      &dbts_0918      },
	{ &dbts_08f9,      &dbts_0909,      &dbts_0909,      &dbts_0919      },
	{ &dbts_08fa,      &dbts_090a,      &dbts_090a,      &dbts_091a      },
	{ &dbts_08fb,      &dbts_090b,      &dbts_090b,      &dbts_091b      },
	{ &dbts_08fc,      &dbts_090c,      &dbts_090c,      &dbts_091c      },
	{ &dbts_08fd,      &dbts_090d,      &dbts_090d,      &dbts_091d      },
	{ &dbts_08fe,      &dbts_090e,      &dbts_090e,      &dbts_091e      },
	{ &dbts_08ff,      &dbts_090f,      &dbts_090f,      &dbts_091f      },
	{ &dbts_0900,      &dbts_0910,      &dbts_0910,      &dbts_0920      },
	{ &dbts_0901,      &dbts_0911,      &dbts_0911,      &dbts_0921      },
	{ &dbts_0902,      &dbts_0912,      &dbts_0912,      &dbts_0922      },
	{ &dbts_0903,      &dbts_0913,      &dbts_0913,      &dbts_0923      },
	{ &dbts_0904,      &dbts_0914,      &dbts_0914,      &dbts_0924      },
	{ &dbts_0925,      &dbts_0925,      &dbts_0925_0926, &dbts_0925_0926 },
	{ &dbts_0925,      &dbts_0925,      &dbts_0925_0927, &dbts_0925_0927 },
	{ &dbts_11c6_092b, &dbts_11c6_092c, &dbts_11c6_092c, &dbts_11c6_092d },
	{ &dbts_11c6_092e, &dbts_11c6_092f, &dbts_11c6_092f, &dbts_11c6_0930 },
	{ &dbts_11c6_0928, &dbts_11c6_0929, &dbts_11c6_0929, &dbts_11c6_092a },
	{ &dbts_07e6_0869, &dbts_07e6_086d, &dbts_07e6_086d, &dbts_0862_0866 },
	{ &dbts_07e6_086a, &dbts_07e6_086e, &dbts_07e6_086e, &dbts_0863_0867 },
	{ &dbts_07e6_086b, &dbts_07e6_086f, &dbts_07e6_086f, &dbts_0864_0868 },
	{ &dbts_07e6_086c, &dbts_07e6_0870, &dbts_07e6_0870, &dbts_0865      },
	{ &dbts_07e6_0931, &dbts_07e6_0935, &dbts_07e6_0935, &dbts_07e6_0939 },
	{ &dbts_07e6_0932, &dbts_07e6_0936, &dbts_07e6_0936, &dbts_07e6_093a },
	{ &dbts_07e6_0933, &dbts_07e6_0937, &dbts_07e6_0937, &dbts_07e6_093b },
	{ &dbts_07e6_0934, &dbts_07e6_0938, &dbts_07e6_0938, &dbts_07e6_093c },
	{ &dbts_081d_0818, &dbts_081d_0819, &dbts_081d_081a, &dbts_081d_081b },
	{ &dbts_081d_081c, &dbts_081d_081c, &dbts_081d_081c, &dbts_081d_081c },
	{ &dbts_07e6,      &dbts_07e6,      &dbts_07e6,      &dbts_07e6      },
	{ &dbts_07e6_1245, &dbts_07e6_1248, &dbts_07e6_1248, &dbts_07e6_124b },
	{ &dbts_07e6_1247, &dbts_07e6_124a, &dbts_07e6_124a, &dbts_07e6_124d },
	{ &dbts_07e6_1246, &dbts_07e6_1249, &dbts_07e6_1249, &dbts_07e6_124c },
	{ &dbts_081d_124e, &dbts_081d_124f, &dbts_081d_1250, &dbts_081d_1251 },
	{ &dbts_081d_1252, &dbts_081d_1252, &dbts_081d_1252, &dbts_081d_1252 },
	{ &dbts_081d_1253, &dbts_081d_1254, &dbts_081d_1254, &dbts_081d_1255 },
	{ &dbts_07e6,      &dbts_07e6,      &dbts_07e6,      &dbts_07e6      },
	{ &dbts_07e6_125b, &dbts_07e6_125e, &dbts_07e6_125e, &dbts_07e6_1261 },
	{ &dbts_07e6_125c, &dbts_07e6_125f, &dbts_07e6_125f, &dbts_07e6_1262 },
	{ &dbts_07e6_125d, &dbts_07e6_1260, &dbts_07e6_1260, &dbts_07e6_1263 },
	{ &dbts_1243,      &dbts_1243_1264, &dbts_1243_1264, &dbts_1243_1268 },
	{ &dbts_1243,      &dbts_1243_1265, &dbts_1243_1265, &dbts_1243_1269 },
	{ &dbts_1243,      &dbts_1243_1266, &dbts_1243_1266, &dbts_1243_126a },
	{ &dbts_1243,      &dbts_1243_1267, &dbts_1243_1267, &dbts_1243_126b },
	{ &dbts_1243,      &dbts_1243,      &dbts_1243,      &dbts_1243_126c },
	{ &dbts_1243,      &dbts_1243,      &dbts_1243,      &dbts_1243      },
	{ &dbts_1271,      &dbts_1271,      &dbts_1271,      &dbts_1271_1279 },
	{ &dbts_1272,      &dbts_1272,      &dbts_1272,      &dbts_1272_127a },
	{ &dbts_1273,      &dbts_1273,      &dbts_1273,      &dbts_1273_127b },
	{ &dbts_1274,      &dbts_1274,      &dbts_1274,      &dbts_1274_127c },
	{ &dbts_1275,      &dbts_1275,      &dbts_1275,      &dbts_1275_127d },
	{ &dbts_1276,      &dbts_1276,      &dbts_1276,      &dbts_1276_127e },
	{ &dbts_1277,      &dbts_1277,      &dbts_1277,      &dbts_1277_127f },
	{ &dbts_1278,      &dbts_1278,      &dbts_1278,      &dbts_1278_1280 },
	{ &dbts_1244,      &dbts_1244,      &dbts_1244,      &dbts_1244      },
	{ &dbts_1244,      &dbts_1244,      &dbts_1244,      &dbts_1244_1284 },
	{ &dbts_1244,      &dbts_1244_1283, &dbts_1244_1283, &dbts_1244_1286 },
	{ &dbts_1244_1281, &dbts_1244_1282, &dbts_1244_1282, &dbts_1244_1285 },
	{ &dbts_1243,      &dbts_1243_1287, &dbts_1243_1287, &dbts_1243_1287 },
	{ &dbts_1244,      &dbts_1244_1288, &dbts_1244_1288, &dbts_1244_1288 },
	{ &dbts_1243,      &dbts_1243_1289, &dbts_1243_1289, &dbts_1243_1289 },
	{ &dbts_1244,      &dbts_1244,      &dbts_1244,      &dbts_1244      },
	{ &dbts_0f8d_129b, &dbts_0f8d_129b, &dbts_0f8d_129b, &dbts_0f8d_129b },
	{ &dbts_0f8d_129c, &dbts_0f8d_129c, &dbts_0f8d_129c, &dbts_0f8d_129c },
	{ &dbts_0f8d_129d, &dbts_0f8d_129d, &dbts_0f8d_129d, &dbts_0f8d_129d },
	{ &dbts_12a0,      &dbts_12a0,      &dbts_12a0,      &dbts_12a0      },
	{ &dbts_12a1,      &dbts_12a1,      &dbts_12a1,      &dbts_12a1      },
	{ &dbts_12a2,      &dbts_12a2,      &dbts_12a2,      &dbts_12a2      },
	{ &dbts_12a3,      &dbts_12a3,      &dbts_12a3,      &dbts_12a3      },
	{ &dbts_0f8d,      &dbts_0f8d,      &dbts_0f8d,      &dbts_0f8d      },
	{ &dbts_0f8d_12a4, &dbts_0f8d_12a4, &dbts_0f8d_12a4, &dbts_0f8d_12a4 },
	{ &dbts_0f8d_12a6, &dbts_0f8d_12a6, &dbts_0f8d_12a6, &dbts_0f8d_12a6 },
	{ &dbts_0f8d_12a5, &dbts_0f8d_12a5, &dbts_0f8d_12a5, &dbts_0f8d_12a5 },
};

/* this is ONLY used for Sugar Mine*/
static const DrawIndustryAnimationStruct _draw_industry_spec1[96] = {
	{  8,   4,   0,   0},
	{  6,   0,   1,   0},
	{  4,   0,   2,   0},
	{  6,   0,   3,   0},
	{  8,   0,   4,   0},
	{ 10,   0,   5,   0},
	{ 12,   0,   6,   0},
	{ 10,   0,   1,   0},
	{  8,   0,   2,   0},
	{  6,   0,   3,   0},
	{  4,   0,   4,   0},
	{  6,   0,   5,   1},
	{  8,   0,   6,   1},
	{ 10,   0,   1,   1},
	{ 12,   0,   2,   1},
	{ 10,   0,   3,   1},
	{  8,   1,   4,   1},
	{  6,   1,   5,   1},
	{  4,   1,   6,   1},
	{  6,   1,   1,   1},
	{  8,   1,   2,   1},
	{ 10,   1,   3,   1},
	{ 12,   1,   4,   1},
	{ 10,   1,   5,   2},
	{  8,   1,   6,   2},
	{  6,   1,   1,   2},
	{  4,   1,   2,   2},
	{  6,   1,   3,   2},
	{  8,   1,   4,   2},
	{ 10,   1,   5,   2},
	{ 12,   1,   6,   2},
	{ 10,   1,   1,   2},
	{  8,   2,   2,   2},
	{  6,   2,   3,   2},
	{  4,   2,   4,   3},
	{  6,   2,   5,   3},
	{  8,   2,   6,   3},
	{ 10,   2,   1,   3},
	{ 12,   2,   2,   3},
	{ 10,   2,   3,   3},
	{  8,   2,   4,   3},
	{  6,   2,   5,   3},
	{  4,   2,   6,   3},
	{  6,   2,   1,   3},
	{  8,   2,   2,   3},
	{ 10,   2,   3,   4},
	{ 12,   2,   4,   4},
	{ 10,   2,   5,   4},
	{  8,   3,   6,   4},
	{  6,   3,   1,   4},
	{  4,   3,   2,   4},
	{  6,   3,   3,   4},
	{  8,   3,   4,   4},
	{ 10,   3,   5,   4},
	{ 12,   3,   6,   4},
	{ 10,   3,   1,   4},
	{  8,   3,   2,   4},
	{  6,   3,   3,   4},
	{  4,   3,   4,   4},
	{  6,   3,   5,   4},
	{  8,   3,   6,   4},
	{ 10,   3,   1,   4},
	{ 12,   3,   2,   4},
	{ 10,   3,   3,   4},
	{  8,   4,   4,   4},
	{  6,   4,   5,   4},
	{  4,   4,   6,   4},
	{  6,   4,   0,   4},
	{  8,   4,   0,   4},
	{ 10,   4,   0,   4},
	{ 12,   4,   0,   4},
	{ 10,   4,   0,   4},
	{  8,   4,   0,   4},
	{  6,   4,   0,   4},
	{  4,   4,   0,   4},
	{  6,   4,   0,   4},
	{  8,   4,   0,   4},
	{ 10,   4,   0,   4},
	{ 12,   4,   0,   4},
	{ 10,   4,   0,   4},
	{  8,   4,   0,   4},
	{  6,   4,   0,   4},
	{  4,   4,   0,   4},
	{  6,   4,   0,   4},
	{  8,   4,   0,   4},
	{ 10,   4,   0,   4},
	{ 12,   4,   0,   4},
	{ 10,   4,   0,   4},
	{  8,   4,   0,   4},
	{  6,   4,   0,   4},
	{  4,   4,   0,   4},
	{  6,   4,   0,   4},
	{  8,   4,   0,   4},
	{ 10,   4,   0,   4},
	{ 12,   4,   0,   4},
	{ 10,   4,   0,   4},
};

/* this is ONLY used for Sugar Mine*/
static const DrawIndustryCoordinates _drawtile_proc1[5] = {
	{22, 73},
	{17, 70},
	{14, 69},
	{10, 66},
	{ 8, 41},
};
