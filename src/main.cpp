#include <variant>
#include <vector>
#include <list>
#include <utility>
#include <tuple>
#include <iostream>
#include <string>
#include <variant>
#include <functional>

#include "cgal_types.hpp"
#include "utilities.hpp"
#include "graph_construction.hpp"
#include "start_nodes.hpp"

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
    std::variant<std::tuple<size_t, fscalar, fscalar>, std::string> maybe_robot_parameters = read_robot_parameters(std::cin);
    if (std::holds_alternative<std::string>(maybe_robot_parameters)) {
        std::cerr << "Error: Invalid robot parameters: " << std::get<std::string>(maybe_robot_parameters) << std::endl;
        return 1;
    }
    size_t robot_count = std::get<0>(std::get<std::tuple<size_t, fscalar, fscalar>>(maybe_robot_parameters));
    fscalar theta_max = std::get<1>(std::get<std::tuple<size_t, fscalar, fscalar>>(maybe_robot_parameters));
    fscalar radius = std::get<2>(std::get<std::tuple<size_t, fscalar, fscalar>>(maybe_robot_parameters));
    
    // Construct the graph
    Graph graph = construct_graph(wall_space, parameters, theta_max, radius);
    
    // Find best starting position(s)
    std::vector<SCC> areas = scc_areas(graph);
    // Find the top n starting positions
    std::tuple<std::vector<std::reference_wrapper<const Graph>>, PolygonSet, fscalar> top_n_starts = top_n_sccs(areas, robot_count);
    std::cout << "Algorithm complete!" << std::endl;
    return 0;
}

