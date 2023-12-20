#!/bin/sh

# Utility to update the EEPROM image (pieeprom.bin) and signature
# (pieeprom.sig) with a new EEPROM config.
#
# This script is now a thin wrapper for the new version in ../tools
#
# pieeprom.original.bin - The source EEPROM from rpi-eeprom repo
# boot.conf - The bootloader config file to apply.

set -e

../tools/update-pieeprom.sh "$@"
