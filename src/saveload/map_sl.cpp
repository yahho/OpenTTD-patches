/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map_sl.cpp Code handling saving and loading of map */

#include "../stdafx.h"
#include "../map_func.h"
#include "../core/bitmath_func.hpp"
#include "../core/random_func.hpp"
#include "../fios.h"
#include "../tile_map.h"
#include "../station_map.h"
#include "../town.h"
#include "../water_map.h"

#include "saveload_buffer.h"


/**
 *  Fix map array after loading an old savegame
 */
void AfterLoadMap(const SavegameTypeVersion *stv)
{
	TileIndex map_size = MapSize();

	/* in version 2.1 of the savegame, town owner was unified. */
	if (IsSavegameVersionBefore(stv, 2, 1)) {
		for (TileIndex tile = 0; tile < map_size; tile++) {
			switch (GetTileType(tile)) {
				case MP_ROAD:
					if (GB(_m[tile].m5, 4, 2) == 1 && HasBit(_m[tile].m3, 7)) {
						_m[tile].m3 = OWNER_TOWN;
					}
					/* FALL THROUGH */

				case MP_TUNNELBRIDGE:
					if (_m[tile].m1 & 0x80) SB(_m[tile].m1, 0, 5, OWNER_TOWN);
					break;

				default: break;
			}
		}
	}

	/* In version 6.1 we put the town index in the map-array. To do this, we need
	 *  to use m2 (16bit big), so we need to clean m2, and that is where this is
	 *  all about ;) */
	if (IsSavegameVersionBefore(stv, 6, 1)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				case MP_HOUSE:
					_m[t].m4 = _m[t].m2;
					_m[t].m2 = 0;
					break;

				case MP_ROAD:
					_m[t].m4 |= (_m[t].m2 << 4);
					_m[t].m2 = 0;
					break;

				default: break;
			}
		}
	}

	/* From version 15, we moved a semaphore bit from bit 2 to bit 3 in m4, making
	 *  room for PBS. Now in version 21 move it back :P. */
	if (IsSavegameVersionBefore(stv, 21) && !IsSavegameVersionBefore(stv, 15)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				case MP_RAILWAY:
					if (GB(_m[t].m5, 6, 2) == 1) {
						/* convert PBS signals to combo-signals */
						if (HasBit(_m[t].m2, 2)) SB(_m[t].m2, 0, 3, 3);

						/* move the signal variant back */
						SB(_m[t].m2, 3, 1, HasBit(_m[t].m2, 3) ? SIG_SEMAPHORE : SIG_ELECTRIC);
						ClrBit(_m[t].m2, 3);
					}

					/* Clear PBS reservation on track */
					if (GB(_m[t].m5, 6, 2) != 3) {
						SB(_m[t].m4, 4, 4, 0);
					} else {
						ClrBit(_m[t].m3, 6);
					}
					break;

				case MP_STATION: // Clear PBS reservation on station
					ClrBit(_m[t].m3, 6);
					break;

				default: break;
			}
		}
	}

	if (IsSavegameVersionBefore(stv, 48)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				case MP_RAILWAY:
					if (!HasBit(_m[t].m5, 7)) {
						/* Swap ground type and signal type for plain rail tiles, so the
						 * ground type uses the same bits as for depots and waypoints. */
						uint tmp = GB(_m[t].m4, 0, 4);
						SB(_m[t].m4, 0, 4, GB(_m[t].m2, 0, 4));
						SB(_m[t].m2, 0, 4, tmp);
					} else if (HasBit(_m[t].m5, 2)) {
						/* Split waypoint and depot rail type and remove the subtype. */
						ClrBit(_m[t].m5, 2);
						ClrBit(_m[t].m5, 6);
					}
					break;

				case MP_ROAD:
					/* Swap m3 and m4, so the track type for rail crossings is the
					 * same as for normal rail. */
					Swap(_m[t].m3, _m[t].m4);
					break;

				default: break;
			}
		}
	}

	/* From version 53, the map array was changed for house tiles to allow
	 * space for newhouses grf features. A new byte, m7, was also added. */
	if (IsSavegameVersionBefore(stv, 53)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileType(t, MP_HOUSE)) {
				if (GB(_m[t].m3, 6, 2) != TOWN_HOUSE_COMPLETED) {
					/* Move the construction stage from m3[7..6] to m5[5..4].
					 * The construction counter does not have to move. */
					SB(_m[t].m5, 3, 2, GB(_m[t].m3, 6, 2));
					SB(_m[t].m3, 6, 2, 0);
				} else {
					/* The "lift has destination" bit has been moved from
					 * m5[7] to m7[0]. */
					SB(_me[t].m7, 0, 1, HasBit(_m[t].m5, 7));
					ClrBit(_m[t].m5, 7);

					/* The "lift is moving" bit has been removed, as it does
					 * the same job as the "lift has destination" bit. */
					ClrBit(_m[t].m1, 7);

					/* The position of the lift goes from m1[7..0] to m6[7..2],
					 * making m1 totally free, now. The lift position does not
					 * have to be a full byte since the maximum value is 36. */
					SB(_m[t].m6, 2, 6, GB(_m[t].m1, 0, 6 ));

					_m[t].m1 = 0;
					_m[t].m3 = 0x80;
				}
			}
		}
	}

	if (IsSavegameVersionBefore(stv, 64)) {
		/* copy the signal type/variant and move signal states bits */
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileType(t, MP_RAILWAY) && GB(_m[t].m5, 6, 2) == 1) {
				SB(_m[t].m4, 4, 4, GB(_m[t].m2, 4, 4));
				SB(_m[t].m2, 7, 1, GB(_m[t].m2, 3, 1));
				SB(_m[t].m2, 4, 3, GB(_m[t].m2, 0, 3));
				ClrBit(_m[t].m2, 7);
			}
		}
	}

	if (IsSavegameVersionBefore(stv, 72)) {
		/* Locks in very old savegames had OWNER_WATER as owner */
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				default: break;

				case MP_WATER:
					if (GB(_m[t].m5, 4, 4) == 1 && GB(_m[t].m1, 0, 5) == OWNER_WATER) SB(_m[t].m1, 0, 5, OWNER_NONE);
					break;

				case MP_STATION: {
					if (HasBit(_m[t].m6, 3)) SetBit(_m[t].m6, 2);
					byte gfx = _m[t].m5;
					int st;
					if (       IsInsideMM(gfx,   0,   8)) { // Rail station
						st = 0;
						_m[t].m5 = gfx - 0;
					} else if (IsInsideMM(gfx,   8,  67)) { // Airport
						st = 1;
						_m[t].m5 = gfx - 8;
					} else if (IsInsideMM(gfx,  67,  71)) { // Truck
						st = 2;
						_m[t].m5 = gfx - 67;
					} else if (IsInsideMM(gfx,  71,  75)) { // Bus
						st = 3;
						_m[t].m5 = gfx - 71;
					} else if (gfx == 75) {                 // Oil rig
						st = 4;
						_m[t].m5 = gfx - 75;
					} else if (IsInsideMM(gfx,  76,  82)) { // Dock
						st = 5;
						_m[t].m5 = gfx - 76;
					} else if (gfx == 82) {                 // Buoy
						st = 6;
						_m[t].m5 = gfx - 82;
					} else if (IsInsideMM(gfx,  83, 168)) { // Extended airport
						st = 1;
						_m[t].m5 = gfx - 83 + 67 - 8;
					} else if (IsInsideMM(gfx, 168, 170)) { // Drive through truck
						st = 2;
						_m[t].m5 = gfx - 168 + GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET;
					} else if (IsInsideMM(gfx, 170, 172)) { // Drive through bus
						st = 3;
						_m[t].m5 = gfx - 170 + GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET;
					} else {
						throw SlCorrupt("Invalid station tile");
					}
					SB(_m[t].m6, 3, 3, st);
					break;
				}
			}
		}
	}

	/* Before version 81, the density of grass was always stored as zero, and
	 * grassy trees were always drawn fully grassy. Furthermore, trees on rough
	 * land used to have zero density, now they have full density. Therefore,
	 * make all grassy/rough land trees have a density of 3. */
	if (IsSavegameVersionBefore(stv, 81)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (GetTileType(t) == MP_TREES) {
				uint groundType = GB(_m[t].m2, 4, 2);
				if (groundType != 2) SB(_m[t].m2, 6, 2, 3);
			}
		}
	}

	/* The void tiles on the southern border used to belong to a wrong class (pre 4.3).
	 * This problem appears in savegame version 21 too, see r3455. But after loading the
	 * savegame and saving again, the buggy map array could be converted to new savegame
	 * version. It didn't show up before r12070. */
	if (IsSavegameVersionBefore(stv, 87)) {
		static const Tile voidtile = { MP_VOID << 4, 0, 0, 0, 0, 0, 0 };
		TileIndex t;

		for (t = MapMaxX(); t < map_size - 1; t += MapSizeX()) {
			_m[t] = voidtile;
			_me[t].m7 = 0;
		}

		for (t = MapSizeX() * MapMaxY(); t < map_size; t++) {
			_m[t] = voidtile;
			_me[t].m7 = 0;
		}
	}

	if (IsSavegameVersionBefore(stv, 114)) {
		bool old_bridge = IsSavegameVersionBefore(stv, 42);
		bool add_roadtypes = IsSavegameVersionBefore(stv, 61);

		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				case MP_CLEAR:
				case MP_RAILWAY:
				case MP_WATER:
				case MP_OBJECT:
					if (old_bridge) SB(_m[t].m6, 6, 2, 0);
					break;

				case MP_ROAD:
					if (add_roadtypes) {
						SB(_m[t].m5, 6, 2, GB(_m[t].m5, 4, 2));
						SB(_me[t].m7, 6, 2, 1);
					} else {
						SB(_me[t].m7, 6, 2, GB(_me[t].m7, 5, 3));
					}
					SB(_me[t].m7, 5, 1, GB(_m[t].m3, 7, 1)); // snow/desert
					switch (GB(_m[t].m5, 6, 2)) {
						default: throw SlCorrupt("Invalid road tile type");
						case 0:
							if (add_roadtypes) {
								SB(_m[t].m6, 2, 4, 0);
							} else {
								SB(_m[t].m5, 0, 4, GB(_m[t].m4, 0, 4)); // road bits
							}
							SB(_me[t].m7, 0, 4, GB(_m[t].m3, 0, 4)); // road works
							SB(_m[t].m6, 3, 3, GB(_m[t].m3, 4, 3));  // ground
							SB(_m[t].m3, 0, 4, add_roadtypes ? 0 : GB(_m[t].m4, 4, 4)); // tram bits
							SB(_m[t].m3, 4, 4, GB(_m[t].m5, 0, 4));  // tram owner
							break;

						case 1:
							SB(_me[t].m7, 0, 5, GB(_m[t].m4, 0, 5)); // road owner
							SB(_m[t].m6, 3, 3, GB(_m[t].m3, 4, 3));  // ground
							SB(_m[t].m3, 4, 4, GB(_m[t].m5, 0, 4));  // tram owner
							SB(_m[t].m5, 0, 1, add_roadtypes ? GB(_m[t].m5, 3, 1) : GB(_m[t].m4, 6, 1)); // road axis
							SB(_m[t].m5, 5, 1, add_roadtypes ? GB(_m[t].m5, 2, 1) : GB(_m[t].m4, 5, 1)); // crossing state
							break;

						case 2:
							break;
					}
					_m[t].m4 = 0;
					if (old_bridge) SB(_m[t].m6, 6, 2, 0);
					break;

				case MP_STATION:
					if (GB(_m[t].m6, 4, 2) != 1) break;

					SB(_me[t].m7, 6, 2, add_roadtypes ? 1 : GB(_m[t].m3, 0, 3));
					SB(_me[t].m7, 0, 5, HasBit(_m[t].m6, 2) ? OWNER_TOWN : (Owner)GB(_m[t].m1, 0, 5));
					SB(_m[t].m3, 4, 4, _m[t].m1);
					_m[t].m4 = 0;
					break;

				case MP_TUNNELBRIDGE: {
					if (!old_bridge || !HasBit(_m[t].m5, 7) || !HasBit(_m[t].m5, 6)) {
						if (((old_bridge && HasBit(_m[t].m5, 7)) ? GB(_m[t].m5, 1, 2) : GB(_m[t].m5, 2, 2)) == 1) {
							/* Middle part of "old" bridges */
							SB(_me[t].m7, 6, 2, add_roadtypes ? 1 : GB(_m[t].m3, 0, 3));

							Owner o = (Owner)GB(_m[t].m1, 0, 5);
							SB(_me[t].m7, 0, 5, o); // road owner
							SB(_m[t].m3, 4, 4, o == OWNER_NONE ? OWNER_TOWN : o); // tram owner
						}
						SB(_m[t].m6, 2, 4, GB(_m[t].m2, 4, 4));  // bridge type
						SB(_me[t].m7, 5, 1, GB(_m[t].m4, 7, 1)); // snow/desert

						_m[t].m2 = 0;
						_m[t].m4 = 0;
						if (old_bridge) SB(_m[t].m6, 6, 2, 0);
					}

					if (!old_bridge || !HasBit(_m[t].m5, 7)) break;

					Axis axis = (Axis)GB(_m[t].m5, 0, 1);

					if (HasBit(_m[t].m5, 6)) { // middle part
						if (HasBit(_m[t].m5, 5)) { // transport route under bridge?
							if (GB(_m[t].m5, 3, 2) == 0) {
								SetTileType(t, MP_RAILWAY);
								_m[t].m2 = 0;
								SB(_m[t].m3, 4, 4, 0);
								_m[t].m5 = axis == AXIS_X ? TRACK_BIT_Y : TRACK_BIT_X;
								_m[t].m4 = _me[t].m7 = 0;
							} else {
								SetTileType(t, MP_ROAD);
								_m[t].m2 = INVALID_TOWN;
								_m[t].m3 = _m[t].m4 = 0;
								SB(_m[t].m3, 4, 4, OWNER_TOWN);
								_m[t].m5 = axis == AXIS_X ? ROAD_Y : ROAD_X;
								_me[t].m7 = 1 << 6;
							}
						} else if (GB(_m[t].m5, 3, 2) == 0) {
							SetTileType(t, MP_CLEAR);
							_m[t].m1 = OWNER_NONE;
							_m[t].m2 = 0;
							_m[t].m3 = _m[t].m4 = _me[t].m7 = 0;
							_m[t].m5 = 3;
						} else if (!IsTileFlat(t)) {
							SetTileType(t, MP_WATER);
							SB(_m[t].m1, 0, 5, OWNER_WATER);
							SB(_m[t].m1, 5, 2, WATER_CLASS_SEA);
							_m[t].m2 = 0;
							_m[t].m3 = _m[t].m4 = _me[t].m7 = 0;
							_m[t].m5 = 1;
						} else if (GB(_m[t].m1, 0, 5) == OWNER_WATER) {
							SetTileType(t, MP_WATER);
							SB(_m[t].m1, 0, 5, OWNER_WATER);
							SB(_m[t].m1, 5, 2, WATER_CLASS_SEA);
							_m[t].m2 = 0;
							_m[t].m3 = _m[t].m4 = _m[t].m5 = _me[t].m7 = 0;
						} else {
							SetTileType(t, MP_WATER);
							SB(_m[t].m1, 5, 2, WATER_CLASS_CANAL);
							_m[t].m2 = 0;
							_m[t].m3 = _m[t].m5 = _me[t].m7 = 0;
							_m[t].m4 = Random();
						}
						SB(_m[t].m6, 2, 6, (1 << 4) << axis);
					} else { // ramp
						uint north_south = GB(_m[t].m5, 5, 1);
						DiagDirection dir = ReverseDiagDir(XYNSToDiagDir(axis, north_south));
						TransportType type = (TransportType)GB(_m[t].m5, 1, 2);

						_m[t].m5 = 1 << 7 | type << 2 | dir;
					}

					break;
				}

				default: break;
			}
		}
	}

	/* From version 82, old style canals (above sealevel (0), WATER owner) are no longer supported.
	    Replace the owner for those by OWNER_NONE. */
	if (IsSavegameVersionBefore(stv, 82)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileType(t, MP_WATER) &&
					_m[t].m5 == 0 &&
					GB(_m[t].m1, 0, 5) == OWNER_WATER &&
					TileHeight(t) != 0) {
				SB(_m[t].m1, 0, 5, OWNER_NONE);
			}
		}
	}

	/*
	 * Add the 'previous' owner to the ship depots so we can reset it with
	 * the correct values when it gets destroyed. This prevents that
	 * someone can remove canals owned by somebody else and it prevents
	 * making floods using the removal of ship depots.
	 */
	if (IsSavegameVersionBefore(stv, 83)) {
		for (TileIndex t = 0; t < map_size; t++) {
			if (IsTileType(t, MP_WATER) && GB(_m[t].m5, 4, 4) == 8) {
				_m[t].m4 = (TileHeight(t) == 0) ? OWNER_WATER : OWNER_NONE;
			}
		}
	}

	/* The water class was moved/unified. */
	if (IsSavegameVersionBefore(stv, 146)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				case MP_STATION:
					switch (GB(_m[t].m6, 3, 3)) {
						case 4:
						case 5:
						case 6:
							SB(_m[t].m1, 5, 2, GB(_m[t].m3, 0, 2));
							SB(_m[t].m3, 0, 2, 0);
							break;

						default:
							SB(_m[t].m1, 5, 2, WATER_CLASS_INVALID);
							break;
					}
					break;

				case MP_WATER:
					SB(_m[t].m1, 5, 2, GB(_m[t].m3, 0, 2));
					SB(_m[t].m3, 0, 2, 0);
					break;

				case MP_OBJECT:
					SB(_m[t].m1, 5, 2, WATER_CLASS_INVALID);
					break;

				default:
					/* No water class. */
					break;
			}
		}
	}

	if (IsSavegameVersionBefore(stv, 86)) {
		for (TileIndex t = 0; t < map_size; t++) {
			/* Move river flag and update canals to use water class */
			if (IsTileType(t, MP_WATER) && GB(_m[t].m1, 5, 2) != WATER_CLASS_RIVER) {
				if (_m[t].m5 == 0) {
					if (GB(_m[t].m1, 0, 5) == OWNER_WATER) {
						SB(_m[t].m1, 5, 2, WATER_CLASS_SEA);
						_m[t].m4 = 0;
					} else {
						SB(_m[t].m1, 5, 2, WATER_CLASS_CANAL);
						_m[t].m4 = Random();
					}
					_m[t].m2 = 0;
					_m[t].m3 = _m[t].m5 = _me[t].m7 = 0;
					SB(_m[t].m6, 2, 4, 0);
				} else if (GB(_m[t].m5, 4, 4) == 8) {
					Owner o = (Owner)_m[t].m4; // Original water owner
					SB(_m[t].m1, 5, 2, o == OWNER_WATER ? WATER_CLASS_SEA : WATER_CLASS_CANAL);
				}
			}
		}
	}

	/* Move the signal variant back up one bit for PBS. We don't convert the old PBS
	 * format here, as an old layout wouldn't work properly anyway. To be safe, we
	 * clear any possible PBS reservations as well. */
	if (IsSavegameVersionBefore(stv, 100)) {
		for (TileIndex t = 0; t < map_size; t++) {
			switch (GetTileType(t)) {
				case MP_RAILWAY:
					if (GB(_m[t].m5, 6, 2) == 1) {
						/* move the signal variant */
						SB(_m[t].m2, 3, 1, HasBit(_m[t].m2, 2) ? SIG_SEMAPHORE : SIG_ELECTRIC);
						SB(_m[t].m2, 7, 1, HasBit(_m[t].m2, 6) ? SIG_SEMAPHORE : SIG_ELECTRIC);
						ClrBit(_m[t].m2, 2);
						ClrBit(_m[t].m2, 6);
					}

					/* Clear PBS reservation on track */
					if (GB(_m[t].m5, 6, 2) == 3) {
						ClrBit(_m[t].m5, 4);
					} else {
						SB(_m[t].m2, 8, 4, 0);
					}
					break;

				case MP_ROAD: // Clear PBS reservation on crossing
					if (GB(_m[t].m5, 6, 2) == 1) ClrBit(_m[t].m5, 4);
					break;

				case MP_STATION: // Clear PBS reservation on station
					if (GB(_m[t].m6, 3, 3) == 0 || GB(_m[t].m6, 3, 3) == 7) ClrBit(_m[t].m6, 2);
					break;

				case MP_TUNNELBRIDGE: // Clear PBS reservation on tunnels/bridges
					if (GB(_m[t].m5, 2, 2) == 0) ClrBit(_m[t].m5, 4);
					break;

				default: break;
			}
		}
	}
}


struct MapDim {
	uint32 x, y;
};

static const SaveLoad _map_dimensions[] = {
	SLE_CONDVAR(MapDim, x, SLE_UINT32, 6, SL_MAX_VERSION),
	SLE_CONDVAR(MapDim, y, SLE_UINT32, 6, SL_MAX_VERSION),
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
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _m[i++].type_height = buf[j];
	}
}

static void Save_MAPT(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _m[i++].type_height;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP1(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _m[i++].m1 = buf[j];
	}
}

static void Save_MAP1(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _m[i++].m1;
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
			reader->IsVersionBefore(5) ? SLE_FILE_U8 | SLE_VAR_U16 : SLE_UINT16
		);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _m[i++].m2 = buf[j];
	}
}

static void Save_MAP2(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<uint16, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size * sizeof(uint16));
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _m[i++].m2;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT16);
	}
}

static void Load_MAP3(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _m[i++].m3 = buf[j];
	}
}

static void Save_MAP3(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _m[i++].m3;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP4(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _m[i++].m4 = buf[j];
	}
}

static void Save_MAP4(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _m[i++].m4;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP5(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _m[i++].m5 = buf[j];
	}
}

static void Save_MAP5(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _m[i++].m5;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP6(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	if (reader->IsVersionBefore(42)) {
		for (TileIndex i = 0; i != size;) {
			/* 1024, otherwise we overflow on 64x64 maps! */
			reader->ReadArray(buf, 1024, SLE_UINT8);
			for (uint j = 0; j != 1024; j++) {
				_m[i++].m6 = GB(buf[j], 0, 2);
				_m[i++].m6 = GB(buf[j], 2, 2);
				_m[i++].m6 = GB(buf[j], 4, 2);
				_m[i++].m6 = GB(buf[j], 6, 2);
			}
		}
	} else {
		for (TileIndex i = 0; i != size;) {
			reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
			for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _m[i++].m6 = buf[j];
		}
	}
}

static void Save_MAP6(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _m[i++].m6;
		dumper->WriteArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
	}
}

static void Load_MAP7(LoadBuffer *reader)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	for (TileIndex i = 0; i != size;) {
		reader->ReadArray(buf, MAP_SL_BUF_SIZE, SLE_UINT8);
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) _me[i++].m7 = buf[j];
	}
}

static void Save_MAP7(SaveDumper *dumper)
{
	SmallStackSafeStackAlloc<byte, MAP_SL_BUF_SIZE> buf;
	TileIndex size = MapSize();

	dumper->WriteRIFFSize(size);
	for (TileIndex i = 0; i != size;) {
		for (uint j = 0; j != MAP_SL_BUF_SIZE; j++) buf[j] = _me[i++].m7;
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
	{ 'MAPE', Save_MAP6, Load_MAP6, NULL, NULL,       CH_RIFF },
	{ 'MAP7', Save_MAP7, Load_MAP7, NULL, NULL,       CH_RIFF | CH_LAST },
};
