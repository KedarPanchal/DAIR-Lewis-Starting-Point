#include "graph_construction.hpp"

#include <iostream>
#include <iterator>
#include <optional>
#include <tuple>
#include <vector>
#include <thread>
#include <mutex>
#include <syncstream>

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
#include "utilities.hpp"
#include "cache.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------

// Hash value function for boost::hash_combine
size_t hash_value(const Node& node) {
    return std::hash<size_t>()(node.ID());
}

std::vector<std::ranges::subrange<graph_iter>> graph_views(
        Graph& g, 
        Cache& cache) {
    std::vector<std::ranges::subrange<graph_iter>> views;

    const size_t num_nodes = g.size();
    if (num_nodes == 0) return views;

    const size_t base_views = num_nodes / 10;
    const size_t remainder = num_nodes % 10;

    auto start = g.begin();
    for (size_t i = 0; i < 10 && start != g.end(); ++i) {
        size_t view_size = base_views + (i < remainder ? 1 : 0);
        auto end = start;
        std::advance(end, view_size);
        views.emplace_back(start, end);
        start = end;
    }

    return views;
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
            size_t ceiling = boost::multiprecision::ceil(
                convert<hpscalar>((length - l) / o_max))
                .convert_to<size_t>();
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
    hpscalar theta_max,
    const AABBTree& tree) {
    hpscalar theta_max_radians =
        theta_max * boost::math::constants::pi<hpscalar>() / 180;
    Vector d1 = Vector{source.source(), target.target()}.transform(
        Transformation(
            CGAL::ROTATION,
            convert<fscalar>(
                boost::multiprecision::sin(theta_max_radians)),
            convert<fscalar>(
                boost::multiprecision::cos(theta_max_radians))));
    Vector d2 = Vector{source.target(), target.source()}.transform(
        Transformation(
            CGAL::ROTATION,
            convert<fscalar>(
                boost::multiprecision::sin(-theta_max_radians)),
            convert<fscalar>(
                boost::multiprecision::cos(-theta_max_radians))));

    // Avoid division by zero, also, if this occurs, then the vector is
    // degenerate anyway
    if (d1.squared_length() == 0 || d2.squared_length() == 0) {
        std::osyncstream(std::cout)
            << "Degenerate transition vector, skipping" << '\n';
        return std::nullopt;
    }

    // A right turn is when the angle between two vectors is greater than pi
    // radians
    if (CGAL::orientation(d1, d2) == CGAL::RIGHT_TURN) {
        std::osyncstream(std::cout)
            << "Angle between vectors is a right turn, skipping" << '\n';
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
        std::osyncstream out(std::cout);
        out << "Raycast failed, skipping" << '\n';
        out << "p1 exists: " << p1.has_value() << '\n';
        out << "p2 exists: " << p2.has_value() << '\n';
        out << "p1 on target: "
            << (p1.has_value() ? target.has_on(*p1) : false) << '\n';
        out << "p2 on target: "
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
            std::osyncstream(std::cout)
                << "Vertex " << vertex
                << " of outer boundary is contained in quadrilateral, "
                << "skipping" << '\n';
            return std::nullopt;
        }
    }
    for (const auto& hole : w.holes()) {
        for (const auto& vertex : hole.vertices()) {
            if (quadrilateral.has_on_bounded_side(vertex)) {
                std::osyncstream(std::cout)
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

void process_view(
        const HoledPolygon& w, 
        std::ranges::subrange<graph_iter> graph,
        const fscalar& radius,
        const hpscalar& theta_max_hp,
        const AABBTree& tree,
        Cache& cache,
        std::mutex& graph_mutex
        ) {
    for (auto& [node, edges] : graph) {
        for (auto& [other_node, _] : graph) {
            auto maybe_edge = has_edge(
                w,
                node.segment(),
                other_node.segment(),
                theta_max_hp,
                tree);

            if (!maybe_edge.has_value()) continue;

            std::lock_guard<std::mutex> lock(graph_mutex);
            std::osyncstream(std::cout)
                << "Adding edge from node "
                << node.ID()
                << " to node "
                << other_node.ID()
                << '\n';
            sqlite3_int64 coverage_id = cache.insert_coverage_area(
                node.segment(),
                other_node.segment(),
                radius);
            edges.emplace_back(other_node, maybe_edge.value(), coverage_id);
        }
    }
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
    
    // Multithreaded processing of graph views
    hpscalar theta_max_hp = convert<hpscalar>(theta_max);
    auto views = graph_views(graph, cache);
    std::mutex graph_mutex;
    {
        std::vector<std::jthread> workers;
        workers.reserve(10);
        for (size_t i = 0; i < 10; ++i) {
            workers.emplace_back(
                process_view,
                std::cref(w),
                views[i],
                std::cref(radius),
                std::cref(theta_max_hp),
                std::cref(tree),
                std::ref(cache),
                std::ref(graph_mutex));
        }
    }

    return graph;
}
