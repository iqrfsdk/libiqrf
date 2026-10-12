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

function(iqrf_add_library LIB_NAME)
    # Argument handling
    set(_flags "")
    set(_oneValueArgs INCLUDE_DIR)
    set(_multiValueArgs
        BOOST_COMPONENTS
        DEPS_INCLUDES
        DEPS_STATIC
        DEPS_SHARED
        HEADERS
        PKGCONFIG_LIBS_PRIVATE
        PKGCONFIG_REQUIRES
        SOURCES
    )
    cmake_parse_arguments(ARG "${_flags}" "${_oneValueArgs}" "${_multiValueArgs}" "" ${ARGN})

    set(_targets_new)

    # ABI may change in every minor release before 1.0, so the minor version is a part of the soname
    if (PROJECT_VERSION_MAJOR EQUAL 0)
        set(_soversion "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    else ()
        set(_soversion "${PROJECT_VERSION_MAJOR}")
    endif ()

    set(PUBLIC_INCLUDE_DIRS
        $<BUILD_INTERFACE:${libiqrf_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    if (
        (NOT DEFINED ARG_HEADERS OR ARG_HEADERS STREQUAL "") AND
        (NOT DEFINED ARG_SOURCES OR ARG_SOURCES STREQUAL "")
    )
        message(FATAL_ERROR "No headers or sources provided for library ${LIB_NAME}.")
    endif ()

    if (NOT DEFINED ARG_INCLUDE_DIR OR ARG_INCLUDE_DIR STREQUAL "")
        message(STATUS "Library ${LIB_NAME} does not specify include directory, using default.")
        set(ARG_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME}")
    else ()
        message(STATUS "Library ${LIB_NAME} include directory: ${ARG_INCLUDE_DIR}")
    endif ()

    if (BUILD_STATIC)
        set(STATIC_TARGET iqrf_${LIB_NAME}_static)
        add_library(${STATIC_TARGET} STATIC ${ARG_SOURCES} ${ARG_HEADERS})
        set_target_properties(${STATIC_TARGET} PROPERTIES
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${_soversion}"
            OUTPUT_NAME iqrf_${LIB_NAME}
            PREFIX "lib"
        )
        target_include_directories(${STATIC_TARGET} PUBLIC ${PUBLIC_INCLUDE_DIRS})
        if (ARG_DEPS_INCLUDES)
            target_include_directories(${STATIC_TARGET} PUBLIC ${ARG_DEPS_INCLUDES})
        endif ()
        if (ARG_BOOST_COMPONENTS)
            set(BOOST_USE_STATIC_LIBS ON)
            find_package(Boost REQUIRED COMPONENTS ${ARG_BOOST_COMPONENTS})
            target_link_libraries(${STATIC_TARGET} PUBLIC ${Boost_LIBRARIES})
        endif ()
        if (ARG_DEPS_STATIC)
            target_link_libraries(${STATIC_TARGET} PUBLIC ${ARG_DEPS_STATIC})
        endif ()

        install(
            TARGETS ${STATIC_TARGET}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${ARG_INCLUDE_DIR}
        )

        list(APPEND _targets_new ${STATIC_TARGET})
    endif ()

    if (BUILD_SHARED)
        set(SHARED_TARGET iqrf_${LIB_NAME})
        add_library(${SHARED_TARGET} SHARED ${ARG_SOURCES} ${ARG_HEADERS})
        set_target_properties(${SHARED_TARGET} PROPERTIES
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${_soversion}"
        )
        target_include_directories(${SHARED_TARGET} PUBLIC ${PUBLIC_INCLUDE_DIRS})
        if (ARG_DEPS_INCLUDES)
            target_include_directories(${SHARED_TARGET} PUBLIC ${ARG_DEPS_INCLUDES})
        endif ()
        if (ARG_BOOST_COMPONENTS)
            set(BOOST_USE_STATIC_LIBS OFF)
            find_package(Boost REQUIRED COMPONENTS ${ARG_BOOST_COMPONENTS})
            target_link_libraries(${SHARED_TARGET} PUBLIC ${Boost_LIBRARIES})
        endif ()
        if (ARG_DEPS_SHARED)
            target_link_libraries(${SHARED_TARGET} PUBLIC ${ARG_DEPS_SHARED})
        endif ()

        install(
            TARGETS ${SHARED_TARGET}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${ARG_INCLUDE_DIR}
        )

        list(APPEND _targets_new ${SHARED_TARGET})
    endif ()

    # Install only headers of the library (e.g. without headers for other platforms)
    file(RELATIVE_PATH REL_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include" "${ARG_INCLUDE_DIR}")
    install(FILES ${ARG_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${REL_INCLUDE_DIR}")

    # pkg-config file, static linking is covered by the same file as both variants share the library name
    set(_pc_name iqrf_${LIB_NAME})
    set(_pc_requires ${ARG_PKGCONFIG_REQUIRES})
    foreach (_dep IN LISTS ARG_DEPS_SHARED)
        if (_dep MATCHES "^iqrf_")
            list(APPEND _pc_requires "${_dep} = ${PROJECT_VERSION}")
        endif ()
    endforeach ()
    list(JOIN _pc_requires ", " _pc_requires)
    list(JOIN ARG_PKGCONFIG_LIBS_PRIVATE " " _pc_libs_private)
    if (IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(_pc_libdir "${CMAKE_INSTALL_LIBDIR}")
    else ()
        set(_pc_libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    endif ()
    if (IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
        set(_pc_includedir "${CMAKE_INSTALL_INCLUDEDIR}")
    else ()
        set(_pc_includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    endif ()
    configure_file(
        "${libiqrf_SOURCE_DIR}/cmake/iqrf.pc.in"
        "${CMAKE_BINARY_DIR}/pkgconfig/${_pc_name}.pc"
        @ONLY
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/pkgconfig/${_pc_name}.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    )

    get_property(_targets_existing GLOBAL PROPERTY IQRF_EXPORTABLE_TARGETS)
    if(NOT _targets_existing)
        set(_targets_existing "")
    endif()

    list(APPEND _targets_existing ${_targets_new})
    set_property(GLOBAL PROPERTY IQRF_EXPORTABLE_TARGETS "${_targets_existing}")
endfunction()
