#!/bin/sh

set -eu

ctrpad_root_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ctrpad_image="ctrpad-linux-i686:ubuntu-24.04"
ctrpad_build_dir="${ctrpad_root_dir}/build-linux-i686-baseline"
ctrpad_host_uid=$(id -u)
ctrpad_host_gid=$(id -g)
ctrpad_build_jobs=${CTRPAD_BUILD_JOBS:-8}

docker build \
    --platform linux/amd64 \
    --file "${ctrpad_root_dir}/tools/docker/linux-i686.Dockerfile" \
    --tag "${ctrpad_image}" \
    "${ctrpad_root_dir}"

mkdir -p "${ctrpad_build_dir}"

docker run --rm \
    --platform linux/amd64 \
    --env "CTRPAD_HOST_UID=${ctrpad_host_uid}" \
    --env "CTRPAD_HOST_GID=${ctrpad_host_gid}" \
    --env "CTRPAD_BUILD_JOBS=${ctrpad_build_jobs}" \
    --volume "${ctrpad_root_dir}:/src:ro" \
    --volume "${ctrpad_build_dir}:/out" \
    "${ctrpad_image}" \
    sh -euxc '
        git config --global --add safe.directory /src
        cmake -S /src -B /out -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_TESTING=ON \
            -DCMAKE_C_FLAGS=-m32 \
            -DCMAKE_EXE_LINKER_FLAGS=-m32
        cmake --build /out --parallel "${CTRPAD_BUILD_JOBS}"
        ctest --test-dir /out --output-on-failure
        file /out/ctr_native
        /out/ctr_native --version
        {
            gcc --version | head -1
            cmake --version | head -1
            ninja --version
            dpkg-query -W -f="\${Package}=\${Version}\n" \
                gcc gcc-multilib libc6-dev-i386 cmake ninja-build \
                libx11-dev:i386 libxcursor-dev:i386 libxext-dev:i386 \
                libxfixes-dev:i386 libxi-dev:i386 libxrandr-dev:i386 \
                libgl1-mesa-dev:i386 libasound2-dev:i386 \
                libudev-dev:i386 libdbus-1-dev:i386
        } > /out/toolchain-packages.txt
        chown -R "${CTRPAD_HOST_UID}:${CTRPAD_HOST_GID}" /out
    '
