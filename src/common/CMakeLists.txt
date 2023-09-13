set(LIB_NAME common)
file(GLOB LIB_HEADERS "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.h" "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.hpp")
file(GLOB LIB_SOURCES "*.c" "*.cpp")

add_library(iqrf_common INTERFACE ${LIB_HEADERS})
add_library(iqrf::common ALIAS iqrf_common)
target_include_directories(iqrf_common INTERFACE ${libiqrf_SOURCE_DIR}/include)

install(TARGETS iqrf_common PUBLIC_HEADER DESTINATION include/${LIB_NAME})
install(DIRECTORY ${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")
