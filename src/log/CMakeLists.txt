set(LIB_NAME log)
file(GLOB LIB_HEADERS "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.h" "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.hpp")
file(GLOB LIB_SOURCES "*.c" "*.cpp")

include(${libiqrf_SOURCE_DIR}/cmake/FindLibgpiod.cmake)

if (BUILD_STATIC)
    add_library(iqrf_${LIB_NAME}_static STATIC ${LIB_SOURCES} ${LIB_HEADERS})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    target_include_directories(iqrf_${LIB_NAME}_static PUBLIC ${libiqrf_SOURCE_DIR}/include)
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES OUTPUT_NAME iqrf_${LIB_NAME})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES PREFIX "lib")
endif ()

add_library(iqrf_${LIB_NAME} SHARED ${LIB_SOURCES} ${LIB_HEADERS})
add_library(iqrf::${LIB_NAME} ALIAS iqrf_${LIB_NAME})
set_target_properties(iqrf_${LIB_NAME} PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
target_include_directories(iqrf_${LIB_NAME} PUBLIC ${libiqrf_SOURCE_DIR}/include)

install(TARGETS iqrf_${LIB_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
if (BUILD_STATIC)
    install(TARGETS iqrf_${LIB_NAME}_static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
endif ()
install(DIRECTORY ${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")
