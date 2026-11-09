#include <vector>
#include <variant>
#include <utility>
#include <iostream>

#include "cgal_types.hpp"
#include "utilities.hpp"

int main() {
    std::variant<HoledPolygon, std::string> maybe_wall_space = read_polygon(std::cin);
    if (std::holds_alternative<std::string>(maybe_wall_space)) {
        std::cerr << "Error in line 1: Invalid wall space polygon: " << std::get<std::string>(maybe_wall_space) << std::endl;
        return 1;
    }
    
    std::vector<std::pair<fscalar, fscalar>> parameters = read_wallpapering_parameters(std::cin);
    if (parameters.empty()) {
        std::cerr << "Error in line 2: Invalid wallpapering parameters or none provided" << std::endl;
        return 1;
    }

    std::variant<std::pair<fscalar, fscalar>, std::string> maybe_robot_parameters = read_robot_parameters(std::cin);
    if (std::holds_alternative<std::string>(maybe_robot_parameters)) {
        std::cerr << "Error in line 3: Invalid robot parameters: " << std::get<std::string>(maybe_robot_parameters) << std::endl;
        return 1;
    }

    return 0;
}
