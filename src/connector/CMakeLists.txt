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

set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/connector")

file(GLOB LIB_HEADERS "${LIB_INCLUDE_DIR}/*.h")

install(
    DIRECTORY ${LIB_INCLUDE_DIR}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf"
)

add_subdirectory(tcp)
add_subdirectory(uart)
