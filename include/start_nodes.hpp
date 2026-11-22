#ifndef START_POINT_HPP
#define START_POINT_HPP

#include <vector>
#include <tuple>

#include "cgal_types.hpp"
#include "graph_construction.hpp"
#include "cache.hpp"

using SCC = std::tuple<Graph, PolygonSet, fscalar>;
using vector_iter = decltype(std::declval<std::vector<Graph>>().begin());

fscalar polygon_area(const CurvedTraits::Polygon_2& polygon);
fscalar area(const PolygonSet& ps);

fscalar compute_coverage(
    const Graph& scc, 
    std::vector<SCC> areas, 
    Cache& cache
    );
void finishing_times(
    const Graph& g,
    const Node& node,
    std::unordered_set<Node>& visited,
    std::stack<Node>& stack
    );
Graph transpose(const Graph& g);
void find_strongly_connected_component(
    const Graph& g,
    const Node& node,
    std::unordered_set<Node>& visited,
    Graph& scc
    );

std::vector<SCC> scc_areas(const Graph& g, Cache& cache);

#endif
