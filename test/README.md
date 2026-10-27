# Raspberry Pi usbboot and code signing tests

## validate-hsm-wrapper
Verifies that the HSM wrapper (example) script and the direct private key API produce identical results.

## test_files
Host tests for `bootfiles.bin` extraction, the file lookup order used by the
file server, the shipped `bootfiles.bin` archives and FACTORY_UUID decoding.
No USB device is required.
```
make check
make check CC=clang CFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all"
```
