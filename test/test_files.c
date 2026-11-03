// Host tests for bootfiles.bin extraction, file lookup and DUID decoding.
// main.c is included directly so that the static file lookup state is
// visible to the tests. No USB device is required.
//
// Usage: test_files <scratch-dir> <top-dir>

#define main rpiboot_main
#include "../main.c"
#undef main

#include <errno.h>
#include <sys/stat.h>

#define TAR_BLOCK 512

static int failures;
static char scratch[256];

#define CHECK(cond) do { \
	if (!(cond)) { \
		fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
		failures++; \
	} \
} while (0)

static void tar_add(FILE *fp, const char *name, const char *size_field,
		const void *data, size_t len)
{
	unsigned char hdr[TAR_BLOCK] = {0};
	static const unsigned char pad[TAR_BLOCK];

	memcpy(hdr, name, strnlen(name, 100));
	snprintf((char *) hdr + 124, 12, "%s", size_field);
	fwrite(hdr, 1, sizeof(hdr), fp);
	fwrite(data, 1, len, fp);
	fwrite(pad, 1, (TAR_BLOCK - (len % TAR_BLOCK)) % TAR_BLOCK, fp);
}

static void tar_add_file(FILE *fp, const char *name, const char *data)
{
	char size[24];

	snprintf(size, sizeof(size), "%011lo", (unsigned long) strlen(data));
	tar_add(fp, name, size, data, strlen(data));
}

static void tar_end(FILE *fp)
{
	static const unsigned char zero[2 * TAR_BLOCK];

	fwrite(zero, 1, sizeof(zero), fp);
	fclose(fp);
}

static FILE *create(const char *fmt, const char *name)
{
	char path[PATH_MAX];
	FILE *fp;

	snprintf(path, sizeof(path), fmt, scratch, name);
	fp = fopen(path, "wb");
	if (!fp) {
		perror(path);
		exit(1);
	}
	return fp;
}

static void write_file(const char *name, const char *data)
{
	FILE *fp = create("%s/%s", name);

	fputs(data, fp);
	fclose(fp);
}

static void make_dir(const char *name)
{
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s/%s", scratch, name);
	if (mkdir(path, 0755) && errno != EEXIST) {
		perror(path);
		exit(1);
	}
}

// Read the whole of fp into buf as a string and close it
static const char *slurp(FILE *fp)
{
	static char buf[256];
	size_t n;

	if (!fp)
		return NULL;
	n = fread(buf, 1, sizeof(buf) - 1, fp);
	buf[n] = 0;
	fclose(fp);
	return buf;
}

static int same(const char *a, const char *b)
{
	return a && b && !strcmp(a, b);
}

static int archive_has(const char *archive, const char *name, const char *data)
{
	unsigned long size = 0;
	unsigned char *buf = bootfiles_read(archive, name, &size);
	int ok = buf && (!data || (size == strlen(data) && !memcmp(buf, data, size)));

	free(buf);
	return ok;
}

static void test_bootfiles_read(void)
{
	char archive[PATH_MAX];
	char name[101];
	FILE *fp;

	snprintf(archive, sizeof(archive), "%s/good.tar", scratch);
	fp = create("%s/%s", "good.tar");
	tar_add_file(fp, "2712/", "");
	tar_add_file(fp, "2712/config.txt", "cfg");
	tar_add_file(fp, "2712/Bootcode5.bin", "boot5");
	tar_end(fp);

	CHECK(archive_has(archive, "2712/config.txt", "cfg"));
	CHECK(archive_has(archive, "2712/bootcode5.bin", "boot5"));
	CHECK(!archive_has(archive, "2711/config.txt", NULL));
	CHECK(!archive_has(archive, "config.txt", NULL));

	snprintf(archive, sizeof(archive), "%s/missing.tar", scratch);
	CHECK(!archive_has(archive, "2712/config.txt", NULL));

	// Size field claims more data than the archive holds
	snprintf(archive, sizeof(archive), "%s/oversize.tar", scratch);
	fp = create("%s/%s", "oversize.tar");
	tar_add(fp, "2712/config.txt", "00000100000", "cfg", 3);
	tar_end(fp);
	CHECK(!archive_has(archive, "2712/config.txt", NULL));

	// Archive truncated part way through a header
	snprintf(archive, sizeof(archive), "%s/short.tar", scratch);
	fp = create("%s/%s", "short.tar");
	tar_add_file(fp, "2712/config.txt", "cfg");
	fwrite("2712/bootcode5.bin", 1, 18, fp);
	fclose(fp);
	CHECK(!archive_has(archive, "2712/bootcode5.bin", NULL));

	// A 100 character name has no terminator in the header
	memset(name, 'a', 100);
	name[100] = 0;
	snprintf(archive, sizeof(archive), "%s/longname.tar", scratch);
	fp = create("%s/%s", "longname.tar");
	tar_add_file(fp, name, "long");
	tar_end(fp);
	CHECK(!archive_has(archive, name, NULL));
	name[99] = 0;
	CHECK(archive_has(archive, name, "long"));
}

static void test_check_file(void)
{
	char dir[512];
	FILE *fp;

	snprintf(dir, sizeof(dir), "%s/msg", scratch);
	make_dir("msg");
	make_dir("msg/2711");
	make_dir("msg/2712");
	make_dir("msg/1-2");
	fp = create("%s/msg/%s", "bootfiles.bin");
	tar_add_file(fp, "2712/bootcode5.bin", "tar-boot5");
	tar_add_file(fp, "2712/config.txt", "tar-config");
	tar_end(fp);
	write_file("msg/2712/config.txt", "dir-config");
	write_file("msg/2711/start4.elf", "dir-2711-start4");
	write_file("msg/start4.elf", "dir-start4");
	write_file("msg/boot.img", "dir-boot");
	write_file("msg/1-2/start4.elf", "port-start4");
	write_file("msg/1-2/bootcode4.bin", "port-boot4");
	write_file("msg/2711/bootcode4.bin", "dir-2711-boot4");

	// BCM2712 with bootfiles.bin
	bcm2711 = 0;
	bcm2712 = 1;
	use_bootfiles = 1;
	snprintf(bootfiles_path, sizeof(bootfiles_path), "%s/bootfiles.bin", dir);
	CHECK(same(slurp(check_file(dir, "bootcode5.bin", 1)), "tar-boot5"));
	CHECK(same(slurp(check_file(dir, "config.txt", 1)), "dir-config"));
	CHECK(same(slurp(check_file(dir, "boot.img", 1)), "dir-boot"));
	CHECK(!check_file(dir, "../msg/boot.img", 1));
	CHECK(!check_file(dir, "missing", 1));
	CHECK(!check_file(NULL, "missing", 1));

	// BCM2711 from a directory, chip subdirectory first
	bcm2711 = 1;
	bcm2712 = 0;
	use_bootfiles = 0;
	CHECK(same(slurp(check_file(dir, "start4.elf", 1)), "dir-2711-start4"));
	CHECK(same(slurp(check_file(dir, "boot.img", 1)), "dir-boot"));

	// Port overlay takes priority except for the second stage
	overlay = 1;
	strcpy(pathname, "1-2");
	CHECK(same(slurp(check_file(dir, "start4.elf", 1)), "port-start4"));
	CHECK(same(slurp(check_file(dir, "bootcode4.bin", 1)), "dir-2711-boot4"));
	overlay = 0;
	pathname[0] = 0;

	// BCM2835/6/7 fall back to the embedded msd files
	bcm2711 = 0;
	fp = check_file(NULL, "bootcode.bin", 1);
	CHECK(fp && fgetc(fp) == msd_bootcode_bin[0]);
	if (fp) {
		fseek(fp, 0, SEEK_END);
		CHECK(ftell(fp) == (long) msd_bootcode_bin_len);
		fclose(fp);
	}
	fp = check_file(NULL, "start.elf", 1);
	CHECK(fp != NULL);
	if (fp)
		fclose(fp);
	CHECK(!check_file(NULL, "bootcode.bin", 0));
	CHECK(!check_file(NULL, "config.txt", 1));
}

// Files that the second stage on each chip loads from bootfiles.bin
static void test_shipped_bootfiles(const char *top)
{
	static const char *archives[] = {
		"firmware", "mass-storage-gadget64", "rpi-imager-embedded",
		"secure-boot-example",
	};
	static const char *files[] = {
		"bootmain", "mcb.bin", "memsys00.bin", "memsys01.bin",
		"memsys02.bin", "memsys03.bin",
	};
	static const struct { const char *chip, *second_stage; } chips[] = {
		{ "2711", "bootcode4.bin" },
		{ "2712", "bootcode5.bin" },
	};
	char archive[PATH_MAX], name[64];
	unsigned long size;
	unsigned char *buf;
	unsigned a, c, f;

	for (a = 0; a < sizeof(archives) / sizeof(archives[0]); a++) {
		snprintf(archive, sizeof(archive), "%s/%s/bootfiles.bin", top, archives[a]);
		for (c = 0; c < sizeof(chips) / sizeof(chips[0]); c++) {
			for (f = 0; f <= sizeof(files) / sizeof(files[0]); f++) {
				snprintf(name, sizeof(name), "%s/%s", chips[c].chip,
						f ? files[f - 1] : chips[c].second_stage);
				size = 0;
				buf = bootfiles_read(archive, name, &size);
				if (!buf || !size)
					fprintf(stderr, "FAIL %s: %s missing\n", archive, name);
				failures += !buf || !size;
				free(buf);
			}
		}
	}
}

static void test_duid(void)
{
	char out[DUID_LENGTH + 1];
	char in[128];

	// "ABC" = (14 * 1600) + (15 * 40) + 16 + 1
	strcpy(in, "59e9");
	CHECK(duid_decode_c40(in, out) == 0 && same(out, "ABC"));
	strcpy(in, "59e9_59e959e9");
	CHECK(duid_decode_c40(in, out) == 0 && same(out, "ABCABCABC"));
	strcpy(in, "59e959e9_59e959e9_59e959e9_59e959e9_59e959e9_59e959e9");
	CHECK(duid_decode_c40(in, out) == 0 && strlen(out) == DUID_LENGTH);
	strcpy(in, "59e959e9_59e959e9_59e959e9_59e959e9_59e959e9_59e959e9_59e9");
	CHECK(duid_decode_c40(in, out) == -1);
	strcpy(in, "1");
	CHECK(duid_decode_c40(in, out) == -1);
	strcpy(in, "");
	CHECK(duid_decode_c40(in, out) == 0 && same(out, ""));
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <scratch-dir> <top-dir>\n", argv[0]);
		return 1;
	}
	snprintf(scratch, sizeof(scratch), "%s", argv[1]);

	test_bootfiles_read();
	test_check_file();
	test_shipped_bootfiles(argv[2]);
	test_duid();

	free(bootfile_data);
	printf("%s: %d failures\n", argv[0], failures);
	return failures != 0;
}
