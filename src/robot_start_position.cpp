#include <iostream>
#include <string>
#include <variant>

#include <BURST/geometry.hpp>
#include <BURST/numeric.hpp>

#include "utilities.hpp"


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
 * Line 3: The maximum rotational error for the robot (theta_max)
 */

int main() {
    // Maintain an error flag so that we can report all errors at once instead of just the first one encountered
    bool error_flag = false;

    // Read wall space from standard input
    auto maybe_wall_space = read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space)) {
        DEFER({ std::cerr << "Error: Invalid wall space polygon: " << std::get<std::string>(maybe_wall_space) << std::endl; });
        error_flag = true;
    }
    auto wall_space = std::get<BURST::geometry::Polygon2D>(maybe_wall_space);
    auto parameters = read_wallpapering_parameters(std::cin);
    if (parameters.empty()) {
        DEFER({ std::cerr << "Error: Invalid wallpapering parameters or none provided" << std::endl; });
        error_flag = true;
    }
    
    // Return, casuing all deferred error messages to be printed if any errors were encountered
    if (error_flag) return 1;
}

