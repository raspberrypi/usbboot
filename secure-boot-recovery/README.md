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
* If possible connect a UART to the CM4 and capture the output for debug

```bash
cd secure-boot-recovery
mkdir -p metadata
../rpiboot -d . -j metadata
```

### Example UART output
```

148.84 Verify BOOT EEPROM
148.85 Reading EEPROM: 524288 bytes 0xc0b60000
148.35 645ms
149.89 BOOT-EEPROM: UPDATED
149.08 secure_boot_provision program_pubkey 1
149.09 bootconf.sig
149.09 hash: b503f8ad7f8aea93a272a5ba5248cc5222d0c55af28a4345d0a496bdba9d16bf
149.10 rsa2048: 612bda4eee969ef9f3e1a651cc4ae6f8b5c5e1eef436e0ff80a4bea2e70bb163b1a54e1376be04a248d7a1f52256bcf0f3dab71b93fb344d7200b61f1020f4620e7ad587cf7fbc35a10b7cd928fe6e3e239d6a7b17a0fd62ddf49ac5fc667686fb43be4b24811a8e1b6e31de525dd2f31ac851f5a19815aa85f9755456610161e034ff7672fd69d567c159d84f703bfbdd76c9a6ec3804236d3dd5550f09d083521d4f6cb6f50ab1ada7c37090a6bc8e306690f04d06dab02b0b80027c9cd27bef18be14f771bb4841bf5a285fadc2731278fc73efccbab1f60fd58c3ada1b35f6a11e9862b5eacdcff420c827f33b498fc2782659e1bcf35bf1ef02adb46c28
149.14 RSA verify
149.81 rsa-verify pass (0x0)
149.18 Public key hash 8251a63a2edee9d8f710d63e9da5d639064929ce15a2238986a189ac6fcd3cee
149.19 OTP-WR: boot-mode 000048b0
149.19 OTP-WR: boot-mode 000048b0
149.19 OTP-WR: flags 00000081
149.19 Write OTP key
149.22 OTP updated for key 8251a63a2edee9d8f710d63e9da5d639064929ce15a2238986a189ac6fcd3cee
149.23 Revoke development key
```

### Metadata
The optional metadata argument causes rpiboot to readback the OTP information and write it to a JSON file in the given directory.
This can be useful for debug or for storing in a provisioning database.

Example metadata:
```json
{
        "MAC_ADDR" : "d8:3a:dd:05:ee:78",
        "CUSTOMER_KEY_HASH" : "8251a63a2edee9d8f710d63e9da5d639064929ce15a2238986a189ac6fcd3cee",
        "BOOT_ROM" : "0000c8b0",
        "BOARD_ATTR" : "00000000",
        "USER_BOARDREV" : "c03141",
        "JTAG_LOCKED" : "0",
        "ADVANCED_BOOT" : "0000e8e8"
}
```
* Power ON CM4

## Locking secure-boot mode
After verifying that the signed OS image boots successfully the system
can be locked into secure-boot mode.  This writes the hash of the
customer public key to "one time programmable" (OTP) bits. From then
onwards:

* The bootloader will only load OS images signed with the customer private key.
* The EEPROM configuration file must be signed with the customer private key.
* It is not possible to downgrade to an old version of the bootloader that doesn't support secure boot.

To enable this edit the `config.txt` file in this directory and set `program_pubkey=1`

## Disabling VideoCore JTAG

VideoCore JTAG may be permanently disabled by setting `program_jtag_lock` in
`config.txt`. This option has no effect unless secure-boot has been enabled.

See [config.txt](config.txt)
