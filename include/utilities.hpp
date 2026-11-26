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

std::ostream& operator<<(std::ostream& os, const Graph& g);
#endif
