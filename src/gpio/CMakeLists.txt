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

set(LIB_NAME gpio)

include(${libiqrf_SOURCE_DIR}/cmake/FindLibgpiod.cmake)

configure_file(version.h.in ${libiqrf_SOURCE_DIR}/include/iqrf/gpio/version.h @ONLY)
file(GLOB LIB_HEADERS_BASE
	"${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/Base.h"
	"${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/Common.h"
	"${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/Config.h"
	"${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/Gpio.h"
	"${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/GpioMap.h"
	"${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/GpioResolver.h"
	"${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/version.h"
)
file(GLOB LIB_SOURCES_BASE
	"Config.cpp"
	"Gpio.cpp"
	"GpioMap.cpp"
	"GpioResolver.cpp"
)
if (LIBGPIOD_VERSION_MAJOR EQUAL "2")
	file(GLOB LIB_HEADERS_EXTRA "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/GpiodV2.h")
	file(GLOB LIB_SOURCES_EXTRA "GpiodV2.cpp")
else()
	file(GLOB LIB_HEADERS_EXTRA "${libiqrf_SOURCE_DIR}/include/${LIB_NAME}/GpiodV1.h")
	file(GLOB LIB_SOURCES_EXTRA "GpiodV1.cpp")
endif()

set(LIB_HEADERS ${LIB_HEADERS_BASE} ${LIB_HEADERS_EXTRA})
set(LIB_SOURCES ${LIB_SOURCES_BASE} ${LIB_SOURCES_EXTRA})

if (BUILD_STATIC)
    add_library(iqrf_${LIB_NAME}_static STATIC ${LIB_SOURCES} ${LIB_HEADERS})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    target_include_directories(iqrf_${LIB_NAME}_static PUBLIC ${libiqrf_SOURCE_DIR}/include)
    target_link_libraries(iqrf_${LIB_NAME}_static ${LIBGPIOD_STATIC_LIBRARIES} iqrf_common iqrf_log)
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES OUTPUT_NAME iqrf_${LIB_NAME})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES PREFIX "lib")
endif ()

if (BUILD_SHARED)
    add_library(iqrf_${LIB_NAME} SHARED ${LIB_SOURCES} ${LIB_HEADERS})
    set_target_properties(iqrf_${LIB_NAME} PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    target_link_libraries(iqrf_${LIB_NAME} ${LIBGPIOD_LIBRARIES} iqrf_common iqrf_log)
    target_include_directories(iqrf_${LIB_NAME} PUBLIC ${libiqrf_SOURCE_DIR}/include)
endif ()

install(TARGETS iqrf_${LIB_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
if (BUILD_STATIC)
    install(TARGETS iqrf_${LIB_NAME}_static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
endif ()
install(DIRECTORY ${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")
