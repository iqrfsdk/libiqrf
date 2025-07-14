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

set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/gpio")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(libgpiod REQUIRED)
    configure_file(
        ${LIB_INCLUDE_DIR}/libgpiodVersion.h.in
        ${LIB_INCLUDE_DIR}/libgpiodVersion.h
        @ONLY
    )
    if (libgpiod_VERSION_MAJOR EQUAL "2")
        file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/GpiodV2.h")
        file(GLOB LIB_SOURCES_EXTRA "GpiodV2.cpp")
    else()
        file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/GpiodV1.h")
        file(GLOB LIB_SOURCES_EXTRA "GpiodV1.cpp")
    endif()
    set(GPIO_STATIC_LIBRARIES ${libgpiod_STATIC_LIBRARIES})
    set(GPIO_LIBRARIES ${libgpiod_LIBRARIES})
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/GpioFreeBsd.h")
    file(GLOB LIB_SOURCES_EXTRA "GpioFreeBsd.cpp")
    set(GPIO_STATIC_LIBRARIES "")
    set(GPIO_LIBRARIES "")
endif()

file(GLOB LIB_HEADERS_BASE
    "${LIB_INCLUDE_DIR}/Base.h"
    "${LIB_INCLUDE_DIR}/Common.h"
    "${LIB_INCLUDE_DIR}/Config.h"
    "${LIB_INCLUDE_DIR}/Gpio.h"
    "${LIB_INCLUDE_DIR}/GpioMap.h"
    "${LIB_INCLUDE_DIR}/GpioMock.h"
    "${LIB_INCLUDE_DIR}/GpioResolver.h"
    "${LIB_INCLUDE_DIR}/version.h"
)
file(GLOB LIB_SOURCES_BASE
    "Config.cpp"
    "Gpio.cpp"
    "GpioMap.cpp"
    "GpioMock.cpp"
    "GpioResolver.cpp"
)

iqrf_add_library(
    gpio
    HEADERS ${LIB_HEADERS_BASE} ${LIB_HEADERS_EXTRA}
    SOURCES ${LIB_SOURCES_BASE} ${LIB_SOURCES_EXTRA}
    DEPS_STATIC ${GPIO_STATIC_LIBRARIES} iqrf_log_static
    DEPS_SHARED ${GPIO_LIBRARIES} iqrf_log
)
