#include <list>
#include <unordered_set>
#include <stack>
#include <utility>
#include <tuple>

#include "start_nodes.hpp"
#include "graph_construction.hpp"
#include "cgal_types.hpp"

// -- GEOMETRIC HELPER FUNCTIONS ----------------------------------------------

// Compute area using the shoelace formula for linear edges and the area of a circular segment for curved edges
// This is used to compute the area of individual holes and outer boundaries
fscalar polygon_area(const CurvedTraits::Polygon_2& polygon) {
    fscalar current_area = 0;
    for (auto edge = polygon.curves_begin(); edge != polygon.curves_end(); ++edge) {
        // Convert from CGAL::Sqrt_extension to fscalar using the formula a0 + a1 * sqrt(root)
        fscalar x0 = edge->source().x().a0() + edge->source().x().a1() * CGAL::sqrt(edge->source().x().root());
        fscalar y0 = edge->source().y().a0() + edge->source().y().a1() * CGAL::sqrt(edge->source().y().root());
        fscalar x1 = edge->target().x().a0() + edge->target().x().a1() * CGAL::sqrt(edge->target().x().root());
        fscalar y1 = edge->target().y().a0() + edge->target().y().a1() * CGAL::sqrt(edge->target().y().root());
        
        // Shoelace formula for linear edges, area of circular segment for curved edges
        if (edge->is_linear()) current_area += 0.5 * (x0 * y1 - x1 * y0);
        // Area of circular segment formula for curved edges
        else {
            fscalar cx = edge->supporting_circle().center().x();
            fscalar cy = edge->supporting_circle().center().y();
            fscalar r2 = edge->supporting_circle().squared_radius();
            fscalar r = CGAL::sqrt(edge->supporting_circle().squared_radius());
            fscalar theta0 = convert<fscalar>(boost::multiprecision::atan2(convert<hpscalar>(y0 - cy), convert<hpscalar>(x0 - cx)));
            fscalar theta1 = convert<fscalar>(boost::multiprecision::atan2(convert<hpscalar>(y1 - cy), convert<hpscalar>(x1 - cx)));
            fscalar dtheta = theta1 - theta0;
            current_area += 0.5 * (cx * (y1 - y0) - cy * (x1 - x0) + r2 * dtheta);
        }
    }
    return current_area;
}

// Compute area of a polygon set by splitting it into separate polygons
fscalar area(const PolygonSet& ps) {
    fscalar total_area = 0;
    // Find the area of the polygon set by splitting it into its polygons with holes
    std::list<CurvedTraits::Polygon_with_holes_2> polygons;
    ps.polygons_with_holes(std::back_inserter(polygons));
    for (const auto& polygon : polygons) {
        // Start with area of outer boundary
        total_area += polygon_area(polygon.outer_boundary());
        // Subtract area from holes
        for (const auto& hole : polygon.holes()) {
            auto reversed_hole = hole;
            reversed_hole.reverse_orientation();
            total_area -= polygon_area(reversed_hole);
        }
    }

    return total_area;
}

// Compute area of all edges in a strongly connected component
fscalar scc_area(const Graph& scc) {
    PolygonSet ccr;
    for (const auto& [node, neighbors] : scc) {
        for (const auto& [_, _, ccr_prime] : neighbors) ccr.join(ccr_prime);
    }

    return area(ccr);
}

// -- GRAPH HELPER FUNCTIONS --------------------------------------------------
// Find the finishing times of the nodes using DFS
// These times are stored as a stack
void finishing_times(
        const Graph& g, 
        const Node& node, 
        std::unordered_set<Node>& visited, 
        std::stack<Node>& stack
        ) {
    visited.insert(node);
    for (const auto& [neighbor, _, _] : g.at(node)) {
        if (visited.find(neighbor) == visited.end()) {
            finishing_times(g, neighbor, visited, stack);
        }
    }
    stack.push(node);
}

// Find the transpose of a graph for Kosaraju's algorithm
Graph transpose(const Graph& g) {
    Graph g_prime;
    for (const auto& [node, neighbors] : g) {
        for (const auto& [neighbor, vec, ccr] : neighbors) {
            g_prime[neighbor].emplace_back(node, vec, ccr);
        }
    }

    return g_prime;
}

// Find the strongly connected component containing a given node using DFS
void find_strongly_connected_component(
        const Graph& g, 
        const Node& node, 
        std::unordered_set<Node>& visited, 
        Graph& scc
        ) {
    visited.insert(node);
    // Add the node to the strongly connected component
    // This is done in case a node has no outgoing edges
    scc.try_emplace(node, std::list<std::tuple<Node, Vector, PolygonSet>>{});
    for (const auto& [neighbor, vec, ccr] : g.at(node)) {
        if (visited.find(neighbor) == visited.end()) {
            scc[neighbor].emplace_back(node, vec, ccr);
            find_strongly_connected_component(g, neighbor, visited, scc);
        }
    }
}

// Implement Kosaraju's algorithm for finding strongly connected components of a directed graph
// The best starting point(s) is the SCC that covers the most area
// Using std:list is okay here since we're not doing any lookups, just iterating over it
// TODO: Eventually replace this with Tarjan's algorithm for better performance
std::list<std::pair<Graph, fscalar>> scc_areas(const Graph& g) {
    std::stack<Node> stack;
    std::unordered_set<Node> visited;
    std::list<std::pair<Graph, fscalar>> areas;

    // Find finishing times of all nodes in the graph using DFS
    for (const auto& [node, _] : g) {
        if (visited.find(node) == visited.end()) finishing_times(g, node, visited, stack);
    }

    Graph g_prime = transpose(g);

    // Find SCCs by performing DFS on the transposed graph in the order of finishing times
    visited.clear();
    while (!stack.empty()) {
        Node current_node = stack.top();
        stack.pop();

        std::cout << "Finding SCC for node " << current_node.ID() << std::endl;

        if (visited.find(current_node) != visited.end()) continue;

        Graph scc;
        find_strongly_connected_component(g_prime, current_node, visited, scc);
        if (scc.empty()) continue;

        fscalar current_area = scc_area(scc);
        std::cout << "Found SCC with area " << current_area << std::endl;
        areas.emplace_back(std::move(scc), current_area);
    }

    return areas;
}
