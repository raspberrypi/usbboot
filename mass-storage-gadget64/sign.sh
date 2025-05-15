#!/bin/sh

set -e
set -u
script_dir="$(cd "$(dirname "$0")" && pwd)"

TMP_DIR=""
SIGN_ARGS=""
PUBLIC_KEY=""

die() {
   echo "$@" >&2
   exit 1
}

cleanup() {
   if [ -d "${TMP_DIR}" ]; then rm -rf "${TMP_DIR}"; fi
}

sign_firmware_blob() {
   echo "Signing firmware in ${1} using $(which rpi-sign-bootcode)"
   rpi-sign-bootcode \
      -c 2712 \
      -i "${1}" \
      -o "${2}" \
      -n 16 \
      -v 0 \
      ${SIGN_ARGS} ${PUBLIC_KEY}
}

sign_bootfiles() {
   echo "Signing OS image ${1}"
   input="${1}"
   output="${2}"
   (
      cd "${TMP_DIR}"
      tar -xf "${input}"
      echo "Signing 2712/bootcode5.bin"
      sign_firmware_blob 2712/bootcode5.bin 2712/bootcode5.bin.signed || die "Failed to sign bootcode5.bin"
      mv -f "2712/bootcode5.bin.signed" "2712/bootcode5.bin"
      tar -cf "${output}" *
      find .
   )
}

trap cleanup EXIT

if [ "${1}" = "-H" ]; then
   HSM_WRAPPER="${2}"
   PUBLIC_KEY="${3}"
   [ -f "${PUBLIC_KEY}" ] || die "HSM requires a public key file in PEM format. Public key \"${PUBLIC_KEY}\" not found."
   PUBLIC_KEY="-p ${3}"
   if ! command -v "${HSM_WRAPPER}"; then
      die "HSM wrapper script \"${HSM_WRAPPER}\" not found"
   fi
   SIGN_ARGS="-H ${HSM_WRAPPER}"
else
   KEY_FILE="${1}"
   [ -f "${KEY_FILE}" ] || die "KEY_FILE: ${KEY_FILE} not found"
   SIGN_ARGS="-k ${KEY_FILE}"
fi

PATH="${script_dir}/../tools:${PATH}"
KEY_FILE="${1}"
TMP_DIR="$(mktemp -d)"
rm -f bootfiles.bin
ln -sf ../firmware/bootfiles.bin bootfiles.original.bin
sign_bootfiles "$(pwd)/bootfiles.original.bin" "$(pwd)/bootfiles.bin"

echo "Signing boot.img with ${SIGN_ARGS}"
rpi-eeprom-digest -i boot.img -o boot.sig ${SIGN_ARGS}
