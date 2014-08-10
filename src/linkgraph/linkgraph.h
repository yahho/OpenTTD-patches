/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file linkgraph.h Declaration of link graph classes used for cargo distribution. */

#ifndef LINKGRAPH_H
#define LINKGRAPH_H

#include "../core/pool_type.hpp"
#include "../core/smallmap_type.hpp"
#include "../station_base.h"
#include "../cargotype.h"
#include "../date_func.h"
#include "linkgraph_type.h"
#include "graph.h"

struct SaveLoad;
struct LoadBuffer;
struct SaveDumper;
class LinkGraph;


/**
 * Node of the link graph. contains all relevant information from the associated
 * station. It's copied so that the link graph job can work on its own data set
 * in a separate thread.
 */
struct LinkGraphNode {
	uint supply;             ///< Supply at the station.
	uint demand;             ///< Acceptance at the station.
	StationID station;       ///< Station ID.
	Date last_update;        ///< When the supply was last updated.
	void Init(StationID st = INVALID_STATION, uint demand = 0);

	/** Get supply of node. */
	uint Supply() const { return this->supply; }

	/** Get demand of node. */
	uint Demand() const { return this->demand; }

	/** Get ID of node station. */
	StationID Station() const { return this->station; }

	/** Get the date of the last node update. */
	Date LastUpdate() const { return this->last_update; }

	/**
	 * Update the node's supply and set last_update to the current date.
	 * @param supply Supply to be added.
	 */
	void UpdateSupply (uint supply)
	{
		this->supply += supply;
		this->last_update = _date;
	}

	/**
	 * Set the node's demand.
	 * @param demand New demand for the node.
	 */
	void SetDemand (uint demand)
	{
		this->demand = demand;
	}
};


/**
 * An edge in the link graph. Corresponds to a link between two stations or at
 * least the distance between them. Edges from one node to itself contain the
 * ID of the opposite Node of the first active edge (i.e. not just distance) in
 * the column as next_edge.
 */
struct LinkGraphEdge {
	uint distance;                 ///< Length of the link.
	uint capacity;                 ///< Capacity of the link.
	uint usage;                    ///< Usage of the link.
	Date last_unrestricted_update; ///< When the unrestricted part of the link was last updated.
	Date last_restricted_update;   ///< When the restricted part of the link was last updated.
	NodeID next_edge;              ///< Destination of next valid edge starting at the same source node.
	void Init(uint distance = 0);

	/** Get edge capacity. */
	uint Capacity() const { return this->capacity; }

	/** Get edge usage. */
	uint Usage() const { return this->usage; }

	/** Get edge distance. */
	uint Distance() const { return this->distance; }

	/** Get the date of the last unrestricted capacity update. */
	Date LastUnrestrictedUpdate() const { return this->last_unrestricted_update; }

	/** Get the date of the last restricted capacity update. */
	Date LastRestrictedUpdate() const { return this->last_restricted_update; }

	/** Get the date of the last capacity update. */
	Date LastUpdate() const { return max (this->last_unrestricted_update, this->last_restricted_update); }

	void Set (uint capacity, uint usage, EdgeUpdateMode mode);
	void Update (uint capacity, uint usage, EdgeUpdateMode mode);

	void Restrict() { this->last_unrestricted_update = INVALID_DATE; }

	void Release() { this->last_restricted_update = INVALID_DATE; }
};


/**
 * A connected component of a link graph. Contains a complete set of stations
 * connected by links as nodes and edges. Each component also holds a copy of
 * the link graph settings at the time of its creation. The global settings
 * might change between the creation and join time so we can't rely on them.
 */
class LinkGraph : public PooledItem <LinkGraph, LinkGraphID, 32, 0xFFFF>,
	public Graph <LinkGraphNode, LinkGraphEdge> {
public:
	typedef Node BaseNode;
	typedef Edge BaseEdge;

	/** Minimum effective distance for timeout calculation. */
	static const uint MIN_TIMEOUT_DISTANCE = 32;

	/** Minimum number of days between subsequent compressions of a LG. */
	static const uint COMPRESSION_INTERVAL = 256;

	/**
	 * Scale a value from a link graph of age orig_age for usage in one of age
	 * target_age. Make sure that the value stays > 0 if it was > 0 before.
	 * @param val Value to be scaled.
	 * @param target_age Age of the target link graph.
	 * @param orig_age Age of the original link graph.
	 * @return scaled value.
	 */
	inline static uint Scale(uint val, uint target_age, uint orig_age)
	{
		return val > 0 ? max(1U, val * target_age / orig_age) : 0;
	}

	/** Bare constructor, only for save/load. */
	LinkGraph() : cargo(INVALID_CARGO), last_compression(0) {}
	/**
	 * Real constructor.
	 * @param cargo Cargo the link graph is about.
	 */
	LinkGraph(CargoID cargo) : cargo(cargo), last_compression(_date) {}

	void Resize(uint size);
	void ShiftDates(int interval);
	void Compress();
	void Merge(LinkGraph *other);

	/* Splitting link graphs is intentionally not implemented.
	 * The overhead in determining connectedness would probably outweigh the
	 * benefit of having to deal with smaller graphs. In real world examples
	 * networks generally grow. Only rarely a network is permanently split.
	 * Reacting to temporary splits here would obviously create performance
	 * problems and detecting the temporary or permanent nature of splits isn't
	 * trivial. */

	/**
	 * Get date of last compression.
	 * @return Date of last compression.
	 */
	inline Date LastCompression() const { return this->last_compression; }

	/**
	 * Get the cargo ID this component's link graph refers to.
	 * @return Cargo ID.
	 */
	inline CargoID Cargo() const { return this->cargo; }

	/**
	 * Scale a value to its monthly equivalent, based on last compression.
	 * @param base Value to be scaled.
	 * @return Scaled value.
	 */
	inline uint Monthly(uint base) const
	{
		return base * 30 / (_date - this->last_compression + 1);
	}

	NodeID AddNode(const Station *st);
	void RemoveNode(NodeID id);
	void UpdateDistances(NodeID id, TileIndex xy);
	void UpdateEdge(NodeID from, NodeID to, uint capacity, uint usage, EdgeUpdateMode mode);
	void RemoveEdge(NodeID from, NodeID to);

protected:
	friend const SaveLoad *GetLinkGraphDesc();
	friend const SaveLoad *GetLinkGraphJobDesc();
	friend void Save_LinkGraph(SaveDumper *dumper, const LinkGraph &lg);
	friend void Load_LinkGraph(LoadBuffer *reader, LinkGraph &lg);

	CargoID cargo;         ///< Cargo of this component's link graph.
	Date last_compression; ///< Last time the capacities and supplies were compressed.
};

#define FOR_ALL_LINK_GRAPHS(var) FOR_ALL_ITEMS_FROM(LinkGraph, link_graph_index, var, 0)

#endif /* LINKGRAPH_H */
