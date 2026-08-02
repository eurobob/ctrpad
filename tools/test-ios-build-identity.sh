#!/usr/bin/env bash
# Test clean source/build identity binding for Apple bundle metadata.

set -euo pipefail

fail() {
    printf 'SELF-TEST ERROR: %s\n' "$*" >&2
    exit 1
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
verify_tool="$script_dir/verify-ios-build-identity.sh"
[[ -x "$verify_tool" ]] || fail "build-identity verifier is not executable: $verify_tool"
plist_buddy='/usr/libexec/PlistBuddy'
[[ -x "$plist_buddy" ]] || fail "PlistBuddy is unavailable"

test_tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/ctrpad-ios-build-identity.XXXXXX")"
cleanup() {
    case "$test_tmp_dir" in
        "${TMPDIR:-/tmp}"/ctrpad-ios-build-identity.*) rm -rf "$test_tmp_dir" ;;
        *) printf 'WARNING: refusing unexpected cleanup path: %s\n' "$test_tmp_dir" >&2 ;;
    esac
}
trap cleanup EXIT INT TERM

create_info() {
    local path="$1"
    local source_commit="${2:-0123456789abcdef0123456789abcdef01234567}"
    local build_identity="${3:-${source_commit:0:12}}"
    plutil -create xml1 "$path"
    "$plist_buddy" -c 'Add :CFBundleIdentifier string io.github.chrissotraidis.ctrpad' "$path"
    "$plist_buddy" -c 'Add :CFBundleShortVersionString string 0.1.0' "$path"
    "$plist_buddy" -c 'Add :CFBundleVersion string 1' "$path"
    "$plist_buddy" -c "Add :CTRNativeSourceCommit string $source_commit" "$path"
    "$plist_buddy" -c "Add :CTRNativeBuildIdentity string $build_identity" "$path"
}

expect_success() {
    local label="$1"
    shift
    local output="$test_tmp_dir/$label.manifest"
    "$verify_tool" "$@" --output "$output" >"$test_tmp_dir/$label.stdout"
    grep -Fxq 'VALIDATION_STATUS=verified' "$output" || \
        fail "$label did not write a verified manifest"
}

expect_failure() {
    local label="$1"
    local expected="$2"
    shift 2
    local output="$test_tmp_dir/$label.manifest"
    if "$verify_tool" "$@" --output "$output" \
        >"$test_tmp_dir/$label.stdout" 2>"$test_tmp_dir/$label.stderr"; then
        fail "$label unexpectedly passed"
    fi
    grep -Fq "$expected" "$test_tmp_dir/$label.stderr" || \
        fail "$label did not fail at the expected boundary"
    [[ ! -e "$output" ]] || fail "$label wrote a success manifest on failure"
}

valid_info="$test_tmp_dir/valid.plist"
create_info "$valid_info"
expect_success internal --info-plist "$valid_info"
expect_success expected-full --info-plist "$valid_info" \
    --expected-source-commit 0123456789abcdef0123456789abcdef01234567

missing_source="$test_tmp_dir/missing-source.plist"
cp "$valid_info" "$missing_source"
plutil -remove CTRNativeSourceCommit "$missing_source"
expect_failure missing-source 'no CTRNativeSourceCommit' --info-plist "$missing_source"

dirty_build="$test_tmp_dir/dirty-build.plist"
create_info "$dirty_build" 0123456789abcdef0123456789abcdef01234567 0123456789ab-dirty
expect_failure dirty-build 'is not the clean source prefix' --info-plist "$dirty_build"

wrong_expected="$test_tmp_dir/wrong-expected.plist"
cp "$valid_info" "$wrong_expected"
expect_failure wrong-expected 'does not equal expected' --info-plist "$wrong_expected" \
    --expected-source-commit fedcba9876543210fedcba9876543210fedcba98

malformed_source="$test_tmp_dir/malformed-source.plist"
create_info "$malformed_source" unknown unknown
expect_failure malformed-source 'is not 40 lowercase hexadecimal' \
    --info-plist "$malformed_source"

truncated_source="$test_tmp_dir/truncated-source.plist"
create_info "$truncated_source" 0123456789ab 0123456789ab
expect_failure truncated-source 'is not 40 lowercase hexadecimal' \
    --info-plist "$truncated_source"

wrong_build="$test_tmp_dir/wrong-build.plist"
create_info "$wrong_build" 0123456789abcdef0123456789abcdef01234567 fedcba987654
expect_failure wrong-build 'is not the clean source prefix' --info-plist "$wrong_build"

printf 'IOS_BUILD_IDENTITY_SELF_TEST=passed positives=2 negatives=6\n'
