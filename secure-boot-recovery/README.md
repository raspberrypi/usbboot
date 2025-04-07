# Raspberry Pi 4 - secure boot

This directory contains the latest stable versions of the bootloader EEPROM
and recovery.bin files that support secure-boot.

Steps for enabling secure boot:

## Extra steps for Raspberry Pi 4B & Pi 400
Raspberry Pi 4B and Pi400 do not have a dedicated RPIBOOT jumper so a different GPIO
must be used to enable RPIBOOT if pulled low. The available GPIOs are `2,4,5,6,7,8`
since these are high by default.

Pi4 Model B rev 1.3 and older use the BCM2711B0 processor which does not support secure-boot.
All CM4, CM4S and Pi400 boards use BCM2711C0 which supports secure-boot.

### Step 1 - Erase the EEPROM
In order to avoid this OTP configuration being accidentally set on Pi 4B / Pi 400
this option can only be set via RPIBOOT. To force RPIBOOT on a Pi 4B / Pi 400
erase the SPI EEPROM.

* Use `Raspberry Pi Imager` to flash a bootloader image to a spare SD card.
* Remove `pieeprom.bin` and `pieeprom.sig` from the SD card image.
* Add a `config.txt` file to the SD card with the following entries then boot the Pi with this card.

```
erase_eeprom=1
uart_2ndstage=1
```

### Step 2 - Select the nRPIBOOT GPIO
Edit the `secure-boot-recovery/config.txt` file to specify the GPIO to use for nRPIBOOT. For example:
```
program_rpiboot_gpio=8
```

This can either be programmed in isolation or combined with the steps to program the secure-boot OTP settings.

## Optional. Specify the private key file in an environment variable.
Alternatively, specify the path when invoking the helper scripts.
```bash
export KEY_FILE="${HOME}/private.pem"
```

## Optional. Customize the EEPROM config.
Custom with the desired bootloader settings.
See: [Bootloader configuration](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#raspberry-pi-bootloader-configuration)

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
* It is not possible to downgrade to an old version of the bootloader that doesn't
  support secure boot.

**WARNING: Modifications to OTP are irreversible. Once `revoke_devkey` has been set it is not possible to unlock secure-boot mode or use a different private key.**

To enable this edit the `config.txt` file in this directory and set
`program_pubkey=1`

* `program_pubkey` - If 1, write the hash of the customer's public key to OTP.
* `revoke_devkey` - If 1, revoke the ROM bootloader development key which
   requires secure-boot mode and prevents downgrades to older bootloader versions that don't support secure boot.

## Disabling VideoCore JTAG

VideoCore JTAG may be permanently disabled by setting `program_jtag_lock` in
`config.txt`. This option has no effect unless `revoke_devkey=1` is set and
the EEPROM and customer OTP key were programmed successfully.

See [config.txt](config.txt)
