#ifndef GRAPH_ALGORITHMS_HPP
#define GRAPH_ALGORITHMS_HPP

#include <vector>
#include <unordered_set>

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

PolygonSet computeCoverage(const Node& source, const Node& target, const Graph& g);

std::unordered_set<std::pair<Node, Node>, pair_hash> computeCoverageEdges(const Node& source, PolygonSet& CCR, const Graph& g);

Node bruteForceBestStartingPoint(const Graph& g);
Node johnsonBestStartingPoint(const Graph& g);

#endif
