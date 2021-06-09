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

```
sudo ./rpiboot -d boot
```

This will serve the boot directory to the Raspberry Pi Device.

## Compute Module 4
On Compute Module 4 EMMC-DISABLE / nRPIBOOT (GPIO 40) must be fitted to switch the ROM to usbboot mode.
Otherwise, the SPI EEPROM bootloader image will be loaded instead.

### Raspberry Pi Imager - BETA
The Raspberry Pi Imager can be run natively on the CM4 providing a GUI for downloading and installing the operating system.

Beta notes:
* The current version runs rpi-update upon completion in order to update the firwamre and kernel to support NVMe.
* uart_2ndstage is enabled 
* The HDMI display is limited to 1080p to avoid potential problems with cables etc if a 4K display is attached.

For NVMe boot update the bootloader first:  
```
sudo ./rpiboot -d nvme
```

Run Raspberry Pi Imager:  
```
sudo ./rpiboot -d imager
```

Once the imager is running you will be prompted to remove the micro-usb cable and connect a mouse.
