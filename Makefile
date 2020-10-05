rpiboot: main.c msd/bootcode.h msd/start.h msd/bootcode4.h msd/start4.h
	$(CC) -Wall -Wextra -g -o $@ $< -lusb-1.0

%.h: %.bin ./bin2c
	./bin2c $< $@

%.h: %.elf ./bin2c
	./bin2c $< $@

bin2c: bin2c.c
	$(CC) -Wall -Wextra -g -o $@ $<

uninstall:
	rm -f /usr/bin/rpiboot
	rm -f /usr/share/rpiboot/usbbootcode.bin
	rm -f /usr/share/rpiboot/usbbootcode4.bin
	rm -f /usr/share/rpiboot/msd.elf
	rm -f /usr/share/rpiboot/msd4.elf
	rm -f /usr/share/rpiboot/buildroot.elf
	rmdir --ignore-fail-on-non-empty /usr/share/rpiboot/

clean:
	rm -f rpiboot msd/*.h bin2c

.PHONY: uninstall clean
