#!/usr/bin/env bash
# Safely update-install an exact CTRPad build on one booted iOS Simulator.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./tools/install-ios-simulator.sh [options]

Options:
  --app PATH              Input Simulator .app
                          (default: build-ios-simulator-arm64/CTRPad.app).
  --device UDID           Expected booted Simulator (default: the only one).
  --launch                Launch the installed app after verification.
  --verify-persistence    Prove the existing retail image and slot-zero save
                          retain inode, size, and SHA-256 across the update.
  -h, --help              Show this help.

The script refuses to run unless exactly one Simulator is booted. It copies
the app to an isolated temporary directory, applies an ad-hoc Simulator-only
signature, verifies that signature strictly, update-installs by bundle ID, and
requires the installed executable SHA-256 to equal the staged executable.
It never modifies the source bundle and does not produce a device-signed app.
EOF
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

file_fingerprint() {
    local path="$1"
    local inode_and_size hash
    [[ -f "$path" ]] || fail "persistence file not found: $path"
    inode_and_size="$(stat -f '%i|%z' "$path")"
    hash="$(shasum -a 256 "$path" | awk '{print $1}')"
    [[ "$hash" =~ ^[0-9a-f]{64}$ ]] || fail "could not hash persistence file: $path"
    printf '%s|%s' "$inode_and_size" "$hash"
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
app_path="$repo_root/build-ios-simulator-arm64/CTRPad.app"
device_udid=""
launch_after=0
verify_persistence=0

while (($#)); do
    case "$1" in
        --app|--device)
            (($# >= 2)) || fail "$1 requires a value"
            case "$1" in
                --app) app_path="$2" ;;
                --device) device_udid="$2" ;;
            esac
            shift 2
            ;;
        --launch)
            launch_after=1
            shift
            ;;
        --verify-persistence)
            verify_persistence=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            fail "unknown option: $1"
            ;;
    esac
done

for command_name in awk basename codesign dirname ditto file find grep lipo \
    mktemp plutil rm sed shasum stat tr uname xcrun; do
    require_command "$command_name"
done

[[ "$(uname -s)" == "Darwin" ]] || fail "iOS Simulator installation requires macOS"
[[ -d "$app_path" ]] || fail "input app bundle not found: $app_path"
app_parent="$(cd "$(dirname "$app_path")" && pwd -P)"
app_path="$app_parent/$(basename "$app_path")"
[[ -f "$app_path/Info.plist" ]] || fail "input app has no Info.plist: $app_path"

booted_devices="$(xcrun simctl list devices booted)"
booted_udids="$(printf '%s\n' "$booted_devices" | \
    sed -nE 's/.*\(([0-9A-Fa-f-]{36})\) \(Booted\).*/\1/p')"
booted_count="$(printf '%s\n' "$booted_udids" | awk 'NF { count += 1 } END { print count + 0 }')"
[[ "$booted_count" == "1" ]] || {
    printf '%s\n' "$booted_devices" >&2
    fail "expected exactly one booted Simulator; found $booted_count"
}
only_booted_udid="$(printf '%s\n' "$booted_udids" | awk 'NF { print; exit }')"

if [[ -n "$device_udid" ]]; then
    [[ "$device_udid" =~ ^[0-9A-Fa-f-]{36}$ ]] || fail "invalid Simulator UDID: $device_udid"
    requested_lower="$(printf '%s' "$device_udid" | tr '[:upper:]' '[:lower:]')"
    booted_lower="$(printf '%s' "$only_booted_udid" | tr '[:upper:]' '[:lower:]')"
    [[ "$requested_lower" == "$booted_lower" ]] || \
        fail "requested Simulator $device_udid is not the only booted Simulator $only_booted_udid"
else
    device_udid="$only_booted_udid"
fi

info_plist="$app_path/Info.plist"
bundle_id="$(plutil -extract CFBundleIdentifier raw -o - "$info_plist")"
executable_name="$(plutil -extract CFBundleExecutable raw -o - "$info_plist")"
package_type="$(plutil -extract CFBundlePackageType raw -o - "$info_plist")"
source_executable="$app_path/$executable_name"

[[ "$package_type" == "APPL" ]] || fail "unexpected CFBundlePackageType: $package_type"
[[ "$bundle_id" =~ ^[A-Za-z0-9.-]+$ ]] || fail "unsupported bundle ID: $bundle_id"
[[ -x "$source_executable" ]] || fail "bundle executable is missing or not executable: $source_executable"
architectures="$(lipo -archs "$source_executable")"
[[ "$architectures" == "arm64" ]] || fail "Simulator executable must be thin arm64; got: $architectures"
xcrun vtool -show-build "$source_executable" | grep -Eq '^[[:space:]]*platform IOSSIMULATOR$' || \
    fail "executable has no iOS Simulator LC_BUILD_VERSION"
file "$source_executable" | grep -q 'Mach-O 64-bit executable arm64' || \
    fail "Simulator executable is not an ARM64 Mach-O"

retail_match="$(find "$app_path" -type f \( \
    -iname 'ctr-u.bin' -o -iname '*.bin' -o -iname '*.img' -o \
    -iname '*.iso' -o -iname '*.cue' -o -iname '*.ccd' -o \
    -iname '*.sub' -o -iname '*.big' -o -iname '*.hwl' -o \
    -iname '*.xa' -o -iname '*.str' \
\) -print -quit)"
[[ -z "$retail_match" ]] || fail "retail-like file found in app bundle: $retail_match"
runtime_match="$(find "$app_path" -type d \( \
    -name Documents -o -name memcards -o -name 'Application Support' \
\) -print -quit)"
[[ -z "$runtime_match" ]] || fail "runtime data directory found in app bundle: $runtime_match"

tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/ctrpad-simulator-install.XXXXXX")"
cleanup() {
    case "$tmp_dir" in
        "${TMPDIR:-/tmp}"/ctrpad-simulator-install.*) rm -rf "$tmp_dir" ;;
        *) printf 'WARNING: refusing to remove unexpected temporary path: %s\n' "$tmp_dir" >&2 ;;
    esac
}
trap cleanup EXIT INT TERM

staged_app="$tmp_dir/CTRPad.app"
ditto --norsrc --noextattr --noqtn --noacl "$app_path" "$staged_app"
case "$staged_app" in
    "$tmp_dir"/*) ;;
    *) fail "staged app escaped the temporary directory" ;;
esac

staged_executable="$staged_app/$executable_name"
source_hash="$(shasum -a 256 "$source_executable" | awk '{print $1}')"
codesign --force --deep --sign - --timestamp=none "$staged_app"
codesign --verify --deep --strict --verbose=2 "$staged_app"
staged_hash="$(shasum -a 256 "$staged_executable" | awk '{print $1}')"

retail_relative='Documents/CTRPad/assets/ctr-u.bin'
save_relative='Library/Application Support/chrissotraidis/CTRPad/memcards/slot0/BASCUS-94426-SLOTS'
retail_before=""
save_before=""
if ((verify_persistence)); then
    data_before="$(xcrun simctl get_app_container "$device_udid" "$bundle_id" data)" || \
        fail "cannot verify persistence because $bundle_id is not already installed"
    retail_before="$(file_fingerprint "$data_before/$retail_relative")"
    save_before="$(file_fingerprint "$data_before/$save_relative")"
    printf 'Persistence before: retail=%s save=%s\n' "$retail_before" "$save_before"
fi

printf 'Installing isolated signed copy on %s...\n' "$device_udid"
xcrun simctl install "$device_udid" "$staged_app"

installed_app="$(xcrun simctl get_app_container "$device_udid" "$bundle_id" app)"
installed_executable="$installed_app/$executable_name"
[[ -f "$installed_executable" ]] || fail "installed executable not found: $installed_executable"
installed_hash="$(shasum -a 256 "$installed_executable" | awk '{print $1}')"
[[ "$installed_hash" == "$staged_hash" ]] || \
    fail "installed executable hash $installed_hash does not match staged hash $staged_hash"
codesign --verify --deep --strict --verbose=2 "$installed_app"

if ((verify_persistence)); then
    data_after="$(xcrun simctl get_app_container "$device_udid" "$bundle_id" data)"
    retail_after="$(file_fingerprint "$data_after/$retail_relative")"
    save_after="$(file_fingerprint "$data_after/$save_relative")"
    [[ "$retail_after" == "$retail_before" ]] || \
        fail "retail image changed across update: before=$retail_before after=$retail_after"
    [[ "$save_after" == "$save_before" ]] || \
        fail "slot-zero save changed across update: before=$save_before after=$save_after"
    printf 'Persistence after:  retail=%s save=%s\n' "$retail_after" "$save_after"
fi

launch_result=""
if ((launch_after)); then
    launch_result="$(xcrun simctl launch --terminate-running-process "$device_udid" "$bundle_id")"
fi

printf 'SIMULATOR_UDID=%s\n' "$device_udid"
printf 'BUNDLE_ID=%s\n' "$bundle_id"
printf 'SOURCE_EXECUTABLE_SHA256=%s\n' "$source_hash"
printf 'SIGNED_STAGED_EXECUTABLE_SHA256=%s\n' "$staged_hash"
printf 'INSTALLED_EXECUTABLE_SHA256=%s\n' "$installed_hash"
printf 'INSTALLED_APP=%s\n' "$installed_app"
if ((verify_persistence)); then
    printf 'PERSISTENCE_VERIFIED=retail-and-slot-zero\n'
fi
if ((launch_after)); then
    printf 'LAUNCH_RESULT=%s\n' "$launch_result"
fi
