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

# Finds libserialport library
#
# The module is installed with the libiqrf CMake package configuration and used by find_dependency().
#
# Result variables:
#   libserialport_FOUND          True if libserialport header and shared or static library were found
#   libserialport_INCLUDE_DIRS   libserialport include directories
#   libserialport_VERSION        libserialport version (from pkg-config, may be empty)
#
# Imported targets (created only if the respective library was found):
#   libserialport::libserialport         shared libserialport library
#   libserialport::libserialport_static  static libserialport library

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_libserialport QUIET libserialport)
endif ()

find_path(libserialport_INCLUDE_DIR
    NAMES libserialport.h
    HINTS ${PC_libserialport_INCLUDEDIR} ${PC_libserialport_INCLUDE_DIRS}
)
find_library(libserialport_LIBRARY
    NAMES serialport
    HINTS ${PC_libserialport_LIBDIR} ${PC_libserialport_LIBRARY_DIRS}
)
find_library(libserialport_STATIC_LIBRARY
    NAMES libserialport.a
    HINTS ${PC_libserialport_LIBDIR} ${PC_libserialport_LIBRARY_DIRS}
)

set(libserialport_VERSION ${PC_libserialport_VERSION})

set(_libserialport_LIBRARIES_FOUND FALSE)
if (libserialport_LIBRARY OR libserialport_STATIC_LIBRARY)
    set(_libserialport_LIBRARIES_FOUND TRUE)
endif ()

find_package_handle_standard_args(libserialport
    REQUIRED_VARS libserialport_INCLUDE_DIR _libserialport_LIBRARIES_FOUND
    VERSION_VAR libserialport_VERSION
)

if (libserialport_FOUND)
    set(libserialport_INCLUDE_DIRS ${libserialport_INCLUDE_DIR})

    if (libserialport_LIBRARY AND NOT TARGET libserialport::libserialport)
        add_library(libserialport::libserialport INTERFACE IMPORTED)
        set_target_properties(libserialport::libserialport PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${libserialport_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${libserialport_LIBRARY}"
        )
    endif ()
    if (libserialport_STATIC_LIBRARY AND NOT TARGET libserialport::libserialport_static)
        add_library(libserialport::libserialport_static INTERFACE IMPORTED)
        set_target_properties(libserialport::libserialport_static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${libserialport_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${libserialport_STATIC_LIBRARY}"
        )
    endif ()
endif ()

mark_as_advanced(libserialport_INCLUDE_DIR libserialport_LIBRARY libserialport_STATIC_LIBRARY)
