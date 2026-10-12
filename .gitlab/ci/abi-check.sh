#!/usr/bin/env bash
# Copyright 2026 MICRORISC s.r.o.
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

# Checks ABI of the shared libraries against the last release tag.
#
# Fails if a library ABI changed incompatibly without the soname change.
# Reports are stored in the abi-report directory.
#
# Environment variables:
#   ABI_BASE_TAG - tag to compare against (default: last release tag before HEAD)

set -euo pipefail

SRC_DIR="$(git rev-parse --show-toplevel)"
REPORT_DIR="${SRC_DIR}/abi-report"
SUPPRESSIONS="${SRC_DIR}/.gitlab/ci/abi.suppr"
BASE_TAG="${ABI_BASE_TAG:-$(git -C "${SRC_DIR}" describe --tags --abbrev=0 --match 'v[0-9]*' HEAD^)}"
WORK_DIR="$(mktemp -d)"

cleanup() {
    git -C "${SRC_DIR}" worktree remove --force "${WORK_DIR}/base" 2>/dev/null || true
    rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

# Builds shared libraries with debug info, abidiff reads type layouts from DWARF
build() {
    local src="$1" build="$2"
    cmake -S "${src}" -B "${build}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_SHARED=ON \
        -DBUILD_STATIC=OFF \
        -DBUILD_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_DOCS=OFF
    cmake --build "${build}" -j"$(nproc)"
}

soname() {
    readelf -d "$1" | sed -n 's/.*Library soname: \[\(.*\)\]/\1/p'
}

echo "Comparing ABI against ${BASE_TAG}"
git -C "${SRC_DIR}" worktree add --detach "${WORK_DIR}/base" "${BASE_TAG}"
build "${WORK_DIR}/base" "${WORK_DIR}/build-base"
build "${SRC_DIR}" "${WORK_DIR}/build-head"

rm -rf "${REPORT_DIR}"
mkdir -p "${REPORT_DIR}"
failed=0

for new_lib in "${WORK_DIR}"/build-head/lib/libiqrf_*.so; do
    lib="$(basename "${new_lib}")"
    old_lib="${WORK_DIR}/build-base/lib/${lib}"
    if [[ ! -e "${old_lib}" ]]; then
        echo "NEW: ${lib} is not present in ${BASE_TAG}"
        continue
    fi

    old_soname="$(soname "${old_lib}")"
    new_soname="$(soname "${new_lib}")"

    # abidiff exit code is a bit mask: 1 - error, 2 - usage error, 4 - ABI change, 8 - incompatible ABI change
    set +e
    abidiff \
        --headers-dir1 "${WORK_DIR}/base/include" \
        --headers-dir2 "${SRC_DIR}/include" \
        --drop-private-types \
        --suppressions "${SUPPRESSIONS}" \
        "${old_lib}" "${new_lib}" > "${REPORT_DIR}/${lib}.txt"
    result=$?
    set -e

    if (( result & 3 )); then
        echo "ERROR: ${lib}: abidiff failed (exit code ${result})"
        failed=1
    elif (( result & 8 )); then
        if [[ "${old_soname}" == "${new_soname}" ]]; then
            echo "ERROR: ${lib}: incompatible ABI change, but soname ${new_soname} is unchanged"
            failed=1
        else
            echo "OK: ${lib}: incompatible ABI change with soname change ${old_soname} -> ${new_soname}"
        fi
    elif (( result & 4 )); then
        echo "OK: ${lib}: compatible ABI change"
    else
        echo "OK: ${lib}: no ABI change"
    fi
    cat "${REPORT_DIR}/${lib}.txt"
done

exit "${failed}"
