#include "graph_construction.hpp"

#include <vector>
#include <list>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <tuple>

#include <CGAL/squared_distance_2.h>
#include <CGAL/Circle_2.h>

#include <boost/container/small_vector.hpp>

#include "cgal_types.hpp"

// -- HELPER FUNCTIONS --------------------------------------------------------

// Hash value function for boost::hash_combine
size_t hash_value(const Node& node) {
    return std::hash<size_t>()(node.ID());
}

// Sort points in counterclockwise order
template <typename Container>
void sort_counterclockwise(Container& points) {
    Point sum = std::accumulate(points.begin(), points.end(), Point(0, 0), [](const Point& acc, const Point& point) {
        return Point(acc.x() + point.x(), acc.y() + point.y());
    });
    Point centroid = Point(sum.x() / points.size(), sum.y() / points.size());

    std::sort(points.begin(), points.end(), [&centroid](const Point& a, const Point& b) {
        hpscalar angle_a = boost::multiprecision::atan2(convert<hpscalar>(a.y() - centroid.y()), convert<hpscalar>(a.x() - centroid.x()));
        hpscalar angle_b = boost::multiprecision::atan2(convert<hpscalar>(b.y() - centroid.y()), convert<hpscalar>(b.x() - centroid.x()));
        return angle_a < angle_b;
    });
}

// Helper functions for computing CCR
CurvedPolygon buildCircle(const Point& center, const fscalar& radius) {
    CGAL::Circle_2<Kernel> circle(center, radius * radius);
    boost::container::small_vector<CurvedTraits::X_monotone_curve_2, 2> semicircles{
        CurvedTraits::X_monotone_curve_2(circle, CurvedTraits::Point_2(center.x() - radius, center.y()), CurvedTraits::Point_2(center.x() + radius, center.y()), CGAL::COUNTERCLOCKWISE),
        CurvedTraits::X_monotone_curve_2(circle, CurvedTraits::Point_2(center.x() + radius, center.y()), CurvedTraits::Point_2(center.x() - radius, center.y()), CGAL::COUNTERCLOCKWISE),
    };

    return CurvedPolygon(semicircles.begin(), semicircles.end());
}

PolygonSet buildStadium(const Point& source, const Point& target, const fscalar& radius) {
    // The circle centers are radius distance along the vector from source to target, in both directions
    Vector direction = Vector{source, target};
    direction = direction / CGAL::sqrt(direction.squared_length()); // Normalize
    Point source_center = source + direction * radius;
    Point target_center = target - direction * radius;
    
    // Add the circles to the polygon set
    PolygonSet stadium{buildCircle(source_center, radius)};
    stadium.join(buildCircle(target_center, radius));

    // Construct a rectangle between the two circles and add it to the polygon set
    boost::container::small_vector<Point, 4> rectangle_points{
        source_center + direction.perpendicular(CGAL::CLOCKWISE) * radius,
        source_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE) * radius,
        target_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE) * radius,
        target_center + direction.perpendicular(CGAL::CLOCKWISE) * radius,
    };
    sort_counterclockwise(rectangle_points);
    // Using the points, construct them into edges pairwise
    boost::container::small_vector<CurvedTraits::X_monotone_curve_2, 4> rectangle_edges;
    for (size_t i = 0; i < rectangle_points.size(); ++i) {
        size_t next = (i + 1) % rectangle_points.size();
        rectangle_edges.emplace_back(rectangle_points[i], rectangle_points[next]);
    }
    CurvedPolygon rectangle(rectangle_edges.begin(), rectangle_edges.end());
    stadium.join(rectangle);

    return stadium;
}

PolygonSet computeCoverage(const Segment& source, const Segment& target, const Vector& direction, const fscalar& radius) {
    auto stadium1 = buildStadium(source.source(), target.target(), radius);
    auto stadium2 = buildStadium(source.target(), target.source(), radius);
    stadium1.join(stadium2);

    return stadium1;
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
    // and sort it according to angle from the point to the origin.
    boost::container::small_vector<Point, 4> quad{source.source(), source.target(), *p1, *p2};
    sort_counterclockwise(quad);
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
            if (maybe_edge.has_value()) edges.emplace_back(other_node, maybe_edge.value(), computeCoverage(node.segment(), other_node.segment(), maybe_edge.value(), radius));
        }
    }

    return graph;
}
