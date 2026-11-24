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

enable_testing()

# Adds a libFuzzer fuzz target and a test replaying its seed corpus
function(iqrf_add_fuzzer NAME)
    add_executable(fuzz_${NAME} ${NAME}Fuzzer.cpp)
    target_link_options(fuzz_${NAME} PRIVATE -fsanitize=fuzzer)
    foreach (_lib IN LISTS ARGN)
        if (TARGET iqrf_${_lib})
            target_link_libraries(fuzz_${NAME} PRIVATE iqrf_${_lib})
        else ()
            target_link_libraries(fuzz_${NAME} PRIVATE iqrf_${_lib}_static)
        endif ()
    endforeach ()
    target_include_directories(fuzz_${NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(
        NAME fuzz_${NAME}_corpus
        COMMAND fuzz_${NAME} -runs=0 ${CMAKE_CURRENT_SOURCE_DIR}/corpus/${NAME}
    )
endfunction()

iqrf_add_fuzzer(CdcProtocol connector_cdc)
iqrf_add_fuzzer(Hdlc connector_uart)
iqrf_add_fuzzer(SpiProtocol connector_spi)
iqrf_add_fuzzer(TrInfo)
