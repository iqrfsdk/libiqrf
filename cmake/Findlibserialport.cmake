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

include(FindPackageHandleStandardArgs)

find_package(PkgConfig)
pkg_check_modules(PC_libserialport QUIET libserialport)

# Look for the header file
find_path(libserialport_INCLUDE_DIR
    NAMES libserialport.h
    HINTS ${PC_libserialport_INCLUDEDIR} ${PC_libserialport_INCLUDE_DIRS}
)

set(libserialport_VERSION ${PC_libserialport_VERSION})

if (BUILD_STATIC)
    find_library(libserialport_STATIC_LIBRARY
        NAMES libserialport.a
        HINTS ${PC_libserialport_LIBDIR} ${PC_libserialport_LIBRARY_DIRS}
    )

    find_package_handle_standard_args(libserialport_STATIC
        FOUND_VAR libserialport_STATIC_FOUND
        REQUIRED_VARS libserialport_STATIC_LIBRARY libserialport_INCLUDE_DIR
        VERSION_VAR libserialport_VERSION
    )

    if(libserialport_STATIC_FOUND)
        set(libserialport_STATIC_LIBRARIES ${libserialport_STATIC_LIBRARY})
        set(libserialport_INCLUDE_DIRS ${libserialport_INCLUDE_DIR})
        message(STATUS "Found libserialport static: ${libserialport_STATIC_LIBRARY}")
        message(STATUS "Found libserialport version: ${libserialport_VERSION}")
    else()
        set(libserialport_STATIC_LIBRARIES)
        set(libserialport_INCLUDE_DIRS)
        message(WARNING "No static libserialport library found.")
    endif()
endif ()

if (BUILD_SHARED)
    # Look for the library
    find_library(libserialport_LIBRARY
        NAMES serialport
        HINTS ${PC_libserialport_LIBDIR} ${PC_libserialport_LIBRARY_DIRS}
    )

    find_package_handle_standard_args(libserialport
        FOUND_VAR libserialport_FOUND
        REQUIRED_VARS libserialport_LIBRARY libserialport_INCLUDE_DIR
        VERSION_VAR libserialport_VERSION
    )

    if(libserialport_FOUND)
        set(libserialport_LIBRARIES ${libserialport_LIBRARY})
        set(libserialport_INCLUDE_DIRS ${libserialport_INCLUDE_DIR})
        message(STATUS "Found libserialport: ${libserialport_LIBRARY}")
        message(STATUS "Found libserialport version: ${libserialport_VERSION}")
    else()
        set(libserialport_LIBRARIES)
        set(libserialport_INCLUDE_DIRS)
        message(WARNING "No shared libserialport library found.")
    endif()
endif ()

mark_as_advanced(libserialport_INCLUDE_DIRS libserialport_LIBRARIES)
