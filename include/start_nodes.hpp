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

void process_area_view(
    std::ranges::subrange<vector_iter> view, 
    Cache& cache, 
    std::vector<std::tuple<Graph, PolygonSet, fscalar>>& areas,
    std::mutex& areas_mutex
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
