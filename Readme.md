# USB boot code

This is the USB MSD (Mass Storage Device) boot code which should work on the following Raspberry Pi models:
- Pi Compute Module 
- Pi Compute Module 3
- Pi Zero
- Pi Zero W
- Pi A
- Pi A+
- Pi 3A+

Read more about this Raspberry Pi boot mode in the [official documentation](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bootmodes/device.md).

This version of rpiboot has been modified to work with and from directories which contain Raspberry Pi booting
firmware (ie: bootcode.bin etc.).

This version of rpiboot comes comes with a default directory `msd/` which contains `bootcode.bin` and `start.elf` to turn the above Raspberry Pi models into a USB Mass Storage Device.

## Building

First, clone this repository on your Pi or a general Linux machine:
```
$ git clone --depth=1 https://github.com/raspberrypi/usbboot
```
To build, you'll need the following package installed. Install it now if you don't already have it.

On Debian and derivatives, such as Ubuntu:
```bash
$ sudo apt install libusb-1.0-0-dev
```

On Fedora, RHEL etc.:
```bash
$ sudo dnf install libusb-devel
```

You can now build the software:
```bash
$ cd usbboot
$ make
```
Lastly, to start the program:
```bash
$ sudo ./rpiboot
```

## Running your own build (not the bundled "MSD" one)

If you would like to boot the Raspberry Pi with a standard build, you just need to copy the content of a FAT partition (ie: the files in that partition) into a subdirectory. It must have at minimum `bootcode.bin` and `start.elf`.

If you take a standard firmware release then this will at the very least boot the linux kernel which will then stop
(and possibly crash!) when it looks for a filesystem.

To provide a filesystem there are many options, you can build an initramfs into the kernel, add an initramfs to the boot directory or provide some other interface to the filesystem.

```
$ sudo ./rpiboot -d boot
```

This will serve the directory called "boot" to the Raspberry Pi Device.

