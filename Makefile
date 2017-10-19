CFLAGS	= -Wall -Wextra -g `pkg-config --cflags libusb-1.0`
LDFLAGS	= `pkg-config --libs libusb-1.0`

rpiboot: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

uninstall:
	rm -f /usr/bin/rpiboot
	rm -f /usr/share/rpiboot/usbbootcode.bin
	rm -f /usr/share/rpiboot/msd.elf
	rm -f /usr/share/rpiboot/buildroot.elf
	rmdir --ignore-fail-on-non-empty /usr/share/rpiboot/

clean: 
	rm -f rpiboot

.PHONY: uninstall clean
