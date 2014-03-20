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

/** Yapf Node for ships */
template <class Tkey_>
struct CYapfShipNodeT
	: CYapfNodeT<Tkey_, CYapfShipNodeT<Tkey_> >
{
};

/* now define two major node types (that differ by key type) */
typedef CYapfShipNodeT<CYapfNodeKeyExitDir <ShipPathPos> > CYapfShipNodeExitDir;
typedef CYapfShipNodeT<CYapfNodeKeyTrackDir<ShipPathPos> > CYapfShipNodeTrackDir;

/* Default Astar types */
typedef Astar<CYapfShipNodeExitDir , 10, 12> AstarShipExitDir;
typedef Astar<CYapfShipNodeTrackDir, 10, 12> AstarShipTrackDir;


/** YAPF class for ships */
template <class TAstar>
class CYapfShipT : public TAstar
{
public:
	typedef typename TAstar::Node Node; ///< this will be our node type

protected:
	const YAPFSettings *const m_settings; ///< current settings (_settings_game.yapf)
	const Ship         *const m_veh;      ///< vehicle that we are trying to drive
	const Station      *const m_dest_station; ///< destination station, or NULL
	const TileIndex           m_dest_tile;    ///< destination tile
	CFollowTrackWater         tf;             ///< track follower
	const ShipVehicleInfo *const svi;         ///< ship vehicle info

	CYapfShipT (const Ship *ship, bool allow_90deg)
		: m_settings(&_settings_game.pf.yapf)
		, m_veh(ship)
		, m_dest_station (ship->current_order.IsType(OT_GOTO_STATION) ? Station::Get(ship->current_order.GetDestination()) : NULL)
		, m_dest_tile    (ship->current_order.IsType(OT_GOTO_STATION) ? m_dest_station->GetClosestTile(ship->tile, STATION_DOCK) : ship->dest_tile)
		, tf(allow_90deg)
		, svi(ShipVehInfo(ship->engine_type))
	{
	}

public:
	/** Called by the A-star underlying class to find the neighbours of a node. */
	inline void Follow (Node *old_node)
	{
		if (!tf.Follow(old_node->GetPos())) return;

		/* precompute trackdir-independent costs */
		int cc = old_node->m_cost + YAPF_TILE_LENGTH * tf.m_tiles_skipped;

		/* Ocean/canal speed penalty. */
		byte speed_frac = (GetEffectiveWaterClass(tf.m_new.tile) == WATER_CLASS_SEA) ? svi->ocean_speed_frac : svi->canal_speed_frac;
		if (speed_frac > 0) cc += YAPF_TILE_LENGTH * (1 + tf.m_tiles_skipped) * speed_frac / (256 - speed_frac);

		/* the ship track follower does not step into wormholes */
		assert (!tf.m_new.in_wormhole());

		/* detect destination */
		bool is_target = (m_dest_station == NULL) ? (tf.m_new.tile == m_dest_tile) : m_dest_station->IsDockingTile(tf.m_new.tile);

		ShipPathPos pos = tf.m_new;
		for (TrackdirBits rtds = tf.m_new.trackdirs; rtds != TRACKDIR_BIT_NONE; rtds = KillFirstBit(rtds)) {
			pos.set_trackdir (FindFirstTrackdir(rtds));
			Node *n = TAstar::CreateNewNode(old_node, pos);

			/* base tile cost depending on distance */
			int c = IsDiagonalTrackdir(n->GetPos().td) ? YAPF_TILE_LENGTH : YAPF_TILE_CORNER_LENGTH;
			/* additional penalty for curves */
			if (n->GetPos().td != NextTrackdir(n->m_parent->GetPos().td)) {
				/* new trackdir does not match the next one when going straight */
				c += YAPF_TILE_LENGTH;
			}

			/* apply it */
			n->m_cost = cc + c;

			/* compute estimated cost */
			if (is_target) {
				n->m_estimate = n->m_cost;
				TAstar::FoundTarget(n);
			} else {
				n->m_estimate = n->m_cost + YapfCalcEstimate (n->GetPos(), m_dest_tile);
				assert(n->m_estimate >= n->m_parent->m_estimate);
				TAstar::InsertNode(n);
			}
		}
	}


	/** Call the node follower */
	static inline void Follow (CYapfShipT *pf, Node *n)
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

				DEBUG(yapf, 3, "[YAPFw]%c%4d- %d us - %d rounds - %d open - %d closed - CHR  0.0%% - C %d D %d - c0(sc0, ts0, o0) -- ",
					bDestFound ? '-' : '!', m_veh->unitnumber, t, TAstar::num_steps, TAstar::OpenCount(), TAstar::ClosedCount(),
					cost, dist
				);
			}
		}
#endif /* !NO_DEBUG_MESSAGES */
		return bDestFound;
	}
};

/* Template proxy */
template <class TAstar, bool allow_90deg>
struct CYapfShip : CYapfShipT<TAstar>
{
	CYapfShip (const Ship *ship)
		: CYapfShipT<TAstar> (ship, allow_90deg)
	{
	}
};

/* YAPF type 1 - uses TileIndex/Trackdir as Node key, allows 90-deg turns */
typedef CYapfShip <AstarShipTrackDir, true>  CYapfShip1;

/* YAPF type 2 - uses TileIndex/DiagDirection as Node key, allows 90-deg turns */
typedef CYapfShip <AstarShipExitDir,  true>  CYapfShip2;

/* YAPF type 3 - uses TileIndex/Trackdir as Node key, forbids 90-deg turns */
typedef CYapfShip <AstarShipTrackDir, false> CYapfShip3;


template <class Tpf>
static Trackdir ChooseShipTrack(const Ship *v, const ShipPathPos &pos, TrackdirBits trackdirs, bool &path_found)
{
	/* create pathfinder instance */
	Tpf pf (v);
	/* set origin node */
	pf.InsertInitialNode (pf.CreateNewNode (NULL, pos));
	/* find best path */
	path_found = pf.FindPath();

	typename Tpf::Node *n = pf.GetBestNode();
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
	ShipPathPos pos = v->GetPos();
	assert(pos.tile == TileAddByDiagDir (tile, ReverseDiagDir(enterdir)));
	assert(IsValidTrackdir(pos.td));

	/* handle special case - when next tile is destination tile */
	if (v->current_order.IsType(OT_GOTO_STATION) ?
			Station::Get(v->current_order.GetDestination())->IsDockingTile(tile) :
			tile == v->dest_tile) {
		/* use vehicle's current direction if that's possible, otherwise use first usable one. */
		Trackdir veh_dir = pos.td;
		return ((trackdirs & TrackdirToTrackdirBits(veh_dir)) != 0) ? veh_dir : FindFirstTrackdir(trackdirs);
	}

	/* default is YAPF type 2 */
	typedef Trackdir (*PfnChooseShipTrack)(const Ship*, const ShipPathPos&, TrackdirBits, bool &path_found);
	PfnChooseShipTrack pfnChooseShipTrack = ChooseShipTrack<CYapfShip2>; // default: ExitDir, allow 90-deg

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnChooseShipTrack = &ChooseShipTrack<CYapfShip3>; // Trackdir, forbid 90-deg
	} else if (_settings_game.pf.yapf.disable_node_optimization) {
		pfnChooseShipTrack = &ChooseShipTrack<CYapfShip1>; // Trackdir, allow 90-deg
	}

	return pfnChooseShipTrack(v, pos, trackdirs, path_found);
}


/**
 * Check whether a ship should reverse to reach its destination.
 * Called when leaving depot.
 * @param v Ship
 * @param pos Current position
 * @return true if the reverse direction is better
 */
template <class Tpf>
static bool CheckShipReverse(const Ship *v, const ShipPathPos &pos)
{
	/* create pathfinder instance */
	Tpf pf (v);
	/* set origin nodes */
	pf.InsertInitialNode (pf.CreateNewNode (NULL, pos));
	pf.InsertInitialNode (pf.CreateNewNode (NULL, ShipPathPos(pos.tile, ReverseTrackdir(pos.td))));
	/* find best path */
	if (!pf.FindPath()) return false;

	typename Tpf::Node *n = pf.GetBestNode();
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
	ShipPathPos pos = v->GetPos();

	typedef bool (*PfnCheckReverseShip)(const Ship*, const ShipPathPos&);
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
