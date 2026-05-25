#ifndef GRAPH_CONSTRUCTION_HPP
#define GRAPH_CONSTRUCTION_HPP

#include <list>
#include <unordered_map>
#include <optional>

#include "cgal_types.hpp"

// Graph node, with numeric ID for Johnson's algorithm
struct Node {
    const size_t id;
    const Segment segment;

    Node(size_t id, const Segment& segment) : id{id}, segment{segment} {}
    operator size_t() const { return id; }
};

// Hash specialization that just uses the numeric ID of the node
namespace std {
    template<>
    struct hash<Node> {
        size_t operator()(const Node& node) const {
            return hash<size_t>()(node.id);
        }
    };
}

// Adjacency list representation of the graph
using Graph = std::unordered_map<Node, std::list<std::pair<Node, fscalar>>>;

// Adds layers around the boundary of a single polygon
inline void addLayerHelper(const Polygon& w, Graph& g, fscalar l, fscalar o_max) {
    size_t node_id = 0;

    for (auto eit = w.edges_begin(); eit != w.edges_end(); ++eit) {
        fscalar length = CGAL::sqrt(eit->squared_length());
        if (length == l) {
            // Add segment *eit as a node of g
            Node node{node_id++, *eit};
            g[node] = std::list<std::pair<Node, fscalar>>{};
        } else if (length > l) {
            // Ceiling value since CGAL doesn't expose a ceil function for Kernel::FT (BURST::numeric::fscalar)
            size_t ceiling = static_cast<size_t>(CGAL::to_double((length - l) / o_max)) + 1;
            fscalar o = (length - l) / ceiling;
            
            Point x1 = eit->source();
            Point x2 = eit->source();

            Vector direction = eit->to_vector();
            direction = direction / CGAL::sqrt(direction.squared_length());

            while (x2 != eit->target()) {
                // x_2 <- x_1 translated l along *eit
                x2 = x1 + direction * l;
                // Add segment x1x2 as a node of g
                Node node{node_id++, Segment(x1, x2)};
                g[node] = std::list<std::pair<Node, fscalar>>{};
                // x_1 <- x_1 translated o along *eit
                x1 += direction * o;
            }
        }
    }
}

// AddLayer algorithm from Lewis's doctoral dissertation (Algorithm 3)
// Well, AddLayerHelper is the actual algorithm, but it's applied to every boundary and hole of the polygon
inline void addLayer(const HoledPolygon& w, Graph& g, fscalar l, fscalar o_max) {
    addLayerHelper(w.outer_boundary(), g, l, o_max);
    for (auto hit = w.holes_begin(); hit != w.holes_end(); ++hit) {
        addLayerHelper(*hit, g, l, o_max);
    }
}

// Helper function for intersection from a raycast

// HasEdge algorithm from Lewis's doctoral dissertation (Algorithm 4)
inline std::optional<fscalar> hasEdge(const HoledPolygon& w, const Segment& source, const Segment& target, hpscalar theta_max) {
    auto d1 = Vector{source.source(), target.target()}.transform(Transformation(CGAL::ROTATION, boost::multiprecision::sin(theta_max), boost::multiprecision::cos(theta_max)));
    auto d2 = Vector{source.target(), target.source()}.transform(Transformation(CGAL::ROTATION, boost::multiprecision::sin(-theta_max), boost::multiprecision::cos(-theta_max)));
    
    fscalar angle = d1 * d2 / (CGAL::sqrt(d1.squared_length()) * CGAL::sqrt(d2.squared_length()));
    if (angle > CGAL_PI) return std::nullopt;

    // Compute p1 and p2
}

#endif
