# USB mass-storage drivers for Compute Module 4

This directory provides a bootloader image that loads a Linux
initramfs that exports common block devices (EMMC, NVMe) as
USB mass storage devices using the Linux gadget-fs drivers.

This allows Raspberry Pi Imager to be run on the host computer
and write OS images to the Compute Module block devices.

## Running
To run load the USB MSD device drivers via RPIBOOT run
```bash
cd mass-storage-gadget
../rpiboot -d .

```
N.B. This takes a few seconds longer to initialise than the 
previous mass storage implementation. However, the write speed
should be much faster now that all of the file-system code
is running on the ARM processors.

### Debug
The mass-storage-gadget image automatically enables a UART console for debugging (user `root` empty password).

## Secure boot
If secure-boot is enabled then `boot.img` must be signed with the customer private key file.
```bash
KEY_FILE=$HOME/private.pem
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```

## Source code
The buildroot configuration and supporting patches is available on
the [mass-storage-gadget](https://github.com/raspberrypi/buildroot/tree/mass-storage-gadget)
branch of the Raspberry Pi [buildroot](https://github.com/raspberrypi/buildroot) repo.

### Building
```bash
git clone --branch mass-storage-gadget git@github.com:raspberrypi/buildroot.git
cd buildroot
make raspberrypicm4io_initrd_defconfig
make
```

The output is written to `output/target/images/sdcard.img` and can be copied
to `boot.img`
