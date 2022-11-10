# Secure Boot Quickstart

## Overview
Secure boot is a mechanism for verifying the integrity of the kernel+initramfs and
other files required during boot by storing them in a signed ramdisk image.
These files include the GPU firmware (start.elf etc), kernel, initrd, Device Tree
and overlays.

Secure boot does not depend on a particular OS, nor does it provide services
to the OS after the Kernel + initramfs has started.

N.B. The memory for the secure ramdisk is reclaimed as soon as the CPU is started.

## Example OS
This example includes a simple buildroot image for Compute Module 4 in order
to demonstrate secure boot on a simple but functional OS.

The example does NOT modify the OTP or make other permanent changes to the system;
the code signing can be disabled by reflashing the default bootloader EEPROM.

Please see the top level [README](../Readme.md#building) and [secure-boot-recovery/README](../secure-boot-recovery/README.md) guides for
instructions about how to permanently enable secure-boot by programming OTP.

### Requirements for running this example
* A Raspberry Pi Compute Module 4
* Micro USB cable for `rpiboot` connection
* USB serial cable (for debug logs)
* Linux, WSL or Cygwin (Windows 10)
* OpenSSL
* Python3
* Python `cryptodomex`

```bash
python3 -m pip install pycryptodomex
# or
pip install pycryptodomex
```

### Clean configuration
Before starting it's advisable to create a fresh clone of the `usbboot` repo
to ensure that there are no stale configuration files.

```bash
git clone https://github.com/raspberrypi/usbboot secure-boot
cd secure-boot
make
```
See the top-level [README](../Readme.md) for build instructions.

### Hardware setup for `rpiboot` mode
Prepare the Compute Module for `rpiboot` mode:

* Set the `nRPIBOOT` jumper which is labelled `Fit jumper to disable eMMC Boot' on the Compute Module 4 IO board.
* Connect the micro USB cable to the `USB slave` port on the CM4 IO board.
* Power cycle the CM4 IO board.
* Connect the USB serial adapter to [GPIO 14/15](https://www.raspberrypi.com/documentation/computers/os.html#gpio-and-the-40-pin-header) on the 40-pin header.

### Reset the Compute Module EEPROM
Enable `rpiboot` mode and flash the latest EEPROM image:
```bash
./rpiboot -d recovery
```

### Boot the example image
Enable `rpiboot` mode and load the OS via `rpiboot` without enabling code-signing:
```bash
./rpiboot -d secure-boot-example
```
The OS should load and show activity on the boot UART and HDMI console.

### Generate a signing key
Secure boot requires a 2048 bit RSA private key. You can either use a pre-existing
key or generate an specific key for this example. The `KEY_FILE` environment variable
used in the following instructions must contain the absolute path to the RSA private key in
PEM format.

```bash
openssl genrsa 2048 > private.pem
export KEY_FILE=$(pwd)/private.pem
```

**In a production environment it's essential that this key file is stored privately and securely.**

### Update the EEPROM to require signed OS images
Enable `rpiboot` mode and flash the bootloader EEPROM with updated setting enables code signing.

The `boot.conf` file sets the `SIGNED_BOOT` property `1` which instructs the bootloader to only
load files (firmware, kernel, overlays etc) from `boot.img` instead of the normal boot partition and verify the `boot.img` signature `boot.sig` using the public key in the EEPROM.

The `update-pieeprom.sh` generates the signed `pieeprom.bin` image.

```bash
cd secure-boot-recovery
# Generate the signed EEPROM image.
../tools/update-pieeprom.sh -k "${KEY_FILE}"
cd ..
./rpiboot -d secure-boot-recovery
```

At this stage OTP has not been modified and the signed image requirement can be reverted by flashing a default, unsigned image.
However, once the [OTP secure-boot flags](../secure-boot-recovery/README.md#locking-secure-boot-mode) are set then `SIGNED_BOOT` is permanently enabled and cannot be overridden via the EEPROM config.


### Update the signature for the example OS image
```bash
cd secure-boot-example
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
cd ..
```

### Launch the signed OS image
Enable `rpiboot` mode and run the example OS as before. However, if the
`boot.sig` signature does not match `boot.img`, the bootloader will refuse to
load the OS.

```bash
./rpiboot -d secure-boot-example
```

This example OS image is minimal Linux ramdisk image. Login as `root` with the empty password.

#### Disk encryption example
Example script which uses a device-specific private key to create/mount an encrypted file-system.

Generating a 256-bit random key for test purposes.
```bash
export KEY_FILE=$(mktemp -d)/key.bin
openssl rand -hex 32 | xxd -rp > ${KEY_FILE}
```

Using [rpi-otp-private-key](../tools/rpi-otp-private-key) to extract the device private key (if programmed).
```bash
export KEY_FILE=$(mktemp -d)/key.bin
rpi-otp-private-key -b > "${KEY_FILE}"
```

Creating an encrypted disk on a specified block device.
```bash
export BLK_DEV=/dev/mmcblk0p3
cryptsetup luksFormat --key-file="${KEY_FILE}" --key-size=256 --type=luks2 ${BLK_DEV}

cryptsetup luksOpen ${BLK_DEV} encrypted-disk --key-file="${KEY_FILE}"
mkfs /dev/mapper/encrypted-disk
mkdir -p /mnt/application-data
mount /dev/mapper/encrypted-disk /mnt/application-data
rm "${KEY_FILE}"
```

### Mount the CM4 SD/EMMC after enabling secure-boot
Now that `SIGNED_BOOT` is enabled the bootloader will only load images signed with private key generated earlier.
To boot the Compute Module in mass storage mode a signed version of this code must be generated.

**This signed image should not be made available for download because it gives access to the EMMC as a block device.**


#### Sign the mass storage firmware image
Sign the mass storage drivers in the `secure-boot-msd` directory. Please see the [top level README](../Readme.md#compute-module-4-extensions) for a description of the different `usbboot` firmware drivers.
```bash
cd secure-boot-msd
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
cd ..
```

#### Enable MSD mode
A new mass storage device should now be visible on the host OS. On Linux check `dmesg` for something like '/dev/sda'.
```bash
./rpiboot -d secure-boot-msd
```

### Loading `boot.img` from SD/EMMC
The bootloader can load a ramdisk `boot.img` from any of the bootable modes defined by the [BOOT_ORDER](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#BOOT_ORDER) EEPROM config setting.

For example:

* Boot the CM4 in MSD mode as explained in the previous step.
* Copy the `boot.img` and `boot.sig` files from the `secure-boot-example` stage to the mass storage drive: No other files are required.
* Remove the `nRPIBOOT` jumper.
* Power cycle the CM4 IO board.
* The system should now boot into the OS.

### Modifying / rebuilding `boot.img`
The secure-boot example image can be rebuilt and modified using buildroot. See [raspberrypi-signed-boot](https://github.com/raspberrypi/buildroot/blob/raspberrypi-signed-boot/README.md) buildroot configuration.
