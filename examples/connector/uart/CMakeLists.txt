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

set(EXAMPLE_NAME iqrf-connector-uart-example)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Boost CONFIG REQUIRED COMPONENTS program_options)
find_package(Threads REQUIRED)

add_executable(${EXAMPLE_NAME} main.cpp)
target_include_directories(${EXAMPLE_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(${EXAMPLE_NAME} PUBLIC iqrf_connector_uart iqrf_gpio iqrf_log)
target_link_libraries(${EXAMPLE_NAME} PRIVATE ${Boost_LIBRARIES} Threads::Threads)

install(TARGETS ${EXAMPLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
