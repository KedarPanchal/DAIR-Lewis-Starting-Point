#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <iostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

// -- UTILITY FUNCTIONS -------------------------------------------------------

std::variant<HoledPolygon, std::string> read_polygon(std::istream& in);

std::vector<std::pair<fscalar, fscalar>> read_wallpapering_parameters(
    std::istream& in);

std::variant<std::pair<fscalar, fscalar>, std::string> read_robot_parameters(
    std::istream& in);

std::ostream& operator<<(std::ostream& os, const Graph& g);
#endif
