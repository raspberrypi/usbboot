# USB mass-storage drivers for Compute Module 4

This directory provides a bootloader image that loads a Linux
initramfs that exports common block devices (EMMC, NVMe) as
USB mass storage devices using the Linux gadget-fs drivers.

This allows Raspberry Pi Imager to be run on the host computer
and write OS images to the Compute Module block devices.

## secure-boot
If secure-boot mode has been locked (via OTP) then both the
bootloader and rpiboot `bootcode4.bin` will only load `boot.img`
files signed with the customer's private key. Therefore, access
to rpiboot mass storage mode is disabled.

Mass storage mode can be re-enabled by signing a boot image
containing the firmware mass storage drivers.

N.B. The signed image should normally be kept secure because can
be used on any device signed with the same customer key.

To sign the mass storage mode boot image run:-
```bash
KEY_FILE=$HOME/private.pem
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```

## Running
To run load the USB MSD device drivers via RPIBOOT run
```bash
../rpiboot -d .
```

The activity LED will flash three times once the USB MSD
devices have been initialised. A message will also be printed
to the HDMI console.

The UART console is also enabled (user 'root' no password)
so that the user can login and debug the Compute Module.

N.B. This takes a few seconds longer to initialise than the 
previous mass storage implementation. However, the write speed
should be much faster now that all of the file-system code
is running on the ARM processors.

## Source code
The buildroot configuration and supporting patches is available on
the [mass-storage-gadget](https://github.com/raspberrypi/buildroot/tree/mass-storage-gadget)
branch of the Raspberry Pi [buildroot](https://github.com/raspberrypi/buildroot) repo.

### Building
```bash
git clone git@github.com:timg236/cm4-scripts.git cm4-msg-overlay
git clone --branch mass-storage-gadget git@github.com:raspberrypi/buildroot.git
cd buildroot
make raspberrypicm4io_initrd_defconfig
make
```

The output is written to `output/target/images/sdcard.img` and can be copied
to `boot.img`

Optional:
Load time can be reduced by optmizing the size of the sdcard.img as
follows (run as root).

```bash
mkdir -p source-files
kpartx -av sdcard.img
mount /dev/mapper/loop0p1 source-files/
usbboot/tools/make-boot-image -d source-files/ -o boot.img
umount source-files
kpartx -dv sdcard.img
dmsetup remove /dev/mapper/loop0p1
losetup -D
```
