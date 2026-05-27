#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <variant>
#include <istream>
#include <list>
#include <string>
#include <utility>

#include "cgal_types.hpp"

// -- UTILITY FUNCTIONS -------------------------------------------------------

std::variant<HoledPolygon, std::string> read_polygon(std::istream& in);

std::list<std::pair<fscalar, fscalar>> read_wallpapering_parameters(std::istream& in);

std::variant<std::pair<fscalar, fscalar>, std::string> read_robot_parameters(std::istream& in);

#endif
