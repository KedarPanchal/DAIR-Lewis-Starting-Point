#ifndef START_POINT_HPP
#define START_POINT_HPP

#include <vector>
#include <tuple>

#include "cgal_types.hpp"
#include "graph_construction.hpp"
#include "cache.hpp"

using SCC = std::tuple<Graph, PolygonSet, fscalar>;

std::vector<SCC> scc_areas(const Graph& g, Cache& cache);

#endif
