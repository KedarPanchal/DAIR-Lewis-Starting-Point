cmake_minimum_required(VERSION 4.2.3)

include(CheckCXXCompilerFlag)

# -- PROJECT CONFIGURATION ----------------------------------------------------

project(
    robot_start_position
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# Add CMake prefix path to find dependencies
if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH 
        "/opt/homebrew" 
        "/usr/local" 
        "opt/homebrew/opt/qt"
    )
endif()

# Rely on Boost to find itself instead of FindBoost
if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Forward timestamps for URL fetching
if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# -- ADDRESS SANITIZER CONFIGURATION ------------------------------------------
if (MSVC)
    set(ASAN_FLAG "/fsanitize=address")
else()
    set(ASAN_FLAG "-fsanitize=address")
endif()

# Check if address sanitizer is supported by the compiler
set(CMAKE_REQUIRED_LINK_OPTIONS ${ASAN_FLAG})
check_cxx_compiler_flag(${ASAN_FLAG} ASAN_SUPPORTED)    
unset(CMAKE_REQUIRED_LINK_OPTIONS)

# -- PROJECT DEPENDENCIES -----------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL)
find_package(CGAL REQUIRED COMPONENTS Qt6)
find_package(Boost REQUIRED)
find_package(MPFR REQUIRED)

# -- LINK PROJECT DEPENDENCIES ------------------------------------------------
# All non-main files should be linked to an object library for tests
add_library(robot_start_position_core OBJECT
    src/utilities.cpp 
    src/graph_construction.cpp 
    src/coverage_path.cpp
    src/start_nodes.cpp
    src/visualization.cpp
)

# Main executable
add_executable(robot_start_position 
    src/main.cpp 
)

# Link libraries and include headers to the object library
target_include_directories(robot_start_position_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(robot_start_position_core PUBLIC
    CGAL::CGAL 
    CGAL::CGAL_Qt6 
    Boost::boost 
    ${MPFR_LIBRARIES}
)

target_link_libraries(robot_start_position PUBLIC
    robot_start_position_core
)

# Define preprocessor macros and compile options
target_compile_definitions(robot_start_position_core PUBLIC
    CGAL_USE_BASIC_VIEWER
    CGAL_CHECK_EXPENSIVE
    CGAL_CHECK_EXACTNESS
)

if (ASAN_SUPPORTED)
    target_compile_options(robot_start_position_core PUBLIC -g ${ASAN_FLAG})
    target_link_options(robot_start_position_core PUBLIC ${ASAN_FLAG})
endif()

# -- TESTING CONFIGURATION -----------------------------------------------------
include(CTest)

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
