# USB boot code

This is the USB MSD boot code which should work on the Raspberry Pi model A, Compute Module, Compute
Module 3, Compute Module 4 and Raspberry Pi Zero.

This version of rpiboot has been modified to work from directories which contain the booting
firmware.  There is a msd/ directory which contains bootcode.bin and start.elf to turn
the Raspberry Pi device into a USB Mass Storage Device (MSD). If run without arguments
embedded versions of bootcode.bin and start.elf are used to enable the MSD behaviour.

For more information run 'rpiboot -h'

## Compute Module 4
On Compute Module 4 the EMMC disable jumper (GPIO 40) must be fitted to switch
the ROM to usbboot mode. Otherwise, the SPI EEPROM bootloader image will be loaded
instead.

## Building

Clone this on your Pi or an Ubuntu linux machine

```
$ git clone --depth=1 https://github.com/raspberrypi/usbboot
$ cd usbboot
$ sudo apt install libusb-1.0-0-dev
$ make
$ sudo ./rpiboot
```

## Running your own (not MSD) build

If you would like to boot the Raspberry Pi with a standard build you just need to copy the FAT partition
files into a subdirectory (it must have at the minimum bootcode.bin and start.elf).  If you take a
standard firmware release then this will at the very least boot the linux kernel which will then stop
(and possibly crash!) when it looks for a filesystem.  To provide a filesystem there are many options,
you can build an initramfs into the kernel, add an initramfs to the boot directory or provide some
other interface to the filesystem.

```
$ sudo ./rpiboot -d boot
```

This will serve the boot directory to the Raspberry Pi Device.

