PKG_VER=$(shell grep rpiboot debian/changelog | head -n1 | sed 's/.*(\(.*\)).*/\1/g')
GIT_VER=$(shell git rev-parse HEAD 2>/dev/null | cut -c1-8 || echo "")
INSTALL_PREFIX?=/usr

rpiboot: main.c bootfiles.c msd/bootcode.h msd/start.h msd/bootcode4.h msd/start4.h
	$(CC) -Wall -Wextra -g -o $@ main.c bootfiles.c `pkg-config --cflags --libs libusb-1.0` -DGIT_VER="\"$(GIT_VER)\"" -DPKG_VER="\"$(PKG_VER)\"" -DINSTALL_PREFIX=\"$(INSTALL_PREFIX)\"

%.h: %.bin ./bin2c
	./bin2c $< $@

%.h: %.elf ./bin2c
	./bin2c $< $@

bin2c: bin2c.c
	$(CC) -Wall -Wextra -g -o $@ $<

install: rpiboot
	install -m 755 rpiboot $(INSTALL_PREFIX)/bin/
	install -d $(INSTALL_PREFIX)/share/rpiboot
	install -d $(INSTALL_PREFIX)/share/rpiboot/msd
	install -d $(INSTALL_PREFIX)/share/rpiboot/mass-storage-gadget64
	install -m 644 msd/bootcode.bin  $(INSTALL_PREFIX)/share/rpiboot/msd
	install -m 644 msd/bootcode4.bin $(INSTALL_PREFIX)/share/rpiboot/msd
	install -m 644 msd/start.elf  $(INSTALL_PREFIX)/share/rpiboot/msd
	install -m 644 msd/start4.elf $(INSTALL_PREFIX)/share/rpiboot/msd
	install -m 644 mass-storage-gadget64/boot.img $(INSTALL_PREFIX)/share/rpiboot/mass-storage-gadget64
	install -m 644 mass-storage-gadget64/config.txt $(INSTALL_PREFIX)/share/rpiboot/mass-storage-gadget64
	install -m 644 mass-storage-gadget64/bootfiles.bin $(INSTALL_PREFIX)/share/rpiboot/mass-storage-gadget64

uninstall:
	rm -f $(INSTALL_PREFIX)/bin/rpiboot
	rm -rf $(INSTALL_PREFIX)/share/rpiboot

clean:
	rm -f rpiboot msd/*.h bin2c

.PHONY: uninstall clean
