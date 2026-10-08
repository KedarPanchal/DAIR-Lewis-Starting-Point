#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <variant>
#include <tuple>
#include <list>
#include <string>
#include <istream>
#include <list>
#include <string>
#include <utility>
#include <ostream>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

// -- UTILITY FUNCTIONS -------------------------------------------------------

std::variant<HoledPolygon, std::string> read_polygon(std::istream& in);

std::list<std::pair<fscalar, fscalar>> read_wallpapering_parameters(std::istream& in);

std::variant<std::tuple<size_t, fscalar, fscalar>, std::string> read_robot_parameters(std::istream& in);

// Helper function for converting numeric types
template <typename To, typename From>
To convert(const From& x) {
    std::ostringstream str_representation;
    str_representation << std::setprecision(HP_PRECISION) << x;
    std::istringstream is(str_representation.str());
    return To(is.str());
}

std::ostream& operator<<(std::ostream& os, const Graph& g);

#endif
