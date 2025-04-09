#!/bin/sh

# Script to validate that the example-hsm-wrapper produces bitwise identical
# signatures to direct signing with the private key

set -e
set -u

script_dir="$(cd "$(dirname "$0")" && pwd)"
export PATH="${script_dir}/../tools:${PATH}"

# Set SOURCE_DATE_EPOCH to current timestamp to ensure consistent timestamps
export SOURCE_DATE_EPOCH=$(date +%s)
echo "Using timestamp: ${SOURCE_DATE_EPOCH}"

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

cleanup() {
   if [ -n "${TMP_DIR:-}" ] && [ -d "${TMP_DIR}" ]; then
      echo "Cleaning up temporary directory: ${TMP_DIR}"
      rm -rf "${TMP_DIR}"
   fi
}

# Get the script directory
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
USBBOOT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
HSM_WRAPPER="${USBBOOT_DIR}/secure-boot-example/example-hsm-wrapper"
PRIVATE_KEY="${USBBOOT_DIR}/secure-boot-example/example-private.pem"
PUBLIC_KEY="-p ${USBBOOT_DIR}/secure-boot-example/example-public.pem"
FIRMWARE_DIR="${USBBOOT_DIR}/firmware"
RECOVERY_DIR="${USBBOOT_DIR}/secure-boot-recovery5"

# Check if bootfiles.bin exists
if [ ! -f "${FIRMWARE_DIR}/bootfiles.bin" ]; then
    echo "ERROR: bootfiles.bin not found in ${FIRMWARE_DIR}"
    echo "Please make sure the firmware directory contains bootfiles.bin"
    exit 1
fi

# Check if pieeprom.original.bin exists
if [ ! -e "${RECOVERY_DIR}/pieeprom.original.bin" ]; then
    echo "ERROR: pieeprom.original.bin not found in ${RECOVERY_DIR}"
    echo "Please make sure the secure-boot-recovery5 directory contains pieeprom.original.bin"
    exit 1
fi

# Create a temporary directory for our test files
TMP_DIR=$(mktemp -d)
trap cleanup EXIT

test_sign_bootcode() {
    echo ""
    echo "======================================================================"
    echo "TEST: BOOTCODE SIGNING COMPARISON"
    echo "======================================================================"
    echo ""

    # Copy bootfiles.bin to our test directory
    BOOTFILES="${TMP_DIR}/bootfiles.bin"
    cp "${FIRMWARE_DIR}/bootfiles.bin" "${BOOTFILES}"

    # Extract bootcode5.bin from bootfiles.bin using tar
    echo "Extracting 2712/bootcode5.bin from bootfiles.bin"
    cd "${TMP_DIR}"
    tar -xf "${BOOTFILES}" 2712/bootcode5.bin
    BOOTCODE="${TMP_DIR}/2712/bootcode5.bin"

    if [ ! -f "${BOOTCODE}" ]; then
        echo "ERROR: Failed to extract 2712/bootcode5.bin from bootfiles.bin"
        exit 1
    fi

    # Sign using private key mode
    echo ""
    echo "----------------------------------------------------------------------"
    echo "PRIVATE KEY SIGNING MODE"
    echo "----------------------------------------------------------------------"
    echo ""
    SIGN_ARGS="-k ${PRIVATE_KEY}"
    PRIVATE_SIGNED="${TMP_DIR}/bootcode5.private.signed"
    sign_firmware_blob "${BOOTCODE}" "${PRIVATE_SIGNED}"

    # Sign using HSM wrapper mode
    echo ""
    echo "----------------------------------------------------------------------"
    echo "HSM WRAPPER SIGNING MODE"
    echo "----------------------------------------------------------------------"
    echo ""
    SIGN_ARGS="-H ${HSM_WRAPPER}"
    HSM_SIGNED="${TMP_DIR}/bootcode5.hsm.signed"
    sign_firmware_blob "${BOOTCODE}" "${HSM_SIGNED}"

    # Compare the signed files
    echo ""
    echo "----------------------------------------------------------------------"
    echo "COMPARING SIGNED FILES"
    echo "----------------------------------------------------------------------"
    echo ""
    if cmp -s "${PRIVATE_SIGNED}" "${HSM_SIGNED}"; then
        echo "SUCCESS: HSM wrapper and private key signing produce identical output"
        return 0
    else
        echo "ERROR: HSM wrapper and private key signing produce different output"
        echo "Files differ. Check the following files for details:"
        echo "  Private key signed: ${PRIVATE_SIGNED}"
        echo "  HSM wrapper signed: ${HSM_SIGNED}"
        return 1
    fi 
}

test_sign_eeprom() {
    echo ""
    echo "======================================================================"
    echo "TEST: EEPROM SIGNING COMPARISON"
    echo "======================================================================"
    echo ""
    
    # Create a test directory for EEPROM signing
    EEPROM_TEST_DIR="${TMP_DIR}/eeprom_test"
    mkdir -p "${EEPROM_TEST_DIR}"
    
    # Copy pieeprom.original.bin to our test directory
    cp "${RECOVERY_DIR}/pieeprom.original.bin" "${EEPROM_TEST_DIR}/"
    cp "${RECOVERY_DIR}/recovery.original.bin" "${EEPROM_TEST_DIR}/"
    
    # Create a minimal boot.conf file
    cat > "${EEPROM_TEST_DIR}/boot.conf" << EOF
[all]
BOOT_UART=1
POWER_OFF_ON_HALT=1
EOF
    
    # Sign using private key mode
    echo ""
    echo "----------------------------------------------------------------------"
    echo "PRIVATE KEY SIGNING MODE"
    echo "----------------------------------------------------------------------"
    echo ""
    cd "${EEPROM_TEST_DIR}"
    update-pieeprom.sh -c boot.conf -k "${PRIVATE_KEY}" -f -o pieeprom.private.bin
    PRIVATE_SIGNED="${EEPROM_TEST_DIR}/pieeprom.private.bin"
    
    # Sleep to verify that SOURCE_DATE_EPOCH is used
    sleep 1

    # Sign using HSM wrapper mode
    echo ""
    echo "----------------------------------------------------------------------"
    echo "HSM WRAPPER SIGNING MODE"
    echo "----------------------------------------------------------------------"
    echo ""
    cd "${EEPROM_TEST_DIR}"
    update-pieeprom.sh -c boot.conf -H "${HSM_WRAPPER}" -p "${USBBOOT_DIR}/secure-boot-example/example-public.pem" -f -o pieeprom.hsm.bin
    HSM_SIGNED="${EEPROM_TEST_DIR}/pieeprom.hsm.bin"
    
    # Compare the signed files
    echo ""
    echo "----------------------------------------------------------------------"
    echo "COMPARING SIGNED FILES"
    echo "----------------------------------------------------------------------"
    echo ""
    if cmp -s "${PRIVATE_SIGNED}" "${HSM_SIGNED}"; then
        echo "SUCCESS: HSM wrapper and private key signing produce identical EEPROM output"
        return 0
    else
        echo "ERROR: HSM wrapper and private key signing produce different EEPROM output"
        echo "Files differ. Check the following files for details:"
        echo "  Private key signed: ${PRIVATE_SIGNED}"
        echo "  HSM wrapper signed: ${HSM_SIGNED}"
        return 1
    fi
}

# Run the tests
test_sign_bootcode
test_sign_eeprom