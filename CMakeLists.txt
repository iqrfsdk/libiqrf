# Copyright 2023-2025 MICRORISC s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.18)

project(libiqrf VERSION 0.1.2 DESCRIPTION "IQRF C/C++ library" LANGUAGES C CXX)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/include")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${libiqrf_SOURCE_DIR}/cmake" CACHE STRING "Modules for CMake" FORCE)

include(${libiqrf_SOURCE_DIR}/cmake/ccache.cmake)
include(${libiqrf_SOURCE_DIR}/cmake/hardening.cmake)
include(${libiqrf_SOURCE_DIR}/cmake/LibraryFunctions.cmake)
include(CMakePackageConfigHelpers)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_DOCS "Build documentation" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_STATIC "Build static library" OFF)
option(BUILD_SHARED "Build shared library" ON)
option(CODE_COVERAGE "Enable coverage reporting" OFF)

if (NOT BUILD_STATIC AND NOT BUILD_SHARED)
    message(FATAL_ERROR "At least one of BUILD_STATIC or BUILD_SHARED must be ON")
endif()

if (BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_IN "${libiqrf_SOURCE_DIR}/docs/doxygen/Doxyfile.in")
    set(DOXYGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    add_custom_target(doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif ()

if (BUILD_TESTS)
    add_compile_definitions(BUILD_TESTS=1)
else ()
    add_compile_definitions(BUILD_TESTS=0)
endif()

if (CODE_COVERAGE)
    set(COVERAGE_FLAGS "--coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_FLAGS}")
endif()

add_subdirectory(src)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

get_property(IQRF_EXPORTABLE_TARGETS GLOBAL PROPERTY IQRF_EXPORTABLE_TARGETS)

install(
    TARGETS ${IQRF_EXPORTABLE_TARGETS}
    EXPORT iqrfTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    EXPORT iqrfTargets
    FILE iqrfTargets.cmake
    NAMESPACE iqrf::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iqrf
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/iqrfConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

configure_package_config_file(
    "${libiqrf_SOURCE_DIR}/cmake/iqrfConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/iqrfConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iqrf
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/iqrfConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/iqrfConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/iqrf
)
