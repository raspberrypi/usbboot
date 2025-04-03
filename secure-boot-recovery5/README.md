# Raspberry Pi 5 - secure boot

This directory contains the beta bootcode5.bin (recovery.bin) and a pre-release pieeprom.bin
bootloader release. Older bootloader and recovery.bin releases do not support secure boot.

# Required packages
```bash
sudo apt install xxd python3-pycryptodome
```

## Optional. Specify the private key file in an environment variable.
Alternatively, specify the path when invoking the helper scripts.
```bash
export KEY_FILE="${HOME}/private.pem"
```

## Optional. Customise the EEPROM config.
Customise with the desired bootloader settings.
See: [Bootloader configuration](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#raspberry-pi-bootloader-configuration)

## Sign the EEPROM and the second stage bootloader
The BCM2712 boot ROM requires the second-stage firmware (recovery.bin / bootcode5.bin) to be counter-signed with the customer private key in secure-boot mode.
Pass the `f` flag to enable firmware counter-signing.

### Using a private key file
```
../tools/update-pieeprom.sh -f -k "${KEY_FILE}"
```

If secure-boot has already been enabled on the device, then `recovery.bin` must also be counter-signed.
However, booting a counter-signed `recovery.bin` image on a fresh board without secure-boot enabled will fail
because the ROM is effectively checking this against a key hash of zero.
```
../tools/update-pieeprom.sh -fr -k "${KEY_FILE}"
```

### Using an HSM wrapper
When using a Hardware Security Module (HSM), you need to provide both the HSM wrapper script and the corresponding public key:

```
../tools/update-pieeprom.sh -f -H hsm-wrapper -p public.pem
```

For recovery.bin signing with HSM:
```
../tools/update-pieeprom.sh -fr -H hsm-wrapper -p public.pem
```

The HSM wrapper script should:
- Take a single argument which is a temporary filename containing the data to sign
- Output the PKCS#1 v1.5 signature in hex format
- Return a non-zero exit code if signing fails

`pieeprom.bin` can then be flashed to the bootloader EEPROM via `rpiboot`.

## Programming the EEPROM image using rpiboot
* Power off the device
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

* The 2712 boot ROM verifies that the second-stage firmware (recovery.bin / bootsys) is
  signed with the customer private key in addition to the Raspberry Pi private key.
* The bootloader will only load OS images signed with the customer private key.
* The EEPROM configuration file must be signed with the customer private key.
* It is not possible to downgrade to an old version of the bootloader that doesn't
  support secure boot.

**WARNING: Modifications to OTP are irreversible. Once `revoke_devkey` has been set, it is not possible to unlock secure-boot mode or use a different private key.**

To enable this, edit the `config.txt` file in this directory and set
`program_pubkey=1`

* `program_pubkey` - If 1, write the hash of the customer's public key to OTP.

## Revoking the development key - NOT SUPPORTED YET
* `revoke_devkey` - If 1, revoke the ROM bootloader development key, which
   requires secure-boot mode and prevents downgrades to bootloader versions that
   don't support secure boot.

## Disabling VideoCore JTAG

VideoCore JTAG may be permanently disabled by setting `program_jtag_lock` in
`config.txt`. This option has no effect unless the public key hash (`program_pubkey`) has been successfully programmed.

See [config.txt](config.txt)
