#include <iostream>
#include <string>
#include <variant>
#include <BURST/geometry.hpp>

// Reads a polygon from the input stream
std::variant<BURST::geometry::Polygon2D, std::string> read_polygon(std::istream& in) {
    // Extract polygon as a string
    std::string string_repr;
    std::getline(in, string_repr);
    std::istringstream is(string_repr);
    // Attempt to parse the polygon
    BURST::geometry::Polygon2D polygon;
    // If successfully parsed, return the polygon; otherwise, return the original string
    if (is >> polygon) return polygon;
    else return string_repr;
}


/** PLANNING COMMENT (DELETE ONCE DONE)
 * Things to read from standard input:
 * - The wall space polygon, which can have some or no holes (done, just make some test files for this)
 * - The parameters for the wallpapering of the wall space, which are, for each layer, pairs of:
 *   - The length of the segment partitions of each edge (l)
 *   - The maximum offset between partition start points of adjacent layers (o_max)
 *
 * Input file format:
 * Line 1: The wall space polygon, however CGAL likes that formatted
 * Line 2: The number of layers (n) followed by pairs of l and o_max for each layer with everything separated by spaces
 */

int main() {
    // Read wall space from standard input
    auto maybe_wall_space = read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space)) {
        std::cerr << "Error: Invalid wall space polygon: " << std::get<std::string>(maybe_wall_space) << std::endl;
        return 1;
    }
    auto wall_space = std::get<BURST::geometry::Polygon2D>(maybe_wall_space);
}

