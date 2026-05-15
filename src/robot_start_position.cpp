#include <iostream>
#include <string>
#include <variant>

#include <BURST/geometry.hpp>
#include <BURST/numeric.hpp>

#include "utilities.hpp"


/* PLANNING COMMENT (DELETE ONCE DONE)
 * Things to read from standard input:
 * - The wall space polygon, which can have some or no holes (done, just make some test files for this)
 * - The parameters for the wallpapering of the wall space, which are, for each layer, pairs of:
 *   - The length of the segment partitions of each edge (l)
 *   - The maximum offset between partition start points of adjacent layers (o_max)
 *
 * Input file format:
 * Line 1: The wall space polygon, however CGAL likes that formatted
 * Line 2: The number of layers (n) followed by pairs of l and o_max for each layer with everything separated by spaces
 * Line 3: The maximum rotational error for the robot (theta_max)
 */

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
    auto maybe_wall_space = read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space)) {
        std::cerr << "Error: Invalid wall space polygon: " << std::get<std::string>(maybe_wall_space) << std::endl;
        return 1;
    }
    auto wall_space = std::get<BURST::geometry::HoledPolygon2D>(maybe_wall_space);
    auto parameters = read_wallpapering_parameters(std::cin);
    if (parameters.empty()) {
        std::cerr << "Error: Invalid wallpapering parameters or none provided" << std::endl;
        return 1;
    }
}

