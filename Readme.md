# USB Device Boot Code

This is the USB device boot code which supports the Raspberry Pi 1A, 3A+, Compute Module, Compute
Module 3, 3+, 4S, 4 and 5, Raspberry Pi Zero and Zero 2 W.

The default behaviour when run with no arguments is to boot the Raspberry Pi with
special firmware so that it emulates USB Mass Storage Device (MSD). The host OS
will treat this as a normal USB mass storage device allowing the file system
to be accessed. If the storage has not been formatted yet (default for Compute Module)
then the [Raspberry Pi Imager App](https://www.raspberrypi.com/software/) can be
used to install a new operating system.

Since `RPIBOOT` is a generic firmware loading interface, it is possible to load
other versions of the firmware by passing the `-d` flag to specify the directory
where the firmware should be loaded from. For example, the firmware in the
[msd](msd/README.md) directory can be replaced with newer or older versions.

From Raspberry Pi 4 onwards, the MSD VPU firmware has been replaced with the Linux-based mass storage gadget.

For more information run `rpiboot -h`.

## Building

Once compiled, `rpiboot` can either be run locally from the source directory by specifying
the directory of the boot image e.g. `sudo ./rpiboot -d mass-storage-gadget`.
If no arguments are specified, `rpiboot` will attempt to boot the mass-storage-gadget
from `INSTALL_PREFIX/share/mass-storage-gadget64`.

The Raspberry Pi OS APT package sets `INSTALL_PREFIX` to `/usr`.

### Linux / Cygwin / WSL
Clone this repository on your Pi or other Linux machine.
Make sure that the system date is set correctly, otherwise Git may produce an error.

* This git repository uses symlinks. For Windows builds, clone the repository under Cygwin and make sure symlinks are enabled. `git config --get core.symlinks` should return true. You can enable symlinks by passing `-c core.symlinks=true` to the "clone" command or enable them globally with `git config --global core.symlinks true`.
* On Windows, make sure you have run the `rpiboot` driver installer once; see `usbboot\win32\rpiboot_setup`.
* Instead of duplicating the EEPROM binaries and tools the rpi-eeprom repository
  is included as a [git submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules)

#### apt (Debian/Ubuntu)
```bash
sudo apt install git libusb-1.0-0-dev pkg-config build-essential
```

#### dnf (Fedora/RHEL)
```bash
sudo dnf install git libusb1-devel pkg-config glibc-devel g++ gcc make
```

#### Building
```bash
git clone --recurse-submodules --shallow-submodules --depth=1 https://github.com/raspberrypi/usbboot
cd usbboot
make
# Either
sudo ./rpiboot -d mass-storage-gadget64
# Or, install rpiboot to /usr/bin and boot images to /usr/share
sudo make install
sudo rpiboot

```

`sudo` isn't required if you have write permissions for the `/dev/bus/usb` device.

### macOS
From a macOS machine, you can also run `usbboot`; just follow the same steps:

1. Clone the `usbboot` repository
2. Install `libusb` (`brew install libusb`)
3. Install `pkg-config` (`brew install pkg-config`)
4. (Optional) Export the `PKG_CONFIG_PATH` so that it includes the directory enclosing `libusb-1.0.pc`
5. Build using make - installing to /usr/local rather than /usr/bin is recommended on macOS
6. Run the binary

```bash
git clone --recurse-submodules --shallow-submodules --depth=1 https://github.com/raspberrypi/usbboot
cd usbboot
brew install libusb
brew install pkg-config
make INSTALL_PREFIX=/usr/local
# Either
sudo ./rpiboot -d mass-storage-gadget64
# Or, install rpiboot to /usr/local/bin and boot images to /usr/local/share
sudo make INSTALL_PREFIX=/usr/local install
sudo rpiboot
```

If the build is unable to find the header file `libusb.h` then most likely the `PKG_CONFIG_PATH` is not set properly.
This should be set via `export PKG_CONFIG_PATH="$(brew --prefix libusb)/lib/pkgconfig"`.

If the build fails on an ARM-based Mac with a linker error such as `ld: warning: ignoring file '/usr/local/Cellar/libusb/1.0.27/lib/libusb-1.0.0.dylib': found architecture 'x86_64', required architecture 'arm64'` then you may need to build and install `libusb-1.0` yourself:
```
curl -OL https://github.com/libusb/libusb/releases/download/v1.0.27/libusb-1.0.27.tar.bz2
tar -xf libusb-1.0.27.tar.bz2
cd libusb-1.0.27
./configure
make
make check
sudo make INSTALL_PREFIX=/usr/local install
cd ..
```
Running `make` again should now succeed.

### Updating the rpi-eeprom submodule
After updating the `usbboot` repo (`git pull --rebase origin master`), update the
submodules by running:

```bash
git submodule update --init
```

## Running

### Compute Module 3
Fit the `EMMC-DISABLE` jumper on the Compute Module IO board before powering on the board
or connecting the USB cable.

### Compute Module 4
On Compute Module 4, EMMC-DISABLE / nRPIBOOT (GPIO 40) must be fitted to switch the ROM to `usbboot` mode.
Otherwise, the SPI EEPROM bootloader image will be loaded instead.

### Compute Module 5
On Compute Module 5, EMMC-DISABLE / nRPIBOOT (BCM2712 GPIO 20) must be fitted to switch the ROM to `usbboot` mode.
Otherwise, the SPI EEPROM bootloader image will be loaded instead.

### Raspberry Pi 5
* Disconnect the USB-C cable. Power must be removed rather than just running "sudo shutdown now"
* Hold the power button down
* Connect the USB-C cable (from the `RPIBOOT` host to the Pi 5)

<a name="extensions"></a>
## Compute Module provisioning extensions
In addition to the MSD functionality, there are a number of other utilities that can be loaded
via RPIBOOT on Compute Module 4 and Compute Module 5.

| Directory | Description |
| ----------| ----------- |
| [recovery](recovery/README.md) | Updates the bootloader EEPROM on a Compute Module 4 |
| [recovery5](recovery5/README.md) | Updates the bootloader EEPROM on a Raspberry Pi 5 |
| [mass-storage-gadget64](mass-storage-gadget64/README.md) | Linux mass storage gadget with 64-bit kernel for BCM2710, BCM2711 and BCM2712 |
| [secure-boot-recovery](secure-boot-recovery/README.md) | Pi4 secure-boot bootloader flash and OTP provisioning |
| [secure-boot-recovery5](secure-boot-recovery5/README.md) | Pi5 secure-boot bootloader flash and OTP provisioning |
| [rpi-imager-embedded](rpi-imager-embedded/README.md) | Runs the embedded version of Raspberry Pi Imager on the target device |
| [secure-boot-example](secure-boot-example/README.md) | Simple Linux initrd with a UART console. |

The APT package for `rpiboot` installs these utility directories to `/usr/share/rpiboot`.

## Booting Linux
The `RPIBOOT` protocol provides a virtual file system to the Raspberry Pi bootloader and GPU firmware. It's therefore possible to
boot Linux. To do this, you will need to copy all of the files from a Raspberry Pi boot partition plus create your own
initramfs.
On Raspberry Pi 4 / CM4 the recommended approach is to use a `boot.img` which is a FAT disk image containing
the minimal set of files required from the boot partition.

## Troubleshooting
See the [troubleshooting guide](docs/troubleshooting.md).

## Reading device metadata from OTP via rpiboot
The `rpiboot` "recovery" modules provide a facility to read the device OTP information. This can be run either as a provisioning step or as a standalone operation.

To enable this make sure that `recovery_metadata=1` is set in the recovery `config.txt` file and pass the `-j metadata` flag to `rpiboot`.

See [board revision](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#new-style-revision-codes-in-use) documentation to decode the `BOARD_ATTR` field.

Example command to extract the OTP metadata from a Compute Module 4:
```bash
cd recovery
mkdir -p metadata
sudo rpiboot -j metadata -d .
```

Example metadata file contents written to `metadata/SERIAL_NUMBER.json`:
```json
{
        "MAC_ADDR" : "d8:3a:dd:05:ee:78",
        "CUSTOMER_KEY_HASH" : "8251a63a2edee9d8f710d63e9da5d639064929ce15a2238986a189ac6fcd3cee",
        "BOOT_ROM" : "0000c8b0",
        "BOARD_ATTR" : "00000000",
        "USER_BOARDREV" : "c03141",
        "JTAG_LOCKED" : "0",
        "ADVANCED_BOOT" : "0000e8e8"
}
```

<a name="secure-boot"></a>
## Secure Boot
See the [secure-boot](docs/secure-boot.md) reference.
