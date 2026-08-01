#!/bin/sh

set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 BUILD_REPORT_DIRECTORY MUTATION_FRAME_OR_AUTO" >&2
    exit 1
fi

ctrpad_root_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
ctrpad_container_image="ctrpad-linux-i686:ubuntu-24.04"
ctrpad_build_dir=${CTRPAD_I686_BUILD_DIR:-"${ctrpad_root_dir}/build-linux-i686-baseline"}
ctrpad_binary_path=${CTRPAD_I686_BINARY:-"${ctrpad_build_dir}/ctr_native"}
ctrpad_alt_loader_path=${CTRPAD_I686_ALT_LOADER:-}
ctrpad_toolchain_manifest=${CTRPAD_TOOLCHAIN_PACKAGES:-"${ctrpad_build_dir}/toolchain-packages.txt"}
ctrpad_require_coverage=${CTRPAD_REQUIRE_COVERAGE:-1}
ctrpad_finalize_only=${CTRPAD_FINALIZE_ONLY:-0}
ctrpad_report_dir=$1
ctrpad_mutation_frame=$2
ctrpad_disc_image=${CTRPAD_DISC_IMAGE:-"${ctrpad_root_dir}/assets/ctr-u.bin"}
ctrpad_expected_source_commit=${CTRPAD_EXPECTED_SOURCE_COMMIT:-$(git -C "${ctrpad_root_dir}" rev-parse HEAD)}
ctrpad_expected_frame_count=${CTRPAD_EXPECTED_FRAME_COUNT:-24232}
ctrpad_expected_checkpoint_count=${CTRPAD_EXPECTED_CHECKPOINT_COUNT:-81}
ctrpad_host_uid=$(id -u)
ctrpad_host_gid=$(id -g)

case "${ctrpad_expected_source_commit}" in
    ""|*[!0-9a-fA-F]*)
        echo "CTRPAD_EXPECTED_SOURCE_COMMIT must be a full hexadecimal Git commit." >&2
        exit 1
        ;;
esac
if [ "${#ctrpad_expected_source_commit}" -ne 40 ] ||
    ! git -C "${ctrpad_root_dir}" cat-file -e "${ctrpad_expected_source_commit}^{commit}" 2>/dev/null; then
    echo "CTRPAD_EXPECTED_SOURCE_COMMIT is not a full commit in this repository: ${ctrpad_expected_source_commit}" >&2
    exit 1
fi
case "${ctrpad_expected_frame_count}:${ctrpad_expected_checkpoint_count}" in
    *[!0-9:]*|0:*|*:0|"":*|*:"")
        echo "Expected frame and checkpoint counts must be nonzero integers." >&2
        exit 1
        ;;
esac

case "${ctrpad_mutation_frame}" in
    auto)
        ;;
    ""|*[!0-9]*)
        echo "MUTATION_FRAME must be a nonnegative integer or 'auto'." >&2
        exit 1
        ;;
esac

case "${ctrpad_require_coverage}" in
    0|1)
        ;;
    *)
        echo "CTRPAD_REQUIRE_COVERAGE must be 0 or 1." >&2
        exit 1
        ;;
esac

case "${ctrpad_finalize_only}" in
    0|1)
        ;;
    *)
        echo "CTRPAD_FINALIZE_ONLY must be 0 or 1." >&2
        exit 1
        ;;
esac

if [ ! -x "${ctrpad_binary_path}" ]; then
    echo "Missing i686 build: ${ctrpad_binary_path}" >&2
    exit 1
fi
if [ -n "${ctrpad_alt_loader_path}" ] && [ ! -x "${ctrpad_alt_loader_path}" ]; then
    echo "Missing alternate i686 loader: ${ctrpad_alt_loader_path}" >&2
    exit 1
fi
if [ ! -f "${ctrpad_toolchain_manifest}" ]; then
    echo "Missing build manifest: ${ctrpad_toolchain_manifest}" >&2
    exit 1
fi
if [ ! -f "${ctrpad_disc_image}" ]; then
    echo "Missing user-supplied NTSC-U raw image: ${ctrpad_disc_image}" >&2
    exit 1
fi
if [ ! -d "${ctrpad_report_dir}" ]; then
    echo "Missing report directory: ${ctrpad_report_dir}" >&2
    exit 1
fi
ctrpad_disc_bytes=$(wc -c < "${ctrpad_disc_image}" | tr -d ' ')
if [ $((ctrpad_disc_bytes % 2352)) -ne 0 ]; then
    echo "Retail image size is not a multiple of 2352-byte raw sectors: ${ctrpad_disc_bytes}" >&2
    exit 1
fi

if [ -n "$(git -C "${ctrpad_root_dir}" status --porcelain --untracked-files=no)" ]; then
    echo "Golden replay verification requires a clean tracked worktree." >&2
    exit 1
fi

ctrpad_build_dir=$(CDPATH= cd -- "${ctrpad_build_dir}" && pwd)
ctrpad_binary_dir=$(CDPATH= cd -- "$(dirname "${ctrpad_binary_path}")" && pwd)
ctrpad_binary_path="${ctrpad_binary_dir}/$(basename "${ctrpad_binary_path}")"
ctrpad_alt_loader_relative=
if [ -n "${ctrpad_alt_loader_path}" ]; then
    ctrpad_alt_loader_dir=$(CDPATH= cd -- "$(dirname "${ctrpad_alt_loader_path}")" && pwd)
    ctrpad_alt_loader_path="${ctrpad_alt_loader_dir}/$(basename "${ctrpad_alt_loader_path}")"
fi
ctrpad_toolchain_dir=$(CDPATH= cd -- "$(dirname "${ctrpad_toolchain_manifest}")" && pwd)
ctrpad_toolchain_manifest="${ctrpad_toolchain_dir}/$(basename "${ctrpad_toolchain_manifest}")"
ctrpad_report_dir=$(CDPATH= cd -- "${ctrpad_report_dir}" && pwd)
ctrpad_disc_dir=$(CDPATH= cd -- "$(dirname "${ctrpad_disc_image}")" && pwd)
ctrpad_disc_image="${ctrpad_disc_dir}/$(basename "${ctrpad_disc_image}")"

case "${ctrpad_binary_path}" in
    "${ctrpad_build_dir}/"*)
        ctrpad_binary_relative=${ctrpad_binary_path#"${ctrpad_build_dir}/"}
        ;;
    *)
        echo "Selected binary must be under ${ctrpad_build_dir}" >&2
        exit 1
        ;;
esac

if [ -n "${ctrpad_alt_loader_path}" ]; then
    case "${ctrpad_alt_loader_path}" in
        "${ctrpad_build_dir}/"*)
            ctrpad_alt_loader_relative=${ctrpad_alt_loader_path#"${ctrpad_build_dir}/"}
            ;;
        *)
            echo "Selected alternate loader must be under ${ctrpad_build_dir}" >&2
            exit 1
            ;;
    esac
fi

case "${ctrpad_report_dir}/" in
    "${ctrpad_build_dir}/"*)
        ctrpad_report_relative=${ctrpad_report_dir#"${ctrpad_build_dir}/"}
        ;;
    *)
        echo "Report must be under ${ctrpad_build_dir}" >&2
        exit 1
        ;;
esac

for ctrpad_required_file in input.ctrreplay state.ctrstates metadata.txt ctr-native.log; do
    if [ ! -f "${ctrpad_report_dir}/${ctrpad_required_file}" ]; then
        echo "Incomplete replay report: missing ${ctrpad_report_dir}/${ctrpad_required_file}" >&2
        exit 1
    fi
done
if [ "${ctrpad_require_coverage}" -eq 1 ] &&
    [ ! -f "${ctrpad_report_dir}/coverage.txt" ]; then
    echo "Incomplete replay report: missing ${ctrpad_report_dir}/coverage.txt" >&2
    exit 1
fi
for ctrpad_required_dir in memcard.seed memcard.recording; do
    if [ ! -d "${ctrpad_report_dir}/${ctrpad_required_dir}" ]; then
        echo "Incomplete replay report: missing ${ctrpad_report_dir}/${ctrpad_required_dir}/" >&2
        exit 1
    fi
done
for ctrpad_metadata_entry in \
    finalized=1 \
    recording_status=finalized \
    replay_version=4
do
    if ! grep -q "^${ctrpad_metadata_entry}$" "${ctrpad_report_dir}/metadata.txt"; then
        echo "Golden report metadata is not accepted: missing ${ctrpad_metadata_entry}." >&2
        exit 1
    fi
done
ctrpad_frame_count=$(sed -n 's/^frame_count=//p' "${ctrpad_report_dir}/metadata.txt")
ctrpad_checkpoint_count=$(sed -n 's/^checkpoint_count=//p' "${ctrpad_report_dir}/metadata.txt")
if [ "${ctrpad_frame_count}" != "${ctrpad_expected_frame_count}" ] ||
    [ "${ctrpad_checkpoint_count}" != "${ctrpad_expected_checkpoint_count}" ]; then
    echo "Golden report count mismatch: frames=${ctrpad_frame_count}/${ctrpad_expected_frame_count} checkpoints=${ctrpad_checkpoint_count}/${ctrpad_expected_checkpoint_count}." >&2
    exit 1
fi

if [ "${ctrpad_require_coverage}" -eq 1 ]; then
    for ctrpad_coverage_key in \
        startup_and_title \
        menu_and_race_load \
        steering_and_acceleration \
        powerslide_and_boost \
        item_acquired_and_used \
        lap_advanced \
        save_action \
        persisted_result_loaded
    do
        if ! grep -q "^${ctrpad_coverage_key}=pass$" "${ctrpad_report_dir}/coverage.txt"; then
            echo "Golden coverage is incomplete: ${ctrpad_coverage_key} is not pass." >&2
            exit 1
        fi
    done
fi
grep -q "\\[CTR Gameplay\\] player powerslide boost:" "${ctrpad_report_dir}/ctr-native.log"

ctrpad_replay_path="/out/${ctrpad_report_relative}/input.ctrreplay"
ctrpad_container_report_dir="/out/${ctrpad_report_relative}"
ctrpad_source_build_id=$(printf '%s' "${ctrpad_expected_source_commit}" | cut -c 1-12)
if ! grep -q "^build_id=${ctrpad_source_build_id}$" "${ctrpad_report_dir}/metadata.txt"; then
    echo "Golden report build_id does not match expected source ${ctrpad_source_build_id}." >&2
    exit 1
fi
ctrpad_binary_version=$(docker run --rm \
    --platform linux/amd64 \
    --volume "${ctrpad_build_dir}:/out:ro" \
    "${ctrpad_container_image}" \
    "/out/${ctrpad_binary_relative}" --version)
case "${ctrpad_binary_version}" in
    *"(${ctrpad_source_build_id})")
        ;;
    *)
        echo "The i686 binary does not match source commit ${ctrpad_source_build_id}:" >&2
        echo "${ctrpad_binary_version}" >&2
        exit 1
        ;;
esac

mkdir -p "${ctrpad_build_dir}/mesa-cache"

if [ "${ctrpad_finalize_only}" -eq 0 ]; then
    printf "running\n" > "${ctrpad_report_dir}/container-exit-status.txt"
    docker run --rm \
        --platform linux/amd64 \
        --user "${ctrpad_host_uid}:${ctrpad_host_gid}" \
        --env DISPLAY=:99 \
        --env SDL_AUDIODRIVER=dummy \
        --env MESA_SHADER_CACHE_DIR=/out/mesa-cache \
        --env MESA_SHADER_CACHE_MAX_SIZE=64M \
        --env XDG_RUNTIME_DIR=/tmp/ctrpad-runtime \
        --env "CTRPAD_BINARY_RELATIVE=${ctrpad_binary_relative}" \
        --env "CTRPAD_ALT_LOADER_RELATIVE=${ctrpad_alt_loader_relative}" \
        --env "CTRPAD_REPLAY_PATH=${ctrpad_replay_path}" \
        --env "CTRPAD_REPORT_DIR=${ctrpad_container_report_dir}" \
        --env "CTRPAD_MUTATION_FRAME=${ctrpad_mutation_frame}" \
        --env "CTRPAD_EXPECTED_FRAME_COUNT=${ctrpad_expected_frame_count}" \
        --volume "${ctrpad_build_dir}:/out" \
        --volume "${ctrpad_disc_image}:/out/assets/ctr-u.bin:ro" \
        "${ctrpad_container_image}" \
        sh -euxc '
        mkdir -p /tmp/.X11-unix /tmp/ctrpad-runtime
        chmod 700 /tmp/ctrpad-runtime
        Xvfb :99 -screen 0 1280x720x24 -nolisten tcp >/tmp/ctrpad-xvfb.log 2>&1 &
        ctrpad_xvfb_pid=$!
        trap "kill ${ctrpad_xvfb_pid} 2>/dev/null || true" EXIT

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

        "/out/${CTRPAD_BINARY_RELATIVE}" --replay "${CTRPAD_REPLAY_PATH}" \
            >"${CTRPAD_REPORT_DIR}/playback-1.log" 2>&1
        if [ -n "${CTRPAD_ALT_LOADER_RELATIVE}" ]; then
            "/out/${CTRPAD_ALT_LOADER_RELATIVE}" "/out/${CTRPAD_BINARY_RELATIVE}" \
                --replay "${CTRPAD_REPLAY_PATH}" \
                >"${CTRPAD_REPORT_DIR}/playback-2.log" 2>&1
        else
            "/out/${CTRPAD_BINARY_RELATIVE}" --replay "${CTRPAD_REPLAY_PATH}" \
                >"${CTRPAD_REPORT_DIR}/playback-2.log" 2>&1
        fi

        grep -q "\\[CTR Replay\\] replay finished after ${CTRPAD_EXPECTED_FRAME_COUNT} frames$" "${CTRPAD_REPORT_DIR}/playback-1.log"
        grep -q "\\[CTR Replay\\] replay finished after ${CTRPAD_EXPECTED_FRAME_COUNT} frames$" "${CTRPAD_REPORT_DIR}/playback-2.log"
        ctrpad_raw_checkpoint_1=$(
            sed -n "s/^\\[CTR State\\] raw checkpoint comparison .* restored-process=\\([^ ]*\\) .*/\\1/p" \
                "${CTRPAD_REPORT_DIR}/playback-1.log" | head -n 1
        )
        ctrpad_raw_checkpoint_2=$(
            sed -n "s/^\\[CTR State\\] raw checkpoint comparison .* restored-process=\\([^ ]*\\) .*/\\1/p" \
                "${CTRPAD_REPORT_DIR}/playback-2.log" | head -n 1
        )
        if [ -z "${ctrpad_raw_checkpoint_1}" ] || [ -z "${ctrpad_raw_checkpoint_2}" ]; then
            echo "Playback logs do not contain restored raw-checkpoint checksums." >&2
            exit 1
        fi
        if [ "${ctrpad_raw_checkpoint_1}" = "${ctrpad_raw_checkpoint_2}" ]; then
            echo "Restored raw-checkpoint checksums did not change across processes." >&2
            exit 1
        fi
        if ! grep -q "\\[CTR State\\] raw checkpoint comparison .* equal=no" "${CTRPAD_REPORT_DIR}/playback-1.log" &&
            ! grep -q "\\[CTR State\\] raw checkpoint comparison .* equal=no" "${CTRPAD_REPORT_DIR}/playback-2.log"; then
            echo "Neither restored raw checkpoint differs from the recording." >&2
            exit 1
        fi

        ctrpad_address_1=$(grep -m 1 "\\[CTR Replay\\] playback host-address sample" "${CTRPAD_REPORT_DIR}/playback-1.log")
        ctrpad_address_2=$(grep -m 1 "\\[CTR Replay\\] playback host-address sample" "${CTRPAD_REPORT_DIR}/playback-2.log")
        if [ "${ctrpad_address_1}" = "${ctrpad_address_2}" ]; then
            echo "Playback host-address samples did not change across processes." >&2
            exit 1
        fi

        ctrpad_mutation_frame=${CTRPAD_MUTATION_FRAME}
        if [ "${ctrpad_mutation_frame}" = auto ]; then
            ctrpad_mutation_frame=$(
                sed -n "s/^\\[CTR Replay\\] race driver\\[0\\] became active at replay frame \\([0-9][0-9]*\\)$/\\1/p" \
                    "${CTRPAD_REPORT_DIR}/playback-1.log" | head -n 1
            )
            if [ -z "${ctrpad_mutation_frame}" ]; then
                echo "No active race driver[0] frame was found in unchanged playback." >&2
                exit 1
            fi
        fi
        printf "%s\n" "${ctrpad_mutation_frame}" >"${CTRPAD_REPORT_DIR}/mutation-frame.txt"

        set +e
        "/out/${CTRPAD_BINARY_RELATIVE}" --replay "${CTRPAD_REPLAY_PATH}" \
            --replay-test-perturb-driver-x "${ctrpad_mutation_frame}" \
            >"${CTRPAD_REPORT_DIR}/playback-mutated.log" 2>&1
        ctrpad_mutation_status=$?
        set -e
        if [ "${ctrpad_mutation_status}" -ne 2 ]; then
            cat "${CTRPAD_REPORT_DIR}/playback-mutated.log" >&2
            echo "Mutated playback exited ${ctrpad_mutation_status}; expected 2." >&2
            exit 1
        fi
        grep -q "\\[CTR Replay\\] divergence at replay frame ${ctrpad_mutation_frame}$" \
            "${CTRPAD_REPORT_DIR}/playback-mutated.log"
        grep -q "\\[CTR Replay\\] first canonical state difference: drivers " \
            "${CTRPAD_REPORT_DIR}/playback-mutated.log"
    '
    printf "0\n" > "${ctrpad_report_dir}/container-exit-status.txt"
fi

if [ ! -f "${ctrpad_report_dir}/container-exit-status.txt" ]; then
    echo "Playback evidence is incomplete: missing ${ctrpad_report_dir}/container-exit-status.txt" >&2
    exit 1
fi
ctrpad_container_exit_status=$(
    tr -d '[:space:]' < "${ctrpad_report_dir}/container-exit-status.txt"
)
if [ "${ctrpad_container_exit_status}" != "0" ]; then
    echo "Playback container does not have a captured successful exit status: ${ctrpad_container_exit_status:-missing}" >&2
    exit 1
fi

for ctrpad_playback_log in playback-1.log playback-2.log playback-mutated.log; do
    if [ ! -f "${ctrpad_report_dir}/${ctrpad_playback_log}" ]; then
        echo "Playback evidence is incomplete: missing ${ctrpad_report_dir}/${ctrpad_playback_log}" >&2
        exit 1
    fi
done

grep -q "\\[CTR Replay\\] replay finished after ${ctrpad_expected_frame_count} frames$" "${ctrpad_report_dir}/playback-1.log"
grep -q "\\[CTR Replay\\] replay finished after ${ctrpad_expected_frame_count} frames$" "${ctrpad_report_dir}/playback-2.log"

ctrpad_raw_checkpoint_1=$(
    sed -n 's/^\[CTR State\] raw checkpoint comparison .* restored-process=\([^ ]*\) .*/\1/p' \
        "${ctrpad_report_dir}/playback-1.log" | head -n 1
)
ctrpad_raw_checkpoint_2=$(
    sed -n 's/^\[CTR State\] raw checkpoint comparison .* restored-process=\([^ ]*\) .*/\1/p' \
        "${ctrpad_report_dir}/playback-2.log" | head -n 1
)
if [ -z "${ctrpad_raw_checkpoint_1}" ] || [ -z "${ctrpad_raw_checkpoint_2}" ]; then
    echo "Playback logs do not contain restored raw-checkpoint checksums." >&2
    exit 1
fi
if [ "${ctrpad_raw_checkpoint_1}" = "${ctrpad_raw_checkpoint_2}" ]; then
    echo "Restored raw-checkpoint checksums did not change across processes." >&2
    exit 1
fi
if ! grep -q "\\[CTR State\\] raw checkpoint comparison .* equal=no" "${ctrpad_report_dir}/playback-1.log" &&
    ! grep -q "\\[CTR State\\] raw checkpoint comparison .* equal=no" "${ctrpad_report_dir}/playback-2.log"; then
    echo "Neither restored raw checkpoint differs from the recording." >&2
    exit 1
fi

ctrpad_address_1=$(grep -m 1 "\\[CTR Replay\\] playback host-address sample" "${ctrpad_report_dir}/playback-1.log")
ctrpad_address_2=$(grep -m 1 "\\[CTR Replay\\] playback host-address sample" "${ctrpad_report_dir}/playback-2.log")
if [ "${ctrpad_address_1}" = "${ctrpad_address_2}" ]; then
    echo "Playback host-address samples did not change across processes." >&2
    exit 1
fi

if [ ! -f "${ctrpad_report_dir}/mutation-frame.txt" ]; then
    echo "Playback evidence is incomplete: missing ${ctrpad_report_dir}/mutation-frame.txt" >&2
    exit 1
fi
ctrpad_recorded_mutation_frame=$(tr -d '[:space:]' < "${ctrpad_report_dir}/mutation-frame.txt")
case "${ctrpad_recorded_mutation_frame}" in
    ""|*[!0-9]*)
        echo "Playback evidence has an invalid or missing mutation frame." >&2
        exit 1
        ;;
esac
if [ "${ctrpad_mutation_frame}" = "auto" ]; then
    ctrpad_expected_mutation_frame=$(
        sed -n 's/^\[CTR Replay\] race driver\[0\] became active at replay frame \([0-9][0-9]*\)$/\1/p' \
            "${ctrpad_report_dir}/playback-1.log" | head -n 1
    )
else
    ctrpad_expected_mutation_frame=${ctrpad_mutation_frame}
fi
if [ -z "${ctrpad_expected_mutation_frame}" ] ||
    [ "${ctrpad_recorded_mutation_frame}" != "${ctrpad_expected_mutation_frame}" ]; then
    echo "Mutation frame does not match the selected unchanged-playback frame." >&2
    exit 1
fi
grep -q "\\[CTR Replay\\] divergence at replay frame ${ctrpad_recorded_mutation_frame}$" \
    "${ctrpad_report_dir}/playback-mutated.log"
grep -q "\\[CTR Replay\\] first canonical state difference: drivers " \
    "${ctrpad_report_dir}/playback-mutated.log"

if command -v sha256sum >/dev/null 2>&1; then
    ctrpad_disc_hash=$(sha256sum "${ctrpad_disc_image}" | awk '{print $1}')
    ctrpad_binary_hash=$(sha256sum "${ctrpad_binary_path}" | awk '{print $1}')
    if [ -n "${ctrpad_alt_loader_path}" ]; then
        ctrpad_alt_loader_hash=$(sha256sum "${ctrpad_alt_loader_path}" | awk '{print $1}')
    else
        ctrpad_alt_loader_hash=none
    fi
    printf "%s  ctr-u.bin\n" "${ctrpad_disc_hash}" > "${ctrpad_report_dir}/disc.sha256"
else
    ctrpad_disc_hash=$(shasum -a 256 "${ctrpad_disc_image}" | awk '{print $1}')
    ctrpad_binary_hash=$(shasum -a 256 "${ctrpad_binary_path}" | awk '{print $1}')
    if [ -n "${ctrpad_alt_loader_path}" ]; then
        ctrpad_alt_loader_hash=$(shasum -a 256 "${ctrpad_alt_loader_path}" | awk '{print $1}')
    else
        ctrpad_alt_loader_hash=none
    fi
    printf "%s  ctr-u.bin\n" "${ctrpad_disc_hash}" > "${ctrpad_report_dir}/disc.sha256"
fi

{
    echo "source_commit=${ctrpad_expected_source_commit}"
    echo "coverage_requirement=${ctrpad_require_coverage}"
    echo "expected_frame_count=${ctrpad_expected_frame_count}"
    echo "expected_checkpoint_count=${ctrpad_expected_checkpoint_count}"
    echo "playback_container_exit_status=${ctrpad_container_exit_status}"
    echo "binary_version=${ctrpad_binary_version}"
    echo "binary_sha256=${ctrpad_binary_hash}"
    echo "alternate_loader=${ctrpad_alt_loader_relative:-none}"
    echo "alternate_loader_sha256=${ctrpad_alt_loader_hash}"
    echo "container_image_id=$(docker image inspect --format '{{.Id}}' "${ctrpad_container_image}")"
    sed 's/^/toolchain=/' "${ctrpad_toolchain_manifest}"
} > "${ctrpad_report_dir}/environment.txt"

(
    cd "${ctrpad_report_dir}/memcard.seed"
    find . -type f -print | LC_ALL=C sort | while IFS= read -r ctrpad_memcard_file; do
        shasum -a 256 "${ctrpad_memcard_file}"
    done
) > "${ctrpad_report_dir}/memcard-seed.sha256"

(
    cd "${ctrpad_report_dir}/memcard.recording"
    find . -type f -print | LC_ALL=C sort | while IFS= read -r ctrpad_memcard_file; do
        shasum -a 256 "${ctrpad_memcard_file}"
    done
) > "${ctrpad_report_dir}/memcard-recording.sha256"

if command -v sha256sum >/dev/null 2>&1; then
    (
        cd "${ctrpad_report_dir}"
        sha256sum input.ctrreplay state.ctrstates metadata.txt ctr-native.log disc.sha256 environment.txt container-exit-status.txt mutation-frame.txt \
            memcard-seed.sha256 memcard-recording.sha256 playback-1.log playback-2.log playback-mutated.log
        if [ "${ctrpad_require_coverage}" -eq 1 ]; then
            sha256sum coverage.txt
        fi
    ) > "${ctrpad_report_dir}/evidence.sha256"
else
    (
        cd "${ctrpad_report_dir}"
        shasum -a 256 input.ctrreplay state.ctrstates metadata.txt ctr-native.log disc.sha256 environment.txt container-exit-status.txt mutation-frame.txt \
            memcard-seed.sha256 memcard-recording.sha256 playback-1.log playback-2.log playback-mutated.log
        if [ "${ctrpad_require_coverage}" -eq 1 ]; then
            shasum -a 256 coverage.txt
        fi
    ) > "${ctrpad_report_dir}/evidence.sha256"
fi

if [ "${ctrpad_require_coverage}" -eq 1 ]; then
    echo "Golden replay verification passed."
else
    echo "Replay process-determinism and mutation verification passed."
fi
echo "Evidence: ${ctrpad_report_dir}"
