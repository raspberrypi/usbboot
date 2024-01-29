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

### Debug
The mass-storage-gadget image automatically enables a UART console for debugging (user `root` empty password).

## Secure boot
Once secure-boot has been enable the OS `boot.img` file must be signed with the customer private key.
On Pi5 firmware must also be counter-signed with this key.

The `sign.sh` script wraps the command do this on Pi4 and Pi5.
```bash
KEY_FILE=$HOME/private.pem
./sign.sh ${KEY_FILE}
```

WARNING: The signed images will not be bootable on a Pi5 without secure-boot enabled. Run `./reset.sh` to reset the signed images to the default unsigned state.

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

The output is written to `output/target/images/sdcard.img` and can be copied to `boot.img`
