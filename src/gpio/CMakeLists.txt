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
set(LIB_INCLUDE_DIR "${libiqrf_SOURCE_DIR}/include/iqrf/${LIB_NAME}")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(libgpiod REQUIRED)
    if (libgpiod_VERSION_MAJOR EQUAL "2")
        file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/GpiodV2.h")
        file(GLOB LIB_SOURCES_EXTRA "GpiodV2.cpp")
    else()
        file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/GpiodV1.h")
        file(GLOB LIB_SOURCES_EXTRA "GpiodV1.cpp")
    endif()
    set(GPIO_STATIC_LIBRARIES ${libgpiod_STATIC_LIBRARIES})
    set(GPIO_LIBRARIES ${libgpiod_LIBRARIES})
elseif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    file(GLOB LIB_HEADERS_EXTRA "${LIB_INCLUDE_DIR}/GpioFreeBsd.h")
    file(GLOB LIB_SOURCES_EXTRA "GpioFreeBsd.cpp")
    set(GPIO_STATIC_LIBRARIES "")
    set(GPIO_LIBRARIES "")
endif()

configure_file(
    ${LIB_INCLUDE_DIR}/version.h.in
    ${LIB_INCLUDE_DIR}/version.h
    @ONLY
)
file(GLOB LIB_HEADERS_BASE
	"${LIB_INCLUDE_DIR}/Base.h"
	"${LIB_INCLUDE_DIR}/Common.h"
	"${LIB_INCLUDE_DIR}/Config.h"
	"${LIB_INCLUDE_DIR}/Gpio.h"
	"${LIB_INCLUDE_DIR}/GpioMap.h"
	"${LIB_INCLUDE_DIR}/GpioResolver.h"
	"${LIB_INCLUDE_DIR}/version.h"
)
file(GLOB LIB_SOURCES_BASE
	"Config.cpp"
	"Gpio.cpp"
	"GpioMap.cpp"
	"GpioResolver.cpp"
)

set(LIB_HEADERS ${LIB_HEADERS_BASE} ${LIB_HEADERS_EXTRA})
set(LIB_SOURCES ${LIB_SOURCES_BASE} ${LIB_SOURCES_EXTRA})

if (BUILD_STATIC)
    add_library(iqrf_${LIB_NAME}_static STATIC ${LIB_SOURCES} ${LIB_HEADERS})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}" SOVERSION "${PROJECT_VERSION_MAJOR}")
    target_include_directories(iqrf_${LIB_NAME}_static PUBLIC $<BUILD_INTERFACE:${libiqrf_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(iqrf_${LIB_NAME}_static ${GPIO_STATIC_LIBRARIES} iqrf_log_static)
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES OUTPUT_NAME iqrf_${LIB_NAME})
    set_target_properties(iqrf_${LIB_NAME}_static PROPERTIES PREFIX "lib")
endif ()

if (BUILD_SHARED)
    add_library(iqrf_${LIB_NAME} SHARED ${LIB_SOURCES} ${LIB_HEADERS})
    set_target_properties(iqrf_${LIB_NAME} PROPERTIES VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}" SOVERSION "${PROJECT_VERSION_MAJOR}")
    target_link_libraries(iqrf_${LIB_NAME} ${GPIO_LIBRARIES} iqrf_log)
    target_include_directories(iqrf_${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${libiqrf_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
endif ()

install(TARGETS iqrf_${LIB_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${LIB_INCLUDE_DIR})
if (BUILD_STATIC)
    install(TARGETS iqrf_${LIB_NAME}_static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${LIB_INCLUDE_DIR})
endif ()
install(DIRECTORY ${LIB_INCLUDE_DIR} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/iqrf")
