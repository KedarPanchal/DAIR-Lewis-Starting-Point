include(FetchContent)
include(GoogleTest)

# -- INCLUDE GTEST ------------------------------------------------------------
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# -- TEST EXECUTABLE ----------------------------------------------------------
# Create test executable
add_executable(robot_start_position_tests
)

# Link library to test
target_link_libraries(robot_start_position_tests PRIVATE
    robot_start_position_core
    GTest::gtest_main
)

if (ASAN_SUPPORTED)
    target_compile_options(robot_start_position_tests PRIVATE -g ${ASAN_FLAG})
    target_link_options(robot_start_position_tests PRIVATE ${ASAN_FLAG})
endif()

gtest_discover_tests(robot_start_position_tests)
