#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <type_traits>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

// -- UTILITY FUNCTIONS -------------------------------------------------------

std::variant<HoledPolygon, std::string> read_polygon(std::istream& in);

std::vector<std::pair<fscalar, fscalar>> read_wallpapering_parameters(
    std::istream& in);

std::variant<std::pair<fscalar, fscalar>, std::string> read_robot_parameters(
    std::istream& in);

// Helper function for converting numeric types
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

std::ostream& operator<<(std::ostream& os, const Graph& g);
#endif
