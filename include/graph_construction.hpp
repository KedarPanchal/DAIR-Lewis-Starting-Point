#ifndef GRAPH_CONSTRUCTION_HPP
#define GRAPH_CONSTRUCTION_HPP

#include <vector>
#include <list>
#include <unordered_map>
#include <optional>

#include <CGAL/squared_distance_2.h>

#include <boost/container/small_vector.hpp>

#include "cgal_types.hpp"

// Graph node, with numeric ID for Johnson's algorithm
struct Node {
    const size_t id;
    const Segment segment;

    Node(size_t id, const Segment& segment) : id{id}, segment{segment} {}
    bool operator==(const Node& other) const { return id == other.id; }
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
using Graph = std::unordered_map<Node, std::list<std::pair<Node, Vector>>>;

// Adds layers around the boundary of a single polygon
inline void addLayerHelper(const Polygon& w, Graph& g, fscalar l, fscalar o_max) {
    static size_t node_id = 0;

    for (auto eit = w.edges_begin(); eit != w.edges_end(); ++eit) {
        fscalar length = CGAL::sqrt(eit->squared_length());
        if (length == l) {
            // Add segment *eit as a node of g
            Node node{node_id++, *eit};
            g[node] = std::list<std::pair<Node, Vector>>{};
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
                g[node] = std::list<std::pair<Node, Vector>>{};
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

// shootRay algorithm for checking safe actions
inline std::optional<Point> shootRay(const HoledPolygon& w, const Point& source, const Vector& direction) {
    // Construct an AABB tree for raycast queries
    std::vector<Segment> segments;
    for (const auto& edge : w.outer_boundary().edges()) segments.push_back(edge);
    for (const auto hole : w.holes()) {
        for (const auto& edge : hole.edges()) segments.push_back(edge);
    }

    AABBTree tree(segments.begin(), segments.end());
    tree.build();

    // Find all intersections of the ray with the polygon boundary
    Ray ray(source, direction);
    std::vector<std::optional<AABBTree::Intersection_and_primitive_id<Ray>::Type>> intersections;
    tree.all_intersections(ray, std::back_inserter(intersections));

    // Return the point closest to the source (but not equal to the source)
    Point closest = source;
    for (const auto& intersection : intersections) {
        if (const Point* ipoint = std::get_if<Point>(&(intersection->first))) if (
            closest == source ||
            *ipoint != source && 
            CGAL::squared_distance(source, *ipoint) < CGAL::squared_distance(source, closest)
            ) closest = *ipoint;
    }
    if (closest == source) return std::nullopt;
    else return closest;
}

// HasEdge algorithm from Lewis's doctoral dissertation (Algorithm 4)
inline std::optional<Vector> hasEdge(const HoledPolygon& w, const Segment& source, const Segment& target, hpscalar theta_max) {
    auto d1 = Vector{source.source(), target.target()}
        .transform(Transformation(
                    CGAL::ROTATION, 
                    boost::multiprecision::sin(theta_max), 
                    boost::multiprecision::cos(theta_max)
                    ));
    auto d2 = Vector{source.target(), target.source()}
        .transform(Transformation(
                    CGAL::ROTATION,
                    boost::multiprecision::sin(-theta_max),
                    boost::multiprecision::cos(-theta_max)
                    ));
    // Compute dot product-based angle since CGAL doesn't expose a function for computing angles between vectors 
    // Avoid division by zero, also, if this occurs, then the vector is degenerate anyway
    if (d1.squared_length() == 0 || d2.squared_length() == 0) return std::nullopt;
    fscalar angle = d1 * d2 / (CGAL::sqrt(d1.squared_length()) * CGAL::sqrt(d2.squared_length()));
    if (angle > CGAL_PI) return std::nullopt;

    // Compute p1 and p2 and check target membership
    std::optional<Point> p1 = shootRay(w, source.source(), Vector{source.source(), target.target()});
    std::optional<Point> p2 = shootRay(w, source.target(), Vector{source.target(), target.source()});
    if (
        !p1.has_value() ||
        !p2.has_value() ||
        !target.has_on(*p1) ||
        !target.has_on(*p2)
        ) return std::nullopt;

    // Create a quadrilateral with source.source(), source.target(), p1, and p2
    // and sort it according to angle from the point to the origin.
    // Sort in ascending order for clockwise ordering
    boost::container::small_vector<Point, 4> quad{source.source(), source.target(), *p1, *p2};
    std::sort(quad.begin(), quad.end(), [](const Point& a, const Point& b) {
        hpscalar angle_a = boost::multiprecision::atan2(to_hpscalar(a.y()), to_hpscalar(a.x()));
        hpscalar angle_b = boost::multiprecision::atan2(to_hpscalar(b.y()), to_hpscalar(b.x()));
        return angle_a < angle_b;
    });
    Polygon quadrilateral(quad.begin(), quad.end());
    
    // Check if any vertices of w are contained in the quadrilateral
    for (const auto& vertex : w.outer_boundary().vertices()) {
        if (quadrilateral.has_on_bounded_side(vertex)) return std::nullopt;
    }
    for (const auto& hole : w.holes()) {
        for (const auto& vertex : hole.vertices()) {
            if (quadrilateral.has_on_bounded_side(vertex)) return std::nullopt;
        }
    }

    // Return the mid-angle bisector of d1 and d2
    return (d1 * CGAL::sqrt(d2.squared_length())) + (d2 * CGAL::sqrt(d1.squared_length()));
}

inline Graph construct_graph(const HoledPolygon& w, const std::list<std::pair<fscalar, fscalar>>& parameters, fscalar theta_max) {
    Graph graph;
    // Add the nodes
    for (const auto& [l, o_max] : parameters) addLayer(w, graph, l, o_max);
    // Add the edges
    for (auto& [node, edges] : graph) {
        for (auto& [other_node, _] : graph) {
            auto maybe_edge = hasEdge(w, node.segment, other_node.segment, to_hpscalar(theta_max));
            if (maybe_edge.has_value()) edges.emplace_back(other_node, maybe_edge.value());
        }
    }

    return graph;
}

#endif
