#!/bin/sh

# Utility to update the EEPROM image (pieeprom.bin) and signature
# (pieeprom.sig) with a new EEPROM config.
#
# pieeprom.original.bin - The source EEPROM from rpi-eeprom repo
# boot.conf - The bootloader config file to apply.

set -e

script_dir="$(cd "$(dirname "$0")" && pwd)" 

${script_dir}/rpi-eeprom-config --config ${script_dir}/boot.conf --out ${script_dir}/pieeprom.bin ${script_dir}/pieeprom.original.bin
sha256sum ${script_dir}/pieeprom.bin | awk '{print $1}' > ${script_dir}/pieeprom.sig
echo "ts: $(date -u +%s)" >> "${script_dir}/pieeprom.sig"
