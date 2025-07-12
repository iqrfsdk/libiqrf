enable_testing()

find_package(GTest REQUIRED)

include(GoogleTest)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${GTEST_INCLUDE_DIRS})

file(GLOB_RECURSE TEST_SOURCES "*Test.cpp")
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests GTest::GTest GTest::Main iqrf_connector_uart iqrf_gpio iqrf_log)

gtest_discover_tests(tests)
