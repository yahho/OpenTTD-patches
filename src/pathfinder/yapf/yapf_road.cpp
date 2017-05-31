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

	CYapfRoadNodeT (const CYapfRoadNodeT *parent, const RoadPathPos &pos)
		: base (parent, pos), m_next()
	{
	}

	CYapfRoadNodeT (const CYapfRoadNodeT *parent, const RoadPathPos &pos, const PathMPos<RoadPathPos> &next)
		: base (parent, pos), m_next (next)
	{
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
static int OneTileCost (const RoadPathPos &pos)
{
	if (!IsDiagonalTrackdir(pos.td)) {
		/* non-diagonal trackdir */
		return YAPF_TILE_CORNER_LENGTH + _settings_game.pf.yapf.road_curve_penalty;
	}

	/* set base cost */
	int cost = YAPF_TILE_LENGTH;
	switch (GetTileType(pos.tile)) {
		case TT_MISC:
			/* Increase the cost for level crossings */
			if (IsLevelCrossingTile(pos.tile)) {
				cost += _settings_game.pf.yapf.road_crossing_penalty;
			}
			break;

		case TT_STATION: {
			const RoadStop *rs = RoadStop::GetByTile(pos.tile, GetRoadStopType(pos.tile));
			if (IsDriveThroughStopTile(pos.tile)) {
				/* Increase the cost for drive-through road stops */
				cost += _settings_game.pf.yapf.road_stop_penalty;
				DiagDirection dir = TrackdirToExitdir(pos.td);
				if (!RoadStop::IsDriveThroughRoadStopContinuation(pos.tile, pos.tile - TileOffsByDiagDir(dir))) {
					/* When we're the first road stop in a 'queue' of them we increase
					 * cost based on the fill percentage of the whole queue. */
					const RoadStop::Platform *platform = rs->GetPlatform();
					cost += platform->GetOccupied(dir) * _settings_game.pf.yapf.road_stop_occupied_penalty / platform->GetLength();
				}
			} else {
				/* Increase cost for filled road stops */
				cost += _settings_game.pf.yapf.road_stop_bay_occupied_penalty * (!rs->IsFreeBay(0) + !rs->IsFreeBay(1)) / 2;
			}
			break;
		}

		default:
			break;
	}

	/* add slope cost */
	if (IsUphill (pos)) {
		cost += _settings_game.pf.yapf.road_slope_penalty;
	}

	return cost;
}

/** Compute the speed penalty for a tile. */
static int SpeedPenalty (const RoadVehicle *v, const RoadPathPos &pos)
{
	if (!IsRoadBridgeTile (pos.tile)) return 0;

	int max_veh_speed = v->GetDisplayMaxSpeed();
	int max_bridge_speed = GetBridgeSpec (GetRoadBridgeType (pos.tile))->speed;
	int speed_penalty = max_veh_speed - max_bridge_speed;
	return (speed_penalty > 0) ? speed_penalty : 0;
}


/** Destination of a road vehicle. */
struct CYapfRoadDest {
	const union {
		StationID station; ///< destination station id, or INVALID_STATION if target is not a station
		OwnerByte owner;   ///< owner, if searching for any depot
	};
	const bool      is_bus;    ///< whether we want a bus stop (as opposed to a truck stop)
	const bool      is_artic;  ///< whether the vehicle is articulated (so we must use a drive-through stop)
	const TileIndex tile;      ///< destination tile, or the special marker INVALID_TILE to search for any depot

	/** Construct a CYapfRoadDest for the current order of a vehicle. */
	CYapfRoadDest (const RoadVehicle *rv)
		: station  (rv->current_order.IsType(OT_GOTO_STATION) ? rv->current_order.GetDestination() : INVALID_STATION),
		  is_bus   (rv->IsBus()),
		  is_artic (rv->HasArticulatedPart()),
		  tile     (rv->current_order.IsType(OT_GOTO_STATION) ?
				Station::Get(station)->GetClosestTile (rv->tile, is_bus ? STATION_BUS : STATION_TRUCK) :
				rv->dest_tile)
	{
		assert (this->tile != INVALID_TILE);
	}

	/** Construct a CYapfRoadDest to look for any depot. */
	CYapfRoadDest (const RoadVehicle *rv, bool ignored)
		: owner    (rv->owner),
		  is_bus   (rv->IsBus()),
		  is_artic (rv->HasArticulatedPart()),
		  tile     (INVALID_TILE)
	{
	}

	bool is_destination_tile (TileIndex t) const
	{
		if (this->tile == INVALID_TILE) {
			return IsRoadDepotTile (t) && IsTileOwner (t, this->owner);
		} else if (this->station == INVALID_STATION) {
			return t == this->tile;
		} else {
			return IsStationTile (t) &&
				GetStationIndex (t) == this->station &&
				(this->is_bus ? IsBusStop (t) : IsTruckStop (t)) &&
				(!this->is_artic || IsDriveThroughStopTile (t));
		}
	}

	bool is_destination (const RoadPathPos &pos) const
	{
		return this->is_destination_tile (pos.tile);
	}

	int calc_estimate (TileIndex src, DiagDirection dir) const
	{
		return (this->tile == INVALID_TILE) ? 0 :
				YapfCalcEstimate (src, dir, this->tile);
	}
};


template <class TAstar>
class CYapfRoadT : public TAstar {
public:
	typedef typename TAstar::Node Node; ///< this will be our node type

protected:
	const RoadVehicle  *const m_veh;      ///< vehicle that we are trying to drive
	const CYapfRoadDest m_dest;           ///< pathfinding destination
	CFollowTrackRoad    tf;               ///< track follower

public:
	/**
	 * Construct an instance of CYapfRoadT
	 * @param rv The road vehicle to pathfind for
	 */
	CYapfRoadT (const RoadVehicle *rv)
		: m_veh (rv), m_dest (rv), tf (rv)
	{
	}

	/**
	 * Construct an instance of CYapfRoadT to look for any depot
	 * @param rv The road vehicle to pathfind for
	 * @param ignored Ignored
	 */
	CYapfRoadT (const RoadVehicle *rv, bool ignored)
		: m_veh (rv), m_dest (rv, true), tf (rv)
	{
	}

	/** Called by the A-star underlying class to find the neighbours of a node. */
	inline void Follow (const Node *old_node)
	{
		/* Previous segment is a dead end? */
		if (!old_node->m_next.is_valid()) return;

		assert (!old_node->m_next.is_empty());

		RoadPathPos pos (old_node->m_next);
		TileIndex last_tile;
		DiagDirection last_dir;

		for (TrackdirBits rtds = old_node->m_next.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.set_trackdir (FindFirstTrackdir(rtds));
			Node n (old_node, pos);

			/* start at pos and walk to the end of segment */
			tf.SetPos (pos);

			int segment_cost = 0;
			uint tiles = 0;
			bool is_target = false;
			for (;;) {
				/* base tile cost depending on distance between edges */
				segment_cost += OneTileCost (tf.m_new);

				/* add max speed penalty */
				int speed_penalty = SpeedPenalty (m_veh, tf.m_new);
				segment_cost += speed_penalty;

				/* we have reached the vehicle's destination - segment should end here to avoid target skipping */
				if (m_dest.is_destination (tf.m_new)) {
					n.m_next = tf.m_new;
					is_target = true;
					break;
				}

				/* stop if we have just entered the depot */
				if (IsRoadDepotTile(tf.m_new.tile) && tf.m_new.td == DiagDirToDiagTrackdir(ReverseDiagDir(GetGroundDepotDirection(tf.m_new.tile)))) {
					/* next time we will reverse and leave the depot */
					n.m_next.set (tf.m_new.tile, ReverseTrackdir (tf.m_new.td));
					last_tile = tf.m_new.tile;
					last_dir = ReverseDiagDir (GetGroundDepotDirection (tf.m_new.tile));
					break;
				}

				/* if there are no reachable trackdirs on new tile, we have end of road */
				CFollowTrackRoad::ErrorCode err = tf.FollowNext();
				tf.m_err = err;
				if (err != CFollowTrackRoad::EC_NONE) {
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
					n.m_next = tf.m_new;
					last_dir = tf.m_exitdir;
					last_tile = TileAddByDiagDir (tf.m_new.tile, ReverseDiagDir (last_dir));
					break;
				}

				/* move to the next tile */
			}

			/* save also tile cost */
			n.m_cost = old_node->m_cost + segment_cost;

			/* compute estimated cost */
			if (is_target) {
				n.m_estimate = n.m_cost;
				TAstar::InsertTarget (n);
			} else {
				n.m_estimate = n.m_cost + this->m_dest.calc_estimate (last_tile, last_dir);
				assert (n.m_estimate >= old_node->m_estimate);
				TAstar::InsertNode(n);
			}
		}
	}

	/** call the node follower */
	static inline void Follow (CYapfRoadT *pf, const Node *n)
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

		bool bDestFound = TAstar::FindPath (Follow, _settings_game.pf.yapf.max_search_nodes);

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
	pf.InsertInitialNode (typename Tpf::Node (NULL, RoadPathPos(),
				PathMPos<RoadPathPos> (tile, trackdirs)));

	/* find the best path */
	path_found = pf.FindPath();

	/* if path not found - return INVALID_TRACKDIR */
	const typename Tpf::Node *n = pf.GetBestNode();
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


static PathMPos<RoadPathPos> FindNearestDepotOrigin (const RoadVehicle *v)
{
	DiagDirection dir;
	Trackdir      td;
	TrackdirBits  trackdirs;

	enum {
		SEL_DIAGDIR,
		SEL_TRACKDIR,
		SEL_TRACKDIRBITS,
	} sel;

	if (v->state == RVSB_WORMHOLE) {
		if ((v->vehstatus & VS_HIDDEN) == 0) {
			/* on a bridge */
			trackdirs = TrackStatusToTrackdirBits(GetTileRoadStatus (v->tile, v->compatible_roadtypes)) & DiagdirReachesTrackdirs(DirToDiagDir(v->direction));
			assert (trackdirs != TRACKDIR_BIT_NONE);
			sel = SEL_TRACKDIRBITS;
		} else {
			/* in a tunnel */
			dir = DirToDiagDir (v->direction);
			sel = SEL_DIAGDIR;
		}
	} else {
		if (v->state == RVSB_IN_DEPOT) {
			/* We'll assume the road vehicle is facing outwards */
			assert (IsRoadDepotTile (v->tile));
			dir = GetGroundDepotDirection (v->tile);
			sel = SEL_DIAGDIR;
		} else if (IsStandardRoadStopTile(v->tile)) {
			/* We'll assume the road vehicle is facing outwards */
			dir = GetRoadStopDir (v->tile); // Road vehicle in a station
			sel = SEL_DIAGDIR;
		} else if (v->state > RVSB_TRACKDIR_MASK) {
			/* Drive-through road stops */
			dir = DirToDiagDir (v->direction);
			sel = SEL_DIAGDIR;
		} else if (IsReversingRoadTrackdir ((Trackdir)v->state)) {
			/* If the vehicle is turning around, it will call the
			 * pathfinder after reversing, so we can use any
			 * available trackdir. */
			trackdirs = TrackStatusToTrackdirBits (GetTileRoadStatus (v->tile, v->compatible_roadtypes));
			dir = TrackdirToExitdir ((Trackdir)v->state);
			trackdirs &= DiagdirReachesTrackdirs (dir);
			if (trackdirs == TRACKDIR_BIT_NONE) {
				/* Long turn at a single-piece road. */
				sel = SEL_DIAGDIR;
			} else {
				sel = SEL_TRACKDIRBITS;
			}
		} else {
			/* Not turning, so use current trackdir. */
			td = (Trackdir)v->state;
			sel = SEL_TRACKDIR;
		}
	}

	switch (sel) {
		case SEL_DIAGDIR:
			td = DiagDirToDiagTrackdir (dir);
			/* fall through */
		case SEL_TRACKDIR:
			return PathMPos<RoadPathPos> (v->tile, td);

		case SEL_TRACKDIRBITS:
			return PathMPos<RoadPathPos> (v->tile, trackdirs);
	};

	NOT_REACHED();
}

template <class Tpf>
static TileIndex FindNearestDepot (const RoadVehicle *v,
	const PathMPos<RoadPathPos> &origin, int max_distance)
{
	Tpf pf (v, true);
	pf.InsertInitialNode (typename Tpf::Node (NULL, RoadPathPos(), origin));

	/* find the best path */
	if (!pf.FindPath()) return INVALID_TILE;

	/* some path found; get found depot tile */
	const typename Tpf::Node *n = pf.GetBestNode();

	if (max_distance > 0 && n->m_cost > max_distance) return INVALID_TILE;

	assert (IsRoadDepotTile (n->m_next.tile));

	return n->m_next.tile;
}

TileIndex YapfRoadVehicleFindNearestDepot(const RoadVehicle *v, uint max_distance)
{
	/* set origin node */
	PathMPos<RoadPathPos> origin (FindNearestDepotOrigin (v));

	/* default is YAPF type 2 */
	typedef TileIndex (*PfnFindNearestDepot) (const RoadVehicle*, const PathMPos<RoadPathPos> &, int);
	PfnFindNearestDepot pfnFindNearestDepot = &FindNearestDepot<CYapfRoad2>;

	/* check if non-default YAPF type should be used */
	if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnFindNearestDepot = &FindNearestDepot<CYapfRoad1>; // Trackdir, allow 90-deg
	}

	return pfnFindNearestDepot (v, origin, max_distance);
}
