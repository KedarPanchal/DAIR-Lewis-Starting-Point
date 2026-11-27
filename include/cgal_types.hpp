#ifndef CGAL_TYPES_HPP
#define CGAL_TYPES_HPP

#include <iomanip>
#include <sstream>
#include <vector>

#include <CGAL/Exact_predicates_exact_constructions_kernel_with_sqrt.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Gps_segment_traits_2.h>
#include <CGAL/Gps_circle_segment_traits_2.h>

#include <CGAL/Point_2.h>
#include <CGAL/Segment_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/General_polygon_set_2.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits_2.h>
#include <CGAL/AABB_segment_primitive_2.h>

#include <CGAL/Aff_transformation_2.h>

#include <boost/multiprecision/mpfr.hpp>

// Geometric types
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using EKernel = CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt;
using LinearTraits = CGAL::Gps_segment_traits_2<Kernel>;
using ELinearTraits = CGAL::Gps_segment_traits_2<EKernel>;
using CurvedTraits = CGAL::Gps_circle_segment_traits_2<EKernel>;

using Point = CGAL::Point_2<Kernel>;
using Segment = CGAL::Segment_2<Kernel>;
using Ray = CGAL::Ray_2<Kernel>;
using Line = CGAL::Line_2<Kernel>;
using Vector = CGAL::Vector_2<Kernel>;
using Polygon = CGAL::Polygon_2<LinearTraits>;
using HoledPolygon = CGAL::Polygon_with_holes_2<LinearTraits>;
using Transformation = CGAL::Aff_transformation_2<Kernel>;

// Exact types
using EPoint = CGAL::Point_2<EKernel>;
using ESegment = CGAL::Segment_2<EKernel>;
using EVector = CGAL::Vector_2<EKernel>;
using Curve = CurvedTraits::Curve_2;
using EPolygon = CGAL::Polygon_2<ELinearTraits>;
using CurvedPolygon = CurvedTraits::General_polygon_2;
using EHoledPolygon = CGAL::Polygon_with_holes_2<ELinearTraits>;

// Complex types
using PolygonSet = CGAL::General_polygon_set_2<CurvedTraits>;
using AABBTree = CGAL::AABB_tree<CGAL::AABB_traits_2<Kernel, CGAL::AABB_segment_primitive_2<Kernel, std::vector<Segment>::const_iterator>>>;

// Numeric types
constexpr unsigned int HP_PRECISION = 1000;
using fscalar = Kernel::FT;
using escalar = EKernel::FT;
const fscalar EPSILON = fscalar(1e-9);
const fscalar EPSILON_SQUARED = EPSILON * EPSILON;
const escalar EXACT_EPSILON = escalar(1e-9);
const escalar EXACT_EPSILON_SQUARED = EXACT_EPSILON * EXACT_EPSILON;

// Helper function for converting types
template <typename To, typename From>
To convert(const From& x) {
    std::ostringstream str_representation;
    str_representation << std::setprecision(HP_PRECISION) << x;

    To result;
    if constexpr (std::is_same_v<To, escalar>) {
        result = escalar(str_representation.str());
    } else {
        std::istringstream is(str_representation.str());
        is >> std::setprecision(HP_PRECISION) >> result;
    }
    return result;
}

template <typename To, typename From>
To convert_point(const From& p) {
    return To(p.x(), p.y());
}

template <typename To, typename From>
To convert_polygon(const From& p) {
    std::vector<typename To::Point_2> points;
    for (const auto& point : p.container()) {
        points.push_back(convert_point<typename To::Point_2>(point));
    }
    return To(points.begin(), points.end());
}

template <typename To, typename From>
To convert_polygon_with_holes(const From& p) {
    typename To::Polygon_2 boundary = convert_polygon<typename To::Polygon_2>(p.outer_boundary());
    std::vector<typename To::Polygon_2> holes;
    for (const auto& hole : p.holes()) {
        typename To::Polygon_2 hole_poly = convert_polygon<typename To::Polygon_2>(hole);
        hole_poly.reverse_orientation();
        holes.push_back(hole_poly);
    }
    To result(boundary, holes.begin(), holes.end());
    return result;
}


#endif
