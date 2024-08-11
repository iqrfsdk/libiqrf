# Copyright 2023-2024 MICRORISC s.r.o.
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

set(LIB_NAME common)
file(GLOB LIB_HEADERS "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.h" "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.hpp")
file(GLOB LIB_SOURCES "*.c" "*.cpp")

add_library(iqrf_${LIB_NAME} INTERFACE ${LIB_HEADERS})
target_include_directories(iqrf_${LIB_NAME} INTERFACE ${libiqrf_SOURCE_DIR}/include)

install(TARGETS iqrf_${LIB_NAME} PUBLIC_HEADER DESTINATION include/${LIB_NAME})
install(DIRECTORY ${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")
