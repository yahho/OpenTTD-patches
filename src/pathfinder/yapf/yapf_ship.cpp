/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_ship.cpp Implementation of YAPF for ships. */

#include "../../stdafx.h"
#include "../../ship.h"

#include "yapf.hpp"
#include "yapf_node_ship.hpp"

/** YAPF class for ships */
template <class Types>
class CYapfShipT : public Types::Astar
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::Astar::Node Node;            ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	const YAPFSettings *m_settings; ///< current settings (_settings_game.yapf)
	const Ship         *m_veh;      ///< vehicle that we are trying to drive

protected:
	Station *m_dest_station;
	TileIndex m_dest_tile;

	CYapfShipT()
		: m_settings(&_settings_game.pf.yapf)
		, m_veh(NULL)
	{
	}

	/** return current settings (can be custom - company based - but later) */
	inline const YAPFSettings& PfGetSettings() const
	{
		return *m_settings;
	}

	const Ship * GetVehicle() const
	{
		return m_veh;
	}

public:
	/** set the destination tile */
	void SetDestination(const Ship *v)
	{
		if (v->current_order.IsType(OT_GOTO_STATION)) {
			m_dest_station = Station::Get(v->current_order.GetDestination());
			m_dest_tile    = m_dest_station->GetClosestTile(v->tile, STATION_DOCK);
		} else {
			m_dest_station = NULL;
			m_dest_tile    = v->dest_tile;
		}
	}

	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node& old_node)
	{
		TrackFollower F(GetVehicle());
		if (!F.Follow(old_node.GetPos())) return;

		bool is_choice = !F.m_new.is_single();
		PathPos pos = F.m_new;
		for (TrackdirBits rtds = F.m_new.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.td = FindFirstTrackdir(rtds);
			Node& n = *Types::Astar::CreateNewNode(&old_node, pos, is_choice);

			/* evaluate the node */
			bool bValid = PfCalcCost(n, &F) && PfCalcEstimate(n);

			/* have the cost or estimate callbacks marked this node as invalid? */
			if (!bValid) continue;

			/* detect the destination */
			if (PfDetectDestination(n)) {
				this->FoundTarget(&n);
			} else {
				this->InsertNode(&n);
			}
		}
	}

	/**
	 * Called by YAPF to calculate the cost from the origin to the given node.
	 *  Calculates only the cost of given node, adds it to the parent node cost
	 *  and stores the result into Node::m_cost member
	 */
	inline bool PfCalcCost(Node& n, const TrackFollower *tf)
	{
		/* base tile cost depending on distance */
		int c = IsDiagonalTrackdir(n.GetPos().td) ? YAPF_TILE_LENGTH : YAPF_TILE_CORNER_LENGTH;
		/* additional penalty for curves */
		if (n.GetPos().td != NextTrackdir(n.m_parent->GetPos().td)) {
			/* new trackdir does not match the next one when going straight */
			c += YAPF_TILE_LENGTH;
		}

		/* Skipped tile cost for aqueducts. */
		c += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

		/* Ocean/canal speed penalty. */
		const ShipVehicleInfo *svi = ShipVehInfo(GetVehicle()->engine_type);
		byte speed_frac = (GetEffectiveWaterClass(n.GetPos().tile) == WATER_CLASS_SEA) ? svi->ocean_speed_frac : svi->canal_speed_frac;
		if (speed_frac > 0) c += YAPF_TILE_LENGTH * (1 + tf->m_tiles_skipped) * speed_frac / (256 - speed_frac);

		/* apply it */
		n.m_cost = n.m_parent->m_cost + c;
		return true;
	}

	inline bool PfDetectDestination(const PathPos &pos)
	{
		if (pos.in_wormhole()) return false;

		if (m_dest_station == NULL) return pos.tile == m_dest_tile;

		return m_dest_station->IsDockingTile(pos.tile);
	}

	/** Called by YAPF to detect if node ends in the desired destination */
	inline bool PfDetectDestination(Node& n)
	{
		return PfDetectDestination(n.GetPos());
	}

	/**
	 * Called by YAPF to calculate cost estimate. Calculates distance to the destination
	 *  adds it to the actual cost from origin and stores the sum to the Node::m_estimate
	 */
	inline bool PfCalcEstimate(Node& n)
	{
		static const int dg_dir_to_x_offs[] = {-1, 0, 1, 0};
		static const int dg_dir_to_y_offs[] = {0, 1, 0, -1};
		if (PfDetectDestination(n)) {
			n.m_estimate = n.m_cost;
			return true;
		}

		TileIndex tile = n.GetPos().tile;
		DiagDirection exitdir = TrackdirToExitdir(n.GetPos().td);
		int x1 = 2 * TileX(tile) + dg_dir_to_x_offs[(int)exitdir];
		int y1 = 2 * TileY(tile) + dg_dir_to_y_offs[(int)exitdir];
		int x2 = 2 * TileX(m_dest_tile);
		int y2 = 2 * TileY(m_dest_tile);
		int dx = abs(x1 - x2);
		int dy = abs(y1 - y2);
		int dmin = min(dx, dy);
		int dxy = abs(dx - dy);
		int d = dmin * YAPF_TILE_CORNER_LENGTH + (dxy - 1) * (YAPF_TILE_LENGTH / 2);
		n.m_estimate = n.m_cost + d;
		assert(n.m_estimate >= n.m_parent->m_estimate);
		return true;
	}

	/**
	 * Called by YAPF to attach cached or local segment cost data to the given node.
	 *  @return true if globally cached data were used or false if local data was used
	 */
	inline bool PfNodeCacheFetch(Node& n)
	{
		return false;
	}

	/** call the node follower */
	static inline void Follow (Tpf *pf, Node *n)
	{
		pf->PfFollowNode(*n);
	}

	/**
	 * Main pathfinder routine:
	 *   - set startup node(s)
	 *   - main loop that stops if:
	 *      - the destination was found
	 *      - or the open list is empty (no route to destination).
	 *      - or the maximum amount of loops reached - m_max_search_nodes (default = 10000)
	 * @return true if the path was found
	 */
	inline bool FindPath(const Ship *v)
	{
		m_veh = v;

#ifndef NO_DEBUG_MESSAGES
		CPerformanceTimer perf;
		perf.Start();
#endif /* !NO_DEBUG_MESSAGES */

		bool bDestFound = Types::Astar::FindPath (Follow, PfGetSettings().max_search_nodes);

#ifndef NO_DEBUG_MESSAGES
		perf.Stop();
		if (_debug_yapf_level >= 2) {
			int t = perf.Get(1000000);
			_total_pf_time_us += t;

			if (_debug_yapf_level >= 3) {
				UnitID veh_idx = (m_veh != NULL) ? m_veh->unitnumber : 0;
				int cost = bDestFound ? Types::Astar::best->m_cost : -1;
				int dist = bDestFound ? Types::Astar::best->m_estimate - Types::Astar::best->m_cost : -1;

				DEBUG(yapf, 3, "[YAPFw]%c%4d- %d us - %d rounds - %d open - %d closed - CHR  0.0%% - C %d D %d - c0(sc0, ts0, o0) -- ",
					bDestFound ? '-' : '!', veh_idx, t, Types::Astar::num_steps, Types::Astar::OpenCount(), Types::Astar::ClosedCount(),
					cost, dist
				);
			}
		}
#endif /* !NO_DEBUG_MESSAGES */
		return bDestFound;
	}
};

/**
 * Config struct of YAPF for ships.
 *  Defines all 6 base YAPF modules as classes providing services for CYapfBaseT.
 */
template <class Tpf_, class Ttrack_follower, class TAstar>
struct CYapfShip_TypesT
{
	/** Types - shortcut for this struct type */
	typedef CYapfShip_TypesT<Tpf_, Ttrack_follower, TAstar>  Types;

	/** Tpf - pathfinder type */
	typedef Tpf_                              Tpf;
	/** track follower helper class */
	typedef Ttrack_follower                   TrackFollower;
	/** node list type */
	typedef TAstar                            Astar;
};

/* YAPF type 1 - uses TileIndex/Trackdir as Node key, allows 90-deg turns */
struct CYapfShip1
	: CYapfBaseT<CYapfShip_TypesT<CYapfShip1, CFollowTrackWater90, AstarShipTrackDir> >
	, CYapfShipT<CYapfShip_TypesT<CYapfShip1, CFollowTrackWater90, AstarShipTrackDir> >
{
};

/* YAPF type 2 - uses TileIndex/DiagDirection as Node key, allows 90-deg turns */
struct CYapfShip2
	: CYapfBaseT<CYapfShip_TypesT<CYapfShip2, CFollowTrackWater90, AstarShipExitDir> >
	, CYapfShipT<CYapfShip_TypesT<CYapfShip2, CFollowTrackWater90, AstarShipExitDir> >
{
};

/* YAPF type 3 - uses TileIndex/Trackdir as Node key, forbids 90-deg turns */
struct CYapfShip3
	: CYapfBaseT<CYapfShip_TypesT<CYapfShip3, CFollowTrackWaterNo90, AstarShipTrackDir> >
	, CYapfShipT<CYapfShip_TypesT<CYapfShip3, CFollowTrackWaterNo90, AstarShipTrackDir> >
{
};


template <class Tpf>
static Trackdir ChooseShipTrack(const Ship *v, const PathPos &pos, DiagDirection enterdir, TrackdirBits trackdirs, bool &path_found)
{
	/* create pathfinder instance */
	Tpf pf;
	/* set origin and destination nodes */
	pf.InsertInitialNode (pf.CreateNewNode (NULL, pos, false));
	pf.SetDestination(v);
	/* find best path */
	path_found = pf.FindPath(v);

	typename Tpf::TT::Astar::Node *n = pf.GetBestNode();
	if (n == NULL) return INVALID_TRACKDIR; // path not found
	assert (n->m_parent != NULL);

	/* walk through the path back to the origin */
	while (n->m_parent->m_parent != NULL) {
		n = n->m_parent;
	}

	/* return trackdir from the best next node (direct child of origin) */
	assert(n->m_parent->GetPos().tile == pos.tile);
	return n->GetPos().td;
}

/** Ship controller helper - path finder invoker */
Trackdir YapfShipChooseTrack(const Ship *v, TileIndex tile, DiagDirection enterdir, TrackdirBits trackdirs, bool &path_found)
{
	/* move back to the old tile/trackdir (where ship is coming from) */
	PathPos pos = v->GetPos();
	assert(pos.tile == TileAddByDiagDir (tile, ReverseDiagDir(enterdir)));
	assert(IsValidTrackdir(pos.td));

	/* handle special case - when next tile is destination tile */
	if (tile == v->dest_tile) {
		/* use vehicle's current direction if that's possible, otherwise use first usable one. */
		Trackdir veh_dir = pos.td;
		return ((trackdirs & TrackdirToTrackdirBits(veh_dir)) != 0) ? veh_dir : FindFirstTrackdir(trackdirs);
	}

	/* default is YAPF type 2 */
	typedef Trackdir (*PfnChooseShipTrack)(const Ship*, const PathPos&, DiagDirection, TrackdirBits, bool &path_found);
	PfnChooseShipTrack pfnChooseShipTrack = ChooseShipTrack<CYapfShip2>; // default: ExitDir, allow 90-deg

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnChooseShipTrack = &ChooseShipTrack<CYapfShip3>; // Trackdir, forbid 90-deg
	} else if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnChooseShipTrack = &ChooseShipTrack<CYapfShip1>; // Trackdir, allow 90-deg
	}

	return pfnChooseShipTrack(v, pos, enterdir, trackdirs, path_found);
}


/**
 * Check whether a ship should reverse to reach its destination.
 * Called when leaving depot.
 * @param v Ship
 * @param pos Current position
 * @return true if the reverse direction is better
 */
template <class Tpf>
static bool CheckShipReverse(const Ship *v, const PathPos &pos)
{
	/* create pathfinder instance */
	Tpf pf;
	/* set origin and destination nodes */
	pf.InsertInitialNode (pf.CreateNewNode (NULL, pos, true));
	pf.InsertInitialNode (pf.CreateNewNode (NULL, PathPos(pos.tile, ReverseTrackdir(pos.td)), true));
	pf.SetDestination(v);
	/* find best path */
	if (!pf.FindPath(v)) return false;

	typename Tpf::TT::Astar::Node *n = pf.GetBestNode();
	if (n == NULL) return false;

	/* path was found; walk through the path back to the origin */
	while (n->m_parent != NULL) {
		n = n->m_parent;
	}

	Trackdir best_trackdir = n->GetPos().td;
	assert(best_trackdir == pos.td || best_trackdir == ReverseTrackdir(pos.td));
	return best_trackdir != pos.td;
}

bool YapfShipCheckReverse(const Ship *v)
{
	PathPos pos = v->GetPos();

	typedef bool (*PfnCheckReverseShip)(const Ship*, const PathPos&);
	PfnCheckReverseShip pfnCheckReverseShip = CheckShipReverse<CYapfShip2>; // default: ExitDir, allow 90-deg

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnCheckReverseShip = &CheckShipReverse<CYapfShip3>; // Trackdir, forbid 90-deg
	} else if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnCheckReverseShip = &CheckShipReverse<CYapfShip1>; // Trackdir, allow 90-deg
	}

	bool reverse = pfnCheckReverseShip(v, pos);

	return reverse;
}
