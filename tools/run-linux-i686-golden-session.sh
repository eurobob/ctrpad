#!/bin/sh

set -eu

ctrpad_root_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ctrpad_container_image="ctrpad-linux-i686:ubuntu-24.04"
ctrpad_container_name="ctrpad-golden-session"
ctrpad_build_dir="${ctrpad_root_dir}/build-linux-i686-baseline"
ctrpad_disc_image=${CTRPAD_DISC_IMAGE:-"${ctrpad_root_dir}/assets/ctr-u.bin"}
ctrpad_novnc_port=${CTRPAD_GOLDEN_PORT:-6080}
ctrpad_host_uid=$(id -u)
ctrpad_host_gid=$(id -g)

case "${ctrpad_novnc_port}" in
    ""|*[!0-9]*)
        echo "CTRPAD_GOLDEN_PORT must be a numeric TCP port." >&2
        exit 1
        ;;
esac

if [ "${ctrpad_novnc_port}" -lt 1 ] || [ "${ctrpad_novnc_port}" -gt 65535 ]; then
    echo "CTRPAD_GOLDEN_PORT must be between 1 and 65535." >&2
    exit 1
fi

if [ ! -x "${ctrpad_build_dir}/ctr_native" ]; then
    echo "Missing i686 build: ${ctrpad_build_dir}/ctr_native" >&2
    echo "Run tools/build-linux-i686-baseline.sh first." >&2
    exit 1
fi

if [ ! -f "${ctrpad_disc_image}" ]; then
    echo "Missing user-supplied NTSC-U raw image: ${ctrpad_disc_image}" >&2
    echo "Set CTRPAD_DISC_IMAGE to an alternate local path if needed." >&2
    exit 1
fi

ctrpad_disc_dir=$(CDPATH= cd -- "$(dirname "${ctrpad_disc_image}")" && pwd)
ctrpad_disc_image="${ctrpad_disc_dir}/$(basename "${ctrpad_disc_image}")"
ctrpad_disc_bytes=$(wc -c < "${ctrpad_disc_image}" | tr -d ' ')
if [ $((ctrpad_disc_bytes % 2352)) -ne 0 ]; then
    echo "Retail image size is not a multiple of 2352-byte raw sectors: ${ctrpad_disc_bytes}" >&2
    exit 1
fi

if [ -n "$(git -C "${ctrpad_root_dir}" status --porcelain --untracked-files=no)" ]; then
    echo "Golden recording requires a clean tracked worktree." >&2
    exit 1
fi

ctrpad_source_build_id=$(git -C "${ctrpad_root_dir}" rev-parse --short=12 HEAD)
ctrpad_binary_version=$(docker run --rm \
    --platform linux/amd64 \
    --volume "${ctrpad_build_dir}:/out:ro" \
    "${ctrpad_container_image}" \
    /out/ctr_native --version)
case "${ctrpad_binary_version}" in
    *"(${ctrpad_source_build_id})")
        ;;
    *)
        echo "The i686 binary does not match source commit ${ctrpad_source_build_id}:" >&2
        echo "${ctrpad_binary_version}" >&2
        echo "Run tools/build-linux-i686-baseline.sh again." >&2
        exit 1
        ;;
esac

if docker container inspect "${ctrpad_container_name}" >/dev/null 2>&1; then
    echo "Container already exists: ${ctrpad_container_name}" >&2
    echo "Stop or rename that session before starting another." >&2
    exit 1
fi

mkdir -p "${ctrpad_build_dir}/assets" "${ctrpad_build_dir}/mesa-cache"

echo "CTRPad golden session: http://127.0.0.1:${ctrpad_novnc_port}/vnc.html?autoconnect=true&resize=scale"
echo "Recording begins at startup. Use F10 to finalize it, then close the game window."
echo "Reports are written under ${ctrpad_build_dir}/debug/reports/."

docker run --rm \
    --name "${ctrpad_container_name}" \
    --platform linux/amd64 \
    --user "${ctrpad_host_uid}:${ctrpad_host_gid}" \
    --publish "127.0.0.1:${ctrpad_novnc_port}:6080" \
    --env DISPLAY=:99 \
    --env SDL_AUDIODRIVER=dummy \
    --env MESA_SHADER_CACHE_DIR=/out/mesa-cache \
    --env MESA_SHADER_CACHE_MAX_SIZE=64M \
    --env XDG_RUNTIME_DIR=/tmp/ctrpad-runtime \
    --volume "${ctrpad_build_dir}:/out" \
    --volume "${ctrpad_disc_image}:/out/assets/ctr-u.bin:ro" \
    "${ctrpad_container_image}" \
    sh -euxc '
        mkdir -p /tmp/.X11-unix /tmp/ctrpad-runtime
        chmod 700 /tmp/ctrpad-runtime

        Xvfb :99 -screen 0 1280x720x24 -nolisten tcp >/tmp/ctrpad-xvfb.log 2>&1 &
        ctrpad_xvfb_pid=$!
        ctrpad_display_ready=0
        ctrpad_attempt=0
        while [ "${ctrpad_attempt}" -lt 100 ]; do
            if xdotool getdisplaygeometry >/dev/null 2>&1; then
                ctrpad_display_ready=1
                break
            fi
            if ! kill -0 "${ctrpad_xvfb_pid}" 2>/dev/null; then
                break
            fi
            ctrpad_attempt=$((ctrpad_attempt + 1))
            sleep 0.1
        done
        if [ "${ctrpad_display_ready}" -ne 1 ]; then
            cat /tmp/ctrpad-xvfb.log >&2
            exit 1
        fi

        x11vnc -display :99 -forever -shared -nopw -localhost -rfbport 5900 \
            -o /tmp/ctrpad-x11vnc.log >/dev/null 2>&1 &
        websockify --web=/usr/share/novnc 6080 localhost:5900 \
            >/tmp/ctrpad-websockify.log 2>&1 &

        exec /out/ctr_native --record --detailed
    '
