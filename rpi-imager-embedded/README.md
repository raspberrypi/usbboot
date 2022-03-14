This directory contains the embedded Raspberry Pi Imager application. 
Running RPi Imager on the target device is probably the simplest way
to install the operating system on a Compute Module 4. However, it 
does not export the CM4 file-system as a mass-storage device.

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
