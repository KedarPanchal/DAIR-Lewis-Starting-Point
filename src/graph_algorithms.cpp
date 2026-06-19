#include "graph_algorithms.hpp"

#include <vector>

#include <CGAL/number_utils.h>

#include <boost/container_hash/hash.hpp>

#include "graph_construction.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------
PolygonSet computeCoverage(const Node& source, const Node& target, const Graph& g) {
    for (const auto& [neighbor, _, ccr] : g.at(source)) {
        if (neighbor == target) return ccr;
    }

    return PolygonSet();
}

fscalar area(const PolygonSet& ps) {
    fscalar area = 0;
    // Find the area of the polygon set by splitting it into its polygons with holes
    std::vector<HoledPolygon> polygons;
    ps.polygons_with_holes(std::back_inserter(polygons));
    for (const auto& polygon : polygons) {
        area += CGAL::abs(polygon.outer_boundary().area());
        // Subtract area from holes
        for (const auto& hole : polygon.holes()) {
            area -= CGAL::abs(hole.area());
        }
    }

    return area;
}

// -- PAPER GRAPH ALGORITHMS --------------------------------------------------

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
                if (dist[i][j] == INF || dist[i][j] > dist[i][k] + dist[k][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    prev[i][j] = prev[k][j];
                }
            }
        }
    }

    return prev;
}

// ComputeCoveredEdges algorithm from Lewis's doctoral dissertation (Algorithm 5)
std::unordered_set<std::pair<Node, Node>, pair_hash> computeCoverageEdges(const Node& source, PolygonSet& ccr, const Graph& g) {
    std::vector<std::vector<std::optional<Node>>> p = floyd_warshall(g);
    std::unordered_set<std::pair<Node, Node>, pair_hash> covered;

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

// -- CYCLE ALGORITHMS --------------------------------------------------------

Node bruteForceBestStartingPoint(const Graph& g) {
    // Run ComputeCoveredEdges for each node in the graph
    // Map each node to its covered area
    std::unordered_map<Node, fscalar> coverage_map;
    for (const auto& [node, _] : g) {
        PolygonSet ccr;
        computeCoverageEdges(node, ccr, g);
        coverage_map[node] = area(ccr);
    }

    // Find the node with the maximum covered area
    Node best_node = g.begin()->first;
    for (const auto& [node, coverage] : coverage_map) {
        if (coverage > coverage_map[best_node]) {
            best_node = node;
        }
    }

    return best_node;
}
