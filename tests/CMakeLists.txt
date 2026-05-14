include(CheckCCompilerFlag)
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Define the address sanitizer flag based on the C compiler
if (MSCV)
    set(ASAN_FLAG "/fsanitize=address")
else()
    set(ASAN_FLAG "-fsanitize=address")
endif()

# Check if address sanitizer is supported
check_c_compiler_flag(${ASAN_FLAG} ASAN_SUPPORTED)

if (TEST_SCOPE STREQUAL "all")
    add_executable(test_all
        test_polygon_read.cpp
        test_parameter_read.cpp
    )
    target_include_directories(test_all PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(test_all GTest::gtest_main)

    if (ASAN_SUPPORTED)
        target_compile_options(test_all PRIVATE ${ASAN_FLAG})
        target_link_libraries(test_all PRIVATE ${ASAN_FLAG})
    endif()
else()
    # Polygon reading tests
    add_executable(test_polygon_read
        test_polygon_read.cpp
    )
    target_include_directories(test_polygon_read PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(test_polygon_read GTest::gtest_main)
    
    # Parameter reading tests
    add_executable(test_parameter_read
        test_parameter_read.cpp
    )
    target_include_directories(test_parameter_read PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(test_parameter_read GTest::gtest_main)

    if (ASAN_SUPPORTED)
        target_compile_options(test_polygon_read PRIVATE ${ASAN_FLAG})
        target_link_options(test_polygon_read PRIVATE ${ASAN_FLAG})
        target_compile_options(test_parameter_read PRIVATE ${ASAN_FLAG})
        target_link_options(test_parameter_read PRIVATE ${ASAN_FLAG})
    endif()
