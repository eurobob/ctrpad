#!/usr/bin/env bash
# Build the canonical CMake visionOS target inside an Xcode build, then place
# its unsigned app where Xcode expects the current target's product. Xcode
# performs development signing after this build phase, allowing Mesa's generic
# deployment bridge to install and launch the result.
set -euo pipefail

: "${SRCROOT:?Xcode did not provide SRCROOT}"
: "${TARGET_BUILD_DIR:?Xcode did not provide TARGET_BUILD_DIR}"
: "${WRAPPER_NAME:?Xcode did not provide WRAPPER_NAME}"

find_tool() {
    local tool="$1"
    local candidate

    if candidate="$(command -v "$tool" 2>/dev/null)"; then
        printf '%s\n' "$candidate"
        return 0
    fi

    for candidate in "/opt/homebrew/bin/$tool" "/usr/local/bin/$tool"; do
        if [ -x "$candidate" ]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    printf 'CTRPad requires %s on the Mac build agent.\n' "$tool" >&2
    return 1
}

cmake_command="$(find_tool cmake)"
ninja_command="$(find_tool ninja)"
native_app="$SRCROOT/build-visionos-device-arm64/CTRPad.app"
xcode_app="$TARGET_BUILD_DIR/$WRAPPER_NAME"

"$cmake_command" --preset visionos-device-arm64 \
    -DCMAKE_MAKE_PROGRAM="$ninja_command"
"$cmake_command" --build --preset visionos-device-arm64

if [ ! -d "$native_app" ]; then
    printf 'CMake completed without producing %s\n' "$native_app" >&2
    exit 1
fi

/bin/rm -rf "$xcode_app"
/usr/bin/ditto "$native_app" "$xcode_app"
