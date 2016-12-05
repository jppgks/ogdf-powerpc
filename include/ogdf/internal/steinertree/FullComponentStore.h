/** \file
 * \brief Definition and implementation of FullComponentStore,
 *   a data structure to store full components
 *
 * \author Stephan Beyer
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/IndexComparer.h>
#include <ogdf/basic/BoundedStack.h>
#include <ogdf/internal/steinertree/EdgeWeightedGraphCopy.h>

namespace ogdf {
namespace steinertree {

#define OGDF_FULLCOMPONENTSTORE_REMOVE_IN_GRAPH_REPRESENTATION_ALSO // unnecessary but may save memory

/*!
 * \brief A data structure to store full components
 */
template<typename T, typename ExtraDataType = void>
class FullComponentStore
{
protected:
	const EdgeWeightedGraph<T> &m_originalGraph; //!< The original Steiner instance
	const List<node> &m_terminals; //!< The terminal list of the original Steiner instance
	const NodeArray<bool> &m_isTerminal; //!< Incidence vector for terminal nodes
	EdgeWeightedGraph<T> m_graph; //!< Our graph representation for the full component store
	NodeArray<node>
	  m_nodeCopy, //!< Mapping of original terminals to m_graph nodes
	  m_nodeOrig; //!< Mapping of m_graph nodes to original nodes

	template<class Y, class Enable = void> // Metadata without extra data
	struct Metadata {
		adjEntry start; //!< Adjacency entry on a terminal where a non-terminal BFS yields the component
		Array<node> terminals; //!< Terminals, sorted by node index
		T cost; //!< Cost
		Metadata()
		  : start(nullptr)
		  , terminals(0)
		  , cost(0)
		{
		}
	};
	template<class Y> // Metadata with extra data
	struct Metadata<Y, typename std::enable_if<!std::is_void<Y>::value>::type> {
		adjEntry start; //!< Adjacency entry on a terminal where a non-terminal BFS yields the component
		Array<node> terminals; //!< Terminals, sorted by node index
		T cost; //!< Cost
		Y extra;
		Metadata()
		  : start(nullptr)
		  , terminals(0)
		  , cost(0)
		{
		}
	};
	ArrayBuffer<Metadata<ExtraDataType>> m_components; //!< List of full components (based on metadata)

public:
	FullComponentStore(const EdgeWeightedGraph<T> &G, const List<node> &terminals, const NodeArray<bool> &isTerminal)
	  : m_originalGraph(G)
	  , m_terminals(terminals)
	  , m_isTerminal(isTerminal)
	  , m_nodeCopy(G, nullptr)
	  , m_nodeOrig(m_graph)
	{
		for (node v : m_terminals) {
			node u = m_graph.newNode();
			m_nodeCopy[v] = u;
			m_nodeOrig[u] = v;
		}
	}

	void insert(const EdgeWeightedGraphCopy<T> &comp)
	{
		OGDF_ASSERT(!comp.empty());
		OGDF_ASSERT(isTree(comp));

		// we temporarily use m_nodeCopy for nonterminals also
		ArrayBuffer<node> tempUse(comp.numberOfNodes() / 2);

		// add all nonterminals of comp to m_graph
		// and find terminals
		Metadata<ExtraDataType> data;
		for (node v : comp.nodes) {
			node vO = comp.original(v);
			if (m_nodeCopy[vO] == nullptr) {
				node vC = m_graph.newNode();
				m_nodeCopy[vO] = vC;
				m_nodeOrig[vC] = vO;
				tempUse.push(vO);
			} else {
				data.terminals.grow(1, vO);
			}
		}
		IndexComparer<node> idx;
		data.terminals.quicksort(idx);

		// add all edges of comp to m_graph
		// and find start adjEntry
		for (edge e : comp.edges) {
			node uO = comp.original(e->source());
			node vO = comp.original(e->target());
			T weight = comp.weight(e);
			edge eC = m_graph.newEdge(m_nodeCopy[uO], m_nodeCopy[vO], weight);
			data.cost += weight;
			if (m_isTerminal[uO]) {
				data.start = eC->adjSource();
			} else
			if (m_isTerminal[vO]) {
				data.start = eC->adjTarget();
			}
		}

		// cleanup m_nodeCopy
		for (node vO : tempUse) {
			m_nodeCopy[vO] = nullptr;
		}

		m_components.push(data);
	}

	void remove(int id)
	{
#ifdef OGDF_FULLCOMPONENTSTORE_REMOVE_IN_GRAPH_REPRESENTATION_ALSO
		// TODO: remove in graph also
		auto &comp = m_components[id];
		if (comp.terminals.size() == 2) {
			m_graph.delEdge(comp.start->theEdge());
		} else {
			BoundedStack<node> stack(2 * comp.terminals.size() - 3);
			stack.push(comp.start->twinNode());
			m_graph.delEdge(comp.start->theEdge());
			while (!stack.empty()) {
				const node v = stack.pop();
				if (!isTerminal(v)) {
					for (adjEntry adj : v->adjEntries) {
						stack.push(adj->twinNode());
					}
					m_graph.delNode(v);
				}
			}
		}
#endif
		if (m_components.size() == id + 1) {
			m_components.pop();
		} else {
			m_components[id] = m_components.popRet();
		}
	}

	//! Returns the number of full components in the store
	int size() const
	{
		return m_components.size();
	}

	//! \brief Checks if the store does not contain any full components
	bool isEmpty() const
	{
		return m_components.empty();
	}

	//! \brief Returns the list of terminals in the full component with given id
	const Array<node> &terminals(int id) const
	{
		OGDF_ASSERT(id >= 0);
		OGDF_ASSERT(id < m_components.size());
		return m_components[id].terminals;
	}

	//! \brief checks if a given node t is a terminal in the full component with given id
	bool isTerminal(int id, node t) const
	{
		OGDF_ASSERT(id >= 0);
		OGDF_ASSERT(id < m_components.size());
		return m_components[id].terminals.linearSearch(t) != -1;
	}

	bool isTerminal(node v) const
	{
		return m_isTerminal[m_nodeOrig[v]];
	}

	//! \brief Returns the sum of edge costs of this full component
	T cost(int i) const
	{
		OGDF_ASSERT(i >= 0);
		OGDF_ASSERT(i < m_components.size());
		return m_components[i].cost;
	}

	adjEntry start(int i) const
	{
		OGDF_ASSERT(i >= 0);
		OGDF_ASSERT(i < m_components.size());
		return m_components[i].start;
	}

	const EdgeWeightedGraph<T> &graph() const
	{
		return m_graph;
	}

	node original(node v) const
	{
		OGDF_ASSERT(m_nodeOrig[v] != nullptr);
		return m_nodeOrig[v];
	}

	template<typename Fun>
	void foreachAdjEntry(int i, Fun f) const
	{
		adjEntry start = m_components[i].start;
		int size = m_components[i].terminals.size();
		if (size == 2) {
			f(start->twin());
			return;
		}
		// size >= 3: do DFS over nonterminals (terminals are only leaves)
		BoundedStack<adjEntry> stack(2*size - 2);
		stack.push(start);
		while (!stack.empty()) {
			const adjEntry back = stack.pop()->twin();
			f(back);
			if (!this->isTerminal(back->theNode())) {
				for (adjEntry adj = back->cyclicSucc(); adj != back; adj = adj->cyclicSucc()) {
					stack.push(adj);
				}
			}
		}
	}

	// \brief Do f(v) for each (original) node v of degree at least 3 in component with given id
	template<typename Fun>
	void foreachNode(int id, Fun f) const
	{
		f(original(start(id)->theNode()));
		foreachAdjEntry(id, [&](adjEntry back) {
			f(original(back->theNode()));
		});
	}

	// \brief Do f(e) for each (original) edge e in component with given id
	template<typename Fun>
	void foreachEdge(int id, const NodeArray<NodeArray<edge>> &pred, Fun f) const
	{
		foreachAdjEntry(id, [&](adjEntry back) {
			const node u = original(back->twinNode());
			for (node v = original(back->theNode()); pred[u][v]; v = pred[u][v]->opposite(v)) {
				f(pred[u][v]);
			}
		});
	}

	// \brief Do f(v) for each node v (also of degree 2) in component with given id
	template<typename Fun>
	void foreachNode(int id, const NodeArray<NodeArray<edge>> &pred, Fun f) const
	{
		if (m_components[id].terminals.size() == 3) {
			// use a variant that works when only pred[t] has been filled for all terminals t
			adjEntry start = m_components[id].start;
			const node c = start->twinNode();
			f(original(c));
			for (adjEntry adj : c->adjEntries) {
				const node u = original(adj->twinNode());
				node v = original(c);
				while (v != u) {
					v = pred[u][v]->opposite(v);
					f(v);
				}
			}
			return;
		}
		f(original(start(id)->theNode()));
		foreachAdjEntry(id, [&](adjEntry back) {
			const node u = original(back->twinNode());
			for (node v = original(back->theNode()); pred[u][v]; v = pred[u][v]->opposite(v)) {
				f(v);
			}
		});
	}
};

/*!
 * \brief A data structure to store full components with extra data for each component
 */
template<typename T, typename ExtraDataType>
class FullComponentWithExtraStore : public FullComponentStore<T, ExtraDataType>
{
public:
	using FullComponentStore<T, ExtraDataType>::FullComponentStore;

	//! \brief Returns a reference to the extra data of this full component
	ExtraDataType &extra(int i)
	{
		OGDF_ASSERT(i >= 0);
		OGDF_ASSERT(i < this->m_components.size());
		return this->m_components[i].extra;
	}

	//! \brief Returns a const reference to the extra data of this full component
	const ExtraDataType &extra(int i) const
	{
		OGDF_ASSERT(i >= 0);
		OGDF_ASSERT(i < this->m_components.size());
		return this->m_components[i].extra;
	}
};

template<typename T>
struct LossMetadata {
	T loss; //!< The loss of a component
	List<edge> bridges; //!< List of non-loss edges
	LossMetadata()
	  : loss(0)
	  , bridges()
	{
	}
};

/*!
 * \brief A data structure to store full components with additional "loss" functionality
 */
template<typename T>
class FullComponentWithLossStore : public FullComponentWithExtraStore<T, LossMetadata<T>>
{
protected:
	NodeArray<node> m_lossTerminal; //!< Indicates which Steiner node is connected to which terminal through the loss edges, indexed by the Steiner node

	/*!
	 * \brief Starting from a Steiner node find the nearest terminal along a shortest path
	 * @param u Steiner node to start from
	 * @param pred The shortest path predecessor data structure
	 * @return first terminal on a shortest path starting from a Steiner node
	 */
	node findLossTerminal(const node u, const NodeArray<edge> &pred)
	{
		if (!m_lossTerminal[u]
		 && pred[u]) {
			m_lossTerminal[u] = findLossTerminal(pred[u]->opposite(u), pred);
		}

		return m_lossTerminal[u];
	}

public:
	using FullComponentWithExtraStore<T, LossMetadata<T>>::FullComponentWithExtraStore;

	//! \brief Compute the loss, both edge set and value, of all full components
	void computeAllLosses()
	{
		m_lossTerminal.init(this->m_graph, nullptr);

		// add zero-cost edges between all terminals (to be removed later),
		// and set m_lossTerminal mapping for terminals
		List<edge> zeroEdges;
		const node s = this->m_terminals.front();
		const node sC = this->m_nodeCopy[s];
		m_lossTerminal[sC] = s;
		for (ListConstIterator<node> it = this->m_terminals.begin().succ(); it.valid(); ++it) {
			const node v = *it;
			const node vC = this->m_nodeCopy[v];
			m_lossTerminal[vC] = v;
			zeroEdges.pushBack(this->m_graph.newEdge(sC, vC, 0));
		}

		// compute minimum spanning tree
		NodeArray<edge> pred(this->m_graph);
		EdgeArray<bool> isLossEdge(this->m_graph, false);
		computeMinST(sC, this->m_graph, this->m_graph.edgeWeights(), pred, isLossEdge);

		// remove zero-cost edges again
		for (edge e : zeroEdges) {
			this->m_graph.delEdge(e);
		}

		// find loss bridges and compute loss value
		for (int id = 0; id < this->size(); ++id) {
			this->foreachAdjEntry(id, [&](adjEntry adj) {
				edge e = adj->theEdge();
				if (!isLossEdge[e]) {
					this->extra(id).bridges.pushBack(e);
					findLossTerminal(e->source(), pred);
					findLossTerminal(e->target(), pred);
				} else {
					this->extra(id).loss += this->m_graph.weight(e);
				}
			});
		}
	}

	//! \brief Returns the loss value of full component with given id
	T loss(int id) const
	{
		return this->extra(id).loss;
	}

	//! \brief Returns a list of non-loss edges (that are bridges between the Loss components) of full component with given id
	const List<edge> &lossBridges(int id) const
	{
		return this->extra(id).bridges;
	}

	/*!
	 * \brief Returns the terminal (in the original graph) that belongs to a given node v (in the store) according to the Loss of the component
	 *
	 * A terminal and a Steiner node are linked if the terminal is the first one on the shortest loss path
	 * starting from the Steiner node.
	 */
	node lossTerminal(node v) const
	{
		OGDF_ASSERT(m_lossTerminal.valid());
		return m_lossTerminal[v];
	}
};

} // end namespace steinertree
} // end namespace ogdf
