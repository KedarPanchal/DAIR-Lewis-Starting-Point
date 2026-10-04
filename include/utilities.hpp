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

#include "cgal_types.hpp"

// -- UTILITY FUNCTIONS -------------------------------------------------------

std::variant<HoledPolygon, std::string> read_polygon(std::istream& in);

std::list<std::pair<fscalar, fscalar>> read_wallpapering_parameters(std::istream& in);

std::variant<std::tuple<size_t, fscalar, fscalar>, std::string> read_robot_parameters(std::istream& in);

template <typename To, typename From>
To convert(const From& x);

#endif
