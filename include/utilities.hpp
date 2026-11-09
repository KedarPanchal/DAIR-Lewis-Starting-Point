#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <variant>
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

std::variant<std::pair<fscalar, fscalar>, std::string> read_robot_parameters(std::istream& in);

// Helper function for converting numeric types
template <typename To, typename From>
To convert(const From& x) {
    std::stringstream str_representation;
    str_representation << std::setprecision(HP_PRECISION) << x;

    To result;
    str_representation >> std::setprecision(HP_PRECISION) >> result;
    return result;
}

std::ostream& operator<<(std::ostream& os, const Graph& g);

#endif
