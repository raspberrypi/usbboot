# Raspberry Pi 4 - secure boot

This directory contains the beta bootcode4.bin (recovery.bin) and pieeprom-2021-05-19.bin
bootloader release. Older bootloader and recovery.bin releases do not support secure boot.

Steps for enabling secure boot:

## Extra steps for Raspberry Pi 4B & Pi 400
Raspberry Pi 4B and Pi400 do not have a dedicated RPIBOOT jumper so a different GPIO
must be used to enable RPIBOOT if pulled low. The available GPIOs are 2,4,5,6,7,8
since these are high by default.

### Step 1 - Erase the EEPROM
In order to avoid this OTP configuration being accidently set on Pi 4B / Pi 400
this option can only be set via RPIBOOT. To force RPIBOOT on a Pi 4B / Pi 400
erase the SPI EEPROM.

Copy recovery.bin to a blank FAT32 formatted SD card with the following `config.txt` file.
Then insert the SD card and boot the Pi and wait at least 10 seconds for the green
LED to flash rapidly.
```
erase_eeprom=1
```

### Step 2 - Select the nRPIBOOT GPIO
Then use rpiboot config.txt specify the GPIO to use for nRPIBOOT. For example:
```
program_rpiboot_gpio=8
```

The OTP setting for nRPIBOOT will then be set in the next steps when the
EEPROM / secure-boot configuration is programmed.

## Optional. Specify the private key file in an environment variable.
Alternatively, specify the path when invoking the helper scripts.
```bash
export KEY_FILE="${HOME}/private.pem"
```

## Optional. Customize the EEPROM config.
Custom with the desired bootloader settings. 
See: [Bootloader configuration](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711_bootloader_config.md)

Setting `SIGNED_BOOT=1` enables signed-boot mode so that the bootloader will only
boot.img files signed with the specified RSA key. Since this is an EEPROM config
option secure-boot can be tested and reverted via `RPIBOOT` at this stage.

## Generate the signed bootloader image
```bash
cd secure-boot-recovery
../tools/update-pieeprom.sh -k "${KEY_FILE}"
```

`pieeprom.bin` can then be flashed to the bootloader EEPROM via `rpiboot`.

## Program the EEPROM image using rpiboot
* Power off CM4
* Set nRPIBOOT jumper and remove EEPROM WP protection
```bash
cd secure-boot-recovery
../rpiboot -d .
```
* Power ON CM4

## Locking secure-boot mode
After verifying that the signed OS image boots successfully the system
can be locked into secure-boot mode.  This writes the hash of the
customer public key to "one time programmable" (OTP) bits. From then
onwards:

* The bootloader will only load OS images signed with the customer private key.
* The EEPROM configuration file must be signed with the customer private key.
* It is not possible to install an old version of the bootloader that does
  support secure boot.
* This option requires EEPROM version 2022-01-06 or newer.
* BETA bootloader releases are not signed with the ROM secure boot key and will
  not boot on a system where `revoke_devkey` has been set.

**WARNING: Modifications to OTP are irreversible. Once `revoke_devkey` has been set it is not possible to unlock secure-boot mode or use a different private key.**

To enable this edit the `config.txt` file in this directory and set
`program_pubkey=1`

* `program_pubkey` - If 1, write the hash of the customer's public key to OTP.
* `revoke_devkey` - If 1, revoke the ROM bootloader development key which
   requires secure-boot mode and prevents downgrades to bootloader versions that
    don't support secure boot.

## Disabling VideoCore JTAG

VideoCore JTAG may be permentantly disabled by setting `program_jtag_lock` in
`config.txt`. This option has no effect unless `revoke_revkey=1` is set and
the EEPROM and customer OTP key were programmed successfully.

See [config.txt](config.txt)
