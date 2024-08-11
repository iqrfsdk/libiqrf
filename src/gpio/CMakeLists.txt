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

set(LIB_NAME gpio)
file(GLOB LIB_HEADERS "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/*.h" "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/driver/*.h")
file(GLOB LIB_SOURCES "*.cpp" "driver/*.cpp")

include(${libiqrf_SOURCE_DIR}/cmake/FindLibgpiod.cmake)

find_package(Boost REQUIRED COMPONENTS filesystem)

if (BUILD_STATIC)
	add_library(iqrf_${LIB_NAME}_static STATIC ${LIB_SOURCES} ${LIB_HEADERS})
	set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
	target_include_directories(iqrf_${LIB_NAME}_static PUBLIC ${libiqrf_SOURCE_DIR}/include)
	target_link_libraries(iqrf_${LIB_NAME}_static ${LIB_LIBGPIOD} ${Boost_LIBRARIES} iqrf_common iqrf_log)
	set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES OUTPUT_NAME iqrf_${LIB_NAME})
	set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES PREFIX "lib")
endif ()

add_library(iqrf_${LIB_NAME} SHARED ${LIB_SOURCES} ${LIB_HEADERS})
set_target_properties(iqrf_${LIB_NAME} PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
target_link_libraries(iqrf_${LIB_NAME} ${LIB_LIBGPIOD} ${Boost_LIBRARIES} iqrf_common iqrf_log)
target_include_directories(iqrf_${LIB_NAME} PUBLIC ${libiqrf_SOURCE_DIR}/include)

install(TARGETS iqrf_${LIB_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
if (BUILD_STATIC)
	install(TARGETS iqrf_${LIB_NAME}_static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
endif ()
install(DIRECTORY ${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")
