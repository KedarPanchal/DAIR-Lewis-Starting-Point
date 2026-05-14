cmake_minimum_required(VERSION 4.2.3)

project(
    robot_start_position
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)

# Add CMake prefix path
if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local" "opt/homebrew/opt/qt")
endif()

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find dependencies
find_package(BURST REQUIRED)

# Add the executable target
add_executable(robot_start_position src/robot_start_position.cpp)

# Link libraries
target_link_libraries(robot_start_position PRIVATE BURST::BURST)
