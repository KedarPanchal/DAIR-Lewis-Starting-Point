#include "constructions.hpp"

#include <CGAL/Kernel/global_functions_2.h>
#include <boost/container/small_vector.hpp>

// Code to build the edges of a stadium polygon
boost::container::small_vector<Curve, 4> build_standard_stadium(
    const EPoint& source_center,
    const EPoint& target_center,
    const fscalar& radius,
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
    const fscalar& radius) {
    CGAL::Circle_2<EKernel> circle(
        center, radius * radius, CGAL::COUNTERCLOCKWISE);
    return {Curve(circle)};
}

BoundaryCheckResult check_boundary(const EPoint& point, const EHoledPolygon& wall_space) {
    if (wall_space.outer_boundary().has_on_boundary(point)) {
        return BoundaryCheckResult::OUTER_BOUNDARY;
    }
    for (const auto& hole : wall_space.holes()) {
        if (hole.has_on_boundary(point)) {
            return BoundaryCheckResult::HOLE_BOUNDARY;
        }
    }
    return BoundaryCheckResult::BOUNDED;
}

void adjust_stadium_center(EPoint& center, const EVector& direction, const EHoledPolygon& wall_space) {
    BoundaryCheckResult boundary_check = check_boundary(center, wall_space);
    if (boundary_check == BoundaryCheckResult::OUTER_BOUNDARY) {
        center = center + direction.perpendicular(CGAL::COUNTERCLOCKWISE);
    } else if (boundary_check == BoundaryCheckResult::HOLE_BOUNDARY) {
        center = center + direction.perpendicular(CGAL::CLOCKWISE);
    }
}

CurvedPolygon build_stadium(
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
    EVector direction = EVector{e_source, e_target};
    direction = direction / CGAL::sqrt(direction.squared_length()) * radius;
    // TODO: These computations don't taken into account moving along an edge of the wall
    // This could lead to clipping of the stadium as the center is still placed along the edge of the wall
    // This can be solved by checking center points on the boundary of the wall and holes
    // If it's on the boundary, check if on wall or hole
    // Rotate left on wall, right on hole
    EPoint source_center = e_source + direction;
    EPoint target_center = e_target - direction;
    adjust_stadium_center(source_center, direction, wall_space);
    adjust_stadium_center(target_center, direction, wall_space);

    const fscalar epsilon = 1e-9 * std::max(1.0, radius);
    const fscalar distance_check = CGAL::squared_distance(source, target);
    boost::container::small_vector<Curve, 4> curves;
    if (distance_check < (2 * radius - epsilon) * (2 * radius - epsilon)) {
        std::cout << "Source and target centers are too close, no stadium can be constructed, building empty stadium." << '\n';
        return CurvedPolygon();
    } else if (distance_check < (2 * radius + epsilon) * (2 * radius + epsilon)) {
        std::cout << "Source and target centers are close, building circular stadium." << '\n';
        curves = build_circular_stadium(CGAL::midpoint(source_center, target_center), radius);
    } else {
        std::cout << "Building standard stadium." << '\n';
        curves = build_standard_stadium(
            source_center, target_center, radius, direction);
    }
    CGAL::Arrangement_2<CurvedTraits> arr;
    CGAL::insert(arr, curves.begin(), curves.end());

    // Find first bounded face in the arrangement and extract its boundary
    // edges to construct a CurvedPolygon
    CGAL::Arrangement_2<CurvedTraits>::Face_const_iterator face =
        arr.unbounded_face();
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
    auto stadium1 =
        build_stadium(source.source(), target.target(), radius, wall_space);
    auto stadium2 =
        build_stadium(source.target(), target.source(), radius, wall_space);

    PolygonSet coverage{std::move(stadium1)};
    coverage.intersection(std::move(stadium2));
    return coverage;
}


