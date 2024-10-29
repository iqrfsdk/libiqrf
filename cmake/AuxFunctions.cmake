# Copyright (C) 2020 Google, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# parse_version() reads and parses the version string from FILE, assigning the
# version string to ${PROJECT}_VERSION and the parsed version to
# ${PROJECT}_VERSION_MAJOR, ${PROJECT}_VERSION_MINOR, ${PROJECT}_VERSION_PATCH,
# and the optional ${PROJECT}_VERSION_FLAVOR.
#
# The version string take one of the forms:
#    <major>.<minor>.<patch>
#    <major>.<minor>.<patch>-<flavor>
function(parse_version VERSION_STRING PREFIX VERBOSE)
    if(${VERSION_STRING} MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)(-[a-zA-Z0-9]+)?")
        set(EXT "")
        if(NOT "${CMAKE_MATCH_4}" STREQUAL "")
            string(SUBSTRING ${CMAKE_MATCH_4} 1 -1 EXT)
        endif()
        set("${PREFIX}_VERSION_MAJOR"  ${CMAKE_MATCH_1} PARENT_SCOPE)
        set("${PREFIX}_VERSION_MINOR"  ${CMAKE_MATCH_2} PARENT_SCOPE)
        set("${PREFIX}_VERSION_PATCH"  ${CMAKE_MATCH_3} PARENT_SCOPE)
        set("${PREFIX}_VERSION_EXT" ${EXT} PARENT_SCOPE)
        set("${PREFIX}_VERSION" "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}${CMAKE_MATCH_4}" PARENT_SCOPE)
		if (VERBOSE)
			message(STATUS "Parsed version: ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}${CMAKE_MATCH_4}")
			message(STATUS "Major version: ${CMAKE_MATCH_1}")
			message(STATUS "Minor version: ${CMAKE_MATCH_2}")
			message(STATUS "Patch version: ${CMAKE_MATCH_3}")
			message(STATUS "Ext version: ${CMAKE_MATCH_4}")
		endif ()
    else()
        message(FATAL_ERROR "Unable to parse version from '${VERSION_STRING}'")
    endif()
endfunction()
