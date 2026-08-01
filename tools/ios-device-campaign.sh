#!/usr/bin/env bash
# Preflight, update-install, launch, and collect local evidence for a signed iPad build.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  ./tools/ios-device-campaign.sh preflight [options]
  ./tools/ios-device-campaign.sh prepare [options]
  ./tools/ios-device-campaign.sh collect [options]

Preflight/prepare options:
  --ipa PATH              Signed CTRPad IPA to validate.
  --device-udid UDID      Provisioned physical-device UDID. Required by
                          prepare; optional but verified by preflight.
  --evidence-dir PATH     New local evidence directory (must not exist).

Collect options:
  --device-udid UDID      Physical-device UDID.
  --bundle-id ID          Installed CTRPad bundle identifier.
  --evidence-dir PATH     Existing prepare evidence directory.

Common options:
  --timeout SECONDS       Per-devicectl timeout (default: 120).
  -h, --help              Show this help.

preflight validates the IPA, non-ad-hoc signature, embedded provisioning profile,
bundle/application/team identifiers, thin ARM64 iOS executable, optional UDID,
retail exclusion, and checksum sidecar without contacting a device.

prepare performs the same preflight, records versioned devicectl JSON/logs,
update-installs without uninstalling, verifies the bundle appears on the
specified physical device, and launches a fresh foreground process.

collect runs after human testing. It records current device/app information and
copies only CTRPad's Application Support tree (logs, saves, diagnostics), never
the Documents tree containing the retail image. Evidence may contain device
identifiers and user save data; keep it local until deliberately reviewed.
EOF
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

guard_local_evidence_path() {
    case "$evidence_dir" in
        "$repo_root")
            fail "evidence directory cannot be the repository root"
            ;;
        "$repo_root"/*)
            command -v git >/dev/null 2>&1 || \
                fail "Git is required to verify an in-repository evidence path is ignored"
            git -C "$repo_root" check-ignore -q -- "$evidence_dir" || \
                fail "in-repository evidence path must be gitignored: $evidence_dir"
            ;;
    esac
}

resolve_new_evidence_dir() {
    local requested="$1"
    local parent_path
    [[ -n "$requested" ]] || fail "--evidence-dir is required"
    if [[ "$requested" != /* ]]; then
        requested="$repo_root/$requested"
    fi
    parent_path="$(dirname "$requested")"
    if [[ ! -d "$parent_path" ]]; then
        case "$parent_path" in
            "$repo_root/dist"|"$repo_root/dist"/*) mkdir -p "$parent_path" ;;
            *) fail "evidence parent directory not found: $parent_path" ;;
        esac
    fi
    parent_path="$(cd "$parent_path" && pwd -P)"
    evidence_dir="$parent_path/$(basename "$requested")"
    [[ ! -e "$evidence_dir" ]] || fail "evidence path already exists: $evidence_dir"
    guard_local_evidence_path
    mkdir "$evidence_dir"
}

resolve_existing_evidence_dir() {
    local requested="$1"
    [[ -n "$requested" ]] || fail "--evidence-dir is required"
    if [[ "$requested" != /* ]]; then
        requested="$repo_root/$requested"
    fi
    [[ -d "$requested" ]] || fail "evidence directory not found: $requested"
    evidence_dir="$(cd "$requested" && pwd -P)"
    guard_local_evidence_path
}

run_devicectl() {
    local evidence_root="$1"
    local label="$2"
    shift 2
    xcrun devicectl --timeout "$device_timeout" \
        --json-output "$evidence_root/$label.json" \
        --log-output "$evidence_root/$label.log" "$@"
}

preflight_signed_ipa() {
    [[ -f "$ipa_path" ]] || fail "IPA not found: $ipa_path"
    ipa_parent="$(cd "$(dirname "$ipa_path")" && pwd -P)"
    ipa_path="$ipa_parent/$(basename "$ipa_path")"
    [[ "$ipa_path" == *.ipa ]] || fail "input path must end in .ipa: $ipa_path"

    ipa_hash="$(shasum -a 256 "$ipa_path" | awk '{print $1}')"
    [[ "$ipa_hash" =~ ^[0-9a-f]{64}$ ]] || fail "could not hash IPA: $ipa_path"
    printf '%s  %s\n' "$ipa_hash" "$(basename "$ipa_path")" >"$evidence_dir/ipa.sha256"

    sidecar_status="absent"
    if [[ -f "$ipa_path.sha256" ]]; then
        sidecar_rows="$(awk 'NF { count += 1 } END { print count + 0 }' "$ipa_path.sha256")"
        sidecar_hash="$(awk 'NF { print $1; exit }' "$ipa_path.sha256")"
        [[ "$sidecar_rows" == "1" ]] || fail "IPA sidecar must contain exactly one hash row"
        [[ "$sidecar_hash" =~ ^[0-9a-f]{64}$ ]] || fail "IPA sidecar has an invalid SHA-256"
        [[ "$sidecar_hash" == "$ipa_hash" ]] || \
            fail "IPA sidecar hash $sidecar_hash does not match $ipa_hash"
        printf '%s: OK\n' "$ipa_path" >"$evidence_dir/input-sidecar-check.txt"
        sidecar_status="verified"
    fi

    unzip -t "$ipa_path" >"$evidence_dir/unzip-test.txt"
    ditto -x -k --norsrc --noextattr --noqtn --noacl "$ipa_path" "$tmp_dir/extracted"
    payload_dir="$tmp_dir/extracted/Payload"
    [[ -d "$payload_dir" ]] || fail "IPA has no Payload directory"
    app_list="$tmp_dir/apps.txt"
    find "$payload_dir" -mindepth 1 -maxdepth 1 -type d -name '*.app' -print >"$app_list"
    app_count="$(awk 'NF { count += 1 } END { print count + 0 }' "$app_list")"
    [[ "$app_count" == "1" ]] || fail "expected exactly one Payload app; found $app_count"
    app_path="$(awk 'NF { print; exit }' "$app_list")"

    info_plist="$app_path/Info.plist"
    [[ -f "$info_plist" ]] || fail "signed app has no Info.plist"
    bundle_id="$(plutil -extract CFBundleIdentifier raw -o - "$info_plist")"
    executable_name="$(plutil -extract CFBundleExecutable raw -o - "$info_plist")"
    package_type="$(plutil -extract CFBundlePackageType raw -o - "$info_plist")"
    bundle_version="$(plutil -extract CFBundleShortVersionString raw -o - "$info_plist")"
    build_version="$(plutil -extract CFBundleVersion raw -o - "$info_plist")"
    executable_path="$app_path/$executable_name"

    [[ "$package_type" == "APPL" ]] || fail "unexpected CFBundlePackageType: $package_type"
    [[ "$bundle_id" =~ ^[A-Za-z0-9.-]+$ ]] || fail "unsupported bundle ID: $bundle_id"
    [[ -x "$executable_path" ]] || fail "signed app executable is missing or not executable"
    architectures="$(lipo -archs "$executable_path")"
    [[ "$architectures" == "arm64" ]] || fail "device executable must be thin arm64; got: $architectures"
    xcrun vtool -show-build "$executable_path" | \
        grep -Eq '^[[:space:]]*platform IOS$' || fail "executable has no iOS LC_BUILD_VERSION"
    file "$executable_path" | grep -q 'Mach-O 64-bit executable arm64' || \
        fail "device executable is not an ARM64 Mach-O"

    for required_resource in LICENSE THIRD_PARTY_NOTICES.md INSTALL-IOS.md; do
        [[ -n "$(find "$app_path" -type f -name "$required_resource" -print -quit)" ]] || \
            fail "signed app is missing distribution resource: $required_resource"
    done

    retail_match="$(find "$app_path" -type f \( \
        -iname 'ctr-u.bin' -o -iname '*.bin' -o -iname '*.img' -o \
        -iname '*.iso' -o -iname '*.cue' -o -iname '*.ccd' -o \
        -iname '*.sub' -o -iname '*.big' -o -iname '*.hwl' -o \
        -iname '*.xa' -o -iname '*.str' \
    \) -print -quit)"
    [[ -z "$retail_match" ]] || fail "retail-like file found in signed app: $retail_match"
    runtime_match="$(find "$app_path" -type d \( \
        -name Documents -o -name memcards -o -name 'Application Support' \
    \) -print -quit)"
    [[ -z "$runtime_match" ]] || fail "runtime data directory found in signed app: $runtime_match"

    profile_path="$app_path/embedded.mobileprovision"
    [[ -f "$profile_path" ]] || fail "signed app has no embedded.mobileprovision"
    [[ -d "$app_path/_CodeSignature" ]] || fail "signed app has no _CodeSignature directory"
    app_trust_dir="$evidence_dir/app-signing-trust"
    "$signing_trust_tool" app --app "$app_path" \
        --output-dir "$app_trust_dir" >/dev/null
    codesign --display --verbose=4 "$app_path" \
        >"$evidence_dir/codesign-display.txt" 2>&1
    app_certificate_hash="$(awk -F= \
        '$1 == "LEAF_CERTIFICATE_SHA256" { print $2 }' \
        "$app_trust_dir/trust-manifest.txt")"
    app_trusted_root_hash="$(awk -F= \
        '$1 == "TRUSTED_ROOT_CERTIFICATE_SHA256" { print $2 }' \
        "$app_trust_dir/trust-manifest.txt")"
    [[ "$app_certificate_hash" =~ ^[0-9a-f]{64}$ ]] || \
        fail "could not read the trusted app signing certificate hash"
    [[ "$app_trusted_root_hash" =~ ^[0-9a-f]{64}$ ]] || \
        fail "could not read the app trusted-root hash"
    signed_entitlements="$tmp_dir/signed-entitlements.plist"
    codesign --display --entitlements - --xml "$app_path" \
        >"$signed_entitlements" 2>"$evidence_dir/codesign-entitlements-display.txt"

    profile_trust_dir="$evidence_dir/profile-signing-trust"
    "$signing_trust_tool" profile --profile "$profile_path" \
        --output-dir "$profile_trust_dir" >/dev/null
    profile_plist="$profile_trust_dir/profile-decoded.plist"
    profile_signer_hash="$(awk -F= \
        '$1 == "SIGNER_CERTIFICATE_SHA256" { print $2 }' \
        "$profile_trust_dir/trust-manifest.txt")"
    profile_trusted_root_hash="$(awk -F= \
        '$1 == "TRUSTED_ROOT_CERTIFICATE_SHA256" { print $2 }' \
        "$profile_trust_dir/trust-manifest.txt")"
    [[ "$profile_signer_hash" =~ ^[0-9a-f]{64}$ ]] || \
        fail "could not read the trusted profile signer hash"
    [[ "$profile_trusted_root_hash" =~ ^[0-9a-f]{64}$ ]] || \
        fail "could not read the profile trusted-root hash"
    profile_platforms="$(plutil -extract Platform json -o - "$profile_plist")"
    grep -q 'iOS' <<<"$profile_platforms" || fail "embedded profile is not valid for iOS"
    profile_name="$(plutil -extract Name raw -o - "$profile_plist")"
    profile_uuid="$(plutil -extract UUID raw -o - "$profile_plist")"
    profile_expiration="$(plutil -extract ExpirationDate raw -o - "$profile_plist")"
    expiration_epoch="$(date -j -u -f '%Y-%m-%dT%H:%M:%SZ' "$profile_expiration" '+%s' 2>/dev/null || true)"
    [[ -n "$expiration_epoch" ]] || fail "could not parse profile expiration: $profile_expiration"
    ((expiration_epoch > $(date -u '+%s'))) || fail "embedded profile expired at $profile_expiration"

    profile_application_id="$(plutil -extract Entitlements.application-identifier raw -o - "$profile_plist")"
    profile_team="$(plutil -extract TeamIdentifier.0 raw -o - "$profile_plist")"
    entitlement_binding_manifest="$evidence_dir/entitlement-binding-manifest.txt"
    "$entitlement_binding_tool" --profile-plist "$profile_plist" \
        --entitlements-plist "$signed_entitlements" --bundle-id "$bundle_id" \
        --output "$entitlement_binding_manifest" >/dev/null
    application_identifier_prefix="$(awk -F= \
        '$1 == "APPLICATION_IDENTIFIER_PREFIX" { print $2 }' \
        "$entitlement_binding_manifest")"
    signed_application_id="$(awk -F= \
        '$1 == "SIGNED_APPLICATION_IDENTIFIER" { print $2 }' \
        "$entitlement_binding_manifest")"
    signed_keychain_group="$(awk -F= \
        '$1 == "SIGNED_KEYCHAIN_ACCESS_GROUP" { print $2 }' \
        "$entitlement_binding_manifest")"
    get_task_allow_status="$(awk -F= \
        '$1 == "GET_TASK_ALLOW_STATUS" { print $2 }' \
        "$entitlement_binding_manifest")"
    profile_certificate_match=0
    profile_certificate_count=0
    while profile_certificate_base64="$(plutil -extract "DeveloperCertificates.$profile_certificate_count" raw -o - \
        "$profile_plist" 2>/dev/null)"; do
        profile_certificate_path="$tmp_dir/profile-certificate-$profile_certificate_count.cer"
        printf '%s' "$profile_certificate_base64" | base64 -D >"$profile_certificate_path"
        profile_certificate_hash="$(shasum -a 256 "$profile_certificate_path" | awk '{print $1}')"
        if [[ "$profile_certificate_hash" == "$app_certificate_hash" ]]; then
            profile_certificate_match=1
        fi
        profile_certificate_count=$((profile_certificate_count + 1))
    done
    ((profile_certificate_count > 0)) || fail "embedded profile has no developer certificates"
    ((profile_certificate_match == 1)) || \
        fail "app leaf signing certificate is not authorized by embedded profile"
    grep -q '^Authority=' "$evidence_dir/codesign-display.txt" || \
        fail "app signature has no certificate authority and may be ad hoc"
    grep -Fq "TeamIdentifier=$profile_team" "$evidence_dir/codesign-display.txt" || \
        fail "signature TeamIdentifier does not match embedded profile team $profile_team"

    udid_status="not-requested"
    if [[ -n "$device_udid" ]]; then
        provisioned_devices="$(plutil -extract ProvisionedDevices json -o - "$profile_plist" 2>/dev/null || true)"
        grep -Fq "\"$device_udid\"" <<<"$provisioned_devices" || \
            fail "embedded profile does not contain requested device UDID $device_udid"
        udid_status="verified"
    fi

    executable_hash="$(shasum -a 256 "$executable_path" | awk '{print $1}')"
    {
        printf 'PHASE=offline-preflight-success\n'
        printf 'IPA=%s\n' "$ipa_path"
        printf 'IPA_SHA256=%s\n' "$ipa_hash"
        printf 'INPUT_SIDECAR=%s\n' "$sidecar_status"
        printf 'BUNDLE_ID=%s\n' "$bundle_id"
        printf 'VERSION=%s\n' "$bundle_version"
        printf 'BUILD=%s\n' "$build_version"
        printf 'EXECUTABLE=%s\n' "$executable_name"
        printf 'EXECUTABLE_SHA256=%s\n' "$executable_hash"
        printf 'ARCHITECTURES=%s\n' "$architectures"
        printf 'PROFILE_NAME=%s\n' "$profile_name"
        printf 'PROFILE_UUID=%s\n' "$profile_uuid"
        printf 'PROFILE_EXPIRATION=%s\n' "$profile_expiration"
        printf 'PROFILE_APPLICATION_ID=%s\n' "$profile_application_id"
        printf 'PROFILE_TEAM=%s\n' "$profile_team"
        printf 'APPLICATION_IDENTIFIER_PREFIX=%s\n' "$application_identifier_prefix"
        printf 'SIGNED_APPLICATION_IDENTIFIER=%s\n' "$signed_application_id"
        printf 'SIGNED_KEYCHAIN_ACCESS_GROUP=%s\n' "$signed_keychain_group"
        printf 'GET_TASK_ALLOW_STATUS=%s\n' "$get_task_allow_status"
        printf 'ENTITLEMENT_BINDING_STATUS=verified\n'
        printf 'PROFILE_CMS_SIGNER_CERTIFICATE_SHA256=%s\n' "$profile_signer_hash"
        printf 'PROFILE_TRUSTED_ROOT_CERTIFICATE_SHA256=%s\n' "$profile_trusted_root_hash"
        printf 'PROFILE_CMS_TRUST_STATUS=verified\n'
        printf 'SIGNER_CERTIFICATE_SHA256=%s\n' "$app_certificate_hash"
        printf 'APP_TRUSTED_ROOT_CERTIFICATE_SHA256=%s\n' "$app_trusted_root_hash"
        printf 'APP_CERTIFICATE_TRUST_STATUS=verified\n'
        printf 'PROFILE_CERTIFICATE_COUNT=%s\n' "$profile_certificate_count"
        printf 'SIGNER_CERTIFICATE_PROFILE_STATUS=verified\n'
        printf 'DEVICE_UDID=%s\n' "${device_udid:-not-requested}"
        printf 'DEVICE_UDID_PROFILE_STATUS=%s\n' "$udid_status"
    } >"$evidence_dir/preflight-manifest.txt"
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
signing_trust_tool="$repo_root/tools/verify-ios-signing-trust.sh"
[[ -x "$signing_trust_tool" ]] || \
    fail "required signing-trust verifier not found: $signing_trust_tool"
entitlement_binding_tool="$repo_root/tools/verify-ios-entitlement-binding.sh"
[[ -x "$entitlement_binding_tool" ]] || \
    fail "required entitlement-binding verifier not found: $entitlement_binding_tool"
command_name="${1:-}"
if [[ -z "$command_name" ]]; then
    usage
    exit 1
fi
shift

ipa_path=""
device_udid=""
bundle_id=""
requested_evidence_dir=""
evidence_dir=""
device_timeout=120

while (($#)); do
    case "$1" in
        --ipa|--device-udid|--bundle-id|--evidence-dir|--timeout)
            (($# >= 2)) || fail "$1 requires a value"
            case "$1" in
                --ipa) ipa_path="$2" ;;
                --device-udid) device_udid="$2" ;;
                --bundle-id) bundle_id="$2" ;;
                --evidence-dir) requested_evidence_dir="$2" ;;
                --timeout) device_timeout="$2" ;;
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

[[ "$device_timeout" =~ ^[1-9][0-9]*$ ]] || fail "--timeout must be a positive integer"
for command_tool in awk base64 basename codesign date dirname ditto file find grep \
    lipo mkdir mktemp plutil rg rm security shasum uname unzip xcrun; do
    require_command "$command_tool"
done
[[ "$(uname -s)" == "Darwin" ]] || fail "physical iOS device work requires macOS"
plist_buddy='/usr/libexec/PlistBuddy'
[[ -x "$plist_buddy" ]] || fail "required tool not found: $plist_buddy"

case "$command_name" in
    preflight|prepare)
        [[ -n "$ipa_path" ]] || fail "--ipa is required for $command_name"
        [[ "$command_name" != "prepare" || -n "$device_udid" ]] || \
            fail "--device-udid is required for prepare"
        [[ -z "$device_udid" || "$device_udid" =~ ^[A-Za-z0-9-]+$ ]] || \
            fail "unsupported device UDID: $device_udid"
        [[ -z "$bundle_id" ]] || fail "--bundle-id is valid only with collect"
        resolve_new_evidence_dir "$requested_evidence_dir"
        tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/ctrpad-device-campaign.XXXXXX")"
        cleanup() {
            case "$tmp_dir" in
                "${TMPDIR:-/tmp}"/ctrpad-device-campaign.*) rm -rf "$tmp_dir" ;;
                *) printf 'WARNING: refusing to remove unexpected temporary path: %s\n' "$tmp_dir" >&2 ;;
            esac
        }
        trap cleanup EXIT INT TERM
        preflight_signed_ipa
        if [[ "$command_name" == "preflight" ]]; then
            printf 'PREFLIGHT_VERIFIED=%s\n' "$evidence_dir"
            exit 0
        fi

        run_devicectl "$evidence_dir" device-list list devices
        run_devicectl "$evidence_dir" device-details device info details \
            --device "$device_udid"
        run_devicectl "$evidence_dir" install device install app \
            --device "$device_udid" "$app_path"
        run_devicectl "$evidence_dir" installed-app device info apps \
            --device "$device_udid" --bundle-id "$bundle_id"
        grep -Fq "$bundle_id" "$evidence_dir/installed-app.json" || \
            fail "installed-app evidence does not contain $bundle_id"
        run_devicectl "$evidence_dir" launch device process launch \
            --device "$device_udid" --terminate-existing "$bundle_id"
        {
            printf 'PHASE=prepare-success\n'
            printf 'DEVICE_UDID=%s\n' "$device_udid"
            printf 'BUNDLE_ID=%s\n' "$bundle_id"
            printf 'SIGNED_PACKAGE_EXECUTABLE_SHA256=%s\n' "$executable_hash"
            printf 'DEVICECTL_VERSION=%s\n' "$(xcrun devicectl --version)"
            printf 'COMPLETED_UTC=%s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
        } >"$evidence_dir/prepare-manifest.txt"
        printf 'DEVICE_PREPARED=%s\n' "$evidence_dir"
        ;;
    collect)
        [[ -z "$ipa_path" ]] || fail "--ipa is valid only with preflight or prepare"
        [[ -n "$device_udid" ]] || fail "--device-udid is required for collect"
        [[ "$device_udid" =~ ^[A-Za-z0-9-]+$ ]] || \
            fail "unsupported device UDID: $device_udid"
        [[ -n "$bundle_id" ]] || fail "--bundle-id is required for collect"
        [[ "$bundle_id" =~ ^[A-Za-z0-9.-]+$ ]] || fail "unsupported bundle ID: $bundle_id"
        resolve_existing_evidence_dir "$requested_evidence_dir"
        prepare_manifest="$evidence_dir/prepare-manifest.txt"
        [[ -f "$prepare_manifest" ]] || \
            fail "evidence directory has no successful prepare manifest: $evidence_dir"
        grep -Fxq 'PHASE=prepare-success' "$prepare_manifest" || \
            fail "evidence directory does not record a successful prepare phase"
        grep -Fxq "DEVICE_UDID=$device_udid" "$prepare_manifest" || \
            fail "collect device does not match the prepare manifest"
        grep -Fxq "BUNDLE_ID=$bundle_id" "$prepare_manifest" || \
            fail "collect bundle does not match the prepare manifest"
        collection_stamp="$(date -u '+%Y%m%dT%H%M%SZ')"
        collection_dir="$evidence_dir/collection-$collection_stamp-$$"
        [[ ! -e "$collection_dir" ]] || fail "collection path already exists: $collection_dir"
        mkdir "$collection_dir"
        run_devicectl "$collection_dir" device-details device info details \
            --device "$device_udid"
        run_devicectl "$collection_dir" installed-app device info apps \
            --device "$device_udid" --bundle-id "$bundle_id"
        grep -Fq "$bundle_id" "$collection_dir/installed-app.json" || \
            fail "installed-app evidence does not contain $bundle_id"
        run_devicectl "$collection_dir" copy-app-support device copy from \
            --device "$device_udid" \
            --domain-type appDataContainer \
            --domain-identifier "$bundle_id" \
            --source 'Library/Application Support/chrissotraidis/CTRPad' \
            --destination "$collection_dir/app-support"

        copied_retail="$(find "$collection_dir/app-support" -type f \( \
            -iname 'ctr-u.bin' -o -iname '*.img' -o -iname '*.iso' -o \
            -iname '*.cue' -o -iname '*.ccd' -o -iname '*.sub' -o \
            -iname '*.big' -o -iname '*.hwl' -o -iname '*.xa' -o \
            -iname '*.str' \
        \) -print -quit)"
        [[ -z "$copied_retail" ]] || fail "retail-like media entered evidence: $copied_retail"
        rg -n --glob 'Crash Team Racing.log*' \
            '\[(ERROR|FATAL)\]|\[CTR AssetRef\]|visibility' \
            "$collection_dir/app-support" \
            >"$collection_dir/targeted-fault-scan.txt" || true
        rg -n --glob 'Crash Team Racing.log*' '\[CTR Native\] FPS:' \
            "$collection_dir/app-support" >"$collection_dir/fps-rows.txt" || true
        rg -n --glob 'Crash Team Racing.log*' \
            '\[CTR (Session|Input|Touch|Lifecycle|Import)\]' \
            "$collection_dir/app-support" >"$collection_dir/campaign-rows.txt" || true
        find "$collection_dir/app-support" -type f -path '*/memcards/*' \
            -exec shasum -a 256 {} \; >"$collection_dir/save-sha256.txt"
        {
            printf 'PHASE=collect-success\n'
            printf 'DEVICE_UDID=%s\n' "$device_udid"
            printf 'BUNDLE_ID=%s\n' "$bundle_id"
            printf 'COLLECTED_UTC=%s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
            printf 'DOCUMENTS_RETAIL_TREE_COPIED=no\n'
            printf 'APP_SUPPORT=%s\n' "$collection_dir/app-support"
            printf 'TARGETED_FAULT_ROWS=%s\n' "$(awk 'END { print NR + 0 }' "$collection_dir/targeted-fault-scan.txt")"
            printf 'FPS_ROWS=%s\n' "$(awk 'END { print NR + 0 }' "$collection_dir/fps-rows.txt")"
        } >"$collection_dir/collect-manifest.txt"
        printf 'DEVICE_EVIDENCE_COLLECTED=%s\n' "$collection_dir"
        ;;
    -h|--help)
        usage
        ;;
    *)
        fail "unknown command: $command_name"
        ;;
esac
