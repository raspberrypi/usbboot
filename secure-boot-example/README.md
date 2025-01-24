# Secure Boot Quickstart

## Overview

This example demonstrates how the low level code signing and provisioning tools can be used to enable
signed boot on Compute Module 4 or Compute Modulde 5. For simplicity, the example is based on the
mass-storage-gadget which small buildroot image.

For production systems we recommend using the higher level [Raspberry Pi Secure Boot Provisioner](https://github.com/raspberrypi/rpi-sb-provisioner)

See also: EEPROM and OTP provisioning guides for secure boot [secure-boot-recovery CM4](../secure-boot-recovery/README.md) secure boot [secure-boot-recovery CM5](../secure-boot-recovery5/README.md)

**WARNING: Enabling signed boot modifies the OTP memory and is irreversible. **

### Requirements for running this example
* A Raspberry Pi Compute Module 5 or Compute Module 4 / 4S and the relevant IO board.
* Micro USB cable for `rpiboot` connection
* USB serial cable (for debug logs)
* Linux, WSL or Cygwin (Windows 11)
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
git submodule update --init
make
```
See the top-level [README](../Readme.md) for full build instructions.

### Hardware setup for `rpiboot` mode
Prepare the Compute Module for `rpiboot` mode:

#### Compute Module 4
* Set the `nRPIBOOT` jumper which is labelled `Fit jumper to disable eMMC Boot' on the Compute Module 4 IO board.
* Connect the micro USB cable to the `USB slave` port on the Compute Module IO board.
* Power cycle the Compute Module IO board.
* Connect the USB serial adapter to [GPIO 14/15](https://www.raspberrypi.com/documentation/computers/os.html#gpio-and-the-40-pin-header) on the 40-pin header.

#### Compute Module 5
* Set the `nRPIBOOT` jumper which is labelled `Fit jumper to disable eMMC Boot' on the Compute Module 5 IO board.
* Disconnect any USB peripherals from the IO board in order to reduce power consumption.
* Connect USB-A to USB-C cable from the rpiboot host to the IO board.
* Connect the USB serial adapter to [GPIO 14/15](https://www.raspberrypi.com/documentation/computers/os.html#gpio-and-the-40-pin-header) on the 40-pin header.
* If the Compute Module has the dedicate boot UART connector fitted then this will provide additional debug.
* Power cycle the Compute Module IO board.

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

Running `update-pieeprom.sh` generates the signed `pieeprom.bin` image.

```bash
cd secure-boot-recovery
# Generate the signed EEPROM image.
../tools/update-pieeprom.sh -k "${KEY_FILE}"
cd ..
# On Compute Modeule 4 or 4S
./rpiboot -d secure-boot-recovery
# On Compute Module 5
./rpiboot -d secure-boot-recovery5
```

## Sign the example image
Once secure-boot has been enable the OS `boot.img` file must be signed with the customer private key.
On Compute Module 5 the Raspberry Pi 5 firmware must also be counter-signed with this key.

The `sign.sh` script wraps the low level commands to do this:-
```bash
./sign.sh ${KEY_FILE}
```

### Launch the signed OS image
Enable `rpiboot` mode and run the example OS. If the `boot.sig` signature does not match `boot.img`,
the bootloader will refuse to load the OS.

```bash
./rpiboot -d secure-boot-example
```
Login as `root` with the empty password.

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

### Mount the Compute Module SD/EMMC after enabling secure-boot
Once signed-boot is enabled the bootloader will only load images signed with private key generated earlier.
To boot the Compute Module in mass storage mode a signed version of this code must be generated.

**This signed image MUST NOT be distributed because it gives access to the EMMC.**


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

* Boot the Compute Module in MSD mode as explained in the previous step.
* Copy the `boot.img` and `boot.sig` files from the `secure-boot-example` stage to the mass storage drive: No other files are required.
* Remove the `nRPIBOOT` jumper.
* Power cycle the Compute Module IO board.
* The system should now boot into the OS.

### Modifying / rebuilding `boot.img`
The secure-boot example image can be rebuilt and modified using buildroot. See [raspberrypi-signed-boot](https://github.com/raspberrypi/buildroot/blob/raspberrypi-signed-boot64/README.md) buildroot configuration.
