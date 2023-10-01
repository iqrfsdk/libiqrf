# Copyright 2023 MICRORISC s.r.o.
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
	set(LIBGPIOD_LIB libgpiod.a gpiod)
else ()
	set(LIBGPIOD_LIB gpiod)
endif ()

find_library(HAVE_LIBGPIOD NAMES ${LIBGPIOD_LIB})
if (HAVE_LIBGPIOD)
	set(LIB_LIBGPIOD ${HAVE_LIBGPIOD})
	message(STATUS "libgpiod found: ${LIB_LIBGPIOD}")
	set(CMAKE_REQUIRED_LIBRARIES ${LIB_LIBGPIOD})
else ()
	message(FATAL_ERROR "libgpiod not found")
endif ()
