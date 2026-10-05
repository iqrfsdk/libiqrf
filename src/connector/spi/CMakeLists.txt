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

set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/connector/spi")

file(GLOB LIB_HEADERS_BASE
    "${LIB_INCLUDE_DIR}/ISpiDevice.h"
    "${LIB_INCLUDE_DIR}/SpiConfig.h"
    "${LIB_INCLUDE_DIR}/SpiConnector.h"
    "${LIB_INCLUDE_DIR}/SpiProtocol.h"
)
file(GLOB LIB_SOURCES_BASE
    "ISpiDevice.cpp"
    "SpiConnector.cpp"
    "SpiProtocol.cpp"
)

# Platform-specific SPI device implementation
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/LinuxSpiDevice.h")
    file(GLOB LIB_SOURCES_EXTRA "LinuxSpiDevice.cpp")
else ()
    message(WARNING "SPI device is not implemented for ${CMAKE_SYSTEM_NAME}, SPI connector will not be usable.")
    set(LIB_HEADERS_EXTRA "")
    set(LIB_SOURCES_EXTRA "")
endif ()

iqrf_add_library(
    connector_spi
    HEADERS ${LIB_HEADERS_BASE} ${LIB_HEADERS_EXTRA}
    SOURCES ${LIB_SOURCES_BASE} ${LIB_SOURCES_EXTRA}
    INCLUDE_DIR ${LIB_INCLUDE_DIR}
    DEPS_STATIC iqrf_gpio_static iqrf_log_static
    DEPS_SHARED iqrf_gpio iqrf_log
)
