# Signing the Raspberry Pi Imager for secure boot

If secure-boot has been enabled then this image must be signed with
the customer's RSA private key. Otherwise, the SPI EEPROM bootloader
will refused to load this image.

To do this run:

```bash
KEY_FILE=$HOME/private.pem
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```

To run load the USB MSD device drivers via RPIBOOT run
```bash
../rpiboot -d .
```
