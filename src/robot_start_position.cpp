#include <iostream>
#include <string>
#include <variant>
#include <BURST/geometry.hpp>

// Reads a polygon from the input stream
std::variant<BURST::geometry::Polygon2D, std::string> read_polygon(std::istream& in) {
    // Extract polygon as a string
    std::string string_repr;
    in >> string_repr;
    std::istringstream is(string_repr);
    // Attempt to parse the polygon
    BURST::geometry::Polygon2D polygon;
    // If successfully parsed, return the polygon; otherwise, return the original string
    if (is >> polygon) return polygon;
    else return string_repr;
}

int main() {
    // Read wall space from standard input
    auto maybe_wall_space = read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space)) {
        std::cerr << "Error: Invalid wall space polygon: " << std::get<std::string>(maybe_wall_space) << std::endl;
        return 1;
    }
    auto wall_space = std::get<BURST::geometry::Polygon2D>(maybe_wall_space);
}

