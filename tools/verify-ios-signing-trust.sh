#!/usr/bin/env bash
# Verify that an iOS provisioning profile or signed app chains to an Apple root.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  ./tools/verify-ios-signing-trust.sh profile --profile PATH --output-dir PATH
  ./tools/verify-ios-signing-trust.sh app --app PATH --output-dir PATH

profile verifies the CMS signature cryptographically, verifies the signer
certificate chain with macOS Security, pins the resulting root to an Apple Root
CA in the system root keychain, and writes profile-decoded.plist plus a manifest.

app verifies the code signature, extracts its certificate chain, validates that
chain under code-signing policy, pins its root to the same Apple system roots,
and writes an app trust manifest. The output directory must not already exist.
EOF
}

fail() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || fail "required command not found: $1"
}

pem_sha256() {
    "$openssl_bin" x509 -in "$1" -outform DER | shasum -a 256 | awk '{print $1}'
}

split_pem_chain() {
    local input_path="$1"
    local output_prefix="$2"
    awk -v prefix="$output_prefix" '
        /-----BEGIN CERTIFICATE-----/ {
            output = sprintf("%s%d.pem", prefix, count)
            count += 1
        }
        output != "" { print >> output }
        /-----END CERTIFICATE-----/ {
            close(output)
            output = ""
        }
        END { print count + 0 }
    ' "$input_path"
}

pin_verified_chain_to_apple_root() {
    local label="$1"
    local policy="$2"
    local leaf_certificate="$3"
    local chain_output="$output_dir/$label-trusted-chain.pem"
    local chain_stderr="$output_dir/$label-trust.stderr"
    local chain_prefix="$output_dir/$label-trusted-certificate-"
    local root_candidates="$output_dir/apple-system-roots.pem"
    local root_prefix="$output_dir/apple-system-root-"
    local chain_count
    local root_count
    local root_index
    local root_certificate
    local candidate_index
    local candidate_hash
    local root_match=0
    local -a verify_args
    shift 3

    verify_args=(security verify-cert -c "$leaf_certificate")
    while (($#)); do
        verify_args+=(-c "$1")
        shift
    done
    verify_args+=(-p "$policy" -L -P)
    if ! "${verify_args[@]}" >"$chain_output" 2>"$chain_stderr"; then
        fail "$label certificate chain is not trusted under $policy policy"
    fi

    chain_count="$(split_pem_chain "$chain_output" "$chain_prefix")"
    ((chain_count > 0)) || fail "$label trust result contained no certificate chain"
    root_index=$((chain_count - 1))
    root_certificate="${chain_prefix}${root_index}.pem"
    verified_root_sha256="$(pem_sha256 "$root_certificate")"

    security find-certificate -a -c 'Apple Root CA' -p \
        "$system_root_keychain" >"$root_candidates"
    root_count="$(split_pem_chain "$root_candidates" "$root_prefix")"
    ((root_count > 0)) || fail "system root keychain contains no Apple Root CA certificates"
    candidate_index=0
    while ((candidate_index < root_count)); do
        candidate_hash="$(pem_sha256 "${root_prefix}${candidate_index}.pem")"
        if [[ "$candidate_hash" == "$verified_root_sha256" ]]; then
            root_match=1
            break
        fi
        candidate_index=$((candidate_index + 1))
    done
    ((root_match == 1)) || \
        fail "$label trusted chain does not terminate at an Apple system root"
    verified_chain_count="$chain_count"
}

mode="${1:-}"
if [[ -z "$mode" ]]; then
    usage
    exit 1
fi
if [[ "$mode" == "-h" || "$mode" == "--help" ]]; then
    usage
    exit 0
fi
case "$mode" in
    profile|app) ;;
    *) fail "unknown mode: $mode" ;;
esac
shift

profile_path=""
app_path=""
requested_output_dir=""
while (($#)); do
    case "$1" in
        --profile|--app|--output-dir)
            (($# >= 2)) || fail "$1 requires a value"
            case "$1" in
                --profile) profile_path="$2" ;;
                --app) app_path="$2" ;;
                --output-dir) requested_output_dir="$2" ;;
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

for command_name in awk basename codesign dirname grep mkdir plutil security \
    shasum uname; do
    require_command "$command_name"
done
[[ "$(uname -s)" == "Darwin" ]] || fail "iOS signing trust verification requires macOS"
openssl_bin='/usr/bin/openssl'
[[ -x "$openssl_bin" ]] || fail "required system tool not found: $openssl_bin"
system_root_keychain='/System/Library/Keychains/SystemRootCertificates.keychain'
[[ -f "$system_root_keychain" ]] || \
    fail "system root keychain not found: $system_root_keychain"

[[ -n "$requested_output_dir" ]] || fail "--output-dir is required"
if [[ "$requested_output_dir" != /* ]]; then
    requested_output_dir="$PWD/$requested_output_dir"
fi
output_parent="$(dirname "$requested_output_dir")"
[[ -d "$output_parent" ]] || fail "output parent directory not found: $output_parent"
output_parent="$(cd "$output_parent" && pwd -P)"
output_dir="$output_parent/$(basename "$requested_output_dir")"
[[ ! -e "$output_dir" ]] || fail "output directory already exists: $output_dir"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(cd "$script_dir/.." && pwd -P)"
case "$output_dir" in
    "$repo_root"|"$repo_root"/*)
        require_command git
        git -C "$repo_root" check-ignore -q -- "$output_dir" || \
            fail "in-repository trust evidence path must be gitignored: $output_dir"
        ;;
esac
mkdir "$output_dir"

case "$mode" in
    profile)
        [[ -n "$profile_path" ]] || fail "--profile is required for profile mode"
        [[ -z "$app_path" ]] || fail "--app is valid only in app mode"
        [[ -f "$profile_path" ]] || fail "profile not found: $profile_path"
        profile_parent="$(cd "$(dirname "$profile_path")" && pwd -P)"
        profile_path="$profile_parent/$(basename "$profile_path")"
        profile_decoded="$output_dir/profile-decoded.plist"
        profile_signer="$output_dir/profile-signer.pem"
        profile_certificates="$output_dir/profile-certificates.pem"
        profile_crypto_log="$output_dir/profile-cryptographic-verify.txt"
        if ! "$openssl_bin" cms -verify -binary -inform DER \
            -in "$profile_path" -noverify -out "$profile_decoded" \
            -signer "$profile_signer" -certsout "$profile_certificates" \
            >"$profile_crypto_log" 2>&1; then
            fail "provisioning profile CMS signature is invalid"
        fi
        plutil -lint "$profile_decoded" >"$output_dir/profile-plist-lint.txt"
        signer_count="$(split_pem_chain "$profile_signer" \
            "$output_dir/profile-signer-certificate-")"
        [[ "$signer_count" == "1" ]] || \
            fail "expected exactly one provisioning-profile CMS signer; found $signer_count"
        certificate_count="$(split_pem_chain "$profile_certificates" \
            "$output_dir/profile-certificate-")"
        ((certificate_count > 0)) || \
            fail "provisioning profile CMS contains no certificates"
        signer_certificate="$output_dir/profile-signer-certificate-0.pem"
        signer_sha256="$(pem_sha256 "$signer_certificate")"
        chain_certificates=()
        certificate_index=0
        while ((certificate_index < certificate_count)); do
            candidate="$output_dir/profile-certificate-$certificate_index.pem"
            candidate_sha256="$(pem_sha256 "$candidate")"
            if [[ "$candidate_sha256" != "$signer_sha256" ]]; then
                chain_certificates+=("$candidate")
            fi
            certificate_index=$((certificate_index + 1))
        done
        if ((${#chain_certificates[@]})); then
            pin_verified_chain_to_apple_root profile-signer basic \
                "$signer_certificate" "${chain_certificates[@]}"
        else
            pin_verified_chain_to_apple_root profile-signer basic \
                "$signer_certificate"
        fi
        "$openssl_bin" x509 -in "$signer_certificate" -noout \
            -subject -issuer -nameopt RFC2253 \
            >"$output_dir/profile-signer-subject.txt"
        grep -Eq '(^|subject=|,)CN=Apple (iPhone OS )?Provisioning Profile Signing(,|$)' \
            "$output_dir/profile-signer-subject.txt" || \
            fail "CMS signer is not an Apple provisioning-profile signing certificate"
        {
            printf 'TYPE=profile\n'
            printf 'CMS_SIGNATURE_STATUS=verified\n'
            printf 'CERTIFICATE_TRUST_STATUS=verified\n'
            printf 'APPLE_ROOT_PIN_STATUS=verified\n'
            printf 'PROVISIONING_PROFILE_SIGNER_PURPOSE_STATUS=verified\n'
            printf 'SIGNER_CERTIFICATE_SHA256=%s\n' "$signer_sha256"
            printf 'TRUSTED_CHAIN_CERTIFICATE_COUNT=%s\n' "$verified_chain_count"
            printf 'TRUSTED_ROOT_CERTIFICATE_SHA256=%s\n' "$verified_root_sha256"
            printf 'DECODED_PLIST=%s\n' "$profile_decoded"
        } >"$output_dir/trust-manifest.txt"
        printf 'PROFILE_TRUST_VERIFIED=%s\n' "$output_dir"
        ;;
    app)
        [[ -n "$app_path" ]] || fail "--app is required for app mode"
        [[ -z "$profile_path" ]] || fail "--profile is valid only in profile mode"
        [[ -d "$app_path" ]] || fail "app bundle not found: $app_path"
        app_parent="$(cd "$(dirname "$app_path")" && pwd -P)"
        app_path="$app_parent/$(basename "$app_path")"
        if ! codesign --verify --deep --strict --verbose=2 "$app_path" \
            >"$output_dir/app-codesign-verify.txt" 2>&1; then
            fail "app code signature failed strict verification"
        fi
        certificate_prefix="$output_dir/app-certificate-"
        if ! codesign --display "--extract-certificates=$certificate_prefix" \
            "$app_path" >"$output_dir/app-certificate-extract.txt" 2>&1; then
            fail "could not extract app signing certificate chain"
        fi
        certificate_count=0
        while [[ -f "${certificate_prefix}${certificate_count}" ]]; do
            certificate_count=$((certificate_count + 1))
        done
        ((certificate_count > 0)) || fail "app signature has no certificate chain"
        leaf_certificate="${certificate_prefix}0"
        leaf_sha256="$(shasum -a 256 "$leaf_certificate" | awk '{print $1}')"
        chain_certificates=()
        certificate_index=1
        while ((certificate_index < certificate_count)); do
            chain_certificates+=("${certificate_prefix}${certificate_index}")
            certificate_index=$((certificate_index + 1))
        done
        if ((${#chain_certificates[@]})); then
            pin_verified_chain_to_apple_root app-signer codeSign \
                "$leaf_certificate" "${chain_certificates[@]}"
        else
            pin_verified_chain_to_apple_root app-signer codeSign \
                "$leaf_certificate"
        fi
        "$openssl_bin" x509 -inform DER -in "$leaf_certificate" \
            -noout -subject -issuer >"$output_dir/app-signer-subject.txt"
        {
            printf 'TYPE=app\n'
            printf 'CODE_SIGNATURE_STATUS=verified\n'
            printf 'CERTIFICATE_TRUST_STATUS=verified\n'
            printf 'APPLE_ROOT_PIN_STATUS=verified\n'
            printf 'LEAF_CERTIFICATE_SHA256=%s\n' "$leaf_sha256"
            printf 'EXTRACTED_CHAIN_CERTIFICATE_COUNT=%s\n' "$certificate_count"
            printf 'TRUSTED_CHAIN_CERTIFICATE_COUNT=%s\n' "$verified_chain_count"
            printf 'TRUSTED_ROOT_CERTIFICATE_SHA256=%s\n' "$verified_root_sha256"
        } >"$output_dir/trust-manifest.txt"
        printf 'APP_TRUST_VERIFIED=%s\n' "$output_dir"
        ;;
esac
