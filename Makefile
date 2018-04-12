rpiboot: main.c msd/bootcode.h msd/start.h
	$(CC) -Wall -Wextra -g -o $@ $< -lusb-1.0

%.h: %.bin
	xxd -i $< > $@

%.h: %.elf
	xxd -i $< > $@

uninstall:
	rm -f /usr/bin/rpiboot
	rm -f /usr/share/rpiboot/usbbootcode.bin
	rm -f /usr/share/rpiboot/msd.elf
	rm -f /usr/share/rpiboot/buildroot.elf
	rmdir --ignore-fail-on-non-empty /usr/share/rpiboot/

clean: 
	rm -f rpiboot

.PHONY: uninstall clean
