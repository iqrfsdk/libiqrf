enable_testing()

find_package(GTest REQUIRED)

include(GoogleTest)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE TEST_SOURCES "*.cpp")
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests GTest::GTest GTest::Main iqrf_gpio)

gtest_discover_tests(tests)
