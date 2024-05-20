To update the SPI EEPROM bootloader on Raspberry Pi 5.

* Modify the EEPROM configuration as desired
* Optionally, replace pieeprom.original.bin with a custom version. The default
  version here is the latest stable release recommended for use on Raspberry Pi 5.

N.B The `bootcode5.bin` file in this directory is actually the `recovery.bin`
file used on Raspberry Pi 5 bootloader update cards.

```bash
cd recovery5
./update-pieeprom.sh
../rpiboot -d .
```
