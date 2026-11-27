#!/bin/sh

# Downloads the rpi-self-test boot.img from the release on the Raspberry Pi
# buildroot repo. The image is not stored in this repository.
#
# https://github.com/raspberrypi/buildroot/releases/tag/rpi-self-test-2026-11-27
#
# Set RPI_SELF_TEST_URL to download a different release. The checksum is only
# verified for the default release.

set -e
set -u

RELEASE="rpi-self-test-2026-11-27"
BOOT_IMG_SHA256="e9e0f5189574423af4c852d2e02f7a1c2024a15f950b471dd835fa6564ab9a3a"
DEFAULT_URL="https://github.com/raspberrypi/buildroot/releases/download/${RELEASE}/${RELEASE}.zip"
URL="${RPI_SELF_TEST_URL:-${DEFAULT_URL}}"

cd "$(dirname "$0")"
trap 'rm -f boot.img.zip boot.img.tmp' EXIT
wget -O boot.img.zip "${URL}"
unzip -p boot.img.zip boot.img > boot.img.tmp
if [ "${URL}" = "${DEFAULT_URL}" ]; then
	echo "${BOOT_IMG_SHA256}  boot.img.tmp" | sha256sum -c - > /dev/null || {
		echo "boot.img does not match the checksum of ${RELEASE}" >&2
		exit 1
	}
fi
mv -f boot.img.tmp boot.img
# A signature for a previous image is no longer valid.
rm -f boot.sig
echo "Downloaded boot.img (${URL})"
sha256sum boot.img
