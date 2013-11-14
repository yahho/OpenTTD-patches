/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map_sl.cpp Code handling saving and loading of map */

#include "../stdafx.h"
#include "../core/bitmath_func.hpp"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../town.h"
#include "../landscape_type.h"
#include "../signal_type.h"
#include "../map/ground.h"
#include "../map/water.h"
#include "../map/station.h"

#include "saveload_buffer.h"


enum OldTileType {
	OLD_MP_CLEAR,           ///< A tile without any structures, i.e. grass, docks, farm fields etc.
	OLD_MP_RAILWAY,         ///< A railway
	OLD_MP_ROAD,            ///< A tile with road (or tram tracks)
	OLD_MP_HOUSE,           ///< A house by a town
	OLD_MP_TREES,           ///< Tile got trees
	OLD_MP_STATION,         ///< A tile of a station
	OLD_MP_WATER,           ///< Water tile
	OLD_MP_VOID,            ///< Invisible tiles at the SW and SE border
	OLD_MP_INDUSTRY,        ///< Part of an industry
	OLD_MP_TUNNELBRIDGE,    ///< Tunnel entry/exit and bridge heads
	OLD_MP_OBJECT,          ///< Contains objects such as transmitters and owned land
};

static inline OldTileType GetOldTileType(TileIndex tile)
{
	assert(tile < MapSize());
	return (OldTileType)GB(_mth[tile], 4, 4);
}

static inline bool IsOldTileType(TileIndex tile, OldTileType type)
{
	return GetOldTileType(tile) == type;
}

static inline void SetOldTileType(TileIndex tile, OldTileType type)
{
	SB(_mth[tile], 4, 4, type);
}

static inline uint OldTileHeight(TileIndex tile)
{
	assert(tile < MapSize());
	return GB(_mth[tile], 0, 4);
}

static bool IsOldTileFlat(TileIndex tile)
{
	assert(tile < MapSize());

	if (TileX(tile) == MapMaxX() || TileY(tile) == MapMaxY() ||
			TileX(tile) == 0 || TileY(tile) == 0) {
		return true;
	}

	uint h = OldTileHeight(tile);
	if (OldTileHeight(tile + TileDiffXY(1, 0)) != h) return false;
	if (OldTileHeight(tile + TileDiffXY(0, 1)) != h) return false;
	if (OldTileHeight(tile + TileDiffXY(1, 1)) != h) return false;

	return true;
}


/**
 *  Fix map array after loading an old savegame
 */
void AfterLoadMap(const SavegameTypeVersion *stv)
{
	TileIndex map_size = MapSize();

	/* in legacy version 2.1 of the savegame, town owner was unified. */
	if (IsOTTDSavegameVersionBefore(stv, 2, 1)) {
		for (TileIndex tile = 0; tile < map_size; tile++) {
			switch (GetOldTileType(tile)) {
				case OLD_MP_ROAD:
					if (GB(_mc[tile].m5, 4, 2) == 1 && HasBit(_mc[tile].m3, 7)) {
						_mc[tile].m3 = OWNER_TOWN;
					}
					/* FALL THROUGH */

				case OLD_MP_TUNNELBRIDGE:
					if (_mc[tile].m1 & 0x80) SB(_mc[tile].m1, 0, 5, OWNER_TOWN);
					break;

				default: break;
			}
		}
	}

	/* In legacy version 6.1 we put the town index in the map-array. To do this, we need
	 *  to use m2 (16bit big), so we need to clean m2, and that is where this is
	 *  all about ;) */
	if (IsOTTDSavegameVersionBefore(stv, 6, 1)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				case OLD_MP_HOUSE:
					_mc[t].m4 = _mc[t].m2;
					_mc[t].m2 = 0;
					break;

				case OLD_MP_ROAD:
					_mc[t].m4 |= (_mc[t].m2 << 4);
					_mc[t].m2 = 0;
					break;

				default: break;
			}
		}
	}

	/* From legacy version 15, we moved a semaphore bit from bit 2 to bit 3 in m4, making
	 *  room for PBS. Now in version 21 move it back :P. */
	if (IsOTTDSavegameVersionBefore(stv, 21) && !IsOTTDSavegameVersionBefore(stv, 15)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				case OLD_MP_RAILWAY:
					if (GB(_mc[t].m5, 6, 2) == 1) {
						/* convert PBS signals to combo-signals */
						if (HasBit(_mc[t].m2, 2)) SB(_mc[t].m2, 0, 3, 3);

						/* move the signal variant back */
						SB(_mc[t].m2, 3, 1, HasBit(_mc[t].m2, 3) ? SIG_SEMAPHORE : SIG_ELECTRIC);
						ClrBit(_mc[t].m2, 3);
					}

					/* Clear PBS reservation on track */
					if (GB(_mc[t].m5, 6, 2) != 3) {
						SB(_mc[t].m4, 4, 4, 0);
					} else {
						ClrBit(_mc[t].m3, 6);
					}
					break;

				case OLD_MP_STATION: // Clear PBS reservation on station
					ClrBit(_mc[t].m3, 6);
					break;

				default: break;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 48)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				case OLD_MP_RAILWAY:
					if (!HasBit(_mc[t].m5, 7)) {
						/* Swap ground type and signal type for plain rail tiles, so the
						 * ground type uses the same bits as for depots and waypoints. */
						uint tmp = GB(_mc[t].m4, 0, 4);
						SB(_mc[t].m4, 0, 4, GB(_mc[t].m2, 0, 4));
						SB(_mc[t].m2, 0, 4, tmp);
					} else if (HasBit(_mc[t].m5, 2)) {
						/* Split waypoint and depot rail type and remove the subtype. */
						ClrBit(_mc[t].m5, 2);
						ClrBit(_mc[t].m5, 6);
					}
					break;

				case OLD_MP_ROAD:
					/* Swap m3 and m4, so the track type for rail crossings is the
					 * same as for normal rail. */
					Swap(_mc[t].m3, _mc[t].m4);
					break;

				default: break;
			}
		}
	}

	/* From legacy version 53, the map array was changed for house tiles to allow
	 * space for newhouses grf features. A new byte, m7, was also added. */
	if (IsOTTDSavegameVersionBefore(stv, 53)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsOldTileType(t, OLD_MP_HOUSE)) {
				if (GB(_mc[t].m3, 6, 2) != TOWN_HOUSE_COMPLETED) {
					/* Move the construction stage from m3[7..6] to m5[5..4].
					 * The construction counter does not have to move. */
					SB(_mc[t].m5, 3, 2, GB(_mc[t].m3, 6, 2));
					SB(_mc[t].m3, 6, 2, 0);
				} else {
					/* The "lift has destination" bit has been moved from
					 * m5[7] to m7[0]. */
					SB(_mc[t].m7, 0, 1, HasBit(_mc[t].m5, 7));
					ClrBit(_mc[t].m5, 7);

					/* The "lift is moving" bit has been removed, as it does
					 * the same job as the "lift has destination" bit. */
					ClrBit(_mc[t].m1, 7);

					/* The position of the lift goes from m1[7..0] to m0[7..2],
					 * making m1 totally free, now. The lift position does not
					 * have to be a full byte since the maximum value is 36. */
					SB(_mc[t].m0, 2, 6, GB(_mc[t].m1, 0, 6 ));

					_mc[t].m1 = 0;
					_mc[t].m3 = 0x80;
				}
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 64)) {
		/* copy the signal type/variant and move signal states bits */
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsOldTileType(t, OLD_MP_RAILWAY) && GB(_mc[t].m5, 6, 2) == 1) {
				SB(_mc[t].m4, 4, 4, GB(_mc[t].m2, 4, 4));
				SB(_mc[t].m2, 7, 1, GB(_mc[t].m2, 3, 1));
				SB(_mc[t].m2, 4, 3, GB(_mc[t].m2, 0, 3));
				ClrBit(_mc[t].m2, 7);
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 72)) {
		/* Locks in very old savegames had OWNER_WATER as owner */
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				default: break;

				case OLD_MP_WATER:
					if (GB(_mc[t].m5, 4, 4) == 1 && GB(_mc[t].m1, 0, 5) == OWNER_WATER) SB(_mc[t].m1, 0, 5, OWNER_NONE);
					break;

				case OLD_MP_STATION: {
					if (HasBit(_mc[t].m0, 3)) SetBit(_mc[t].m0, 2);
					byte gfx = _mc[t].m5;
					int st;
					if (       IsInsideMM(gfx,   0,   8)) { // Rail station
						st = 0;
						_mc[t].m5 = gfx - 0;
					} else if (IsInsideMM(gfx,   8,  67)) { // Airport
						st = 1;
						_mc[t].m5 = gfx - 8;
					} else if (IsInsideMM(gfx,  67,  71)) { // Truck
						st = 2;
						_mc[t].m5 = gfx - 67;
					} else if (IsInsideMM(gfx,  71,  75)) { // Bus
						st = 3;
						_mc[t].m5 = gfx - 71;
					} else if (gfx == 75) {                 // Oil rig
						st = 4;
						_mc[t].m5 = gfx - 75;
					} else if (IsInsideMM(gfx,  76,  82)) { // Dock
						st = 5;
						_mc[t].m5 = gfx - 76;
					} else if (gfx == 82) {                 // Buoy
						st = 6;
						_mc[t].m5 = gfx - 82;
					} else if (IsInsideMM(gfx,  83, 168)) { // Extended airport
						st = 1;
						_mc[t].m5 = gfx - 83 + 67 - 8;
					} else if (IsInsideMM(gfx, 168, 170)) { // Drive through truck
						st = 2;
						_mc[t].m5 = gfx - 168 + GFX_ROAD_DT_OFFSET;
					} else if (IsInsideMM(gfx, 170, 172)) { // Drive through bus
						st = 3;
						_mc[t].m5 = gfx - 170 + GFX_ROAD_DT_OFFSET;
					} else {
						throw SlCorrupt("Invalid station tile");
					}
					SB(_mc[t].m0, 3, 3, st);
					break;
				}
			}
		}
	}

	/* Before legacy version 81, the density of grass was always stored as zero, and
	 * grassy trees were always drawn fully grassy. Furthermore, trees on rough
	 * land used to have zero density, now they have full density. Therefore,
	 * make all grassy/rough land trees have a density of 3. */
	if (IsOTTDSavegameVersionBefore(stv, 81)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (GetOldTileType(t) == OLD_MP_TREES) {
				uint groundType = GB(_mc[t].m2, 4, 2);
				if (groundType != 2) SB(_mc[t].m2, 6, 2, 3);
			}
		}
	}

	/* The void tiles on the southern border used to belong to a wrong class (pre 4.3).
	 * This problem appears in savegame version 21 too, see r3455. But after loading the
	 * savegame and saving again, the buggy map array could be converted to new savegame
	 * version. It didn't show up before r12070. */
	if (IsOTTDSavegameVersionBefore(stv, 87)) {
		TileIndex t;

		for (t = MapMaxX(); t < map_size - 1; t += MapSizeX()) {
			_mth[t] = OLD_MP_VOID << 4;
			memset(&_mc[t], 0, sizeof(_mc[t]));
		}

		for (t = MapSizeX() * MapMaxY(); t < map_size; t++) {
			_mth[t] = OLD_MP_VOID << 4;
			memset(&_mc[t], 0, sizeof(_mc[t]));
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 114)) {
		bool old_bridge = IsOTTDSavegameVersionBefore(stv, 42);
		bool add_roadtypes = IsOTTDSavegameVersionBefore(stv, 61);

		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				case OLD_MP_CLEAR:
				case OLD_MP_RAILWAY:
				case OLD_MP_WATER:
				case OLD_MP_OBJECT:
					if (old_bridge) SB(_mc[t].m0, 6, 2, 0);
					break;

				case OLD_MP_ROAD:
					if (add_roadtypes) {
						SB(_mc[t].m5, 6, 2, GB(_mc[t].m5, 4, 2));
						SB(_mc[t].m7, 6, 2, 1);
					} else {
						SB(_mc[t].m7, 6, 2, GB(_mc[t].m7, 5, 3));
					}
					SB(_mc[t].m7, 5, 1, GB(_mc[t].m3, 7, 1)); // snow/desert
					switch (GB(_mc[t].m5, 6, 2)) {
						default: throw SlCorrupt("Invalid road tile type");
						case 0:
							if (add_roadtypes) {
								SB(_mc[t].m0, 2, 4, 0);
							} else {
								SB(_mc[t].m5, 0, 4, GB(_mc[t].m4, 0, 4)); // road bits
							}
							SB(_mc[t].m7, 0, 4, GB(_mc[t].m3, 0, 4)); // road works
							SB(_mc[t].m0, 3, 3, GB(_mc[t].m3, 4, 3)); // ground
							SB(_mc[t].m3, 0, 4, add_roadtypes ? 0 : GB(_mc[t].m4, 4, 4)); // tram bits
							SB(_mc[t].m3, 4, 4, GB(_mc[t].m5, 0, 4)); // tram owner
							break;

						case 1:
							SB(_mc[t].m7, 0, 5, GB(_mc[t].m4, 0, 5)); // road owner
							SB(_mc[t].m0, 3, 3, GB(_mc[t].m3, 4, 3)); // ground
							SB(_mc[t].m3, 4, 4, GB(_mc[t].m5, 0, 4)); // tram owner
							SB(_mc[t].m5, 0, 1, add_roadtypes ? GB(_mc[t].m5, 3, 1) : GB(_mc[t].m4, 6, 1)); // road axis
							SB(_mc[t].m5, 5, 1, add_roadtypes ? GB(_mc[t].m5, 2, 1) : GB(_mc[t].m4, 5, 1)); // crossing state
							break;

						case 2:
							break;
					}
					_mc[t].m4 = 0;
					if (old_bridge) SB(_mc[t].m0, 6, 2, 0);
					break;

				case OLD_MP_STATION:
					if (GB(_mc[t].m0, 4, 2) != 1) break;

					SB(_mc[t].m7, 6, 2, add_roadtypes ? 1 : GB(_mc[t].m3, 0, 3));
					SB(_mc[t].m7, 0, 5, HasBit(_mc[t].m0, 2) ? OWNER_TOWN : (Owner)GB(_mc[t].m1, 0, 5));
					SB(_mc[t].m3, 4, 4, _mc[t].m1);
					_mc[t].m4 = 0;
					break;

				case OLD_MP_TUNNELBRIDGE: {
					if (!old_bridge || !HasBit(_mc[t].m5, 7) || !HasBit(_mc[t].m5, 6)) {
						if (((old_bridge && HasBit(_mc[t].m5, 7)) ? GB(_mc[t].m5, 1, 2) : GB(_mc[t].m5, 2, 2)) == 1) {
							/* Middle part of "old" bridges */
							SB(_mc[t].m7, 6, 2, add_roadtypes ? 1 : GB(_mc[t].m3, 0, 3));

							Owner o = (Owner)GB(_mc[t].m1, 0, 5);
							SB(_mc[t].m7, 0, 5, o); // road owner
							SB(_mc[t].m3, 4, 4, o == OWNER_NONE ? OWNER_TOWN : o); // tram owner
						}
						SB(_mc[t].m0, 2, 4, GB(_mc[t].m2, 4, 4)); // bridge type
						SB(_mc[t].m7, 5, 1, GB(_mc[t].m4, 7, 1)); // snow/desert

						_mc[t].m2 = 0;
						_mc[t].m4 = 0;
						if (old_bridge) SB(_mc[t].m0, 6, 2, 0);
					}

					if (!old_bridge || !HasBit(_mc[t].m5, 7)) break;

					Axis axis = (Axis)GB(_mc[t].m5, 0, 1);

					if (HasBit(_mc[t].m5, 6)) { // middle part
						if (HasBit(_mc[t].m5, 5)) { // transport route under bridge?
							if (GB(_mc[t].m5, 3, 2) == 0) {
								SetOldTileType(t, OLD_MP_RAILWAY);
								_mc[t].m2 = 0;
								SB(_mc[t].m3, 4, 4, 0);
								_mc[t].m5 = axis == AXIS_X ? TRACK_BIT_Y : TRACK_BIT_X;
								_mc[t].m4 = _mc[t].m7 = 0;
							} else {
								SetOldTileType(t, OLD_MP_ROAD);
								_mc[t].m2 = INVALID_TOWN;
								_mc[t].m3 = _mc[t].m4 = 0;
								SB(_mc[t].m3, 4, 4, OWNER_TOWN);
								_mc[t].m5 = axis == AXIS_X ? ROAD_Y : ROAD_X;
								_mc[t].m7 = 1 << 6;
							}
						} else if (GB(_mc[t].m5, 3, 2) == 0) {
							SetOldTileType(t, OLD_MP_CLEAR);
							_mc[t].m1 = OWNER_NONE;
							_mc[t].m2 = 0;
							_mc[t].m3 = _mc[t].m4 = _mc[t].m7 = 0;
							_mc[t].m5 = 3;
						} else if (!IsOldTileFlat(t)) {
							SetOldTileType(t, OLD_MP_WATER);
							SB(_mc[t].m1, 0, 5, OWNER_WATER);
							SB(_mc[t].m1, 5, 2, WATER_CLASS_SEA);
							_mc[t].m2 = 0;
							_mc[t].m3 = _mc[t].m4 = _mc[t].m7 = 0;
							_mc[t].m5 = 1;
						} else if (GB(_mc[t].m1, 0, 5) == OWNER_WATER) {
							SetOldTileType(t, OLD_MP_WATER);
							SB(_mc[t].m1, 0, 5, OWNER_WATER);
							SB(_mc[t].m1, 5, 2, WATER_CLASS_SEA);
							_mc[t].m2 = 0;
							_mc[t].m3 = _mc[t].m4 = _mc[t].m5 = _mc[t].m7 = 0;
						} else {
							SetOldTileType(t, OLD_MP_WATER);
							SB(_mc[t].m1, 5, 2, WATER_CLASS_CANAL);
							_mc[t].m2 = 0;
							_mc[t].m3 = _mc[t].m5 = _mc[t].m7 = 0;
							_mc[t].m4 = Random();
						}
						SB(_mc[t].m0, 2, 6, (1 << 4) << axis);
					} else { // ramp
						DiagDirection dir = AxisToDiagDir(axis);
						if (HasBit(_mc[t].m5, 5)) dir = ReverseDiagDir(dir);
						TransportType type = (TransportType)GB(_mc[t].m5, 1, 2);

						_mc[t].m5 = 1 << 7 | type << 2 | dir;
					}

					break;
				}

				default: break;
			}
		}
	}

	/* From legacy version 82, old style canals (above sealevel (0), WATER owner) are no longer supported.
	    Replace the owner for those by OWNER_NONE. */
	if (IsOTTDSavegameVersionBefore(stv, 82)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsOldTileType(t, OLD_MP_WATER) &&
					_mc[t].m5 == 0 &&
					GB(_mc[t].m1, 0, 5) == OWNER_WATER &&
					OldTileHeight(t) != 0) {
				SB(_mc[t].m1, 0, 5, OWNER_NONE);
			}
		}
	}

	/*
	 * Add the 'previous' owner to the ship depots so we can reset it with
	 * the correct values when it gets destroyed. This prevents that
	 * someone can remove canals owned by somebody else and it prevents
	 * making floods using the removal of ship depots.
	 */
	if (IsOTTDSavegameVersionBefore(stv, 83)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsOldTileType(t, OLD_MP_WATER) && GB(_mc[t].m5, 4, 4) == 8) {
				_mc[t].m4 = (OldTileHeight(t) == 0) ? OWNER_WATER : OWNER_NONE;
			}
		}
	}

	/* The water class was moved/unified. */
	if (IsOTTDSavegameVersionBefore(stv, 146)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				case OLD_MP_STATION:
					switch (GB(_mc[t].m0, 3, 3)) {
						case 4:
						case 5:
						case 6:
							SB(_mc[t].m1, 5, 2, GB(_mc[t].m3, 0, 2));
							SB(_mc[t].m3, 0, 2, 0);
							break;

						default:
							SB(_mc[t].m1, 5, 2, WATER_CLASS_INVALID);
							break;
					}
					break;

				case OLD_MP_WATER:
					SB(_mc[t].m1, 5, 2, GB(_mc[t].m3, 0, 2));
					SB(_mc[t].m3, 0, 2, 0);
					break;

				case OLD_MP_OBJECT:
					SB(_mc[t].m1, 5, 2, WATER_CLASS_INVALID);
					break;

				default:
					/* No water class. */
					break;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 86)) {
		for (TileIndex t = 0; t < map_size; t++) {
			/* Move river flag and update canals to use water class */
			if (IsOldTileType(t, OLD_MP_WATER) && GB(_mc[t].m1, 5, 2) != WATER_CLASS_RIVER) {
				if (_mc[t].m5 == 0) {
					if (GB(_mc[t].m1, 0, 5) == OWNER_WATER) {
						SB(_mc[t].m1, 5, 2, WATER_CLASS_SEA);
						_mc[t].m4 = 0;
					} else {
						SB(_mc[t].m1, 5, 2, WATER_CLASS_CANAL);
						_mc[t].m4 = Random();
					}
					_mc[t].m2 = 0;
					_mc[t].m3 = _mc[t].m5 = _mc[t].m7 = 0;
					SB(_mc[t].m0, 2, 4, 0);
				} else if (GB(_mc[t].m5, 4, 4) == 8) {
					Owner o = (Owner)_mc[t].m4; // Original water owner
					SB(_mc[t].m1, 5, 2, o == OWNER_WATER ? WATER_CLASS_SEA : WATER_CLASS_CANAL);
				}
			}
		}
	}

	/* Move the signal variant back up one bit for PBS. We don't convert the old PBS
	 * format here, as an old layout wouldn't work properly anyway. To be safe, we
	 * clear any possible PBS reservations as well. */
	if (IsOTTDSavegameVersionBefore(stv, 100)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				case OLD_MP_RAILWAY:
					if (GB(_mc[t].m5, 6, 2) == 1) {
						/* move the signal variant */
						SB(_mc[t].m2, 3, 1, HasBit(_mc[t].m2, 2) ? SIG_SEMAPHORE : SIG_ELECTRIC);
						SB(_mc[t].m2, 7, 1, HasBit(_mc[t].m2, 6) ? SIG_SEMAPHORE : SIG_ELECTRIC);
						ClrBit(_mc[t].m2, 2);
						ClrBit(_mc[t].m2, 6);
					}

					/* Clear PBS reservation on track */
					if (GB(_mc[t].m5, 6, 2) == 3) {
						ClrBit(_mc[t].m5, 4);
					} else {
						SB(_mc[t].m2, 8, 4, 0);
					}
					break;

				case OLD_MP_ROAD: // Clear PBS reservation on crossing
					if (GB(_mc[t].m5, 6, 2) == 1) ClrBit(_mc[t].m5, 4);
					break;

				case OLD_MP_STATION: // Clear PBS reservation on station
					if (GB(_mc[t].m0, 3, 3) == 0 || GB(_mc[t].m0, 3, 3) == 7) ClrBit(_mc[t].m0, 2);
					break;

				case OLD_MP_TUNNELBRIDGE: // Clear PBS reservation on tunnels/bridges
					if (GB(_mc[t].m5, 2, 2) == 0) ClrBit(_mc[t].m5, 4);
					break;

				default: break;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 112)) {
		for (TileIndex t = 0; t < map_size; t++) {
			/* Check for HQ bit being set, instead of using map accessor,
			 * since we've already changed it code-wise */
			if (IsOldTileType(t, OLD_MP_OBJECT) && HasBit(_mc[t].m5, 7)) {
				/* Move size and part identification of HQ out of the m5 attribute,
				 * on new locations */
				_mc[t].m3 = GB(_mc[t].m5, 0, 5);
				_mc[t].m5 = 4;
			}
		}
	}

	/* The bits for the tree ground and tree density have
	 * been swapped (m2 bits 7..6 and 5..4. */
	if (IsOTTDSavegameVersionBefore(stv, 135)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsOldTileType(t, OLD_MP_CLEAR)) {
				if (GB(_mc[t].m5, 2, 3) == 4) {
					SB(_mc[t].m5, 2, 6, 0);
					SetBit(_mc[t].m3, 4);
				} else {
					ClrBit(_mc[t].m3, 4);
				}
			} else if (IsOldTileType(t, OLD_MP_TREES)) {
				uint density = GB(_mc[t].m2, 6, 2);
				uint ground  = GB(_mc[t].m2, 4, 2);
				uint counter = GB(_mc[t].m2, 0, 4);
				_mc[t].m2 = ground << 6 | density << 4 | counter;
			}
		}
	}

	/* Reset tropic zone for VOID tiles, they shall not have any. */
	if (IsOTTDSavegameVersionBefore(stv, 141)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsOldTileType(t, OLD_MP_VOID)) SB(_mc[t].m0, 0, 2, 0);
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 144)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsOldTileType(t, OLD_MP_OBJECT)) continue;

			/* Reordering/generalisation of the object bits. */
			bool is_hq = _mc[t].m5 == 4;
			SB(_mc[t].m0, 2, 4, is_hq ? GB(_mc[t].m3, 2, 3) : 0);
			_mc[t].m3 = is_hq ? GB(_mc[t].m3, 1, 1) | GB(_mc[t].m3, 0, 1) << 4 : 0;

			/* Make sure those bits are clear as well! */
			_mc[t].m4 = 0;
			_mc[t].m7 = 0;
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 147)) {
		/* Move the animation frame to the same location (m7) for all objects. */
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetOldTileType(t)) {
				case OLD_MP_HOUSE:
					/* This needs GRF knowledge, so it is done in AfterLoadGame */
					break;

				case OLD_MP_INDUSTRY:
					Swap(_mc[t].m3, _mc[t].m7);
					break;

				case OLD_MP_OBJECT:
					/* hack: temporarily store offset in m4;
					 * it will be used (and removed) in AfterLoadGame */
					_mc[t].m4 = _mc[t].m3;

					/* move the animation state. */
					_mc[t].m7 = GB(_mc[t].m0, 2, 4);
					SB(_mc[t].m0, 2, 4, 0);
					_mc[t].m3 = 0;
					break;

				default:
					/* For stations/airports it's already at m7 */
					break;
			}
		}
	}

	if (IsOTTDSavegameVersionBefore(stv, 164)) {
		/* We store 4 fences in the field tiles instead of only SE and SW. */
		for (TileIndex t = 0; t < map_size; t++) {
			if (!IsOldTileType(t, OLD_MP_CLEAR) && !IsOldTileType(t, OLD_MP_TREES)) continue;
			if (IsOldTileType(t, OLD_MP_CLEAR) && !HasBit(_mc[t].m3, 4) && GB(_mc[t].m5, 2, 3) == 3) continue;

			uint fence = GB(_mc[t].m4, 5, 3);
			if (fence != 0) {
				TileIndex neighbour = TILE_ADDXY(t, 1, 0);
				if (IsOldTileType(neighbour, OLD_MP_CLEAR) && !HasBit(_mc[neighbour].m3, 4) && GB(_mc[neighbour].m5, 2, 3) == 3) {
					SB(_mc[neighbour].m3, 5, 3, fence);
				}
			}

			fence = GB(_mc[t].m4, 2, 3);
			if (fence != 0) {
				TileIndex neighbour = TILE_ADDXY(t, 0, 1);
				if (IsOldTileType(neighbour, OLD_MP_CLEAR) && !HasBit(_mc[neighbour].m3, 4) && GB(_mc[neighbour].m5, 2, 3) == 3) {
					SB(_mc[neighbour].m0, 2, 3, fence);
				}
			}

			SB(_mc[t].m4, 2, 3, 0);
			SB(_mc[t].m4, 5, 3, 0);
		}
	}

	/* Switch to the new map array */
	if (IsFullSavegameVersionBefore(stv, 1)) {
		for (TileIndex t = 0; t < map_size; t++) {
			uint zone = GB(_mc[t].m0, 0, 2);

			switch (GetOldTileType(t)) {
				case OLD_MP_CLEAR: {
					uint fence_nw = GB(_mc[t].m0, 2, 3);
					_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_GROUND << 4);
					uint ground = GB(_mc[t].m5, 2, 3);
					if (ground == 3) {
						SB(_mc[t].m1, 6, 2, TT_GROUND_FIELDS);
						uint counter = GB(_mc[t].m5, 5, 3);
						SB(_mc[t].m5, 2, 3, fence_nw);
						SB(_mc[t].m5, 5, 3, GB(_mc[t].m3, 5, 3));
						SB(_mc[t].m3, 4, 4, GB(_mc[t].m3, 0, 4));
						SB(_mc[t].m3, 0, 4, counter);
					} else {
						SB(_mc[t].m1, 6, 2, TT_GROUND_CLEAR);
						_mc[t].m4 = GB(_mc[t].m5, 0, 2);
						if (HasBit(_mc[t].m3, 4)) {
							switch (ground) {
								case 1:  ground = GROUND_SNOW_ROUGH; break;
								case 2:  ground = GROUND_SNOW_ROCKS; break;
								default: ground = GROUND_SNOW;       break;
							}
						} else {
							switch (ground) {
								default: ground = GROUND_GRASS;  break;
								case 1:  ground = GROUND_ROUGH;  break;
								case 2:  ground = GROUND_ROCKS;  break;
								case 4:  ground = GROUND_SNOW;   break;
								case 5:  ground = GROUND_DESERT; break;
							}
						}
						SB(_mc[t].m3, 4, 4, ground);
						SB(_mc[t].m3, 0, 4, GB(_mc[t].m5, 5, 3));
						_mc[t].m5 = 0;
					}
					_mc[t].m7 = 0;
					break;
				}

				case OLD_MP_RAILWAY: {
					uint ground = GB(_mc[t].m4, 0, 4);
					if (!HasBit(_mc[t].m5, 7)) { // track
						_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_RAILWAY << 4);
						SB(_mc[t].m1, 6, 2, TT_TRACK);
						if (HasBit(_mc[t].m5, 6)) { // with signals
							_mc[t].m7 = GB(_mc[t].m4, 4, 2) | (GB(_mc[t].m3, 4, 2) << 2) | (GB(_mc[t].m2, 4, 3) << 4) | (GB(_mc[t].m2, 7, 1) << 7);
							_mc[t].m4 = GB(_mc[t].m4, 6, 2) | (GB(_mc[t].m3, 6, 2) << 2) | (GB(_mc[t].m2, 0, 3) << 4) | (GB(_mc[t].m2, 3, 1) << 7);
						} else {
							_mc[t].m4 = _mc[t].m7 = 0;
						}
						SB(_mc[t].m3, 4, 4, ground);
						SB(_mc[t].m2, 0, 8, GB(_mc[t].m5, 0, 6));
					} else if (HasBit(_mc[t].m5, 6)) { // depot
						_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_MISC << 4);
						SB(_mc[t].m1, 6, 2, TT_MISC_DEPOT);
						ClrBit(_mc[t].m1, 5);
						SB(_mc[t].m3, 4, 4, ground == 12 ? 1 : 0);
						_mc[t].m5 &= 0x13;
						_mc[t].m4 = _mc[t].m7 = 0;
					} else { // old waypoint
						if (!IsOTTDSavegameVersionBefore(stv, 123)) {
							throw SlCorrupt("Invalid rail tile type");
						}
						/* temporary hack; AfterLoadGame will fix this */
						_mc[t].m0 = GB(_mc[t].m5, 4, 1) | (STATION_WAYPOINT << 1) | (TT_STATION << 4);
					}
					break;
				}

				case OLD_MP_ROAD: {
					uint roadside = GB(_mc[t].m0, 3, 3);
					_mc[t].m0 = GB(_mc[t].m0, 6, 2);
					switch (GB(_mc[t].m5, 6, 2)) {
						case 0: { // normal road
							_mc[t].m0 |= (TT_ROAD << 4);
							SB(_mc[t].m1, 6, 2, TT_TRACK);
							_mc[t].m4 = GB(_mc[t].m5, 0, 4) | (GB(_mc[t].m3, 0, 4) << 4);
							SB(_mc[t].m5, 0, 4, GB(_mc[t].m3, 4, 4));
							_mc[t].m3 = (GB(_mc[t].m5, 4, 2) << 6) | (GB(_mc[t].m7, 5, 1) << 4);
							SB(_mc[t].m5, 4, 4, roadside);
							break;
						}

						case 1: // level crossing
							_mc[t].m0 |= (TT_MISC << 4);
							SB(_mc[t].m1, 6, 2, TT_MISC_CROSSING);
							SB(_mc[t].m3, 4, 4, GB(_mc[t].m7, 5, 1));
							SB(_mc[t].m4, 5, 1, GB(_mc[t].m5, 0, 1));
							SB(_mc[t].m4, 6, 1, GB(_mc[t].m5, 5, 1));
							SB(_mc[t].m4, 7, 1, GB(_mc[t].m5, 4, 1));
							_mc[t].m5 = GB(_mc[t].m3, 4, 4) | (roadside << 4);
							break;

						case 2: // road depot
							_mc[t].m0 |= (TT_MISC << 4);
							SB(_mc[t].m1, 6, 2, TT_MISC_DEPOT);
							SetBit(_mc[t].m1, 5);
							_mc[t].m3 = GB(_mc[t].m7, 5, 1) << 4;
							_mc[t].m4 = 0;
							_mc[t].m5 &= 0x03;
							_mc[t].m7 &= 0xE0;
							break;
					}
					break;
				}

				case OLD_MP_HOUSE: {
					uint random = _mc[t].m1;
					_mc[t].m1 = GB(_mc[t].m0, 2, 6) | (_mc[t].m3 & 0xC0);
					_mc[t].m0 = GB(_mc[t].m3, 0, 6) | 0xC0;
					_mc[t].m3 = random;
					break;
				}

				case OLD_MP_TREES:
					_mc[t].m0 = TT_GROUND << 4;
					SB(_mc[t].m1, 6, 2, TT_GROUND_TREES);
					_mc[t].m7 = _mc[t].m3;
					switch (GB(_mc[t].m2, 6, 3)) {
						case 0: _mc[t].m3 = GROUND_GRASS << 4; break;
						case 1: _mc[t].m3 = GROUND_ROUGH << 4; break;
						case 2: _mc[t].m3 = _settings_game.game_creation.landscape == LT_TROPIC ? GROUND_DESERT << 4 : GROUND_SNOW << 4; break;
						case 3: _mc[t].m3 = GROUND_SHORE << 4; break;
						case 4: _mc[t].m3 = GROUND_SNOW_ROUGH << 4; break;
					}
					SB(_mc[t].m3, 0, 4, GB(_mc[t].m2, 0, 4));
					_mc[t].m4 = GB(_mc[t].m2, 4, 2);
					_mc[t].m2 = 0;
					break;

				case OLD_MP_STATION: {
					uint type = GB(_mc[t].m0, 2, 4);
					if ((type == STATION_WAYPOINT) && IsOTTDSavegameVersionBefore(stv, 123)) {
						throw SlCorrupt("Invalid station type");
					}
					_mc[t].m0 = type | (TT_STATION << 4);
					break;
				}

				case OLD_MP_WATER:
					_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_WATER << 4);
					_mc[t].m3 = _mc[t].m4;
					_mc[t].m4 = 0;
					break;

				case OLD_MP_VOID:
					_mc[t].m0 = TT_GROUND << 4;
					_mc[t].m1 = TT_GROUND_VOID << 6;
					_mc[t].m2 = 0;
					_mc[t].m3 = _mc[t].m4 = _mc[t].m5 = _mc[t].m7 = 0;
					break;

				case OLD_MP_INDUSTRY:
					_mc[t].m0 = GB(_mc[t].m0, 3, 3) | (GB(_mc[t].m0, 2, 1) << 3) | 0x80;
					break;

				case OLD_MP_TUNNELBRIDGE:
					if (HasBit(_mc[t].m5, 7)) { // bridge
						uint type = GB(_mc[t].m0, 2, 4);
						switch (GB(_mc[t].m5, 2, 2)) {
							case 0: // rail
								_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_RAILWAY << 4);
								SB(_mc[t].m1, 6, 2, TT_BRIDGE);
								SB(_mc[t].m2, 12, 4, type);
								SB(_mc[t].m3, 4, 2, GB(_mc[t].m7, 5, 1));
								SB(_mc[t].m3, 6, 2, GB(_mc[t].m5, 0, 2));
								break;
							case 1: { // road
								_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_ROAD << 4);
								SB(_mc[t].m1, 6, 2, TT_BRIDGE);
								if (HasBit(_mc[t].m7, 6)) SB(_mc[t].m1, 0, 5, GB(_mc[t].m7, 0, 5));
								uint tram = GB(_mc[t].m3, 4, 4);
								_mc[t].m3 = (GB(_mc[t].m5, 0, 2) << 6) | (GB(_mc[t].m7, 5, 1) << 4);
								_mc[t].m5 = tram;
								SB(_mc[t].m7, 0, 4, type);
								} break;
							case 2: // aqueduct
								_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_MISC << 4);
								SB(_mc[t].m1, 6, 2, TT_MISC_AQUEDUCT);
								_mc[t].m3 = (GB(_mc[t].m5, 0, 2) << 6) | (GB(_mc[t].m7, 5, 1) << 4);
								_mc[t].m5 = 0;
								break;
							default:
								throw SlCorrupt("Invalid bridge transport type");
						}
					} else { // tunnel
						_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_MISC << 4);
						SB(_mc[t].m1, 6, 2, TT_MISC_TUNNEL);
						uint tram = GB(_mc[t].m3, 4, 4);
						SB(_mc[t].m3, 4, 2, GB(_mc[t].m7, 5, 1));
						SB(_mc[t].m3, 6, 2, GB(_mc[t].m5, 0, 2));
						SB(_mc[t].m5, 6, 2, GB(_mc[t].m5, 2, 2));
						SB(_mc[t].m5, 0, 4, tram);
					}
					break;

				case OLD_MP_OBJECT:
					_mc[t].m0 = GB(_mc[t].m0, 6, 2) | (TT_OBJECT << 4);
					break;

				default:
					throw SlCorrupt("Invalid tile type");
			}

			SB(_mth[t], 4, 4, zone << 2);
		}
	}

	/* Add second railtype to rail tiles */
	if (IsFullSavegameVersionBefore(stv, 3)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileTypeSubtype(t, TT_RAILWAY, TT_TRACK)) {
				SB(_mc[t].m5, 0, 4, GB(_mc[t].m3, 0, 4));
			}
		}
	}

	/* Add road layout to road bridgeheads */
	if (IsFullSavegameVersionBefore(stv, 7)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileTypeSubtype(t, TT_ROAD, TT_BRIDGE)) {
				RoadBits bits = AxisToRoadBits(DiagDirToAxis((DiagDirection)GB(_mc[t].m3, 6, 2)));
				SB(_mc[t].m4, 0, 4, HasBit(_mc[t].m7, 6) ? bits : 0);
				SB(_mc[t].m4, 4, 4, HasBit(_mc[t].m7, 7) ? bits : 0);
			}
		}
	}

	/* Add track layout to rail bridgeheads */
	if (IsFullSavegameVersionBefore(stv, 8)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileTypeSubtype(t, TT_RAILWAY, TT_BRIDGE)) {
				Track track = DiagDirToDiagTrack((DiagDirection)GB(_mc[t].m3, 6, 2));
				bool reserved = HasBit(_mc[t].m5, 4);
				SB(_mc[t].m2, 0, 6, TrackToTrackBits(track));
				SB(_mc[t].m2, 6, 1, reserved ? 1 : 0);
				SB(_mc[t].m2, 8, 4, reserved ? (track + 1) : 0);
				SB(_mc[t].m5, 4, 4, 0);
				_mc[t].m4 = _mc[t].m7 = 0;
			}
		}
	}

	/* Split tunnelhead/tunnel PBS reservation */
	if (IsFullSavegameVersionBefore(stv, 9)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileTypeSubtype(t, TT_MISC, TT_MISC_TUNNEL) && GB(_mc[t].m5, 6, 2) == 0) {
				if (HasBit(_mc[t].m5, 4)) {
					SetBit(_mc[t].m5, 5);
				} else {
					ClrBit(_mc[t].m5, 5);
				}
			}
		}
	}

	/* Roadworks now count down, not up */
	if (IsFullSavegameVersionBefore(stv, 12)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileTypeSubtype(t, TT_ROAD, TT_TRACK)) {
				uint roadside = GB(_mc[t].m5, 4, 3);
				if (roadside > 5) {
					SB(_mc[t].m5, 4, 3, roadside - 5);
					SB(_mc[t].m7, 0, 4, 0xF - GB(_mc[t].m7, 0, 4));
				}
			}
		}
	}

	/* Store direction for ship depots */
	if (IsFullSavegameVersionBefore(stv, 14)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileType(t, TT_WATER) && GB(_mc[t].m5, 4, 4) == 8) {
				DiagDirection dir = AxisToDiagDir((Axis)GB(_mc[t].m5, 1, 1));
				SB(_mc[t].m5, 0, 2, HasBit(_mc[t].m5, 0) ? dir : ReverseDiagDir(dir));
			}
		}
	}
}


struct MapDim {
	uint32 x, y;
};

static const SaveLoad _map_dimensions[] = {
	SLE_VAR(MapDim, x, SLE_UINT32, 0, , 6, ),
	SLE_VAR(MapDim, y, SLE_UINT32, 0, , 6, ),
	SLE_END()
};

static void Save_MAPS(SaveDumper *dumper)
{
	MapDim map_dim;
	map_dim.x = MapSizeX();
	map_dim.y = MapSizeY();
	dumper->WriteRIFFObject(&map_dim, _map_dimensions);
}

static void Load_MAPS(LoadBuffer *reader)
{
	MapDim map_dim;
	reader->ReadObject(&map_dim, _map_dimensions);
	AllocateMap(map_dim.x, map_dim.y);
}

static void Check_MAPS(LoadBuffer *reader)
{
	MapDim map_dim;
	reader->ReadObject(&map_dim, _map_dimensions);
	_load_check_data.map_size_x = map_dim.x;
	_load_check_data.map_size_y = map_dim.y;
}

static const uint MAP_SL_BUF_SIZE = 4096;

static void Load_MAPT(LoadBuffer *reader)
{
	reader->CopyBytes(_mth, MapSize());
}

static void Save_MAPT(SaveDumper *dumper)
{
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	dumper->CopyBytes(_mth, size);
}

static void Load_MAP1(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _mc[i++].m1 = buf[j];
	}
}

static void Save_MAP1(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _mc[i++].m1;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP2(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<uint16, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE,
			/* In those versions the m2 was 8 bits */
			reader->IsOTTDVersionBefore(5) ? SLE_FILE_U8 | SLE_VAR_U16 : SLE_UINT16
		);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _mc[i++].m2 = buf[j];
	}
}

static void Save_MAP2(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<uint16, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size * sizeof(uint16));
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _mc[i++].m2;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT16);
	}
}

static void Load_MAP3(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _mc[i++].m3 = buf[j];
	}
}

static void Save_MAP3(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _mc[i++].m3;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP4(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _mc[i++].m4 = buf[j];
	}
}

static void Save_MAP4(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _mc[i++].m4;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP5(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _mc[i++].m5 = buf[j];
	}
}

static void Save_MAP5(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _mc[i++].m5;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP0(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	if (reader->IsOTTDVersionBefore(42)) {
		for (TileIndex i = 0; i != size;) {
			/* 1024, otherwise we overflow on 64x64 maps! */
			reader->ReadArray(buf, 1024, SLE_UINT8);
			for (uint j = 0; j != 1024; j++) {
				_mc[i++].m0 = GB(buf[j], 0, 2);
				_mc[i++].m0 = GB(buf[j], 2, 2);
				_mc[i++].m0 = GB(buf[j], 4, 2);
				_mc[i++].m0 = GB(buf[j], 6, 2);
			}
		}
	} else {
		for (TileIndex i = 0; i != size;) {
			reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
			for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _mc[i++].m0 = buf[j];
		}
	}
}

static void Save_MAP0(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _mc[i++].m0;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP7(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _mc[i++].m7 = buf[j];
	}
}

static void Save_MAP7(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _mc[i++].m7;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

extern const ChunkHandler _map_chunk_handlers[] = {
	{ 'MAPS', Save_MAPS, Load_MAPS, NULL, Check_MAPS, CH_RIFF },
	{ 'MAPT', Save_MAPT, Load_MAPT, NULL, NULL,       CH_RIFF },
	{ 'MAPO', Save_MAP1, Load_MAP1, NULL, NULL,       CH_RIFF },
	{ 'MAP2', Save_MAP2, Load_MAP2, NULL, NULL,       CH_RIFF },
	{ 'M3LO', Save_MAP3, Load_MAP3, NULL, NULL,       CH_RIFF },
	{ 'M3HI', Save_MAP4, Load_MAP4, NULL, NULL,       CH_RIFF },
	{ 'MAP5', Save_MAP5, Load_MAP5, NULL, NULL,       CH_RIFF },
	{ 'MAPE', Save_MAP0, Load_MAP0, NULL, NULL,       CH_RIFF },
	{ 'MAP7', Save_MAP7, Load_MAP7, NULL, NULL,       CH_RIFF | CH_LAST },
};
