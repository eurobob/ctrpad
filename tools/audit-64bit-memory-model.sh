#!/bin/sh

set -eu

ctrpad_root_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ctrpad_i686_build_dir="${ctrpad_root_dir}/build-linux-i686-baseline"
ctrpad_object="${ctrpad_i686_build_dir}/CMakeFiles/ctr_native.dir/main.c.o"
ctrpad_output_dir="${ctrpad_root_dir}/build-64bit-audit"
ctrpad_container_image="ctrpad-linux-i686:ubuntu-24.04"
ctrpad_compile_log="${ctrpad_output_dir}/forced-lp64-compile.stderr"
ctrpad_compile_stdout="${ctrpad_output_dir}/forced-lp64-compile.stdout"

if [ ! -f "${ctrpad_object}" ]; then
    echo "Missing i686 debug object: ${ctrpad_object}" >&2
    echo "Run tools/build-linux-i686-baseline.sh first." >&2
    exit 1
fi
if ! command -v dwarfdump >/dev/null 2>&1; then
    echo "dwarfdump is required for the pinned-layout census." >&2
    exit 1
fi
if ! command -v node >/dev/null 2>&1; then
    echo "Node.js is required for the pinned-layout census." >&2
    exit 1
fi

mkdir -p "${ctrpad_output_dir}"

"${ctrpad_root_dir}/tools/audit-64bit-layout-census.mjs" \
    "${ctrpad_object}" \
    "${ctrpad_root_dir}" \
    >"${ctrpad_output_dir}/layout-census.tsv" \
    2>"${ctrpad_output_dir}/layout-census-summary.txt"

set +e
docker run --rm \
    --platform linux/amd64 \
    --volume "${ctrpad_root_dir}:/src:ro" \
    --volume "${ctrpad_i686_build_dir}:/out:ro" \
    "${ctrpad_container_image}" \
    sh -c '
        cc \
            -DBUILD=926 \
            -DCTR_INTERNAL \
            -DCTR_NATIVE \
            -DCTR_NATIVE_BUILD_ID=\"lp64-audit\" \
            -DCTR_NATIVE_VERSION=\"lp64-audit\" \
            -I/src/include \
            -I/out/externals/SDL/include-revision \
            -I/src/externals/SDL/include \
            -std=c17 \
            -Wpointer-to-int-cast \
            -Wint-to-pointer-cast \
            -Wno-error \
            -fmax-errors=0 \
            -fsyntax-only \
            /src/main.c
    ' >"${ctrpad_compile_stdout}" 2>"${ctrpad_compile_log}"
ctrpad_compile_status=$?
set -e

sed -n -E \
    's#^/src/([^:]+):([0-9]+):([0-9]+): warning: cast from pointer to integer of different size.*#\1:\2:\3#p' \
    "${ctrpad_compile_log}" |
    LC_ALL=C sort -u >"${ctrpad_output_dir}/pointer-to-integer-sites.txt"

sed -n -E \
    's#^/src/([^:]+):([0-9]+):([0-9]+): warning: cast to pointer from integer of different size.*#\1:\2:\3#p' \
    "${ctrpad_compile_log}" |
    LC_ALL=C sort -u >"${ctrpad_output_dir}/integer-to-pointer-sites.txt"

sed -E 's/:[0-9]+$//' "${ctrpad_output_dir}/pointer-to-integer-sites.txt" \
    >"${ctrpad_output_dir}/pointer-to-integer-lines.txt"
sed -E 's/:[0-9]+$//' "${ctrpad_output_dir}/integer-to-pointer-sites.txt" \
    >"${ctrpad_output_dir}/integer-to-pointer-lines.txt"
LC_ALL=C sort -u \
    "${ctrpad_output_dir}/pointer-to-integer-lines.txt" \
    "${ctrpad_output_dir}/integer-to-pointer-lines.txt" \
    >"${ctrpad_output_dir}/all-pointer-narrowing-lines.txt"

ctrpad_pointer_to_integer_count=$(wc -l <"${ctrpad_output_dir}/pointer-to-integer-sites.txt" | tr -d ' ')
ctrpad_integer_to_pointer_count=$(wc -l <"${ctrpad_output_dir}/integer-to-pointer-sites.txt" | tr -d ' ')
ctrpad_pointer_line_count=$(wc -l <"${ctrpad_output_dir}/all-pointer-narrowing-lines.txt" | tr -d ' ')
ctrpad_static_assert_count=$(grep -c 'error: static assertion failed' "${ctrpad_compile_log}" || true)
ctrpad_compile_error_count=$(grep -c ': error:' "${ctrpad_compile_log}" || true)

if [ "${ctrpad_compile_status}" -ne 0 ] && [ "${ctrpad_compile_error_count}" -eq 0 ]; then
    echo "The forced LP64 compiler failed without a captured compiler error." >&2
    exit 1
fi
if [ "${ctrpad_compile_error_count}" -ne "${ctrpad_static_assert_count}" ]; then
    echo "The forced LP64 compiler exposed non-layout errors." >&2
    exit 1
fi

if command -v sha256sum >/dev/null 2>&1; then
    ctrpad_object_hash=$(sha256sum "${ctrpad_object}" | awk '{print $1}')
    ctrpad_census_hash=$(sha256sum "${ctrpad_output_dir}/layout-census.tsv" | awk '{print $1}')
else
    ctrpad_object_hash=$(shasum -a 256 "${ctrpad_object}" | awk '{print $1}')
    ctrpad_census_hash=$(shasum -a 256 "${ctrpad_output_dir}/layout-census.tsv" | awk '{print $1}')
fi

{
    echo "source_commit=$(git -C "${ctrpad_root_dir}" rev-parse HEAD)"
    echo "i686_object_sha256=${ctrpad_object_hash}"
    echo "layout_census_sha256=${ctrpad_census_hash}"
    echo "forced_lp64_compile_exit=${ctrpad_compile_status}"
    echo "forced_lp64_compile_errors=${ctrpad_compile_error_count}"
    echo "forced_lp64_static_assert_failures=${ctrpad_static_assert_count}"
    echo "pointer_to_integer_coordinates=${ctrpad_pointer_to_integer_count}"
    echo "integer_to_pointer_coordinates=${ctrpad_integer_to_pointer_count}"
    echo "unique_pointer_narrowing_source_lines=${ctrpad_pointer_line_count}"
    sed 's/^/census=/' "${ctrpad_output_dir}/layout-census-summary.txt"
} >"${ctrpad_output_dir}/summary.txt"

cat "${ctrpad_output_dir}/summary.txt"
