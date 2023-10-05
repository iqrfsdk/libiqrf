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

if (BUILD_STATIC)
	find_library(FOUND_LIBGPIOD_C_STATIC NAMES libgpiod.a)
	find_library(FOUND_LIBGPIOD_CXX_STATIC NAMES libgpiodcxx.a)

	if (NOT FOUND_LIBGPIOD_C_STATIC)
		message(FATAL_ERROR "Libgpiod C static library missing")
	endif()

	if (NOT FOUND_LIBGPIOD_CXX_STATIC)
		message(FATAL_ERROR "Libgpiod C++ static library missing")
	endif()

	message(STATUS "Found Libgpiod C (static): ${FOUND_LIBGPIOD_C_STATIC}")
	message(STATUS "Found Libgpiod CXX (static): ${FOUND_LIBGPIOD_CXX_STATIC}")
	set(LIBGPIOD_LIB_STATIC ${FOUND_LIBGPIOD_C_STATIC} ${FOUND_LIBGPIOD_CXX_STATIC})
endif ()

if (BUILD_SHARED)
	find_library(FOUND_LIBGPIOD_CXX NAMES libgpiodcxx.so)

	if (NOT FOUND_LIBGPIOD_CXX)
		message(FATAL_ERROR "Libgpiod CXX library missing")
	endif()

	message(STATUS "Found Libgpiod CXX: ${FOUND_LIBGPIOD_CXX}")
	set(LIBGPIOD_LIB ${FOUND_LIBGPIOD_CXX})
endif ()
