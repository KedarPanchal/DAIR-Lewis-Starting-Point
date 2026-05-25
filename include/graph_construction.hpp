#ifndef GRAPH_CONSTRUCTION_HPP
#define GRAPH_CONSTRUCTION_HPP

#include <list>
#include <unordered_map>

#include <BURST/geometry.hpp>
#include <BURST/wall_space.hpp>
#include <BURST/numeric.hpp>

#include <CGAL/number_utils.h>
#include <CGAL/Aff_transformation_2.h>

// Graph node, with numeric ID for Johnson's algorithm
struct Node {
    const size_t id;
    const BURST::geometry::Segment2D segment;

    Node(size_t id, const BURST::geometry::Segment2D& segment) : id{id}, segment{segment} {}
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
using Graph = std::unordered_map<Node, std::list<Node>>;

// AddLayer algorithm from Lewis's doctoral dissertation (Algorithm 3)
inline void addLayer(const BURST::geometry::WallSpace& w, Graph& g, BURST::numeric::fscalar l, BURST::numeric::fscalar o_max) {
    size_t node_id = 0;

    for (auto eit = w.edges_begin(); eit != w.edges_end(); ++eit) {
        BURST::numeric::fscalar length = CGAL::sqrt(eit->squared_length());
        if (length == l) {
            // Add segment *eit as a node of g
            Node node{node_id++, *eit};
            g[node] = std::list<Node>{};
        } else if (length > l) {
            // Ceiling value since CGAL doesn't expose a ceil function for Kernel::FT (BURST::numeric::fscalar)
            size_t ceiling = static_cast<size_t>(CGAL::to_double((length - l) / o_max)) + 1;
            BURST::numeric::fscalar o = (length - l) / ceiling;
            
            BURST::geometry::Point2D x1 = eit->source();
            BURST::geometry::Point2D x2 = eit->source();

            BURST::geometry::Vector2D direction = eit->to_vector();
            direction = direction / CGAL::sqrt(direction.squared_length());

            while (x2 != eit->target()) {
                // x_2 <- x_1 translated l along *eit
                x2 = x1 + direction * l;
                // Add segment x1x2 as a node of g
                Node node{node_id++, BURST::geometry::Segment2D(x1, x2)};
                // x_1 <- x_1 translated o along *eit
                x1 += direction * o;
            }
        }
    }
}

#endif
