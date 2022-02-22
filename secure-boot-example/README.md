This directory contains an example secure boot image signed with the example private key in this directory.

To sign the image with a different key run
```bash
../tools/rpi-eeprom-digest -i boot.img -o boot.sig -k "${KEY_FILE}"
```

Clearly, product releases should never be signed with `example-private.pem`.
