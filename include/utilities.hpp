#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <variant>
#include <list>
#include <string>
#include <sstream>
#include <utility>

#include <BURST/geometry.hpp>
#include <BURST/numeric.hpp>

// -- UTILITY FUNCTIONS -------------------------------------------------------

// Reads a polygon from the input stream
inline std::variant<BURST::geometry::HoledPolygon2D, std::string> read_polygon(std::istream& in) {
    // Extract polygon as a string
    std::string string_repr;
    std::getline(in, string_repr);
    std::istringstream is{string_repr};
    // Attempt to parse the polygon
    BURST::geometry::HoledPolygon2D polygon;
    // If successfully parsed, return the polygon; otherwise, return the original string
    if (is >> polygon) return polygon;
    else return string_repr;
}

inline std::list<std::pair<BURST::numeric::fscalar, BURST::numeric::fscalar>> read_wallpapering_parameters(std::istream& in) {
    // Extract the line of parameters
    std::string parameter_line;
    std::getline(in, parameter_line);
    std::istringstream is{parameter_line};

    // Extract the number of parameter pairs (layers)
    unsigned int layer_count;
    if (!(is >> layer_count)) return {};
    
    std::list<std::pair<BURST::numeric::fscalar, BURST::numeric::fscalar>> parameters;
    for (unsigned int i = 0; i < layer_count; ++i) {
        std::string l_str, o_max_str;
        if (!(is >> l_str >> o_max_str)) return {};
        try {
            BURST::numeric::fscalar l{l_str};
            BURST::numeric::fscalar o_max{o_max_str};
            parameters.emplace_back(l, o_max);
        } catch (const std::exception& e) { // Don't know the exact exception, so just catch all
            return {};
        }
    }

    return parameters;
}

#endif
