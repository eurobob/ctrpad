#!/usr/bin/env bash
# Exercise positive and negative iOS profile/signature entitlement bindings.

set -euo pipefail

fail() {
    printf 'SELF-TEST ERROR: %s\n' "$*" >&2
    exit 1
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
binding_tool="$script_dir/verify-ios-entitlement-binding.sh"
[[ -x "$binding_tool" ]] || fail "binding verifier is not executable: $binding_tool"
plist_buddy='/usr/libexec/PlistBuddy'
[[ -x "$plist_buddy" ]] || fail "PlistBuddy is unavailable"

test_tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/ctrpad-entitlement-test.XXXXXX")"
cleanup() {
    case "$test_tmp_dir" in
        "${TMPDIR:-/tmp}"/ctrpad-entitlement-test.*) rm -rf "$test_tmp_dir" ;;
        *) printf 'WARNING: refusing unexpected cleanup path: %s\n' "$test_tmp_dir" >&2 ;;
    esac
}
trap cleanup EXIT INT TERM

create_profile() {
    local path="$1"
    local application_id_pattern="$2"
    local keychain_pattern="$3"
    local team_entitlement="${4:-TESTTEAM01}"
    plutil -create xml1 "$path"
    "$plist_buddy" -c 'Add :ApplicationIdentifierPrefix array' "$path"
    "$plist_buddy" -c 'Add :ApplicationIdentifierPrefix:0 string TESTPREFIX1' "$path"
    "$plist_buddy" -c 'Add :TeamIdentifier array' "$path"
    "$plist_buddy" -c 'Add :TeamIdentifier:0 string TESTTEAM01' "$path"
    "$plist_buddy" -c 'Add :Entitlements dict' "$path"
    "$plist_buddy" -c "Add :Entitlements:application-identifier string $application_id_pattern" "$path"
    "$plist_buddy" -c "Add :Entitlements:com.apple.developer.team-identifier string $team_entitlement" "$path"
    "$plist_buddy" -c 'Add :Entitlements:keychain-access-groups array' "$path"
    "$plist_buddy" -c "Add :Entitlements:keychain-access-groups:0 string $keychain_pattern" "$path"
    "$plist_buddy" -c 'Add :Entitlements:get-task-allow bool true' "$path"
}

create_entitlements() {
    local path="$1"
    plutil -create xml1 "$path"
    "$plist_buddy" -c 'Add :application-identifier string TESTPREFIX1.io.github.chrissotraidis.ctrpad' "$path"
    "$plist_buddy" -c 'Add :com.apple.developer.team-identifier string TESTTEAM01' "$path"
    "$plist_buddy" -c 'Add :keychain-access-groups array' "$path"
    "$plist_buddy" -c 'Add :keychain-access-groups:0 string TESTPREFIX1.io.github.chrissotraidis.ctrpad' "$path"
    "$plist_buddy" -c 'Add :get-task-allow bool true' "$path"
}

expect_success() {
    local label="$1"
    local profile="$2"
    local entitlements="$3"
    local output="$test_tmp_dir/$label.manifest"
    "$binding_tool" --profile-plist "$profile" \
        --entitlements-plist "$entitlements" \
        --bundle-id io.github.chrissotraidis.ctrpad \
        --output "$output" >"$test_tmp_dir/$label.stdout"
    grep -Fxq 'AUTHORIZATION_STATUS=verified' "$output" || \
        fail "$label did not write a verified manifest"
}

expect_failure() {
    local label="$1"
    local expected="$2"
    local profile="$3"
    local entitlements="$4"
    local output="$test_tmp_dir/$label.manifest"
    if "$binding_tool" --profile-plist "$profile" \
        --entitlements-plist "$entitlements" \
        --bundle-id io.github.chrissotraidis.ctrpad \
        --output "$output" >"$test_tmp_dir/$label.stdout" \
        2>"$test_tmp_dir/$label.stderr"; then
        fail "$label unexpectedly passed"
    fi
    grep -Fq "$expected" "$test_tmp_dir/$label.stderr" || \
        fail "$label did not fail at the expected boundary"
    [[ ! -e "$output" ]] || fail "$label wrote a success manifest on failure"
}

exact_profile="$test_tmp_dir/exact-profile.plist"
wildcard_profile="$test_tmp_dir/wildcard-profile.plist"
base_entitlements="$test_tmp_dir/base-entitlements.plist"
create_profile "$exact_profile" \
    TESTPREFIX1.io.github.chrissotraidis.ctrpad 'TESTPREFIX1.*'
create_profile "$wildcard_profile" TESTPREFIX1.io.github.chrissotraidis.* \
    'TESTPREFIX1.*'
create_entitlements "$base_entitlements"
expect_success exact "$exact_profile" "$base_entitlements"
expect_success wildcard "$wildcard_profile" "$base_entitlements"

wrong_prefix="$test_tmp_dir/wrong-prefix.plist"
cp "$base_entitlements" "$wrong_prefix"
"$plist_buddy" -c \
    'Set :application-identifier WRONGPREFIX.io.github.chrissotraidis.ctrpad' \
    "$wrong_prefix"
expect_failure wrong-prefix 'does not equal TESTPREFIX1.' \
    "$exact_profile" "$wrong_prefix"

extra_entitlement="$test_tmp_dir/extra-entitlement.plist"
cp "$base_entitlements" "$extra_entitlement"
"$plist_buddy" -c 'Add :aps-environment string development' "$extra_entitlement"
expect_failure extra-entitlement 'unsupported entitlement: aps-environment' \
    "$exact_profile" "$extra_entitlement"

multiple_keychain="$test_tmp_dir/multiple-keychain.plist"
cp "$base_entitlements" "$multiple_keychain"
"$plist_buddy" -c \
    'Add :keychain-access-groups:1 string TESTPREFIX1.shared' \
    "$multiple_keychain"
expect_failure multiple-keychain 'more than one keychain access group' \
    "$exact_profile" "$multiple_keychain"

wrong_keychain_profile="$test_tmp_dir/wrong-keychain-profile.plist"
create_profile "$wrong_keychain_profile" \
    TESTPREFIX1.io.github.chrissotraidis.ctrpad 'OTHERPREFIX.*'
expect_failure wrong-keychain 'does not authorize signed keychain group' \
    "$wrong_keychain_profile" "$base_entitlements"

wrong_task_allow="$test_tmp_dir/wrong-task-allow.plist"
cp "$base_entitlements" "$wrong_task_allow"
"$plist_buddy" -c 'Set :get-task-allow false' "$wrong_task_allow"
expect_failure wrong-task-allow 'does not match profile authorization' \
    "$exact_profile" "$wrong_task_allow"

malformed_wildcard_profile="$test_tmp_dir/malformed-wildcard-profile.plist"
create_profile "$malformed_wildcard_profile" TESTPREFIX1.io.*.ctrpad \
    'TESTPREFIX1.*'
expect_failure malformed-wildcard 'wildcard must be final' \
    "$malformed_wildcard_profile" "$base_entitlements"

wrong_profile_team="$test_tmp_dir/wrong-profile-team.plist"
create_profile "$wrong_profile_team" \
    TESTPREFIX1.io.github.chrissotraidis.ctrpad 'TESTPREFIX1.*' WRONGTEAM01
expect_failure wrong-profile-team 'profile team entitlement does not match' \
    "$wrong_profile_team" "$base_entitlements"

printf 'IOS_ENTITLEMENT_BINDING_SELF_TEST=passed positives=2 negatives=7\n'
