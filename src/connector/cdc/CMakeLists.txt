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

find_package(libserialport REQUIRED)
if (BUILD_SHARED AND NOT TARGET libserialport::libserialport)
    message(FATAL_ERROR "Shared libserialport library not found")
endif ()
if (BUILD_STATIC AND NOT TARGET libserialport::libserialport_static)
    message(FATAL_ERROR "Static libserialport library not found")
endif ()

# libserialport depends on libusb on FreeBSD
if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(CDC_PLATFORM_LIBRARIES usb)
    set(CDC_PKGCONFIG_LIBS_PRIVATE -lusb)
else ()
    set(CDC_PLATFORM_LIBRARIES "")
    set(CDC_PKGCONFIG_LIBS_PRIVATE "")
endif ()

set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/connector/cdc")

file(GLOB LIB_HEADERS "${LIB_INCLUDE_DIR}/*.h")
file(GLOB LIB_SOURCES "*.cpp")

iqrf_add_library(
    connector_cdc
    HEADERS ${LIB_HEADERS}
    SOURCES ${LIB_SOURCES}
    INCLUDE_DIR ${LIB_INCLUDE_DIR}
    DEPS_STATIC libserialport::libserialport_static ${CDC_PLATFORM_LIBRARIES} iqrf_log_static
    DEPS_SHARED libserialport::libserialport ${CDC_PLATFORM_LIBRARIES} iqrf_log
    PKGCONFIG_REQUIRES libserialport
    PKGCONFIG_LIBS_PRIVATE ${CDC_PKGCONFIG_LIBS_PRIVATE}
)
