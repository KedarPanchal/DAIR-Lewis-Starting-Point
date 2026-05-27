#include "graph_algorithms.hpp"

#include <vector>

#include <boost/container_hash/hash.hpp>

#include "graph_construction.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------
namespace std {
    template <>
    struct hash<std::pair<Node, Node>> {
        size_t operator()(const std::pair<Node, Node>& p) const {
            size_t seed = 0;
            boost::hash_combine(seed, p.first);
            boost::hash_combine(seed, p.second);
            return seed;
        }
    };
}

PolygonSet computeCoverage(const Node& source, const Node& target, const Graph& g) {
    for (const auto& [neighbor, _, ccr] : g.at(source)) {
        if (neighbor == target) return ccr;
    }

    return PolygonSet();
}

// -- GRAPH ALGORITHMS --------------------------------------------------------

// Performs the Floyd-Warshall algorithm and returns the predecessor matrix
std::vector<std::vector<std::optional<Node>>> floyd_warshall(const Graph& g) {
    // Create the distance matrix initialized with INF and set the diagonal to 0
    // Also set neighboring vertices to 1
    std::vector<std::vector<int>> dist(g.size(), std::vector<int>(g.size(), INF));
    // Initialize prev matrix with nullopt
    std::vector<std::vector<std::optional<Node>>> prev(g.size(), std::vector<std::optional<Node>>(g.size(), std::nullopt));

    for (const auto& [u, neighbors] : g)  {
        dist[u][u] = 0;
        prev[u][u] = u;
        for (const auto& [v, vec, region] : neighbors) {
            dist[u][v] = 1;
            prev[u][v] = u;
        }
    }

    // Actually run the algorithm
    for (size_t k = 0; k < dist.size(); ++k) {
        for (size_t i = 0; i < dist.size(); ++i) {
            if (i == k) continue;
            for (size_t j = 0; j < dist.size(); ++j) {
                if (j == k || j == i) continue;
                // Skip if either path is currently unreachable
                if (dist[i][k] == INF || dist[k][j] == INF) continue;
                if (dist[i][j] > dist[i][k] + dist[k][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    prev[i][j] = prev[k][j];
                }
            }
        }
    }

    return prev;
}

// ComputeCoveredEdges algorithm from Lewis's doctoral dissertation (Algorithm 5)
std::unordered_set<std::pair<Node, Node>> computeCoverageEdges(const Node& source, PolygonSet& ccr, const Graph& g) {
    std::vector<std::vector<std::optional<Node>>> p = floyd_warshall(g);
    std::unordered_set<std::pair<Node, Node>> covered;

    // For all edges of G
    for (const auto& [u, neighbors] : g) {
        for (const auto& v : neighbors) {
            std::optional<Node> s = p[source][u];
            std::optional<Node> t = p[std::get<0>(v)][source];
            PolygonSet ccr_prime = std::get<2>(v);

            PolygonSet difference = ccr_prime;
            difference.difference(ccr);
            if (s.has_value() && t.has_value() && !difference.is_empty()) {
                Node end = u;
                while (end != *s) {
                    Node penultimate = *p[source][end];
                    ccr.join(computeCoverage(penultimate, end, g));
                    covered.emplace(penultimate, end);
                    end = penultimate;
                }
                ccr.join(ccr_prime);
                end = source;
                while (end != *t) {
                    Node penultimate = *p[end][source];
                    ccr.join(computeCoverage(end, penultimate, g));
                    covered.emplace(end, penultimate);
                    end = penultimate;
                }
            }
        }
    }

    return covered;
}
