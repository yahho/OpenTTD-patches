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
#include "../../roadstop_base.h"

/** Yapf Node for road YAPF */
template <class Tkey_>
struct CYapfRoadNodeT : CYapfNodeT<Tkey_, CYapfRoadNodeT<Tkey_> > {
	typedef CYapfNodeT<Tkey_, CYapfRoadNodeT<Tkey_> > base;

	PathMPos<RoadPathPos> m_next; ///< next pos after segment end, invalid if this segment is a dead end

	void Set(CYapfRoadNodeT *parent, const RoadPathPos &pos)
	{
		base::Set(parent, pos);
		m_next = PathMPos<RoadPathPos>();
	}

	void Set (CYapfRoadNodeT *parent, const RoadPathPos &pos, const PathMPos<RoadPathPos> &next)
	{
		base::Set(parent, pos);
		m_next.set (next);
	}
};

/* now define two major node types (that differ by key type) */
typedef CYapfRoadNodeT<CYapfNodeKeyExitDir <RoadPathPos> > CYapfRoadNodeExitDir;
typedef CYapfRoadNodeT<CYapfNodeKeyTrackDir<RoadPathPos> > CYapfRoadNodeTrackDir;

/* Default Astar types */
typedef Astar<CYapfRoadNodeExitDir , 8, 10> AstarRoadExitDir;
typedef Astar<CYapfRoadNodeTrackDir, 8, 10> AstarRoadTrackDir;


/** Check if the road on a given tile is uphill in a given direction. */
static bool IsUphill (TileIndex tile, DiagDirection dir)
{
	/* compute the middle point of the tile */
	int x = TileX (tile) * TILE_SIZE + TILE_SIZE / 2;
	int y = TileY (tile) * TILE_SIZE + TILE_SIZE / 2;

	/* compute difference vector */
	CoordDiff diff = CoordDiffByDiagDir (dir);
	diff.x *= TILE_SIZE / 4;
	diff.y *= TILE_SIZE / 4;

	/* check height different along the direction */
	return (GetSlopePixelZ (x + diff.x, y + diff.y) - GetSlopePixelZ (x - diff.x, y - diff.y)) > 1;
}

/** Check if the road on a given position is uphill. */
static inline bool IsUphill (const RoadPathPos &pos)
{
	return IsUphill (pos.tile, TrackdirToExitdir (pos.td));
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
						const RoadStop::Platform *platform = rs->GetPlatform();
						cost += platform->GetOccupied(dir) * settings->road_stop_occupied_penalty / platform->GetLength();
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

		/* add slope cost */
		if (IsUphill (pos)) {
			cost += settings->road_slope_penalty;
		}
	} else {
		/* non-diagonal trackdir */
		cost = YAPF_TILE_CORNER_LENGTH + settings->road_curve_penalty;
	}
	return cost;
}


template <class TAstar>
class CYapfRoadT : public TAstar {
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
			return IsRoadDepotTile(tile) && IsTileOwner(tile, m_veh->owner);
		}
	}

	inline bool IsDestination(const RoadPathPos &pos) const
	{
		return IsDestinationTile(pos.tile);
	}

	/** Called by the A-star underlying class to find the neighbours of a node. */
	inline void Follow (Node *old_node)
	{
		/* Previous segment is a dead end? */
		if (!old_node->m_next.is_valid()) return;

		assert (!old_node->m_next.is_empty());

		RoadPathPos pos (old_node->m_next);
		TileIndex last_tile;
		DiagDirection last_dir;

		for (TrackdirBits rtds = old_node->m_next.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.set_trackdir (FindFirstTrackdir(rtds));
			Node *n = TAstar::CreateNewNode(old_node, pos);

			/* start at pos and walk to the end of segment */
			tf.SetPos (pos);

			int segment_cost = 0;
			uint tiles = 0;
			bool is_target = false;
			for (;;) {
				/* base tile cost depending on distance between edges */
				segment_cost += OneTileCost (m_settings, tf.m_new);

				/* add max speed penalty */
				int speed_penalty;
				if (IsRoadBridgeTile (tf.m_new.tile)) {
					int max_veh_speed = m_veh->GetDisplayMaxSpeed();
					int max_speed = GetBridgeSpec (GetRoadBridgeType (tf.m_new.tile))->speed;
					speed_penalty = max_veh_speed - max_speed;
					if (speed_penalty > 0) {
						segment_cost += speed_penalty;
					} else {
						speed_penalty = 0;
					}
				} else {
					speed_penalty = 0;
				}

				/* we have reached the vehicle's destination - segment should end here to avoid target skipping */
				if (IsDestination(tf.m_new)) {
					n->m_next = tf.m_new;
					is_target = true;
					break;
				}

				/* stop if we have just entered the depot */
				if (IsRoadDepotTile(tf.m_new.tile) && tf.m_new.td == DiagDirToDiagTrackdir(ReverseDiagDir(GetGroundDepotDirection(tf.m_new.tile)))) {
					/* next time we will reverse and leave the depot */
					n->m_next.set (tf.m_new.tile, ReverseTrackdir (tf.m_new.td));
					last_tile = tf.m_new.tile;
					last_dir = ReverseDiagDir (GetGroundDepotDirection (tf.m_new.tile));
					break;
				}

				/* if there are no reachable trackdirs on new tile, we have end of road */
				if (!tf.FollowNext()) {
					last_tile = tf.m_old.tile;
					last_dir = TrackdirToExitdir (tf.m_old.td);
					break;
				}

				/* stop if RV is on simple loop with no junctions */
				if (tf.m_new == pos) return;

				/* if we skipped some tunnel tiles, add their cost */
				segment_cost += tf.m_tiles_skipped * YAPF_TILE_LENGTH;
				if (tf.m_flag == tf.TF_BRIDGE) segment_cost += tf.m_tiles_skipped * speed_penalty;
				tiles += tf.m_tiles_skipped + 1;

				/* if there are more trackdirs available & reachable, we are at the end of segment */
				if (!tf.m_new.is_single() || tiles > MAX_MAP_SIZE) {
					n->m_next = tf.m_new;
					last_dir = tf.m_exitdir;
					last_tile = TileAddByDiagDir (tf.m_new.tile, ReverseDiagDir (last_dir));
					break;
				}

				/* move to the next tile */
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
					n->m_estimate = n->m_cost + YapfCalcEstimate (last_tile, last_dir, m_dest_tile);
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

	/* set origin node */
	pf.InsertInitialNode (pf.CreateNewNode (NULL, RoadPathPos(),
			PathMPos<RoadPathPos> (tile, trackdirs)));

	/* find the best path */
	path_found = pf.FindPath();

	/* if path not found - return INVALID_TRACKDIR */
	typename Tpf::Node *n = pf.GetBestNode();
	if ((n == NULL) || (n->m_parent == NULL)) return INVALID_TRACKDIR;

	/* path was found or at least suggested
	 * walk through the path back to its origin */
	while (n->m_parent->m_parent != NULL) {
		n = n->m_parent;
	}

	/* return trackdir from the best origin node */
	assert (!n->m_parent->GetPos().is_valid());
	assert (n->GetPos().tile == tile);
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
static TileIndex FindNearestDepot(const RoadVehicle *v, int max_distance)
{
	Tpf pf (v, true);

	/* set origin node */
	if (v->state == RVSB_WORMHOLE) {
		if ((v->vehstatus & VS_HIDDEN) == 0) {
			/* on a bridge */
			TrackdirBits trackdirs = TrackStatusToTrackdirBits(GetTileRoadStatus (v->tile, v->compatible_roadtypes)) & DiagdirReachesTrackdirs(DirToDiagDir(v->direction));
			assert (trackdirs != TRACKDIR_BIT_NONE);
			pf.InsertInitialNode (pf.CreateNewNode (NULL, RoadPathPos(), PathMPos<RoadPathPos> (v->tile, trackdirs)));
		} else {
			/* in a tunnel */
			Trackdir td = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
			pf.InsertInitialNode (pf.CreateNewNode (NULL, RoadPathPos(), PathMPos<RoadPathPos> (v->tile, td)));
		}
	} else {
		Trackdir td;

		if (v->state == RVSB_IN_DEPOT) {
			/* We'll assume the road vehicle is facing outwards */
			assert (IsRoadDepotTile (v->tile));
			td = DiagDirToDiagTrackdir(GetGroundDepotDirection(v->tile));
		} else if (IsStandardRoadStopTile(v->tile)) {
			/* We'll assume the road vehicle is facing outwards */
			td = DiagDirToDiagTrackdir(GetRoadStopDir(v->tile)); // Road vehicle in a station
		} else if (v->state > RVSB_TRACKDIR_MASK) {
			/* Drive-through road stops */
			td = DiagDirToDiagTrackdir(DirToDiagDir(v->direction));
		} else {
			/* If vehicle's state is a valid track direction (vehicle is not turning around) return it,
			 * otherwise transform it into a valid track direction */
			td = (Trackdir)((IsReversingRoadTrackdir((Trackdir)v->state)) ? (v->state - 6) : v->state);
		}

		if ((TrackStatusToTrackdirBits (GetTileRoadStatus (v->tile, v->compatible_roadtypes)) & TrackdirToTrackdirBits (td)) == 0) {
			return INVALID_TILE;
		}

		pf.InsertInitialNode (pf.CreateNewNode (NULL, RoadPathPos(), PathMPos<RoadPathPos> (v->tile, td)));
	}

	/* find the best path */
	if (!pf.FindPath()) return INVALID_TILE;

	/* some path found; get found depot tile */
	typename Tpf::Node *n = pf.GetBestNode();

	if (max_distance > 0 && n->m_cost > max_distance) return INVALID_TILE;

	assert (IsRoadDepotTile (n->m_next.tile));

	return n->m_next.tile;
}

TileIndex YapfRoadVehicleFindNearestDepot(const RoadVehicle *v, uint max_distance)
{
	/* default is YAPF type 2 */
	typedef TileIndex (*PfnFindNearestDepot)(const RoadVehicle*, int);
	PfnFindNearestDepot pfnFindNearestDepot = &FindNearestDepot<CYapfRoad2>;

	/* check if non-default YAPF type should be used */
	if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnFindNearestDepot = &FindNearestDepot<CYapfRoad1>; // Trackdir, allow 90-deg
	}

	return pfnFindNearestDepot(v, max_distance);
}
