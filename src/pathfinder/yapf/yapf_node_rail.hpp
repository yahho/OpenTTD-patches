/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_node_rail.hpp Node tailored for rail pathfinding. */

#ifndef YAPF_NODE_RAIL_HPP
#define YAPF_NODE_RAIL_HPP

#include <bitset>

/** key for YAPF rail nodes */
typedef CYapfNodeKeyTrackDir<PathPos> CYapfRailKey;

/** key for cached segment cost for rail YAPF */
struct CYapfRailSegmentKey
{
	uint32    m_value;

	inline CYapfRailSegmentKey(const CYapfRailSegmentKey& src) : m_value(src.m_value) {}

	inline CYapfRailSegmentKey(const CYapfRailKey& node_key)
	{
		Set(node_key);
	}

	inline void Set(const CYapfRailSegmentKey& src)
	{
		m_value = src.m_value;
	}

	inline void Set(const CYapfRailKey& node_key)
	{
		m_value = node_key.CalcHash();
	}

	inline int32 CalcHash() const
	{
		return m_value;
	}

	inline bool operator == (const CYapfRailSegmentKey& other) const
	{
		return m_value == other.m_value;
	}

	void Dump(DumpTarget &dmp) const
	{
		dmp.WriteTile("tile", (TileIndex)(m_value >> 4));
		dmp.WriteEnumT("td", (Trackdir)(m_value & 0x0F));
	}
};

/** cached segment cost for rail YAPF */
struct CYapfRailSegment
{
	typedef CYapfRailSegmentKey Key;

	CYapfRailSegmentKey    m_key;
	PathPos                m_last;
	int                    m_cost;
	PathPos                m_last_signal;
	EndSegmentReasonBits   m_end_segment_reason;
	CYapfRailSegment      *m_hash_next;

	inline CYapfRailSegment(const CYapfRailSegmentKey& key)
		: m_key(key)
		, m_last()
		, m_cost(-1)
		, m_last_signal()
		, m_end_segment_reason(ESRB_NONE)
		, m_hash_next(NULL)
	{}

	inline const Key& GetKey() const
	{
		return m_key;
	}

	inline CYapfRailSegment *GetHashNext()
	{
		return m_hash_next;
	}

	inline void SetHashNext(CYapfRailSegment *next)
	{
		m_hash_next = next;
	}

	void Dump(DumpTarget &dmp) const
	{
		dmp.WriteStructT("m_key", &m_key);
		dmp.WriteTile("m_last.tile", m_last.tile);
		dmp.WriteEnumT("m_last.td", m_last.td);
		dmp.WriteLine("m_cost = %d", m_cost);
		dmp.WriteTile("m_last_signal.tile", m_last_signal.tile);
		dmp.WriteEnumT("m_last_signal.td", m_last_signal.td);
		dmp.WriteEnumT("m_end_segment_reason", m_end_segment_reason);
	}
};

/** Yapf Node for rail YAPF */
struct CYapfRailNodeTrackDir
	: CYapfNodeT<CYapfRailKey, CYapfRailNodeTrackDir>
{
	typedef CYapfNodeT<CYapfRailKey, CYapfRailNodeTrackDir> base;
	typedef CYapfRailSegment CachedData;

	enum {
		FLAG_TARGET_SEEN,
		FLAG_CHOICE_SEEN,
		FLAG_LAST_SIGNAL_WAS_RED,
		NFLAGS
	};

	CYapfRailSegment *m_segment;
	uint16            m_num_signals_passed;
	std::bitset<NFLAGS> flags;
	SignalType        m_last_red_signal_type;
	SignalType        m_last_signal_type;

	inline void Set(CYapfRailNodeTrackDir *parent, const PathPos &pos, bool is_choice)
	{
		base::Set(parent, pos);
		m_segment = NULL;
		if (parent == NULL) {
			m_num_signals_passed      = 0;
			flags                     = 0;
			m_last_red_signal_type    = SIGTYPE_NORMAL;
			/* We use PBS as initial signal type because if we are in
			 * a PBS section and need to route, i.e. we're at a safe
			 * waiting point of a station, we need to account for the
			 * reservation costs. If we are in a normal block then we
			 * should be alone in there and as such the reservation
			 * costs should be 0 anyway. If there would be another
			 * train in the block, i.e. passing signals at danger
			 * then avoiding that train with help of the reservation
			 * costs is not a bad thing, actually it would probably
			 * be a good thing to do. */
			m_last_signal_type        = SIGTYPE_PBS;
		} else {
			m_num_signals_passed      = parent->m_num_signals_passed;
			flags                     = parent->flags;
			m_last_red_signal_type    = parent->m_last_red_signal_type;
			m_last_signal_type        = parent->m_last_signal_type;
		}
		flags.set (FLAG_CHOICE_SEEN, is_choice);
	}

	inline const PathPos& GetLastPos() const
	{
		assert(m_segment != NULL);
		return m_segment->m_last;
	}

	void Dump(DumpTarget &dmp) const
	{
		base::Dump(dmp);
		dmp.WriteStructT("m_segment", m_segment);
		dmp.WriteLine("m_num_signals_passed = %d", m_num_signals_passed);
		dmp.WriteLine("m_target_seen = %s", flags.test(FLAG_TARGET_SEEN) ? "Yes" : "No");
		dmp.WriteLine("m_choice_seen = %s", flags.test(FLAG_CHOICE_SEEN) ? "Yes" : "No");
		dmp.WriteLine("m_last_signal_was_red = %s", flags.test(FLAG_LAST_SIGNAL_WAS_RED) ? "Yes" : "No");
		dmp.WriteEnumT("m_last_red_signal_type", m_last_red_signal_type);
	}
};

/* Default Astar type */
typedef Astar<CYapfRailNodeTrackDir, 8, 10> AstarRailTrackDir;

#endif /* YAPF_NODE_RAIL_HPP */
