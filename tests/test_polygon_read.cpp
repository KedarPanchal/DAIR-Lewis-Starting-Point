#include <gtest/gtest.h>

#include <fstream>
#include <variant>
#include <set>

#include "utilities.hpp"

// Test for correct load of a non-holed polygon from a file
TEST(PolygonReadTest, NonHoledPolygon) {
    std::ifstream file("build/tests/test_data/non_holed_square_1_layer.dat");
    auto result = read_polygon(file);

    ASSERT_TRUE(std::holds_alternative<BURST::geometry::HoledPolygon2D>(result)) << "Expected a Polygon2D, got an error with: " << std::get<std::string>(result);
    ASSERT_FALSE(std::get<BURST::geometry::HoledPolygon2D>(result).has_holes()) << "Expected no holes in the polygon, but holes were found";
    
    // Check if all vertices in the resultant polygon are parsed correctly
    auto polygon_result = std::get<BURST::geometry::HoledPolygon2D>(result);
    std::set<BURST::geometry::Point2D> expected_vertices = {
        BURST::geometry::Point2D{0, 0},
        BURST::geometry::Point2D{10, 0},
        BURST::geometry::Point2D{10, 10},
        BURST::geometry::Point2D{0, 10}
    };
    for (const auto& vertex : polygon_result.outer_boundary().vertices()) {
        if (!expected_vertices.contains(vertex)) FAIL() << "Unexpected vertex found in the parsed polygon: (" << vertex.x() << ", " << vertex.y() << ")";
        expected_vertices.erase(vertex);
    }
    ASSERT_TRUE(expected_vertices.empty()) << "Not all expected vertices were present in the parsed polygon";
}

// Test for correct load of a holed polygon from a file
TEST(PolygonReadTest, HoledPolygon) {
    std::ifstream file("build/tests/test_data/holed_square_3_layer.dat");
    auto result = read_polygon(file);

    ASSERT_TRUE(std::holds_alternative<BURST::geometry::HoledPolygon2D>(result)) << "Expected a Polygon2D, got an error with: " << std::get<std::string>(result);
    ASSERT_EQ(std::get<BURST::geometry::HoledPolygon2D>(result).number_of_holes(), 3) << "Expected 3 holes in the polygon, but a different number was found";

    // Check if all vertices in the outer boundary are parsed correctly
    auto polygon_result = std::get<BURST::geometry::HoledPolygon2D>(result);
    std::set<BURST::geometry::Point2D> expected_outer_vertices = {
        BURST::geometry::Point2D{0, 0},
        BURST::geometry::Point2D{10, 0},
        BURST::geometry::Point2D{10, 10},
        BURST::geometry::Point2D{0, 10}
    };
    for (const auto& vertex : polygon_result.outer_boundary().vertices()) {
        if (!expected_outer_vertices.contains(vertex)) FAIL() << "Unexpected vertex found in the parsed polygon's outer boundary: (" << vertex.x() << ", " << vertex.y() << ")";
        expected_outer_vertices.erase(vertex);
    }
    ASSERT_TRUE(expected_outer_vertices.empty()) << "Not all expected vertices were present in the parsed polygon's outer boundary";

    // Check if all vertices in the holes are parsed correctly
    std::set<BURST::geometry::Point2D> expected_hole_vertices = {
        BURST::geometry::Point2D{2, 4},
        BURST::geometry::Point2D{3, 4},
        BURST::geometry::Point2D{3, 6},
        BURST::geometry::Point2D{2, 6},
        BURST::geometry::Point2D{5, 3},
        BURST::geometry::Point2D{7, 3},
        BURST::geometry::Point2D{7, 4},
        BURST::geometry::Point2D{5, 4},
        BURST::geometry::Point2D{5, 7},
        BURST::geometry::Point2D{7, 7},
        BURST::geometry::Point2D{7, 8},
        BURST::geometry::Point2D{5, 8}
    };
    for (const auto& hole : polygon_result.holes()) {
        for (const auto& vertex : hole.vertices()) {
            if (!expected_hole_vertices.contains(vertex)) FAIL() << "Unexpected vertex found in the parsed polygon's holes: (" << vertex.x() << ", " << vertex.y() << ")";
            expected_hole_vertices.erase(vertex);
        }
    }
    ASSERT_TRUE(expected_hole_vertices.empty()) << "Not all expected vertices were present in the parsed polygon's holes";
}
