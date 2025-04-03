# Copyright MICRORISC s.r.o.
# SPDX-License-Identifier: Apache-2.0
# File: CMakeLists.txt
# Authors: Ondřej Hujňák <ondrej.hujnak@iqrf.org>
# Date: 2025-04-03
#
# This file is a part of the LIBIQRF. For the full license information, see the
# LICENSE file in the project root.

set(EXAMPLE_NAME iqrf-log-boost-logger)

# Find the Boost package
find_package(Boost REQUIRED COMPONENTS log log_setup system)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${EXAMPLE_NAME} main.cpp)
target_link_libraries(${EXAMPLE_NAME} PUBLIC iqrf_log)
target_link_libraries(${EXAMPLE_NAME} PRIVATE ${Boost_LIBRARIES})

install(TARGETS ${EXAMPLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
