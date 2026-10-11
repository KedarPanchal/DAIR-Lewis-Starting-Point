#ifndef START_POINT_HPP
#define START_POINT_HPP

#include <vector>
#include <tuple>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

using SCC = std::tuple<Graph, PolygonSet, fscalar>;

std::vector<SCC> scc_areas(const Graph& g);
std::vector<SCC> top_n_sccs(const std::vector<SCC>& areas, size_t n);

#endif
