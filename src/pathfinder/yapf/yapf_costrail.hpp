/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_costrail.hpp Cost determination for rails. */

#ifndef YAPF_COSTRAIL_HPP
#define YAPF_COSTRAIL_HPP

#include "../../map/slope.h"
#include "../../pbs.h"

template <class Types>
class CYapfCostRailT
{
public:
	typedef typename Types::Tpf Tpf;              ///< the pathfinder class (derived from THIS class)
	typedef typename Types::TrackFollower TrackFollower;
	typedef typename Types::NodeList::Titem Node; ///< this will be our node type
	typedef typename Node::Key Key;               ///< key to hash tables
	typedef typename Node::CachedData CachedData;

protected:

	/* Structure used inside PfCalcCost() to keep basic tile information. */
	struct TILE : PFPos {
		TileType    tile_type;
		RailType    rail_type;

		TILE() : PFPos()
		{
			tile_type = TT_GROUND;
			rail_type = INVALID_RAILTYPE;
		}

		TILE(const PFPos &pos) : PFPos(pos)
		{
			if (!pos.InWormhole()) {
				this->tile_type = GetTileType(pos.tile);
				this->rail_type = GetTileRailType(pos.tile, TrackdirToTrack(pos.td));
			} else {
				this->tile_type = TT_GROUND;
				this->rail_type = IsRailwayTile(pos.wormhole) ? GetBridgeRailType(pos.wormhole) : GetRailType(pos.wormhole);
			}
		}

		TILE(const TILE &src) : PFPos(src)
		{
			tile_type = src.tile_type;
			rail_type = src.rail_type;
		}
	};

protected:
	/**
	 * @note maximum cost doesn't work with caching enabled
	 * @todo fix maximum cost failing with caching (e.g. FS#2900)
	 */
	int           m_max_cost;
	CBlobT<int>   m_sig_look_ahead_costs;
	bool          m_disable_cache;

public:
	bool          m_stopped_on_first_two_way_signal;
protected:

	static const int s_max_segment_cost = 10000;

	CYapfCostRailT()
		: m_max_cost(0)
		, m_disable_cache(false)
		, m_stopped_on_first_two_way_signal(false)
	{
		/* pre-compute look-ahead penalties into array */
		int p0 = Yapf().PfGetSettings().rail_look_ahead_signal_p0;
		int p1 = Yapf().PfGetSettings().rail_look_ahead_signal_p1;
		int p2 = Yapf().PfGetSettings().rail_look_ahead_signal_p2;
		int *pen = m_sig_look_ahead_costs.GrowSizeNC(Yapf().PfGetSettings().rail_look_ahead_max_signals);
		for (uint i = 0; i < Yapf().PfGetSettings().rail_look_ahead_max_signals; i++) {
			pen[i] = p0 + i * (p1 + i * p2);
		}
	}

	/** to access inherited path finder */
	Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	inline int SlopeCost(const PFPos &pos)
	{
		CPerfStart perf_cost(Yapf().m_perf_slope_cost);

		if (pos.InWormhole() || !IsDiagonalTrackdir(pos.td)) return 0;

		/* Only rail tracks and bridgeheads can have sloped rail. */
		if (!IsRailwayTile(pos.tile)) return 0;

		bool uphill;
		if (IsTileSubtype(pos.tile, TT_BRIDGE)) {
			/* it is bridge ramp, check if we are entering the bridge */
			DiagDirection dir = GetTunnelBridgeDirection(pos.tile);
			if (dir != TrackdirToExitdir(pos.td)) return 0; // no, we are leaving it, no penalty
			/* we are entering the bridge */
			Slope tile_slope = GetTileSlope(pos.tile);
			Axis axis = DiagDirToAxis(dir);
			uphill = !HasBridgeFlatRamp(tile_slope, axis);
		} else {
			/* not bridge ramp */
			Slope tile_slope = GetTileSlope(pos.tile);
			uphill = IsUphillTrackdir(tile_slope, pos.td); // slopes uphill => apply penalty
		}

		return uphill ? Yapf().PfGetSettings().rail_slope_penalty : 0;
	}

	inline int CurveCost(Trackdir td1, Trackdir td2)
	{
		assert(IsValidTrackdir(td1));
		assert(IsValidTrackdir(td2));
		int cost = 0;
		if (TrackFollower::Allow90degTurns()
				&& ((TrackdirToTrackdirBits(td2) & (TrackdirBits)TrackdirCrossesTrackdirs(td1)) != 0)) {
			/* 90-deg curve penalty */
			cost += Yapf().PfGetSettings().rail_curve90_penalty;
		} else if (td2 != NextTrackdir(td1)) {
			/* 45-deg curve penalty */
			cost += Yapf().PfGetSettings().rail_curve45_penalty;
		}
		return cost;
	}

	inline int SwitchCost(const PFPos &pos1, const PFPos &pos2, DiagDirection exitdir)
	{
		if (!pos1.InWormhole() && IsRailwayTile(pos1.tile) && !pos2.InWormhole() && IsRailwayTile(pos2.tile)) {
			bool t1 = KillFirstBit(GetTrackBits(pos1.tile) & DiagdirReachesTracks(ReverseDiagDir(exitdir))) != TRACK_BIT_NONE;
			bool t2 = KillFirstBit(GetTrackBits(pos2.tile) & DiagdirReachesTracks(exitdir)) != TRACK_BIT_NONE;
			if (t1 && t2) return Yapf().PfGetSettings().rail_doubleslip_penalty;
		}
		return 0;
	}

	/** Return one tile cost (base cost + level crossing penalty). */
	inline int OneTileCost(const PFPos &pos)
	{
		int cost = 0;
		/* set base cost */
		if (IsDiagonalTrackdir(pos.td)) {
			cost += YAPF_TILE_LENGTH;
			if (IsLevelCrossingTile(pos.tile)) {
				/* Increase the cost for level crossings */
				cost += Yapf().PfGetSettings().rail_crossing_penalty;
			}
		} else {
			/* non-diagonal trackdir */
			cost = YAPF_TILE_CORNER_LENGTH;
		}
		return cost;
	}

	/** Check for a reserved station platform. */
	inline bool IsAnyStationTileReserved(const PFPos &pos, int skipped)
	{
		TileIndexDiff diff = TileOffsByDiagDir(TrackdirToExitdir(ReverseTrackdir(pos.td)));
		for (TileIndex tile = pos.tile; skipped >= 0; skipped--, tile += diff) {
			if (HasStationReservation(tile)) return true;
		}
		return false;
	}

	/** The cost for reserved tiles, including skipped ones. */
	inline int ReservationCost(Node& n, const PFPos &pos, int skipped)
	{
		if (n.m_num_signals_passed >= m_sig_look_ahead_costs.Size() / 2) return 0;
		if (!IsPbsSignal(n.m_last_signal_type)) return 0;

		if (!pos.InWormhole() && IsRailStationTile(pos.tile) && IsAnyStationTileReserved(pos, skipped)) {
			return Yapf().PfGetSettings().rail_pbs_station_penalty * (skipped + 1);
		} else if (TrackOverlapsTracks(GetReservedTrackbits(pos.tile), TrackdirToTrack(pos.td))) {
			int cost = Yapf().PfGetSettings().rail_pbs_cross_penalty;
			if (!IsDiagonalTrackdir(pos.td)) cost = (cost * YAPF_TILE_CORNER_LENGTH) / YAPF_TILE_LENGTH;
			return cost * (skipped + 1);
		}
		return 0;
	}

	int SignalCost(Node& n, const PFPos &pos)
	{
		int cost = 0;
		/* if there is one-way signal in the opposite direction, then it is not our way */
		CPerfStart perf_cost(Yapf().m_perf_other_cost);

		if (HasSignalAlongPos(pos)) {
			SignalState sig_state = GetSignalStateByPos(pos);
			SignalType sig_type = GetSignalType(pos);

			n.m_last_signal_type = sig_type;

			/* cache the look-ahead polynomial constant only if we didn't pass more signals than the look-ahead limit is */
			int look_ahead_cost = (n.m_num_signals_passed < m_sig_look_ahead_costs.Size()) ? m_sig_look_ahead_costs.Data()[n.m_num_signals_passed] : 0;
			if (sig_state != SIGNAL_STATE_RED) {
				/* green signal */
				n.flags_u.flags_s.m_last_signal_was_red = false;
				/* negative look-ahead red-signal penalties would cause problems later, so use them as positive penalties for green signal */
				if (look_ahead_cost < 0) {
					/* add its negation to the cost */
					cost -= look_ahead_cost;
				}
			} else {
				/* we have a red signal in our direction
				 * was it first signal which is two-way? */
				if (!IsPbsSignal(sig_type) && Yapf().TreatFirstRedTwoWaySignalAsEOL() && n.flags_u.flags_s.m_choice_seen && HasSignalAgainstPos(pos) && n.m_num_signals_passed == 0) {
					/* yes, the first signal is two-way red signal => DEAD END. Prune this branch... */
					Yapf().PruneIntermediateNodeBranch();
					n.m_segment->m_end_segment_reason |= ESRB_DEAD_END;
					Yapf().m_stopped_on_first_two_way_signal = true;
					return -1;
				}
				n.m_last_red_signal_type = sig_type;
				n.flags_u.flags_s.m_last_signal_was_red = true;

				/* look-ahead signal penalty */
				if (!IsPbsSignal(sig_type) && look_ahead_cost > 0) {
					/* add the look ahead penalty only if it is positive */
					cost += look_ahead_cost;
				}

				/* special signal penalties */
				if (n.m_num_signals_passed == 0) {
					switch (sig_type) {
						case SIGTYPE_COMBO:
						case SIGTYPE_EXIT:   cost += Yapf().PfGetSettings().rail_firstred_exit_penalty; break; // first signal is red pre-signal-exit
						case SIGTYPE_NORMAL:
						case SIGTYPE_ENTRY:  cost += Yapf().PfGetSettings().rail_firstred_penalty; break;
						default: break;
					}
				}
			}

			n.m_num_signals_passed++;
			n.m_segment->m_last_signal = pos;
		} else if (HasSignalAgainstPos(pos)) {
			if (GetSignalType(pos) != SIGTYPE_PBS) {
				/* one-way signal in opposite direction */
				n.m_segment->m_end_segment_reason |= ESRB_DEAD_END;
			} else {
				cost += n.m_num_signals_passed < Yapf().PfGetSettings().rail_look_ahead_max_signals ? Yapf().PfGetSettings().rail_pbs_signal_back_penalty : 0;
			}
		}

		return cost;
	}

	inline int PlatformLengthPenalty(int platform_length)
	{
		int cost = 0;
		const Train *v = Yapf().GetVehicle();
		assert(v != NULL);
		assert(v->type == VEH_TRAIN);
		assert(v->gcache.cached_total_length != 0);
		int missing_platform_length = CeilDiv(v->gcache.cached_total_length, TILE_SIZE) - platform_length;
		if (missing_platform_length < 0) {
			/* apply penalty for longer platform than needed */
			cost += Yapf().PfGetSettings().rail_longer_platform_penalty + Yapf().PfGetSettings().rail_longer_platform_per_tile_penalty * -missing_platform_length;
		} else if (missing_platform_length > 0) {
			/* apply penalty for shorter platform than needed */
			cost += Yapf().PfGetSettings().rail_shorter_platform_penalty + Yapf().PfGetSettings().rail_shorter_platform_per_tile_penalty * missing_platform_length;
		}
		return cost;
	}

public:
	inline void SetMaxCost(int max_cost)
	{
		m_max_cost = max_cost;
	}

	/**
	 * Called by YAPF to calculate the cost from the origin to the given node.
	 *  Calculates only the cost of given node, adds it to the parent node cost
	 *  and stores the result into Node::m_cost member
	 */
	inline bool PfCalcCost(Node &n, const TrackFollower *tf)
	{
		assert(!n.flags_u.flags_s.m_targed_seen);
		assert(tf->m_new.tile == n.GetPos().tile);
		assert(tf->m_new.wormhole == n.GetPos().wormhole);
		assert((TrackdirToTrackdirBits(n.GetPos().td) & tf->m_new.trackdirs) != TRACKDIR_BIT_NONE);

		CPerfStart perf_cost(Yapf().m_perf_cost);

		/* Does the node have some parent node? */
		bool has_parent = (n.m_parent != NULL);

		/* Do we already have a cached segment? */
		CachedData &segment = *n.m_segment;
		bool is_cached_segment = (segment.m_cost >= 0);

		int parent_cost = has_parent ? n.m_parent->m_cost : 0;

		/* Each node cost contains 2 or 3 main components:
		 *  1. Transition cost - cost of the move from previous node (tile):
		 *    - curve cost (or zero for straight move)
		 *  2. Tile cost:
		 *    - base tile cost
		 *      - YAPF_TILE_LENGTH for diagonal tiles
		 *      - YAPF_TILE_CORNER_LENGTH for non-diagonal tiles
		 *    - tile penalties
		 *      - tile slope penalty (upward slopes)
		 *      - red signal penalty
		 *      - level crossing penalty
		 *      - speed-limit penalty (bridges)
		 *      - station platform penalty
		 *      - penalty for reversing in the depot
		 *      - etc.
		 *  3. Extra cost (applies to the last node only)
		 *    - last red signal penalty
		 *    - penalty for too long or too short platform on the destination station
		 */
		int extra_cost = 0;

		/* Segment: one or more tiles connected by contiguous tracks of the same type.
		 * Each segment cost includes 'Tile cost' for all its tiles (including the first
		 * and last), and the 'Transition cost' between its tiles. The first transition
		 * cost of segment entry (move from the 'parent' node) is not included!
		 */
		int segment_entry_cost = 0;
		int segment_cost = 0;

		const Train *v = Yapf().GetVehicle();

		/* start at n and walk to the end of segment */
		TILE cur(n.GetPos());

		EndSegmentReasonBits end_segment_reason = ESRB_NONE;

		TrackFollower tf_local(v, Yapf().GetCompatibleRailTypes(), &Yapf().m_perf_ts_cost);

		TileIndex prev;

		if (has_parent) {
			/* First transition cost goes to segment entry cost */
			PFPos ppos = n.m_parent->GetLastPos();
			segment_entry_cost = Yapf().CurveCost(ppos.td, cur.td);
			segment_entry_cost += Yapf().SwitchCost(ppos, cur, TrackdirToExitdir(ppos.td));

			/* It is the right time now to look if we can reuse the cached segment cost. */
			if (is_cached_segment) {
				/* Yes, we already know the segment cost. */
				segment_cost = segment.m_cost;
				/* We know also the reason why the segment ends. */
				end_segment_reason = segment.m_end_segment_reason;
				/* We will need also some information about the last signal (if it was red). */
				if (segment.m_last_signal.tile != INVALID_TILE) {
					assert(HasSignalAlongPos(segment.m_last_signal));
					SignalState sig_state = GetSignalStateByPos(segment.m_last_signal);
					bool is_red = (sig_state == SIGNAL_STATE_RED);
					n.flags_u.flags_s.m_last_signal_was_red = is_red;
					if (is_red) {
						n.m_last_red_signal_type = GetSignalType(segment.m_last_signal);
					}
				}
				/* No further calculation needed. */
				cur = TILE(n.GetLastPos());
				goto cached_segment;
			}

			prev = ppos.tile;
		} else {
			assert(!is_cached_segment);
			prev = INVALID_TILE;
		}

		for (;;) {
			/* All other tile costs will be calculated here. */
			segment_cost += Yapf().OneTileCost(cur);

			/* If we skipped some tunnel/bridge/station tiles, add their base cost */
			segment_cost += YAPF_TILE_LENGTH * tf->m_tiles_skipped;

			/* Slope cost. */
			segment_cost += Yapf().SlopeCost(cur);

			/* Signal cost (routine can modify segment data). */
			segment_cost += Yapf().SignalCost(n, cur);

			/* Reserved tiles. */
			segment_cost += Yapf().ReservationCost(n, cur, tf->m_tiles_skipped);

			end_segment_reason = segment.m_end_segment_reason;

			/* Tests for 'potential target' reasons to close the segment. */
			if (cur.tile == prev) {
				/* Penalty for reversing in a depot. */
				assert(!cur.InWormhole());
				assert(IsRailDepotTile(cur.tile));
				assert(cur.td == DiagDirToDiagTrackdir(GetGroundDepotDirection(cur.tile)));
				segment_cost += Yapf().PfGetSettings().rail_depot_reverse_penalty;
				/* We will end in this pass (depot is possible target) */
				end_segment_reason |= ESRB_DEPOT;

			} else if (cur.tile_type == TT_STATION && IsRailWaypoint(cur.tile)) {
				if (v->current_order.IsType(OT_GOTO_WAYPOINT) &&
						GetStationIndex(cur.tile) == v->current_order.GetDestination() &&
						!Waypoint::Get(v->current_order.GetDestination())->IsSingleTile()) {
					/* This waypoint is our destination; maybe this isn't an unreserved
					 * one, so check that and if so see that as the last signal being
					 * red. This way waypoints near stations should work better. */
					CFollowTrackRail ft(v);
					ft.SetPos(cur);
					while (ft.FollowNext()) {
						assert(ft.m_old.tile != ft.m_new.tile);
						if (!ft.m_new.IsTrackdirSet()) {
							/* We encountered a junction; it's going to be too complex to
							 * handle this perfectly, so just bail out. There is no simple
							 * free path, so try the other possibilities. */
							break;
						}
						/* If this is a safe waiting position we're done searching for it */
						if (IsSafeWaitingPosition(v, ft.m_new, _settings_game.pf.forbid_90_deg)) break;
					}

					/* In the case this platform is (possibly) occupied we add penalty so the
					 * other platforms of this waypoint are evaluated as well, i.e. we assume
					 * that there is a red signal in the waypoint when it's occupied. */
					if (!ft.m_new.IsTrackdirSet() ||
							!IsFreeSafeWaitingPosition(v, ft.m_new, _settings_game.pf.forbid_90_deg)) {
						extra_cost += Yapf().PfGetSettings().rail_lastred_penalty;
					}
				}
				/* Waypoint is also a good reason to finish. */
				end_segment_reason |= ESRB_WAYPOINT;

			} else if (tf->m_flag == tf->TF_STATION) {
				/* Station penalties. */
				uint platform_length = tf->m_tiles_skipped + 1;
				/* We don't know yet if the station is our target or not. Act like
				 * if it is pass-through station (not our destination). */
				segment_cost += Yapf().PfGetSettings().rail_station_penalty * platform_length;
				/* We will end in this pass (station is possible target) */
				end_segment_reason |= ESRB_STATION;

			} else if (TrackFollower::DoTrackMasking() && cur.tile_type == TT_RAILWAY) {
				/* Searching for a safe tile? */
				if (HasSignalAlongPos(cur) && !IsPbsSignal(GetSignalType(cur))) {
					end_segment_reason |= ESRB_SAFE_TILE;
				}
			}

			/* Apply min/max speed penalties only when inside the look-ahead radius. Otherwise
			 * it would cause desync in MP. */
			if (n.m_num_signals_passed < m_sig_look_ahead_costs.Size())
			{
				int min_speed = 0;
				int max_speed = tf->GetSpeedLimit(&min_speed);
				int max_veh_speed = v->GetDisplayMaxSpeed();
				if (max_speed < max_veh_speed) {
					extra_cost += YAPF_TILE_LENGTH * (max_veh_speed - max_speed) * (4 + tf->m_tiles_skipped) / max_veh_speed;
				}
				if (min_speed > max_veh_speed) {
					extra_cost += YAPF_TILE_LENGTH * (min_speed - max_veh_speed);
				}
			}

			/* Finish if we already exceeded the maximum path cost (i.e. when
			 * searching for the nearest depot). */
			if (m_max_cost > 0 && (parent_cost + segment_entry_cost + segment_cost) > m_max_cost) {
				end_segment_reason |= ESRB_PATH_TOO_LONG;
			}

			/* Move to the next tile/trackdir. */
			tf = &tf_local;
			assert(tf_local.m_veh_owner == v->owner);
			assert(tf_local.m_railtypes == Yapf().GetCompatibleRailTypes());
			assert(tf_local.m_pPerf == &Yapf().m_perf_ts_cost);

			if (!tf_local.Follow(cur)) {
				assert(tf_local.m_err != TrackFollower::EC_NONE);
				/* Can't move to the next tile (EOL?). */
				if (tf_local.m_err == TrackFollower::EC_RAIL_TYPE) {
					end_segment_reason |= ESRB_RAIL_TYPE;
				} else {
					end_segment_reason |= ESRB_DEAD_END;
				}

				if (TrackFollower::DoTrackMasking() && !HasOnewaySignalBlockingPos(cur)) {
					end_segment_reason |= ESRB_SAFE_TILE;
				}
				break;
			}

			/* Check if the next tile is not a choice. */
			if (!tf_local.m_new.IsTrackdirSet()) {
				/* More than one segment will follow. Close this one. */
				end_segment_reason |= ESRB_CHOICE_FOLLOWS;
				break;
			}

			/* Gather the next tile/trackdir/tile_type/rail_type. */
			TILE next(tf_local.m_new);

			if (TrackFollower::DoTrackMasking()) {
				if (HasSignalAlongPos(next) && IsPbsSignal(GetSignalType(next))) {
					/* Possible safe tile. */
					end_segment_reason |= ESRB_SAFE_TILE;
				} else if (HasSignalAgainstPos(next) && GetSignalType(next) == SIGTYPE_PBS_ONEWAY) {
					/* Possible safe tile, but not so good as it's the back of a signal... */
					end_segment_reason |= ESRB_SAFE_TILE | ESRB_DEAD_END;
					extra_cost += Yapf().PfGetSettings().rail_lastred_exit_penalty;
				}
			}

			/* Check the next tile for the rail type. */
			if (next.rail_type != cur.rail_type) {
				/* Segment must consist from the same rail_type tiles. */
				end_segment_reason |= ESRB_RAIL_TYPE;
				break;
			}

			/* Avoid infinite looping. */
			if (next == n.GetPos()) {
				end_segment_reason |= ESRB_INFINITE_LOOP;
				break;
			}

			if (segment_cost > s_max_segment_cost) {
				/* Potentially in the infinite loop (or only very long segment?). We should
				 * not force it to finish prematurely unless we are on a regular tile. */
				if (!tf->m_new.InWormhole() && IsNormalRailTile(tf->m_new.tile)) {
					end_segment_reason |= ESRB_SEGMENT_TOO_LONG;
					break;
				}
			}

			/* Any other reason bit set? */
			if (end_segment_reason != ESRB_NONE) {
				break;
			}

			/* Transition cost (cost of the move from previous tile) */
			segment_cost += Yapf().CurveCost(cur.td, next.td);
			segment_cost += Yapf().SwitchCost(cur, next, TrackdirToExitdir(cur.td));

			/* For the next loop set new prev and cur tile info. */
			prev = cur.tile;
			cur = next;
		} // for (;;)

cached_segment:

		bool target_seen = false;
		if ((end_segment_reason & ESRB_POSSIBLE_TARGET) != ESRB_NONE) {
			/* Depot, station or waypoint. */
			if (Yapf().PfDetectDestination(cur)) {
				/* Destination found. */
				target_seen = true;
			}
		}

		/* Update the segment if needed. */
		if (!is_cached_segment) {
			/* Write back the segment information so it can be reused the next time. */
			segment.m_cost = segment_cost;
			segment.m_end_segment_reason = end_segment_reason & ESRB_CACHED_MASK;
			/* Save end of segment back to the node. */
			n.SetLastTileTrackdir(cur);
		}

		/* Do we have an excuse why not to continue pathfinding in this direction? */
		if (!target_seen && (end_segment_reason & ESRB_ABORT_PF_MASK) != ESRB_NONE) {
			/* Reason to not continue. Stop this PF branch. */
			return false;
		}

		/* Special costs for the case we have reached our target. */
		if (target_seen) {
			n.flags_u.flags_s.m_targed_seen = true;
			/* Last-red and last-red-exit penalties. */
			if (n.flags_u.flags_s.m_last_signal_was_red) {
				if (n.m_last_red_signal_type == SIGTYPE_EXIT) {
					/* last signal was red pre-signal-exit */
					extra_cost += Yapf().PfGetSettings().rail_lastred_exit_penalty;
				} else if (!IsPbsSignal(n.m_last_red_signal_type)) {
					/* Last signal was red, but not exit or path signal. */
					extra_cost += Yapf().PfGetSettings().rail_lastred_penalty;
				}
			}

			/* Station platform-length penalty. */
			if ((end_segment_reason & ESRB_STATION) != ESRB_NONE) {
				const BaseStation *st = BaseStation::GetByTile(n.GetLastPos().tile);
				assert(st != NULL);
				uint platform_length = st->GetPlatformLength(n.GetLastPos().tile, ReverseDiagDir(TrackdirToExitdir(n.GetLastPos().td)));
				/* Reduce the extra cost caused by passing-station penalty (each station receives it in the segment cost). */
				extra_cost -= Yapf().PfGetSettings().rail_station_penalty * platform_length;
				/* Add penalty for the inappropriate platform length. */
				extra_cost += PlatformLengthPenalty(platform_length);
			}
		}

		/* total node cost */
		n.m_cost = parent_cost + segment_entry_cost + segment_cost + extra_cost;

		return true;
	}

	inline bool CanUseGlobalCache(Node& n) const
	{
		return !m_disable_cache
			&& (n.m_parent != NULL)
			&& (n.m_parent->m_num_signals_passed >= m_sig_look_ahead_costs.Size());
	}

	inline void ConnectNodeToCachedData(Node& n, CachedData& ci)
	{
		n.m_segment = &ci;
		if (n.m_segment->m_cost < 0) {
			n.m_segment->m_last = n.GetPos();
		}
	}

	void DisableCache(bool disable)
	{
		m_disable_cache = disable;
	}
};

#endif /* YAPF_COSTRAIL_HPP */
