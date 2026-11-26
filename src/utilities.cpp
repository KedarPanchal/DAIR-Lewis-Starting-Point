#include "utilities.hpp"

#include <exception>
#include <istream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <CGAL/IO/WKT.h>

#include "cgal_types.hpp"

// Reads a polygon from the input stream
std::variant<HoledPolygon, std::string> read_polygon(std::istream& in) {
    // Extract polygon as a string
    std::string string_repr;
    std::getline(in, string_repr);
    std::istringstream is{string_repr};

    // Attempt to parse the polygon
    HoledPolygon polygon;

    // If successfully parsed, return the polygon; otherwise, return the
    // original string
    if (CGAL::IO::read_polygon_WKT(is, polygon)) {
        return polygon;
    } else {
        return string_repr;
    }
}

std::vector<std::pair<fscalar, fscalar>> read_wallpapering_parameters(
    std::istream& in) {
    // Extract the line of parameters
    std::string parameter_line;
    std::getline(in, parameter_line);
    std::istringstream is{parameter_line};

    // Extract the number of parameter pairs (layers)
    unsigned int layer_count;
    if (!(is >> layer_count)) {
        return {};
    }

    std::vector<std::pair<fscalar, fscalar>> parameters;
    for (unsigned int i = 0; i < layer_count; ++i) {
        std::string l_str;
        std::string o_max_str;
        if (!(is >> l_str >> o_max_str)) {
            return {};
        }

        try {
            std::istringstream l_stream{l_str};
            std::istringstream o_max_stream{o_max_str};
            fscalar l;
            fscalar o_max;
            l_stream >> l;
            o_max_stream >> o_max;
            if (l <= 0 || o_max <= 0) {
                return {};
            }
            parameters.emplace_back(l, o_max);
        } catch (const std::exception& e) {
            // Don't know the exact exception, so just catch all
            return {};
        }
    }

    return parameters;
}

std::variant<std::pair<fscalar, fscalar>, std::string>
read_robot_parameters(std::istream& in) {
    std::string string_repr;
    std::getline(in, string_repr);
    std::istringstream is{string_repr};

    fscalar theta_max;
    fscalar radius;
    if (is >> theta_max >> radius) {
        return std::make_pair(theta_max, radius);
    } else {
        return string_repr;
    }
}

std::ostream& operator<<(std::ostream& os, const Graph& g) {
    for (const auto& [v, neighbors] : g) {
        os << "[" << v.ID() << "] -> ";
        for ([[maybe_unused]] const auto& [u, _1, _2] : neighbors) {
            os << u.ID() << " ";
        }
    }

    return os;
}

Curve::Point_2 convert(const Point& p) {
    return Curve::Point_2(p.x(), p.y());
}
