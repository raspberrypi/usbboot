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
TMP_CONFIG_SIG=""

die() {
   echo "$@" >&2
   exit ${EXIT_FAILED}
}

cleanup() {
   [ -f "${TMP_CONFIG}" ] && rm -f "${TMP_CONFIG}"
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

    -c Bootloader config file - default: "${SRC_IMAGE}"
    -i Source EEPROM image - default: "${CONFIG}"
    -o Output EEPROM image - default: "${DST_IMAGE}"
    -k Optional RSA private PEM file - default: "${PEM_FILE}"

The -k argument signs the EEPROM configuration using the specified RSA 2048
bit private key in PEM format. It also embeds the public portion of the RSA
key pair in the EEPROM image so that the bootloader can verify the signed OS
image.
EOF
}

update_eeprom() {
    src_image="$1"
    config="$2"
    dst_image="$3"
    pem_file="$4"
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
        sign_args="-d ${TMP_CONFIG_SIG} -p ${pem_file}"
    fi

    rm -f "${dst_image}"
    ${script_dir}/rpi-eeprom-config \
        --config "${config}" \
        --out "${dst_image}" ${sign_args} \
        "${src_image}" || die "Failed to update EEPROM image"

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

while getopts "c:hi:o:k:" option; do
    case "${option}" in
        c) CONFIG="${OPTARG}"
            ;;
        i) SRC_IMAGE="${OPTARG}"
            ;;
        o) DST_IMAGE="${OPTARG}"
            ;;
        k) PEM_FILE="${OPTARG}"
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

DST_IMAGE_SIG="$(echo "${DST_IMAGE}" | sed 's/\..*//').sig"

update_eeprom "${SRC_IMAGE}" "${CONFIG}" "${DST_IMAGE}" "${PEM_FILE}"
image_digest "${DST_IMAGE}" "${DST_IMAGE_SIG}"

