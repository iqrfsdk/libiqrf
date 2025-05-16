# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2020-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mygpiod

# Try to find libgpiod
#
# libgpiod_FOUND
# libgpiod_INCLUDE_DIRS
# libgpiod_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_libgpiod QUIET libgpiod)
include(FindPackageHandleStandardArgs)
include(${libiqrf_SOURCE_DIR}/cmake/AuxFunctions.cmake)

# Look for the header file
find_path(libgpiod_INCLUDE_DIR
	NAMES gpiod.h
	HINTS ${PC_libgpiod_INCLUDEDIR} ${PC_libgpiod_INCLUDE_DIRS}
)

set(libgpiod_VERSION ${PC_libgpiod_VERSION})

if (BUILD_STATIC)
	find_library(libgpiod_STATIC_LIBRARY
		NAMES libgpiod.a
		HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS}
	)

	find_library(libgpiod_STATIC_LIBRARY_CXX
		NAMES libgpiodcxx.a
		HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS}
	)

	find_package_handle_standard_args(libgpiod_STATIC
		FOUND_VAR libgpiod_STATIC_FOUND
		REQUIRED_VARS
			libgpiod_STATIC_LIBRARY
			libgpiod_STATIC_LIBRARY_CXX
			libgpiod_INCLUDE_DIR
		VERSION_VAR libgpiod_VERSION
	)

    if(libgpiod_STATIC_FOUND)
        set(libgpiod_STATIC_LIBRARIES ${libgpiod_STATIC_LIBRARY} ${libgpiod_STATIC_LIBRARY_CXX})
        set(libgpiod_INCLUDE_DIRS ${libgpiod_INCLUDE_DIR})
        message(STATUS "Found libgpiod static CXX: ${libgpiod_STATIC_LIBRARY_CXX}")
        message(STATUS "Found libgpiod static: ${libgpiod_STATIC_LIBRARY}")
        message(STATUS "Found libgpiod version: ${libgpiod_VERSION}")
    else()
        set(libgpiod_STATIC_LIBRARIES)
        set(libgpiod_INCLUDE_DIRS)
        message(WARNING "No static libgpiod library found.")
    endif()
endif ()

if (BUILD_SHARED)
	# Look for the library
	find_library(libgpiod_LIBRARY
		NAMES gpiod
		HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS}
	)

	find_library(libgpiod_LIBRARY_CXX
		NAMES gpiodcxx
		HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS}
	)

	find_package_handle_standard_args(libgpiod
		FOUND_VAR libgpiod_FOUND
		REQUIRED_VARS
			libgpiod_LIBRARY
			libgpiod_LIBRARY_CXX
			libgpiod_INCLUDE_DIR
		VERSION_VAR libgpiod_VERSION
	)

    if(libgpiod_FOUND)
        set(libgpiod_LIBRARIES ${libgpiod_LIBRARY} ${libgpiod_LIBRARY_CXX})
        set(libgpiod_INCLUDE_DIRS ${libgpiod_INCLUDE_DIR})
        message(STATUS "Found libgpiod CXX: ${libgpiod_LIBRARY_CXX}")
        message(STATUS "Found libgpiod: ${libgpiod_LIBRARY}")
        message(STATUS "Found libgpiod version: ${libgpiod_VERSION}")
    else()
        set(libgpiod_LIBRARIES)
        set(libgpiod_INCLUDE_DIRS)
        message(WARNING "No shared libgpiod library found.")
    endif()
endif ()

parse_version(${libgpiod_VERSION} "libgpiod" TRUE)

mark_as_advanced(libgpiod_INCLUDE_DIRS libgpiod_LIBRARIES)
