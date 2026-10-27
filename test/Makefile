TOP = ..
SCRATCH = test_files.tmp

check: test_files
	rm -rf $(SCRATCH) && mkdir $(SCRATCH)
	./test_files $(SCRATCH) $(TOP)
	rm -rf $(SCRATCH)

test_files: test_files.c $(TOP)/main.c $(TOP)/bootfiles.c $(TOP)/decode_duid.c $(TOP)/msd/bootcode.h $(TOP)/msd/start.h
	$(CC) -Wall -Wextra -g $(CPPFLAGS) $(CFLAGS) -o $@ test_files.c $(TOP)/bootfiles.c $(TOP)/decode_duid.c `pkg-config --cflags --libs libusb-1.0` -DGIT_VER=\"test\" -DPKG_VER=\"test\" -DBUILD_DATE=\"test\" -DDEFAULT_MSG_DIR=\"/nonexistent/\" $(LDFLAGS)

$(TOP)/msd/%.h:
	$(MAKE) -C $(TOP) msd/$*.h

clean:
	rm -rf test_files $(SCRATCH)

.PHONY: check clean
