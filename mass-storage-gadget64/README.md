# AARCH64 USB mass-storage gadget for BCM2711 and BCM2712

This directory provides a bootloader image that loads a Linux
initramfs that exports common block devices (EMMC, NVMe) as
USB mass storage devices using the Linux gadget-fs drivers.

This allows Raspberry Pi Imager to be run on the host computer
and write OS images to the Raspberry Pi or Compute Module block devices.

## Running
To run load the USB MSD device drivers via RPIBOOT run
```bash
cd mass-storage-gadget64
../rpiboot -d .

```
N.B. This takes a few seconds longer to initialise than the 
previous mass storage implementation. However, the write speed
should be much faster now that all of the file-system code
is running on the ARM processors.

### Debug
The mass-storage-gadget image automatically enables a UART console for debugging (user `root` empty password).

## secure-boot
Not supported in this test version.

## Source code
The buildroot configuration and supporting patches is available on
the [mass-storage-gadget64](https://github.com/raspberrypi/buildroot/tree/mass-storage-gadget64)
branch of the Raspberry Pi [buildroot](https://github.com/raspberrypi/buildroot) repo.

### Building
```bash
git clone --branch mass-storage-gadget git@github.com:raspberrypi/buildroot.git
cd buildroot
make raspberrypi64-mass-storage-gadget_defconfig
make
```

The output is written to `output/target/images/sdcard.img` and can be copied
to `boot.img`
