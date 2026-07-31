#!/usr/bin/env bash
# Build, validate, optionally sign, and package CTRPad for iOS/iPadOS sideloading.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./package-ios.sh [options]

Options:
  --build                 Configure and build the ios-device-arm64 preset first.
  --app PATH              Input .app (default: build-ios-device-arm64/CTRPad.app).
  --output PATH           Output .ipa (default: dist/CTRPad-<version>-<mode>.ipa).
  --bundle-id ID          Expected bundle ID; passed to CMake with --build.
  --identity IDENTITY     Apple Development/Distribution codesign identity.
  --profile PATH          Matching .mobileprovision file.
  --device UDID           Require a signed profile to contain this device UDID.
  -h, --help              Show this help.

Signing requires both --identity and --profile. Without them, the script emits
an unsigned IPA intended for a compatible user-side re-signing tool. It never
copies a private key, retail disc image, save, or local runtime container.
Set SOURCE_DATE_EPOCH to override the archive timestamp; otherwise the current
source commit time is used, with 2000-01-01 UTC as a non-Git fallback.
EOF
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$script_dir"
app_path="$repo_root/build-ios-device-arm64/CTRPad.app"
output_path=""
expected_bundle_id=""
signing_identity=""
profile_path=""
device_udid=""
build_first=0

while (($#)); do
    case "$1" in
        --build)
            build_first=1
            shift
            ;;
        --app|--output|--bundle-id|--identity|--profile|--device)
            (($# >= 2)) || fail "$1 requires a value"
            case "$1" in
                --app) app_path="$2" ;;
                --output) output_path="$2" ;;
                --bundle-id) expected_bundle_id="$2" ;;
                --identity) signing_identity="$2" ;;
                --profile) profile_path="$2" ;;
                --device) device_udid="$2" ;;
            esac
            shift 2
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

for command_name in cmake ditto file find grep lipo plutil security shasum strings unzip xcrun; do
    require_command "$command_name"
done
plist_buddy='/usr/libexec/PlistBuddy'
[[ -x "$plist_buddy" ]] || fail "required tool not found: $plist_buddy"

if [[ -n "$signing_identity" || -n "$profile_path" ]]; then
    [[ -n "$signing_identity" && -n "$profile_path" ]] || \
        fail "--identity and --profile must be supplied together"
    require_command codesign
    [[ -f "$profile_path" ]] || fail "provisioning profile not found: $profile_path"
fi

if [[ -n "$device_udid" && -z "$profile_path" ]]; then
    fail "--device requires --identity and --profile"
fi

if ((build_first)); then
    configure_args=(--preset ios-device-arm64)
    if [[ -n "$expected_bundle_id" ]]; then
        configure_args+=("-DCTR_NATIVE_IOS_BUNDLE_IDENTIFIER=$expected_bundle_id")
    fi
    cmake "${configure_args[@]}"
    cmake --build --preset ios-device-arm64
fi

[[ -d "$app_path" ]] || fail "input app bundle not found: $app_path"
[[ -f "$app_path/Info.plist" ]] || fail "input app has no Info.plist: $app_path"

tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/ctrpad-ios-package.XXXXXX")"
cleanup() {
    case "$tmp_dir" in
        "${TMPDIR:-/tmp}"/ctrpad-ios-package.*) rm -rf "$tmp_dir" ;;
        *) printf 'WARNING: refusing to remove unexpected temporary path: %s\n' "$tmp_dir" >&2 ;;
    esac
}
trap cleanup EXIT INT TERM

payload_dir="$tmp_dir/Payload"
staged_app="$payload_dir/CTRPad.app"
mkdir -p "$payload_dir"
ditto --norsrc --noextattr --noqtn --noacl "$app_path" "$staged_app"

case "$staged_app" in
    "$tmp_dir"/*) ;;
    *) fail "staged app escaped the temporary directory" ;;
esac
rm -rf "$staged_app/_CodeSignature"
rm -f "$staged_app/embedded.mobileprovision"

info_plist="$staged_app/Info.plist"
bundle_id="$(plutil -extract CFBundleIdentifier raw -o - "$info_plist")"
bundle_version="$(plutil -extract CFBundleShortVersionString raw -o - "$info_plist")"
build_version="$(plutil -extract CFBundleVersion raw -o - "$info_plist")"
executable_name="$(plutil -extract CFBundleExecutable raw -o - "$info_plist")"
package_type="$(plutil -extract CFBundlePackageType raw -o - "$info_plist")"
minimum_os="$(plutil -extract MinimumOSVersion raw -o - "$info_plist")"
executable_path="$staged_app/$executable_name"

[[ "$package_type" == "APPL" ]] || fail "unexpected CFBundlePackageType: $package_type"
[[ "$bundle_id" =~ ^[A-Za-z0-9.-]+$ ]] || fail "bundle ID contains unsupported characters: $bundle_id"
[[ -f "$executable_path" ]] || fail "bundle executable not found: $executable_path"
[[ -z "$expected_bundle_id" || "$bundle_id" == "$expected_bundle_id" ]] || \
    fail "bundle ID $bundle_id does not match requested $expected_bundle_id"

architectures="$(lipo -archs "$executable_path")"
[[ "$architectures" == "arm64" ]] || fail "device executable must be thin arm64; got: $architectures"
xcrun vtool -show-build "$executable_path" | grep -Eq '^[[:space:]]*platform IOS$' || \
    fail "device executable has no iOS LC_BUILD_VERSION"
file "$executable_path" | grep -q 'Mach-O 64-bit executable arm64' || \
    fail "device executable is not an ARM64 Mach-O"

for required_resource in LICENSE THIRD_PARTY_NOTICES.md INSTALL-IOS.md; do
    [[ -n "$(find "$staged_app" -type f -name "$required_resource" -print -quit)" ]] || \
        fail "bundle is missing distribution resource: $required_resource"
done

retail_match="$(find "$staged_app" -type f \( \
    -iname 'ctr-u.bin' -o -iname '*.bin' -o -iname '*.img' -o \
    -iname '*.iso' -o -iname '*.cue' -o -iname '*.ccd' -o \
    -iname '*.sub' -o -iname '*.big' -o -iname '*.hwl' -o \
    -iname '*.xa' -o -iname '*.str' \
\) -print -quit)"
[[ -z "$retail_match" ]] || fail "retail-like file found in bundle: $retail_match"

runtime_match="$(find "$staged_app" -type d \( \
    -name Documents -o -name memcards -o -name 'Application Support' \
\) -print -quit)"
[[ -z "$runtime_match" ]] || fail "runtime data directory found in bundle: $runtime_match"

mode="unsigned"
if [[ -n "$signing_identity" ]]; then
    identity_rows="$(security find-identity -v -p codesigning)"
    grep -Fq "$signing_identity" <<<"$identity_rows" || \
        fail "codesign identity is not available in the current keychain"

    profile_plist="$tmp_dir/profile.plist"
    security cms -D -i "$profile_path" -o "$profile_plist"
    profile_platforms="$(plutil -extract Platform json -o - "$profile_plist")"
    grep -q 'iOS' <<<"$profile_platforms" || fail "profile is not valid for iOS"

    expiration="$(plutil -extract ExpirationDate raw -o - "$profile_plist")"
    expiration_epoch="$(date -j -u -f '%Y-%m-%dT%H:%M:%SZ' "$expiration" '+%s' 2>/dev/null || true)"
    [[ -n "$expiration_epoch" ]] || fail "could not parse provisioning profile expiration: $expiration"
    ((expiration_epoch > $(date -u '+%s'))) || fail "provisioning profile expired at $expiration"

    profile_application_id="$(plutil -extract Entitlements.application-identifier raw -o - "$profile_plist")"
    profile_suffix="${profile_application_id#*.}"
    if [[ "$profile_suffix" == *'*' ]]; then
        allowed_prefix="${profile_suffix%\*}"
        [[ "$bundle_id" == "$allowed_prefix"* ]] || \
            fail "profile application identifier $profile_application_id does not allow $bundle_id"
    else
        [[ "$profile_suffix" == "$bundle_id" ]] || \
            fail "profile application identifier $profile_application_id does not match $bundle_id"
    fi

    if [[ -n "$device_udid" ]]; then
        provisioned_devices="$(plutil -extract ProvisionedDevices json -o - "$profile_plist" 2>/dev/null || true)"
        grep -Fq "\"$device_udid\"" <<<"$provisioned_devices" || \
            fail "profile does not contain requested device UDID $device_udid"
    fi

    team_identifier="$(plutil -extract TeamIdentifier.0 raw -o - "$profile_plist")"
    application_prefix="$(plutil -extract ApplicationIdentifierPrefix.0 raw -o - "$profile_plist")"
    [[ "$team_identifier" =~ ^[A-Za-z0-9]+$ ]] || \
        fail "profile team identifier contains unsupported characters"
    [[ "$application_prefix" =~ ^[A-Za-z0-9]+\.$ ]] || \
        fail "profile application identifier prefix is malformed: $application_prefix"
    signed_application_id="${application_prefix}${bundle_id}"
    entitlements_plist="$tmp_dir/CTRPad.entitlements"
    plutil -create xml1 "$entitlements_plist"
    "$plist_buddy" -c "Add :application-identifier string $signed_application_id" "$entitlements_plist"
    "$plist_buddy" -c "Add :com.apple.developer.team-identifier string $team_identifier" "$entitlements_plist"
    "$plist_buddy" -c 'Add :keychain-access-groups array' "$entitlements_plist"
    "$plist_buddy" -c "Add :keychain-access-groups:0 string $signed_application_id" "$entitlements_plist"
    get_task_allow="$(plutil -extract Entitlements.get-task-allow raw -o - "$profile_plist" 2>/dev/null || true)"
    if [[ "$get_task_allow" == "true" || "$get_task_allow" == "false" ]]; then
        "$plist_buddy" -c "Add :get-task-allow bool $get_task_allow" "$entitlements_plist"
    fi

    ditto --norsrc --noextattr --noqtn --noacl "$profile_path" "$staged_app/embedded.mobileprovision"
    codesign --force --sign "$signing_identity" --entitlements "$entitlements_plist" \
        --generate-entitlement-der --timestamp=none "$staged_app"
    codesign --verify --deep --strict --verbose=2 "$staged_app"
    signed_entitlements="$tmp_dir/signed-entitlements.plist"
    codesign --display --entitlements - --xml "$staged_app" >"$signed_entitlements" 2>/dev/null
    actual_application_id="$(plutil -extract application-identifier raw -o - "$signed_entitlements")"
    [[ "$actual_application_id" == "$signed_application_id" ]] || \
        fail "signed application identifier mismatch: $actual_application_id"
    mode="signed"
fi

source_date_epoch="${SOURCE_DATE_EPOCH:-}"
if [[ -z "$source_date_epoch" ]] && command -v git >/dev/null 2>&1 && \
    git -C "$repo_root" rev-parse --verify HEAD >/dev/null 2>&1; then
    source_date_epoch="$(git -C "$repo_root" show -s --format=%ct HEAD)"
fi
source_date_epoch="${source_date_epoch:-946684800}"
[[ "$source_date_epoch" =~ ^[0-9]+$ ]] || \
    fail "SOURCE_DATE_EPOCH must be a non-negative integer"
((source_date_epoch >= 315532800)) || \
    fail "SOURCE_DATE_EPOCH must be representable by ZIP (1980-01-01 or later)"
archive_timestamp="$(date -r "$source_date_epoch" -u '+%Y%m%d%H%M.%S')"
find "$payload_dir" -exec touch -h -t "$archive_timestamp" {} +

if [[ -z "$output_path" ]]; then
    output_path="$repo_root/dist/CTRPad-${bundle_version}-${build_version}-${mode}.ipa"
elif [[ "$output_path" != /* ]]; then
    output_path="$repo_root/$output_path"
fi

output_dir="$(dirname "$output_path")"
mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"
output_path="$output_dir/$(basename "$output_path")"
[[ "$output_path" == *.ipa ]] || fail "output path must end in .ipa"
[[ ! -e "$output_path" && ! -e "$output_path.sha256" ]] || \
    fail "output already exists; choose a new --output path: $output_path"

archive_path="$tmp_dir/CTRPad.ipa"
(
    cd "$tmp_dir"
    ditto -c -k --norsrc --noextattr --noqtn --noacl --keepParent \
        --zlibCompressionLevel 9 Payload "$archive_path"
)

archive_retail_match="$(unzip -Z1 "$archive_path" | grep -Ei \
    '(^|/)(ctr-u\.bin|[^/]+\.(bin|img|iso|cue|ccd|sub|big|hwl|xa|str))$' | head -1 || true)"
[[ -z "$archive_retail_match" ]] || fail "retail-like file found in IPA: $archive_retail_match"
unzip -Z1 "$archive_path" | grep -q '^Payload/CTRPad\.app/Info\.plist$' || \
    fail "IPA does not contain the expected Payload/CTRPad.app"

mv "$archive_path" "$output_path"
shasum -a 256 "$output_path" >"$output_path.sha256"

printf 'Wrote %s\n' "$output_path"
printf 'Wrote %s\n' "$output_path.sha256"
printf 'Bundle: %s %s (%s), minimum iOS %s, %s, %s\n' \
    "$bundle_id" "$bundle_version" "$build_version" "$minimum_os" "$architectures" "$mode"
printf 'Retail media: excluded\n'
printf 'Distribution resources: LICENSE, THIRD_PARTY_NOTICES.md, INSTALL-IOS.md\n'
