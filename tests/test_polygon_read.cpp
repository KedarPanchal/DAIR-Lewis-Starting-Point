#include <gtest/gtest.h>

#include <fstream>
#include <variant>
#include <set>

#include "utilities.hpp"

// Test for correct load of a non-holed polygon from a file
TEST(PolygonReadTest, NonHoledPolygon) {
    std::ifstream file("build/tests/test_data/non_holed_polygon_1_layer.dat");
    auto result = read_polygon(file);

    ASSERT_TRUE(std::holds_alternative<BURST::geometry::HoledPolygon2D>(result)) << "Expected a Polygon2D, got an error with: " << std::get<std::string>(result);
    ASSERT_FALSE(std::get<BURST::geometry::HoledPolygon2D>(result).has_holes()) << "Expected no holes in the polygon, but holes were found";
    
    // Check if all vertices in the resultant polygon are parsed correctly
    std::set<BURST::geometry::Point2D> expected_vertices = {
        BURST::geometry::Point2D{0, 0},
        BURST::geometry::Point2D{10, 0},
        BURST::geometry::Point2D{10, 10},
        BURST::geometry::Point2D{0, 10}
    };
    auto polygon_result = std::get<BURST::geometry::HoledPolygon2D>(result);
    for (const auto& vertex : polygon_result.outer_boundary().vertices()) expected_vertices.erase(vertex);
    ASSERT_TRUE(expected_vertices.empty()) << "Not all expected vertices were present in the parsed polygon";
}
