# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2020-2024 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mygpiod

# Try to find libgpiod
#
# LIBGPIOD_FOUND
# LIBGPIOD_INCLUDE_DIRS
# LIBGPIOD_LIBRARIES

find_package(PkgConfig)
pkg_check_modules(PC_LIBGPIOD QUIET libgpiod)
include(FindPackageHandleStandardArgs)
include(${libiqrf_SOURCE_DIR}/cmake/AuxFunctions.cmake)

# Look for the header file
find_path(LIBGPIOD_INCLUDE_DIR
	NAMES gpiod.h
	HINTS ${PC_LIBGPIOD_INCLUDEDIR} ${PC_LIBGPIOD_INCLUDE_DIRS}
)

set(LIBGPIOD_VERSION ${PC_LIBGPIOD_VERSION})

if (BUILD_STATIC)
	find_library(LIBGPIOD_STATIC_LIBRARY
		NAMES libgpiod.a
		HINTS ${PC_LIBGPIOD_LIBDIR} ${PC_LIBGPIOD_LIBRARY_DIRS}
	)

	find_library(LIBGPIOD_STATIC_LIBRARY_CXX
		NAMES libgpiodcxx.a
		HINTS ${PC_LIBGPIOD_LIBDIR} ${PC_LIBGPIOD_LIBRARY_DIRS}
	)

	find_package_handle_standard_args(LIBGPIOD_STATIC
		FOUND_VAR LIBGPIOD_STATIC_FOUND
		REQUIRED_VARS
			LIBGPIOD_STATIC_LIBRARY
			LIBGPIOD_STATIC_LIBRARY_CXX
			LIBGPIOD_INCLUDE_DIR
		VERSION_VAR LIBGPIOD_VERSION
	)
endif ()

if (BUILD_SHARED)
	# Look for the library
	find_library(LIBGPIOD_LIBRARY
		NAMES gpiod
		HINTS ${PC_LIBGPIOD_LIBDIR} ${PC_LIBGPIOD_LIBRARY_DIRS}
	)

	find_library(LIBGPIOD_LIBRARY_CXX
		NAMES gpiodcxx
		HINTS ${PC_LIBGPIOD_LIBDIR} ${PC_LIBGPIOD_LIBRARY_DIRS}
	)

	find_package_handle_standard_args(LIBGPIOD
		FOUND_VAR LIBGPIOD_FOUND
		REQUIRED_VARS
			LIBGPIOD_LIBRARY
			LIBGPIOD_LIBRARY_CXX
			LIBGPIOD_INCLUDE_DIR
		VERSION_VAR LIBGPIOD_VERSION
	)
endif ()

if(LIBGPIOD_STATIC_FOUND)
	set(LIBGPIOD_STATIC_LIBRARIES ${LIBGPIOD_STATIC_LIBRARY} ${LIBGPIOD_STATIC_LIBRARY_CXX})
	set(LIBGPIOD_INCLUDE_DIRS ${LIBGPIOD_INCLUDE_DIR})
	message(STATUS "Found libgpiod static CXX: ${LIBGPIOD_STATIC_LIBRARY_CXX}")
	message(STATUS "Found libgpiod static: ${LIBGPIOD_STATIC_LIBRARY}")
	message(STATUS "Found libgpiod version: ${LIBGPIOD_VERSION}")
else()
	set(LIBGPIOD_STATIC_LIBRARIES)
	set(LIBGPIOD_INCLUDE_DIRS)
	message(WARNING "No static libgpiod library found.")
endif()

if(LIBGPIOD_FOUND)
	set(LIBGPIOD_LIBRARIES ${LIBGPIOD_LIBRARY} ${LIBGPIOD_LIBRARY_CXX})
	set(LIBGPIOD_INCLUDE_DIRS ${LIBGPIOD_INCLUDE_DIR})
	message(STATUS "Found libgpiod CXX: ${LIBGPIOD_LIBRARY_CXX}")
	message(STATUS "Found libgpiod: ${LIBGPIOD_LIBRARY}")
	message(STATUS "Found libgpiod version: ${LIBGPIOD_VERSION}")
else()
	set(LIBGPIOD_LIBRARIES)
	set(LIBGPIOD_INCLUDE_DIRS)
	message(WARNING "No shared libgpiod library found.")
endif()

parse_version(${LIBGPIOD_VERSION} "LIBGPIOD" TRUE)

mark_as_advanced(LIBGPIOD_INCLUDE_DIRS LIBGPIOD_LIBRARIES)
