#ifndef GRAPH_ALGORITHMS_HPP
#define GRAPH_ALGORITHMS_HPP

#include <unordered_set>
#include <list>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

// Since all weights are 1, just use -1 to represent infinity for simplicity
#define INF -1

struct pair_hash {
    template <typename T1, typename T2>
    size_t operator()(const std::pair<T1, T2>& p) const {
        size_t seed = 0;
        boost::hash_combine(seed, p.first);
        boost::hash_combine(seed, p.second);
        return seed;
    }
};

PolygonSet compute_coverage(const Node& source, const Node& target, const Graph& g);

std::unordered_set<std::pair<Node, Node>, pair_hash> compute_coverage_edges(const Node& source, PolygonSet& CCR, const Graph& g);

std::list<std::pair<Graph, fscalar>> scc_areas(const Graph& g);

#endif
