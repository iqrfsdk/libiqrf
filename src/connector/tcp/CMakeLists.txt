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


set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/connector/uart")

file(GLOB LIB_HEADERS "${LIB_INCLUDE_DIR}/*.h")
file(GLOB LIB_SOURCES "*.cpp")

find_package(Boost CONFIG REQUIRED COMPONENTS headers)

iqrf_add_library(
    connector_tcp
    HEADERS ${LIB_HEADERS}
    SOURCES ${LIB_SOURCES}
    INCLUDE_DIR ${LIB_INCLUDE_DIR}
    DEPS_INCLUDES ${Boost_INCLUDE_DIRS}
    DEPS_STATIC iqrf_log_static
    DEPS_SHARED iqrf_log
)
