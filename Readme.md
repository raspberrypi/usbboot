# USB boot code

This is the USB MSD boot code which should work on the Raspberry Pi model A, Compute Module, Compute
Module 3, Compute Module 4 and Raspberry Pi Zero.

This version of rpiboot has been modified to work from directories which contain the booting
firmware.  There is a msd/ directory which contains bootcode.bin and start.elf to turn
the Raspberry Pi device into a USB Mass Storage Device (MSD). If run without arguments
embedded versions of bootcode.bin and start.elf are used to enable the MSD behaviour.

For more information run 'rpiboot -h'

## Building

### Ubuntu
Clone this on your Pi or an Ubuntu linux machine

```
git clone --depth=1 https://github.com/raspberrypi/usbboot
cd usbboot
sudo apt install libusb-1.0-0-dev
make
sudo ./rpiboot
```

### macOS
From a macOS machine, you can also run usbboot, just follow the same steps:

1. Clone the `usbboot` repository
2. Install `libusb` (`brew install libusb`)
3. Build using make
4. Run the binary

```
git clone --depth=1 https://github.com/raspberrypi/usbboot
cd usbboot
brew install libusb
make
sudo ./rpiboot
```

**Note:** You might see an OS warning message about a new disk that it can't access, click "ignore", this likely means that the storage is empty and has no filesystem. From here I recommend installing an OS using the [Raspberry Pi Imager App](https://www.raspberrypi.org/software/), or using any other means like `dd`.

## Running your own (not MSD) build

If you would like to boot the Raspberry Pi with a standard build you just need to copy the FAT partition
files into a subdirectory (it must have at the minimum bootcode.bin and start.elf).  If you take a
standard firmware release then this will at the very least boot the linux kernel which will then stop
(and possibly crash!) when it looks for a filesystem.  To provide a filesystem there are many options,
you can build an initramfs into the kernel, add an initramfs to the boot directory or provide some
other interface to the filesystem.

```bash
sudo ./rpiboot -d boot
```

This will serve the boot directory to the Raspberry Pi Device.

## Compute Module 4
On Compute Module 4 EMMC-DISABLE / nRPIBOOT (GPIO 40) must be fitted to switch the ROM to usbboot mode.
Otherwise, the SPI EEPROM bootloader image will be loaded instead.

<a name="secure-boot"></a>
## Secure Boot -  BETA
Secure Boot is currently a BETA release feature and the functionality to permanently enable secure-boot via OTP is not enabled in this release.

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
Secure boot requires a boot.img FAT image to be created. This plus a signature file (boot.sig)
must be placed in the boot partition of the Raspberry Pi.

The contents of the boot.img are the files normally present in the Raspberry Pi OS boot
partition i.e. firmware, DTBs and kernel image. However, in order to reduce boot time
it is advisable to remove unused files e.g. firmware or kernel images for Pi models.

The firmware must be new enough to support secure boot. The latest firmware APT
package supports secure boot. To download the firmware files directly.

`git clone --depth 1 --branch stable https://github.com/raspberrypi/firmware`

A helper script (`make-boot-image`) is provided to automate the image creation process. This
script depends upon the `mkfs.fat` and `losetup` tools and only runs on Linux.

#### Clone the Raspberry Pi OS boot files
Copy the contents of `/boot` to a local directory called `secure-boot-files`

#### Set the kernel root device
Since the boot filesystem for the firmware is now in a signed disk image the OS cannot write to this.
Therefore, any changes to `cmdline.txt` must be made before the `boot.img` file is signed.

* Verify that `cmdline.txt` in `secure-boot-files` points to the correct UUID for the root file-system.
  Alternatively, for testing, you can specify the root device name e.g. `root=/dev/mmcblk0p2`.

* Remove `init-resize.sh` from `cmdline.txt`


#### Create the boot image
The `-p` product argument (pi4,pi400,cm4) tells the script to discard files which are not required by that product. This makes the image smaller and reduces the time taken to calculate the hash of the image file thereby reducing the boot time.
```bash
sudo ../tools/make-boot-image -d secure-boot-files -o boot.img -p pi4
```

The maximum supported size for boot.img is currently 96 megabytes.

#### Sign the boot image
```bash
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```

#### Copy the secure boot image to the boot partition on the Raspberry Pi.
Copy `boot.img` and `boot.sig` to the chosen boot filesystem. Secure boot images can be loaded from any of the normal boot devices (e.g. SD, USB, Network).

### Raspberry Pi Imager - BETA
The Raspberry Pi Imager can be run natively on the CM4 providing a GUI for downloading and installing the operating system.

Beta notes:
* The current version runs rpi-update upon completion in order to update the firwamre and kernel.
* uart_2ndstage is enabled 
* The HDMI display is limited to 1080p to avoid potential problems with cables etc if a 4K display is attached.

Run Raspberry Pi Imager:  
```bash
sudo ./rpiboot -d imager
```

Once the imager is running you will be prompted to remove the micro-usb cable and connect a mouse.
