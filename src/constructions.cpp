#include "constructions.hpp"

#include <iomanip>
#include <iostream>
#include <optional>

#include <CGAL/Kernel/global_functions_2.h>
#include <boost/container/small_vector.hpp>

// Code to build the edges of a stadium polygon
boost::container::small_vector<Curve, 4> build_standard_stadium(
    const EPoint& source_center,
    const EPoint& target_center,
    const escalar& radius,
    const EVector& direction) {
    CGAL::Circle_2<EKernel> source_circle(
        source_center, radius * radius, CGAL::COUNTERCLOCKWISE);
    CGAL::Circle_2<EKernel> target_circle(
        target_center, radius * radius, CGAL::COUNTERCLOCKWISE);

    // Construct the 4 points needed to construct the rectangle
    EPoint source1 = source_center + direction.perpendicular(CGAL::CLOCKWISE);
    EPoint source2 = source_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE);
    EPoint target1 = target_center + direction.perpendicular(CGAL::COUNTERCLOCKWISE);
    EPoint target2 = target_center + direction.perpendicular(CGAL::CLOCKWISE);
    std::cout << "Stadium points: " << std::setprecision(17)
        << source1 << ", " << source2 << ", " << target1 << ", " << target2 
        << '\n';

    // If both points are a source or target, construct an arc in
    // counterclockwise order, otherwise construct a line segment
    boost::container::small_vector<Curve, 4> curves;
    curves.emplace_back(target_circle, convert_point<Curve::Point_2>(target2), convert_point<Curve::Point_2>(target1));
    curves.emplace_back(target1, source2);
    curves.emplace_back(source_circle, convert_point<Curve::Point_2>(source2), convert_point<Curve::Point_2>(source1));
    curves.emplace_back(source1, target2);

    return curves;
}

boost::container::small_vector<Curve, 4> build_circular_stadium(
    const EPoint& center,
    const escalar& radius) {
    CGAL::Circle_2<EKernel> circle(
        center, radius * radius, CGAL::COUNTERCLOCKWISE);
    return {Curve(circle)};
}

std::optional<ESegment> containing_edge(const EPoint& point, const EHoledPolygon& wall_space) {
    for (const auto& edge : wall_space.outer_boundary().edges()) {
        if (CGAL::squared_distance(edge, point) < EXACT_EPSILON_SQUARED) {
            return edge;
        }
    }
    for (const auto& hole : wall_space.holes()) {
        for (const auto& edge : hole.edges()) {
            if (CGAL::squared_distance(edge, point) < EXACT_EPSILON_SQUARED) {
                return edge;
            }
        }
    }
    return std::nullopt;
}

EPoint circle_center(const EPoint& source, const EHoledPolygon& wall_space, const escalar& radius) {
    std::optional<ESegment> edge = containing_edge(source, wall_space);
    if (!edge) {
        std::cerr << "Error: Source point " << source.x() << ", " << source.y() << " is not on the boundary of the wall space." << '\n';
        throw std::runtime_error("Source point is not on the boundary of the wall space.");
    }
    EVector direction = edge->to_vector().perpendicular(CGAL::COUNTERCLOCKWISE);
    direction = direction / CGAL::sqrt(direction.squared_length()) * radius;
    return source + direction;
}

std::optional<CurvedPolygon> build_stadium(
    const Point& source,
    const Point& target,
    const fscalar& radius,
    const EHoledPolygon& wall_space) {
    std::cout << "Source: " << source
                            << ", Target: " << target
                            << ", Radius: " << radius << '\n';

    // The circle centers are radius distance along the vector from source
    // to target, in both directions
    EPoint e_source = convert_point<EPoint>(source);
    EPoint e_target = convert_point<EPoint>(target);
    escalar e_radius = convert<escalar>(radius);
    EPoint source_center = circle_center(e_source, wall_space, e_radius);
    EPoint target_center = circle_center(e_target, wall_space, e_radius);

    const fscalar epsilon = 1e-9 * std::max(1.0, radius);
    const fscalar distance_check = CGAL::squared_distance(source, target);
    boost::container::small_vector<Curve, 4> curves;
    if (distance_check < (2 * radius - epsilon) * (2 * radius - epsilon)) {
        std::cout << "Source and target centers are too close, no stadium can be constructed, building empty stadium." << '\n';
        return std::nullopt;
    } else if (distance_check < (2 * radius + epsilon) * (2 * radius + epsilon)) {
        std::cout << "Source and target centers are close, building circular stadium." << '\n';
        curves = build_circular_stadium(CGAL::midpoint(source_center, target_center), e_radius);
    } else {
        std::cout << "Building standard stadium." << '\n';
        EVector direction{source_center, target_center};
        direction = direction / CGAL::sqrt(direction.squared_length()) * e_radius;
        curves = build_standard_stadium(source_center, target_center, e_radius, direction);
    }
    CGAL::Arrangement_2<CurvedTraits> arr;
    CGAL::insert(arr, curves.begin(), curves.end());

    // Find first bounded face in the arrangement and extract its boundary
    // edges to construct a CurvedPolygon
    CGAL::Arrangement_2<CurvedTraits>::Face_const_iterator face = arr.unbounded_face();
    for (face = arr.faces_begin(); face != arr.faces_end(); ++face) {
        if (!face->is_unbounded()) {
            break;
        }
    }
    boost::container::small_vector<CurvedTraits::X_monotone_curve_2, 4>
        stadium_edges;
    CGAL::Arrangement_2<CurvedTraits>::Ccb_halfedge_const_circulator circ =
        face->outer_ccb();
    CGAL::Arrangement_2<CurvedTraits>::Ccb_halfedge_const_circulator start =
        circ;
    do {
        stadium_edges.push_back(circ->curve());
    } while (++circ != start);

    return CurvedPolygon(stadium_edges.begin(), stadium_edges.end());
}

PolygonSet compute_coverage(
    const Segment& source,
    const Segment& target,
    const fscalar& radius,
    const EHoledPolygon& wall_space) {
    auto stadium1 = build_stadium(source.source(), target.target(), radius, wall_space);
    auto stadium2 = build_stadium(source.target(), target.source(), radius, wall_space);

    if (stadium1.has_value() && stadium2.has_value()) {
        std::cout << "Both stadiums constructed successfully." << '\n';
        PolygonSet coverage{std::move(*stadium1)};
        coverage.intersection(std::move(*stadium2));
        return coverage;
    } else if (stadium1.has_value()) {
        std::cout << "Only stadium1 constructed successfully." << '\n';
        return PolygonSet{std::move(*stadium1)};
    } else if (stadium2.has_value()) {
        std::cout << "Only stadium2 constructed successfully." << '\n';
        return PolygonSet{std::move(*stadium2)};
    } else {
        std::cout << "Neither stadium could be constructed." << '\n';
        return PolygonSet();
    }
}


