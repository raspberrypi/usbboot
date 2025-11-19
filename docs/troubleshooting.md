# Troubleshooting

This section describes how to diagnose common `rpiboot` failures for Compute Modules. Whilst `rpiboot` is tested on every Compute Module during manufacture the system relies on multiple hardware and software elements. The aim of this guide is to make it easier to identify which component is failing.

## Product Information Portal
The [Product Information Portal](https://pip.raspberrypi.com/) contains the official documentation for hardware revision changes for Raspberry Pi computers.
Please check this first to check that the software is up to date.

## Hardware
* Inspect the Compute Module pins and connector for signs of damage and verify that the socket is free from debris.
* Check that the Compute Module is fully inserted.
* Check that `nRPIBOOT` / EMMC disable is pulled low BEFORE powering on the device.
   * On BCM2711, if the USB cable is disconnected and the nRPIBOOT jumper is fitted then the green LED should be OFF. If the LED is on then the ROM is detecting that the GPIO for nRPIBOOT is high.
* Remove any hubs between the Compute Module and the host.
* Disconnect all other peripherals from the IO board.
* Verify that the red power LED switches on when the IO board is powered.
* Use another computer to verify that the USB cable for `rpiboot` can reliably transfer data. For example, connect it to a Raspberry Pi keyboard with other devices connected to the keyboard USB hub.

### Hardware - CM4 / CM5
* The CM5 EEPROM supports MMC, USB-MSD, USB 2.0 (CM4 only), Network and NVMe boot by default. Try booting to Linux from an alternate boot mode (e.g. network) to verify the `nRPIBOOT` GPIO can be pulled low and that the USB 2.0 interface is working.
* If `rpiboot` is running but the mass storage device does not appear then try running the `rpiboot -d mass-storage-gadget64` because this uses Linux instead of a custom VPU firmware to implement the mass-storage gadget. This also provides a login console on UART and HDMI.

### Hardware - Raspberry Pi 5 / Compute Module 5
* Press, and hold the power button before supplying power to the device.
* Release the power button immediately after supplying power to the device.
* Remove any non-essential USB peripherals or HATs.
* Use a USB-3 port capable of supplying at least 900mA and use a high quality USB-C cable OR supply additional power via the 40-pin header.

## Software
The recommended host setup is Raspberry Pi with Raspberry Pi OS. Alternatively, most Linux X86 builds are also suitable. Windows adds some extra complexity for the USB drivers so we recommend debugging on Linux first.

* Update to the latest software release using `apt update rpiboot` or download and rebuild this repository from Github.
* Run `rpiboot -v | tee log` to capture verbose log output. N.B. This can be very verbose on some systems.

### Boot flow
The `rpiboot` system runs in multiple stages. The ROM, bootcode.bin, the VPU firmware (start.elf) and for the `mass-storage-gadget64` or `rpi-imager` a Linux initramfs. Each stage disconnects the USB device and presents a different USB descriptor. Each stage will appears as a new USB device connect in the `dmesg` log.

See also: [EEPROM boot flow](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#eeprom-boot-flow)

### bootcode.bin
Be careful not to overwrite `bootcode.bin` or `bootcode4.bin` with the executable from a different subdirectory. The `rpiboot` process simply looks for a file called `bootcode.bin` (or `bootcode4.bin` on BCM2711). However, the file in `recovery`/`secure-boot-recovery` directories is actually the `recovery.bin` EEPROM flashing tool.
