#ifndef CGAL_TYPES_HPP
#define CGAL_TYPES_HPP

#include <vector>

#include <CGAL/Exact_predicates_exact_constructions_kernel_with_sqrt.h>
#include <CGAL/Gps_segment_traits_2.h>

#include <CGAL/Point_2.h>
#include <CGAL/Segment_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits_2.h>
#include <CGAL/AABB_segment_primitive_2.h>

#include <CGAL/Aff_transformation_2.h>

#include <boost/multiprecision/mpfr.hpp>

// Geometric types
using Kernel = CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt;
using Traits = CGAL::Gps_segment_traits_2<Kernel>;

using Point = CGAL::Point_2<Kernel>;
using Segment = CGAL::Segment_2<Kernel>;
using Ray = CGAL::Ray_2<Kernel>;
using Vector = CGAL::Vector_2<Kernel>;
using Polygon = CGAL::Polygon_2<Traits>;
using HoledPolygon = CGAL::Polygon_with_holes_2<Traits>;
using Transformation = CGAL::Aff_transformation_2<Kernel>;

// Complex types
using AABBTree = CGAL::AABB_tree<CGAL::AABB_traits_2<Kernel, CGAL::AABB_segment_primitive_2<Kernel, std::vector<Segment>::const_iterator>>>;

// Numeric types
constexpr unsigned int HP_PRECISION = 1000;
using fscalar = Kernel::FT;
using hpscalar = boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<HP_PRECISION>>; 

#endif
