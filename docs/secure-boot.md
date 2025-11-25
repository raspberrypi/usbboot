# Secure boot

This page describes how to enable secure-boot mode using the `rpiboot` provisioning tool and provides a low-level overview of how secure-boot works on a Raspberry Pi 4 (or newer). These tools are used by the [Raspberry Pi - Secure Boot Provisioner](https://github.com/raspberrypi/rpi-sb-provisioner), the officially supported application for provisioning secure-boot on Raspberry Pi devices.  

Before programming secure-boot, a suitable OS image must be created. This is part of the application or product design process and is outside the scope of this document. We recommend using [rpi-image-gen](https://github.com/raspberrypi/rpi-image-gen) to generate custom Debian-based operating system images, though it is also possible to use Yocto or Buildroot.

**Once secure-boot has been enabled by programming the one-time programmable (OTP) fuses, it cannot be disabled and a different key cannot be programmed.**


## Secure-boot overview

## Verified boot flow - chain-of-trust
Secure-boot uses cryptographic signing to ensure the OS kernel and all required boot components loaded into RAM are authenticated using the customer’s private key. The bootloader firmware verifies these signatures, and the bootloader itself is verified by the SoC BootROM using the Raspberry Pi public keys embedded in the BootROM. The BootROM is fused into the silicon and serves as the immutable root of trust.

### Secure-boot high-level flow
* BootROM loads the second-stage (`bootsys`) from SPI flash and verifies it is signed by a valid Raspberry Pi BootROM key.  
* `bootsys` loads the customer’s public key from EEPROM and checks it against the customer key hash stored in OTP, previously written via `rpiboot`.  
* `bootsys` initializes SDRAM and loads `bootmain`. When loading firmware dependencies from SPI flash, it verifies each dependency’s hash against a list embedded within `bootsys` at build time.  
* `bootmain` loads the OS boot ramdisk files (`boot.img` and `boot.sig`) from SD/EMMC/USB/NVMe/Network into memory. It verifies that the hash of `boot.img` matches `boot.sig` and that the RSA signature in `boot.sig` can be validated using the customer’s public key.  
* `bootmain` then uses the verified ramdisk to load the GPU firmware (`start.elf`) and start the operating system. With secure-boot enabled, firmware only loads files from the verified ramdisk, ensuring the OS kernel and all dependencies are authenticated against the customer’s key.  

If any signature or hash verification fails, the current boot mode is aborted and the firmware advances to the next boot mode.

### Pi 4 vs Pi 5 secure-boot differences
On Raspberry Pi 4 devices using the BCM2711 SoC, the boot ROM only checks that `bootsys` is signed by Raspberry Pi’s key.

On Raspberry Pi 5 devices using the BCM2712 SoC, when secure-boot is enabled, the boot ROM requires `bootsys` to be signed by Raspberry Pi’s private key *and* counter-signed with the customer’s private key. This allows customers to authorize specific Raspberry Pi bootloader firmware versions: a firmware update cannot be installed unless the customer signs it.

See also:-
* Secure boot BCM2711 [chain of trust diagram](secure-boot-chain-of-trust-2711.pdf).
* Secure boot BCM2712 [chain of trust diagram](secure-boot-chain-of-trust-2712.pdf).

## boot.img files
Secure-boot requires a self-contained ramdisk (`boot.img`) FAT image containing the GPU firmware, kernel and any other dependencies that would normally be loaded from the boot partition.

This enables the bootloader to verify that the kernel and all dependencies loaded into memory can be validated against the signature in `boot.sig`, signed with the customer's private RSA key.

The `boot.img` and corresponding `boot.sig` file must be placed in the device's boot partition or network download location.

The `boot.img` file should contain:
* config.txt
* Device tree and overlay files
* GPU firmware (`start.elf` and `fixup.dat`)
* Linux kernel image
* Linux initramfs containing the application OR scripts to mount/create an encrypted filesystem.

### Additional documentation

* Secure boot [configuration properties](https://www.raspberrypi.com/documentation/computers/config_txt.html#secure-boot-configuration-properties).
* Device tree [bootloader signed-boot property](https://www.raspberrypi.com/documentation/computers/configuration.html#bcm2711-and-bcm2712-specific-bootloader-properties-chosenbootloader).
* Device tree [public key - NVMEM property](https://www.raspberrypi.com/documentation/computers/configuration.html#nvmem-nodes).
* Raspberry Pi [OTP registers](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#otp-register-and-bit-definitions).
* Raspberry Pi [device specific private key](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#device-specific-private-key).

## Configuring and running the rpiboot secure-boot tools

### Host Setup
Secure boot requires a 2048-bit RSA asymmetric keypair and the Python `pycryptodome` module to sign the bootloader EEPROM config and boot image.

#### Install Python Crypto Support (the pycryptodomex module)
```bash
sudo apt install python3-pycryptodome
```

#### Create an RSA key-pair using OpenSSL. Must be 2048 bits
Secure-boot requires that the SPI flash configuration and `boot.img` file is signed with the customer's RSA private key.

It is **critical** that this key is stored securely and backed up. It should not be installed in the target OS image.

For reference, this command will generate a private key in the expected format.
```bash
cd $HOME
openssl genrsa 2048 > private.pem
```

### Programming the OTP and signed EEPROM image
* Please see the [secure boot EEPROM guide](../secure-boot-recovery/README.md) to enable via rpiboot `recovery.bin`.
* Please see the [secure boot MSD guide](../mass-storage-gadget64/README.md) for instructions about how to mount the eMMC via USB mass-storage once secure-boot has been enabled.

### Disk encryption
Secure-boot is responsible for loading the Kernel + initramfs and loads all of the data
from a single `boot.img` file stored on an unencrypted FAT/EFI partition.

**There is no support in the ROM or firmware for full-disk encryption.**

Instead, we recommend storing the root filesystem within a LUKS container that is mounted by scripts within initramfs. The LUKS passphrase can be derived from a key stored in OTP.

See [device specific private key](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#device-specific-private-key).

The OTP key is protected against external access by the verified boot flow provided by secure-boot. However, Raspberry Pi computers do not have a secure hardware enclave, and within the secure-boot OS image this key is accessible to any process with access to `/dev/vcio` (`vcmailbox`).

**It is not possible to prevent code running in ARM supervisor mode (e.g. kernel code) from accessing OTP hardware directly.**

See also:
* [LUKS](https://en.wikipedia.org/wiki/Linux_Unified_Key_Setup)
* [cryptsetup FAQ](https://gitlab.com/cryptsetup/cryptsetup/-/wikis/FrequentlyAskedQuestions)
* [rpi-otp-private-key](./tools/rpi-otp-private-key)

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
package supports secure boot. To download the firmware files directly, run:

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
losetup ${LOOP} boot.img
mount ${LOOP} boot-mount/

echo boot.img contains
find boot-mount/

umount boot-mount
losetup -d ${LOOP}
rmdir boot-mount
```

#### Signing the boot image
For secure-boot, `rpi-eeprom-digest` extends the current `.sig` format of
sha256 + timestamp to include a hex-format RSA PKCS#1 v1.5 signature. The key length
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
`rpi-eeprom-digest` supports delegating the RSA signing operation to an external **HSM wrapper** script via the `-H` argument, instead of reading a private key from a `.PEM` file with `-k`.
The wrapper script is responsible for performing an `rsa2048-sha256` PKCS#1 v1.5 signature using a key that is internal to the HSM (or otherwise not stored on disk) and printing the raw signature bytes as a hex string on stdout.
The wrapper is invoked as:

```bash
rpi-eeprom-digest -H hsm-wrapper -i bootconf.txt -o bootconf.sig
```

where `hsm-wrapper` is a program that implements the interface:

```bash
hsm-wrapper -a rsa2048-sha256 INPUT_FILE
```

and writes the signature in hexadecimal format to stdout.
This encapsulates the private key handling inside the HSM wrapper while keeping the `rpi-eeprom-digest` and `rpi-sign-bootcode` command-line interfaces unchanged.

`rpi-eeprom-digest` is called by `update-pieeprom.sh` to sign the EEPROM config file, and the same HSM wrapper mechanism can be used there to keep the private key entirely within the HSM or wrapper.

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
