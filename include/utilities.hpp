#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cgal_types.hpp"
#include "graph_construction.hpp"

// -- UTILITY FUNCTIONS -------------------------------------------------------

std::variant<HoledPolygon, std::string> read_polygon(std::istream& in);

std::vector<std::pair<fscalar, fscalar>> read_wallpapering_parameters(
    std::istream& in);

std::variant<std::pair<fscalar, fscalar>, std::string> read_robot_parameters(
    std::istream& in);

// Helper function for converting numeric types
template <typename To, typename From>
To convert(const From& x) {
    std::ostringstream str_representation;
    str_representation << std::setprecision(HP_PRECISION) << x;
    std::istringstream is(str_representation.str());

    To result;
    is >> std::setprecision(HP_PRECISION) >> result;
    return result;
}

std::ostream& operator<<(std::ostream& os, const Graph& g);

template <typename Collection, typename IterType>
std::vector<std::ranges::subrange<IterType>> split_into_views(Collection& collection) {
    std::vector<std::ranges::subrange<IterType>> views;

    const size_t num_elements = collection.size();
    if (num_elements == 0) return views;

    const size_t base_views = num_elements / NUM_THREADS;
    const size_t remainder = num_elements % NUM_THREADS;

    auto start = collection.begin();
    for (size_t i = 0; i < NUM_THREADS && start != collection.end(); ++i) {
        size_t view_size = base_views + (i < remainder ? 1 : 0);
        auto end = start;
        std::advance(end, view_size);
        views.emplace_back(start, end);
        start = end;
    }

    return views;
}

#endif
