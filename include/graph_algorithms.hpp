#ifndef GRAPH_ALGORITHMS_HPP
#define GRAPH_ALGORITHMS_HPP

#include <vector>
#include <unordered_set>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

#define INF -1

std::vector<std::vector<std::optional<Node>>> floyd_warshall(const Graph& g);

PolygonSet computeCoverage(const Node& source, const Node& target, const Graph& g);

std::unordered_set<std::pair<Node, Node>> computeCoverageEdges(const Node& source, PolygonSet& CCR, const Graph& g);

#endif
