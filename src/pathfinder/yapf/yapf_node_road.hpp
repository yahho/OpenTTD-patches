/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_node_road.hpp Node tailored for road pathfinding. */

#ifndef YAPF_NODE_ROAD_HPP
#define YAPF_NODE_ROAD_HPP

/** Yapf Node for road YAPF */
template <class Tkey_>
struct CYapfRoadNodeT
	: CYapfNodeT<Tkey_, CYapfRoadNodeT<Tkey_> >
{
	typedef CYapfNodeT<Tkey_, CYapfRoadNodeT<Tkey_> > base;

	PathPos m_segment_last;

	void Set(CYapfRoadNodeT *parent, const PathPos &pos)
	{
		base::Set(parent, pos);
		m_segment_last = pos;
	}
};

/* now define two major node types (that differ by key type) */
typedef CYapfRoadNodeT<CYapfNodeKeyExitDir>  CYapfRoadNodeExitDir;
typedef CYapfRoadNodeT<CYapfNodeKeyTrackDir> CYapfRoadNodeTrackDir;

/* Default Astar types */
typedef Astar<CYapfRoadNodeExitDir , 8, 10> AstarRoadExitDir;
typedef Astar<CYapfRoadNodeTrackDir, 8, 10> AstarRoadTrackDir;

#endif /* YAPF_NODE_ROAD_HPP */
