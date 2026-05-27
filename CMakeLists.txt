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

if (MSVC)
    set(ASAN_FLAG "/fsanitize=address")
else()
    set(ASAN_FLAG "-fsanitize=address")
endif()

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL)
find_package(CGAL REQUIRED COMPONENTS Qt6)
find_package(Boost REQUIRED)
find_package(MPFR REQUIRED)

# Add the executable target
add_executable(robot_start_position 
    src/robot_start_position.cpp 
    src/utilities.cpp 
    src/graph_construction.cpp 
    src/graph_algorithms.cpp
)

# Link libraries and include headers
target_include_directories(robot_start_position PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(robot_start_position PRIVATE CGAL::CGAL CGAL::CGAL_Qt6 Boost::boost ${MPFR_LIBRARIES})

# Define preprocessor macros and compile options
target_compile_definitions(robot_start_position PRIVATE CGAL_USE_BASIC_VIEWER)
target_compile_options(robot_start_position PRIVATE -g ${ASAN_FLAG})
target_link_options(robot_start_position PRIVATE ${ASAN_FLAG})
