#ifndef GRAPH_ALGORITHMS_HPP
#define GRAPH_ALGORITHMS_HPP

#include <vector>

#include "graph_construction.hpp"

#define INF -1

std::vector<std::vector<int>> floyd_warshall(const Graph& g);

#endif
