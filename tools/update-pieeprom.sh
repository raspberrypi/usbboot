#!/bin/sh

# Utility to update the EEPROM image (pieeprom.bin) and signature
# (pieeprom.sig) with a new EEPROM config.
#
# pieeprom.original.bin - The source EEPROM from rpi-eeprom repo
# boot.conf - The bootloader config file to apply.

set -e
set -u

script_dir="$(cd "$(dirname "$0")" && pwd)"
export PATH="${script_dir}:${PATH}"

# Minimum version for secure-boot support
BOOTLOADER_SECURE_BOOT_MIN_VERSION=1632136573
SRC_IMAGE="pieeprom.original.bin"
CONFIG="boot.conf"
DST_IMAGE="pieeprom.bin"
PEM_FILE=""
PUBLIC_PEM_FILE=""
TMP_CONFIG_SIG=""
SIGN_RECOVERY=0
TMP_DIR=""
SIGN_FIRMWARE=${SIGN_FIRMWARE:-0}
HSM_WRAPPER=""

die() {
   echo "$@" >&2
   exit 1
}

cleanup() {
   if [ -f "${TMP_CONFIG_SIG}" ]; then rm -f "${TMP_CONFIG_SIG}"; fi
   if [ -d "${TMP_DIR}" ]; then rm -rf "${TMP_DIR}"; fi
}

usage() {
cat <<EOF
    update-pieeprom.sh [key]

    Updates an EEPROM image by replacing the configuration file and generates
    the new pieeprom.sig file.

    The bootloader configuration may also be signed by specifying the name
    of an .pem with containing a 2048bit RSA public/private key pair.

    RSA signature support requires the Python Crypto module. To install:
    python3 -m pip install Crypto

    -c Bootloader config file - default: "${CONFIG}"
    -i Source EEPROM image - default: "${SRC_IMAGE}"
    -o Output EEPROM image - default: "${DST_IMAGE}"
    -H The name of the HSM wrapper script to invoke - default ""
    -k Optional RSA private key PEM file.
    -p Optional RSA public key PEM file - required if using a HSM wrapper

    Flags:
    -f Countersign the EEPROM firmware
    -r Countersign recovery.bin. Once secure-boot has been enabled (but not
       before!) the recovery.bin file must be countersign in addition to
       bootcode.bin.

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

    if [ -n "${pem_file}" ] || [ -n "${HSM_WRAPPER}" ]; then
        update_version=$(strings "${src_image}" | grep BUILD_TIMESTAMP | sed 's/.*=//g')
        if [ "${BOOTLOADER_SECURE_BOOT_MIN_VERSION}" -gt "${update_version}" ]; then
            die "Source bootloader image ${src_image} does not support secure-boot. Please use a newer version."
        fi

        TMP_CONFIG_SIG="$(mktemp)"
        echo "Signing bootloader config"
        if [ -n "${HSM_WRAPPER}" ]; then
           rpi-eeprom-digest \
              -i "${config}" -o "${TMP_CONFIG_SIG}" \
              -H "${HSM_WRAPPER}" || die "Failed to sign EEPROM config using HSM"
        else
           rpi-eeprom-digest \
              -i "${config}" -o "${TMP_CONFIG_SIG}" \
              -k "${pem_file}" || die "Failed to sign EEPROM config"
        fi

        cat "${TMP_CONFIG_SIG}"

        # rpi-eeprom-config extracts the public key args from the specified
        # PEM file. It will also accept just the public key so it's possible
        # to tweak this script so that rpi-eeprom-config never sees the private
        # key.
        sign_args="-d ${TMP_CONFIG_SIG} -p ${public_pem_file}"
    fi

    rm -f "${dst_image}"
    set -x
    rpi-eeprom-config \
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
    rpi-eeprom-digest -i "${1}" -o "${2}"
}

sign_firmware_blob() {

  if [ -n "${HSM_WRAPPER}" ]; then
      rpi-sign-bootcode \
         -c 2712 \
         -i "${1}" \
         -o "${2}" \
         -n 16 \
         -v 0 \
         -p "${PUBLIC_PEM_FILE}" \
         -H "${HSM_WRAPPER}" || die "Failed to sign firmware blob using HSM wrapper"

  else

      [ -f "${PEM_FILE}" ] || die "sign-firmware: key-file ${PEM_FILE} not found"
      rpi-sign-bootcode \
         -c 2712 \
         -i "${1}" \
         -o "${2}" \
         -n 16 \
         -v 0 \
         -k "${PEM_FILE}" || die "Failed to sign firmware blob using private key"
   fi
}

sign_firmware() {
   if [ "${SIGN_FIRMWARE}" = 1 ]; then
      echo "SIGN_RECOVERY: ${SIGN_RECOVERY}"

      if [ "${SIGN_RECOVERY}" = 1 ]; then
         # Run from the secure-boot-recovery directory so sign recovery.bin as well
         recovery_src="recovery.original.bin"
         if [ -f "${recovery_src}" ]; then
            echo "Signing ${recovery_src} as bootcode5.bin"
            rm -f "bootcode5.bin"
            sign_firmware_blob "${recovery_src}" bootcode5.bin
         fi
      else
         echo "Using unsigned recovery.bin"
         rm -f bootcode5.bin
         cp -fv recovery.original.bin bootcode5.bin
      fi

      # Extract bootcode.bin from the bootloader image and sign it
      pieeprom_src="${1}"
      pieeprom_dst="pieeprom.signed_boot.bin"
      if [ -f "${pieeprom_src}" ]; then
         echo "Signing ${pieeprom_src} as ${pieeprom_dst}"
         (
            cp -f "${pieeprom_src}" "${TMP_DIR}"
            cd "${TMP_DIR}"
            rpi-eeprom-config -x "${pieeprom_src}"
         )
         sign_firmware_blob "${TMP_DIR}/bootcode.bin" "${TMP_DIR}/bootcode.bin.signed"
         rpi-eeprom-config --bootcode "${TMP_DIR}/bootcode.bin.signed" -o "${pieeprom_dst}" "${pieeprom_src}"
         SRC_IMAGE="${pieeprom_dst}"
      fi
   fi
}

trap cleanup EXIT

while getopts "c:i:o:k:p:H:fhr" option; do
    case "${option}" in
        c) CONFIG="${OPTARG}"
            ;;
        i) SRC_IMAGE="${OPTARG}"
            ;;
        o) DST_IMAGE="${OPTARG}"
            ;;
        k) PEM_FILE="${OPTARG}"
           [ -n "${PEM_FILE}" ] || die "Private key not specified [-k]"
           [ -f "${PEM_FILE}" ] || die "Private key file ${PEM_FILE} not found"
            ;;
        p) PUBLIC_PEM_FILE="${OPTARG}"
            ;;
        f) SIGN_FIRMWARE=1
            ;;
        r) SIGN_RECOVERY=1
           ;;
        h) usage
           exit 0
           ;;
        H) HSM_WRAPPER="${OPTARG}"
           ;;
        *) echo "Unknown argument \"${option}\""
            usage
            ;;
    esac
done

# If this is run from the source repo the check that the submodule has been update.
# The APT installed version does not contain an rpi-eeprom directory.
if [ -d ../rpi-eeprom ]; then
   submodule_check="${script_dir}/../rpi-eeprom/firmware-2711/default/recovery.bin"
   if [ ! -f "${submodule_check}" ]; then
      echo "WARNING:${submodule_check} not found. To update the rpi-eeprom submodule run:"
      echo "git submodule init && git submodule update"
   fi
fi

[ -f "${SRC_IMAGE}" ] || die "Source image \"${SRC_IMAGE}\" not found"
[ -f "${CONFIG}" ] || die "Bootloader config file \"${CONFIG}\" not found"
if [ -n "${PEM_FILE}" ]; then
    [ -f "${PEM_FILE}" ] || die "RSA key file \"${PEM_FILE}\" not found"
fi

if [ -n "${HSM_WRAPPER}" ]; then
   if ! command -v "${HSM_WRAPPER}" > /dev/null; then
      die "HSM wrapper script \"${HSM_WRAPPER}\" not found"
   fi
   [ -n "${PUBLIC_PEM_FILE}" ] || die "Public key (-p public.pem) must specified in HSM mode"
   [ -f "${PUBLIC_PEM_FILE}" ] || die "Public key \"${PUBLIC_PEM_FILE}\" not found"
fi

# If a public key is specified then use it. Otherwise, if just the private
# key is specified then let rpi-eeprom-config automatically extract the
# public key from the private key PEM file.
if [ -z "${PUBLIC_PEM_FILE}" ]; then
    PUBLIC_PEM_FILE="${PEM_FILE}"
fi

DST_IMAGE_SIG="$(echo "${DST_IMAGE}" | sed 's/\.[^./]*$//').sig"
TMP_DIR="$(mktemp -d)"
rm -f "${DST_IMAGE}" "${DST_IMAGE_SIG}"
sign_firmware "${SRC_IMAGE}"
update_eeprom "${SRC_IMAGE}" "${CONFIG}" "${DST_IMAGE}" "${PEM_FILE}" "${PUBLIC_PEM_FILE}"
image_digest "${DST_IMAGE}" "${DST_IMAGE_SIG}"
