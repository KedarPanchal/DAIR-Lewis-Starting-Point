#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <concepts>
#include <variant>
#include <list>
#include <string>
#include <sstream>
#include <utility>

#include <BURST/geometry.hpp>
#include <BURST/numeric.hpp>

// -- UTILITY MACROS ----------------------------------------------------------

#define CONCAT(a, b) a##b
#define CONCAT_LINE(a, b) CONCAT(a, b)

template <typename F> requires std::invocable<F&>
struct defer : F {
    ~defer() { static_cast<F&>(*this)(); }
};

#define DEFER(...) do { \
    auto CONCAT_LINE(_defer_, __LINE__) = ::defer{[&] __VA_ARGS__}; \
} while(0)


// -- UTILITY FUNCTIONS -------------------------------------------------------

// Reads a polygon from the input stream
inline std::variant<BURST::geometry::Polygon2D, std::string> read_polygon(std::istream& in) {
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

inline std::list<std::pair<BURST::numeric::fscalar, BURST::numeric::fscalar>> read_wallpapering_parameters(std::istream& in) {
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

#endif
