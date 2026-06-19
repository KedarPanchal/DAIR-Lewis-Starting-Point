#include "graph_algorithms.hpp"

#include <vector>
#include <iterator>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <stack>

#include <CGAL/number_utils.h>

#include <boost/container_hash/hash.hpp>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------
PolygonSet compute_coverage(const Node& source, const Node& target, const Graph& g) {
    for (const auto& [neighbor, _, ccr] : g.at(source)) {
        if (neighbor == target) return ccr;
    }

    return PolygonSet();
}

// TODO: Actually implement this
fscalar area(const CurvedTraits::Polygon_2& polygon) {
    return CGAL::abs(polygon.area());
}

fscalar area(const PolygonSet& ps) {
    fscalar total_area = 0;
    // Find the area of the polygon set by splitting it into its polygons with holes
    std::vector<CurvedTraits::Polygon_with_holes_2> polygons;
    ps.polygons_with_holes(std::back_inserter(polygons));
    for (const auto& polygon : polygons) {
        total_area += area(polygon.outer_boundary());
        // Subtract area from holes
        for (const auto& hole : polygon.holes()) {
            auto reversed_hole = hole;
            reversed_hole.reverse_orientation();
            total_area -= area(reversed_hole);
        }
    }

    return total_area;
}

void finishing_times(const Graph& g, const Node& node, std::unordered_set<Node>& visited, std::stack<Node>& stack, size_t min_node) {
    visited.insert(node);
    for (const auto& [neighbor, _, _] : g.at(node)) {
        if (visited.find(neighbor) == visited.end() && neighbor.ID() >= min_node) {
            finishing_times(g, neighbor, visited, stack, min_node);
        }
    }
    stack.push(node);
}

Graph transpose(const Graph& g) {
    Graph g_prime;
    for (const auto& [node, neighbors] : g) {
        for (const auto& [neighbor, vec, ccr] : neighbors) {
            g_prime[neighbor].emplace_back(node, vec, ccr);
        }
    }

    return g_prime;
}

void find_strongly_connected_component(const Graph& g, const Node& node, std::unordered_set<Node>& visited, Graph& scc, size_t min_node) {
    visited.insert(node);
    for (const auto& [neighbor, vec, ccr] : g.at(node)) {
        if (visited.find(neighbor) == visited.end() && neighbor.ID() >= min_node) {
            scc[neighbor].emplace_back(node, vec, ccr);
            find_strongly_connected_component(g, neighbor, visited, scc, min_node);
        }
    }
}

void unblock(const Node& node, std::vector<bool>& blocked, std::unordered_map<Node, std::unordered_set<Node>>& predecessors) {
    blocked[node.ID()] = false;
    for (const Node& predecessor : predecessors[node]) {
        if (blocked[predecessor.ID()]) unblock(predecessor, blocked, predecessors);
    }
    predecessors[node].clear();
}

bool circuit(
        const Graph& g, 
        const Node& v, 
        const Node& s, 
        std::vector<bool>& blocked, 
        std::unordered_map<Node, std::unordered_set<Node>>& predecessors, 
        std::stack<Node>& stack, 
        PolygonSet& ccr, 
        std::unordered_map<Node, fscalar>& coverage_map
        ) {
    bool found_cycle = false;

    stack.push(v);
    blocked[v.ID()] = true;
    for (const auto& [w, _, coverage] : g.at(v)) {
        if (w == s) {
            // Found a cycle
            coverage_map[s] += area(ccr);
            found_cycle = true;
        } else if (!blocked[w.ID()]) {
            PolygonSet new_ccr = ccr;
            new_ccr.join(coverage);
            if (circuit(g, w, s, blocked, predecessors, stack, new_ccr, coverage_map)) found_cycle = true;
        }
    }
    if (found_cycle) unblock(v, blocked, predecessors);
    else for (const auto& [w, _, _] : g.at(v)) predecessors[w].insert(v);

    return found_cycle;
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
std::unordered_set<std::pair<Node, Node>, pair_hash> compute_coverage_edges(const Node& source, PolygonSet& ccr, const Graph& g) {
    std::vector<std::vector<std::optional<Node>>> p = floyd_warshall(g);
    std::unordered_set<std::pair<Node, Node>, pair_hash> covered;

    // For all edges of G
    for (const auto& [u, neighbors] : g) {
        for (const auto& [v, _, ccr_prime] : neighbors) {
            std::optional<Node> s = p[source][u];
            std::optional<Node> t = p[v][source];

            PolygonSet difference = ccr_prime;
            difference.difference(ccr);
            if (s.has_value() && t.has_value() && !difference.is_empty()) {
                Node end = u;
                while (end != *s) {
                    Node penultimate = *p[source][end];
                    ccr.join(compute_coverage(penultimate, end, g));
                    covered.emplace(penultimate, end);
                    end = penultimate;
                }
                ccr.join(ccr_prime);
                end = source;
                while (end != *t) {
                    Node penultimate = *p[end][source];
                    ccr.join(compute_coverage(end, penultimate, g));
                    covered.emplace(end, penultimate);
                    end = penultimate;
                }
            }
        }
    }

    return covered;
}

// -- CYCLE ALGORITHMS --------------------------------------------------------

// Brute force algorithm for finding the best starting point for Lewis's algorithm
// Used to validate the actual algorithm for finding the best starting point
Node brute_force_best_starting_point(const Graph& g) {
    // Run ComputeCoveredEdges for each node in the graph
    // Map each node to its covered area
    std::unordered_map<Node, fscalar> coverage_map;
    for (const auto& [node, _] : g) {
        PolygonSet ccr;
        compute_coverage_edges(node, ccr, g);
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

// Implement Kosaraju's algorithm for finding strongly connected components of a directed graph
std::vector<Graph> kosaraju(const Graph& g, Node min_node) {
    std::vector<Graph> sccs;
    std::stack<Node> stack;
    std::unordered_set<Node> visited;

    finishing_times(g, min_node, visited, stack, min_node);
    Graph g_prime = transpose(g);

    visited.clear();
    for (const auto& [node, _] : g) {
        if (node.ID() < min_node.ID()) continue;
        if (visited.find(node) != visited.end()) continue;
        Graph scc;
        find_strongly_connected_component(g_prime, node, visited, scc, min_node);
        if (!scc.empty()) sccs.push_back(std::move(scc));
    }

    return sccs;
}

// Implement Johnson's algorithm for enumerating over the cycles of a directed graph
Node johnson_best_starting_point(const Graph& g) {
    std::vector<bool> blocked(g.size(), false);
    std::unordered_map<Node, std::unordered_set<Node>> predecessors;
    std::stack<Node> stack;
    std::unordered_map<Node, fscalar> coverage_map;
    
    // Store nodes in a vector for easy access by ID
    std::vector<Node> nodes_by_id(g.size());
    for (const auto& [node, _] : g) {
        nodes_by_id[node.ID()] = node;
    }
    Node s = nodes_by_id[0];

    while (s < g.size()) {
        // Find the strongly connected component that contains s
        std::vector<Graph> sccs = kosaraju(g, s);
        Graph scc;
        for (const Graph& component : sccs) {
            if (component.find(s) != component.end()) {
                scc = component;
                break;
            }
        }

        // Reset blocked and predecessors for the scc
        for (const auto& [node, _] : scc) {
            blocked[node.ID()] = false;
            predecessors[node].clear();
        }

        // Find cycles in the scc using Johnson's algorithm
        PolygonSet ccr;
        circuit(scc, s, s, blocked, predecessors, stack, ccr, coverage_map);
        s = nodes_by_id[s.ID() + 1];
    }
    
    // Find the node with the greatest coverage
    Node best_node = g.begin()->first;
    for (const auto& [node, coverage] : coverage_map) {
        if (coverage > coverage_map[best_node]) best_node = node;
    }
    return best_node;
}
