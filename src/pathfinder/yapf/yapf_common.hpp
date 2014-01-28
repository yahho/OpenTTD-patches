/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_common.hpp Commonly used classes for YAPF. */

#ifndef YAPF_COMMON_HPP
#define YAPF_COMMON_HPP

/** YAPF origin provider base class - used when origin is one tile / multiple trackdirs */
template <class Tpf>
class CYapfOriginTileT
{
protected:
	PathPos      m_org;                           ///< origin position
	TrackdirBits m_trackdirs;                     ///< origin trackdir mask

	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/** Set origin position */
	void SetOrigin(const PathPos &pos)
	{
		m_org = pos;
		m_trackdirs = TrackdirToTrackdirBits(pos.td);
	}

	/** Set origin tile / trackdir mask */
	void SetOrigin(TileIndex tile, TrackdirBits trackdirs)
	{
		m_org = PathPos(tile, (KillFirstBit(trackdirs) == TRACKDIR_BIT_NONE) ? FindFirstTrackdir(trackdirs) : INVALID_TRACKDIR);
		m_trackdirs = trackdirs;
	}

	/** Called when YAPF needs to place origin nodes into open list */
	void PfSetStartupNodes()
	{
		if (m_org.td != INVALID_TRACKDIR) {
			Yapf().AddStartupNode(m_org, false);
		} else {
			PathPos pos = m_org;
			for (TrackdirBits tdb = m_trackdirs; tdb != TRACKDIR_BIT_NONE; tdb = KillFirstBit(tdb)) {
				pos.td = FindFirstTrackdir(tdb);
				Yapf().AddStartupNode(pos, true);
			}
		}
	}
};

/** YAPF origin provider base class - used when there are two tile/trackdir origins */
template <class Tpf>
class CYapfOriginTileTwoWayT
{
protected:
	PathPos     m_org;                            ///< first origin position
	PathPos     m_rev;                            ///< second (reversed) origin position
	int         m_reverse_penalty;                ///< penalty to be added for using the reversed origin
	bool        m_treat_first_red_two_way_signal_as_eol; ///< in some cases (leaving station) we need to handle first two-way signal differently

	/** to access inherited path finder */
	inline Tpf& Yapf()
	{
		return *static_cast<Tpf*>(this);
	}

public:
	/** set origin */
	void SetOrigin(const PathPos &pos, const PathPos &rev = PathPos(), int reverse_penalty = 0, bool treat_first_red_two_way_signal_as_eol = true)
	{
		m_org = pos;
		m_rev = rev;
		m_reverse_penalty = reverse_penalty;
		m_treat_first_red_two_way_signal_as_eol = treat_first_red_two_way_signal_as_eol;
	}

	/** Called when YAPF needs to place origin nodes into open list */
	void PfSetStartupNodes()
	{
		if (m_org.tile != INVALID_TILE && m_org.td != INVALID_TRACKDIR) {
			Yapf().AddStartupNode(m_org, false);
		}
		if (m_rev.tile != INVALID_TILE && m_rev.td != INVALID_TRACKDIR) {
			Yapf().AddStartupNode(m_rev, false, m_reverse_penalty);
		}
	}

	/** return true if first two-way signal should be treated as dead end */
	inline bool TreatFirstRedTwoWaySignalAsEOL()
	{
		return Yapf().PfGetSettings().rail_firstred_twoway_eol && m_treat_first_red_two_way_signal_as_eol;
	}
};

/**
 * YAPF template that uses Ttypes template argument to determine all YAPF
 *  components (base classes) from which the actual YAPF is composed.
 *  For example classes consult: CYapfRail_TypesT template and its instantiations:
 *  CYapfRail1, CYapfRail2, CYapfRail3, CYapfAnyDepotRail1, CYapfAnyDepotRail2, CYapfAnyDepotRail3
 */
template <class Ttypes>
class CYapfT
	: public Ttypes::PfBase         ///< Instance of CYapfBaseT - main YAPF loop and support base class
	, public Ttypes::PfCost         ///< Cost calculation provider base class
	, public Ttypes::PfCache        ///< Segment cost cache provider
	, public Ttypes::PfOrigin       ///< Origin (tile or two-tile origin)
	, public Ttypes::PfDestination  ///< Destination detector and distance (estimate) calculation provider
	, public Ttypes::PfFollow       ///< Node follower (stepping provider)
{
};



#endif /* YAPF_COMMON_HPP */
