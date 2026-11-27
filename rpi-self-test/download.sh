#!/bin/sh

# Downloads the latest rpi-self-test boot.img. The image is not stored in
# this repository so that the current version is always used.

set -e
set -u

URL="${RPI_SELF_TEST_URL:-https://github.com/raspberrypi/buildroot/releases/download/rpi-self-test/boot.img}"

cd "$(dirname "$0")"
wget -O boot.img.tmp "${URL}"
mv -f boot.img.tmp boot.img
# A signature for a previous image is no longer valid.
rm -f boot.sig
echo "Downloaded boot.img"
sha256sum boot.img
