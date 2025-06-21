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

set(LIB_NAME connector_spi)
set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/connector/spi")

file(GLOB LIB_HEADERS "${LIB_INCLUDE_DIR}/*.h")
file(GLOB LIB_SOURCES "*.cpp")

if (BUILD_STATIC)
    add_library(iqrf_${LIB_NAME}_static STATIC ${LIB_SOURCES} ${LIB_HEADERS})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}" SOVERSION "${PROJECT_VERSION_MAJOR}")
    target_include_directories(iqrf_${LIB_NAME}_static PUBLIC ${libiqrf_SOURCE_DIR}/include)
    target_link_libraries(iqrf_${LIB_NAME}_static iqrf_gpio_static iqrf_log_static)
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES OUTPUT_NAME iqrf_${LIB_NAME})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES PREFIX "lib")
endif ()

if (BUILD_SHARED)
    add_library(iqrf_${LIB_NAME} SHARED ${LIB_SOURCES} ${LIB_HEADERS})
    set_target_properties(iqrf_${LIB_NAME} PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}" SOVERSION "${PROJECT_VERSION_MAJOR}")
    target_link_libraries(iqrf_${LIB_NAME} iqrf_gpio iqrf_log)
    target_include_directories(iqrf_${LIB_NAME} PUBLIC ${libiqrf_SOURCE_DIR}/include)
endif ()

install(TARGETS iqrf_${LIB_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${LIB_INCLUDE_DIR})
if (BUILD_STATIC)
    install(TARGETS iqrf_${LIB_NAME}_static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${LIB_INCLUDE_DIR})
endif ()
install(DIRECTORY ${LIB_INCLUDE_DIR} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf/connector")
