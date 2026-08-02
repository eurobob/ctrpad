#!/usr/bin/env bash
# Verify that an Apple bundle carries one clean, reproducible source identity.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./tools/verify-ios-build-identity.sh \
  --info-plist PATH [--expected-source-commit HEX] --output PATH

The bundle must declare a full 40-character CTRNativeSourceCommit and a clean
CTRNativeBuildIdentity equal to its 12-character prefix. Dirty, unknown,
missing, malformed, truncated, or mismatched identities are rejected before a
verification manifest is written.
EOF
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

requested_info_plist=""
requested_output_path=""
expected_source_commit=""
while (($#)); do
    case "$1" in
        --info-plist|--expected-source-commit|--output)
            (($# >= 2)) || fail "$1 requires a value"
            case "$1" in
                --info-plist) requested_info_plist="$2" ;;
                --expected-source-commit) expected_source_commit="$2" ;;
                --output) requested_output_path="$2" ;;
            esac
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *) fail "unknown option: $1" ;;
    esac
done

for command_name in awk basename dirname git plutil shasum tr uname; do
    require_command "$command_name"
done
[[ "$(uname -s)" == "Darwin" ]] || fail "iOS build-identity verification requires macOS"
[[ -f "$requested_info_plist" ]] || fail "Info.plist not found: $requested_info_plist"
[[ -n "$requested_output_path" ]] || fail "--output is required"

info_parent="$(cd "$(dirname "$requested_info_plist")" && pwd -P)"
info_plist="$info_parent/$(basename "$requested_info_plist")"
output_parent="$(dirname "$requested_output_path")"
[[ -d "$output_parent" ]] || fail "output parent directory not found: $output_parent"
output_parent="$(cd "$output_parent" && pwd -P)"
output_path="$output_parent/$(basename "$requested_output_path")"
[[ ! -e "$output_path" ]] || fail "output path already exists: $output_path"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(cd "$script_dir/.." && pwd -P)"
case "$output_path" in
    "$repo_root"|"$repo_root"/*)
        git -C "$repo_root" check-ignore -q -- "$output_path" || \
            fail "in-repository build-identity output must be gitignored: $output_path"
        ;;
esac

plutil -p "$info_plist" >/dev/null
source_commit="$(plutil -extract CTRNativeSourceCommit raw -o - \
    "$info_plist" 2>/dev/null)" || fail "Info.plist has no CTRNativeSourceCommit"
build_identity="$(plutil -extract CTRNativeBuildIdentity raw -o - \
    "$info_plist" 2>/dev/null)" || fail "Info.plist has no CTRNativeBuildIdentity"
bundle_id="$(plutil -extract CFBundleIdentifier raw -o - "$info_plist" 2>/dev/null)" || \
    fail "Info.plist has no CFBundleIdentifier"
bundle_version="$(plutil -extract CFBundleShortVersionString raw -o - \
    "$info_plist" 2>/dev/null)" || fail "Info.plist has no CFBundleShortVersionString"
build_version="$(plutil -extract CFBundleVersion raw -o - "$info_plist" 2>/dev/null)" || \
    fail "Info.plist has no CFBundleVersion"

[[ "$source_commit" =~ ^[0-9a-f]{40}$ ]] || \
    fail "CTRNativeSourceCommit is not 40 lowercase hexadecimal characters: $source_commit"
source_commit_short="${source_commit:0:12}"
[[ "$build_identity" == "$source_commit_short" ]] || \
    fail "CTRNativeBuildIdentity $build_identity is not the clean source prefix $source_commit_short"
[[ "$bundle_id" =~ ^[A-Za-z0-9.-]+$ ]] || fail "bundle ID is malformed: $bundle_id"
[[ -n "$bundle_version" ]] || fail "bundle version is empty"
[[ -n "$build_version" ]] || fail "build version is empty"

expected_status="not-requested"
if [[ -n "$expected_source_commit" ]]; then
    expected_source_commit="$(printf '%s' "$expected_source_commit" | tr '[:upper:]' '[:lower:]')"
    [[ "$expected_source_commit" =~ ^[0-9a-f]{40}$ ]] || \
        fail "expected source commit must contain exactly 40 hexadecimal characters"
    [[ "$source_commit" == "$expected_source_commit" ]] || \
        fail "bundle source commit $source_commit does not equal expected $expected_source_commit"
    expected_status="verified"
fi

info_sha256="$(shasum -a 256 "$info_plist" | awk '{print $1}')"
{
    printf 'TYPE=ios-build-identity-verification\n'
    printf 'VALIDATION_STATUS=verified\n'
    printf 'SOURCE_COMMIT=%s\n' "$source_commit"
    printf 'BUILD_IDENTITY=%s\n' "$build_identity"
    printf 'EXPECTED_SOURCE_COMMIT_STATUS=%s\n' "$expected_status"
    printf 'BUNDLE_ID=%s\n' "$bundle_id"
    printf 'VERSION=%s\n' "$bundle_version"
    printf 'BUILD=%s\n' "$build_version"
    printf 'INFO_PLIST_SHA256=%s\n' "$info_sha256"
} >"$output_path"

printf 'IOS_BUILD_IDENTITY_VERIFIED=%s\n' "$output_path"
