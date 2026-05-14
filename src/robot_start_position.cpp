#include <iostream>
#include <string>
#include <sstream>
#include <variant>
#include <list>
#include <utility>
#include <exception>

#include <BURST/geometry.hpp>
#include <BURST/numeric.hpp>

#define CONCAT(a, b) a##b
#define CONCAT_LINE(a, b) CONCAT(a, b)

template <typename F>
struct defer : F {
    ~defer() { static_cast<F&>(*this)(); }
};

#define DEFER(...) do { \
    auto CONCAT_LINE(_defer_, __LINE__) = ::defer{[&] __VA_ARGS__}; \
} while(0)

// Reads a polygon from the input stream
std::variant<BURST::geometry::Polygon2D, std::string> read_polygon(std::istream& in) {
    // Extract polygon as a string
    std::string string_repr;
    std::getline(in, string_repr);
    std::istringstream is{string_repr};
    // Attempt to parse the polygon
    BURST::geometry::Polygon2D polygon;
    // If successfully parsed, return the polygon; otherwise, return the original string
    if (is >> polygon) return polygon;
    else return string_repr;
}

std::list<std::pair<BURST::numeric::fscalar, BURST::numeric::fscalar>> read_wallpapering_parameters(std::istream& in) {
    // Extract the line of parameters
    std::string parameter_line;
    std::getline(in, parameter_line);
    std::istringstream is{parameter_line};

    // Extract the number of parameter pairs (layers)
    int layer_count;
    if (!(is >> layer_count)) return {};
    
    std::list<std::pair<BURST::numeric::fscalar, BURST::numeric::fscalar>> parameters;
    for (size_t i = 0; i < layer_count; ++i) {
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

