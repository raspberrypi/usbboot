#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Reads bootloader files (e.g. DDR init) from a single packaged file
// to ensure that the DDR init code, firmware and next stage are in sync.
// For simplicty the implemenation uses .tar and other files e.g. config.txt
// maybe added to the package.

extern int verbose;

struct tar_header{
   char filename[100];
   char mode[8];
   char uid[8];
   char gid[8];
   char size[12];
   char mtime[12];
   char csum[8];
   char link[1];
   char lname[100];
};

unsigned char *bootfiles_read(const char *archive, const char *filename, unsigned long *psize)
{
   FILE *fp = NULL;
   struct tar_header hdr;
   unsigned char *data = NULL;
   unsigned archive_size;

   fp = fopen(archive, "rb");
   if (fseek(fp, 0, SEEK_END) < 0)
      goto fail;
   archive_size = ftell(fp);

   if (fseek(fp, 0, SEEK_SET) < 0)
      goto fail;
   do
   {
      unsigned long size;
      unsigned long offset;

      if (fread(&hdr, sizeof(hdr), 1, fp) != 1)
         goto fail;

      if (fseek(fp, 512 - sizeof(hdr), SEEK_CUR) < 0)
         goto fail;
      offset = ftell(fp);

      size = strtoul(hdr.size, NULL, 8);
      if (offset + size > archive_size)
      {
         fprintf(stderr, "Corrupted archive");
         goto fail;
      }
      hdr.filename[sizeof(hdr.filename) - 1] = 0;
      if (verbose > 1)
          printf("%s position %lu size %lu\n", hdr.filename, ftell(fp), size);

      if (strcasecmp(hdr.filename, filename) == 0)
      {
         data = malloc(size);
         if (fread(data, 1, size, fp) != size)
            goto fail;
         *psize = size;
         goto end;
      }
      else
      {
         if (fseek(fp, (size + 511) &~ 511, SEEK_CUR) < 0)
            goto fail;
      }
   } while (!feof(fp));

   if (verbose > 1)
      printf("File %s not found in %s\n", filename, archive);
    goto end;

fail:
   if (data)
      free(data);
   printf("read_file: Failed to read \"%s\" from \"%s\" - \%s\n", filename, archive, strerror(errno));
end:
   if (fp)
      fclose(fp);
   if (verbose && data)
      printf("Read file %s in archive %s length %lu\n", archive, filename, *psize);
   return data;
}
