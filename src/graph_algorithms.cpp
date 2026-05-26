#include "graph_algorithms.hpp"

#include <vector>
#include <algorithm>

#include "graph_construction.hpp"

std::vector<std::vector<int>> floyd_warshall(const Graph& g) {
    // Create the distance matrix initialized with INF and set the diagonal to 0
    // Also set neighboring vertices to 1
    std::vector<std::vector<int>> dist(g.size(), std::vector<int>(g.size(), INF));
    for (const auto& [u, neighbors] : g)  {
        dist[u][u] = 0;
        for (const auto& [v, vec, region] : neighbors) dist[u][v] = 1;
    }

    // Actually run the algorithm
    for (size_t k = 0; k < dist.size(); ++k) {
        for (size_t i = 0; i < dist.size(); ++i) {
            if (i == k) continue;
            for (size_t j = 0; j < dist.size(); ++j) {
                if (j == k || j == i) continue;
                // Skip if either path is currently unreachable
                if (dist[i][k] == INF || dist[k][j] == INF) continue;
                dist[i][j] = std::min(dist[i][j], dist[i][k] + dist[k][j]);
            }
        }
    }

    return dist;
}

// ComputeCoveredEdges algorithm from Lewis's doctoral dissertation (Algorithm 5)
std::unordered_set<std::pair<Node, Node>> computeCoverageEdges(const Node& source, PolygonSet& CCR, const Graph& g) {
    std::vector<std::vector<int>> p = floyd_warshall(g);
    std::unordered_set<std::pair<Node, Node>> covered;

    // For all edges of G
    for (const auto& [u, neighbors] : g) {
        for (const auto& v : neighbors) {
            int s = p[source][u];
            int t = p[std::get<0>(v)][source];
        }
    }

    return covered;
}
