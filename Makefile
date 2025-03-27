# Copyright 2023-2025 IQRF Tech s.r.o.
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

build: clean
	mkdir build
	cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
	cmake --build build

lint:
	cpplint --quiet --recursive include/ src/ examples/
	cppcheck --enable=all --inconclusive -I include/ --suppress=missingIncludeSystem --quiet src/ examples/

test:
	ctest --test-dir build/tests

clean:
	rm -rf build
