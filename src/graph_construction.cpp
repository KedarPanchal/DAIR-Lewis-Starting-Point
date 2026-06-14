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

#include <boost/container/small_vector.hpp>

#include "cgal_types.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------

// Hash value function for boost::hash_combine
size_t hash_value(const Node& node) {
    return std::hash<size_t>()(node.ID());
}

CurvedPolygon buildStadium(const Point& source, const Point& target, const fscalar& radius) {
    // The circle centers are radius distance along the vector from source to target, in both directions
    Vector direction = Vector{source, target};
    direction = direction / CGAL::sqrt(direction.squared_length()); // Normalize
    Point source_center = source + direction * radius;
    Point target_center = target - direction * radius;
    CGAL::Circle_2<Kernel> source_circle(source_center, radius * radius, CGAL::COUNTERCLOCKWISE);
    CGAL::Circle_2<Kernel> target_circle(target_center, radius * radius, CGAL::COUNTERCLOCKWISE);

    // Construct the 4 points needed to construct the rectangle
    Point source1 = source_center + direction.perpendicular(CGAL::CLOCKWISE) * radius;
    Point source2 = source_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE) * radius;
    Point target1 = target_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE) * radius;
    Point target2 = target_center + direction.perpendicular(CGAL::CLOCKWISE) * radius;

    // Place them all into a vector and sort them in counterclockwise order
    boost::container::small_vector<Point, 4> rectangle_points{source1, source2, target1, target2};
    boost::container::small_vector<Point, 4> sorted_rectangle_points;
    CGAL::convex_hull_2(rectangle_points.begin(), rectangle_points.end(), std::back_inserter(sorted_rectangle_points));

    // If both points are a source or target, construct an arc in counterclockwise order, otherwise construct a line segment
    CGAL::Arrangement_2<CurvedTraits> arr;
    for (size_t i = 0; i < 4; ++i) {
        size_t next = (i + 1)  % 4;
        if (
            (sorted_rectangle_points[i] == source1 || sorted_rectangle_points[i] == source2) &&
            (sorted_rectangle_points[next] == source1 || sorted_rectangle_points[next] == source2)
            ) {
            CurvedTraits::Point_2 source_point(sorted_rectangle_points[i].x(), sorted_rectangle_points[i].y());
            CurvedTraits::Point_2 target_point(sorted_rectangle_points[next].x(), sorted_rectangle_points[next].y());
            CGAL::insert(arr, CurvedTraits::Curve_2(source_circle, source_point, target_point));
        } else if (
            (sorted_rectangle_points[i] == target1 || sorted_rectangle_points[i] == target2) &&
            (sorted_rectangle_points[next] == target1 || sorted_rectangle_points[next] == target2)
            ) {
            CurvedTraits::Point_2 source_point(sorted_rectangle_points[i].x(), sorted_rectangle_points[i].y());
            CurvedTraits::Point_2 target_point(sorted_rectangle_points[next].x(), sorted_rectangle_points[next].y());
            CGAL::insert(arr, CurvedTraits::Curve_2(target_circle, source_point, target_point));
        } else {
            CGAL::insert(arr, CurvedTraits::Curve_2(sorted_rectangle_points[i], sorted_rectangle_points[next]));
        }
    }

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

PolygonSet computeCoverage(const Segment& source, const Segment& target, const fscalar& radius) {
    auto stadium1 = buildStadium(source.source(), target.target(), radius);
    auto stadium2 = buildStadium(source.target(), target.source(), radius);
    
    PolygonSet coverage{std::move(stadium1)};
    coverage.intersection(std::move(stadium2));
    return coverage;
}

// -- GRAPH CONSTRUCTION ALGORITHM --------------------------------------------

// Adds layers around the boundary of a single polygon
void addLayerHelper(const Polygon& w, Graph& g, fscalar l, fscalar o_max) {
    static size_t node_id = 0;

    for (auto eit = w.edges_begin(); eit != w.edges_end(); ++eit) {
        fscalar length = CGAL::sqrt(eit->squared_length());
        if (length == l) {
            // Add segment *eit as a node of g
            Node node{node_id++, *eit};
            g[node] = std::list<std::tuple<Node, Vector, PolygonSet>>{};
        } else if (length > l) {
            // Ceiling value since CGAL doesn't expose a ceil function for Kernel::FT
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
                g[node] = std::list<std::tuple<Node, Vector, PolygonSet>>{};
                // x_1 <- x_1 translated o along *eit
                x1 += direction * o;
            }
        }
    }
}

// AddLayer algorithm from Lewis's doctoral dissertation (Algorithm 3)
// Well, AddLayerHelper is the actual algorithm, but it's applied to every boundary and hole of the polygon
void addLayer(const HoledPolygon& w, Graph& g, fscalar l, fscalar o_max) {
    addLayerHelper(w.outer_boundary(), g, l, o_max);
    for (auto hit = w.holes_begin(); hit != w.holes_end(); ++hit) {
        addLayerHelper(*hit, g, l, o_max);
    }
}

// shootRay algorithm for checking safe actions
std::optional<Point> shootRay(const HoledPolygon& w, const Point& source, const Vector& direction, const AABBTree& tree) {
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
std::optional<Vector> hasEdge(const HoledPolygon& w, const Segment& source, const Segment& target, hpscalar theta_max, const AABBTree& tree) {
    auto d1 = Vector{source.source(), target.target()}
        .transform(Transformation(
                    CGAL::ROTATION, 
                    convert<fscalar>(boost::multiprecision::sin(theta_max)), 
                    convert<fscalar>(boost::multiprecision::cos(theta_max))
                    ));
    auto d2 = Vector{source.target(), target.source()}
        .transform(Transformation(
                    CGAL::ROTATION,
                    convert<fscalar>(boost::multiprecision::sin(-theta_max)),
                    convert<fscalar>(boost::multiprecision::cos(-theta_max))
                    ));
    // Compute dot product-based angle since CGAL doesn't expose a function for computing angles between vectors 
    // Avoid division by zero, also, if this occurs, then the vector is degenerate anyway
    if (d1.squared_length() == 0 || d2.squared_length() == 0) return std::nullopt;
    fscalar cosine = d1 * d2 / (CGAL::sqrt(d1.squared_length()) * CGAL::sqrt(d2.squared_length()));
    hpscalar angle = boost::multiprecision::acos(convert<hpscalar>(cosine));
    if (angle > CGAL_PI) return std::nullopt;

    // Compute p1 and p2 and check target membership
    std::optional<Point> p1 = shootRay(w, source.source(), Vector{source.source(), target.target()}, tree);
    std::optional<Point> p2 = shootRay(w, source.target(), Vector{source.target(), target.source()}, tree);
    if (
        !p1.has_value() ||
        !p2.has_value() ||
        !target.has_on(*p1) ||
        !target.has_on(*p2)
        ) return std::nullopt;

    // Create a quadrilateral with source.source(), source.target(), p1, and p2
    // and sort it according to angle from the centroid
    boost::container::small_vector<Point, 4> quad_points{source.source(), source.target(), *p1, *p2};
    boost::container::small_vector<Point, 4> sorted_quad_points;
    CGAL::convex_hull_2(quad_points.begin(), quad_points.end(), std::back_inserter(sorted_quad_points));
    Polygon quadrilateral(sorted_quad_points.begin(), sorted_quad_points.end());
    
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

Graph construct_graph(const HoledPolygon& w, const std::list<std::pair<fscalar, fscalar>>& parameters, fscalar theta_max, fscalar radius) {
    Graph graph;
    // Add the nodes
    for (const auto& [l, o_max] : parameters) addLayer(w, graph, l, o_max);

    // Add the edges
    // Construct an AABB tree for raycast queries
    std::vector<Segment> segments;
    for (const auto& edge : w.outer_boundary().edges()) segments.push_back(edge);
    for (const auto hole : w.holes()) {
        for (const auto& edge : hole.edges()) segments.push_back(edge);
    }

    AABBTree tree(segments.begin(), segments.end());
    tree.build();

    for (auto& [node, edges] : graph) {
        for (auto& [other_node, _] : graph) {
            auto maybe_edge = hasEdge(w, node.segment(), other_node.segment(), convert<hpscalar>(theta_max), tree);
            if (maybe_edge.has_value()) edges.emplace_back(other_node, maybe_edge.value(), computeCoverage(node.segment(), other_node.segment(), radius));
        }
    }

    return graph;
}
