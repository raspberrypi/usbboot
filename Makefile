rpiboot$(rpiboot_lsb_id): main.c msd/bootcode.h msd/start.h
	$(CC) -Wall -Wextra -g -o $@ $< -lusb-1.0

%.h: %.bin ./bin2c
	./bin2c $< $@

%.h: %.elf ./bin2c
	./bin2c $< $@

bin2c: bin2c.c
	$(CC) -Wall -Wextra -g -o $@ $<

all: docker-centos docker-debian docker-fedora

docker_build = \
	docker build --tag rpiboot-builder:$(1) --file docker/$(1).Dockerfile . &&\
	docker run --rm -e "rpiboot_lsb_id=-$(1)" --volume=`pwd`:/src:Z rpiboot-builder:$(1)

docker-centos:
	$(call docker_build,centos)

docker-debian:
	$(call docker_build,debian)

docker-fedora:
	$(call docker_build,fedora)

uninstall:
	rm -f /usr/bin/rpiboot
	rm -f /usr/share/rpiboot/usbbootcode.bin
	rm -f /usr/share/rpiboot/msd.elf
	rm -f /usr/share/rpiboot/buildroot.elf
	rmdir --ignore-fail-on-non-empty /usr/share/rpiboot/

clean:
	rm -f rpiboot* msd/*.h bin2c

.PHONY: all clean docker-centos docker-debian docker-fedora uninstall
