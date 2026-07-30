#!/bin/sh

set -eu

ctrpad_root_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ctrpad_build_dir="${ctrpad_root_dir}/build-macos-arm64-app"
ctrpad_app="${ctrpad_build_dir}/CTRPad.app"
ctrpad_executable="${ctrpad_app}/Contents/MacOS/CTRPad"
ctrpad_disc_image=${CTRPAD_DISC_IMAGE:-"${ctrpad_root_dir}/ref/CTR/CTR - Crash Team Racing (USA).bin"}

if [ ! -x "${ctrpad_executable}" ]; then
    echo "Missing macOS application build: ${ctrpad_app}" >&2
    echo "Run: cmake --preset macos-arm64-app && cmake --build --preset macos-arm64-app" >&2
    exit 1
fi
if [ ! -f "${ctrpad_disc_image}" ]; then
    echo "Missing user-supplied NTSC-U raw image: ${ctrpad_disc_image}" >&2
    echo "Set CTRPAD_DISC_IMAGE to an alternate local path if needed." >&2
    exit 1
fi

ctrpad_disc_bytes=$(wc -c < "${ctrpad_disc_image}" | tr -d ' ')
if [ $((ctrpad_disc_bytes % 2352)) -ne 0 ]; then
    echo "Retail image size is not a multiple of 2352-byte raw sectors: ${ctrpad_disc_bytes}" >&2
    exit 1
fi

mkdir -p "${ctrpad_build_dir}/assets"
ln -sfn "${ctrpad_disc_image}" "${ctrpad_build_dir}/assets/ctr-u.bin"

echo "CTRPad app: ${ctrpad_app}"
echo "Retail image remains external: ${ctrpad_disc_image}"
exec "${ctrpad_executable}" "$@"
