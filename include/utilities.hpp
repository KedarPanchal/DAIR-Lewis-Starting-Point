#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <variant>
#include <list>
#include <string>
#include <utility>

#include "cgal_types.hpp"

// -- UTILITY FUNCTIONS -------------------------------------------------------

std::variant<HoledPolygon, std::string> read_polygon(std::istream& in);

std::list<std::pair<fscalar, fscalar>> read_wallpapering_parameters(std::istream& in);

std::variant<fscalar, std::string> read_theta_max(std::istream& in);

#endif
