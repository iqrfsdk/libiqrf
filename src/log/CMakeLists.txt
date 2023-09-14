set(LIB_NAME log)
file(GLOB LIB_HEADERS "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.h" "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.hpp")
file(GLOB LIB_SOURCES "*.c" "*.cpp")

add_library(iqrf_${LIB_NAME} INTERFACE ${LIB_HEADERS})
add_library(iqrf::${LIB_NAME} ALIAS iqrf_${LIB_NAME})
target_include_directories(iqrf_${LIB_NAME} INTERFACE ${libiqrf_SOURCE_DIR}/include)

install(TARGETS iqrf_${LIB_NAME} PUBLIC_HEADER DESTINATION include/${LIB_NAME})
install(DIRECTORY ${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")