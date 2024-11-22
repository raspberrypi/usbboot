@echo off
@echo USB mass storage gadget for Raspberry Pi 5
rpiboot -d mass-storage-gadget64
@echo:
@echo Raspberry Pi Mass Storage Gadget started
@echo EMMC/NVMe devices should be visible in the Raspberry Pi Imager in a few seconds.
@echo For debug, you can login to the device using the USB serial gadget - see COM ports in Device Manager.
@echo: 
set /p dummy=Press a key to close this window.