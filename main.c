#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "bootfiles.h"
#include "decode_duid.h"
#include "msd/bootcode.h"
#include "msd/start.h"
#include "msd/bootcode4.h"
// 2712 doesn't use start5.elf

/*
 * Old OS X/BSD do not implement fmemopen().  If the version of POSIX
 * supported is old enough that fmemopen() isn't included, assume
 * we're on a BSD compatible system and define a fallback fmemopen()
 * that depends on funopen().
 */
#if _POSIX_VERSION <= 200112L
#include "fmemopen.c"
#endif

#define SELECTION_MODE_VID	0
#define SELECTION_MODE_SERIAL	1

int selection_mode = SELECTION_MODE_VID;
char * target_serialno = NULL;
int signed_boot = 0;
int verbose = 0;
int metadata = 0;
int loop = 0;
int overlay = 0;
long delay = 500;
char * directory = NULL;
char * metadata_path = NULL;
char pathname[18] = {0};
char * targetpathname = NULL;
uint8_t targetPortNo = 99;

int out_ep;
int in_ep;
int bcm2711;
int bcm2712;

#define MAX_PATH_LEN 256
#define FILE_NAME_LENGTH 250
#define DUID_LENGTH 36

unsigned char serial_num[MAX_PATH_LEN];
static char bootfiles_path[MAX_PATH_LEN];
static int use_bootfiles;
static void *bootfile_data;
static FILE * check_file(const char * dir, const char *fname, int use_fmem);
static int second_stage_prep(FILE *fp, FILE *fp_sig);

typedef struct MESSAGE_S {
		int length;
		unsigned char signature[20];
} boot_message_t;

void usage(int error)
{
	FILE * dest = error ? stderr : stdout;

	fprintf(dest, "Usage: rpiboot\n");
	fprintf(dest, "   or: rpiboot -d [directory]\n");
	fprintf(dest, "Boot a Raspberry Pi in device mode either directly into a mass storage device\n");
	fprintf(dest, "or provide a set of boot files in a directory from which to boot.  This can\n");
	fprintf(dest, "then contain a initramfs to boot through to linux kernel\n\n");
	fprintf(dest, "To flash the default bootloader EEPROM image on Compute Module 4 run\n");
	fprintf(dest, "rpiboot -d recovery\n\n");
	fprintf(dest, "For more information about the bootloader EEPROM please see:\n");
	fprintf(dest, "https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#raspberry-pi-bootloader-configuration\n\n");
	fprintf(dest, "rpiboot                  : Boot the device into mass storage device\n");
	fprintf(dest, "rpiboot -d [directory]   : Boot the device using the boot files in 'directory'\n");
	fprintf(dest, "Further options:\n");
	fprintf(dest, "        -l               : Loop forever\n");
	fprintf(dest, "        -o               : Use files from overlay subdirectory if they exist (when using a custom directory)\n");
	fprintf(dest, "                           USB Path (1-1.3.2 for example) is shown in verbose mode.\n");
	fprintf(dest, "                           (bootcode.bin is always preloaded from the base directory)\n");
	fprintf(dest, "        -m delay         : Microseconds delay between checking for new devices (default 500)\n");
	fprintf(dest, "        -v               : Verbose\n");
	fprintf(dest, "        -V               : Displays the version string and exits\n");
	fprintf(dest, "        -s               : Signed using bootsig.bin\n");
	fprintf(dest, "        -0/1/2/3/4/5/6   : Only look for CMs attached to USB port number 0-6\n");
	fprintf(dest, "        -p [pathname]    : Only look for CM with USB pathname\n");
	fprintf(dest, "        -i [serialno]    : Only look for a Raspberry Pi Device with a given serialno\n");
	fprintf(dest, "        -j [path]        : Enable output of metadata JSON files in a given directory for BCM2712/2711\n");
	fprintf(dest, "        -h               : This help\n");

	exit(error ? -1 : 0);
}

libusb_device_handle * LIBUSB_CALL open_device_with_serialno(
	libusb_context *ctx, char *serialno)
{
	struct libusb_device **devices;
	struct libusb_device *cursor;
	struct libusb_device_handle *handle = NULL;
	int r = 0;

	if (libusb_get_device_list(ctx, &devices) < 0)
		return NULL;

	uint32_t device_index = 0;
	unsigned char *serial_buffer = calloc(33, sizeof(uint8_t));
	if (serial_buffer == NULL)
		goto out_serialno;

	while ((cursor = devices[device_index++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(cursor, &desc);
		if (r < 0)
			goto out_serialno;

		r = libusb_open(cursor, &handle);
		if (r < 0)
			goto out_serialno;
		
		if (handle != NULL) {
			if (libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial_buffer, 31) >= 0) {
				if (strncmp(serialno, (char *)serial_buffer, 32)) {
					libusb_close(handle);
					handle = NULL;
					continue;
				}
				
				// Match the magic numbers for Raspberry Pi generations
				if (desc.idVendor == 0x0a5c) {
					if (desc.idProduct == 0x2763 ||
						desc.idProduct == 0x2764 ||
						desc.idProduct == 0x2711 ||
						desc.idProduct == 0x2712)
					{
						FILE *fp_second_stage = NULL;
						FILE *fp_sign = NULL;
						const char *second_stage;

						bcm2711 = (desc.idProduct == 0x2711);
						bcm2712 = (desc.idProduct == 0x2712);
						if (bcm2711)
							second_stage = "bootcode4.bin";
						else if (bcm2712)
							second_stage = "bootcode5.bin";
						else
							second_stage = "bootcode.bin";

						fp_second_stage = check_file(directory, second_stage, 1);
						if (!fp_second_stage) {
							fprintf(stderr, "Failed to open %s\n", second_stage);
							exit(EXIT_FAILURE);
						}

						if (signed_boot && !bcm2711 && !bcm2712) { // Signed boot uses a different mechanism on BCM2711 and BCM2712
							const char *sig_file = "bootcode.sig";
							fp_sign = check_file(directory, sig_file, 1);
							if (!fp_sign)
							{
								fprintf(stderr, "Unable to open '%s'\n", sig_file);
								usage(1);
								exit(EXIT_FAILURE);
							}
						}

						if (second_stage_prep(fp_second_stage, fp_sign) != 0) {
							fprintf(stderr, "Failed to prepare the second stage bootcode\n");
							exit(EXIT_FAILURE);
						}

						if (fp_second_stage)
							fclose(fp_second_stage);

						if (fp_sign)
							fclose(fp_sign);

						break;
					} else {
						// Serial number matches, VID matches, but we don't know about this product. Abort.
						fprintf(stderr, "Unknown Raspberry Pi Product, wanted 2763, 2764, 2711 or 2712. Got: %04x\n", desc.idProduct);
						libusb_close(handle);
						handle = NULL;
						continue;
					}
				} else {
					// Serial number matches, but the VID doesn't. Invalid action.
					fprintf(stderr, "Unknown USB Vendor ID. Wanted 0a5c. Got: %04x\n", desc.idVendor);
					libusb_close(handle);
					handle = NULL;
					continue;
				}
			} else {
				// No serial number specified, not a good sign at all.
				libusb_close(handle);
				handle = NULL;
				continue;
			}
		}
	}

out_serialno:
	if (serial_buffer)
		free(serial_buffer);

	libusb_free_device_list(devices, 1);
	return handle;
}

libusb_device_handle * LIBUSB_CALL open_device_with_vid(
	libusb_context *ctx, uint16_t vendor_id)
{
	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *handle = NULL;
	uint32_t i = 0;
	int r, j, len;
	uint8_t path[8];	// Needed for libusb_get_port_numbers
	uint8_t portNo = 0;

	if (libusb_get_device_list(ctx, &devs) < 0)
		return NULL;

	while ((dev = devs[i++]) != NULL) {
		len = 0;
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0)
			goto out;

		if(overlay || verbose == 2 || targetpathname!=NULL)
		{
			r = libusb_get_port_numbers(dev, path, sizeof(path));
			len = snprintf(&pathname[len], 18-len, "%d", libusb_get_bus_number(dev));
			if (r > 0) {
				len += snprintf(&pathname[len], 18-len, "-");
				len += snprintf(&pathname[len], 18-len, "%d", path[0]);
				for (j = 1; j < r; j++)
				{
					len += snprintf(&pathname[len], 18-len, ".%d", path[j]);
				}
			}
		}

		/*
		  http://libusb.sourceforge.net/api-1.0/group__dev.html#ga14879a0ea7daccdcddb68852d86c00c4

		  The port number returned by this call is usually guaranteed to be uniquely tied to a physical port,
		  meaning that different devices plugged on the same physical port should return the same port number.
		*/
		portNo = libusb_get_port_number(dev);

		if(verbose == 2)
		{
			printf("Found device %u idVendor=0x%04x idProduct=0x%04x\n", i, desc.idVendor, desc.idProduct);
			printf("Bus: %d, Device: %d Path: %s\n",libusb_get_bus_number(dev), libusb_get_device_address(dev), pathname);
		}
		
		if (desc.idVendor == vendor_id) {
			if(desc.idProduct == 0x2763 ||
			   desc.idProduct == 0x2764 ||
			   desc.idProduct == 0x2711 ||
			   desc.idProduct == 0x2712)
			{
				FILE *fp_second_stage = NULL;
				FILE *fp_sign = NULL;
				const char *second_stage;

				if(verbose == 2)
					printf("Found candidate Compute Module...\n");

				// Check if we should match against a specific port number or path
				if ((targetPortNo == 99 || portNo == targetPortNo) &&
					(targetpathname == NULL || strcmp(targetpathname, pathname) == 0))
				{
					if(verbose)
						printf("Device located successfully\n");
					found = dev;
				}
				else
				{
					if(verbose == 2)
						printf("Device port / path does not match, trying again\n");

					continue;
				}

				bcm2711 = (desc.idProduct == 0x2711);
				bcm2712 = (desc.idProduct == 0x2712);
				if (bcm2711)
					second_stage = "bootcode4.bin";
				else if (bcm2712)
					second_stage = "bootcode5.bin";
				else
					second_stage = "bootcode.bin";

				if ((bcm2711 || bcm2712) && !directory) {
					directory = INSTALL_PREFIX "/share/rpiboot/mass-storage-gadget64/";
					use_bootfiles = 1;
					snprintf(bootfiles_path, sizeof(bootfiles_path),"%s%s", directory, "bootfiles.bin");
					printf("Directory not specified - trying default %s\n", directory);

					fp_second_stage = check_file(directory, second_stage, 1);
					if (!fp_second_stage)
					{
						directory = "mass-storage-gadget64/";
						snprintf(bootfiles_path, sizeof(bootfiles_path),"%s%s", directory, "bootfiles.bin");
						printf("Trying local path %s\n", directory);
						fp_second_stage = check_file(directory, second_stage, 1);
					}
				}
				else {
					fp_second_stage = check_file(directory, second_stage, 1);
				}

				if (!fp_second_stage)
				{
					fprintf(stderr, "Failed to open second stage bootloader (%s)\n", second_stage);
					fprintf(stderr, "\nPlease try specifying the directory e.g. rpiboot -d mass-storage-gadget64\n");
					exit(1);
				}

				if (signed_boot && !bcm2711 && !bcm2712) // Signed boot use a different mechanism on BCM2711 and BCM2712
				{
					const char *sig_file = "bootcode.sig";
					fp_sign = check_file(directory, sig_file, 1);
					if (!fp_sign)
					{
						fprintf(stderr, "Unable to open '%s'\n", sig_file);
						usage(1);
					}
				}

				if (second_stage_prep(fp_second_stage, fp_sign) != 0)
				{
					fprintf(stderr, "Failed to prepare the second stage bootcode\n");
					exit(-1);
				}
				if (fp_second_stage)
					fclose(fp_second_stage);

				if (fp_sign)
					fclose(fp_sign);

				if (found)
					break;
			}
		}
	}

	if (found) {
		sleep(1);
		r = libusb_open(found, &handle);
		if (r == LIBUSB_ERROR_ACCESS)
		{
			printf("Permission to access USB device denied. Make sure you are a member of the plugdev group.\n");
			exit(-1);
		}
		else if (r < 0)
		{
			if(verbose) printf("Failed to open the requested device\n");
			handle = NULL;
		}
	}

out:
	libusb_free_device_list(devs, 1);
	return handle;

}

int Initialize_Device(libusb_context ** ctx, libusb_device_handle ** usb_device)
{
	int ret = 0;
	int interface;
	struct libusb_config_descriptor *config;

	switch (selection_mode) {
		case SELECTION_MODE_SERIAL: {
			*usb_device = open_device_with_serialno(*ctx, target_serialno);
			if (*usb_device == NULL)
			{
				usleep(200);
				return -1;
			}
		}
		break;
		case SELECTION_MODE_VID: {
			*usb_device = open_device_with_vid(*ctx, 0x0a5c);
			if (*usb_device == NULL)
			{
				usleep(200);
				return -1;
			}
		}
		break;
	}

	libusb_get_active_config_descriptor(libusb_get_device(*usb_device), &config);
	if(config == NULL)
	{
		printf("Failed to read config descriptor\n");
		exit(-1);
	}

	// Handle 2837 where it can start with two interfaces, the first is mass storage
	// the second is the vendor interface for programming
	if(config->bNumInterfaces == 1)
	{
		interface = 0;
		out_ep = 1;
		in_ep = 2;
	}
	else
	{
		interface = 1;
		out_ep = 3;
		in_ep = 4;
	}

	ret = libusb_claim_interface(*usb_device, interface);
	if (ret)
	{
		libusb_close(*usb_device);
		printf("Failed to claim interface\n");
		return ret;
	}

	if(verbose) printf("Initialised device correctly\n");

	return ret;
}

#define LIBUSB_MAX_TRANSFER (16 * 1024)

int ep_write(void *buf, int len, libusb_device_handle * usb_device)
{
	int a_len = 0;
	int sending, sent;
	int ret =
	    libusb_control_transfer(usb_device, LIBUSB_REQUEST_TYPE_VENDOR, 0,
				    len & 0xffff, len >> 16, NULL, 0, 1000);

	if(ret != 0)
	{
		printf("Failed control transfer (%d,%d)\n", ret, len);
		return ret;
	}

	while(len > 0)
	{
		sending = len < LIBUSB_MAX_TRANSFER ? len : LIBUSB_MAX_TRANSFER;
		ret = libusb_bulk_transfer(usb_device, out_ep, buf, sending, &sent, 5000);
		if (ret)
			break;
		a_len += sent;
		buf += sent;
		len -= sent;
	}
	if(verbose)
		printf("libusb_bulk_transfer sent %d bytes; returned %d\n", a_len, ret);

	return a_len;
}

int ep_read(void *buf, int len, libusb_device_handle * usb_device)
{
	int ret =
	    libusb_control_transfer(usb_device,
				    LIBUSB_REQUEST_TYPE_VENDOR |
				    LIBUSB_ENDPOINT_IN, 0, len & 0xffff,
				    len >> 16, buf, len, 20000);
	if(ret >= 0)
		return len;
	else
		return ret;
}

void print_version(void)
{
	printf("RPIBOOT: build-date %s pkg-version %s %s\n", BUILD_DATE, PKG_VER, GIT_VER);
}

void get_options(int argc, char *argv[])
{
	// Skip the command name
	argv++; argc--;
	while(*argv)
	{
		if(strcmp(*argv, "-d") == 0)
		{
			argv++; argc--;
			if(argc < 1)
				usage(1);
			directory = *argv;
		}
		else if(strcmp(*argv, "-p") == 0)
		{
			argv++; argc--;
			if(argc < 1)
				usage(1);
			targetpathname = *argv;
		}
		else if(strcmp(*argv, "-h") == 0 || strcmp(*argv, "--help") == 0)
		{
			usage(0);
		}
		else if(strcmp(*argv, "-l") == 0)
		{
			loop = 1;
		}
		else if(strcmp(*argv, "-v") == 0)
		{
			verbose = 1;
		}
		else if((strcmp(*argv, "-V") == 0) || (strcmp(*argv, "--version")) == 0)
		{
			print_version();
			exit(0);
		}
		else if(strcmp(*argv, "-o") == 0)
		{
			overlay = 1;
		}
		else if(strcmp(*argv, "-m") == 0)
		{
			argv++; argc--;
			if(argc < 1)
				usage(1);
			delay = atol(*argv);
		}
		else if(strcmp(*argv, "-vv") == 0)
		{
			verbose = 2;
		}
		else if(strcmp(*argv, "-s") == 0)
		{
			signed_boot = 1;
		}
		else if(strcmp(*argv, "-j") == 0)
		{
			argv++; argc--;
			if(argc < 1)
				usage(1);
			metadata_path = *argv;
			metadata = 1;
		}
		else if (strcmp(*argv, "-i") == 0)
		{
			selection_mode = SELECTION_MODE_SERIAL;
			argv++; argc--;
			if (argc < 1) {
				usage(1);
			} else {
				target_serialno = *argv;
			}
		}
		else if(strcmp(*argv, "-0") == 0)
		{
			targetPortNo = 0;
		}
		else if(strcmp(*argv, "-1") == 0)
		{
			targetPortNo = 1;
		}
		else if(strcmp(*argv, "-2") == 0)
		{
			targetPortNo = 2;
		}
		else if(strcmp(*argv, "-3") == 0)
		{
			targetPortNo = 3;
		}
		else if(strcmp(*argv, "-4") == 0)
		{
			targetPortNo = 4;
		}
		else if(strcmp(*argv, "-5") == 0)
		{
			targetPortNo = 5;
		}
		else if(strcmp(*argv, "-6") == 0)
		{
			targetPortNo = 6;
		}
		else
		{
			usage(1);
		}

		argv++; argc--;
	}
	if(overlay&&!directory)
	{
		usage(1);
	}
	if(!delay)
	{
		usage(1);
	}
	if((targetPortNo != 99) && (targetpathname != NULL))
	{
		usage(1);
	}
}

boot_message_t boot_message;
void *second_stage_txbuf;

int second_stage_prep(FILE *fp, FILE *fp_sig)
{
	int size;

	fseek(fp, 0, SEEK_END);
	boot_message.length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(fp_sig != NULL)
	{
		size = fread(boot_message.signature, 1, sizeof(boot_message.signature), fp_sig);
		if (size != sizeof(boot_message.signature))
		{
			fprintf(stderr, "Failed to read bootcode signature \n");
			return -1;
		}
	}

	if (second_stage_txbuf)
		free(second_stage_txbuf);
	second_stage_txbuf = NULL;

	second_stage_txbuf = (uint8_t *) malloc(boot_message.length);
	if (second_stage_txbuf == NULL)
	{
		fprintf(stderr, "Failed to allocate memory\n");
		return -1;
	}

	size = fread(second_stage_txbuf, 1, boot_message.length, fp);
	if(size != boot_message.length)
	{
		fprintf(stderr, "Failed to read second stage\n");
		return -1;
	}

	return 0;
}

int second_stage_boot(libusb_device_handle *usb_device)
{
	int size, retcode = 0;

	size = ep_write(&boot_message, sizeof(boot_message), usb_device);
	if (size != sizeof(boot_message))
	{
		printf("Failed to write correct length, returned %d\n", size);
		return -1;
	}

	if(verbose) printf("Writing %d bytes\n", boot_message.length);
	size = ep_write(second_stage_txbuf, boot_message.length, usb_device);
	if (size != boot_message.length)
	{
		printf("Failed to read correct length, returned %d\n", size);
		return -1;
	}

	sleep(1);
	size = ep_read((unsigned char *)&retcode, sizeof(retcode), usb_device);

	if (size > 0 && retcode == 0)
	{
		printf("Successful read %d bytes \n", size);
	}
	else
	{
		printf("Failed : 0x%x\n", retcode);
	}

	return retcode;

}


FILE * check_file(const char * dir, const char *fname, int use_fmem)
{
	FILE * fp = NULL;
	char path[MAX_PATH_LEN];

	// Prevent USB device from requesting files in parent directories
	if(strstr(fname, ".."))
	{
		printf("Denying request for filename containing .. to prevent path traversal\n");
		return NULL;
	}

	if (use_bootfiles && use_fmem)
	{
		const char *prefix = bcm2712 ? "2712" : bcm2711 ? "2711" : "2710";
		unsigned long length = 0;

		// If 'dir' is specified and the file exists then load this in preference
		// to the file in bootfiles.bin e.g. use a custom config.txt or cmdline.txt
		// to override settings in the mass-storage-gadget
		if (dir)
		{
			snprintf(path, sizeof(path), "%s/%s/%s", dir, prefix, fname);
			path[sizeof(path) - 1] = 0;
			fp = fopen(path, "rb");

			if (fp)
			{
				printf("Loading bootfiles.bin overlay: %s\n", path);
				return fp;
			}
		}

		snprintf(path, sizeof(path), "%s/%s", prefix, fname);
		path[sizeof(path) - 1] = 0;
		if (bootfile_data)
			free(bootfile_data);
		bootfile_data = bootfiles_read(bootfiles_path, path, &length);
		if (bootfile_data)
			fp = fmemopen(bootfile_data, length, "rb");
		if (fp)
			return fp;
	}

	if(dir)
	{
		if(overlay && (pathname[0] != 0) &&
				(strcmp(fname, "bootcode5.bin") != 0) &&
				(strcmp(fname, "bootcode4.bin") != 0) &&
				(strcmp(fname, "bootcode.bin") != 0))
		{
			snprintf(path, sizeof(path), "%s/%s/%s", dir, pathname, fname);
			path[sizeof(path) - 1] = 0;
			fp = fopen(path, "rb");
			if (fp)
				printf("Loading: %s\n", path);
			memset(path, 0, sizeof(path));
		}

		if (fp == NULL)
		{
			snprintf(path, sizeof(path), "%s/%s", dir, fname);
			path[sizeof(path) - 1] = 0;
			fp = fopen(path, "rb");
			if (fp)
				printf("Loading: %s\n", path);
		}
	}

	// Failover to fmem unless use_fmem is zero in which case this function
	// is being used to check if a file exists.
	if(fp == NULL && use_fmem)
	{
		if (bcm2711)
		{
			if(strcmp(fname, "bootcode4.bin") == 0)
				fp = fmemopen(msd_bootcode4_bin, msd_bootcode4_bin_len, "rb");
			else if(strcmp(fname, "start4.elf") == 0)
				fp = fmemopen(msd_start_elf, msd_start_elf_len, "rb");
		}
		else
		{
			if(strcmp(fname, "bootcode.bin") == 0)
				fp = fmemopen(msd_bootcode_bin, msd_bootcode_bin_len, "rb");
			else if(strcmp(fname, "start.elf") == 0)
				fp = fmemopen(msd_start_elf, msd_start_elf_len, "rb");
		}
		if (fp)
			printf("Loading embedded: %s\n", fname);
	}

	return fp;
}

void close_metadata_file(FILE ** fp){
	fprintf(*fp, "\n}");
	fclose(*fp);
}

void write_metadata_file(char *metadata_str, FILE **fp, int index)
{
	char *token, *property, *value;

	token = strtok(metadata_str, "*");
	if(!token) return;
	property = strdup(token);
	token = strtok(NULL, "*");

	if(token)
	{
		value = strdup(token);
		if (index != 0)
			fprintf(*fp, ",");

		if (strcmp(property, "FACTORY_UUID") == 0)
		{
			char c40_str[DUID_LENGTH];
			if (duid_decode_c40(value, c40_str) == -1)
				fprintf(stderr, "Failed to decode a FACTORY_UUID: invalid input\n");
			else
				fprintf(*fp, "\n\t\"%s\" : \"%s\"", property, c40_str);
		}
		else
		{
			fprintf(*fp, "\n\t\"%s\" : \"%s\"", property, value);
		}
		free(value);
	}
	free(property);
}

void create_metadata_file(FILE ** fp)
{
	char fname[MAX_PATH_LEN + FILE_NAME_LENGTH + 5]; // 5 for extension .json
	snprintf(fname, sizeof(fname), "%s/%s.json", metadata_path, (char *)serial_num);

	*fp = fopen(fname, "w");
	if (*fp)
	{
		printf("Created metadata file: %s\n", fname);
		fprintf(*fp, "{");
	}
	else
	{
		fprintf(stderr, "Failed to create metadata file: %s\n", fname);
		metadata = 0;
	}
}

int file_server(libusb_device_handle * usb_device)
{
	int going = 1;
	struct file_message {
		int command;
		char fname[MAX_PATH_LEN];
	} message;
	static FILE * fp = NULL;
	FILE * metadata_fp = NULL;
	char metadata_fname[FILE_NAME_LENGTH];
	int metadata_index = 0;

	if (metadata)
	{
		if (bcm2711 || bcm2712)
		{
			create_metadata_file(&metadata_fp);
		}
		else
		{
			fprintf(stderr, "Failed to create metadata file: expected BCM2712/2711");
			metadata = 0;
		}
	}

	while(going)
	{
		char message_name[][20] = {"GetFileSize", "ReadFile", "Done"};
		int i = ep_read(&message, sizeof(message), usb_device);
		if(i < 0)
		{
			// Drop out if the device goes away
			if(i == LIBUSB_ERROR_NO_DEVICE || i == LIBUSB_ERROR_IO)
				break;
			sleep(1);
			continue;
		}
		if(verbose) printf("Received message %s: %s\n", message_name[message.command], message.fname);

		// Done can also just be null filename
		if(strlen(message.fname) == 0)
		{
			ep_write(NULL, 0, usb_device);
			break;
		}

		// Metadata files
		if ((message.fname[0] == '*') && (message.command != 2))
		{
			if (metadata)
			{
				strcpy(metadata_fname, message.fname);
				write_metadata_file(metadata_fname + 1, &metadata_fp, metadata_index++);
			}
			ep_write(NULL, 0, usb_device);
			continue;
		}

		switch(message.command)
		{
			case 0: // Get file size
				if(fp)
					fclose(fp);
				fp = check_file(directory, message.fname, 1);
				if(strlen(message.fname) && fp != NULL)
				{
					int file_size;

					fseek(fp, 0, SEEK_END);
					file_size = ftell(fp);
					fseek(fp, 0, SEEK_SET);

					if(verbose || !file_size)
						printf("File size = %d bytes\n", file_size);

					int sz = libusb_control_transfer(usb_device, LIBUSB_REQUEST_TYPE_VENDOR, 0,
					    file_size & 0xffff, file_size >> 16, NULL, 0, 1000);

					if(sz < 0)
						return -1;
				}
				else
				{
					ep_write(NULL, 0, usb_device);
					printf("Cannot open file %s\n", message.fname);
					break;
				}
				break;

			case 1: // Read file
				if(fp != NULL)
				{
					int file_size;
					void *buf;

					printf("File read: %s\n", message.fname);

					fseek(fp, 0, SEEK_END);
					file_size = ftell(fp);
					fseek(fp, 0, SEEK_SET);

					if (!file_size)
						printf("WARNING: %s is empty\n", message.fname);

					buf = malloc(file_size);
					if(buf == NULL)
					{
						printf("Failed to allocate buffer for file %s\n", message.fname);
						return -1;
					}
					int read = fread(buf, 1, file_size, fp);
					if(read != file_size)
					{
						printf("Failed to read from input file\n");
						free(buf);
						return -1;
					}

					int sz = ep_write(buf, file_size, usb_device);

					free(buf);
					fclose(fp);
					fp = NULL;

					if(sz != file_size)
					{
						printf("Failed to write complete file to USB device\n");
						return -1;
					}
				}
				else
				{
					if(verbose) printf("No file %s found\n", message.fname);
					ep_write(NULL, 0, usb_device);
				}
				break;

			case 2: // Done, exit file server
				if(verbose) printf("CMD exit\n");
				going = 0;
				break;

			default:
				printf("Unknown message\n");
				return -1;
		}
	}

	if (metadata)
		close_metadata_file(&metadata_fp);

	printf("Second stage boot server done\n");
	return 0;
}

int main(int argc, char *argv[])
{
	libusb_context *ctx;
	libusb_device_handle *usb_device;
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;

	get_options(argc, argv);
	print_version();
	printf("\nPlease fit the EMMC_DISABLE / nRPIBOOT jumper before connecting the power and USB cables to the target device.\n");
	printf("If the device fails to connect then please see https://rpltd.co/rpiboot for debugging tips.\n\n");

	// flush immediately
	setbuf(stdout, NULL);

	// If the boot directory is specified then check that it contains bootcode files.
	if (directory)
	{
		FILE *f, *f4, *f5;

		if (verbose)
			printf("Boot directory '%s'\n", directory);

		f = check_file(directory, "bootfiles.bin", 0);
		if (f)
		{
			snprintf(bootfiles_path, sizeof(bootfiles_path),"%s/%s", directory, "bootfiles.bin");
			printf("Using %s\n", bootfiles_path);
			bootfiles_path[sizeof(bootfiles_path) - 1] = 0;
			use_bootfiles = 1;
			fclose(f);
			f = NULL;
		}
		else
		{
			f = check_file(directory, "bootcode.bin", 0);
			f4 = check_file(directory, "bootcode4.bin", 0);
			f5 = check_file(directory, "bootcode5.bin", 0);
			if (!f && !f4 && !f5)
			{
				fprintf(stderr, "No 'bootcode' files found in '%s'\n", directory);
				usage(1);
			}
			if (f)
				fclose(f);
			if (f4)
				fclose(f4);
			if (f5)
				fclose(f5);
		}

		if (signed_boot)
		{
			f = check_file(directory, "bootsig.bin", 0);
			if (!f)
			{
				fprintf(stderr, "Unable to open 'bootsig.bin' from %s\n", directory);
				usage(1);
			}
			fclose(f);
		}
	}

	int ret = libusb_init(&ctx);
	if (ret)
	{
		printf("Failed to initialise libUSB\n");
		exit(-1);
	}

#if LIBUSBX_API_VERSION < 0x01000106
	libusb_set_debug(ctx, (verbose == 2)? LIBUSB_LOG_LEVEL_WARNING : 0);
#else
	libusb_set_option(
		ctx,
		LIBUSB_OPTION_LOG_LEVEL,
		verbose ? verbose == 2 ? LIBUSB_LOG_LEVEL_INFO : LIBUSB_LOG_LEVEL_WARNING : 0
		);
#endif

	do
	{
		int last_serial = -1;

		printf("Waiting for BCM2835/6/7/2711/2712...\n\n");

		// Wait for a device to get plugged in
		do
		{
			ret = Initialize_Device(&ctx, &usb_device);
			if(ret == 0)
			{
				libusb_get_device_descriptor(libusb_get_device(usb_device), &desc);

				if(verbose)
					printf("Found serial number %d\n", desc.iSerialNumber);

				// Make sure we've re-enumerated since the last time
				if(desc.iSerialNumber == last_serial)
				{
					ret = -1;
					libusb_close(usb_device);
				}

				libusb_get_active_config_descriptor(libusb_get_device(usb_device), &config);
			}

			if (ret)
			{
				usleep(delay);
			}
		}
		while (ret);

		ret = libusb_get_string_descriptor_ascii(usb_device, desc.iSerialNumber, serial_num, sizeof(serial_num));
		// if metadata output is enabled and could not get serial number
		if (metadata && (ret <= 0)) {
			metadata = 0; // disable metadata
		}

		if (verbose) printf("last_serial %d serial %d\n", last_serial, desc.iSerialNumber);
		last_serial = desc.iSerialNumber;
		if(desc.iSerialNumber == 0 || desc.iSerialNumber == 3)
		{
			printf("Sending bootcode.bin\n");
			second_stage_boot(usb_device);
		}
		else
		{
			printf("Second stage boot server\n");
			file_server(usb_device);
		}

		libusb_close(usb_device);
		sleep(1);

	}
	while(loop || desc.iSerialNumber == 0 || desc.iSerialNumber == 3);

	libusb_exit(ctx);

	return 0;
}

