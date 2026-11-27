#ifndef CONSTRUCTIONS_HPP
#define CONSTRUCTIONS_HPP

#include <boost/container/small_vector.hpp>

#include "cgal_types.hpp"

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
    const Segment& source_segment,
    const Segment& target_segment,
    const escalar& radius,
    const EHoledPolygon& wall_space);
PolygonSet compute_coverage(
    const Segment& source,
    const Segment& target,
    const fscalar& radius,
    const EHoledPolygon& wall_space);

#endif
