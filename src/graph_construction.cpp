#include "graph_construction.hpp"

#include <vector>
#include <list>
#include <unordered_map>
#include <optional>
#include <iterator>
#include <tuple>

#include <CGAL/squared_distance_2.h>
#include <CGAL/Circle_2.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/Kernel/global_functions.h>
#include <CGAL/enum.h>
#include <CGAL/squared_distance_2.h>

#include <boost/container/small_vector.hpp>
#include <boost/multiprecision/mpfr.hpp>
#include <boost/math/constants/constants.hpp>

#include "cgal_types.hpp"
#include "utilities.hpp"

// -- GLOBAL CONSTANTS --------------------------------------------------------
const fscalar FSCALAR_EPSILON = convert<fscalar>(EPSILON);

// -- HELPER FUNCTIONS --------------------------------------------------------

// Hash value function for boost::hash_combine
size_t hash_value(const Node& node) {
    return std::hash<size_t>()(node.ID());
}

// Code to build the edges of a stadium polygon
boost::container::small_vector<Curve, 4> build_standard_stadium(Point source_center, Point target_center, fscalar radius_squared, const Vector& direction) {
    CGAL::Circle_2<Kernel> source_circle(source_center, radius_squared, CGAL::COUNTERCLOCKWISE);
    CGAL::Circle_2<Kernel> target_circle(target_center, radius_squared, CGAL::COUNTERCLOCKWISE);

    // Construct the 4 points needed to construct the rectangle
    Point source1 = source_center + direction.perpendicular(CGAL::CLOCKWISE);
    Point source2 = source_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE);
    Point target1 = target_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE);
    Point target2 = target_center + direction.perpendicular(CGAL::CLOCKWISE);
    std::cout << "Stadium points: " << source1 << ", " << source2 << ", " << target1 << ", " << target2 << std::endl;
    // Place them all into a vector and sort them in counterclockwise order
    boost::container::small_vector<Point, 4> rectangle_points{source1, source2, target1, target2};
    boost::container::small_vector<Point, 4> sorted_rectangle_points;
    CGAL::convex_hull_2(rectangle_points.begin(), rectangle_points.end(), std::back_inserter(sorted_rectangle_points));

    // If both points are a source or target, construct an arc in counterclockwise order, otherwise construct a line segment
    boost::container::small_vector<Curve, 4> curves;
    for (size_t i = 0; i < 4; ++i) {
        size_t next = (i + 1)  % 4;
        if (
            (sorted_rectangle_points[i] == source1 || sorted_rectangle_points[i] == source2) &&
            (sorted_rectangle_points[next] == source1 || sorted_rectangle_points[next] == source2)
            ) {
            CurvedTraits::Point_2 source_point(sorted_rectangle_points[i].x(), sorted_rectangle_points[i].y());
            CurvedTraits::Point_2 target_point(sorted_rectangle_points[next].x(), sorted_rectangle_points[next].y());
            curves.emplace_back(source_circle, source_point, target_point);
        } else if (
            (sorted_rectangle_points[i] == target1 || sorted_rectangle_points[i] == target2) &&
            (sorted_rectangle_points[next] == target1 || sorted_rectangle_points[next] == target2)
            ) {
            CurvedTraits::Point_2 source_point(sorted_rectangle_points[i].x(), sorted_rectangle_points[i].y());
            CurvedTraits::Point_2 target_point(sorted_rectangle_points[next].x(), sorted_rectangle_points[next].y());
            curves.emplace_back(target_circle, source_point, target_point);
        } else {
            curves.emplace_back(sorted_rectangle_points[i], sorted_rectangle_points[next]);
        }
    }

    return curves;
}

boost::container::small_vector<Curve, 4> build_circular_stadium(Point center, fscalar radius_squared) {
    CGAL::Circle_2<Kernel> circle(center, radius_squared, CGAL::COUNTERCLOCKWISE);
    return {Curve(circle)};
}

CurvedPolygon build_stadium(const Point& source, const Point& target, const fscalar& radius) {
    std::cout << "Source: " << source << ", Target: " << target << ", Radius: " << radius << std::endl;
    // The circle centers are radius distance along the vector from source to target, in both directions
    Vector direction = Vector{source, target};
    // Approximate and remove sqrt requirement from Kernel
    fscalar length = convert<fscalar>(boost::multiprecision::sqrt(convert<hpscalar>(direction.squared_length())));
    direction = direction / length * radius;
    Point source_center = source + direction;
    Point target_center = target - direction;

    // If source and target centers are the same, then the stadium is just a circle
    // Pass direction.squared_length() since the approximation may lead to a precondition violation if radius is passed instead
    boost::container::small_vector<Curve, 4> curves = CGAL::squared_distance(source_center, target_center) < FSCALAR_EPSILON
        ? build_circular_stadium(source_center, direction.squared_length()) 
        : build_standard_stadium(source_center, target_center, direction.squared_length(), direction);
    
    CGAL::Arrangement_2<CurvedTraits> arr;
    CGAL::insert(arr, curves.begin(), curves.end());
    
    // Find first bounded face in the arrangement and extract its boundary edges to construct a CurvedPolygon
    CGAL::Arrangement_2<CurvedTraits>::Face_const_iterator face = arr.unbounded_face();
    for (face = arr.faces_begin(); face != arr.faces_end(); ++face) {
        if (!face->is_unbounded()) break;
    }
    boost::container::small_vector<CurvedTraits::X_monotone_curve_2, 4> stadium_edges;
    CGAL::Arrangement_2<CurvedTraits>::Ccb_halfedge_const_circulator circ = face->outer_ccb();
    CGAL::Arrangement_2<CurvedTraits>::Ccb_halfedge_const_circulator start = circ;
    do {
        stadium_edges.push_back(circ->curve());
    } while (++circ != start);

    return CurvedPolygon(stadium_edges.begin(), stadium_edges.end());
}

PolygonSet compute_coverage(const Segment& source, const Segment& target, const fscalar& radius) {
    auto stadium1 = build_stadium(source.source(), target.target(), radius);
    auto stadium2 = build_stadium(source.target(), target.source(), radius);
    
    PolygonSet coverage{std::move(stadium1)};
    coverage.intersection(std::move(stadium2));
    return coverage;
}

// -- GRAPH CONSTRUCTION ALGORITHM --------------------------------------------

// Adds layers around the boundary of a single polygon
void add_layer_helper(const Polygon& w, Graph& g, fscalar l, fscalar o_max) {
    static size_t node_id = 0;

    for (auto eit = w.edges_begin(); eit != w.edges_end(); ++eit) {
        fscalar length = convert<fscalar>(boost::multiprecision::sqrt(convert<hpscalar>(eit->squared_length())));
        Vector unit_vector = eit->to_vector() / length;
        if (length == l) {
            // Add segment *eit as a node of g
            Node node{node_id++, *eit};
            g[node] = std::list<std::tuple<Node, Vector, PolygonSet>>{};
        } else if (length > l) {
            // Ceiling value since CGAL doesn't expose a ceil function for Kernel::FT
            size_t ceiling = boost::multiprecision::ceil(convert<hpscalar>((length - l) / o_max)).convert_to<size_t>();
            fscalar o = (length - l) / convert<fscalar>(ceiling);
            
            Point x1 = eit->source();
            Point x2 = eit->source();
            Vector direction = unit_vector * l;
            Vector shift = unit_vector * o;
            
            // Ceiling is the number of segments to add, so we need to add ceiling segments
            for(size_t i = 0; i <= ceiling; ++i) {
                // x_2 <- x_1 translated l along *eit
                x2 = x1 + direction;
                // Add segment x1x2 as a node of g
                Node node{node_id++, Segment(x1, x2)};
                g[node] = std::list<std::tuple<Node, Vector, PolygonSet>>{};
                // x_1 <- x_1 translated o along *eit
                x1 += shift;
            }
        }
    }
}

// add_layer algorithm from Lewis's doctoral dissertation (Algorithm 3)
// Well, add_layer_helper is the actual algorithm, but it's applied to every boundary and hole of the polygon
void add_layer(const HoledPolygon& w, Graph& g, fscalar l, fscalar o_max) {
    add_layer_helper(w.outer_boundary(), g, l, o_max);
    for (auto hit = w.holes_begin(); hit != w.holes_end(); ++hit) {
        add_layer_helper(*hit, g, l, o_max);
    }
}

// shoot_ray algorithm for checking safe actions
std::optional<Point> shoot_ray(const Point& source, const Vector& direction, const AABBTree& tree) {
    // Find all intersections of the ray with the polygon boundary
    Ray ray(source, direction);
    auto result = tree.first_intersection(ray, [&](const auto& primitive_id) {
        const Segment& boundary = *primitive_id;
        const auto hit = CGAL::intersection(ray, boundary);
        if (!hit) return false;
        const Point* ipoint = std::get_if<Point>(&(*hit));
        return ipoint != nullptr && CGAL::squared_distance(source, *ipoint) < FSCALAR_EPSILON;
    });

    if (!result.has_value()) return std::nullopt;
    const Point* ipoint = std::get_if<Point>(&(result->first));
    if (ipoint == nullptr) return std::nullopt;
    return *ipoint;
}

// has_edge algorithm from Lewis's doctoral dissertation (Algorithm 4)
std::optional<Vector> has_edge(const HoledPolygon& w, const Segment& source, const Segment& target, hpscalar theta_max, const AABBTree& tree) {
    hpscalar theta_max_radians = theta_max * boost::math::constants::pi<hpscalar>() / 180;
    Vector d1 = Vector{source.source(), target.target()}
        .transform(Transformation(
                    CGAL::ROTATION, 
                    convert<fscalar>(boost::multiprecision::sin(theta_max_radians)), 
                    convert<fscalar>(boost::multiprecision::cos(theta_max_radians))
                    ));
    Vector d2 = Vector{source.target(), target.source()}
        .transform(Transformation(
                    CGAL::ROTATION,
                    convert<fscalar>(boost::multiprecision::sin(-theta_max_radians)),
                    convert<fscalar>(boost::multiprecision::cos(-theta_max_radians))
                    ));
    // Avoid division by zero, also, if this occurs, then the vector is degenerate anyway
    if (d1.squared_length() == 0 || d2.squared_length() == 0) {
        std::cout << "Degenerate vector detected, returning no edge" << std::endl;
        return std::nullopt;
    }

    // A right turn is when the angle between two vectors is greater than pi radians
    if (CGAL::orientation(d1, d2) == CGAL::RIGHT_TURN) {
        std::cout << "Vectors are a right turn, returning no edge" << std::endl;
        return std::nullopt;
    }

    // Compute p1 and p2 and check target membership
    std::optional<Point> p1 = shoot_ray(source.source(), Vector{source.source(), target.target()}, tree);
    std::optional<Point> p2 = shoot_ray(source.target(), Vector{source.target(), target.source()}, tree);
    if (
        !p1.has_value() ||
        !p2.has_value() ||
        !(CGAL::squared_distance(target, *p1) < FSCALAR_EPSILON) ||
        !(CGAL::squared_distance(target, *p2) < FSCALAR_EPSILON)
        ) {
        std::cout << "Raycast doesn't reach target, returning no edge" << std::endl;
        std::cout << "p1 has value: " << p1.has_value() << std::endl;
        std::cout << "p2 has value: " << p2.has_value() << std::endl;
        std::cout << "p1 is on target " << (p1.has_value() ? (CGAL::squared_distance(target, *p1) < FSCALAR_EPSILON) : false) << std::endl;
        std::cout << "p2 is on target " << (p2.has_value() ? (CGAL::squared_distance(target, *p2) < FSCALAR_EPSILON) : false) << std::endl;
        return std::nullopt;
    }

    // Create a quadrilateral with source.source(), source.target(), p1, and p2
    boost::container::small_vector<Point, 4> quad_points{source.source(), source.target(), *p2, *p1};
    Polygon quadrilateral;
    CGAL::convex_hull_2(quad_points.begin(), quad_points.end(), std::back_inserter(quadrilateral));
    
    // Check if any vertices of w are contained in the quadrilateral
    for (const auto& vertex : w.outer_boundary().vertices()) {
        if (quadrilateral.has_on_bounded_side(vertex)) {
            std::cout << "Vertex of polygon is contained in quadrilateral, returning no edge" << std::endl;
            return std::nullopt;
        }
    }
    for (const auto& hole : w.holes()) {
        for (const auto& vertex : hole.vertices()) {
            if (quadrilateral.has_on_bounded_side(vertex)) {
                std::cout << "Vertex of hole is contained in quadrilateral, returning no edge" << std::endl;
                return std::nullopt;
            }
        }
    }

    // Return the mid-angle bisector of d1 and d2
    Line d1_line(source.source(), d1);
    Line d2_line(source.target(), d2);
    return CGAL::bisector(d1_line, d2_line).to_vector();
}

Graph construct_graph(const HoledPolygon& w, const std::list<std::pair<fscalar, fscalar>>& parameters, fscalar theta_max, fscalar radius) {
    Graph graph;
    // Add the nodes
    for (const auto& [l, o_max] : parameters) {
        std::cout << "Adding layer with length " << l << " and offset " << o_max << std::endl;
        add_layer(w, graph, l, o_max);
    }
    std::cout << "Added " << graph.size() << " nodes to the graph" << std::endl;
    // Add the edges
    // Construct an AABB tree for raycast queries
    std::cout << "Constructing AABB tree for raycast queries" << std::endl;
    std::vector<Segment> segments;
    for (const auto& edge : w.outer_boundary().edges()) segments.push_back(edge);
    for (const auto hole : w.holes()) {
        for (const auto& edge : hole.edges()) segments.push_back(edge);
    }

    AABBTree tree(segments.begin(), segments.end());
    tree.build();
    hpscalar theta_max_hp = convert<hpscalar>(theta_max);
    for (auto& [node, edges] : graph) {
        for (auto& [other_node, _] : graph) {
            std::cout << "Checking edge from node " << node.ID() << " to node " << other_node.ID() << std::endl;
            auto maybe_edge = has_edge(w, node.segment(), other_node.segment(), theta_max_hp, tree);
            if (maybe_edge.has_value()) {
                std::cout << "Adding edge from node " << node.ID() << " to node " << other_node.ID() << std::endl;
                edges.emplace_back(other_node, maybe_edge.value(), compute_coverage(node.segment(), other_node.segment(), radius));
            }
        }
    }

    return graph;
}
