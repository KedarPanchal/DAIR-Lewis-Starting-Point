#include <iostream>
#include <string>
#include <variant>

#include <BURST/geometry.hpp>
#include <BURST/numeric.hpp>
#include <BURST/wall_space.hpp>

#include "utilities.hpp"
#include "wallpapering.hpp"

/* POLYGON INPUT FORMAT
 * Non-holed polygon: <number of vertices> <x1> <y1> <x2> <y2> ... <xn> <yn>  0
 *   - e.g. 4 0 0 10 0 10 10 0 10  0
 *   - Note the double space between the vertices and the hole count (which is 0 for non-holed polygons)
 * Holed polygon: <number of vertices in boundary> <x1> <y1> ... <xn> <yn>  <number of holes> <number of vertices in hole 1> <x1> <y1> ... <xm> <ym>  <number of vertices in hole 2> <x1> <y1> ... <xp> <yp> ...
 *  - e.g. 4 0 0 10 0 10 10 0 10  2 4 3 3 3 7 7 7 7 3  4 1 1 1 2 2 2 2 1
 *  - Note the double spaces between the boundary and the holes and between the holes themselves
 */

int main() {
    // Read wall space from standard input
    auto maybe_wall_space_polygon = read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space_polygon)) {
        std::cerr << "Error: Invalid wall space polygon: " << std::get<std::string>(maybe_wall_space_polygon) << std::endl;
        return 1;
    }
    auto wall_space_polygon = std::get<BURST::geometry::HoledPolygon2D>(maybe_wall_space_polygon);
    auto maybe_wall_space = BURST::geometry::WallSpace::create(wall_space_polygon);
    if (!maybe_wall_space) {
        std::cerr << "Error: Failed to create wall space from degenerate polygon" << std::endl;
        return 1;
    }
    BURST::geometry::WallSpace wall_space = *maybe_wall_space;
    
    // Read parameters from standard input
    auto parameters = read_wallpapering_parameters(std::cin);
    if (parameters.empty()) {
        std::cerr << "Error: Invalid wallpapering parameters or none provided" << std::endl;
        return 1;
    }
}

