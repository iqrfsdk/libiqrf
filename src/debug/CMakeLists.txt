set(LIB_NAME debug)
file(GLOB LIB_HEADERS "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.h" "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.hpp")
file(GLOB LIB_SOURCES "*.c" "*.cpp")

add_library(iqrf_debug INTERFACE ${LIB_HEADERS})
add_library(iqrf::debug ALIAS iqrf_debug)
target_include_directories(iqrf_debug INTERFACE ${libiqrf_SOURCE_DIR}/include)

install(TARGETS iqrf_debug PUBLIC_HEADER DESTINATION include/${LIB_NAME})
install(DIRECTORY ${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")