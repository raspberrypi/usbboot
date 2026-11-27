# Raspberry Pi self-test

A simple hardware health check for BCM2711 (Pi 4, Pi 400, CM4, CM4S) and
BCM2712 (Pi 5, Pi 500, CM5). It is loaded into memory with `rpiboot`, so it
does not need a working bootloader EEPROM, eMMC or SD card on the device.

Run it as a pre-check before the other tools in this repository. It is
read-only:

* It does not write to the OTP, the bootloader EEPROM or the eMMC / SD / NVMe.
* There is no login, no shell and no access to the contents of the storage.
* It does not accept inbound connections.

## Running

The image is downloaded rather than stored in this repository, so that the
current version is always used.

```bash
cd rpi-self-test
./download.sh
sudo ../rpiboot -d .
```

Connect the device as for any other `rpiboot` image (for a Compute Module,
fit the `nRPIBOOT` jumper on the IO board). For the full test also connect:

* An HDMI monitor - the report is shown on one screen at 1080p.
* Ethernet, with DHCP and Internet access.
* Optionally a UART (115200 baud) to capture the report as text.

The report appears once Linux has started. The second line is the status:
it turns green when every test has finished without a problem, and lists the
issues otherwise. Lines which report a problem are red and in upper case,
e.g. `NO LINK`. The memory test runs last and takes several minutes.

The reference platforms for Compute Modules are the CM4 IO board and the
CM5 IO board. On another carrier board, expect the tests of ports which are
not fitted to fail.

## What it sends

To test the Internet connection the image makes one HTTPS request to
`downloads.raspberrypi.com` with the serial number, board revision, SDRAM
size and `over_voltage_avs` voltage of the device, so that Raspberry Pi can
match it with its manufacturing records. Nothing else from the report is
sent. See the source for the details and for how to build an image without
this.

## Secure boot

Once secure-boot has been enabled `boot.img` must be signed with the customer
private key. Read "Should I sign it?" in the source readme first: a signed
image will boot on every device which trusts that key.

```bash
./sign.sh private.pem
```

or as follows if using a HSM wrapper script.

```bash
./sign.sh -H hsm-wrapper public.pem
```

`sign.sh` is the same script as in `mass-storage-gadget64`. It also adds the
public key to `bootfiles.bin` and, for Pi 5, counter-signs the firmware.

WARNING: The signed images will not boot on a Pi 5 without secure-boot
enabled. Run `./reset.sh` to return to the unsigned state.

## Source code

The Buildroot configuration, the self-test script and the full readme are on
the [rpi-self-test](https://github.com/raspberrypi/buildroot/tree/rpi-self-test)
branch of the Raspberry Pi
[buildroot](https://github.com/raspberrypi/buildroot) repo. To use your own
build, copy its `output/images/boot.img` to this directory instead of running
`download.sh`.
