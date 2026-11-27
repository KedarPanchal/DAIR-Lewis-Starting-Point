#ifndef CONSTRUCTIONS_HPP
#define CONSTRUCTIONS_HPP

#include <boost/container/small_vector.hpp>

#include "cgal_types.hpp"

enum class BoundaryCheckResult {
    OUTER_BOUNDARY,
    HOLE_BOUNDARY,
    BOUNDED
};

boost::container::small_vector<Curve, 4> build_standard_stadium(
    const EPoint& source_center,
    const EPoint& target_center,
    const escalar& radius,
    const EVector& direction);
boost::container::small_vector<Curve, 4> build_circular_stadium(
    const EPoint& center,
    const escalar& radius);
std::optional<CurvedPolygon> build_stadium(
    const Point& source,
    const Point& target,
    const escalar& radius,
    const EHoledPolygon& wall_space);
BoundaryCheckResult check_boundary(
    const EPoint& point, 
    const EHoledPolygon& wall_space);
void adjust_stadium_center(
    EPoint& center, 
    const EVector& direction, 
    const EHoledPolygon& wall_space);
PolygonSet compute_coverage(
    const Segment& source,
    const Segment& target,
    const fscalar& radius,
    const EHoledPolygon& wall_space);

#endif
