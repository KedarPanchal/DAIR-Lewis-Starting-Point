#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <list>

#include "utilities.hpp"

// Test for correct load of a single layer/pair of valid parameters from a file
TEST(ParameterReadTest, SingleValidParameterLayer) {
    std::string file_path = "build/tests/test_data/non_holed_square_1_layer.dat";
    std::ifstream file(file_path);
    ASSERT_TRUE(file.is_open()) << "Failed to open test file: " << file_path;

    // Skip the first line which contains the polygon
    std::string dummy_line;
    std::getline(file, dummy_line);

    auto parameters = read_wallpapering_parameters(file);
    ASSERT_TRUE(parameters.size() == 1) << "Expected exactly one pair of parameters, but got " << parameters.size();

    // Check that the parameters were parsed correctly
    std::list<std::pair<BURST::numeric::fscalar, BURST::numeric::fscalar>> correct_parameters = {{1, 1}};
    for (size_t i = 0; !parameters.empty(); ++i) {
        ASSERT_EQ(parameters.front().first, correct_parameters.front().first) << "Parameter l for layer " << i << " was parsed incorrectly";
        ASSERT_EQ(parameters.front().second, correct_parameters.front().second) << "Parameter o_max for layer " << i << " was parsed incorrectly";
        parameters.pop_front();
        correct_parameters.pop_front();
    }
}

// Test for correct load of multiple layers/pairs of valid parameters from a file
TEST(ParameterReadTest, MultipleValidParameterLayer) {
    std::string file_path = "build/tests/test_data/holed_square_3_layer.dat";
    std::ifstream file(file_path);
    ASSERT_TRUE(file.is_open()) << "Failed to open test file: " << file_path;

    // Skip the first line which contains the polygon
    std::string dummy_line;
    std::getline(file, dummy_line);

    auto parameters = read_wallpapering_parameters(file);
    ASSERT_TRUE(parameters.size() == 3) << "Expected exactly three pairs of parameters, but got " << parameters.size();

    // Check that the parameters were parsed correctly
    std::list<std::pair<BURST::numeric::fscalar, BURST::numeric::fscalar>> correct_parameters = {{3, 2}, {2, 1}, {1, 0.5}};
    for (size_t i = 0; !parameters.empty(); ++i) {
        ASSERT_EQ(parameters.front().first, correct_parameters.front().first) << "Parameter l for layer " << i << " was parsed incorrectly";
        ASSERT_EQ(parameters.front().second, correct_parameters.front().second) << "Parameter o_max for layer " << i << " was parsed incorrectly";
        parameters.pop_front();
        correct_parameters.pop_front();
    }
}

// Test for correct handling of invalid parameter input (non-paired)
TEST(ParameterReadTest, InvalidParameterFormat) {
    std::string file_path = "build/tests/test_data/invalid_non_holed_polygon_invalid_parameter_format.dat";
    std::ifstream file(file_path);
    ASSERT_TRUE(file.is_open()) << "Failed to open test file: " << file_path;

    // Skip the first line which contains the polygon
    std::string dummy_line;
    std::getline(file, dummy_line);
    
    // Check parameters are blank
    auto parameters = read_wallpapering_parameters(file);
    ASSERT_TRUE(parameters.empty()) << "Expected no parameters to be parsed due to invalid format, but some parameters were returned";
}

// Test for correct handling of invalid parameter input (wrong number of parameters)
TEST(ParameterReadTest, WrongNumberOfParameters) {
    std::string file_path = "build/tests/test_data/invalid_holed_polygon_invalid_parameter_count.dat";
    std::ifstream file(file_path);
    ASSERT_TRUE(file.is_open()) << "Failed to open test file: " << file_path;

    // Skip the first line which contains the polygon
    std::string dummy_line;
    std::getline(file, dummy_line);

    // Check parameters are blank
    auto parameters = read_wallpapering_parameters(file);
    ASSERT_TRUE(parameters.empty()) << "Expected no parameters to be parsed due to wrong number of parameters, but some parameters were returned";
}
