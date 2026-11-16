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


set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/connector/tcp")

file(GLOB LIB_HEADERS_BASE
    "${LIB_INCLUDE_DIR}/ITcpSocket.h"
    "${LIB_INCLUDE_DIR}/TcpConfig.h"
    "${LIB_INCLUDE_DIR}/TcpConnector.h"
)
file(GLOB LIB_SOURCES_BASE
    "ITcpSocket.cpp"
    "TcpConnector.cpp"
)

# Platform-specific TCP socket implementation
set(TCP_PLATFORM_LIBRARIES "")
set(TCP_PKGCONFIG_LIBS_PRIVATE "")
if (WIN32)
    file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/WinsockTcpSocket.h")
    file(GLOB LIB_SOURCES_EXTRA "WinsockTcpSocket.cpp")
    set(TCP_PLATFORM_LIBRARIES ws2_32)
    set(TCP_PKGCONFIG_LIBS_PRIVATE -lws2_32)
elseif (UNIX)
    file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/PosixTcpSocket.h")
    file(GLOB LIB_SOURCES_EXTRA "PosixTcpSocket.cpp")
else ()
    message(WARNING "TCP socket is not implemented for ${CMAKE_SYSTEM_NAME}, TCP connector will not be usable.")
    set(LIB_HEADERS_EXTRA "")
    set(LIB_SOURCES_EXTRA "")
endif ()

iqrf_add_library(
    connector_tcp
    HEADERS ${LIB_HEADERS_BASE} ${LIB_HEADERS_EXTRA}
    SOURCES ${LIB_SOURCES_BASE} ${LIB_SOURCES_EXTRA}
    INCLUDE_DIR ${LIB_INCLUDE_DIR}
    DEPS_STATIC ${TCP_PLATFORM_LIBRARIES} iqrf_log_static
    DEPS_SHARED ${TCP_PLATFORM_LIBRARIES} iqrf_log
    PKGCONFIG_LIBS_PRIVATE ${TCP_PKGCONFIG_LIBS_PRIVATE}
)
