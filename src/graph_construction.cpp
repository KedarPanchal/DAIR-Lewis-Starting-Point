#include "graph_construction.hpp"

#include <iostream>
#include <iterator>
#include <optional>
#include <tuple>
#include <vector>

#include <sqlite3.h>

#include <CGAL/squared_distance_2.h>
#include <CGAL/Circle_2.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/Kernel/global_functions.h>
#include <CGAL/enum.h>

#include <boost/container/small_vector.hpp>
#include <boost/multiprecision/mpfr.hpp>
#include <boost/math/constants/constants.hpp>

#include "cgal_types.hpp"
#include "cache.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------

// Hash value function for boost::hash_combine
size_t hash_value(const Node& node) {
    return std::hash<size_t>()(node.ID());
}

// -- GRAPH CONSTRUCTION ALGORITHM --------------------------------------------

// Adds layers around the boundary of a single polygon
void add_layer_helper(const Polygon& w, Graph& g, fscalar l, fscalar o_max) {
    static size_t node_id = 0;

    for (auto eit = w.edges_begin(); eit != w.edges_end(); ++eit) {
        fscalar length = CGAL::sqrt(eit->squared_length());
        Vector unit_vector = eit->to_vector() / length;
        if (length == l) {
            // Add segment *eit as a node of g
            Node node{node_id++, *eit};
            g[node] =
                std::vector<std::tuple<Node, Vector, sqlite3_int64>>{};
        } else if (length > l) {
            // Ceiling value since CGAL doesn't expose a ceil function for
            // Kernel::FT
            size_t ceiling = static_cast<size_t>(std::ceil((length - l) / o_max));
            fscalar o = (length - l) / ceiling;

            Point x1 = eit->source();
            Point x2 = eit->source();
            Vector direction = unit_vector * l;
            Vector shift = unit_vector * o;

            // Ceiling is the number of segments to add, so we need to add
            // ceiling segments
            for (size_t i = 0; i <= ceiling; ++i) {
                // x_2 <- x_1 translated l along *eit
                x2 = x1 + direction;

                // Add segment x1x2 as a node of g
                Node node{node_id++, Segment(x1, x2)};
                g[node] =
                    std::vector<std::tuple<Node, Vector, sqlite3_int64>>{};

                // x_1 <- x_1 translated o along *eit
                x1 += shift;
            }
        }
    }
}

// add_layer algorithm from Lewis's doctoral dissertation (Algorithm 3)
// Well, add_layer_helper is the actual algorithm, but it's applied to every
// boundary and hole of the polygon
void add_layer(const HoledPolygon& w, Graph& g, fscalar l, fscalar o_max) {
    add_layer_helper(w.outer_boundary(), g, l, o_max);
    for (auto hit = w.holes_begin(); hit != w.holes_end(); ++hit) {
        add_layer_helper(*hit, g, l, o_max);
    }
}

// shoot_ray algorithm for checking safe actions
std::optional<Point> shoot_ray(
    const HoledPolygon& w,
    const Point& source,
    const Vector& direction,
    const AABBTree& tree) {
    // Find all intersections of the ray with the polygon boundary
    Ray ray(source, direction);
    auto result =
        tree.first_intersection(ray, [&](const auto& intersection) {
            const Segment& boundary = *intersection;
            const auto hit = CGAL::intersection(ray, boundary);

            if (!hit) {
                return false;
            }

            const Point* ipoint = std::get_if<Point>(&(*hit));
            return ipoint != nullptr && *ipoint == source;
        });

    if (!result.has_value()) {
        return std::nullopt;
    }

    const Point* ipoint = std::get_if<Point>(&(result->first));
    if (ipoint == nullptr) {
        return std::nullopt;
    }

    return *ipoint;
}

// has_edge algorithm from Lewis's doctoral dissertation (Algorithm 4)
std::optional<Vector> has_edge(
    const HoledPolygon& w,
    const Segment& source,
    const Segment& target,
    fscalar theta_max,
    const AABBTree& tree) {
    fscalar theta_max_radians = theta_max * boost::math::constants::pi<fscalar>() / 180;
    Vector d1 = Vector{source.source(), target.target()}.transform(
        Transformation(CGAL::ROTATION, std::sin(theta_max_radians), std::cos(theta_max_radians)));
    Vector d2 = Vector{source.target(), target.source()}.transform(
        Transformation(
            CGAL::ROTATION, std::sin(-theta_max_radians), std::cos(-theta_max_radians)));

    // Avoid division by zero, also, if this occurs, then the vector is
    // degenerate anyway
    const fscalar epsilon = 1e-9 * std::max(1.0, std::max(d1.squared_length(), d2.squared_length()));
    if (d1.squared_length() < epsilon || d2.squared_length() < epsilon) {
        std::cout << "Degenerate transition vector, skipping" << '\n';
        return std::nullopt;
    }

    // A right turn is when the angle between two vectors is greater than pi
    // radians
    if (CGAL::orientation(d1, d2) == CGAL::RIGHT_TURN) {
        std::cout << "Angle between vectors is a right turn, skipping" << '\n';
        return std::nullopt;
    }

    // Compute p1 and p2 and check target membership
    std::optional<Point> p1 = shoot_ray(
        w,
        source.source(),
        Vector{source.source(), target.target()},
        tree);
    std::optional<Point> p2 = shoot_ray(
        w,
        source.target(),
        Vector{source.target(), target.source()},
        tree);
    if (!p1.has_value()
        || !p2.has_value()
        || !target.has_on(*p1)
        || !target.has_on(*p2)) {
        std::cout << "Raycast failed, skipping" << '\n';
        std::cout << "p1 exists: " << p1.has_value() << '\n';
        std::cout << "p2 exists: " << p2.has_value() << '\n';
        std::cout << "p1 on target: "
            << (p1.has_value() ? target.has_on(*p1) : false) << '\n';
        std::cout << "p2 on target: "
            << (p2.has_value() ? target.has_on(*p2) : false) << '\n';
        return std::nullopt;
    }

    // Create a quadrilateral with source.source(), source.target(), p1, and p2
    boost::container::small_vector<Point, 4> quad_points{
        source.source(), source.target(), *p2, *p1};
    Polygon quadrilateral;
    CGAL::convex_hull_2(
        quad_points.begin(),
        quad_points.end(),
        std::back_inserter(quadrilateral));

    // Check if any vertices of w are contained in the quadrilateral
    for (const auto& vertex : w.outer_boundary().vertices()) {
        if (quadrilateral.has_on_bounded_side(vertex)) {
            std::cout
                << "Vertex " << vertex
                << " of outer boundary is contained in quadrilateral, "
                << "skipping" << '\n';
            return std::nullopt;
        }
    }
    for (const auto& hole : w.holes()) {
        for (const auto& vertex : hole.vertices()) {
            if (quadrilateral.has_on_bounded_side(vertex)) {
                std::cout
                    << "Vertex " << vertex
                    << " of hole is contained in quadrilateral, skipping"
                    << '\n';
                return std::nullopt;
            }
        }
    }

    // Return the mid-angle bisector of d1 and d2
    Line d1_line(source.source(), d1);
    Line d2_line(source.target(), d2);
    return CGAL::bisector(d1_line, d2_line).to_vector();
}

Graph construct_graph(
        const HoledPolygon& w,
        const std::vector<std::pair<fscalar, fscalar>>& parameters,
        fscalar theta_max,
        fscalar radius,
        Cache& cache) {
    Graph graph;

    // Add the nodes
    for (const auto& [l, o_max] : parameters) {
        std::cout << "Adding layer with length " << l
                  << " and offset " << o_max << '\n';
        add_layer(w, graph, l, o_max);
    }

    // Add the edges
    // Construct an AABB tree for raycast queries
    std::vector<Segment> segments;
    for (const auto& edge : w.outer_boundary().edges()) {
        segments.push_back(edge);
    }
    for (const auto& hole : w.holes()) {
        for (const auto& edge : hole.edges()) {
            segments.push_back(edge);
        }
    }

    AABBTree tree(segments.begin(), segments.end());
    tree.build();
    
    for (auto& [node, edges] : graph) {
        for (const auto& [other_node, _] : graph) {
            auto maybe_edge = has_edge(
                w,
                node.segment(),
                other_node.segment(),
                theta_max,
                tree);

            if (!maybe_edge.has_value()) continue;

            std::cout
                << "Adding edge from node "
                << node.ID()
                << " to node "
                << other_node.ID()
                << '\n';
            sqlite3_int64 coverage_id = cache.insert_coverage_area(
                node.segment(),
                other_node.segment(),
                radius
                );
            edges.emplace_back(other_node, maybe_edge.value(), coverage_id);
        }
    }

    return graph;
}
