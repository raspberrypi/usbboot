# USB device boot code

This is the USB MSD boot code which should work on the Raspberry Pi model A, Compute Module, Compute
Module 3, Compute Module 4 and Raspberry Pi Zero.

The default behaviour when run with no arguments is to boot the Raspberry Pi with
special firmware so that it emulates USB Mass Storage Device (MSD). The host OS
will treat this as a normal USB mass storage device allowing the file-system
to be accessed. If the storage has not been formatted yet (default for Compute Module)
then the [Raspberry Pi Imager App](https://www.raspberrypi.com/software/) can be
used to install a new operating system.

Since `RPIBOOT` is a generic firmware loading interface it is possible to load
other versions of the firmware by passing the `-d` flag to specify the directory
where the firmware should be loaded from.
E.g. The firmware in the [msd](msd/README.md) can be replaced with newer/older versions.

For more information run `rpiboot -h`

## Building

### Linux / Cygwin / WSL
Clone this on your Pi or a Linux machine.  
Make sure that the system date is set correctly, otherwise Git may produce an error.

```
sudo apt install git libusb-1.0-0-dev
git clone --depth=1 https://github.com/raspberrypi/usbboot
cd usbboot
make
sudo ./rpiboot
```

### macOS
From a macOS machine, you can also run usbboot, just follow the same steps:

1. Clone the `usbboot` repository
2. Install `libusb` (`brew install libusb`)
3. Install `pkg-config` (`brew install pkg-config`)
4. Build using make
5. Run the binary

```
git clone --depth=1 https://github.com/raspberrypi/usbboot
cd usbboot
brew install libusb
brew install pkg-config
make
sudo ./rpiboot
```

## Running

### Compute Module 3
Fit the `EMMC-DISABLE` jumper on the Compute Module IO board before powering on the board
or connecting the USB cable.

### Compute Module 4
On Compute Module 4 EMMC-DISABLE / nRPIBOOT (GPIO 40) must be fitted to switch the ROM to usbboot mode.
Otherwise, the SPI EEPROM bootloader image will be loaded instead.


<a name="extensions"></a>
## Compute Module 4 extensions
In addition to the MSD functionality, there are a number of other utilities that can be loaded
via RPIBOOT on Compute Module 4.

| Directory | Description |
| ----------| ----------- |
| [recovery](recovery/README.md) | Updates the bootloader EEPROM on a Compute Module 4 |
| [rpi-imager-embedded](rpi-imager-embedded/README.md) | Runs the embedded version of Raspberry Pi Imager on the target device |
| [mass-storage-gadget](mass-storage-gadget/README.md) | Replacement for MSD firmware. Uses Linux USB gadgetfs drivers to export all block devices (e.g. NVMe, EMMC) as MSD devices |
| [secure-boot-recovery](secure-boot-recovery/README.md) | Scripts that extend the `recovery` process to enable secure-boot, sign images etc |
| [secure-boot-msd](secure-boot-msd/README.md) | Scripts for signing the MSD firmware so that it can be used on a secure-boot device |
| [secure-boot-example](secure-boot-example/README.md) | Simple Linux initrd with a UART console.

## Booting Linux
The `RPIBOOT` protocol provides a virtual file-system to the Raspberry Pi bootloader and GPU firmware. It's therefore possible to
boot Linux. To do this, you will need to copy all of the files from a Raspberry Pi boot partition plus create your own
initramfs.
On Raspberry Pi 4 / CM4 the recommended approach is to use a `boot.img` which is a FAT disk image containing
the minimal set of files required from the boot partition.

<a name="secure-boot"></a>
## Secure Boot
Secure Boot requires the latest stable bootloader image.
WARNING: If the `revoke_devkey` option is used to revoke the ROM development key then it will
not be possible to downgrade to a bootloader older than 2022-01-06 OR disable secure-boot mode.

### Host setup
Secure boot require a 2048 bit RSA asymmetric keypair and the Python `pycrytodomex` module to sign the EEPROM config and boot image.

#### Install Python Crypto support (the pycryptodomex module)
```bash
python3 -m pip install pycryptodomex
# or
pip install pycryptodomex
```

#### Create an RSA key-pair using OpenSSL. Must be 2048 bits
```bash
cd $HOME
openssl genrsa 2048 > private.pem
```

### Secure Boot - configuration
* Please see the [secure boot EEPROM guide](secure-boot-recovery/README.md) to enable via rpiboot `recovery.bin`.
* Please see the [secure boot MSD guide](secure-boot-msd/README.md) for instructions about to mount the EMMC via USB mass-storage once secure-boot has been enabled.

## Secure Boot - image creation
Secure boot requires a `boot.img` FAT image to be created. This plus a signature file (boot.sig)
must be placed in the boot partition of the Raspberry Pi.

The contents of the `boot.img` are the files normally present in the Raspberry Pi OS boot
partition i.e. firmware, DTBs and kernel image. However, in order to reduce boot time
it is advisable to remove unused files e.g. firmware or kernel images for Pi models.

The firmware must be new enough to support secure boot. The latest firmware APT
package supports secure boot. To download the firmware files directly.

`git clone --depth 1 --branch stable https://github.com/raspberrypi/firmware`

A helper script (`make-boot-image`) is provided to automate the image creation process. This
script depends upon the `mkfs.fat` and `losetup` tools and only runs on Linux.

### Root file-system
Normally, the Kernel modules and overlays for a secure-boot system would be provided
in an [initramfs](https://www.raspberrypi.com/documentation/computers/config_txt.html#initramfs)
and built with (buildroot)[https://buildroot.org/] or (yocto)[https://www.yoctoproject.org/].

This ensures that all of the kernel modules and device tree dependencies are covered
by the secure-boot signature.

Since the `initramfs` is part of the `boot.img` it is possible to replace GPU firmware,
kernel and dependencies in a single file update.

Alternatively, for test/development the following instructions explain how a normal
Raspberry Pi OS install can be modified to be booted with the secure-boot loader.

#### Clone the Raspberry Pi OS boot files
Copy the contents of `/boot` to a local directory called `secure-boot-files`

#### Set the kernel root device
Since the boot file-system for the firmware is now in a signed disk image the OS cannot write to this.
Therefore, any changes to `cmdline.txt` must be made before the `boot.img` file is signed.

* Verify that `cmdline.txt` in `secure-boot-files` points to the correct UUID for the root file-system.
  Alternatively, for testing, you can specify the root device name e.g. `root=/dev/mmcblk0p2`.

* Remove `init-resize.sh` from `cmdline.txt`


#### Create the boot image
The `-b` product argument (pi4,pi400,cm4) tells the script to discard files which are not required by that product. This makes the image smaller and reduces the time taken to calculate the hash of the image file thereby reducing the boot time.
```bash
sudo ../tools/make-boot-image -d secure-boot-files -o boot.img -b pi4
```

The maximum supported size for boot.img is currently 96 megabytes.

#### Verify the boot image
To verify that the boot image has been created correctly use losetup to mount the .img file.

```bash
sudo su
mkdir -p boot-mount
LOOP=$(losetup -f)
losetup -f boot.img
mount ${LOOP} boot-mount/

 echo boot.img contains
find boot-mount/

umount boot-mount
losetup -d ${LOOP}
rmdir boot-mount
```

#### Sign the boot image
For secure-boot, `rpi-eeprom-digest` extends the current `.sig` format of
sha256 + timestamp to include an hex format RSA bit PKCS#1 v1.5 signature. The key length
must be 2048 bits.

```bash
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```
#### Hardware security modules
`rpi-eeprom-digest` is a shell script that wraps a call to `openssl dgst -sign`.
If the private key is stored withing a hardware security module instead of
a .PEM file the `openssl` command will need to be replaced with the appropriate call to the HSM.

`rpi-eeprom-digest` called by `update-pieeprom.sh` to sign the EEPROM config file.

The RSA public key must be stored within the EEPROM so that it can be used by the bootloader.
By default, the RSA public key is automatically extracted from the private key PEM file. Alternatively,
the public key may be specified separately via the `-p` argument to `update-pieeprom.sh` and `rpi-eeprom-config`.

To extract the public key in PEM format from a private key PEM file, run:  
```bash
openssl rsa -in private.pem -pubout -out public.pem`
```

#### Copy the secure boot image to the boot partition on the Raspberry Pi.
Copy `boot.img` and `boot.sig` to the chosen boot filesystem. Secure boot images can be loaded from any of the normal boot devices (e.g. SD, USB, Network).
