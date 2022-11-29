# USB Device Boot Code

This is the USB MSD boot code which supports the Raspberry Pi 1A, 3A+, Compute Module, Compute
Module 3, 3+ and 4, Raspberry Pi Zero and Zero 2 W.

The default behaviour when run with no arguments is to boot the Raspberry Pi with
special firmware so that it emulates USB Mass Storage Device (MSD). The host OS
will treat this as a normal USB mass storage device allowing the file system
to be accessed. If the storage has not been formatted yet (default for Compute Module)
then the [Raspberry Pi Imager App](https://www.raspberrypi.com/software/) can be
used to install a new operating system.

Since `RPIBOOT` is a generic firmware loading interface, it is possible to load
other versions of the firmware by passing the `-d` flag to specify the directory
where the firmware should be loaded from.
E.g. The firmware in the [msd](msd/README.md) can be replaced with newer/older versions.

For more information run `rpiboot -h`.

## Building

### Linux / Cygwin / WSL
Clone this repository on your Pi or other Linux machine.
Make sure that the system date is set correctly, otherwise Git may produce an error.

**This git repository uses symlinks. For Windows builds clone the repository under Cygwin**

```
sudo apt install git libusb-1.0-0-dev pkg-config
git clone --depth=1 https://github.com/raspberrypi/usbboot
cd usbboot
make
sudo ./rpiboot
```

`sudo` isn't required if you have write permissions for the `/dev/bus/usb` device.

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
## Compute Module 4 Extensions
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

**The `secure-boot-msd`, `rpi-imager-embedded` and `mass-storage-gadget` extensions require that the `2022-04-26` (or newer) bootloader EEPROM release has already been written to the EEPROM using `recovery.bin`**

## Booting Linux
The `RPIBOOT` protocol provides a virtual file system to the Raspberry Pi bootloader and GPU firmware. It's therefore possible to
boot Linux. To do this, you will need to copy all of the files from a Raspberry Pi boot partition plus create your own
initramfs.
On Raspberry Pi 4 / CM4 the recommended approach is to use a `boot.img` which is a FAT disk image containing
the minimal set of files required from the boot partition.

## Troubleshooting

This section describes how to diagnose common `rpiboot` failures for Compute Modules. Whilst `rpiboot` is tested on every Compute Module during manufacture the system relies on multiple hardware and software elements. The aim of this guide is to make it easier to identify which component is failing.

### Product Information Portal
The [Product Information Portal](https://pip.raspberrypi.com/) contains the official documentation for hardware revision changes for Raspberry Pi computers.
Please check this first to check that the software is up to date.

### Hardware
* Inspect the Compute Module pins and connector for signs of damage and verify that the socket is free from debris.
* Check that the Compute Module is fully inserted.
* Check that `nRPIBOOT` / EMMC disable is pulled low BEFORE powering on the device.
   * On BCM2711, if the USB cable is disconected and the nRPIBOOT jumper is fitted then the green LED should be OFF. If the LED is on then the ROM is detecting that the GPIO for nRPIBOOT is high.
* Remove any hubs between the Compute Module and the host.
* Disconnect all other peripherals from the IO board.
* Verify that the red power LED switches on when the IO board is powered.
* Use another computer to verify that the USB cable for `rpiboot` can reliably transfer data. For example, connect it to a Raspberry Pi keyboard with other devices connected to the keyboard USB hub.

#### Hardware - CM4
* The CM4 EEPROM supports MMC, USB-MSD, USB 2.0, Network and NVMe boot by default. Try booting to Linux from an alternate boot mode (e.g. network) to verify the `nRPIBOOT` GPIO can be pulled low and that the USB 2.0 interface is working.
* If `rpiboot` is running but the mass storage device does not appear then try running the `rpiboot -d mass-storage-gadget` because this uses Linux instead of a custom VPU firmware to implement the mass-storage gadget. This also provides a login console on UART and HDMI.

### Software
The recommended host setup is Raspberry Pi with Raspberry Pi OS. Alternatively, most Linux X86 builds are also suitable. Windows adds some extra complexity for the USB drivers so we recommend debugging on Linux first.

* Update to the latest software release using `apt update rpiboot` or download and rebuild this repository from Github.
* Run `rpiboot -v | tee log` to capture verbose log output. N.B. This can be very verbose on some systems.

#### Boot flow
The `rpiboot` system runs in multiple stages. The ROM, bootcode.bin, the VPU firmware (start.elf) and for the `mass-storage-gadget` or `rpi-imager` a Linux initramfs. Each stage disconnects the USB device and presents a different USB descriptor. Each stage will appears as a new USB device connect in the `dmesg` log.

See also: [Raspberry Pi4 Boot Flow](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#raspberry-pi-4-boot-flow)

#### bootcode.bin
Be careful not to overwrite `bootcode.bin` or `bootcode4.bin` with the executable from a different subdirectory. The `rpiboot` process simply looks for a file called `bootcode.bin` (or `bootcode4.bin` on BCM2711). However, the file in `recovery`/`secure-boot-recovery` directories is actually the `recovery.bin` EEPROM flashing tool.

### Diagnostics
* Monitor the Linux `dmesg` output and verify that a BCM boot device is detected immediately after powering on the device. If not, please check the `hardware` section.
* Check the green activity LED. On Compute Module 4 this is activated by the software bootloader and should remain on. If not, then it's likely that the initial USB transfer to the ROM failed.
* On Compute Module 4 connect a HDMI monitor for additional debug output. Flashing the EEPROM using `recovery.bin` will show a green screen and the `mass-storage-gadget` enables a console on the HDMI display.
* If `rpiboot` starts to download `bootcode4.bin` but the transfer fails then can indicate a cable issue OR a corrupted file. Check the hash of `bootcode.bin` file against this repository and check `dmesg` for USB error.
* If `bootcode.bin` or the `start.elf` detects an error then [error-code](https://www.raspberrypi.com/documentation/computers/configuration.html#led-warning-flash-codes) will be indicated by flashing the green activity LED.
* Add `uart_2ndstage=1` to the `config.txt` file in `msd/` or `recovery/` directories to enable UART debug output.

<a name="secure-boot"></a>
## Secure Boot
Secure Boot requires a recent bootloader stable image e.g. the version in this repository.

### Tutorial
Creating a secure boot system from scratch can be quite complex. The [secure boot tutorial](secure-boot-example/README.md) uses a minimal example OS image to demonstrate how the Raspberry Pi-specific aspects of secure boot work.

### Host Setup
Secure boot require a 2048 bit RSA asymmetric keypair and the Python `pycrytodomex` module to sign the bootloader EEPROM config and boot image.

#### Install Python Crypto Support (the pycryptodomex module)
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
Secure Boot requires self-contained ramdisk (`boot.img`) FAT image to be created containing the GPU
firmware, kernel and any other dependencies that would normally be loaded from the boot partition.

This plus a signature file (`boot.sig`) must be placed in the boot partition of the Raspberry Pi
or network download location.

The `boot.img` file should contain:-
* The kernel
* Device tree overlays
* GPU firmware (start.elf and fixup.dat)
* Linux initramfs containing the application OR scripts to mount/create an encrypted file-system.


### Disk encryption
Secure-boot is responsible for loading the Kernel + initramfs and loads all of the data
from a single `boot.img` file stored on an unencrypted FAT/EFI partition.

**There is no support in the ROM or firmware for full-disk encryption.**

If a custom OS image needs to use an encrypted file-system then this would normally be implemented
via scripts within the initramfs.

Raspberry Pi computers do not have a secure enclave, however, it's possible to store a 256 bit
[device specific private key](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#device-specific-private-key)
in OTP. The key is accessible to any process with access to `/dev/vcio` (`vcmailbox`), therefore, the
secure-boot OS must ensure that access to this interface is restricted.

**It is not possible to prevent code running in ARM supervisor mode (e.g. kernel code) from accessing OTP hardware directly**

See also:-
* [LUKS](https://en.wikipedia.org/wiki/Linux_Unified_Key_Setup)
* [cryptsetup FAQ](https://gitlab.com/cryptsetup/cryptsetup/-/wikis/FrequentlyAskedQuestions)
* [rpi-otp-private-key](../tools/rpi-otp-private-key)

The [secure boot tutorial](secure-boot-example/README.md) contains a `boot.img` that supports cryptsetup and a simple example.

### Building `boot.img` using buildroot

The `secure-boot-example` directory contains a simple `boot.img` example with working HDMI,
network, UART console and common tools in an initramfs.

This was generated from the [raspberrypi-signed-boot](https://github.com/raspberrypi/buildroot/blob/raspberrypi-signed-boot/README.md)
buildroot config. Whilst not a generic fully featured configuration it should be relatively
straightforward to cherry-pick the `raspberrypi-secure-boot` package and helper scripts into
other buildroot configurations.

#### Minimum firmware version
The firmware must be new enough to support secure boot. The latest firmware APT
package supports secure boot. To download the firmware files directly.

```bash
git clone --depth 1 --branch stable https://github.com/raspberrypi/firmware
```

To check the version information within a `start4.elf` firmware file run
```bash
strings start4.elf | grep VC_BUILD_
```

#### Verifying the contents of a `boot.img` file
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

#### Signing the boot image
For secure-boot, `rpi-eeprom-digest` extends the current `.sig` format of
sha256 + timestamp to include an hex format RSA bit PKCS#1 v1.5 signature. The key length
must be 2048 bits.

```bash
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```

To verify the signature of an existing image set the `PUBLIC_KEY_FILE` environment variable
to the path of the public key file in PEM format.

```bash
../tools/rpi-eeprom-digest -i boot.img -k "${PUBLIC_KEY_FILE}" -v boot.sig
```


#### Hardware security modules
`rpi-eeprom-digest` is a shell script that wraps a call to `openssl dgst -sign`.
If the private key is stored within a hardware security module instead of
a .PEM file the `openssl` command will need to be replaced with the appropriate call to the HSM.

`rpi-eeprom-digest` called by `update-pieeprom.sh` to sign the EEPROM config file.

The RSA public key must be stored within the EEPROM so that it can be used by the bootloader.
By default, the RSA public key is automatically extracted from the private key PEM file. Alternatively,
the public key may be specified separately via the `-p` argument to `update-pieeprom.sh` and `rpi-eeprom-config`.

To extract the public key in PEM format from a private key PEM file, run:
```bash
openssl rsa -in private.pem -pubout -out public.pem
```

#### Copy the secure boot image to the boot partition on the Raspberry Pi.
Copy `boot.img` and `boot.sig` to the boot filesystem.
Secure boot images can be loaded from any of the normal boot modes (e.g. SD, USB, Network).
