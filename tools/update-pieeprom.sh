#!/bin/sh

# Utility to update the EEPROM image (pieeprom.bin) and signature
# (pieeprom.sig) with a new EEPROM config.
#
# pieeprom.original.bin - The source EEPROM from rpi-eeprom repo
# boot.conf - The bootloader config file to apply.

set -e

script_dir="$(cd "$(dirname "$0")" && pwd)"

# Minimum version for secure-boot support
BOOTLOADER_SECURE_BOOT_MIN_VERSION=1632136573
SRC_IMAGE="pieeprom.original.bin"
CONFIG="boot.conf"
DST_IMAGE="pieeprom.bin"
PEM_FILE=""
PUBLIC_PEM_FILE=""
TMP_CONFIG_SIG=""

die() {
   echo "$@" >&2
   exit ${EXIT_FAILED}
}

cleanup() {
   if [ -f "${TMP_CONFIG}" ]; then rm -f "${TMP_CONFIG}"; fi
}

usage() {
cat <<EOF
    update-pieeprom.sh [key]

    Updates and EEPROM image by replacing the configuration file and generates
    the new pieeprom.sig file.

    The bootloader configuration may also be signed by specifying the name
    of an .pem with containing a 2048bit RSA public/private key pair.

    RSA signature support requires the Python Crypto module. To install:
    python3 -m pip install Crypto

    -c Bootloader config file - default: "${CONFIG}"
    -i Source EEPROM image - default: "${SRC_IMAGE}"
    -o Output EEPROM image - default: "${DST_IMAGE}"
    -k Optional RSA private key PEM file.
    -p Optional RSA public key PEM file.

The -k argument signs the EEPROM configuration using the specified RSA 2048
bit private key in PEM format. It also embeds the public portion of the RSA
key pair in the EEPROM image so that the bootloader can verify the signed OS
image.

If the public key is not specified then rpi-eeprom-config will extract this
automatically from the private key. Typically, the [-p] public key argument
would only be used if rpi-eeprom-digest has been modified to use a hardware
security module instead of a private key file.

EOF
}

update_eeprom() {
    src_image="$1"
    config="$2"
    dst_image="$3"
    pem_file="$4"
    public_pem_file="$5"
    sign_args=""

    if [ -n "${pem_file}" ]; then
        if ! grep -q "SIGNED_BOOT=1" "${CONFIG}"; then
            # If the OTP bit to require secure boot are set then then
            # SIGNED_BOOT=1 is implicitly set in the EEPROM config.
            # For debug in signed-boot mode it's normally useful to set this
            echo "Warning: SIGNED_BOOT=1 not found in \"${CONFIG}\""
        fi
        update_version=$(strings "${src_image}" | grep BUILD_TIMESTAMP | sed 's/.*=//g')
        if [ "${BOOTLOADER_SECURE_BOOT_MIN_VERSION}" -gt "${update_version}" ]; then
            die "Source bootloader image ${src_image} does not support secure-boot. Please use a newer verison."
        fi

        TMP_CONFIG_SIG="$(mktemp)"
        echo "Signing bootloader config"
        "${script_dir}/rpi-eeprom-digest" \
            -i "${config}" -o "${TMP_CONFIG_SIG}" \
            -k "${pem_file}" || die "Failed to sign EEPROM config"

        cat "${TMP_CONFIG_SIG}"

        # rpi-eeprom-config extracts the public key args from the specified
        # PEM file. It will also accept just the public key so it's possible
        # to tweak this script so that rpi-eeprom-config never sees the private
        # key.
        sign_args="-d ${TMP_CONFIG_SIG} -p ${public_pem_file}"
    fi

    rm -f "${dst_image}"
    set -x
    ${script_dir}/rpi-eeprom-config \
        --config "${config}" \
        --out "${dst_image}" ${sign_args} \
        "${src_image}" || die "Failed to update EEPROM image"
    set +x

cat <<EOF
new-image: ${dst_image}
source-image: ${src_image}
config: ${config}
EOF
}

image_digest() {
    "${script_dir}/rpi-eeprom-digest" \
        -i "${1}" -o "${2}"
}

trap cleanup EXIT

while getopts "c:hi:o:k:p:" option; do
    case "${option}" in
        c) CONFIG="${OPTARG}"
            ;;
        i) SRC_IMAGE="${OPTARG}"
            ;;
        o) DST_IMAGE="${OPTARG}"
            ;;
        k) PEM_FILE="${OPTARG}"
            ;;
        p) PUBLIC_PEM_FILE="${OPTARG}"
            ;;
        h) usage
            ;;
        *) echo "Unknown argument \"${option}\""
            usage
            ;;
    esac
done

[ -f "${SRC_IMAGE}" ] || die "Source image \"${SRC_IMAGE}\" not found"
[ -f "${CONFIG}" ] || die "Bootloader config file \"${CONFIG}\" not found"
if [ -n "${PEM_FILE}" ]; then
    [ -f "${PEM_FILE}" ] || die "RSA key file \"${PEM_FILE}\" not found"
fi

# If a public key is specified then use it. Otherwise, if just the private
# key is specified then let rpi-eeprom-config automatically extract the
# public key from the private key PEM file.
if [ -z "${PUBLIC_PEM_FILE}" ]; then
    PUBLIC_PEM_FILE="${PEM_FILE}"
fi

DST_IMAGE_SIG="$(echo "${DST_IMAGE}" | sed 's/\.[^./]*$//').sig"

update_eeprom "${SRC_IMAGE}" "${CONFIG}" "${DST_IMAGE}" "${PEM_FILE}" "${PUBLIC_PEM_FILE}"
image_digest "${DST_IMAGE}" "${DST_IMAGE_SIG}"

