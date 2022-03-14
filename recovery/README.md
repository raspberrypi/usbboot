To update the SPI EEPROM bootloader on a Compute Module 4.

* Modify the EEPROM configuration as desired
* Optionally, replace pieeprom.original.bin with a custom version. The default
  version here is the latest stable release recommeded for use on Compute Module 4.

N.B The `bootcode4.bin` file in this directory is actually the `recovery.bin`
file used on Raspberry Pi 4 bootloader update cards.

```bash
cd recovery
./update-pieeprom.sh
../rpiboot -d .
```
