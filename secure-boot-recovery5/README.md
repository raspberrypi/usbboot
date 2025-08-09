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
mkdir -p metadata
../rpiboot -d . -j metadata
```

### Example UART output
This output is given by the EEPROM bootloader when it verifies the signature of a `boot.img`. It can be read over the [3-pin Serial Debug Port](https://datasheets.raspberrypi.com/debug/debug-connector-specification.pdf). This is accessible via a JST-SH header on the Pi 5B. On the CM5, it is unpopulated on the top side of the board.
```
3.04 OTP boardrev b04170 bootrom a a
3.06 Customer key hash 8251a63a2edee9d8f710d63e9da5d639064929ce15a2238986a189ac6fcd3cee
3.13 VC-JTAG unlocked
3.36 RP1_BOOT chip ID: 0x20001927
3.41 bootconf.sig
3.41 hash: f71ede8fad8bea2f853bcff41173ffedde48c5b76ed46bc38fa057ce46e5d58b
3.47 rsa2048: 3f215305d5aff620219da94f6f1294787e3a407102a507da96c28e9195d3ccb2f144cac66919f9d86ba9f54a8d20ff57c80d6d269e6e49a16dc23553974489947fe05bf3b7df5cd2c5040a9eebadca754ff4be50600b06fd9f565639adc859d88052e15e0ff6eecf7fec0386d41f81e5d009b04520bb83f17663b62b1271b9d27ec2344c73a20d42dfd68facd741d48c0453e8149448537abfed1d4805872c16182a3e9f25c0b86e002e88949d62c148a561aa8137c257ce0d3e0ae5761aa64c225e9c9782b2bb613de7d90499567c56218bb18a239d4347967b68b3ebd06eaa48215f16316d2a697bb2e67cb3883068f6284e2ca71d25ce0099a1ceb37a85c9
3.94 RSA verify
3.10 rsa-verify pass (0x0)

```

### Metadata
The optional metadata argument causes rpiboot to readback the OTP information and write it to a JSON file in the given directory.
This can be useful for debug or for storing in a provisioning database.

Example metadata:
```json
{
        "USER_SERIAL_NUM" : "a7eb274c",
        "MAC_ADDR" : "2c:cf:67:70:76:f3",
        "CUSTOMER_KEY_HASH" : "8251a63a2edee9d8f710d63e9da5d639064929ce15a2238986a189ac6fcd3cee",
        "BOOT_ROM" : "0000000a",
        "BOARD_ATTR" : "00000000",
        "USER_BOARDREV" : "b04170",
        "JTAG_LOCKED" : "0",
        "MAC_WIFI_ADDR" : "2c:cf:67:70:76:f4",
        "MAC_BT_ADDR" : "2c:cf:67:70:76:f5",
        "FACTORY_UUID" : "001000911006186073"
}
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


To enable this, edit the `config.txt` file in this directory and set `program_pubkey=1`

## Disabling VideoCore JTAG

VideoCore JTAG may be permanently disabled by setting `program_jtag_lock` in
`config.txt`. This option has no effect unless the public key hash (`program_pubkey`) has been successfully programmed.

See [config.txt](config.txt)
