rpiboot: main.c msd/bootcode.h msd/start.h
	$(CC) -Wall -Wextra -g -o $@ $< -lusb-1.0

%.h: %.bin ./bin2c
	./bin2c $< $@

%.h: %.elf ./bin2c
	./bin2c $< $@

bin2c: bin2c.c
	$(CC) -Wall -Wextra -g -o $@ $<

docker-centos:
	docker build --tag rpiboot-builder:centos --file Dockerfile.centos .
	docker run --rm --volume=`pwd`:/src:Z rpiboot-builder:centos

docker-debian:
	docker build --tag rpiboot-builder:debian --file Dockerfile.debian .
	docker run --rm --volume=`pwd`:/src:Z rpiboot-builder:debian

docker-fedora:
	docker build --tag rpiboot-builder:fedora --file Dockerfile.fedora .
	docker run --rm --volume=`pwd`:/src:Z rpiboot-builder:fedora

uninstall:
	rm -f /usr/bin/rpiboot
	rm -f /usr/share/rpiboot/usbbootcode.bin
	rm -f /usr/share/rpiboot/msd.elf
	rm -f /usr/share/rpiboot/buildroot.elf
	rmdir --ignore-fail-on-non-empty /usr/share/rpiboot/

clean:
	rm -f rpiboot msd/*.h bin2c

.PHONY: uninstall clean
