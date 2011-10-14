/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file road_map.h Map accessors for roads. */

#ifndef ROAD_MAP_H
#define ROAD_MAP_H

#include "track_func.h"
#include "depot_type.h"
#include "rail_type.h"
#include "road_func.h"
#include "tile_map.h"
#include "bridge.h"


/**
 * Return whether a tile is a road depot tile.
 * @param t Tile to query.
 * @return True if road depot tile.
 */
static inline bool IsRoadDepotTile(TileIndex t)
{
	return IsGroundDepotTile(t) && HasBit(_mc[t].m1, 5);
}

/**
 * Get the present road bits for a specific road type.
 * @param t  The tile to query.
 * @param rt Road type.
 * @pre IsRoadTile(t)
 * @return The present road bits for the road type.
 */
static inline RoadBits GetRoadBits(TileIndex t, RoadType rt)
{
	assert(IsRoadTile(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: return (RoadBits)GB(_mc[t].m4, 0, 4);
		case ROADTYPE_TRAM: return (RoadBits)GB(_mc[t].m4, 4, 4);
	}
}

/**
 * Get all RoadBits set on a tile except from the given RoadType
 *
 * @param t The tile from which we want to get the RoadBits
 * @param rt The RoadType which we exclude from the querry
 * @return all set RoadBits of the tile which are not from the given RoadType
 */
static inline RoadBits GetOtherRoadBits(TileIndex t, RoadType rt)
{
	return GetRoadBits(t, rt == ROADTYPE_ROAD ? ROADTYPE_TRAM : ROADTYPE_ROAD);
}

/**
 * Get all set RoadBits on the given tile
 *
 * @param tile The tile from which we want to get the RoadBits
 * @return all set RoadBits of the tile
 */
static inline RoadBits GetAllRoadBits(TileIndex tile)
{
	return GetRoadBits(tile, ROADTYPE_ROAD) | GetRoadBits(tile, ROADTYPE_TRAM);
}

/**
 * Set the present road bits for a specific road type.
 * @param t  The tile to change.
 * @param r  The new road bits.
 * @param rt Road type.
 * @pre IsRoadTile(t)
 */
static inline void SetRoadBits(TileIndex t, RoadBits r, RoadType rt)
{
	assert(IsRoadTile(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: SB(_mc[t].m4, 0, 4, r); break;
		case ROADTYPE_TRAM: SB(_mc[t].m4, 4, 4, r); break;
	}
}

/**
 * Get the present road types of a tile.
 * @param t The tile to query.
 * @return Present road types.
 */
static inline RoadTypes GetRoadTypes(TileIndex t)
{
	return (RoadTypes)GB(_mc[t].m7, 6, 2);
}

/**
 * Set the present road types of a tile.
 * @param t  The tile to change.
 * @param rt The new road types.
 */
static inline void SetRoadTypes(TileIndex t, RoadTypes rt)
{
	assert(IsRoadTile(t) || IsLevelCrossingTile(t) || IsTunnelTile(t) || IsRoadDepotTile(t) || IsStationTile(t));
	SB(_mc[t].m7, 6, 2, rt);
}

/**
 * Check if a tile has a specific road type.
 * @param t  The tile to check.
 * @param rt Road type to check.
 * @return True if the tile has the specified road type.
 */
static inline bool HasTileRoadType(TileIndex t, RoadType rt)
{
	return HasBit(GetRoadTypes(t), rt);
}

/**
 * Get the owner of a specific road type.
 * @param t  The tile to query.
 * @param rt The road type to get the owner of.
 * @return Owner of the given road type.
 */
static inline Owner GetRoadOwner(TileIndex t, RoadType rt)
{
	assert(IsRoadTile(t) || IsLevelCrossingTile(t) || IsTunnelTile(t) || IsStationTile(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: return (Owner)GB(IsRoadTile(t) ? _mc[t].m1 : _mc[t].m7, 0, 5);
		case ROADTYPE_TRAM: {
			/* Trams don't need OWNER_TOWN, and remapping OWNER_NONE
			 * to OWNER_TOWN makes it use one bit less */
			Owner o = (Owner)(IsStationTile(t) ? GB(_mc[t].m3, 4, 4) : GB(_mc[t].m5, 0, 4));
			return o == OWNER_TOWN ? OWNER_NONE : o;
		}
	}
}

/**
 * Set the owner of a specific road type.
 * @param t  The tile to change.
 * @param rt The road type to change the owner of.
 * @param o  New owner of the given road type.
 */
static inline void SetRoadOwner(TileIndex t, RoadType rt, Owner o)
{
	assert(IsRoadTile(t) || IsLevelCrossingTile(t) || IsTunnelTile(t) || IsStationTile(t));
	switch (rt) {
		default: NOT_REACHED();
		case ROADTYPE_ROAD: SB(IsRoadTile(t) ? _mc[t].m1 : _mc[t].m7, 0, 5, o); break;
		case ROADTYPE_TRAM:
			if (o == OWNER_NONE) o = OWNER_TOWN;
			if (IsStationTile(t)) {
				SB(_mc[t].m3, 4, 4, o);
			} else {
				SB(_mc[t].m5, 0, 4, o);
			}
			break;
	}
}

/**
 * Check if a specific road type is owned by an owner.
 * @param t  The tile to query.
 * @param rt The road type to compare the owner of.
 * @param o  Owner to compare with.
 * @pre HasTileRoadType(t, rt)
 * @return True if the road type is owned by the given owner.
 */
static inline bool IsRoadOwner(TileIndex t, RoadType rt, Owner o)
{
	assert(HasTileRoadType(t, rt));
	return (GetRoadOwner(t, rt) == o);
}

/**
 * Checks if given tile has town owned road
 * @param t tile to check
 * @pre IsRoadTile(t)
 * @return true iff tile has road and the road is owned by a town
 */
static inline bool HasTownOwnedRoad(TileIndex t)
{
	return HasTileRoadType(t, ROADTYPE_ROAD) && IsRoadOwner(t, ROADTYPE_ROAD, OWNER_TOWN);
}

/** Which directions are disallowed ? */
enum DisallowedRoadDirections {
	DRD_NONE,       ///< None of the directions are disallowed
	DRD_SOUTHBOUND, ///< All southbound traffic is disallowed
	DRD_NORTHBOUND, ///< All northbound traffic is disallowed
	DRD_BOTH,       ///< All directions are disallowed
	DRD_END,        ///< Sentinel
};
DECLARE_ENUM_AS_BIT_SET(DisallowedRoadDirections)
/** Helper information for extract tool. */
template <> struct EnumPropsT<DisallowedRoadDirections> : MakeEnumPropsT<DisallowedRoadDirections, byte, DRD_NONE, DRD_END, DRD_END, 2> {};

/**
 * Gets the disallowed directions
 * @param t the tile to get the directions from
 * @return the disallowed directions
 */
static inline DisallowedRoadDirections GetDisallowedRoadDirections(TileIndex t)
{
	assert(IsNormalRoadTile(t));
	return (DisallowedRoadDirections)GB(_mc[t].m3, 6, 2);
}

/**
 * Sets the disallowed directions
 * @param t   the tile to set the directions for
 * @param drd the disallowed directions
 */
static inline void SetDisallowedRoadDirections(TileIndex t, DisallowedRoadDirections drd)
{
	assert(IsNormalRoadTile(t));
	assert(drd < DRD_END);
	SB(_mc[t].m3, 6, 2, drd);
}

/**
 * Get the road axis of a level crossing.
 * @param t The tile to query.
 * @pre IsLevelCrossingTile(t)
 * @return The axis of the road.
 */
static inline Axis GetCrossingRoadAxis(TileIndex t)
{
	assert(IsLevelCrossingTile(t));
	return (Axis)GB(_mc[t].m4, 5, 1);
}

/**
 * Get the rail axis of a level crossing.
 * @param t The tile to query.
 * @pre IsLevelCrossingTile(t)
 * @return The axis of the rail.
 */
static inline Axis GetCrossingRailAxis(TileIndex t)
{
	assert(IsLevelCrossingTile(t));
	return OtherAxis((Axis)GetCrossingRoadAxis(t));
}

/**
 * Get the road bits of a level crossing.
 * @param tile The tile to query.
 * @return The present road bits.
 */
static inline RoadBits GetCrossingRoadBits(TileIndex tile)
{
	return GetCrossingRoadAxis(tile) == AXIS_X ? ROAD_X : ROAD_Y;
}

/**
 * Get the rail track of a level crossing.
 * @param tile The tile to query.
 * @return The rail track.
 */
static inline Track GetCrossingRailTrack(TileIndex tile)
{
	return AxisToTrack(GetCrossingRailAxis(tile));
}

/**
 * Get the rail track bits of a level crossing.
 * @param tile The tile to query.
 * @return The rail track bits.
 */
static inline TrackBits GetCrossingRailBits(TileIndex tile)
{
	return AxisToTrackBits(GetCrossingRailAxis(tile));
}


/**
 * Get the reservation state of the rail crossing
 * @param t the crossing tile
 * @return reservation state
 * @pre IsLevelCrossingTile(t)
 */
static inline bool HasCrossingReservation(TileIndex t)
{
	assert(IsLevelCrossingTile(t));
	return HasBit(_mc[t].m4, 7);
}

/**
 * Set the reservation state of the rail crossing
 * @param t the crossing tile
 * @param b the reservation state
 * @pre IsLevelCrossingTile(t)
 */
static inline void SetCrossingReservation(TileIndex t, bool b)
{
	assert(IsLevelCrossingTile(t));
	SB(_mc[t].m4, 7, 1, b ? 1 : 0);
}

/**
 * Get the reserved track bits for a rail crossing
 * @param t the tile
 * @pre IsLevelCrossingTile(t)
 * @return reserved track bits
 */
static inline TrackBits GetCrossingReservationTrackBits(TileIndex t)
{
	return HasCrossingReservation(t) ? GetCrossingRailBits(t) : TRACK_BIT_NONE;
}

/**
 * Check if the level crossing is barred.
 * @param t The tile to query.
 * @pre IsLevelCrossingTile(t)
 * @return True if the level crossing is barred.
 */
static inline bool IsCrossingBarred(TileIndex t)
{
	assert(IsLevelCrossingTile(t));
	return HasBit(_mc[t].m4, 6);
}

/**
 * Set the bar state of a level crossing.
 * @param t The tile to modify.
 * @param barred True if the crossing should be barred, false otherwise.
 * @pre IsLevelCrossingTile(t)
 */
static inline void SetCrossingBarred(TileIndex t, bool barred)
{
	assert(IsLevelCrossingTile(t));
	SB(_mc[t].m4, 6, 1, barred ? 1 : 0);
}

/**
 * Unbar a level crossing.
 * @param t The tile to change.
 */
static inline void UnbarCrossing(TileIndex t)
{
	SetCrossingBarred(t, false);
}

/**
 * Bar a level crossing.
 * @param t The tile to change.
 */
static inline void BarCrossing(TileIndex t)
{
	SetCrossingBarred(t, true);
}


/** The possible road side decorations. */
enum Roadside {
	ROADSIDE_BARREN           = 0, ///< Road on barren land
	ROADSIDE_GRASS            = 1, ///< Road on grass
	ROADSIDE_PAVED            = 2, ///< Road with paved sidewalks
	ROADSIDE_STREET_LIGHTS    = 3, ///< Road with street lights on paved sidewalks
	ROADSIDE_TREES            = 5, ///< Road with trees on paved sidewalks
	ROADSIDE_GRASS_ROAD_WORKS = 6, ///< Road on grass with road works
	ROADSIDE_PAVED_ROAD_WORKS = 7, ///< Road with sidewalks and road works
};

/**
 * Get the decorations of a road.
 * @param tile The tile to query.
 * @return The road decoration of the tile.
 */
static inline Roadside GetRoadside(TileIndex tile)
{
	assert(IsNormalRoadTile(tile) || IsLevelCrossingTile(tile));
	return (Roadside)GB(_mc[tile].m5, 4, 3);
}

/**
 * Set the decorations of a road.
 * @param tile The tile to change.
 * @param s    The new road decoration of the tile.
 */
static inline void SetRoadside(TileIndex tile, Roadside s)
{
	assert(IsNormalRoadTile(tile) || IsLevelCrossingTile(tile));
	SB(_mc[tile].m5, 4, 3, s);
}

/**
 * Check if a tile has road works.
 * @param t The tile to check.
 * @return True if the tile has road works in progress.
 */
static inline bool HasRoadWorks(TileIndex t)
{
	return GetRoadside(t) >= ROADSIDE_GRASS_ROAD_WORKS;
}

/**
 * Increase the progress counter of road works.
 * @param t The tile to modify.
 * @return True if the road works are in the last stage.
 */
static inline bool IncreaseRoadWorksCounter(TileIndex t)
{
	AB(_mc[t].m7, 0, 4, 1);

	return GB(_mc[t].m7, 0, 4) == 15;
}

/**
 * Start road works on a tile.
 * @param t The tile to start the work on.
 * @pre !HasRoadWorks(t)
 */
static inline void StartRoadWorks(TileIndex t)
{
	assert(!HasRoadWorks(t));
	/* Remove any trees or lamps in case or roadwork */
	switch (GetRoadside(t)) {
		case ROADSIDE_BARREN:
		case ROADSIDE_GRASS:  SetRoadside(t, ROADSIDE_GRASS_ROAD_WORKS); break;
		default:              SetRoadside(t, ROADSIDE_PAVED_ROAD_WORKS); break;
	}
}

/**
 * Terminate road works on a tile.
 * @param t Tile to stop the road works on.
 * @pre HasRoadWorks(t)
 */
static inline void TerminateRoadWorks(TileIndex t)
{
	assert(HasRoadWorks(t));
	SetRoadside(t, (Roadside)(GetRoadside(t) - ROADSIDE_GRASS_ROAD_WORKS + ROADSIDE_GRASS));
	/* Stop the counter */
	SB(_mc[t].m7, 0, 4, 0);
}


/**
 * Determines the type of road bridge on a tile
 * @param t The tile to analyze
 * @pre IsRoadBridgeTile(t)
 * @return The bridge type
 */
static inline BridgeType GetRoadBridgeType(TileIndex t)
{
	assert(IsRoadBridgeTile(t));
	return GB(_mc[t].m7, 0, 4);
}

/**
 * Set the type of road bridge on a tile
 * @param t The tile to set
 * @param type The type to set
 */
static inline void SetRoadBridgeType(TileIndex t, BridgeType type)
{
	assert(IsRoadBridgeTile(t));
	SB(_mc[t].m7, 0, 4, type);
}

/**
 * Check if a road bridge is an extended bridge head
 * @param t The tile to check
 * @return Whether there are road bits set not in the axis of the bridge
 */
static inline bool IsExtendedRoadBridge(TileIndex t)
{
	assert(IsRoadBridgeTile(t));

	RoadBits axis = AxisToRoadBits(DiagDirToAxis(GetTunnelBridgeDirection(t)));
	RoadBits road = GetRoadBits(t, ROADTYPE_ROAD);
	RoadBits tram = GetRoadBits(t, ROADTYPE_TRAM);

	return (road != ROAD_NONE && road != axis) || (tram != ROAD_NONE && tram != axis);
}


RoadBits GetAnyRoadBits(TileIndex tile, RoadType rt, bool tunnel_bridge_entrance = false);


/**
 * Make a normal road tile.
 * @param t    Tile to make a normal road.
 * @param bits Road bits to set for all present road types.
 * @param rot  New present road types.
 * @param town Town ID if the road is a town-owned road.
 * @param road New owner of road.
 * @param tram New owner of tram tracks.
 */
static inline void MakeRoadNormal(TileIndex t, RoadBits bits, RoadTypes rot, TownID town, Owner road, Owner tram)
{
	SetTileTypeSubtype(t, TT_ROAD, TT_TRACK);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, road);
	_mc[t].m2 = town;
	_mc[t].m3 = 0;
	_mc[t].m4 = (HasBit(rot, ROADTYPE_ROAD) ? bits : 0) | ((HasBit(rot, ROADTYPE_TRAM) ? bits : 0) << 4);
	_mc[t].m5 = 0;
	_mc[t].m7 = rot << 6;
	SetRoadOwner(t, ROADTYPE_TRAM, tram);
}

/**
 * Make a bridge ramp for roads.
 * @param t          the tile to make a bridge ramp
 * @param owner_road the new owner of the road on the bridge
 * @param owner_tram the new owner of the tram on the bridge
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @param r          the road type of the bridge
 * @param town       owner/closest town ID
 */
static inline void MakeRoadBridgeRamp(TileIndex t, Owner owner_road, Owner owner_tram, BridgeType bridgetype, DiagDirection d, RoadTypes r, uint town)
{
	SetTileTypeSubtype(t, TT_ROAD, TT_BRIDGE);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, owner_road);
	_mc[t].m2 = town;
	_mc[t].m3 = d << 6;
	RoadBits bits = AxisToRoadBits(DiagDirToAxis(d));
	_mc[t].m4 = (HasBit(r, ROADTYPE_ROAD) ? bits : 0) | ((HasBit(r, ROADTYPE_TRAM) ? bits : 0) << 4);
	_mc[t].m5 = 0;
	_mc[t].m7 = bridgetype | (r << 6);
	if (owner_tram != OWNER_TOWN) SetRoadOwner(t, ROADTYPE_TRAM, owner_tram);
}

/**
 * Make a normal road tile from a road bridge ramp.
 * @param t the tile to make a normal road
 * @note roadbits will have to be adjusted when this function is called
 */
static inline void MakeNormalRoadFromBridge(TileIndex t)
{
	assert(IsRoadBridgeTile(t));
	SetTileTypeSubtype(t, TT_ROAD, TT_TRACK);
	SB(_mc[t].m3, 6, 2, 0);
	SB(_mc[t].m7, 0, 4, 0);
}

/**
 * Make a road bridge tile from a normal road.
 * @param t          the tile to make a road bridge
 * @param bridgetype the type of bridge this bridge ramp belongs to
 * @param d          the direction this ramp must be facing
 * @note roadbits will have to be adjusted when this function is called
 */
static inline void MakeRoadBridgeFromRoad(TileIndex t, BridgeType bridgetype, DiagDirection d)
{
	assert(IsNormalRoadTile(t));
	SetTileTypeSubtype(t, TT_ROAD, TT_BRIDGE);
	SB(_mc[t].m3, 6, 2, d);
	SB(_mc[t].m5, 4, 3, 0);
	SB(_mc[t].m7, 0, 4, bridgetype);
}

/**
 * Make a level crossing.
 * @param t       Tile to make a level crossing.
 * @param road    New owner of road.
 * @param tram    New owner of tram tracks.
 * @param rail    New owner of the rail track.
 * @param roaddir Axis of the road.
 * @param rat     New rail type.
 * @param rot     New present road types.
 * @param town    Town ID if the road is a town-owned road.
 */
static inline void MakeRoadCrossing(TileIndex t, Owner road, Owner tram, Owner rail, Axis roaddir, RailType rat, RoadTypes rot, uint town)
{
	SetTileTypeSubtype(t, TT_MISC, TT_MISC_CROSSING);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, rail);
	_mc[t].m2 = town;
	_mc[t].m3 = rat;
	_mc[t].m4 = roaddir << 5;
	_mc[t].m5 = 0;
	_mc[t].m7 = rot << 6 | road;
	SetRoadOwner(t, ROADTYPE_TRAM, tram);
}

/**
 * Make a road depot.
 * @param t     Tile to make a level crossing.
 * @param owner New owner of the depot.
 * @param did   New depot ID.
 * @param dir   Direction of the depot exit.
 * @param rt    Road type of the depot.
 */
static inline void MakeRoadDepot(TileIndex t, Owner owner, DepotID did, DiagDirection dir, RoadType rt)
{
	SetTileTypeSubtype(t, TT_MISC, TT_MISC_DEPOT);
	SetBit(_mc[t].m1, 5);
	SB(_mc[t].m0, 2, 2, 0);
	SetTileOwner(t, owner);
	_mc[t].m2 = did;
	_mc[t].m3 = 0;
	_mc[t].m4 = 0;
	_mc[t].m5 = dir;
	_mc[t].m7 = RoadTypeToRoadTypes(rt) << 6;
}

#endif /* ROAD_MAP_H */
