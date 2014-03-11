/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_road.cpp The road pathfinding. */

#include "../../stdafx.h"
#include "yapf.hpp"
#include "yapf_node_road.hpp"
#include "../../roadstop_base.h"

static int SlopeCost(const YAPFSettings *settings, TileIndex tile, TileIndex next)
{
	/* height of the center of the current tile */
	int x1 = TileX(tile) * TILE_SIZE;
	int y1 = TileY(tile) * TILE_SIZE;
	int z1 = GetSlopePixelZ(x1 + TILE_SIZE / 2, y1 + TILE_SIZE / 2);

	/* height of the center of the next tile */
	int x2 = TileX(next) * TILE_SIZE;
	int y2 = TileY(next) * TILE_SIZE;
	int z2 = GetSlopePixelZ(x2 + TILE_SIZE / 2, y2 + TILE_SIZE / 2);

	return (z2 - z1 > 1) ? settings->road_slope_penalty : 0;
}

/** return one tile cost */
static int OneTileCost(const YAPFSettings *settings, const RoadPathPos &pos)
{
	int cost = 0;
	/* set base cost */
	if (IsDiagonalTrackdir(pos.td)) {
		cost += YAPF_TILE_LENGTH;
		switch (GetTileType(pos.tile)) {
			case TT_MISC:
				/* Increase the cost for level crossings */
				if (IsLevelCrossingTile(pos.tile)) {
					cost += settings->road_crossing_penalty;
				}
				break;

			case TT_STATION: {
				const RoadStop *rs = RoadStop::GetByTile(pos.tile, GetRoadStopType(pos.tile));
				if (IsDriveThroughStopTile(pos.tile)) {
					/* Increase the cost for drive-through road stops */
					cost += settings->road_stop_penalty;
					DiagDirection dir = TrackdirToExitdir(pos.td);
					if (!RoadStop::IsDriveThroughRoadStopContinuation(pos.tile, pos.tile - TileOffsByDiagDir(dir))) {
						/* When we're the first road stop in a 'queue' of them we increase
						 * cost based on the fill percentage of the whole queue. */
						const RoadStop::Entry *entry = rs->GetEntry(dir);
						cost += entry->GetOccupied() * settings->road_stop_occupied_penalty / entry->GetLength();
					}
				} else {
					/* Increase cost for filled road stops */
					cost += settings->road_stop_bay_occupied_penalty * (!rs->IsFreeBay(0) + !rs->IsFreeBay(1)) / 2;
				}
				break;
			}

			default:
				break;
		}
	} else {
		/* non-diagonal trackdir */
		cost = YAPF_TILE_CORNER_LENGTH + settings->road_curve_penalty;
	}
	return cost;
}


template <class TAstar>
class CYapfRoadT : public TAstar
{
public:
	typedef typename TAstar::Node Node; ///< this will be our node type

protected:
	const YAPFSettings *const m_settings; ///< current settings (_settings_game.yapf)
	const RoadVehicle  *const m_veh;      ///< vehicle that we are trying to drive
	const bool          m_bus;            ///< whether m_veh is a bus
	const bool          m_artic;          ///< whether m_veh is articulated
	const StationID     m_dest_station;   ///< destination station id, or INVALID_STATION if target is not a station
	const TileIndex     m_dest_tile;      ///< destination tile, or the special marker INVALID_TILE to search for any depot
	CFollowTrackRoad    tf;               ///< track follower

public:
	/**
	 * Construct an instance of CYapfRoadT
	 * @param rv The road vehicle to pathfind for
	 * @param depot Look for any depot, else use current vehicle order
	 */
	CYapfRoadT (const RoadVehicle *rv, bool depot = false)
		: m_settings(&_settings_game.pf.yapf)
		, m_veh(rv)
		, m_bus(rv->IsBus())
		, m_artic(rv->HasArticulatedPart())
		, m_dest_station(!depot && rv->current_order.IsType(OT_GOTO_STATION) ? rv->current_order.GetDestination() : INVALID_STATION)
		, m_dest_tile(depot ? INVALID_TILE :
			rv->current_order.IsType(OT_GOTO_STATION) ?
				Station::Get(m_dest_station)->GetClosestTile(rv->tile, m_bus ? STATION_BUS : STATION_TRUCK) :
				rv->dest_tile)
		, tf(rv)
	{
	}

	inline bool IsDestinationTile(TileIndex tile) const
	{
		if (m_dest_station != INVALID_STATION) {
			return IsStationTile(tile) &&
				GetStationIndex(tile) == m_dest_station &&
				(m_bus ? IsBusStop(tile) : IsTruckStop(tile)) &&
				(!m_artic || IsDriveThroughStopTile(tile));
		} else if (m_dest_tile != INVALID_TILE) {
			return tile == m_dest_tile;
		} else {
			return IsRoadDepotTile(tile);
		}
	}

	inline bool IsDestination(const RoadPathPos &pos) const
	{
		return IsDestinationTile(pos.tile);
	}

	/** Called by the A-star underlying class to find the neighbours of a node. */
	inline void Follow (Node *old_node)
	{
		if (!tf.Follow(old_node->m_segment_last)) return;

		uint initial_skipped_tiles = tf.m_tiles_skipped;
		RoadPathPos pos = tf.m_new;
		for (TrackdirBits rtds = tf.m_new.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.set_trackdir (FindFirstTrackdir(rtds));
			Node *n = TAstar::CreateNewNode(old_node, pos);

			uint tiles = initial_skipped_tiles;
			int segment_cost = tiles * YAPF_TILE_LENGTH;

			/* start at pos and walk to the end of segment */
			n->m_segment_last = pos;
			tf.SetPos (pos);

			bool is_target = false;
			for (;;) {
				/* base tile cost depending on distance between edges */
				segment_cost += OneTileCost (m_settings, tf.m_new);

				/* we have reached the vehicle's destination - segment should end here to avoid target skipping */
				if (IsDestination(tf.m_new)) {
					is_target = true;
					break;
				}

				/* stop if we have just entered the depot */
				if (IsRoadDepotTile(tf.m_new.tile) && tf.m_new.td == DiagDirToDiagTrackdir(ReverseDiagDir(GetGroundDepotDirection(tf.m_new.tile)))) {
					/* next time we will reverse and leave the depot */
					break;
				}

				/* if there are no reachable trackdirs on new tile, we have end of road */
				if (!tf.FollowNext()) break;

				/* if there are more trackdirs available & reachable, we are at the end of segment */
				if (!tf.m_new.is_single()) break;

				/* stop if RV is on simple loop with no junctions */
				if (tf.m_new == pos) return;

				/* if we skipped some tunnel tiles, add their cost */
				segment_cost += tf.m_tiles_skipped * YAPF_TILE_LENGTH;
				tiles += tf.m_tiles_skipped + 1;

				/* add hilly terrain penalty */
				assert (!tf.m_new.in_wormhole());
				segment_cost += SlopeCost(m_settings, tf.m_old.tile, tf.m_new.tile);

				/* add max speed penalty */
				int max_veh_speed = m_veh->GetDisplayMaxSpeed();
				int max_speed = tf.GetSpeedLimit();
				if (max_speed < max_veh_speed) segment_cost += 1 * (max_veh_speed - max_speed);

				/* move to the next tile */
				n->m_segment_last = tf.m_new;
				if (tiles > MAX_MAP_SIZE) break;
			}

			/* save also tile cost */
			n->m_cost = old_node->m_cost + segment_cost;

			/* compute estimated cost */
			if (is_target) {
				n->m_estimate = n->m_cost;
				TAstar::FoundTarget(n);
			} else {
				if (m_dest_tile == INVALID_TILE) {
					n->m_estimate = n->m_cost;
				} else {
					static const int dg_dir_to_x_offs[] = {-1, 0, 1, 0};
					static const int dg_dir_to_y_offs[] = {0, 1, 0, -1};

					TileIndex tile = n->m_segment_last.tile;
					DiagDirection exitdir = TrackdirToExitdir(n->m_segment_last.td);
					int x1 = 2 * TileX(tile) + dg_dir_to_x_offs[(int)exitdir];
					int y1 = 2 * TileY(tile) + dg_dir_to_y_offs[(int)exitdir];
					int x2 = 2 * TileX(m_dest_tile);
					int y2 = 2 * TileY(m_dest_tile);
					int dx = abs(x1 - x2);
					int dy = abs(y1 - y2);
					int dmin = min(dx, dy);
					int dxy = abs(dx - dy);
					int d = dmin * YAPF_TILE_CORNER_LENGTH + (dxy - 1) * (YAPF_TILE_LENGTH / 2);
					n->m_estimate = n->m_cost + d;
					assert(n->m_estimate >= old_node->m_estimate);
				}

				TAstar::InsertNode(n);
			}
		}
	}

	/** call the node follower */
	static inline void Follow (CYapfRoadT *pf, Node *n)
	{
		pf->Follow(n);
	}

	/** Invoke the underlying pathfinder. */
	inline bool FindPath (void)
	{
#ifndef NO_DEBUG_MESSAGES
		CPerformanceTimer perf;
		perf.Start();
#endif /* !NO_DEBUG_MESSAGES */

		bool bDestFound = TAstar::FindPath (Follow, m_settings->max_search_nodes);

#ifndef NO_DEBUG_MESSAGES
		perf.Stop();
		if (_debug_yapf_level >= 2) {
			int t = perf.Get(1000000);
			_total_pf_time_us += t;

			if (_debug_yapf_level >= 3) {
				int cost = bDestFound ? TAstar::best->m_cost : -1;
				int dist = bDestFound ? TAstar::best->m_estimate - TAstar::best->m_cost : -1;

				DEBUG(yapf, 3, "[YAPFr]%c%4d- %d us - %d rounds - %d open - %d closed - CHR  0.0%% - C %d D %d - c0(sc0, ts0, o0) -- ",
					bDestFound ? '-' : '!', m_veh->unitnumber, t, TAstar::num_steps, TAstar::OpenCount(), TAstar::ClosedCount(),
					cost, dist
				);
			}
		}
#endif /* !NO_DEBUG_MESSAGES */
		return bDestFound;
	}
};


typedef CYapfRoadT<AstarRoadTrackDir> CYapfRoad1;
typedef CYapfRoadT<AstarRoadExitDir > CYapfRoad2;


template <class Tpf>
static Trackdir ChooseRoadTrack(const RoadVehicle *v, TileIndex tile, TrackdirBits trackdirs, bool &path_found)
{
	Tpf pf (v);

	/* set origin nodes */
	RoadPathPos pos;
	pos.tile = tile;
	for (TrackdirBits tdb = trackdirs; tdb != TRACKDIR_BIT_NONE; tdb = KillFirstBit(tdb)) {
		pos.set_trackdir (FindFirstTrackdir(tdb));
		pf.InsertInitialNode (pf.CreateNewNode (NULL, pos));
	}

	/* find the best path */
	path_found = pf.FindPath();

	/* if path not found - return INVALID_TRACKDIR */
	typename Tpf::Node *n = pf.GetBestNode();
	if (n == NULL) return INVALID_TRACKDIR;

	/* path was found or at least suggested
	 * walk through the path back to its origin */
	while (n->m_parent != NULL) {
		n = n->m_parent;
	}

	/* return trackdir from the best origin node (one of start nodes) */
	assert(n->GetPos().tile == tile);
	return n->GetPos().td;
}

Trackdir YapfRoadVehicleChooseTrack(const RoadVehicle *v, TileIndex tile, DiagDirection enterdir, TrackdirBits trackdirs, bool &path_found)
{
	/* We really should not be called unless there is a choice to make. */
	assert (!HasAtMostOneBit(trackdirs));

	/* Handle special case - when next tile is destination tile.
	 * However, when going to a station the (initial) destination
	 * tile might not be a station, but a junction, in which case
	 * this method forces the vehicle to jump in circles. */
	if (tile == v->dest_tile && !v->current_order.IsType(OT_GOTO_STATION)) {
		/* choose diagonal trackdir reachable from enterdir */
		return DiagDirToDiagTrackdir(enterdir);
	}

	/* default is YAPF type 2 */
	typedef Trackdir (*PfnChooseRoadTrack)(const RoadVehicle*, TileIndex, TrackdirBits, bool &path_found);
	PfnChooseRoadTrack pfnChooseRoadTrack = &ChooseRoadTrack<CYapfRoad2>; // default: ExitDir, allow 90-deg

	/* check if non-default YAPF type should be used */
	if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnChooseRoadTrack = &ChooseRoadTrack<CYapfRoad1>; // Trackdir, allow 90-deg
	}

	Trackdir td_ret = pfnChooseRoadTrack(v, tile, trackdirs, path_found);
	return (td_ret != INVALID_TRACKDIR) ? td_ret : (Trackdir)FindFirstBit2x64(trackdirs);
}


template <class Tpf>
static TileIndex FindNearestDepot(const RoadVehicle *v, const RoadPathPos &pos, int max_distance)
{
	Tpf pf (v, true);

	/* set origin node */
	pf.InsertInitialNode (pf.CreateNewNode (NULL, pos));

	/* find the best path */
	if (!pf.FindPath()) return INVALID_TILE;

	/* some path found; get found depot tile */
	typename Tpf::Node *n = pf.GetBestNode();

	if (max_distance > 0 && n->m_cost > max_distance * YAPF_TILE_LENGTH) return INVALID_TILE;

	return n->m_segment_last.tile;
}

TileIndex YapfRoadVehicleFindNearestDepot(const RoadVehicle *v, uint max_distance)
{
	RoadPathPos pos = v->GetPos();
	if ((TrackStatusToTrackdirBits(GetTileRoadStatus(pos.tile, v->compatible_roadtypes)) & TrackdirToTrackdirBits(pos.td)) == 0) {
		return false;
	}

	/* default is YAPF type 2 */
	typedef TileIndex (*PfnFindNearestDepot)(const RoadVehicle*, const RoadPathPos&, int);
	PfnFindNearestDepot pfnFindNearestDepot = &FindNearestDepot<CYapfRoad2>;

	/* check if non-default YAPF type should be used */
	if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnFindNearestDepot = &FindNearestDepot<CYapfRoad1>; // Trackdir, allow 90-deg
	}

	return pfnFindNearestDepot(v, pos, max_distance);
}
