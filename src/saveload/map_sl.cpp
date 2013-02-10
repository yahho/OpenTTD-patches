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
#include "../fios.h"

#include "saveload_buffer.h"

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
