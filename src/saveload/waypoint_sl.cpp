/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file waypoint_sl.cpp Code handling saving and loading of waypoints */

#include "../stdafx.h"
#include "../waypoint_base.h"
#include "../newgrf_station.h"
#include "../vehicle_base.h"
#include "../town.h"
#include "../newgrf.h"

#include "table/strings.h"

#include "saveload_internal.h"
#include "saveload_error.h"

/** Helper structure to convert from the old waypoint system. */
struct OldWaypoint {
	size_t index;
	TileIndex xy;
	TownID town_index;
	Town *town;
	uint16 town_cn;
	StringID string_id;
	char *name;
	uint8 delete_ctr;
	Date build_date;
	uint8 localidx;
	uint32 grfid;
	const StationSpec *spec;
	OwnerByte owner;

	size_t new_index;
};

/** Temporary array with old waypoints. */
static SmallVector<OldWaypoint, 16> _old_waypoints;

/**
 * Update the waypoint orders to get the new waypoint ID.
 * @param o the order 'list' to check.
 */
static void UpdateWaypointOrder(Order *o)
{
	if (!o->IsType(OT_GOTO_WAYPOINT)) return;

	for (OldWaypoint *wp = _old_waypoints.Begin(); wp != _old_waypoints.End(); wp++) {
		if (wp->index != o->GetDestination()) continue;

		o->SetDestination((DestinationID)wp->new_index);
		return;
	}
}

/**
 * Perform all steps to upgrade from the old waypoints to the new version
 * that uses station. This includes some old saveload mechanics.
 */
void MoveWaypointsToBaseStations(const SavegameTypeVersion *stv)
{
	/* In legacy version 17, ground type is moved from m2 to m4 for depots and
	 * waypoints to make way for storing the index in m2. The custom graphics
	 * id which was stored in m4 is now saved as a grf/id reference in the
	 * waypoint struct. */
	if (IsOTTDSavegameVersionBefore(stv, 17)) {
		for (OldWaypoint *wp = _old_waypoints.Begin(); wp != _old_waypoints.End(); wp++) {
			if (wp->delete_ctr != 0) continue; // The waypoint was deleted

			/* Waypoint indices were not added to the map prior to this. */
			_mc[wp->xy].m2 = (StationID)wp->index;

			if (HasBit(_mc[wp->xy].m3, 4)) {
				wp->spec = StationClass::Get(STAT_CLASS_WAYP)->GetSpec(_mc[wp->xy].m4 + 1);
			}
		}
	} else {
		/* As of version 17, we recalculate the custom graphic ID of waypoints
		 * from the GRF ID / station index. */
		for (OldWaypoint *wp = _old_waypoints.Begin(); wp != _old_waypoints.End(); wp++) {
			StationClass* stclass = StationClass::Get(STAT_CLASS_WAYP);
			for (uint i = 0; i < stclass->GetSpecCount(); i++) {
				const StationSpec *statspec = stclass->GetSpec(i);
				if (statspec != NULL && statspec->grf_prop.grffile->grfid == wp->grfid && statspec->grf_prop.local_id == wp->localidx) {
					wp->spec = statspec;
					break;
				}
			}
		}
	}

	if (!Waypoint::CanAllocateItem(_old_waypoints.Length())) throw SlException(STR_ERROR_TOO_MANY_STATIONS_LOADING);

	assert(IsOTTDSavegameVersionBefore(stv, 123));

	/* All saveload conversions have been done. Create the new waypoints! */
	for (OldWaypoint *wp = _old_waypoints.Begin(); wp != _old_waypoints.End(); wp++) {
		Waypoint *new_wp = new Waypoint(wp->xy);
		new_wp->town       = wp->town;
		new_wp->town_cn    = wp->town_cn;
		new_wp->name       = wp->name;
		new_wp->delete_ctr = 0; // Just reset delete counter for once.
		new_wp->build_date = wp->build_date;
		new_wp->owner      = wp->owner;

		new_wp->string_id = STR_SV_STNAME_WAYPOINT;

		TileIndex t = wp->xy;
		if (IsRailWaypointTile(t) && _mc[t].m2 == wp->index) {
			/* The tile might've been reserved! */
			bool reserved = !IsOTTDSavegameVersionBefore(stv, 100) && HasBit(_mc[t].m0, 0);

			/* The tile really has our waypoint, so reassign the map array */
			MakeRailWaypoint(t, GetTileOwner(t), new_wp->index, (Axis)GB(_mc[t].m5, 0, 1), 0, GetRailType(t));
			new_wp->facilities |= FACIL_TRAIN;
			new_wp->owner = GetTileOwner(t);

			SetRailStationReservation(t, reserved);

			if (wp->spec != NULL) {
				SetCustomStationSpecIndex(t, AllocateSpecToStation(wp->spec, new_wp, true));
			}
			new_wp->rect.BeforeAddTile(t, StationRect::ADD_FORCE);
		}

		wp->new_index = new_wp->index;
	}

	/* Update the orders of vehicles */
	OrderList *ol;
	FOR_ALL_ORDER_LISTS(ol) {
		if (ol->GetFirstSharedVehicle()->type != VEH_TRAIN) continue;

		for (Order *o = ol->GetFirstOrder(); o != NULL; o = o->next) UpdateWaypointOrder(o);
	}

	Vehicle *v;
	FOR_ALL_VEHICLES(v) {
		if (v->type != VEH_TRAIN) continue;

		UpdateWaypointOrder(&v->current_order);
	}

	_old_waypoints.Reset();
}

static const SaveLoad _old_waypoint_desc[] = {
	SLE_VAR(OldWaypoint, xy,         SLE_FILE_U16 | SLE_VAR_U32,  , ,   0,   5),
	SLE_VAR(OldWaypoint, xy,         SLE_UINT32,                 0, ,   6,    ),
	SLE_VAR(OldWaypoint, town_index, SLE_UINT16,                  , ,  12, 121),
	SLE_REF(OldWaypoint, town,       REF_TOWN,                   0, , 122,    ),
	SLE_VAR(OldWaypoint, town_cn,    SLE_FILE_U8 | SLE_VAR_U16,   , ,  12,  88),
	SLE_VAR(OldWaypoint, town_cn,    SLE_UINT16,                 0, ,  89,    ),
	SLE_VAR(OldWaypoint, string_id,  SLE_STRINGID,                , ,   0,  83),
	SLE_STR(OldWaypoint, name,       SLS_STR, 0,                 0, ,  84,    ),
	SLE_VAR(OldWaypoint, delete_ctr, SLE_UINT8),

	SLE_VAR(OldWaypoint, build_date, SLE_FILE_U16 | SLE_VAR_I32,  , ,   3,  30),
	SLE_VAR(OldWaypoint, build_date, SLE_INT32,                  0, ,  31,    ),
	SLE_VAR(OldWaypoint, localidx,   SLE_UINT8,                  0, ,   3,    ),
	SLE_VAR(OldWaypoint, grfid,      SLE_UINT32,                 0, ,  17,    ),
	SLE_VAR(OldWaypoint, owner,      SLE_UINT8,                  0, , 101,    ),

	SLE_END()
};

static void Load_WAYP(LoadBuffer *reader)
{
	/* Precaution for when loading failed and it didn't get cleared */
	_old_waypoints.Clear();

	int index;

	while ((index = reader->IterateChunk()) != -1) {
		OldWaypoint *wp = _old_waypoints.Append();
		memset(wp, 0, sizeof(*wp));

		wp->index = index;
		reader->ReadObject(wp, _old_waypoint_desc);
	}
}

static void Ptrs_WAYP(const SavegameTypeVersion *stv)
{
	for (OldWaypoint *wp = _old_waypoints.Begin(); wp != _old_waypoints.End(); wp++) {
		SlObjectPtrs(wp, _old_waypoint_desc, stv);
		if (stv == NULL) continue;

		if (IsOTTDSavegameVersionBefore(stv, 12)) {
			wp->town_cn = (wp->string_id & 0xC000) == 0xC000 ? (wp->string_id >> 8) & 0x3F : 0;
			wp->town = ClosestTownFromTile(wp->xy, UINT_MAX);
		} else if (IsOTTDSavegameVersionBefore(stv, 122)) {
			/* Only for versions 12 .. 122 */
			if (!Town::IsValidID(wp->town_index)) {
				/* Upon a corrupted waypoint we'll likely get here. The next step will be to
				 * loop over all Ptrs procs to NULL the pointers. However, we don't know
				 * whether we're in the NULL or "normal" Ptrs proc. So just clear the list
				 * of old waypoints we constructed and then this waypoint (and the other
				 * possibly corrupt ones) will not be queried in the NULL Ptrs proc run. */
				_old_waypoints.Clear();
				throw SlCorrupt("Referencing invalid Town");
			}
			wp->town = Town::Get(wp->town_index);
		}
		if (IsOTTDSavegameVersionBefore(stv, 84)) {
			wp->name = CopyFromOldName(stv, wp->string_id);
		}
	}
}

extern const ChunkHandler _waypoint_chunk_handlers[] = {
	{ 'CHKP', NULL, Load_WAYP, Ptrs_WAYP, NULL, CH_ARRAY | CH_LAST},
};
