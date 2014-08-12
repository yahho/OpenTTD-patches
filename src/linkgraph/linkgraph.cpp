/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file linkgraph.cpp Definition of link graph classes used for cargo distribution. */

#include "../stdafx.h"
#include "../core/pool_func.hpp"
#include "linkgraph.h"

/* Initialize the link-graph-pool */
template<> LinkGraph::Pool LinkGraph::PoolItem::pool ("LinkGraph");
INSTANTIATE_POOL_METHODS(LinkGraph)

/**
 * Create a node.
 * @param st Associated station.
 * @param demand Demand for cargo at the station.
 */
inline void LinkGraphNode::Init (const ::Station *st, uint demand)
{
	this->supply = 0;
	this->demand = demand;
	this->station = st->index;
	this->last_update = INVALID_DATE;
}

/**
 * Clear a node.
 */
inline void LinkGraphNode::Init (void)
{
	this->supply = 0;
	this->demand = 0;
	this->station = INVALID_STATION;
	this->last_update = INVALID_DATE;
}

/**
 * Create an edge.
 * @param distance Length of the link as manhattan distance.
 */
inline void LinkGraphEdge::Init(uint distance)
{
	this->distance = distance;
	this->capacity = 0;
	this->usage = 0;
	this->last_unrestricted_update = INVALID_DATE;
	this->last_restricted_update = INVALID_DATE;
	this->next_edge = INVALID_NODE;
}

/**
 * Shift all dates by given interval.
 * This is useful if the date has been modified with the cheat menu.
 * @param interval Number of days to be added or subtracted.
 */
void LinkGraph::ShiftDates(int interval)
{
	this->last_compression += interval;
	for (NodeID node1 = 0; node1 < this->Size(); ++node1) {
		Node &source = this->nodes[node1];
		if (source.last_update != INVALID_DATE) source.last_update += interval;
		for (NodeID node2 = 0; node2 < this->Size(); ++node2) {
			Edge &edge = this->edges[node1][node2];
			if (edge.last_unrestricted_update != INVALID_DATE) edge.last_unrestricted_update += interval;
			if (edge.last_restricted_update != INVALID_DATE) edge.last_restricted_update += interval;
		}
	}
}

void LinkGraph::Compress()
{
	this->last_compression = (_date + this->last_compression) / 2;
	for (NodeID node1 = 0; node1 < this->Size(); ++node1) {
		this->nodes[node1].supply /= 2;
		for (NodeID node2 = 0; node2 < this->Size(); ++node2) {
			Edge &edge = this->edges[node1][node2];
			if (edge.capacity > 0) {
				edge.capacity = max(1U, edge.capacity / 2);
				edge.usage /= 2;
			}
		}
	}
}

/**
 * Merge a link graph with another one.
 * @param other LinkGraph to be merged into this one.
 */
void LinkGraph::Merge(LinkGraph *other)
{
	Date age = _date - this->last_compression + 1;
	Date other_age = _date - other->last_compression + 1;
	NodeID first = this->Size();
	for (NodeID node1 = 0; node1 < other->Size(); ++node1) {
		Station *st = Station::Get(other->nodes[node1].station);
		NodeID new_node = this->AddNode(st);
		this->nodes[new_node].supply = LinkGraph::Scale(other->nodes[node1].supply, age, other_age);
		st->goods[this->cargo].link_graph = this->index;
		st->goods[this->cargo].node = new_node;
		for (NodeID node2 = 0; node2 < node1; ++node2) {
			Edge &forward = this->edges[new_node][first + node2];
			Edge &backward = this->edges[first + node2][new_node];
			forward = other->edges[node1][node2];
			backward = other->edges[node2][node1];
			forward.capacity = LinkGraph::Scale(forward.capacity, age, other_age);
			forward.usage = LinkGraph::Scale(forward.usage, age, other_age);
			if (forward.next_edge != INVALID_NODE) forward.next_edge += first;
			backward.capacity = LinkGraph::Scale(backward.capacity, age, other_age);
			backward.usage = LinkGraph::Scale(backward.usage, age, other_age);
			if (backward.next_edge != INVALID_NODE) backward.next_edge += first;
		}
		Edge &new_start = this->edges[new_node][new_node];
		new_start = other->edges[node1][node1];
		if (new_start.next_edge != INVALID_NODE) new_start.next_edge += first;
	}
	delete other;
}

/**
 * Remove a node from the link graph by overwriting it with the last node.
 * @param id ID of the node to be removed.
 */
void LinkGraph::RemoveNode(NodeID id)
{
	assert(id < this->Size());

	NodeID last_node = this->Size() - 1;
	for (NodeID i = 0; i <= last_node; ++i) {
		(*this)[i].unlink(id);
		Edge *node_edges = this->edges[i];
		NodeID prev = i;
		NodeID next = node_edges[i].next_edge;
		while (next != INVALID_NODE) {
			if (next == last_node) {
				node_edges[prev].next_edge = id;
				break;
			}
			prev = next;
			next = node_edges[prev].next_edge;
		}
		node_edges[id] = node_edges[last_node];
	}
	Station::Get(this->nodes[last_node].station)->goods[this->cargo].node = id;
	this->nodes.Erase(this->nodes.Get(id));
	this->edges.EraseColumn(id);
	/* Not doing EraseRow here, as having the extra invalid row doesn't hurt
	 * and removing it would trigger a lot of memmove. The data has already
	 * been copied around in the loop above. */
}

/**
 * Update distances between the given node and all others.
 * @param id Node that changed position.
 * @param xy New position of the node.
 */
void LinkGraph::UpdateDistances(NodeID id, TileIndex xy)
{
	assert(id < this->Size());
	for (NodeID other = 0; other < this->Size(); ++other) {
		if (other == id) continue;
		this->edges[id][other].distance = this->edges[other][id].distance =
				DistanceMaxPlusManhattan(xy, Station::Get(this->nodes[other].station)->xy);
	}
}

/**
 * Add a node to the component and create empty edges associated with it. Set
 * the station's last_component to this component. Calculate the distances to all
 * other nodes. The distances to _all_ nodes are important as the demand
 * calculator relies on their availability.
 * @param st New node's station.
 * @return New node's ID.
 */
NodeID LinkGraph::AddNode(const Station *st)
{
	const GoodsEntry &good = st->goods[this->cargo];

	NodeID new_node = this->Size();
	this->nodes.Append();
	/* Avoid reducing the height of the matrix as that is expensive and we
	 * most likely will increase it again later which is again expensive. */
	this->edges.Resize(new_node + 1U,
			max(new_node + 1U, this->edges.Height()));

	this->nodes[new_node].Init (st,
			HasBit(good.status, GoodsEntry::GES_ACCEPTANCE));

	Edge *new_edges = this->edges[new_node];

	/* Reset the first edge starting at the new node */
	new_edges[new_node].next_edge = INVALID_NODE;

	for (NodeID i = 0; i <= new_node; ++i) {
		uint distance = DistanceMaxPlusManhattan(st->xy, Station::Get(this->nodes[i].station)->xy);
		new_edges[i].Init(distance);
		this->edges[i][new_node].Init(distance);
	}
	return new_node;
}

/**
 * Creates an edge if none exists yet or updates an existing edge.
 * @param from Source node.
 * @param to Target node.
 * @param capacity Capacity of the link.
 * @param usage Usage to be added.
 * @param mode Update mode to be used.
 */
void LinkGraph::UpdateEdge (NodeID from, NodeID to, uint capacity, uint usage, EdgeUpdateMode mode)
{
	assert(from != to);
	assert(capacity > 0);
	assert(usage <= capacity);

	Edge *edges = this->edges[from];
	if (edges[to].capacity == 0) {
		edges[to].Set (capacity, usage, mode);
		edges[to].next_edge = edges[from].next_edge;
		edges[from].next_edge = to;
	} else {
		edges[to].Update(capacity, usage, mode);
	}
}

/**
 * Remove an edge from the graph.
 * @param from ID of source node.
 * @param to ID of destination node.
 */
void LinkGraph::RemoveEdge (NodeID from, NodeID to)
{
	assert(from != to);
	Edge *edge = (*this)[from].unlink(to);
	edge->capacity = 0;
	edge->last_unrestricted_update = INVALID_DATE;
	edge->last_restricted_update = INVALID_DATE;
	edge->usage = 0;
}

/**
 * Set an edge. Set the restricted or unrestricted update timestamp according
 * to the given update mode.
 * @param capacity Capacity to be set.
 * @param usage Usage to be set.
 * @param mode Update mode to be applied.
 */
void LinkGraphEdge::Set (uint capacity, uint usage, EdgeUpdateMode mode)
{
	this->capacity = capacity;
	this->usage = usage;
	if (mode & EUM_UNRESTRICTED) this->last_unrestricted_update = _date;
	if (mode & EUM_RESTRICTED)   this->last_restricted_update   = _date;
}

/**
 * Update an edge. If mode contains UM_REFRESH refresh the edge to have at
 * least the given capacity and usage, otherwise add the capacity and usage.
 * In any case set the respective update timestamp(s), according to the given
 * mode.
 * @param capacity Capacity to be added/updated.
 * @param usage Usage to be added.
 * @param mode Update mode to be applied.
 */
void LinkGraphEdge::Update (uint capacity, uint usage, EdgeUpdateMode mode)
{
	assert (this->capacity > 0);
	assert (capacity >= usage);

	if (mode & EUM_INCREASE) {
		this->capacity += capacity;
		this->usage += usage;
	} else if (mode & EUM_REFRESH) {
		this->capacity = max (this->capacity, capacity);
		this->usage = max (this->usage, usage);
	}
	if (mode & EUM_UNRESTRICTED) this->last_unrestricted_update = _date;
	if (mode & EUM_RESTRICTED) this->last_restricted_update = _date;
}

/**
 * Resize the component and fill it with empty nodes and edges. Used when
 * loading from save games. The component is expected to be empty before.
 * @param size New size of the component.
 */
void LinkGraph::Resize(uint size)
{
	Graph <LinkGraphNode, LinkGraphEdge>::Resize (size);

	for (uint i = 0; i < size; ++i) {
		this->nodes[i].Init();
		Edge *column = this->edges[i];
		for (uint j = 0; j < size; ++j) column[j].Init();
	}
}
