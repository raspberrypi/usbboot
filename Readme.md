# Raspberry Pi USB device boot - RPIBOOT

This repository contains the Raspberry Pi USB device boot software known as `rpiboot`. The `rpiboot` tool provides a file server for loading software into memory on a Raspberry Pi for provisioning. By default, it boots the device with firmware that makes it appear to the host as a USB mass-storage device. The host operating system then treats it as a standard USB drive, allowing the filesystem to be accessed. An operating system image can be written to the device using the [Raspberry Pi Imager](https://www.raspberrypi.com/software/).

On Compute Module 4 and newer devices, `rpiboot` is also used to update the bootloader SPI flash EEPROM.

For more information, run `rpiboot -h`.

### Compatible devices

Devices supporting the fast Linux-based `mass-storage-gadget`

* Raspberry Pi Zero2W
* Raspberry Pi 3A+
* Compute Module 3
* Compute Module 3+
* Compute Module 3E
* Raspberry Pi 4B (requires rpiboot to be enabled first)
* Compute Module 4
* Compute Module 4S
* Raspberry Pi 400 (requires rpiboot to be enabled first)
* Raspberry Pi 5
* Raspberry Pi 500 (requires rpiboot to be enabled first)
* Raspberry Pi 500+
* Compute Module 5

Devices which require the legacy `msd` firmware loading interface
* Raspberry Pi 1A+
* Compute Module 1
* Raspberry Pi Zero

The `mass-storage-gadget` boots a Linux initramfs image that scans for SD/EMMC, NVMe, and USB block devices and uses `configfs` to expose them as USB mass-storage devices. Because it runs Linux, it also provides a console login via both the hardware UART and the USB CDC-UART interfaces.

The legacy `msd` firmware is a special build of the VideoCore firmware that exposes the SD/EMMC device as an emulated USB mass-storage device. The legacy `msd` firmware runs on Raspberry Pi 4 and older devices. It does not provide a console login or diagnostics, and although it loads quickly, write performance is significantly lower than with the Linux-based `mass-storage-gadget`.

### Recommended `rpiboot` host configuration

The recommended host setup for running `rpiboot` is:

* A Raspberry Pi 4 or Raspberry Pi 5 computer
* A Raspberry Pi Powered USB Hub to supply power and data to the Raspberry Pi being provisioned
* High-quality, short USB cables
* Raspberry Pi OS 64-bit (Trixie)

The `rpiboot` host software uses `libusb` to communicate with the Raspberry Pi. `rpiboot` is widely used on Linux x86 (various distributions), Windows 11 (Cygwin), and macOS. Support for other platforms is provided on a best-effort and/or community-maintained basis.


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
* Instead of duplicating the EEPROM binaries and tools, the rpi-eeprom repository
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
sudo ./rpiboot -d mass-storage-gadget
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

## Enabling `rpiboot` support — extra steps for Pi 4B, Pi 400 & Pi 500

### Pi 4B and Pi 400
Raspberry Pi 4B and Pi 400 do not have a dedicated `nRPIBOOT` jumper. Instead, a GPIO on the 40-pin header can be selected which, if held low during boot, forces the boot ROM into `rpiboot` mode.

This is configured using the `make-pi4-rpiboot-gpio-sd` utility, which generates an SD card that programs the target device at power-on with the desired GPIO setting.

* Available GPIOs: `2, 4, 5, 6, 7, 8`
* The selected GPIO can be used normally after boot, but it must **not** be pulled low unless `rpiboot` mode is intended. Confirm that this does not conflict with any HATs you may attach.
* **This option permanently modifies the OTP and cannot be changed afterwards.**

#### Dependencies
This tool must be run on Linux and depends upon the `kpartx` command.
```bash
sudo apt update && sudo apt full-upgrade
sudo apt install kpartx
```

#### Example 
Build an SD card image (`images-2711/pi4-program-rpiboot-gpio8.zip`) that configures GPIO 8 as `nRPIBOOT`:
```bash
sudo ./rpi-eeprom/imager/make-pi4-rpiboot-gpio-sd 8
```


### Pi 500
Pi 500 requires the QMK keyboard firmware to be updated via `raspi-config` to the latest release to enable `rpiboot` through the power button.

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

### Raspberry Pi 5, Pi 500 and Pi 500+
* Disconnect the USB-C cable. Power must be removed rather than just running "sudo shutdown now"
* Hold the power button down
* Connect the USB-C cable (from the `RPIBOOT` host to the Pi 5)

Launch the Linux based mass-storage-gadget
```bash
sudo rpiboot -d mass-storage-gadget
```

Launch the legacy MSD firmware
```bash
sudo rpiboot -d msd
```

<a name="extensions"></a>
## Provisioning extensions
In addition to the MSD functionality, there are a number of other utilities that can be loaded.

| Directory | Description |
| ----------| ----------- |
| [recovery](recovery/README.md) | Updates the bootloader EEPROM on a Compute Module 4 |
| [recovery5](recovery5/README.md) | Updates the bootloader EEPROM on a Raspberry Pi 5 |
| [mass-storage-gadget](mass-storage-gadget/README.md) | Linux mass storage gadget with 64-bit kernel for BCM2710, BCM2711 and BCM2712 |
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
The `rpiboot` "recovery" modules provide a facility to read the device OTP information. This can be run either as a provisioning step or as a standalone operation. Pass the `-j metadata` flag to `rpiboot` to write metadata JSON to a specified "metadata" directory.

Metadata output is enabled by default. To disable add `recovery_metadata=0` to the recovery `config.txt` file.

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
        "MAC_ADDR": "d8:3a:dd:05:ee:78",
        "EEPROM_UPDATE": "success",
        "EEPROM_HASH": "dfc8ef2c77b8152a5cfa008c2296246413fd580fdc26dfacd431e348571a2137",
        "CUSTOMER_KEY_HASH": "8251a63a2edee9d8f710d63e9da5d639064929ce15a2238986a189ac6fcd3cee",
        "BOOT_ROM": "0000c8b0",
        "BOARD_ATTR": "00000000",
        "USER_BOARDREV": "c03141",
        "JTAG_LOCKED": "0",
        "ADVANCED_BOOT": "0000e8e8"
}
```

<a name="secure-boot"></a>
## Secure Boot
See the [secure-boot](docs/secure-boot.md) reference.
