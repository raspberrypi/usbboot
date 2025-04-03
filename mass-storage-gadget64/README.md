# USB mass-storage gadget for BCM2711 and BCM2712

This directory provides a bootloader image that loads a Linux
initramfs that exports common block devices (EMMC, NVMe) as
USB mass storage devices using the Linux gadget-fs drivers.

This allows Raspberry Pi Imager to be run on the host computer
and write OS images to the Raspberry Pi or Compute Module block devices.

## Running
To run load the USB MSD device drivers via RPIBOOT run
```bash
rpiboot -d mass-storage-gadget64

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
or as follows if using a HSM wrapper script.
```bash
./sign.sh -H hsm-wrapper public.pem
```

WARNING: The signed images will not be bootable on a Pi5 without secure-boot enabled. Run `./reset.sh` to reset the signed images to the default unsigned state.

## Source code
The buildroot configuration and supporting patches is available on
the [mass-storage-gadget64](https://github.com/raspberrypi/buildroot/tree/mass-storage-gadget64)
branch of the Raspberry Pi [buildroot](https://github.com/raspberrypi/buildroot) repo.

### Building

In order to build directly on a Linux host that has the needed dependencies, run:
```bash
git clone --branch mass-storage-gadget64 git@github.com:raspberrypi/buildroot.git
cd buildroot
make raspberrypi64-mass-storage-gadget_defconfig
make
```

The output is written to `output/images/sdcard.img` and can be copied to `boot.img`

Alternatively, if you have docker installed and would like to use the upstream buildroot CI docker image for a build environment, use its `utils/docker-run` helper script:
```bash
$ git clone --branch mass-storage-gadget64 git@github.com:raspberrypi/buildroot.git
$ cd buildroot
$ ./utils/docker-run /bin/bash -c "make raspberrypi64-mass-storage-gadget_defconfig && make"
```
