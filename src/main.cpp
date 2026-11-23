#include <iostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cgal_types.hpp"
#include "utilities.hpp"
#include "cache.hpp"
#include "graph_construction.hpp"
#include "coverage_segmentation.hpp"
#include "visualization.hpp"

int main() {
    // Read wall space from standard input
    std::variant<HoledPolygon, std::string> maybe_wall_space =
        read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space)) {
        std::cerr
            << "Error: Invalid wall space polygon: "
            << std::get<std::string>(maybe_wall_space)
            << '\n';
        return 1;
    }
    HoledPolygon wall_space = std::get<HoledPolygon>(maybe_wall_space);

    // Read parameters from standard input
    std::vector<std::pair<fscalar, fscalar>> parameters =
        read_wallpapering_parameters(std::cin);
    if (parameters.empty()) {
        std::cerr
            << "Error: Invalid wallpapering parameters or none provided"
            << '\n';
        return 1;
    }

    // Read theta_max and robot radius from standard input
    std::variant<std::pair<fscalar, fscalar>, std::string>
        maybe_robot_parameters = read_robot_parameters(std::cin);
    if (std::holds_alternative<std::string>(maybe_robot_parameters)) {
        std::cerr
            << "Error: Invalid robot parameters: "
            << std::get<std::string>(maybe_robot_parameters)
            << '\n';
        return 1;
    }
    fscalar theta_max =
        std::get<std::pair<fscalar, fscalar>>(maybe_robot_parameters).first;
    fscalar radius =
        std::get<std::pair<fscalar, fscalar>>(maybe_robot_parameters).second;

    // Create the cache for graph edges
    Cache cache("graph_cache.db");

    // Construct the graph
    Graph graph =
        construct_graph(wall_space, parameters, theta_max, radius, cache);

    // Find best starting position(s)
    std::vector<SCC> areas = scc_areas(graph, cache);
    visualize(wall_space, areas);
    return 0;
}
