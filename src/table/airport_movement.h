/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_movement.h Heart of the airports and their finite state machines */

#ifndef AIRPORT_MOVEMENT_H
#define AIRPORT_MOVEMENT_H


///////////////////////////////////////////////////////////////////////
/////**********Movement Machine on Airports*********************///////
static const byte _airport_entries_dummy[] = {0, 1, 2, 3};
static const AirportFTAClass::Position _airport_fta_dummy[] = {
	{ 0, 0, 3,   0,  0, DIR_N | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL },
	{ 0, 0, 0,   0, 96, DIR_N | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL },
	{ 0, 0, 1,  96, 96, DIR_N | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL },
	{ 0, 0, 2,  96,  0, DIR_N | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL },
};

/* First element of terminals array tells us how many depots there are (to know size of array)
 * this may be changed later when airports are moved to external file  */
static const HangarTileTable _airport_depots_country[] = { {{3, 0}, DIR_SE, 0} };
static const byte _airport_terminal_country[] = { 1, 0, 2 };
static const byte _airport_entries_country[] = {16, 15, 18, 17};

static const AirportFTAClass::Transition _airport_fta_country_1[] = {
	{ 0,           HANGAR,       0, false },
	{ TERM1_block, TERM1,        2, false },
	{ 0,           TERM2,        4, false },
	{ 0,           HELITAKEOFF, 19, false },
	{ 0,           0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_country_4[] = {
	{ 0,           TERM2,        5, false },
	{ 0,           HANGAR,       1, false },
	{ 0,           TAKEOFF,      6, false },
	{ 0,           HELITAKEOFF,  1, true  }
};
static const AirportFTAClass::Transition _airport_fta_country_5[] = {
	{ TERM2_block, TERM2,        3, false },
	{ 0,           0,            4, true  }
};
static const AirportFTAClass::Transition _airport_fta_country_10[] = {
	{ 0,           LANDING,     11, false },
	{ 0,           HELILANDING, 20, true  }
};
static const AirportFTAClass::Transition _airport_fta_country_13[] = {
	{ 0,           TERM2,        5, false },
	{ 0,           0,           14, true  }
};
static const AirportFTAClass::Position _airport_fta_country[22] = {
	{ NOTHING_block,      HANGAR,          1,   53,   3, DIR_SE | AMED_EXACTPOS,                   NULL                    }, // 00 In Hangar
	{ AIRPORT_BUSY_block, 255,             0,   53,  27, DIR_N,                                    _airport_fta_country_1  }, // 01 Taxi to right outside depot
	{ TERM1_block,        TERM1,           1,   32,  23, DIR_NW | AMED_EXACTPOS,                   NULL                    }, // 02 Terminal 1
	{ TERM2_block,        TERM2,           5,   10,  23, DIR_NW | AMED_EXACTPOS,                   NULL                    }, // 03 Terminal 2
	{ AIRPORT_BUSY_block, 255,             0,   43,  37, DIR_N,                                    _airport_fta_country_4  }, // 04 Going towards terminal 2
	{ AIRPORT_BUSY_block, 255,             0,   24,  37, DIR_N,                                    _airport_fta_country_5  }, // 05 Going towards terminal 2
	{ AIRPORT_BUSY_block, 0,               7,   53,  37, DIR_N,                                    NULL                    }, // 06 Going for takeoff
	/* takeoff */
	{ AIRPORT_BUSY_block, TAKEOFF,         8,   61,  40, DIR_NE | AMED_EXACTPOS,                   NULL                    }, // 07 Taxi to start of runway (takeoff)
	{ NOTHING_block,      STARTTAKEOFF,    9,    3,  40, DIR_N  | AMED_NOSPDCLAMP,                 NULL                    }, // 08 Accelerate to end of runway
	{ NOTHING_block,      ENDTAKEOFF,      0,  -79,  40, DIR_N  | AMED_NOSPDCLAMP | AMED_TAKEOFF,  NULL                    }, // 09 Take off
	/* landing */
	{ NOTHING_block,      FLYING,         15,  177,  40, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_country_10 }, // 10 Fly to landing position in air
	{ AIRPORT_BUSY_block, LANDING,        12,   56,  40, DIR_N  | AMED_NOSPDCLAMP | AMED_LAND,     NULL                    }, // 11 Going down for land
	{ AIRPORT_BUSY_block, 0,              13,    3,  40, DIR_N  | AMED_NOSPDCLAMP | AMED_BRAKE,    NULL                    }, // 12 Just landed, brake until end of runway
	{ AIRPORT_BUSY_block, ENDLANDING,     14,    7,  40, DIR_N,                                    _airport_fta_country_13 }, // 13 Just landed, turn around and taxi 1 square
	{ AIRPORT_BUSY_block, 0,               1,   53,  40, DIR_N,                                    NULL                    }, // 14 Taxi from runway to crossing
	/* flying */
	{ NOTHING_block,      0,              16,    1, 193, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 15 Fly around waiting for a landing spot (north-east)
	{ NOTHING_block,      0,              17,    1,   1, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 16 Fly around waiting for a landing spot (north-west)
	{ NOTHING_block,      0,              18,  257,   1, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 17 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,      0,              10,  273,  47, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 18 Fly around waiting for a landing spot (south)
	{ NOTHING_block,      HELITAKEOFF,     0,   44,  37, DIR_N  | AMED_HELI_RAISE,                 NULL                    }, // 19 Helicopter takeoff
	{ AIRPORT_BUSY_block, HELILANDING,    21,   44,  40, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 20 In position above landing spot helicopter
	{ AIRPORT_BUSY_block, HELIENDLANDING,  1,   44,  40, DIR_N  | AMED_HELI_LOWER,                 NULL                    }, // 21 Helicopter landing
};

static const HangarTileTable _airport_depots_commuter[] = { {{4, 0}, DIR_SE, 0} };
static const byte _airport_terminal_commuter[] = { 1, 0, 3 };
static const byte _airport_entries_commuter[] = {22, 21, 24, 23};

static const AirportFTAClass::Transition _airport_fta_commuter_0[] = {
	{ HELIPAD2_block,     HELITAKEOFF,  1, false },
	{ 0,                  0,            1, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_1[] = {
	{ 0,                  HANGAR,       0, false },
	{ 0,                  TAKEOFF,     11, false },
	{ TAXIWAY_BUSY_block, TERM1,       10, false },
	{ TAXIWAY_BUSY_block, TERM2,       10, false },
	{ TAXIWAY_BUSY_block, TERM3,       10, false },
	{ TAXIWAY_BUSY_block, HELIPAD1,    10, false },
	{ TAXIWAY_BUSY_block, HELIPAD2,    10, false },
	{ TAXIWAY_BUSY_block, HELITAKEOFF, 10, false },
	{ 0,                  0,            0, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_2[] = {
	{ 0,                  HANGAR,       8, false },
	{ 0,                  TERM1,        8, false },
	{ 0,                  TERM2,        8, false },
	{ 0,                  TERM3,        8, false },
	{ 0,                  HELIPAD1,     8, false },
	{ 0,                  HELIPAD2,     8, false },
	{ 0,                  HELITAKEOFF,  8, false },
	{ 0,                  0,            2, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_3[] = {
	{ 0,                  HANGAR,       8, false },
	{ 0,                  TAKEOFF,      8, false },
	{ 0,                  0,            3, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_4[] = {
	{ 0,                  HANGAR,       9, false },
	{ 0,                  TAKEOFF,      9, false },
	{ 0,                  0,            4, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_5[] = {
	{ 0,                  HANGAR,      10, false },
	{ 0,                  TAKEOFF,     10, false },
	{ 0,                  0,            5, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_6[] = {
	{ TAXIWAY_BUSY_block, HANGAR,       9, false },
	{ 0,                  HELITAKEOFF, 35, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_7[] = {
	{ TAXIWAY_BUSY_block, HANGAR,      10, false },
	{ 0,                  HELITAKEOFF, 36, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_8[] = {
	{ TAXIWAY_BUSY_block, TAKEOFF,      9, false },
	{ TAXIWAY_BUSY_block, HANGAR,       9, false },
	{ TERM1_block,        TERM1,        3, false },
	{ TAXIWAY_BUSY_block, 0,            9, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_9[] = {
	{ TAXIWAY_BUSY_block, TAKEOFF,     10, false },
	{ TAXIWAY_BUSY_block, HANGAR,      10, false },
	{ TERM2_block,        TERM2,        4, false },
	{ HELIPAD1_block,     HELIPAD1,     6, false },
	{ HELIPAD1_block,     HELITAKEOFF,  6, false },
	{ TAXIWAY_BUSY_block, TERM1,        8, false },
	{ TAXIWAY_BUSY_block, 0,           10, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_10[] = {
	{ TERM3_block,        TERM3,        5, false },
	{ 0,                  HELIPAD1,     9, false },
	{ HELIPAD2_block,     HELIPAD2,     7, false },
	{ HELIPAD2_block,     HELITAKEOFF,  7, false },
	{ TAXIWAY_BUSY_block, TAKEOFF,      1, false },
	{ TAXIWAY_BUSY_block, HANGAR,       1, false },
	{ TAXIWAY_BUSY_block, 0,            9, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_16[] = {
	{ IN_WAY_block,       LANDING,     17, false },
	{ 0,                  HELILANDING, 25, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_26[] = {
	{ 0,                  HELIPAD1,    27, false },
	{ 0,                  HELIPAD2,    28, false },
	{ 0,                  HANGAR,      33, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_29[] = {
	{ HELIPAD1_block,     HELIPAD1,     6, true  }
};
static const AirportFTAClass::Transition _airport_fta_commuter_30[] = {
	{ HELIPAD2_block,     HELIPAD2,     7, true  }
};
static const AirportFTAClass::Position _airport_fta_commuter[37] = {
	{ NOTHING_block,          HANGAR,          1,   69,   3, DIR_SE | AMED_EXACTPOS,                   _airport_fta_commuter_0  }, // 00 In Hangar
	{ TAXIWAY_BUSY_block,     255,             0,   72,  22, DIR_N,                                    _airport_fta_commuter_1  }, // 01 Taxi to right outside depot
	{ AIRPORT_ENTRANCE_block, 255,             2,    8,  22, DIR_SW | AMED_EXACTPOS,                   _airport_fta_commuter_2  }, // 01 Taxi to right outside depot
	{ TERM1_block,            TERM1,           8,   24,  36, DIR_SE | AMED_EXACTPOS,                   _airport_fta_commuter_3  }, // 03 Terminal 1
	{ TERM2_block,            TERM2,           9,   40,  36, DIR_SE | AMED_EXACTPOS,                   _airport_fta_commuter_4  }, // 04 Terminal 2
	{ TERM3_block,            TERM3,          10,   56,  36, DIR_SE | AMED_EXACTPOS,                   _airport_fta_commuter_5  }, // 05 Terminal 3
	{ HELIPAD1_block,         HELIPAD1,        6,   40,   8, DIR_NE | AMED_EXACTPOS,                   _airport_fta_commuter_6  }, // 06 Helipad 1
	{ HELIPAD2_block,         HELIPAD2,        7,   56,   8, DIR_NE | AMED_EXACTPOS,                   _airport_fta_commuter_7  }, // 07 Helipad 2
	{ TAXIWAY_BUSY_block,     255,             8,   24,  22, DIR_SW,                                   _airport_fta_commuter_8  }, // 08 Taxiing
	{ TAXIWAY_BUSY_block,     255,             9,   40,  22, DIR_SW,                                   _airport_fta_commuter_9  }, // 09 Taxiing
	{ TAXIWAY_BUSY_block,     255,            10,   56,  22, DIR_SW,                                   _airport_fta_commuter_10 }, // 10 Taxiing
	{ OUT_WAY_block,          0,              12,   72,  40, DIR_SE,                                   NULL                     }, // 11 Airport OUTWAY
	/* takeoff */
	{ RUNWAY_IN_OUT_block,    TAKEOFF,        13,   72,  54, DIR_NE | AMED_EXACTPOS,                   NULL                     }, // 12 Accelerate to end of runway
	{ RUNWAY_IN_OUT_block,    0,              14,    7,  54, DIR_N  | AMED_NOSPDCLAMP,                 NULL                     }, // 13 Release control of runway, for smoother movement
	{ RUNWAY_IN_OUT_block,    STARTTAKEOFF,   15,    5,  54, DIR_N  | AMED_NOSPDCLAMP,                 NULL                     }, // 14 End of runway
	{ NOTHING_block,          ENDTAKEOFF,      0,  -79,  54, DIR_N  | AMED_NOSPDCLAMP | AMED_TAKEOFF,  NULL                     }, // 15 Take off
	/* landing */
	{ NOTHING_block,          FLYING,         21,  145,  54, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_commuter_16 }, // 16 Fly to landing position in air
	{ RUNWAY_IN_OUT_block,    LANDING,        18,   73,  54, DIR_N  | AMED_NOSPDCLAMP | AMED_LAND,     NULL                     }, // 17 Going down for land
	{ RUNWAY_IN_OUT_block,    0,              19,    3,  54, DIR_N  | AMED_NOSPDCLAMP | AMED_BRAKE,    NULL                     }, // 18 Just landed, brake until end of runway
	{ RUNWAY_IN_OUT_block,    0,              20,   12,  54, DIR_NW | AMED_SLOWTURN,                   NULL                     }, // 19 Just landed, turn around and taxi
	{ IN_WAY_block,           ENDLANDING,      2,    8,  32, DIR_NW,                                   NULL                     }, // 20 Taxi from runway to crossing
	/* flying */
	{ NOTHING_block,          0,              22,    1, 149, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 21 Fly around waiting for a landing spot (north-east)
	{ NOTHING_block,          0,              23,    1,   6, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 22 Fly around waiting for a landing spot (north-west)
	{ NOTHING_block,          0,              24,  193,   6, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 23 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,          0,              16,  225,  62, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 24 Fly around waiting for a landing spot (south)
	/* helicopter -- stay in air in special place as a buffer to choose from helipads */
	{ PRE_HELIPAD_block,      HELILANDING,    26,   80,   0, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 25 Bufferspace before helipad
	{ PRE_HELIPAD_block,      HELIENDLANDING, 26,   80,   0, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_commuter_26 }, // 26 Bufferspace before helipad
	{ NOTHING_block,          0,              29,   32,   8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 27 Get in position for Helipad1
	{ NOTHING_block,          0,              30,   48,   8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 28 Get in position for Helipad2
	/* landing */
	{ NOTHING_block,          255,             0,   32,   8, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_commuter_29 }, // 29 Land at Helipad1
	{ NOTHING_block,          255,             0,   48,   8, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_commuter_30 }, // 30 Land at Helipad2
	/* helicopter takeoff */
	{ NOTHING_block,          HELITAKEOFF,     0,   32,   8, DIR_N  | AMED_HELI_RAISE,                 NULL                     }, // 31 Takeoff Helipad1
	{ NOTHING_block,          HELITAKEOFF,     0,   48,   8, DIR_N  | AMED_HELI_RAISE,                 NULL                     }, // 32 Takeoff Helipad2
	{ TAXIWAY_BUSY_block,     0,              34,   64,  22, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                     }, // 33 Go to position for Hangarentrance in air
	{ TAXIWAY_BUSY_block,     0,               1,   64,  22, DIR_N  | AMED_HELI_LOWER,                 NULL                     }, // 34 Land in front of hangar
	{ HELIPAD1_block,         0,              31,   40,   8, DIR_N  | AMED_EXACTPOS,                   NULL                     }, // 35 pre-helitakeoff helipad 1
	{ HELIPAD2_block,         0,              32,   56,   8, DIR_N  | AMED_EXACTPOS,                   NULL                     }, // 36 pre-helitakeoff helipad 2
};

static const HangarTileTable _airport_depots_city[] = { {{5, 0}, DIR_SE, 0} };
static const byte _airport_terminal_city[] = { 1, 0, 3 };
static const byte _airport_entries_city[] = {26, 29, 27, 28};

static const AirportFTAClass::Transition _airport_fta_city_0[] = {
	{ OUT_WAY_block, TAKEOFF,      1, false },
	{ 0,             0,            1, true  }
};
static const AirportFTAClass::Transition _airport_fta_city_1[] = {
	{ 0,             HANGAR,       0, false },
	{ 0,             TERM2,        6, false },
	{ 0,             TERM3,        6, false },
	{ 0,             0,            7, true  } // for all else, go to 7
};
static const AirportFTAClass::Transition _airport_fta_city_2[] = {
	{ OUT_WAY_block, TAKEOFF,      7, false },
	{ 0,             0,            7, true  }
};
static const AirportFTAClass::Transition _airport_fta_city_3[] = {
	{ OUT_WAY_block, TAKEOFF,      6, false },
	{ 0,             0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_city_4[] = {
	{ OUT_WAY_block, TAKEOFF,      5, false },
	{ 0,             0,            5, true  }
};
static const AirportFTAClass::Transition _airport_fta_city_5[] = {
	{ TERM2_block,   TERM2,        3, false },
	{ TERM3_block,   TERM3,        4, false },
	{ 0,             0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_city_6[] = {
	{ TERM2_block,   TERM2,        3, false },
	{ 0,             TERM3,        5, false },
	{ 0,             HANGAR,       1, false },
	{ 0,             0,            7, true  }
};
static const AirportFTAClass::Transition _airport_fta_city_7[] = {
	{ TERM1_block,   TERM1,        2, false },
	{ OUT_WAY_block, TAKEOFF,      8, false },
	{ 0,             HELITAKEOFF, 22, false },
	{ 0,             HANGAR,       1, false },
	{ 0,             0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_city_13[] = {
	{ 0,             LANDING,     14, false },
	{ 0,             HELILANDING, 23, true  }
};
static const AirportFTAClass::Position _airport_fta_city[] = {
	{ NOTHING_block,       HANGAR,          1,   85,   3, DIR_SE | AMED_EXACTPOS,                   _airport_fta_city_0  }, // 00 In Hangar
	{ TAXIWAY_BUSY_block,  255,             0,   85,  22, DIR_N,                                    _airport_fta_city_1  }, // 01 Taxi to right outside depot
	{ TERM1_block,         TERM1,           7,   26,  41, DIR_SW | AMED_EXACTPOS,                   _airport_fta_city_2  }, // 02 Terminal 1
	{ TERM2_block,         TERM2,           5,   56,  22, DIR_SE | AMED_EXACTPOS,                   _airport_fta_city_3  }, // 03 Terminal 2
	{ TERM3_block,         TERM3,           5,   38,   8, DIR_SW | AMED_EXACTPOS,                   _airport_fta_city_4  }, // 04 Terminal 3
	{ TAXIWAY_BUSY_block,  255,             0,   65,   6, DIR_N,                                    _airport_fta_city_5  }, // 05 Taxi to right in infront of terminal 2/3
	{ TAXIWAY_BUSY_block,  255,             0,   80,  27, DIR_N,                                    _airport_fta_city_6  }, // 06 Taxiway terminals 2-3
	{ TAXIWAY_BUSY_block,  255,             0,   44,  63, DIR_N,                                    _airport_fta_city_7  }, // 07 Taxi to Airport center
	{ OUT_WAY_block,       0,               9,   58,  71, DIR_N,                                    NULL                 }, // 08 Towards takeoff
	{ RUNWAY_IN_OUT_block, 0,              10,   72,  85, DIR_N,                                    NULL                 }, // 09 Taxi to runway (takeoff)
	/* takeoff */
	{ RUNWAY_IN_OUT_block, TAKEOFF,        11,   89,  85, DIR_NE | AMED_EXACTPOS,                   NULL                 }, // 10 Taxi to start of runway (takeoff)
	{ NOTHING_block,       STARTTAKEOFF,   12,    3,  85, DIR_N  | AMED_NOSPDCLAMP,                 NULL                 }, // 11 Accelerate to end of runway
	{ NOTHING_block,       ENDTAKEOFF,      0,  -79,  85, DIR_N  | AMED_NOSPDCLAMP | AMED_TAKEOFF,  NULL                 }, // 12 Take off
	/* landing */
	{ NOTHING_block,       FLYING,         18,  177,  87, DIR_N  | AMED_HOLD       | AMED_SLOWTURN, _airport_fta_city_13 }, // 13 Fly to landing position in air
	{ RUNWAY_IN_OUT_block, LANDING,        15,   89,  87, DIR_N  | AMED_HOLD       | AMED_LAND,     NULL                 }, // 14 Going down for land
	{ RUNWAY_IN_OUT_block, 0,              17,   20,  87, DIR_N  | AMED_NOSPDCLAMP | AMED_BRAKE,    NULL                 }, // 15 Just landed, brake until end of runway
	{ RUNWAY_IN_OUT_block, 0,              17,   20,  87, DIR_N,                                    NULL                 }, // 16 Just landed, turn around and taxi 1 square // not used, left for compatibility
	{ IN_WAY_block,        ENDLANDING,      7,   36,  71, DIR_N,                                    NULL                 }, // 17 Taxi from runway to crossing
	/* flying */
	{ NOTHING_block,       0,              25,  160,  87, DIR_N  | AMED_HOLD       | AMED_SLOWTURN, NULL                 }, // 18 Fly around waiting for a landing spot (north-east)
	{ NOTHING_block,       0,              20,  140,   1, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                 }, // 19 Final approach fix
	{ NOTHING_block,       0,              21,  257,   1, DIR_N  | AMED_HOLD       | AMED_SLOWTURN, NULL                 }, // 20 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,       0,              13,  273,  49, DIR_N  | AMED_HOLD       | AMED_SLOWTURN, NULL                 }, // 21 Fly around waiting for a landing spot (south)
	/* helicopter */
	{ NOTHING_block,       HELITAKEOFF,     0,   44,  63, DIR_N  | AMED_HELI_RAISE,                 NULL                 }, // 22 Helicopter takeoff
	{ IN_WAY_block,        HELILANDING,    24,   28,  74, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                 }, // 23 In position above landing spot helicopter
	{ IN_WAY_block,        HELIENDLANDING, 17,   28,  74, DIR_N  | AMED_HELI_LOWER,                 NULL                 }, // 24 Helicopter landing
	{ NOTHING_block,       0,              20,  145,   1, DIR_N  | AMED_HOLD       | AMED_SLOWTURN, NULL                 }, // 25 Fly around waiting for a landing spot (north-west)
	{ NOTHING_block,       0,              19,  -32,   1, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                 }, // 26 Initial approach fix (north)
	{ NOTHING_block,       0,              28,  300, -48, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                 }, // 27 Initial approach fix (south)
	{ NOTHING_block,       0,              19,  140, -48, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                 }, // 28 Intermediate Approach fix (south), IAF (west)
	{ NOTHING_block,       0,              26,  -32, 120, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                 }, // 29 Initial approach fix (east)
};

static const HangarTileTable _airport_depots_metropolitan[] = { {{5, 0}, DIR_SE, 0} };
static const byte _airport_terminal_metropolitan[] = { 1, 0, 3 };
static const byte _airport_entries_metropolitan[] = {20, 19, 22, 21};

static const AirportFTAClass::Transition _airport_fta_metropolitan_1[] = {
	{ 0,            HANGAR,       0, false },
	{ 0,            TERM2,        6, false },
	{ 0,            TERM3,        6, false },
	{ 0,            0,            7, true  } // for all else, go to 7
};
static const AirportFTAClass::Transition _airport_fta_metropolitan_5[] = {
	{ TERM2_block,  TERM2,        3, false },
	{ TERM3_block,  TERM3,        4, false },
	{ 0,            0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_metropolitan_6[] = {
	{ TERM2_block,  TERM2,        3, false },
	{ 0,            TERM3,        5, false },
	{ 0,            HANGAR,       1, false },
	{ 0,            0,            7, true  }
};
static const AirportFTAClass::Transition _airport_fta_metropolitan_7[] = {
	{ TERM1_block,  TERM1,        2, false },
	{ 0,            TAKEOFF,      8, false },
	{ 0,            HELITAKEOFF, 23, false },
	{ 0,            HANGAR,       1, false },
	{ 0,            0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_metropolitan_13[] = {
	{ 0,            LANDING,     14, false },
	{ 0,            HELILANDING, 25, true  }
};
static const AirportFTAClass::Transition _airport_fta_metropolitan_16[] = {
	{ IN_WAY_block, ENDLANDING,  17, true  }
};
static const AirportFTAClass::Transition _airport_fta_metropolitan_17[] = {
	{ IN_WAY_block, ENDLANDING,  18, true  }
};
static const AirportFTAClass::Transition _airport_fta_metropolitan_27[] = {
	{ TERM1_block,  TERM1,        2, false },
	{ 0,            0,            7, true  }
};
static const AirportFTAClass::Position _airport_fta_metropolitan[28] = {
	{ NOTHING_block,      HANGAR,          1,   85,   3, DIR_SE | AMED_EXACTPOS,                   NULL                         }, // 00 In Hangar
	{ TAXIWAY_BUSY_block, 255,             0,   85,  22, DIR_N,                                    _airport_fta_metropolitan_1  }, // 01 Taxi to right outside depot
	{ TERM1_block,        TERM1,           7,   26,  41, DIR_SW | AMED_EXACTPOS,                   NULL                         }, // 02 Terminal 1
	{ TERM2_block,        TERM2,           6,   56,  22, DIR_SE | AMED_EXACTPOS,                   NULL                         }, // 03 Terminal 2
	{ TERM3_block,        TERM3,           5,   38,   8, DIR_SW | AMED_EXACTPOS,                   NULL                         }, // 04 Terminal 3
	{ TAXIWAY_BUSY_block, 255,             0,   65,   6, DIR_N,                                    _airport_fta_metropolitan_5  }, // 05 Taxi to right in infront of terminal 2/3
	{ TAXIWAY_BUSY_block, 255,             0,   80,  27, DIR_N,                                    _airport_fta_metropolitan_6  }, // 06 Taxiway terminals 2-3
	{ TAXIWAY_BUSY_block, 255,             0,   49,  58, DIR_N,                                    _airport_fta_metropolitan_7  }, // 07 Taxi to Airport center
	{ OUT_WAY_block,      0,               9,   72,  58, DIR_N,                                    NULL                         }, // 08 Towards takeoff
	{ RUNWAY_OUT_block,   0,              10,   72,  69, DIR_N,                                    NULL                         }, // 09 Taxi to runway (takeoff)
	/* takeoff */
	{ RUNWAY_OUT_block,   TAKEOFF,        11,   89,  69, DIR_NE | AMED_EXACTPOS,                   NULL                         }, // 10 Taxi to start of runway (takeoff)
	{ NOTHING_block,      STARTTAKEOFF,   12,    3,  69, DIR_N  | AMED_NOSPDCLAMP,                 NULL                         }, // 11 Accelerate to end of runway
	{ NOTHING_block,      ENDTAKEOFF,      0,  -79,  69, DIR_N  | AMED_NOSPDCLAMP | AMED_TAKEOFF,  NULL                         }, // 12 Take off
	/* landing */
	{ NOTHING_block,      FLYING,         19,  177,  85, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_metropolitan_13 }, // 13 Fly to landing position in air
	{ RUNWAY_IN_block,    LANDING,        15,   89,  85, DIR_N  | AMED_NOSPDCLAMP | AMED_LAND,     NULL                         }, // 14 Going down for land
	{ RUNWAY_IN_block,    0,              16,    3,  85, DIR_N  | AMED_NOSPDCLAMP | AMED_BRAKE,    NULL                         }, // 15 Just landed, brake until end of runway
	{ RUNWAY_IN_block,    255,             0,   21,  85, DIR_N,                                    _airport_fta_metropolitan_16 }, // 16 Just landed, turn around and taxi 1 square
	{ RUNWAY_OUT_block,   255,             0,   21,  69, DIR_N,                                    _airport_fta_metropolitan_17 }, // 17 On Runway-out taxiing to In-Way
	{ IN_WAY_block,       ENDLANDING,     27,   21,  58, DIR_SW | AMED_EXACTPOS,                   NULL                         }, // 18 Taxi from runway to crossing
	/* flying */
	{ NOTHING_block,      0,              20,    1, 193, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                         }, // 19 Fly around waiting for a landing spot (north-east)
	{ NOTHING_block,      0,              21,    1,   1, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                         }, // 20 Fly around waiting for a landing spot (north-west)
	{ NOTHING_block,      0,              22,  257,   1, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                         }, // 21 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,      0,              13,  273,  49, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                         }, // 22 Fly around waiting for a landing spot (south)
	/* helicopter */
	{ NOTHING_block,      0,              24,   44,  58, DIR_N,                                    NULL                         }, // 23 Helicopter takeoff spot on ground (to clear airport sooner)
	{ NOTHING_block,      HELITAKEOFF,     0,   44,  63, DIR_N  | AMED_HELI_RAISE,                 NULL                         }, // 24 Helicopter takeoff
	{ IN_WAY_block,       HELILANDING,    26,   15,  54, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                         }, // 25 Get in position above landing spot helicopter
	{ IN_WAY_block,       HELIENDLANDING, 18,   15,  54, DIR_N  | AMED_HELI_LOWER,                 NULL                         }, // 26 Helicopter landing
	{ TAXIWAY_BUSY_block, 255,            27,   21,  58, DIR_SW | AMED_EXACTPOS,                   _airport_fta_metropolitan_27 }, // 27 Transitions after landing to on-ground movement
};

static const HangarTileTable _airport_depots_international[] = { {{0, 3}, DIR_SE, 0}, {{6, 1}, DIR_SE, 1} };
static const byte _airport_terminal_international[] = { 2, 0, 3, 6 };
static const byte _airport_entries_international[] = { 38, 37, 40, 39 };

static const AirportFTAClass::Transition _airport_fta_international_0[] = {
	{ TERM_GROUP1_block,        255,          0, false },
	{ TERM_GROUP2_ENTER1_block, 255,          1, false },
	{ HELIPAD1_block,           HELITAKEOFF,  2, false },
	{ 0,                        0,            2, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_1[] = {
	{ HANGAR2_AREA_block,       255,          1, false },
	{ HELIPAD2_block,           HELITAKEOFF,  3, false },
	{ 0,                        0,            3, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_2[] = {
	{ 0,                        HANGAR,       0, false },
	{ 0,                        TERM4,       12, false },
	{ 0,                        TERM5,       12, false },
	{ 0,                        TERM6,       12, false },
	{ 0,                        HELIPAD1,    12, false },
	{ 0,                        HELIPAD2,    12, false },
	{ 0,                        HELITAKEOFF, 12, false },
	{ 0,                        0,           23, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_3[] = {
	{ 0,                        HANGAR,       1, false },
	{ 0,                        0,           18, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_4[] = {
	{ AIRPORT_ENTRANCE_block,   HANGAR,      23, false },
	{ 0,                        0,           23, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_5[] = {
	{ AIRPORT_ENTRANCE_block,   HANGAR,      24, false },
	{ 0,                        0,           24, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_6[] = {
	{ AIRPORT_ENTRANCE_block,   HANGAR,      25, false },
	{ 0,                        0,           25, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_7[] = {
	{ HANGAR2_AREA_block,       HANGAR,      16, false },
	{ 0,                        0,           16, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_8[] = {
	{ HANGAR2_AREA_block,       HANGAR,      17, false },
	{ 0,                        0,           17, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_9[] = {
	{ HANGAR2_AREA_block,       HANGAR,      18, false },
	{ 0,                        0,           18, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_10[] = {
	{ HANGAR2_AREA_block,       HANGAR,      16, false },
	{ 0,                        HELITAKEOFF, 47, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_11[] = {
	{ HANGAR2_AREA_block,       HANGAR,      17, false },
	{ 0,                        HELITAKEOFF, 48, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_16[] = {
	{ TERM4_block,              TERM4,        7, false },
	{ HELIPAD1_block,           HELIPAD1,    10, false },
	{ HELIPAD1_block,           HELITAKEOFF, 10, false },
	{ 0,                        0,           17, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_17[] = {
	{ TERM5_block,              TERM5,        8, false },
	{ 0,                        TERM4,       16, false },
	{ 0,                        HELIPAD1,    16, false },
	{ HELIPAD2_block,           HELIPAD2,    11, false },
	{ HELIPAD2_block,           HELITAKEOFF, 11, false },
	{ 0,                        0,           18, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_18[] = {
	{ TERM6_block,              TERM6,        9, false },
	{ 0,                        TAKEOFF,     19, false },
	{ HANGAR2_AREA_block,       HANGAR,       3, false },
	{ 0,                        0,           17, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_23[] = {
	{ TERM1_block,              TERM1,        4, false },
	{ AIRPORT_ENTRANCE_block,   HANGAR,       2, false },
	{ 0,                        0,           24, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_24[] = {
	{ TERM2_block,              TERM2,        5, false },
	{ 0,                        TERM1,       23, false },
	{ 0,                        HANGAR,      23, false },
	{ 0,                        0,           25, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_25[] = {
	{ TERM3_block,              TERM3,        6, false },
	{ 0,                        TAKEOFF,     26, false },
	{ 0,                        0,           24, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_26[] = {
	{ 0,                        TAKEOFF,     27, false },
	{ 0,                        0,           25, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_32[] = {
	{ 0,                        LANDING,     33, false },
	{ 0,                        HELILANDING, 41, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_36[] = {
	{ TERM_GROUP1_block,        255,          0, false },
	{ TERM_GROUP2_ENTER1_block, 255,          1, false },
	{ 0,                        TERM4,       12, false },
	{ 0,                        TERM5,       12, false },
	{ 0,                        TERM6,       12, false },
	{ 0,                        0,            2, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_42[] = {
	{ 0,                        HELIPAD1,    43, false },
	{ 0,                        HELIPAD2,    44, false },
	{ 0,                        HANGAR,      49, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_45[] = {
	{ HELIPAD1_block,           HELIPAD1,    10, true  }
};
static const AirportFTAClass::Transition _airport_fta_international_46[] = {
	{ HELIPAD2_block,           HELIPAD2,    11, true  }
};
static const AirportFTAClass::Position _airport_fta_international[51] = {
	{ NOTHING_block,            HANGAR,          2,    7,  55, DIR_SE | AMED_EXACTPOS,                   _airport_fta_international_0  }, // 00 In Hangar 1
	{ NOTHING_block,            HANGAR,          3,  100,  21, DIR_SE | AMED_EXACTPOS,                   _airport_fta_international_1  }, // 01 In Hangar 2
	{ AIRPORT_ENTRANCE_block,   255,             0,    7,  70, DIR_N,                                    _airport_fta_international_2  }, // 02 Taxi to right outside depot
	{ HANGAR2_AREA_block,       255,             0,  100,  36, DIR_N,                                    _airport_fta_international_3  }, // 03 Taxi to right outside depot
	{ TERM1_block,              TERM1,          23,   38,  70, DIR_SW | AMED_EXACTPOS,                   _airport_fta_international_4  }, // 04 Terminal 1
	{ TERM2_block,              TERM2,          24,   38,  54, DIR_SW | AMED_EXACTPOS,                   _airport_fta_international_5  }, // 05 Terminal 2
	{ TERM3_block,              TERM3,          25,   38,  38, DIR_SW | AMED_EXACTPOS,                   _airport_fta_international_6  }, // 06 Terminal 3
	{ TERM4_block,              TERM4,          16,   70,  70, DIR_NE | AMED_EXACTPOS,                   _airport_fta_international_7  }, // 07 Terminal 4
	{ TERM5_block,              TERM5,          17,   70,  54, DIR_NE | AMED_EXACTPOS,                   _airport_fta_international_8  }, // 08 Terminal 5
	{ TERM6_block,              TERM6,          18,   70,  38, DIR_NE | AMED_EXACTPOS,                   _airport_fta_international_9  }, // 09 Terminal 6
	{ HELIPAD1_block,           HELIPAD1,       10,  104,  71, DIR_NE | AMED_EXACTPOS,                   _airport_fta_international_10 }, // 10 Helipad 1
	{ HELIPAD2_block,           HELIPAD2,       11,  104,  55, DIR_NE | AMED_EXACTPOS,                   _airport_fta_international_11 }, // 11 Helipad 2
	{ TERM_GROUP2_ENTER1_block, 0,              13,   22,  87, DIR_N,                                    NULL                          }, // 12 Towards Terminals 4/5/6, Helipad 1/2
	{ TERM_GROUP2_ENTER1_block, 0,              14,   60,  87, DIR_N,                                    NULL                          }, // 13 Towards Terminals 4/5/6, Helipad 1/2
	{ TERM_GROUP2_ENTER2_block, 0,              15,   66,  87, DIR_N,                                    NULL                          }, // 14 Towards Terminals 4/5/6, Helipad 1/2
	{ TERM_GROUP2_ENTER2_block, 0,              16,   86,  87, DIR_NW | AMED_EXACTPOS,                   NULL                          }, // 15 Towards Terminals 4/5/6, Helipad 1/2
	{ TERM_GROUP2_block,        255,             0,   86,  70, DIR_N,                                    _airport_fta_international_16 }, // 16 In Front of Terminal 4 / Helipad 1
	{ TERM_GROUP2_block,        255,             0,   86,  54, DIR_N,                                    _airport_fta_international_17 }, // 17 In Front of Terminal 5 / Helipad 2
	{ TERM_GROUP2_block,        255,             0,   86,  38, DIR_N,                                    _airport_fta_international_18 }, // 18 In Front of Terminal 6
	{ TERM_GROUP2_EXIT1_block,  0,              20,   86,  22, DIR_N,                                    NULL                          }, // 19 Towards Terminals Takeoff (Taxiway)
	{ TERM_GROUP2_EXIT1_block,  0,              21,   66,  22, DIR_N,                                    NULL                          }, // 20 Towards Terminals Takeoff (Taxiway)
	{ TERM_GROUP2_EXIT2_block,  0,              22,   60,  22, DIR_N,                                    NULL                          }, // 21 Towards Terminals Takeoff (Taxiway)
	{ TERM_GROUP2_EXIT2_block,  0,              26,   38,  22, DIR_N,                                    NULL                          }, // 22 Towards Terminals Takeoff (Taxiway)
	{ TERM_GROUP1_block,        255,             0,   22,  70, DIR_N,                                    _airport_fta_international_23 }, // 23 In Front of Terminal 1
	{ TERM_GROUP1_block,        255,             0,   22,  58, DIR_N,                                    _airport_fta_international_24 }, // 24 In Front of Terminal 2
	{ TERM_GROUP1_block,        255,             0,   22,  38, DIR_N,                                    _airport_fta_international_25 }, // 25 In Front of Terminal 3
	{ TAXIWAY_BUSY_block,       255,             0,   22,  22, DIR_NW | AMED_EXACTPOS,                   _airport_fta_international_26 }, // 26 Going for Takeoff
	{ OUT_WAY_block,            0,              28,   22,   6, DIR_N,                                    NULL                          }, // 27 On Runway-out, prepare for takeoff
	/* takeoff */
	{ OUT_WAY_block,            TAKEOFF,        29,    3,   6, DIR_SW | AMED_EXACTPOS,                   NULL                          }, // 28 Accelerate to end of runway
	{ RUNWAY_OUT_block,         0,              30,   60,   6, DIR_N  | AMED_NOSPDCLAMP,                 NULL                          }, // 29 Release control of runway, for smoother movement
	{ NOTHING_block,            STARTTAKEOFF,   31,  105,   6, DIR_N  | AMED_NOSPDCLAMP,                 NULL                          }, // 30 End of runway
	{ NOTHING_block,            ENDTAKEOFF,      0,  190,   6, DIR_N  | AMED_NOSPDCLAMP | AMED_TAKEOFF,  NULL                          }, // 31 Take off
	/* landing */
	{ NOTHING_block,            FLYING,         37,  193, 104, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_international_32 }, // 32 Fly to landing position in air
	{ RUNWAY_IN_block,          LANDING,        34,  105, 104, DIR_N  | AMED_NOSPDCLAMP | AMED_LAND,     NULL                          }, // 33 Going down for land
	{ RUNWAY_IN_block,          0,              35,    3, 104, DIR_N  | AMED_NOSPDCLAMP | AMED_BRAKE,    NULL                          }, // 34 Just landed, brake until end of runway
	{ RUNWAY_IN_block,          0,              36,   12, 104, DIR_N  | AMED_SLOWTURN,                   NULL                          }, // 35 Just landed, turn around and taxi 1 square
	{ IN_WAY_block,             ENDLANDING,     36,    7,  84, DIR_N,                                    _airport_fta_international_36 }, // 36 Taxi from runway to crossing
	/* flying */
	{ NOTHING_block,            0,              38,    1, 209, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 37 Fly around waiting for a landing spot (north-east)
	{ NOTHING_block,            0,              39,    1,   6, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 38 Fly around waiting for a landing spot (north-west)
	{ NOTHING_block,            0,              40,  273,   6, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 39 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,            0,              32,  305,  81, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 40 Fly around waiting for a landing spot (south)
	/* helicopter -- stay in air in special place as a buffer to choose from helipads */
	{ PRE_HELIPAD_block,        HELILANDING,    42,  128,  80, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 41 Bufferspace before helipad
	{ PRE_HELIPAD_block,        HELIENDLANDING, 42,  128,  80, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_international_42 }, // 42 Bufferspace before helipad
	{ NOTHING_block,            0,              45,   96,  71, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 43 Get in position for Helipad1
	{ NOTHING_block,            0,              46,   96,  55, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 44 Get in position for Helipad2
	/* landing */
	{ NOTHING_block,            255,             0,   96,  71, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_international_45 }, // 45 Land at Helipad1
	{ NOTHING_block,            255,             0,   96,  55, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_international_46 }, // 46 Land at Helipad2
	/* helicopter takeoff */
	{ NOTHING_block,            HELITAKEOFF,     0,  104,  71, DIR_N  | AMED_HELI_RAISE,                 NULL                          }, // 47 Takeoff Helipad1
	{ NOTHING_block,            HELITAKEOFF,     0,  104,  55, DIR_N  | AMED_HELI_RAISE,                 NULL                          }, // 48 Takeoff Helipad2
	{ HANGAR2_AREA_block,       0,              50,  104,  32, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                          }, // 49 Go to position for Hangarentrance in air
	{ HANGAR2_AREA_block,       0,               3,  104,  32, DIR_N  | AMED_HELI_LOWER,                 NULL                          }, // 50 Land in HANGAR2_AREA to go to hangar
};

/* intercontinental */
static const HangarTileTable _airport_depots_intercontinental[] = { {{0, 5}, DIR_SE, 0}, {{8, 4}, DIR_SE, 1} };
static const byte _airport_terminal_intercontinental[] = { 2, 0, 4, 8 };
static const byte _airport_entries_intercontinental[] = { 44, 43, 46, 45 };

static const AirportFTAClass::Transition _airport_fta_intercontinental_0[] = {
	{ HANGAR1_AREA_block | TERM_GROUP1_block, 255,          0, false },
	{ HANGAR1_AREA_block | TERM_GROUP1_block, 255,          1, false },
	{ HANGAR1_AREA_block | TERM_GROUP1_block, TAKEOFF,      2, false },
	{ 0,                                      0,            2, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_1[] = {
	{ HANGAR2_AREA_block,                     255,          1, false },
	{ HANGAR2_AREA_block,                     255,          0, false },
	{ 0,                                      0,            3, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_2[] = {
	{ TERM_GROUP1_block,                      255,          0, false },
	{ TERM_GROUP1_block,                      255,          1, false },
	{ 0,                                      HANGAR,       0, false },
	{ TERM_GROUP1_block,                      TAKEOFF,     27, false },
	{ 0,                                      TERM5,       26, false },
	{ 0,                                      TERM6,       26, false },
	{ 0,                                      TERM7,       26, false },
	{ 0,                                      TERM8,       26, false },
	{ 0,                                      HELIPAD1,    26, false },
	{ 0,                                      HELIPAD2,    26, false },
	{ 0,                                      HELITAKEOFF, 74, false },
	{ 0,                                      0,           27, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_3[] = {
	{ 0,                                      HANGAR,       1, false },
	{ 0,                                      HELITAKEOFF, 75, false },
	{ 0,                                      TAKEOFF,     59, false },
	{ 0,                                      0,           20, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_4[] = {
	{ HANGAR1_AREA_block | TERM_GROUP1_block, HANGAR,      26, false },
	{ 0,                                      0,           26, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_5[] = {
	{ HANGAR1_AREA_block | TERM_GROUP1_block, HANGAR,      27, false },
	{ 0,                                      0,           27, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_6[] = {
	{ HANGAR1_AREA_block | TERM_GROUP1_block, HANGAR,      28, false },
	{ 0,                                      0,           28, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_7[] = {
	{ HANGAR1_AREA_block | TERM_GROUP1_block, HANGAR,      29, false },
	{ 0,                                      0,           29, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_8[] = {
	{ HANGAR2_AREA_block,                     HANGAR,      18, false },
	{ 0,                                      0,           18, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_9[] = {
	{ HANGAR2_AREA_block,                     HANGAR,      19, false },
	{ 0,                                      0,           19, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_10[] = {
	{ HANGAR2_AREA_block,                     HANGAR,      20, false },
	{ 0,                                      0,           20, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_11[] = {
	{ HANGAR2_AREA_block,                     HANGAR,      21, false },
	{ 0,                                      0,           21, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_12[] = {
	{ 0,                                      HANGAR,      70, false },
	{ 0,                                      HELITAKEOFF, 72, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_13[] = {
	{ 0,                                      HANGAR,      71, false },
	{ 0,                                      HELITAKEOFF, 73, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_18[] = {
	{ TERM5_block,                            TERM5,        8, false },
	{ 0,                                      TAKEOFF,     19, false },
	{ HELIPAD1_block,                         HELITAKEOFF, 19, false },
	{ TERM_GROUP2_EXIT1_block,                0,           19, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_19[] = {
	{ TERM6_block,                            TERM6,        9, false },
	{ 0,                                      TERM5,       18, false },
	{ 0,                                      TAKEOFF,     57, false },
	{ HELIPAD1_block,                         HELITAKEOFF, 20, false },
	{ TERM_GROUP2_EXIT1_block,                0,           20, true  } // add exit to runway out 2
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_20[] = {
	{ TERM7_block,                            TERM7,       10, false },
	{ 0,                                      TERM5,       19, false },
	{ 0,                                      TERM6,       19, false },
	{ HANGAR2_AREA_block,                     HANGAR,       3, false },
	{ 0,                                      TAKEOFF,     19, false },
	{ TERM_GROUP2_EXIT1_block,                0,           21, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_21[] = {
	{ TERM8_block,                            TERM8,       11, false },
	{ HANGAR2_AREA_block,                     HANGAR,      20, false },
	{ 0,                                      TERM5,       20, false },
	{ 0,                                      TERM6,       20, false },
	{ 0,                                      TERM7,       20, false },
	{ 0,                                      TAKEOFF,     20, false },
	{ TERM_GROUP2_EXIT1_block,                0,           22, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_22[] = {
	{ 0,                                      HANGAR,      21, false },
	{ 0,                                      TERM5,       21, false },
	{ 0,                                      TERM6,       21, false },
	{ 0,                                      TERM7,       21, false },
	{ 0,                                      TERM8,       21, false },
	{ 0,                                      TAKEOFF,     21, false },
	{ 0,                                      0,           23, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_25[] = {
	{ HANGAR1_AREA_block | TERM_GROUP1_block, HANGAR,      29, false },
	{ 0,                                      0,           29, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_26[] = {
	{ TERM1_block,                            TERM1,        4, false },
	{ HANGAR1_AREA_block,                     HANGAR,      27, false },
	{ TERM_GROUP2_ENTER1_block,               TERM5,       14, false },
	{ TERM_GROUP2_ENTER1_block,               TERM6,       14, false },
	{ TERM_GROUP2_ENTER1_block,               TERM7,       14, false },
	{ TERM_GROUP2_ENTER1_block,               TERM8,       14, false },
	{ TERM_GROUP2_ENTER1_block,               HELIPAD1,    14, false },
	{ TERM_GROUP2_ENTER1_block,               HELIPAD2,    14, false },
	{ TERM_GROUP2_ENTER1_block,               HELITAKEOFF, 14, false },
	{ 0,                                      0,           27, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_27[] = {
	{ TERM2_block,                            TERM2,        5, false },
	{ HANGAR1_AREA_block,                     HANGAR,       2, false },
	{ 0,                                      TERM1,       26, false },
	{ 0,                                      TERM5,       26, false },
	{ 0,                                      TERM6,       26, false },
	{ 0,                                      TERM7,       26, false },
	{ 0,                                      TERM8,       26, false },
	{ 0,                                      HELIPAD1,    14, false },
	{ 0,                                      HELIPAD2,    14, false },
	{ 0,                                      0,           28, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_28[] = {
	{ TERM3_block,                            TERM3,        6, false },
	{ HANGAR1_AREA_block,                     HANGAR,      27, false },
	{ 0,                                      TERM1,       27, false },
	{ 0,                                      TERM2,       27, false },
	{ 0,                                      TERM4,       29, false },
	{ 0,                                      TERM5,       14, false },
	{ 0,                                      TERM6,       14, false },
	{ 0,                                      TERM7,       14, false },
	{ 0,                                      TERM8,       14, false },
	{ 0,                                      HELIPAD1,    14, false },
	{ 0,                                      HELIPAD2,    14, false },
	{ 0,                                      0,           29, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_29[] = {
	{ TERM4_block,                            TERM4,        7, false },
	{ HANGAR1_AREA_block,                     HANGAR,      27, false },
	{ 0,                                      TAKEOFF,     30, false },
	{ 0,                                      0,           28, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_42[] = {
	{ TERM_GROUP1_block,                      255,          0, false },
	{ TERM_GROUP1_block,                      255,          1, false },
	{ 0,                                      HANGAR,       2, false },
	{ 0,                                      0,           26, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_44[] = {
	{ 0,                                      HELILANDING, 47, false },
	{ 0,                                      LANDING,     69, false },
	{ 0,                                      0,           45, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_46[] = {
	{ 0,                                      LANDING,     76, false },
	{ 0,                                      0,           43, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_48[] = {
	{ 0,                                      HELIPAD1,    49, false },
	{ 0,                                      HELIPAD2,    50, false },
	{ 0,                                      HANGAR,      55, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_51[] = {
	{ HELIPAD1_block,                         HELIPAD1,    12, false },
	{ 0,                                      HANGAR,      55, false },
	{ 0,                                      0,           12, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_52[] = {
	{ HELIPAD2_block,                         HELIPAD2,    13, false },
	{ 0,                                      HANGAR,      55, false },
	{ 0,                                      0,           13, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_57[] = {
	{ 0,                                      TAKEOFF,     58, false },
	{ 0,                                      0,           58, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_66[] = {
	{ 0,                                      255,          1, false },
	{ 0,                                      255,          0, false },
	{ 0,                                      0,           67, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_68[] = {
	{ TERM_GROUP2_block,                      255,          1, false },
	{ TERM_GROUP1_block,                      255,          0, false },
	{ HANGAR2_AREA_block,                     HANGAR,      22, false },
	{ 0,                                      0,           22, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_69[] = {
	{ RUNWAY_IN2_block,                       0,           63, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_70[] = {
	{ HELIPAD1_block,                         HELIPAD1,    12, false },
	{ HELIPAD1_block,                         HELITAKEOFF, 12, false },
	{ 0,                                      0,           71, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_71[] = {
	{ HELIPAD2_block,                         HELIPAD2,    13, false },
	{ HELIPAD1_block,                         HELITAKEOFF, 12, false },
	{ 0,                                      0,           24, true  }
};
static const AirportFTAClass::Transition _airport_fta_intercontinental_76[] = {
	{ RUNWAY_IN_block,                        0,           37, true  }
};
static const AirportFTAClass::Position _airport_fta_intercontinental[77] = {
	{ NOTHING_block,            HANGAR,          2,     8,   87, DIR_SE | AMED_EXACTPOS,                   _airport_fta_intercontinental_0  }, // 00 In Hangar 1
	{ NOTHING_block,            HANGAR,          3,   136,   72, DIR_SE | AMED_EXACTPOS,                   _airport_fta_intercontinental_1  }, // 01 In Hangar 2
	{ HANGAR1_AREA_block,       255,             0,     8,  104, DIR_N,                                    _airport_fta_intercontinental_2  }, // 02 Taxi to right outside depot 1
	{ HANGAR2_AREA_block,       255,             0,   136,   88, DIR_N,                                    _airport_fta_intercontinental_3  }, // 03 Taxi to right outside depot 2
	{ TERM1_block,              TERM1,          26,    56,  120, DIR_W  | AMED_EXACTPOS,                   _airport_fta_intercontinental_4  }, // 04 Terminal 1
	{ TERM2_block,              TERM2,          27,    56,  104, DIR_SW | AMED_EXACTPOS,                   _airport_fta_intercontinental_5  }, // 05 Terminal 2
	{ TERM3_block,              TERM3,          28,    56,   88, DIR_SW | AMED_EXACTPOS,                   _airport_fta_intercontinental_6  }, // 06 Terminal 3
	{ TERM4_block,              TERM4,          29,    56,   72, DIR_SW | AMED_EXACTPOS,                   _airport_fta_intercontinental_7  }, // 07 Terminal 4
	{ TERM5_block,              TERM5,          18,    88,  120, DIR_N  | AMED_EXACTPOS,                   _airport_fta_intercontinental_8  }, // 08 Terminal 5
	{ TERM6_block,              TERM6,          19,    88,  104, DIR_NE | AMED_EXACTPOS,                   _airport_fta_intercontinental_9  }, // 09 Terminal 6
	{ TERM7_block,              TERM7,          20,    88,   88, DIR_NE | AMED_EXACTPOS,                   _airport_fta_intercontinental_10 }, // 10 Terminal 7
	{ TERM8_block,              TERM8,          21,    88,   72, DIR_NE | AMED_EXACTPOS,                   _airport_fta_intercontinental_11 }, // 11 Terminal 8
	{ HELIPAD1_block,           HELIPAD1,       12,    88,   56, DIR_SE | AMED_EXACTPOS,                   _airport_fta_intercontinental_12 }, // 12 Helipad 1
	{ HELIPAD2_block,           HELIPAD2,       13,    72,   56, DIR_NE | AMED_EXACTPOS,                   _airport_fta_intercontinental_13 }, // 13 Helipad 2
	{ TERM_GROUP2_ENTER1_block, 0,              15,    40,  136, DIR_N,                                    NULL                             }, // 14 Term group 2 enter 1 a
	{ TERM_GROUP2_ENTER1_block, 0,              16,    56,  136, DIR_N,                                    NULL                             }, // 15 Term group 2 enter 1 b
	{ TERM_GROUP2_ENTER2_block, 0,              17,    88,  136, DIR_N,                                    NULL                             }, // 16 Term group 2 enter 2 a
	{ TERM_GROUP2_ENTER2_block, 0,              18,   104,  136, DIR_N,                                    NULL                             }, // 17 Term group 2 enter 2 b
	{ TERM_GROUP2_block,        255,             0,   104,  120, DIR_N,                                    _airport_fta_intercontinental_18 }, // 18 Term group 2 - opp term 5
	{ TERM_GROUP2_block,        255,             0,   104,  104, DIR_N,                                    _airport_fta_intercontinental_19 }, // 19 Term group 2 - opp term 6 & exit2
	{ TERM_GROUP2_block,        255,             0,   104,   88, DIR_N,                                    _airport_fta_intercontinental_20 }, // 20 Term group 2 - opp term 7 & hangar area 2
	{ TERM_GROUP2_block,        255,             0,   104,   72, DIR_N,                                    _airport_fta_intercontinental_21 }, // 21 Term group 2 - opp term 8
	{ TERM_GROUP2_block,        255,             0,   104,   56, DIR_N,                                    _airport_fta_intercontinental_22 }, // 22 Taxi Term group 2 exit a
	{ TERM_GROUP2_EXIT1_block,  0,              70,   104,   40, DIR_N,                                    NULL                             }, // 23 Taxi Term group 2 exit b
	{ TERM_GROUP2_EXIT2_block,  0,              25,    56,   40, DIR_N,                                    NULL                             }, // 24 Term group 2 exit 2a
	{ TERM_GROUP2_EXIT2_block,  255,             0,    40,   40, DIR_N,                                    _airport_fta_intercontinental_25 }, // 25 Term group 2 exit 2b
	{ TERM_GROUP1_block,        255,             0,    40,  120, DIR_N,                                    _airport_fta_intercontinental_26 }, // 26 Term group 1 - opp term 1
	{ TERM_GROUP1_block,        255,             0,    40,  104, DIR_N,                                    _airport_fta_intercontinental_27 }, // 27 Term group 1 - opp term 2 & hangar area 1
	{ TERM_GROUP1_block,        255,             0,    40,   88, DIR_N,                                    _airport_fta_intercontinental_28 }, // 28 Term group 1 - opp term 3
	{ TERM_GROUP1_block,        255,             0,    40,   72, DIR_N,                                    _airport_fta_intercontinental_29 }, // 29 Term group 1 - opp term 4
	{ OUT_WAY_block2,           0,              31,    18,   72, DIR_NW,                                   NULL                             }, // 30 Outway 1
	{ OUT_WAY_block,            0,              32,     8,   40, DIR_NW,                                   NULL                             }, // 31 Airport OUTWAY
	/* takeoff */
	{ RUNWAY_OUT_block,         TAKEOFF,        33,     8,   24, DIR_SW | AMED_EXACTPOS,                   NULL                             }, // 32 Accelerate to end of runway
	{ RUNWAY_OUT_block,         0,              34,   119,   24, DIR_N  | AMED_NOSPDCLAMP,                 NULL                             }, // 33 Release control of runway, for smoother movement
	{ NOTHING_block,            STARTTAKEOFF,   35,   117,   24, DIR_N  | AMED_NOSPDCLAMP,                 NULL                             }, // 34 End of runway
	{ NOTHING_block,            ENDTAKEOFF,      0,   197,   24, DIR_N  | AMED_NOSPDCLAMP | AMED_TAKEOFF,  NULL                             }, // 35 Take off
	/* landing */
	{ 0,                        0,               0,   254,   84, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                             }, // 36 Flying to landing position in air
	{ RUNWAY_IN_block,          LANDING,        38,   117,  168, DIR_N  | AMED_NOSPDCLAMP | AMED_LAND,     NULL                             }, // 37 Going down for land
	{ RUNWAY_IN_block,          0,              39,     8,  168, DIR_N  | AMED_NOSPDCLAMP | AMED_BRAKE,    NULL                             }, // 38 Just landed, brake until end of runway
	{ RUNWAY_IN_block,          0,              40,     8,  168, DIR_N,                                    NULL                             }, // 39 Just landed, turn around and taxi
	{ RUNWAY_IN_block,          ENDLANDING,     41,     8,  144, DIR_NW,                                   NULL                             }, // 40 Taxi from runway
	{ IN_WAY_block,             0,              42,     8,  128, DIR_NW,                                   NULL                             }, // 41 Taxi from runway
	{ IN_WAY_block,             255,             0,     8,  120, DIR_NW | AMED_EXACTPOS,                   _airport_fta_intercontinental_42 }, // 42 Airport entrance
	/* flying */
	{ 0,                        0,              44,    56,  344, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                             }, // 43 Fly around waiting for a landing spot (north-east)
	{ 0,                        FLYING,         45,  -200,   88, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_intercontinental_44 }, // 44 Fly around waiting for a landing spot (north-west)
	{ 0,                        0,              46,    56, -168, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                             }, // 45 Fly around waiting for a landing spot (south-west)
	{ 0,                        FLYING,         43,   312,   88, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_intercontinental_46 }, // 46 Fly around waiting for a landing spot (south)
	/* helicopter -- stay in air in special place as a buffer to choose from helipads */
	{ PRE_HELIPAD_block,        HELILANDING,    48,    96,   40, DIR_N  | AMED_NOSPDCLAMP,                 NULL                             }, // 47 Bufferspace before helipad
	{ PRE_HELIPAD_block,        HELIENDLANDING, 48,    96,   40, DIR_N  | AMED_NOSPDCLAMP,                 _airport_fta_intercontinental_48 }, // 48 Bufferspace before helipad
	{ NOTHING_block,            0,              51,    82,   54, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                             }, // 49 Get in position for Helipad1
	{ NOTHING_block,            0,              52,    64,   56, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                             }, // 50 Get in position for Helipad2
	/* landing */
	{ NOTHING_block,            255,             0,    81,   55, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_intercontinental_51 }, // 51 Land at Helipad1
	{ NOTHING_block,            255,             0,    64,   56, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_intercontinental_52 }, // 52 Land at Helipad2
	/* helicopter takeoff */
	{ NOTHING_block,            HELITAKEOFF,     0,    80,   56, DIR_N  | AMED_HELI_RAISE,                 NULL                             }, // 53 Takeoff Helipad1
	{ NOTHING_block,            HELITAKEOFF,     0,    64,   56, DIR_N  | AMED_HELI_RAISE,                 NULL                             }, // 54 Takeoff Helipad2
	{ HANGAR2_AREA_block,       0,              56,   136,   96, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                             }, // 55 Go to position for Hangarentrance in air
	{ HANGAR2_AREA_block,       0,               3,   136,   96, DIR_N  | AMED_HELI_LOWER,                 NULL                             }, // 56 Land in front of hangar2
	/* runway 2 out support */
	{ OUT_WAY2_block,           255,             0,   126,  104, DIR_SE,                                   _airport_fta_intercontinental_57 }, // 57 Outway 2
	{ OUT_WAY2_block,           0,              59,   136,  136, DIR_NE,                                   NULL                             }, // 58 Airport OUTWAY 2
	{ RUNWAY_OUT2_block,        TAKEOFF,        60,   136,  152, DIR_NE | AMED_EXACTPOS,                   NULL                             }, // 59 Accelerate to end of runway2
	{ RUNWAY_OUT2_block,        0,              61,    16,  152, DIR_N  | AMED_NOSPDCLAMP,                 NULL                             }, // 60 Release control of runway2, for smoother movement
	{ NOTHING_block,            STARTTAKEOFF,   62,    20,  152, DIR_N  | AMED_NOSPDCLAMP,                 NULL                             }, // 61 End of runway2
	{ NOTHING_block,            ENDTAKEOFF,      0,   -56,  152, DIR_N  | AMED_NOSPDCLAMP | AMED_TAKEOFF,  NULL                             }, // 62 Take off2
	/* runway 2 in support */
	{ RUNWAY_IN2_block,         LANDING,        64,    24,    8, DIR_N  | AMED_NOSPDCLAMP | AMED_LAND,     NULL                             }, // 63 Going down for land2
	{ RUNWAY_IN2_block,         0,              65,   136,    8, DIR_N  | AMED_NOSPDCLAMP | AMED_BRAKE,    NULL                             }, // 64 Just landed, brake until end of runway2in
	{ RUNWAY_IN2_block,         0,              66,   136,    8, DIR_N,                                    NULL                             }, // 65 Just landed, turn around and taxi
	{ RUNWAY_IN2_block,         ENDLANDING,      0,   136,   24, DIR_SE,                                   _airport_fta_intercontinental_66 }, // 66 Taxi from runway 2in
	{ IN_WAY2_block,            0,              68,   136,   40, DIR_SE,                                   NULL                             }, // 67 Taxi from runway 2in
	{ IN_WAY2_block,            255,             0,   136,   56, DIR_NE | AMED_EXACTPOS,                   _airport_fta_intercontinental_68 }, // 68 Airport entrance2
	{ RUNWAY_IN2_block,         255,             0,   -56,    8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_intercontinental_69 }, // 69 Fly to landing position in air2
	{ TERM_GROUP2_EXIT1_block,  255,             0,    88,   40, DIR_N,                                    _airport_fta_intercontinental_70 }, // 70 Taxi Term group 2 exit - opp heli1
	{ TERM_GROUP2_EXIT1_block,  255,             0,    72,   40, DIR_N,                                    _airport_fta_intercontinental_71 }, // 71 Taxi Term group 2 exit - opp heli2
	{ HELIPAD1_block,           0,              53,    88,   57, DIR_SE | AMED_EXACTPOS,                   NULL                             }, // 72 pre-helitakeoff helipad 1
	{ HELIPAD2_block,           0,              54,    71,   56, DIR_NE | AMED_EXACTPOS,                   NULL                             }, // 73 pre-helitakeoff helipad 2
	{ NOTHING_block,            HELITAKEOFF,     0,     8,  120, DIR_N  | AMED_HELI_RAISE,                 NULL                             }, // 74 Helitakeoff outside depot 1
	{ NOTHING_block,            HELITAKEOFF,     0,   136,  104, DIR_N  | AMED_HELI_RAISE,                 NULL                             }, // 75 Helitakeoff outside depot 2
	{ RUNWAY_IN_block,          255,             0,   197,  168, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_intercontinental_76 }, // 76 Fly to landing position in air1
};


/* heliports, oilrigs don't have depots */
static const byte _airport_entries_heliport[] = { 7, 7, 7, 7 };

static const AirportFTAClass::Transition _airport_fta_heliport_2[] = {
	{ 0,              HELILANDING, 3, false },
	{ 0,              HELITAKEOFF, 1, true  }
};
static const AirportFTAClass::Transition _airport_fta_heliport_4[] = {
	{ HELIPAD1_block, HELIPAD1,    0, false },
	{ 0,              HELITAKEOFF, 2, true  }
};
static const AirportFTAClass::Transition _airport_fta_heliport_8[] = {
	{ HELIPAD1_block, HELILANDING, 2, true  }
};
static const AirportFTAClass::Position _airport_fta_heliport[9] = {
	{ HELIPAD1_block,     HELIPAD1,       1,    5,   9, DIR_NE | AMED_EXACTPOS,                   NULL                    }, // 0 - At heliport terminal
	{ NOTHING_block,      HELITAKEOFF,    0,    2,   9, DIR_N  | AMED_HELI_RAISE,                 NULL                    }, // 1 - Take off (play sound)
	{ AIRPORT_BUSY_block, 255,            0,   -3,   9, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_heliport_2 }, // 2 - In position above landing spot helicopter
	{ AIRPORT_BUSY_block, HELILANDING,    4,   -3,   9, DIR_N  | AMED_HELI_LOWER,                 NULL                    }, // 3 - Land
	{ AIRPORT_BUSY_block, HELIENDLANDING, 4,    2,   9, DIR_N,                                    _airport_fta_heliport_4 }, // 4 - Goto terminal on ground
	/* flying */
	{ NOTHING_block,      0,              6,  -31,  59, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 5 - Circle #1 (north-east)
	{ NOTHING_block,      0,              7,  -31, -49, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 6 - Circle #2 (north-west)
	{ NOTHING_block,      0,              8,   49, -49, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 7 - Circle #3 (south-west)
	{ NOTHING_block,      FLYING,         5,   70,   9, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_heliport_8 }, // 8 - Circle #4 (south)
};

#define _airport_entries_oilrig _airport_entries_heliport

static const AirportFTAClass::Position _airport_fta_oilrig[9] = {
	{ HELIPAD1_block,     HELIPAD1,       1,   31,   9, DIR_NE | AMED_EXACTPOS,                   NULL                    }, // 0 - At oilrig terminal
	{ NOTHING_block,      HELITAKEOFF,    0,   28,   9, DIR_N  | AMED_HELI_RAISE,                 NULL                    }, // 1 - Take off (play sound)
	{ AIRPORT_BUSY_block, 255,            0,   23,   9, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_heliport_2 }, // 2 - In position above landing spot helicopter
	{ AIRPORT_BUSY_block, HELILANDING,    4,   23,   9, DIR_N  | AMED_HELI_LOWER,                 NULL                    }, // 3 - Land
	{ AIRPORT_BUSY_block, HELIENDLANDING, 4,   28,   9, DIR_N,                                    _airport_fta_heliport_4 }, // 4 - Goto terminal on ground
	/* flying */
	{ NOTHING_block,      0,              6,  -31,  69, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 5 - circle #1 (north-east)
	{ NOTHING_block,      0,              7,  -31, -49, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 6 - circle #2 (north-west)
	{ NOTHING_block,      0,              8,   69, -49, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                    }, // 7 - circle #3 (south-west)
	{ NOTHING_block,      FLYING,         5,   69,   9, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_heliport_8 }, // 8 - circle #4 (south)
};

/* helidepots */
static const HangarTileTable _airport_depots_helidepot[] = { {{1, 0}, DIR_SE, 0} };
static const byte _airport_entries_helidepot[] = { 4, 4, 4, 4 };

static const AirportFTAClass::Transition _airport_fta_helidepot_1[] = {
	{ 0,                 HANGAR,       0, false },
	{ HELIPAD1_block,    HELIPAD1,    14, false },
	{ 0,                 HELITAKEOFF, 15, false },
	{ 0,                 0,            0, true  }
};
static const AirportFTAClass::Transition _airport_fta_helidepot_2[] = {
	{ PRE_HELIPAD_block, HELILANDING,  7, false },
	{ 0,                 HANGAR,      12, false },
	{ NOTHING_block,     HELITAKEOFF, 16, true  }
};
static const AirportFTAClass::Transition _airport_fta_helidepot_8[] = {
	{ 0,                 HELIPAD1,     9, false },
	{ 0,                 HANGAR,      12, false },
	{ 0,                 0,            2, true  }
};
static const AirportFTAClass::Transition _airport_fta_helidepot_10[] = {
	{ HELIPAD1_block,    HELIPAD1,    14, false },
	{ 0,                 HANGAR,       1, false },
	{ 0,                 0,           14, true  }
};
static const AirportFTAClass::Transition _airport_fta_helidepot_14[] = {
	{ 0,                 HANGAR,       1, false },
	{ 0,                 HELITAKEOFF, 17, true  }
};
static const AirportFTAClass::Position _airport_fta_helidepot[18] = {
	{ NOTHING_block,      HANGAR,          1,   24,   4, DIR_NE | AMED_EXACTPOS,                   NULL                      }, // 0 - At depot
	{ HANGAR2_AREA_block, 255,             0,   24,  28, DIR_N,                                    _airport_fta_helidepot_1  }, // 1 Taxi to right outside depot
	{ NOTHING_block,      FLYING,          3,    5,  38, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_helidepot_2  }, // 2 Flying
	/* flying */
	{ NOTHING_block,      0,               4,  -15, -15, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                      }, // 3 - Circle #1 (north-east)
	{ NOTHING_block,      0,               5,  -15, -49, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                      }, // 4 - Circle #2 (north-west)
	{ NOTHING_block,      0,               6,   49, -49, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                      }, // 5 - Circle #3 (south-west)
	{ NOTHING_block,      0,               2,   49, -15, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                      }, // 6 - Circle #4 (south-east)
	/* helicopter -- stay in air in special place as a buffer to choose from helipads */
	{ PRE_HELIPAD_block,  HELILANDING,     8,    8,  32, DIR_NW | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                      }, // 7 - PreHelipad
	{ PRE_HELIPAD_block,  HELIENDLANDING,  8,    8,  32, DIR_NW | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_helidepot_8  }, // 8 - Helipad
	{ NOTHING_block,      0,              10,    8,  16, DIR_NW | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                      }, // 9 - Land
	/* landing */
	{ NOTHING_block,      255,            10,    8,  16, DIR_NW | AMED_HELI_LOWER,                 _airport_fta_helidepot_10 }, // 10 - Land
	/* helicopter takeoff */
	{ NOTHING_block,      HELITAKEOFF,     0,    8,  24, DIR_N  | AMED_HELI_RAISE,                 NULL                      }, // 11 - Take off (play sound)
	{ HANGAR2_AREA_block, 0,              13,   32,  24, DIR_NW | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                      }, // 12 Air to above hangar area
	{ HANGAR2_AREA_block, 0,               1,   32,  24, DIR_NW | AMED_HELI_LOWER,                 NULL                      }, // 13 Taxi to right outside depot
	{ HELIPAD1_block,     HELIPAD1,       14,    8,  24, DIR_NW | AMED_EXACTPOS,                   _airport_fta_helidepot_14 }, // 14 - on helipad1
	{ NOTHING_block,      HELITAKEOFF,     0,   24,  28, DIR_N  | AMED_HELI_RAISE,                 NULL                      }, // 15 Takeoff right outside depot
	{ 0,                  HELITAKEOFF,    14,    8,  24, DIR_SW | AMED_HELI_RAISE,                 NULL                      }, // 16 - Take off (play sound)
	{ NOTHING_block,      0,              11,    8,  24, DIR_E  | AMED_SLOWTURN | AMED_EXACTPOS,   NULL                      }, // 17 - turn on helipad1 for takeoff
};

/* helistation */
static const HangarTileTable _airport_depots_helistation[] = { {{0, 0}, DIR_SE, 0} };
static const byte _airport_entries_helistation[] = { 25, 25, 25, 25 };

static const AirportFTAClass::Transition _airport_fta_helistation_0[] = {
	{ 0,                  HELIPAD1,     1, false },
	{ 0,                  HELIPAD2,     1, false },
	{ 0,                  HELIPAD3,     1, false },
	{ 0,                  HELITAKEOFF,  1, false },
	{ 0,                  0,            0, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_1[] = {
	{ 0,                  HANGAR,       0, false },
	{ 0,                  HELITAKEOFF,  3, false },
	{ 0,                  0,            4, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_2[] = {
	{ 0,                  HELILANDING, 15, false },
	{ 0,                  0,           28, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_4[] = {
	{ HANGAR2_AREA_block, HANGAR,       1, false },
	{ 0,                  HELITAKEOFF,  1, false },
	{ 0,                  0,            5, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_5[] = {
	{ HELIPAD1_block,     HELIPAD1,     6, false },
	{ HELIPAD2_block,     HELIPAD2,     7, false },
	{ HELIPAD3_block,     HELIPAD3,     8, false },
	{ 0,                  0,            4, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_6[] = {
	{ HANGAR2_AREA_block, HANGAR,       5, false },
	{ 0,                  HELITAKEOFF,  9, false },
	{ 0,                  0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_7[] = {
	{ HANGAR2_AREA_block, HANGAR,       5, false },
	{ 0,                  HELITAKEOFF, 10, false },
	{ 0,                  0,            7, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_8[] = {
	{ HANGAR2_AREA_block, HANGAR,       5, false },
	{ 0,                  HELITAKEOFF, 11, false },
	{ 0,                  0,            8, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_16[] = {
	{ 0,                  HELIPAD1,    17, false },
	{ 0,                  HELIPAD2,    18, false },
	{ 0,                  HELIPAD3,    19, false },
	{ 0,                  HANGAR,      23, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_20[] = {
	{ HELIPAD1_block,     HELIPAD1,     6, false },
	{ 0,                  HANGAR,      23, false },
	{ 0,                  0,            6, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_21[] = {
	{ HELIPAD2_block,     HELIPAD2,     7, false },
	{ 0,                  HANGAR,      23, false },
	{ 0,                  0,            7, true  }
};
static const AirportFTAClass::Transition _airport_fta_helistation_22[] = {
	{ HELIPAD3_block,     HELIPAD3,     8, false },
	{ 0,                  HANGAR,      23, false },
	{ 0,                  0,            8, true  }
};
static const AirportFTAClass::Position _airport_fta_helistation[33] = {
	{ NOTHING_block,      HANGAR,          8,    8,   3, DIR_SE | AMED_EXACTPOS,                   _airport_fta_helistation_0  }, // 00 In Hangar2
	{ HANGAR2_AREA_block, 255,             0,    8,  22, DIR_N,                                    _airport_fta_helistation_1  }, // 01 outside hangar 2
	/* landing */
	{ NOTHING_block,      FLYING,         28,  116,  24, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_helistation_2  }, // 02 Fly to landing position in air
	/* helicopter side */
	{ NOTHING_block,      HELITAKEOFF,     0,   14,  22, DIR_N  | AMED_HELI_RAISE,                 NULL                        }, // 03 Helitakeoff outside hangar1 (play sound)
	{ TAXIWAY_BUSY_block, 255,             0,   24,  22, DIR_N,                                    _airport_fta_helistation_4  }, // 04 taxiing
	{ TAXIWAY_BUSY_block, 255,             0,   40,  22, DIR_N,                                    _airport_fta_helistation_5  }, // 05 taxiing
	{ HELIPAD1_block,     HELIPAD1,        5,   40,   8, DIR_NE | AMED_EXACTPOS,                   _airport_fta_helistation_6  }, // 06 Helipad 1
	{ HELIPAD2_block,     HELIPAD2,        5,   56,   8, DIR_NE | AMED_EXACTPOS,                   _airport_fta_helistation_7  }, // 07 Helipad 2
	{ HELIPAD3_block,     HELIPAD3,        5,   56,  24, DIR_NE | AMED_EXACTPOS,                   _airport_fta_helistation_8  }, // 08 Helipad 3
	{ HELIPAD1_block,     0,              12,   40,   8, DIR_N  | AMED_EXACTPOS,                   NULL                        }, // 09 pre-helitakeoff helipad 1
	{ HELIPAD2_block,     0,              13,   56,   8, DIR_N  | AMED_EXACTPOS,                   NULL                        }, // 10 pre-helitakeoff helipad 2
	{ HELIPAD3_block,     0,              14,   56,  24, DIR_N  | AMED_EXACTPOS,                   NULL                        }, // 11 pre-helitakeoff helipad 3
	{ NOTHING_block,      HELITAKEOFF,     0,   32,   8, DIR_N  | AMED_HELI_RAISE,                 NULL                        }, // 12 Takeoff Helipad1
	{ NOTHING_block,      HELITAKEOFF,     0,   48,   8, DIR_N  | AMED_HELI_RAISE,                 NULL                        }, // 13 Takeoff Helipad2
	{ NOTHING_block,      HELITAKEOFF,     0,   48,  24, DIR_N  | AMED_HELI_RAISE,                 NULL                        }, // 14 Takeoff Helipad3
	/* flying */
	{ PRE_HELIPAD_block,  HELILANDING,    16,   84,  24, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 15 Bufferspace before helipad
	{ PRE_HELIPAD_block,  HELIENDLANDING, 16,   68,  24, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, _airport_fta_helistation_16 }, // 16 Bufferspace before helipad
	{ NOTHING_block,      0,              20,   32,   8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 17 Get in position for Helipad1
	{ NOTHING_block,      0,              21,   48,   8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 18 Get in position for Helipad2
	{ NOTHING_block,      0,              22,   48,  24, DIR_NE | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 19 Get in position for Helipad3
	/* helicopter landing */
	{ NOTHING_block,      255,             0,   40,   8, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_helistation_20 }, // 20 Land at Helipad1
	{ NOTHING_block,      255,             0,   48,   8, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_helistation_21 }, // 21 Land at Helipad2
	{ NOTHING_block,      255,             0,   48,  24, DIR_N  | AMED_HELI_LOWER,                 _airport_fta_helistation_22 }, // 22 Land at Helipad3
	{ HANGAR2_AREA_block, 0,              24,    0,  22, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 23 Go to position for Hangarentrance in air
	{ HANGAR2_AREA_block, 0,               1,    0,  22, DIR_N  | AMED_HELI_LOWER,                 NULL                        }, // 24 Land in front of hangar
	{ NOTHING_block,      0,              26,  148,  -8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 25 Fly around waiting for a landing spot (south-east)
	{ NOTHING_block,      0,              27,  148,   8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 26 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,      0,               2,  132,  24, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 27 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,      0,              29,  100,  24, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 28 Fly around waiting for a landing spot (north-east)
	{ NOTHING_block,      0,              30,   84,   8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 29 Fly around waiting for a landing spot (south-east)
	{ NOTHING_block,      0,              31,   84,  -8, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 30 Fly around waiting for a landing spot (south-west)
	{ NOTHING_block,      0,              32,  100, -24, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 31 Fly around waiting for a landing spot (north-west)
	{ NOTHING_block,      0,              25,  132, -24, DIR_N  | AMED_NOSPDCLAMP | AMED_SLOWTURN, NULL                        }, // 32 Fly around waiting for a landing spot (north-east)
};

#endif /* AIRPORT_MOVEMENT_H */
