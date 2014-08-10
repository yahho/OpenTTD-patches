/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file graph.h Declaration of a link graph class template for cargo distribution. */

#ifndef LINKGRAPH_GRAPH_H
#define LINKGRAPH_GRAPH_H

#include "../core/smallvec_type.hpp"
#include "../core/smallmatrix_type.hpp"
#include "../date_func.h"
#include "linkgraph_type.h"

/**
 * Link graph base class template.
 * @tparam TNode Node class.
 * @tparam TEdge Edge class.
 */
template <class TNode, class TEdge>
struct Graph {
	typedef TNode Node;
	typedef TEdge Edge;

protected:
	SmallVector <Node, 16> nodes; ///< Nodes in the graph.
	SmallMatrix <Edge>     edges; ///< Edges in the graph.

public:
	/** Get the size (order) of the graph. */
	uint Size (void) const
	{
		assert (this->edges.Height() == this->nodes.Length());
		assert (this->edges.Width()  == this->nodes.Length());
		return this->nodes.Length();
	}

	/** Resize the graph to a given size (order). Graph must be empty. */
	void Resize (uint size)
	{
		assert (this->Size() == 0);
		this->nodes.Resize (size);
		this->edges.Resize (size, size);
	}

private:
	/** Edge iterator template. */
	template <class QEdge>
	struct EdgeIteratorT {
	private:
		QEdge *const edges; ///< pointer to row of edges
		NodeID id;          ///< current index within the row

	public:
		/**
		 * Begin edge iterator constructor.
		 * @param row Pointer to the first edge in the row.
		 * @param index Row number.
		 */
		EdgeIteratorT (QEdge *row, NodeID index) : edges (row),
			id (row[index].next_edge)
		{
		}

		/**
		 * End edge iterator constructor.
		 * @param row Pointer to the first edge in the row.
		 */
		EdgeIteratorT (QEdge *row) : edges(row), id(INVALID_NODE)
		{
		}

		/** Prefix increment. */
		EdgeIteratorT &operator++ (void)
		{
			assert (this->id != INVALID_NODE);
			this->id = this->edges[this->id].next_edge;
			return *this;
		}

		/** Postfix increment. */
		EdgeIteratorT operator++ (int)
		{
			EdgeIteratorT ret (*this);
			++*this;
			return ret;
		}

		/** Equality comparator. */
		bool operator== (const EdgeIteratorT &other) const
		{
			return this->edges == other.edges && this->id == other.id;
		}

		/** Inequality comparator. */
		bool operator!= (const EdgeIteratorT &other) const
		{
			return !operator==(other);
		}

		/** Get current edge target node id. */
		NodeID get_id (void) const
		{
			return this->id;
		}

		/** Get current edge. */
		QEdge &operator* (void) const
		{
			return this->edges[this->id];
		}

		/** Get current edge. */
		QEdge *operator-> (void) const
		{
			return this->edges + this->id;
		}
	};
public:
	typedef EdgeIteratorT <Edge> EdgeIterator;
	typedef EdgeIteratorT <const Edge> ConstEdgeIterator;

	/** Get an edge iterator to the beginning of a row. */
	EdgeIterator NodeBegin (NodeID node)
	{
		return EdgeIterator (this->edges[node], node);
	}

	/** Get an edge iterator to the end of a row. */
	EdgeIterator NodeEnd (NodeID node)
	{
		return EdgeIterator (this->edges[node]);
	}

	/** Get an edge iterator to the beginning of a row, const version. */
	ConstEdgeIterator NodeBegin (NodeID node) const
	{
		return ConstEdgeIterator (this->edges[node], node);
	}

	/** Get an edge iterator to the end of a row, const version. */
	ConstEdgeIterator NodeEnd (NodeID node) const
	{
		return ConstEdgeIterator (this->edges[node]);
	}

	/** Get a const edge iterator to the beginning of a row. */
	ConstEdgeIterator NodeCBegin (NodeID node)
	{
		return ConstEdgeIterator (this->edges[node], node);
	}

	/** Get a const edge iterator to the end of a row. */
	ConstEdgeIterator NodeCEnd (NodeID node)
	{
		return ConstEdgeIterator (this->edges[node]);
	}

private:
	/**
	 * Node reference template.
	 * @tparam QNode Actual node class, either "const Node" or just "Node".
	 * @tparam QEdge Actual edge class, either "const Edge" or just "Edge".
	 */
	template <typename QNode, typename QEdge>
	class NodeWrapper {
	protected:
		QNode *const node;  ///< node
		QEdge *const edges; ///< outgoing edges for node
		const NodeID index; ///< node id

	public:
		/**
		 * Construct a reference to a node plus its outgoing edges.
		 * @param node Node to be wrapped.
		 * @param edges Outgoing edges for node to be wrapped.
		 * @param index ID of node to be wrapped.
		 */
		NodeWrapper (Graph *graph, NodeID id)
			: node(&graph->nodes[id]),
				edges(graph->edges[id]),
				index(id)
		{
		}

		/**
		 * Construct a const reference to a node plus its outgoing edges.
		 * @param node Node to be wrapped.
		 * @param edges Outgoing edges for node to be wrapped.
		 * @param index ID of node to be wrapped.
		 */
		NodeWrapper (const Graph *graph, NodeID id)
			: node((const QNode*)&graph->nodes[id]),
				edges((const QEdge*)graph->edges[id]),
				index(id)
		{
		}

		/** Get the underlying node. */
		QNode &operator* (void)
		{
			return *this->node;
		}

		/** Get the underlying node, const version. */
		const QNode &operator* (void) const
		{
			return *this->node;
		}

		/** Get the underlying node. */
		QNode *operator-> (void)
		{
			return this->node;
		}

		/** Get the underlying node, const version. */
		const QNode *operator-> (void) const
		{
			return this->node;
		}

		/** Get the outgoing edge to another node. */
		QEdge &operator[] (NodeID to)
		{
			return this->edges[to];
		}

		/** Get the outgoing edge to another node, const version. */
		const QEdge &operator[] (NodeID to) const
		{
			return this->edges[to];
		}

		/** Get an iterator to the beginning of the outgoing row. */
		EdgeIteratorT<QEdge> Begin (void) const
		{
			return EdgeIteratorT<QEdge> (this->edges, index);
		}

		/** Get an iterator to the end of the outgoing row. */
		EdgeIteratorT<QEdge> End (void) const
		{
			return EdgeIteratorT<QEdge> (this->edges);
		}

		/** Get a const iterator to the beginning of the outgoing row. */
		EdgeIteratorT<const QEdge> CBegin (void) const
		{
			return EdgeIteratorT<const QEdge> (this->edges, index);
		}

		/** Get a const iterator to the end of the outgoing row. */
		EdgeIteratorT<const QEdge> CEnd (void) const
		{
			return EdgeIteratorT<const QEdge> (this->edges);
		}

		/** Remove an edge from the outgoing row list. */
		QEdge *unlink (NodeID to)
		{
			assert (this->index != to);

			QEdge *prev = &this->edges[this->index];
			while (prev->next_edge != to) {
				assert (prev->next_edge != INVALID_NODE);
				prev = &this->edges[prev->next_edge];
			}

			QEdge *edge = &this->edges[to];
			prev->next_edge = edge->next_edge;
			return edge;
		}
	};

public:
	typedef NodeWrapper <Node, Edge> NodeRef;
	typedef NodeWrapper <const Node, const Edge> ConstNodeRef;

	/** Get a reference to a given node. */
	NodeRef operator[] (NodeID from)
	{
		return NodeRef (this, from);
	}

	/** Get a const reference to a given node. */
	ConstNodeRef operator[] (NodeID from) const
	{
		return ConstNodeRef (this, from);
	}
};

#endif /* LINKGRAPH_GRAPH_H */
