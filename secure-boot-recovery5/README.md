# Raspberry Pi 5 - secure boot

*** WARNING: Secure boot firmware and tooling is currently in BETA on 2712 ***

This directory contains the beta bootcode5.bin (recovery.bin) and a pre-release pieeprom.bin
bootloader release. Older bootloader and recovery.bin releases do not support secure boot.

# Required packages
sudo apt install xxd python3-pycryptodome

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

## Sign the EEPROM and the second stage bootloader
The BCM2712 boot ROM requires the second stage firmware (recovery.bin / bootcode5.bin) to be counter-signed with the customer private key in secure-mode.
Pass the `f` flag to enable firmware counter-signing.

../tools/update-pieeprom.sh -f -k "${KEY_FILE}"
```

If secure-boot has already been enabled on the device then `recovery.bin` must then also be counter-signed.
However, booting a counter-signed `recovery.bin` image on a fresh board without secure-boot enabled will fail
because the ROM is effectively checking this against a key hash of zero.
```
../tools/update-pieeprom.sh -fr -k "${KEY_FILE}"
```

`pieeprom.bin` can then be flashed to the bootloader EEPROM via `rpiboot`.

## Program the EEPROM image using rpiboot
* Power off DUT
* Set nRPIBOOT jumper (or hold power button before power on) and remove EEPROM WP protection
```bash
cd secure-boot-recovery5
../rpiboot -d .
```
* Power ON the DUT

## Locking secure-boot mode
After verifying that the signed OS image boots successfully the system
can be locked into secure-boot mode.  This writes the hash of the
customer public key to "one time programmable" (OTP) bits. From then
onwards:

* The 2712 boot ROM verifies that the secondstage firmware (recovery.bin / bootsys) are
  signed with the customer private key in addition to the Raspberry Pi private key.
* The bootloader will only load OS images signed with the customer private key.
* The EEPROM configuration file must be signed with the customer private key.
* It is not possible to downgrade to an old version of the bootloader that doesn't
  support secure boot.
* BETA bootloader releases are not signed with the ROM secure boot key and will
  not boot on a system where `revoke_devkey` has been set.

**WARNING: Modifications to OTP are irreversible. Once `revoke_devkey` has been set it is not possible to unlock secure-boot mode or use a different private key.**

To enable this edit the `config.txt` file in this directory and set
`program_pubkey=1`

* `program_pubkey` - If 1, write the hash of the customer's public key to OTP.

## Revoking the dev key - NOT SUPPORTED YET
* `revoke_devkey` - If 1, revoke the ROM bootloader development key which
   requires secure-boot mode and prevents downgrades to bootloader versions that
    don't support secure boot.

## Disabling VideoCore JTAG

VideoCore JTAG may be permanently disabled by setting `program_jtag_lock` in
`config.txt`.  This option has no effect unless the public key hash (`program_pubkey`) has been successfully programmed.

See [config.txt](config.txt)
