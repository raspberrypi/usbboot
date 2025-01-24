#!/bin/sh

set -e
script_dir="$(cd "$(dirname "$0")" && pwd)"

TMP_DIR=""

die() {
   echo "$@" >&2
   exit 1
}

cleanup() {
   if [ -d "${TMP_DIR}" ]; then rm -rf "${TMP_DIR}"; fi
}

sign_firmware_blob() {
   echo "Signing firmware in ${1}"
   [ -f "${KEY_FILE}" ] || die "sign-firmware: key-file ${KEY_FILE} not found"
   rpi-sign-bootcode \
      -c 2712 \
      -i "${1}" \
      -o "${2}" \
      -n 16 \
      -v 0 \
      -k "${KEY_FILE}"
}

sign_bootfiles() {
   echo "Signing OS image ${1}"
   input="${1}"
   output="${2}"
   (
      cd "${TMP_DIR}"
      tar -xf "${input}"
      echo "Signing 2712/bootcode5.bin"
      sign_firmware_blob 2712/bootcode5.bin 2712/bootcode5.bin.signed "${KEY_FILE}" || die "Failed to sign bootcode5.bin"
      mv -f "2712/bootcode5.bin.signed" "2712/bootcode5.bin"
      tar -cf "${output}" *
      find .
   )

}

trap cleanup EXIT

KEY_FILE="${1}"
[ -f "${KEY_FILE}" ] || die "KEY_FILE: ${KEY_FILE} not found"

PATH="${script_dir}/../tools:${PATH}"
KEY_FILE="${1}"
TMP_DIR="$(mktemp -d)"
rm -f bootfiles.bin
ln -sf ../firmware/bootfiles.bin bootfiles.original.bin
sign_bootfiles "$(pwd)/bootfiles.original.bin" "$(pwd)/bootfiles.bin" "${KEY_FILE}"

echo "Signing boot.img with ${KEY_FILE}"
rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
