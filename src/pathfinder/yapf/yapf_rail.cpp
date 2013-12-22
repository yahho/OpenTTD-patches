/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_rail.cpp The rail pathfinding. */

#include "../../stdafx.h"

#include "yapf.hpp"
#include "yapf_cache.h"
#include "yapf_node_rail.hpp"
#include "yapf_costrail.hpp"
#include "yapf_destrail.hpp"
#include "../../viewport_func.h"
#include "../../newgrf_station.h"
#include "../../station_func.h"

#define DEBUG_YAPF_CACHE 0

#if DEBUG_YAPF_CACHE
template <typename Tpf> void DumpState(Tpf &pf1, Tpf &pf2)
{
	DumpTarget dmp1, dmp2;
	pf1.DumpBase(dmp1);
	pf2.DumpBase(dmp2);
	FILE *f1 = fopen("yapf1.txt", "wt");
	FILE *f2 = fopen("yapf2.txt", "wt");
	fwrite(dmp1.m_out.Data(), 1, dmp1.m_out.Size(), f1);
	fwrite(dmp2.m_out.Data(), 1, dmp2.m_out.Size(), f2);
	fclose(f1);
	fclose(f2);
}
#endif

int _total_pf_time_us = 0;

template <class Types>
class CYapfReserveTrack
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::NodeList::Titem Node;        ///< this will be our node type

protected:
	/** to access inherited pathfinder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

private:
	PFPos     m_res_dest;         ///< The reservation target
	Node      *m_res_node;        ///< The reservation target node
	TileIndex m_origin_tile;      ///< Tile our reservation will originate from

	bool FindSafePositionProc(const PFPos &pos, PFPos*)
	{
		if (IsSafeWaitingPosition(Yapf().GetVehicle(), pos, !TrackFollower::Allow90degTurns())) {
			m_res_dest = pos;
			return false;   // Stop iterating segment
		}
		return true;
	}

	/** Try to reserve a single track/platform. */
	bool ReserveSingleTrack(const PFPos &pos, PFPos *fail)
	{
		if (!pos.InWormhole() && IsRailStationTile(pos.tile)) {
			TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
			TileIndex t = pos.tile;

			do {
				if (HasStationReservation(t)) {
					/* Platform could not be reserved, undo. */
					TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(pos.td));
					while (t != pos.tile) {
						t = TILE_ADD(t, diff);
						SetRailStationReservation(t, false);
					}
					*fail = pos;
					return false;
				}
				SetRailStationReservation(t, true);
				MarkTileDirtyByTile(t);
				t = TILE_ADD(t, diff);
			} while (IsCompatibleTrainStationTile(t, pos.tile) && t != m_origin_tile);

			TriggerStationRandomisation(NULL, pos.tile, SRT_PATH_RESERVATION);
		} else {
			if (!TryReserveRailTrack(pos)) {
				/* Tile couldn't be reserved, undo. */
				*fail = pos;
				return false;
			}
		}

		return pos != m_res_dest;
	}

	/** Unreserve a single track/platform. Stops when the previous failer is reached. */
	bool UnreserveSingleTrack(const PFPos &pos, PFPos *stop)
	{
		if (stop != NULL && pos == *stop) return false;

		if (!pos.InWormhole() && IsRailStationTile(pos.tile)) {
			TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
			TileIndex     t = pos.tile;
			while (IsCompatibleTrainStationTile(t, pos.tile) && t != m_origin_tile) {
				assert(HasStationReservation(t));
				SetRailStationReservation(t, false);
				t = TILE_ADD(t, diff);
			}
		} else {
			UnreserveRailTrack(pos);
		}

		return pos != m_res_dest;
	}

public:
	/** Set the target to where the reservation should be extended. */
	inline void SetReservationTarget(Node *node, const PFPos &pos)
	{
		m_res_node = node;
		m_res_dest = pos;
	}

	/** Check the node for a possible reservation target. */
	inline void FindSafePositionOnNode(Node *node)
	{
		assert(node->m_parent != NULL);

		/* We will never pass more than two signals, no need to check for a safe tile. */
		if (node->m_parent->m_num_signals_passed >= 2) return;

		if (!node->IterateTiles(Yapf().GetVehicle(), Yapf(), *this, &CYapfReserveTrack<Types>::FindSafePositionProc)) {
			m_res_node = node;
		}
	}

	/** Try to reserve the path till the reservation target. */
	bool TryReservePath(PBSTileInfo *target, TileIndex origin)
	{
		m_origin_tile = origin;

		if (target != NULL) {
			target->pos  = m_res_dest;
			target->okay = false;
		}

		/* Don't bother if the target is reserved. */
		if (!IsWaitingPositionFree(Yapf().GetVehicle(), m_res_dest)) return false;

		PFPos res_fail;

		for (Node *node = m_res_node; node->m_parent != NULL; node = node->m_parent) {
			node->IterateTiles(Yapf().GetVehicle(), Yapf(), *this, &CYapfReserveTrack<Types>::ReserveSingleTrack, &res_fail);
			if (res_fail.tile != INVALID_TILE) {
				/* Reservation failed, undo. */
				Node *failed_node = node;
				for (node = m_res_node; node != failed_node; node = node->m_parent) {
					node->IterateTiles(Yapf().GetVehicle(), Yapf(), *this, &CYapfReserveTrack<Types>::UnreserveSingleTrack, NULL);
				}
				failed_node->IterateTiles(Yapf().GetVehicle(), Yapf(), *this, &CYapfReserveTrack<Types>::UnreserveSingleTrack, &res_fail);
				return false;
			}
		}

		if (target != NULL) target->okay = true;

		if (Yapf().CanUseGlobalCache(*m_res_node)) {
			YapfNotifyTrackLayoutChange(INVALID_TILE, INVALID_TRACK);
		}

		return true;
	}
};

template <class Types>
class CYapfFollowAnyDepotRailT
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::NodeList::Titem Node;        ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node& old_node)
	{
		TrackFollower F(Yapf().GetVehicle());
		if (F.Follow(old_node.GetLastPos())) {
			Yapf().AddMultipleNodes(&old_node, F);
		}
	}

	/** return debug report character to identify the transportation type */
	inline char TransportTypeChar() const
	{
		return 't';
	}

	static bool stFindNearestDepotTwoWay(const Train *v, const PFPos &pos1, const PFPos &pos2, int max_penalty, int reverse_penalty, TileIndex *depot_tile, bool *reversed)
	{
		Tpf pf1;
		/*
		 * With caching enabled it simply cannot get a reliable result when you
		 * have limited the distance a train may travel. This means that the
		 * cached result does not match uncached result in all cases and that
		 * causes desyncs. So disable caching when finding for a depot that is
		 * nearby. This only happens with automatic servicing of vehicles,
		 * so it will only impact performance when you do not manually set
		 * depot orders and you do not disable automatic servicing.
		 */
		if (max_penalty != 0) pf1.DisableCache(true);
		bool result1 = pf1.FindNearestDepotTwoWay(v, pos1, pos2, max_penalty, reverse_penalty, depot_tile, reversed);

#if DEBUG_YAPF_CACHE
		Tpf pf2;
		TileIndex depot_tile2 = INVALID_TILE;
		bool reversed2 = false;
		pf2.DisableCache(true);
		bool result2 = pf2.FindNearestDepotTwoWay(v, pos1, pos2, max_penalty, reverse_penalty, &depot_tile2, &reversed2);
		if (result1 != result2 || (result1 && (*depot_tile != depot_tile2 || *reversed != reversed2))) {
			DEBUG(yapf, 0, "CACHE ERROR: FindNearestDepotTwoWay() = [%s, %s]", result1 ? "T" : "F", result2 ? "T" : "F");
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	inline bool FindNearestDepotTwoWay(const Train *v, const PFPos &pos1, const PFPos &pos2, int max_penalty, int reverse_penalty, TileIndex *depot_tile, bool *reversed)
	{
		/* set origin and destination nodes */
		Yapf().SetOrigin(pos1, pos2, reverse_penalty, true);
		Yapf().SetDestination(v);
		Yapf().SetMaxCost(max_penalty);

		/* find the best path */
		bool bFound = Yapf().FindPath(v);
		if (!bFound) return false;

		/* some path found
		 * get found depot tile */
		Node *n = Yapf().GetBestNode();
		*depot_tile = n->GetLastPos().tile;

		/* walk through the path back to the origin */
		Node *pNode = n;
		while (pNode->m_parent != NULL) {
			pNode = pNode->m_parent;
		}

		/* if the origin node is our front vehicle tile/Trackdir then we didn't reverse
		 * but we can also look at the cost (== 0 -> not reversed, == reverse_penalty -> reversed) */
		*reversed = (pNode->m_cost != 0);

		return true;
	}
};

template <class Types>
class CYapfFollowAnySafeTileRailT : public CYapfReserveTrack<Types>
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::NodeList::Titem Node;        ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node& old_node)
	{
		TrackFollower F(Yapf().GetVehicle(), Yapf().GetCompatibleRailTypes());
		if (F.Follow(old_node.GetLastPos()) && F.MaskReservedTracks()) {
			Yapf().AddMultipleNodes(&old_node, F);
		}
	}

	/** Return debug report character to identify the transportation type */
	inline char TransportTypeChar() const
	{
		return 't';
	}

	static bool stFindNearestSafeTile(const Train *v, const PFPos &pos, bool override_railtype)
	{
		/* Create pathfinder instance */
		Tpf pf1;
#if !DEBUG_YAPF_CACHE
		bool result1 = pf1.FindNearestSafeTile(v, pos, override_railtype, false);

#else
		bool result2 = pf1.FindNearestSafeTile(v, pos, override_railtype, true);
		Tpf pf2;
		pf2.DisableCache(true);
		bool result1 = pf2.FindNearestSafeTile(v, pos, override_railtype, false);
		if (result1 != result2) {
			DEBUG(yapf, 0, "CACHE ERROR: FindSafeTile() = [%s, %s]", result2 ? "T" : "F", result1 ? "T" : "F");
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	bool FindNearestSafeTile(const Train *v, const PFPos &pos, bool override_railtype, bool dont_reserve)
	{
		/* Set origin and destination. */
		Yapf().SetOrigin(pos);
		Yapf().SetDestination(v, override_railtype);

		bool bFound = Yapf().FindPath(v);
		if (!bFound) return false;

		/* Found a destination, set as reservation target. */
		Node *pNode = Yapf().GetBestNode();
		this->SetReservationTarget(pNode, pNode->GetLastPos());

		/* Walk through the path back to the origin. */
		Node *pPrev = NULL;
		while (pNode->m_parent != NULL) {
			pPrev = pNode;
			pNode = pNode->m_parent;

			this->FindSafePositionOnNode(pPrev);
		}

		return dont_reserve || this->TryReservePath(NULL, pNode->GetLastPos().tile);
	}
};

template <class Types>
class CYapfFollowRailT : public CYapfReserveTrack<Types>
{
public:
	typedef typename Types::Tpf Tpf;                     ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::NodeList::Titem Node;        ///< this will be our node type
	typedef typename Node::Key Key;                      ///< key to hash tables

protected:
	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/**
	 * Called by YAPF to move from the given node to the next tile. For each
	 *  reachable trackdir on the new tile creates new node, initializes it
	 *  and adds it to the open list by calling Yapf().AddNewNode(n)
	 */
	inline void PfFollowNode(Node& old_node)
	{
		TrackFollower F(Yapf().GetVehicle());
		if (F.Follow(old_node.GetLastPos())) {
			Yapf().AddMultipleNodes(&old_node, F);
		}
	}

	/** return debug report character to identify the transportation type */
	inline char TransportTypeChar() const
	{
		return 't';
	}

	static Trackdir stChooseRailTrack(const Train *v, bool &path_found, bool reserve_track, PBSTileInfo *target)
	{
		/* create pathfinder instance */
		Tpf pf1;
#if !DEBUG_YAPF_CACHE
		Trackdir result1 = pf1.ChooseRailTrack(v, path_found, reserve_track, target);

#else
		Trackdir result1 = pf1.ChooseRailTrack(v, path_found, false, NULL);
		Tpf pf2;
		pf2.DisableCache(true);
		Trackdir result2 = pf2.ChooseRailTrack(v, path_found, reserve_track, target);
		if (result1 != result2) {
			DEBUG(yapf, 0, "CACHE ERROR: ChooseRailTrack() = [%d, %d]", result1, result2);
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	inline Trackdir ChooseRailTrack(const Train *v, bool &path_found, bool reserve_track, PBSTileInfo *target)
	{
		if (target != NULL) target->pos.tile = INVALID_TILE;

		/* set origin and destination nodes */
		PFPos origin;
		FollowTrainReservation(v, &origin);
		Yapf().SetOrigin(origin);
		Yapf().SetDestination(v);

		/* find the best path */
		path_found = Yapf().FindPath(v);

		/* if path not found - return INVALID_TRACKDIR */
		Trackdir next_trackdir = INVALID_TRACKDIR;
		Node *pNode = Yapf().GetBestNode();
		if (pNode != NULL) {
			/* reserve till end of path */
			this->SetReservationTarget(pNode, pNode->GetLastPos());

			/* path was found or at least suggested
			 * walk through the path back to the origin */
			Node *pPrev = NULL;
			while (pNode->m_parent != NULL) {
				pPrev = pNode;
				pNode = pNode->m_parent;

				this->FindSafePositionOnNode(pPrev);
			}
			/* return trackdir from the best origin node (one of start nodes) */
			Node& best_next_node = *pPrev;
			next_trackdir = best_next_node.GetPos().td;

			if (reserve_track && path_found) this->TryReservePath(target, pNode->GetLastPos().tile);
		}

		/* Treat the path as found if stopped on the first two way signal(s). */
		path_found |= Yapf().m_stopped_on_first_two_way_signal;
		return next_trackdir;
	}

	static bool stCheckReverseTrain(const Train *v, const PFPos &pos1, const PFPos &pos2, int reverse_penalty)
	{
		Tpf pf1;
		bool result1 = pf1.CheckReverseTrain(v, pos1, pos2, reverse_penalty);

#if DEBUG_YAPF_CACHE
		Tpf pf2;
		pf2.DisableCache(true);
		bool result2 = pf2.CheckReverseTrain(v, pos1, pos2, reverse_penalty);
		if (result1 != result2) {
			DEBUG(yapf, 0, "CACHE ERROR: CheckReverseTrain() = [%s, %s]", result1 ? "T" : "F", result2 ? "T" : "F");
			DumpState(pf1, pf2);
		}
#endif

		return result1;
	}

	inline bool CheckReverseTrain(const Train *v, const PFPos &pos1, const PFPos &pos2, int reverse_penalty)
	{
		/* create pathfinder instance
		 * set origin and destination nodes */
		Yapf().SetOrigin(pos1, pos2, reverse_penalty, false);
		Yapf().SetDestination(v);

		/* find the best path */
		bool bFound = Yapf().FindPath(v);

		if (!bFound) return false;

		/* path was found
		 * walk through the path back to the origin */
		Node *pNode = Yapf().GetBestNode();
		while (pNode->m_parent != NULL) {
			pNode = pNode->m_parent;
		}

		/* check if it was reversed origin */
		Node& best_org_node = *pNode;
		bool reversed = (best_org_node.m_cost != 0);
		return reversed;
	}
};

template <class Tpf_, class Ttrack_follower, class Tnode_list, template <class Types> class TdestinationT, template <class Types> class TfollowT>
struct CYapfRail_TypesT
{
	typedef CYapfRail_TypesT<Tpf_, Ttrack_follower, Tnode_list, TdestinationT, TfollowT>  Types;

	typedef Tpf_                                Tpf;
	typedef Ttrack_follower                     TrackFollower;
	typedef Tnode_list                          NodeList;
	typedef Train                               VehicleType;
	typedef CYapfBaseT<Types>                   PfBase;
	typedef TfollowT<Types>                     PfFollow;
	typedef CYapfOriginTileTwoWayT<Types>       PfOrigin;
	typedef TdestinationT<Types>                PfDestination;
	typedef CYapfSegmentCostCacheGlobalT<Types> PfCache;
	typedef CYapfCostRailT<Types>               PfCost;
};

struct CYapfRail1         : CYapfT<CYapfRail_TypesT<CYapfRail1        , CFollowTrackRail90  , CRailNodeListTrackDir, CYapfDestinationTileOrStationRailT, CYapfFollowRailT> > {};
struct CYapfRail2         : CYapfT<CYapfRail_TypesT<CYapfRail2        , CFollowTrackRailNo90, CRailNodeListTrackDir, CYapfDestinationTileOrStationRailT, CYapfFollowRailT> > {};

struct CYapfAnyDepotRail1 : CYapfT<CYapfRail_TypesT<CYapfAnyDepotRail1, CFollowTrackRail90  , CRailNodeListTrackDir, CYapfDestinationAnyDepotRailT     , CYapfFollowAnyDepotRailT> > {};
struct CYapfAnyDepotRail2 : CYapfT<CYapfRail_TypesT<CYapfAnyDepotRail2, CFollowTrackRailNo90, CRailNodeListTrackDir, CYapfDestinationAnyDepotRailT     , CYapfFollowAnyDepotRailT> > {};

struct CYapfAnySafeTileRail1 : CYapfT<CYapfRail_TypesT<CYapfAnySafeTileRail1, CFollowTrackFreeRail90  , CRailNodeListTrackDir, CYapfDestinationAnySafeTileRailT , CYapfFollowAnySafeTileRailT> > {};
struct CYapfAnySafeTileRail2 : CYapfT<CYapfRail_TypesT<CYapfAnySafeTileRail2, CFollowTrackFreeRailNo90, CRailNodeListTrackDir, CYapfDestinationAnySafeTileRailT , CYapfFollowAnySafeTileRailT> > {};


Trackdir YapfTrainChooseTrack(const Train *v, bool &path_found, bool reserve_track, PBSTileInfo *target)
{
	/* default is YAPF type 2 */
	typedef Trackdir (*PfnChooseRailTrack)(const Train*, bool&, bool, PBSTileInfo*);
	PfnChooseRailTrack pfnChooseRailTrack = &CYapfRail1::stChooseRailTrack;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnChooseRailTrack = &CYapfRail2::stChooseRailTrack; // Trackdir, forbid 90-deg
	}

	return pfnChooseRailTrack(v, path_found, reserve_track, target);
}

bool YapfTrainCheckReverse(const Train *v)
{
	const Train *last_veh = v->Last();

	/* tiles where front and back are */
	PFPos pos = v->GetPos();
	PFPos rev = last_veh->GetReversePos();

	int reverse_penalty = 0;

	if (pos.InWormhole()) {
		/* front in tunnel / on bridge */
		assert(TrackdirToExitdir(pos.td) == ReverseDiagDir(GetTunnelBridgeDirection(pos.wormhole)));

		/* Current position of the train in the wormhole */
		TileIndex cur_tile = TileVirtXY(v->x_pos, v->y_pos);

		/* Add distance to drive in the wormhole as penalty for the forward path, i.e. bonus for the reverse path
		 * Note: Negative penalties are ok for the start tile. */
		reverse_penalty -= DistanceManhattan(cur_tile, pos.tile) * YAPF_TILE_LENGTH;
	}

	if (rev.InWormhole()) {
		/* back in tunnel / on bridge */
		assert(TrackdirToExitdir(rev.td) == ReverseDiagDir(GetTunnelBridgeDirection(rev.wormhole)));

		/* Current position of the last wagon in the wormhole */
		TileIndex cur_tile = TileVirtXY(last_veh->x_pos, last_veh->y_pos);

		/* Add distance to drive in the wormhole as penalty for the revere path. */
		reverse_penalty += DistanceManhattan(cur_tile, rev.tile) * YAPF_TILE_LENGTH;
	}

	typedef bool (*PfnCheckReverseTrain)(const Train*, const PFPos&, const PFPos&, int);
	PfnCheckReverseTrain pfnCheckReverseTrain = CYapfRail1::stCheckReverseTrain;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnCheckReverseTrain = &CYapfRail2::stCheckReverseTrain; // Trackdir, forbid 90-deg
	}

	/* slightly hackish: If the pathfinders finds a path, the cost of the first node is tested to distinguish between forward- and reverse-path. */
	if (reverse_penalty == 0) reverse_penalty = 1;

	bool reverse = pfnCheckReverseTrain(v, pos, rev, reverse_penalty);

	return reverse;
}

bool YapfTrainFindNearestDepot(const Train *v, uint max_penalty, FindDepotData *res)
{
	PFPos origin;
	FollowTrainReservation(v, &origin);
	PFPos rev = v->Last()->GetReversePos();

	typedef bool (*PfnFindNearestDepotTwoWay)(const Train*, const PFPos&, const PFPos&, int, int, TileIndex*, bool*);
	PfnFindNearestDepotTwoWay pfnFindNearestDepotTwoWay = &CYapfAnyDepotRail1::stFindNearestDepotTwoWay;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnFindNearestDepotTwoWay = &CYapfAnyDepotRail2::stFindNearestDepotTwoWay; // Trackdir, forbid 90-deg
	}

	return pfnFindNearestDepotTwoWay(v, origin, rev, max_penalty, YAPF_INFINITE_PENALTY, &res->tile, &res->reverse);
}

bool YapfTrainFindNearestSafeTile(const Train *v, const PFPos &pos, bool override_railtype)
{
	typedef bool (*PfnFindNearestSafeTile)(const Train*, const PFPos&, bool);
	PfnFindNearestSafeTile pfnFindNearestSafeTile = CYapfAnySafeTileRail1::stFindNearestSafeTile;

	/* check if non-default YAPF type needed */
	if (_settings_game.pf.forbid_90_deg) {
		pfnFindNearestSafeTile = &CYapfAnySafeTileRail2::stFindNearestSafeTile;
	}

	return pfnFindNearestSafeTile(v, pos, override_railtype);
}

/** if any track changes, this counter is incremented - that will invalidate segment cost cache */
int CSegmentCostCacheBase::s_rail_change_counter = 0;

void YapfNotifyTrackLayoutChange(TileIndex tile, Track track)
{
	CSegmentCostCacheBase::NotifyTrackLayoutChange(tile, track);
}
