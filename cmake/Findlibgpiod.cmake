# Copyright 2023-2026 MICRORISC s.r.o.
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

# Finds libgpiod C and C++ libraries
#
# The module is installed with the libiqrf CMake package configuration and used by find_dependency().
#
# Result variables:
#   libgpiod_FOUND          True if libgpiod headers and shared or static libraries were found
#   libgpiod_INCLUDE_DIRS   libgpiod include directories
#   libgpiod_VERSION        libgpiod version (from pkg-config), parsed into libgpiod_VERSION_MAJOR,
#                           libgpiod_VERSION_MINOR, libgpiod_VERSION_PATCH and libgpiod_VERSION_EXT
#
# Imported targets (created only if the respective libraries were found):
#   libgpiod::libgpiod         shared libgpiod and libgpiodcxx libraries
#   libgpiod::libgpiod_static  static libgpiod and libgpiodcxx libraries

include(FindPackageHandleStandardArgs)
include("${CMAKE_CURRENT_LIST_DIR}/AuxFunctions.cmake")

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_libgpiod QUIET libgpiod)
endif ()

find_path(libgpiod_INCLUDE_DIR
    NAMES gpiod.hpp
    HINTS ${PC_libgpiod_INCLUDEDIR} ${PC_libgpiod_INCLUDE_DIRS}
)
find_library(libgpiod_LIBRARY NAMES gpiod HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS})
find_library(libgpiod_CXX_LIBRARY NAMES gpiodcxx HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS})
find_library(libgpiod_STATIC_LIBRARY NAMES libgpiod.a HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS})
find_library(libgpiod_CXX_STATIC_LIBRARY
    NAMES libgpiodcxx.a
    HINTS ${PC_libgpiod_LIBDIR} ${PC_libgpiod_LIBRARY_DIRS}
)

set(libgpiod_VERSION ${PC_libgpiod_VERSION})

set(_libgpiod_SHARED_FOUND FALSE)
if (libgpiod_LIBRARY AND libgpiod_CXX_LIBRARY)
    set(_libgpiod_SHARED_FOUND TRUE)
endif ()
set(_libgpiod_STATIC_FOUND FALSE)
if (libgpiod_STATIC_LIBRARY AND libgpiod_CXX_STATIC_LIBRARY)
    set(_libgpiod_STATIC_FOUND TRUE)
endif ()
set(_libgpiod_LIBRARIES_FOUND FALSE)
if (_libgpiod_SHARED_FOUND OR _libgpiod_STATIC_FOUND)
    set(_libgpiod_LIBRARIES_FOUND TRUE)
endif ()

find_package_handle_standard_args(libgpiod
    REQUIRED_VARS libgpiod_INCLUDE_DIR libgpiod_VERSION _libgpiod_LIBRARIES_FOUND
    VERSION_VAR libgpiod_VERSION
)

if (libgpiod_FOUND)
    set(_libgpiod_VERBOSE TRUE)
    if (libgpiod_FIND_QUIETLY)
        set(_libgpiod_VERBOSE FALSE)
    endif ()
    parse_version(${libgpiod_VERSION} "libgpiod" ${_libgpiod_VERBOSE})
    set(libgpiod_INCLUDE_DIRS ${libgpiod_INCLUDE_DIR})

    if (_libgpiod_SHARED_FOUND AND NOT TARGET libgpiod::libgpiod)
        add_library(libgpiod::libgpiod INTERFACE IMPORTED)
        set_target_properties(libgpiod::libgpiod PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${libgpiod_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${libgpiod_CXX_LIBRARY};${libgpiod_LIBRARY}"
        )
    endif ()
    if (_libgpiod_STATIC_FOUND AND NOT TARGET libgpiod::libgpiod_static)
        add_library(libgpiod::libgpiod_static INTERFACE IMPORTED)
        set_target_properties(libgpiod::libgpiod_static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${libgpiod_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${libgpiod_CXX_STATIC_LIBRARY};${libgpiod_STATIC_LIBRARY}"
        )
    endif ()
endif ()

mark_as_advanced(
    libgpiod_INCLUDE_DIR
    libgpiod_LIBRARY
    libgpiod_CXX_LIBRARY
    libgpiod_STATIC_LIBRARY
    libgpiod_CXX_STATIC_LIBRARY
)
