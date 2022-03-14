This directory contains the second stage bootloader and firmware for
booting the Raspberry Pi as a mass-storage device. The files here are
embedded into the `rpiboot` executable as part of the build process.

To load the files from this directory directly, run:  

```bash
cd msd
../rpiboot -d .
```
