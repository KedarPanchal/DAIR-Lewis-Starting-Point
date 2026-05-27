#include "utilities.hpp"


#include <variant>
#include <list>
#include <string>
#include <istream>
#include <sstream>
#include <utility>

#include "cgal_types.hpp"

// Reads a polygon from the input stream
std::variant<HoledPolygon, std::string> read_polygon(std::istream& in) {
    // Extract polygon as a string
    std::string string_repr;
    std::getline(in, string_repr);
    std::istringstream is{string_repr};
    // Attempt to parse the polygon
    HoledPolygon polygon;
    // If successfully parsed, return the polygon; otherwise, return the original string
    if (is >> polygon) return polygon;
    else return string_repr;
}

std::list<std::pair<fscalar, fscalar>> read_wallpapering_parameters(std::istream& in) {
    // Extract the line of parameters
    std::string parameter_line;
    std::getline(in, parameter_line);
    std::istringstream is{parameter_line};

    // Extract the number of parameter pairs (layers)
    unsigned int layer_count;
    if (!(is >> layer_count)) return {};
    
    std::list<std::pair<fscalar, fscalar>> parameters;
    for (unsigned int i = 0; i < layer_count; ++i) {
        std::string l_str, o_max_str;
        if (!(is >> l_str >> o_max_str)) return {};
        try {
            fscalar l{l_str};
            fscalar o_max{o_max_str};
            if (l <= 0 || o_max <= 0) return {};
            parameters.emplace_back(l, o_max);
        } catch (const std::exception& e) { // Don't know the exact exception, so just catch all
            return {};
        }
    }

    return parameters;
}

std::variant<std::pair<fscalar, fscalar>, std::string> read_robot_parameters(std::istream& in) {
    std::string string_repr;
    std::getline(in, string_repr);
    std::istringstream is{string_repr};
    fscalar theta_max;
    fscalar radius;
    if (is >> theta_max >> radius) return std::make_pair(theta_max, radius);
    else return string_repr;
}
