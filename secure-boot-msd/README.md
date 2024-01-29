# USB MSD FIRMWARE device mode drivers for signed-boot

NOTE: This module is deprecated. Please use the Linux based
mass-storage-gadget64 which provides better performance and
support for NVMe and MSD devices.

If secure-boot mode has been locked (via OTP) then both the
bootloader and rpiboot `bootcode4.bin` will only load `boot.img`
files signed with the customer's private key. Therefore, access
to rpiboot mass storage mode is disabled.

Mass storage mode can be re-enabled by signing a boot image
containing the firmware mass storage drivers.

N.B. The signed image should normally be kept secure because can
be used on any device signed with the same customer key.

WARNING: This image is NOT compatible with Pi5.

To sign the mass storage mode boot image run:-
```bash
KEY_FILE=$HOME/private.pem
rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```

To run load the USB MSD device drivers via RPIBOOT run
```bash
../rpiboot -d .
```
