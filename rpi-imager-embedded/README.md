# Raspberry Pi Imager - Embedded

This directory contains the configuration files required to launch the
embedded version of the Raspberry Pi imager via rpiboot.
This requires a Raspberry Pi 4 or newer.

The `boot.img` file is no longer stored in this repository.

To download the latest version, run:  
```bash
wget https://downloads.raspberrypi.com/net_install/boot.img
```

To run:  
```bash
cd rpi-imager-embedded
../rpiboot -d .
```

Make sure that the HDMI display is connected. Once Linux has started
you will need to unplug the micro-USB cable (when prompted) and connect
a keyboard and mouse.
