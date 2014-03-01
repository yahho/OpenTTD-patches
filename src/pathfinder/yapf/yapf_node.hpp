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
template <class PPos>
struct CYapfNodeKey : PPos {
	typedef PPos Pos;

	inline void Set(const Pos &pos)
	{
		Pos::set(pos);
	}

	void Dump(DumpTarget &dmp) const
	{
		dmp.WriteTile("m_tile", Pos::tile);
		dmp.WriteEnumT("m_td", Pos::td);
	}
};

/** Yapf Node Key that evaluates hash from (and compares) tile & exit dir. */
template <class PPos>
struct CYapfNodeKeyExitDir : public CYapfNodeKey<PPos>
{
	DiagDirection  exitdir;

	inline void Set(const PPos &pos)
	{
		CYapfNodeKey<PPos>::Set(pos);
		exitdir = (pos.td == INVALID_TRACKDIR) ? INVALID_DIAGDIR : TrackdirToExitdir(pos.td);
	}

	inline int CalcHash() const
	{
		return exitdir | (PPos::tile << 2);
	}

	inline bool operator == (const CYapfNodeKeyExitDir& other) const
	{
		return PPos::PathTile::operator==(other) && (exitdir == other.exitdir);
	}

	void Dump(DumpTarget &dmp) const
	{
		CYapfNodeKey<PPos>::Dump(dmp);
		dmp.WriteEnumT("m_exitdir", exitdir);
	}
};

/** Yapf Node Key that evaluates hash from (and compares) tile & track dir. */
template <class PPos>
struct CYapfNodeKeyTrackDir : public CYapfNodeKey<PPos>
{
	inline int CalcHash() const
	{
		return (PPos::in_wormhole() ? (PPos::td + 6) : PPos::td) | (PPos::tile << 4);
	}

	inline bool operator == (const CYapfNodeKeyTrackDir& other) const
	{
		return PPos::PathTile::operator==(other) && (PPos::td == other.td);
	}
};

/** Yapf Node base */
template <class Tkey_, class Tnode>
struct CYapfNodeT : AstarNodeBase<Tnode> {
	typedef AstarNodeBase<Tnode> ABase;
	typedef Tkey_ Key;
	typedef typename Key::Pos Pos;
	typedef Tnode Node;

	Tkey_       m_key;

	inline void Set(Node *parent, const Pos &pos)
	{
		ABase::Set (parent);
		m_key.Set(pos);
	}

	inline const Pos& GetPos() const {return m_key;}
	inline const Tkey_& GetKey() const {return m_key;}

	void Dump(DumpTarget &dmp) const
	{
		dmp.WriteStructT("m_parent", ABase::m_parent);
		dmp.WriteLine("m_cost = %d", ABase::m_cost);
		dmp.WriteLine("m_estimate = %d", ABase::m_estimate);
		dmp.WriteStructT("m_key", &m_key);
	}
};

#endif /* YAPF_NODE_HPP */
