/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file yapf_node.hpp Node in the pathfinder's graph. */

#ifndef YAPF_NODE_HPP
#define YAPF_NODE_HPP

#include "../pos.h"

/** Yapf Node Key base class. */
struct CYapfNodeKey : PathPos {
	inline void Set(const PathPos &pos)
	{
		PathPos::set(pos);
	}

	void Dump(DumpTarget &dmp) const
	{
		dmp.WriteTile("m_tile", tile);
		dmp.WriteEnumT("m_td", td);
	}
};

/** Yapf Node Key that evaluates hash from (and compares) tile & exit dir. */
struct CYapfNodeKeyExitDir : public CYapfNodeKey
{
	DiagDirection  exitdir;

	inline void Set(const PathPos &pos)
	{
		CYapfNodeKey::Set(pos);
		exitdir = (pos.td == INVALID_TRACKDIR) ? INVALID_DIAGDIR : TrackdirToExitdir(pos.td);
	}

	inline int CalcHash() const
	{
		return exitdir | (in_wormhole() ? 4 : 0) | (tile << 3);
	}

	inline bool operator == (const CYapfNodeKeyExitDir& other) const
	{
		return PathTile::operator==(other) && (exitdir == other.exitdir);
	}

	void Dump(DumpTarget &dmp) const
	{
		CYapfNodeKey::Dump(dmp);
		dmp.WriteEnumT("m_exitdir", exitdir);
	}
};

/** Yapf Node Key that evaluates hash from (and compares) tile & track dir. */
struct CYapfNodeKeyTrackDir : public CYapfNodeKey
{
	inline int CalcHash() const
	{
		return (in_wormhole() ? (td + 6) : td) | (tile << 4);
	}

	inline bool operator == (const CYapfNodeKeyTrackDir& other) const
	{
		return PathTile::operator==(other) && (td == other.td);
	}
};

/** Yapf Node base */
template <class Tkey_, class Tnode>
struct CYapfNodeT : AstarNodeBase<Tnode> {
	typedef AstarNodeBase<Tnode> ABase;
	typedef Tkey_ Key;
	typedef Tnode Node;

	Tkey_       m_key;

	inline void Set(Node *parent, const PathPos &pos)
	{
		ABase::Set (parent);
		m_key.Set(pos);
	}

	inline const PathPos& GetPos() const {return m_key;}
	inline const Tkey_& GetKey() const {return m_key;}

	void Dump(DumpTarget &dmp) const
	{
		ABase::Dump(dmp);
		dmp.WriteStructT("m_key", &m_key);
	}
};

#endif /* YAPF_NODE_HPP */
