#!/usr/bin/env bash
# Verify CTRPad's signed entitlements against a trusted provisioning-profile plist.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  ./tools/verify-ios-entitlement-binding.sh \
    --profile-plist PATH --entitlements-plist PATH --bundle-id ID --output PATH

The caller must first authenticate the decoded profile and the app signature.
This tool binds CTRPad's minimal signed entitlements to that trusted profile:
the exact App ID prefix/bundle ID, team, sole keychain group, get-task-allow
when present, and a strict four-key entitlement allowlist.
EOF
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

profile_plist=""
entitlements_plist=""
bundle_id=""
requested_output_path=""

while (($#)); do
    case "$1" in
        --profile-plist|--entitlements-plist|--bundle-id|--output)
            (($# >= 2)) || fail "$1 requires a value"
            case "$1" in
                --profile-plist) profile_plist="$2" ;;
                --entitlements-plist) entitlements_plist="$2" ;;
                --bundle-id) bundle_id="$2" ;;
                --output) requested_output_path="$2" ;;
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

for command_name in awk basename dirname git grep plutil sed uname; do
    require_command "$command_name"
done
[[ "$(uname -s)" == "Darwin" ]] || \
    fail "iOS entitlement binding verification requires macOS"
plist_buddy='/usr/libexec/PlistBuddy'
[[ -x "$plist_buddy" ]] || fail "required tool not found: $plist_buddy"

[[ -f "$profile_plist" ]] || fail "profile plist not found: $profile_plist"
[[ -f "$entitlements_plist" ]] || \
    fail "signed-entitlements plist not found: $entitlements_plist"
[[ "$bundle_id" =~ ^[A-Za-z0-9.-]+$ ]] || \
    fail "unsupported bundle ID: $bundle_id"
[[ -n "$requested_output_path" ]] || fail "--output is required"

profile_parent="$(cd "$(dirname "$profile_plist")" && pwd -P)"
profile_plist="$profile_parent/$(basename "$profile_plist")"
entitlements_parent="$(cd "$(dirname "$entitlements_plist")" && pwd -P)"
entitlements_plist="$entitlements_parent/$(basename "$entitlements_plist")"
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
            fail "in-repository authorization output must be gitignored: $output_path"
        ;;
esac

plutil -lint "$profile_plist" >/dev/null
plutil -lint "$entitlements_plist" >/dev/null

application_prefix="$(plutil -extract ApplicationIdentifierPrefix.0 raw -o - \
    "$profile_plist")"
application_prefix="${application_prefix%.}"
[[ "$application_prefix" =~ ^[A-Za-z0-9]+$ ]] || \
    fail "profile application identifier prefix is malformed: $application_prefix"
if plutil -extract ApplicationIdentifierPrefix.1 raw -o - \
    "$profile_plist" >/dev/null 2>&1; then
    fail "profile contains multiple application identifier prefixes"
fi

team_identifier="$(plutil -extract TeamIdentifier.0 raw -o - "$profile_plist")"
[[ "$team_identifier" =~ ^[A-Za-z0-9]+$ ]] || \
    fail "profile team identifier is malformed: $team_identifier"
if plutil -extract TeamIdentifier.1 raw -o - \
    "$profile_plist" >/dev/null 2>&1; then
    fail "profile contains multiple team identifiers"
fi

profile_application_id="$(plutil -extract \
    Entitlements.application-identifier raw -o - "$profile_plist")"
profile_identifier_prefix="${profile_application_id%%.*}"
[[ "$profile_identifier_prefix" == "$application_prefix" ]] || \
    fail "profile application-identifier prefix does not match ApplicationIdentifierPrefix"
profile_suffix="${profile_application_id#*.}"
[[ "$profile_suffix" != "$profile_application_id" && -n "$profile_suffix" ]] || \
    fail "profile application-identifier is malformed: $profile_application_id"

if grep -Fq '*' <<<"$profile_suffix"; then
    allowed_bundle_prefix="${profile_suffix%\*}"
    [[ "$allowed_bundle_prefix" != "$profile_suffix" ]] || \
        fail "profile application-identifier wildcard must be final"
    [[ "$allowed_bundle_prefix" != *'*'* ]] || \
        fail "profile application-identifier contains multiple wildcards"
    [[ -z "$allowed_bundle_prefix" || "$allowed_bundle_prefix" == *. ]] || \
        fail "profile application-identifier wildcard must occupy the final component"
    [[ "$bundle_id" == "$allowed_bundle_prefix"* ]] || \
        fail "profile application-identifier does not allow bundle ID $bundle_id"
else
    [[ "$profile_suffix" == "$bundle_id" ]] || \
        fail "profile application-identifier does not match bundle ID $bundle_id"
fi

profile_entitlement_team=""
if profile_entitlement_team="$("$plist_buddy" -c \
    'Print :Entitlements:com.apple.developer.team-identifier' \
    "$profile_plist" 2>/dev/null)"; then
    [[ "$profile_entitlement_team" == "$team_identifier" ]] || \
        fail "profile team entitlement does not match TeamIdentifier"
fi

signed_application_id="$(plutil -extract application-identifier raw -o - \
    "$entitlements_plist")"
expected_application_id="${application_prefix}.${bundle_id}"
[[ "$signed_application_id" == "$expected_application_id" ]] || \
    fail "signed application-identifier $signed_application_id does not equal $expected_application_id"

signed_team="$("$plist_buddy" -c \
    'Print :com.apple.developer.team-identifier' "$entitlements_plist")"
[[ "$signed_team" == "$team_identifier" ]] || \
    fail "signed team $signed_team does not match profile team $team_identifier"

signed_keychain_group="$(plutil -extract keychain-access-groups.0 raw -o - \
    "$entitlements_plist")"
[[ "$signed_keychain_group" == "$expected_application_id" ]] || \
    fail "signed keychain group $signed_keychain_group does not equal $expected_application_id"
if plutil -extract keychain-access-groups.1 raw -o - \
    "$entitlements_plist" >/dev/null 2>&1; then
    fail "CTRPad signature contains more than one keychain access group"
fi

profile_keychain_count=0
profile_keychain_match=0
matched_profile_keychain_pattern=""
while profile_keychain_pattern="$(plutil -extract \
    "Entitlements.keychain-access-groups.$profile_keychain_count" raw -o - \
    "$profile_plist" 2>/dev/null)"; do
    [[ -n "$profile_keychain_pattern" ]] || \
        fail "profile contains an empty keychain access-group pattern"
    if grep -Fq '*' <<<"$profile_keychain_pattern"; then
        profile_keychain_prefix="${profile_keychain_pattern%\*}"
        [[ "$profile_keychain_prefix" != "$profile_keychain_pattern" ]] || \
            fail "profile keychain access-group wildcard must be final"
        [[ "$profile_keychain_prefix" != *'*'* ]] || \
            fail "profile keychain access-group contains multiple wildcards"
        [[ -z "$profile_keychain_prefix" || "$profile_keychain_prefix" == *. ]] || \
            fail "profile keychain access-group wildcard must occupy the final component"
        if [[ "$signed_keychain_group" == "$profile_keychain_prefix"* ]]; then
            profile_keychain_match=1
            matched_profile_keychain_pattern="$profile_keychain_pattern"
        fi
    elif [[ "$signed_keychain_group" == "$profile_keychain_pattern" ]]; then
        profile_keychain_match=1
        matched_profile_keychain_pattern="$profile_keychain_pattern"
    fi
    profile_keychain_count=$((profile_keychain_count + 1))
done
((profile_keychain_count > 0)) || \
    fail "profile contains no keychain access-group authorization"
((profile_keychain_match == 1)) || \
    fail "profile does not authorize signed keychain group $signed_keychain_group"

signed_get_task_allow=""
profile_get_task_allow=""
get_task_allow_status="absent-from-signature"
if signed_get_task_allow="$(plutil -extract get-task-allow raw -o - \
    "$entitlements_plist" 2>/dev/null)"; then
    [[ "$(plutil -type get-task-allow "$entitlements_plist")" == "bool" ]] || \
        fail "signed get-task-allow is not Boolean"
    if ! profile_get_task_allow="$(plutil -extract \
        Entitlements.get-task-allow raw -o - "$profile_plist" 2>/dev/null)"; then
        fail "signature requests get-task-allow but profile does not authorize it"
    fi
    [[ "$(plutil -type Entitlements.get-task-allow "$profile_plist")" == "bool" ]] || \
        fail "profile get-task-allow authorization is not Boolean"
    [[ "$profile_get_task_allow" == "$signed_get_task_allow" ]] || \
        fail "signed get-task-allow does not match profile authorization"
    get_task_allow_status="verified-$signed_get_task_allow"
fi

signed_entitlement_count=0
while IFS= read -r entitlement_key; do
    [[ -n "$entitlement_key" ]] || continue
    signed_entitlement_count=$((signed_entitlement_count + 1))
    case "$entitlement_key" in
        application-identifier|com.apple.developer.team-identifier|\
        get-task-allow|keychain-access-groups)
            ;;
        *)
            fail "CTRPad signature contains unsupported entitlement: $entitlement_key"
            ;;
    esac
done < <(plutil -p "$entitlements_plist" | \
    sed -n 's/^  "\([^"]*\)" =>.*/\1/p')
((signed_entitlement_count >= 3 && signed_entitlement_count <= 4)) || \
    fail "CTRPad signature entitlement count is outside the minimal contract: $signed_entitlement_count"

{
    printf 'TYPE=ios-entitlement-binding\n'
    printf 'AUTHORIZATION_STATUS=verified\n'
    printf 'BUNDLE_ID=%s\n' "$bundle_id"
    printf 'APPLICATION_IDENTIFIER_PREFIX=%s\n' "$application_prefix"
    printf 'PROFILE_APPLICATION_ID_PATTERN=%s\n' "$profile_application_id"
    printf 'TEAM_IDENTIFIER=%s\n' "$team_identifier"
    printf 'SIGNED_APPLICATION_IDENTIFIER=%s\n' "$signed_application_id"
    printf 'SIGNED_KEYCHAIN_ACCESS_GROUP=%s\n' "$signed_keychain_group"
    printf 'MATCHED_PROFILE_KEYCHAIN_PATTERN=%s\n' "$matched_profile_keychain_pattern"
    printf 'GET_TASK_ALLOW_STATUS=%s\n' "$get_task_allow_status"
    printf 'SIGNED_ENTITLEMENT_KEY_COUNT=%s\n' "$signed_entitlement_count"
    printf 'SIGNED_ENTITLEMENT_ALLOWLIST_STATUS=verified\n'
} >"$output_path"

printf 'ENTITLEMENT_BINDING_VERIFIED=%s\n' "$output_path"
