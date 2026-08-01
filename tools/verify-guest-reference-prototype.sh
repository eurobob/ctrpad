#!/bin/sh

set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 GOLDEN_REPORT_DIRECTORY" >&2
    exit 1
fi

ctrpad_root_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ctrpad_report_dir=$(CDPATH= cd -- "$1" && pwd)
ctrpad_state_file="${ctrpad_report_dir}/state.ctrstates"
ctrpad_build_dir="${ctrpad_root_dir}/build-guest-reference-prototype"
ctrpad_frame=${CTRPAD_GUEST_REF_FRAME:-1800}
ctrpad_cc=${CC:-cc}

if [ ! -f "${ctrpad_state_file}" ]; then
    echo "Missing golden checkpoint container: ${ctrpad_state_file}" >&2
    exit 1
fi

mkdir -p "${ctrpad_build_dir}"
"${ctrpad_cc}" \
    -std=c17 \
    -Wall \
    -Wextra \
    -Werror \
    -pedantic \
    "${ctrpad_root_dir}/tools/prototype-guest-references.c" \
    -o "${ctrpad_build_dir}/prototype-guest-references"

"${ctrpad_build_dir}/prototype-guest-references" "${ctrpad_state_file}" "${ctrpad_frame}"
