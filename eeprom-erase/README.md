The `erase_eeprom` `config.txt` option causes `recovery.bin` to execute a chip-erase operation on the bootloader SPI EEPROM.
This is a test/debug option and there is no need to manually erase an EEPROM before flashing it.

If the SPI EEPROM is erased then the Raspberry Pi will not boot until a new EEPROM image has been written via `RPIBOOT`
or the Raspberry Pi Imager (Pi4 and Pi400 only).

```bash
cd erase-eeprom
../rpiboot -d .
```
