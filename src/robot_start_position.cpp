#include <iostream>
#include <string>
#include <variant>

#include "cgal_types.hpp"
#include "utilities.hpp"
#include "graph_construction.hpp"
#include "graph_algorithms.hpp"

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
    std::variant<HoledPolygon, std::string> maybe_wall_space = read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space)) {
        std::cerr << "Error: Invalid wall space polygon: " << std::get<std::string>(maybe_wall_space) << std::endl;
        return 1;
    }
    auto wall_space = std::get<HoledPolygon>(maybe_wall_space);
    
    // Read parameters from standard input
    std::list<std::pair<fscalar, fscalar>> parameters = read_wallpapering_parameters(std::cin);
    if (parameters.empty()) {
        std::cerr << "Error: Invalid wallpapering parameters or none provided" << std::endl;
        return 1;
    }

    // Read theta_max from standard input
    std::variant<std::pair<fscalar, fscalar>, std::string> maybe_robot_parameters = read_robot_parameters(std::cin);
    if (std::holds_alternative<std::string>(maybe_robot_parameters)) {
        std::cerr << "Error: Invalid robot parameters: " << std::get<std::string>(maybe_robot_parameters) << std::endl;
        return 1;
    }
    auto theta_max = std::get<std::pair<fscalar, fscalar>>(maybe_robot_parameters).first;
    auto radius = std::get<std::pair<fscalar, fscalar>>(maybe_robot_parameters).second;
    
    // Construct the graph
    Graph graph = construct_graph(wall_space, parameters, theta_max, radius);
    
    // Validate algorithm correctness
    Node brute_force_starting_position = brute_force_best_starting_point(graph);
    Node johnson_starting_position = johnson_best_starting_point(graph);
    std::cout << "Brute force starting position: " << brute_force_starting_position.ID() << std::endl;
    std::cout << "Johnson's algorithm starting position: " << johnson_starting_position.ID() << std::endl;
    if (brute_force_starting_position != johnson_starting_position) {
        std::cerr << "Error: Starting positions do not match!" << std::endl;
        return 1;
    } 
    std::cout << "Starting positions match!" << std::endl;
    return 0;
}

