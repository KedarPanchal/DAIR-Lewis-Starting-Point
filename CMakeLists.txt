cmake_minimum_required(VERSION 3.26.5)

# Set release to optimize for speed
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

project(
    robot_start_position
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fexperimental-library)
endif()

# Add CMake prefix path to find dependencies
if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local" "opt/homebrew/opt/qt")
endif()

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Set sanitizer flags based on the compiler
if (MSVC)
    set(ASAN_FLAG "/fsanitize=address")
    set(TSAN_FLAG "/fsanitize=thread")
else()
    set(ASAN_FLAG "-fsanitize=address")
    set(TSAN_FLAG "-fsanitize=thread")
endif()

# Choose ASAN or TSAN based on user input
option(USE_ASAN "Enable AddressSanitizer" OFF)
option(USE_TSAN "Enable ThreadSanitizer" OFF)

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL)
find_package(CGAL REQUIRED COMPONENTS Qt6)
find_package(Boost REQUIRED)
find_package(MPFR REQUIRED)
find_package(SQLite3 REQUIRED)

# Add the executable target
add_executable(robot_start_position 
    src/main.cpp 
    src/utilities.cpp 
    src/graph_construction.cpp 
    src/coverage_path.cpp
    src/coverage_segmentation.cpp
    src/visualization.cpp
    src/cache.cpp
)

add_executable(validate_parameters
    tools/validate_parameters.cpp
    src/utilities.cpp
)

set(ALL_TARGETS robot_start_position validate_parameters)

# Link libraries and include headers
foreach (target IN LISTS ALL_TARGETS)
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${target} PRIVATE 
        CGAL::CGAL 
        CGAL::CGAL_Qt6 
        Boost::boost 
        ${MPFR_LIBRARIES}
        SQLite3::SQLite3
    )
    if (NOT APPLE)
        target_compile_options(${target} PRIVATE -fno-plt)
    endif()
    target_compile_options(${target} PRIVATE -march=native)
endforeach()

# Define preprocessor macros and compile options
target_compile_definitions(robot_start_position PRIVATE CGAL_USE_BASIC_VIEWER)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(robot_start_position PRIVATE 
        CGAL_CHECK_EXPENSIVE
        CGAL_CHECK_EXACTNESS
    )
endif()

if (USE_ASAN)
    target_compile_options(robot_start_position PRIVATE -g ${ASAN_FLAG})
    target_link_options(robot_start_position PRIVATE ${ASAN_FLAG})
elseif (USE_TSAN)
    target_compile_options(robot_start_position PRIVATE -g ${TSAN_FLAG})
    target_link_options(robot_start_position PRIVATE ${TSAN_FLAG})
endif()
