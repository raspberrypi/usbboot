#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Reads bootloader files (e.g. DDR init) from a single packaged file
// to ensure that the DDR init code, firmware and next stage are in sync.
// For simplicity the implementation uses .tar and other files e.g. config.txt
// maybe added to the package.

extern int verbose;
#define BLOCK_SIZE 512

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
} __attribute__((packed));

unsigned char *bootfiles_read(const char *archive, const char *filename, unsigned long *psize)
{
   FILE *fp = NULL;
   struct tar_header hdr;
   unsigned char *data = NULL;
   long archive_size;

   fp = fopen(archive, "rb");
   if (!fp){
      goto fail;
   }
   if (fseek(fp, 0, SEEK_END) < 0)
      goto fail;
   archive_size = ftell(fp);

   if (fseek(fp, 0, SEEK_SET) < 0)
      goto fail;
   do
   {
      unsigned long size;
      long offset;

      offset = ftell(fp);
      if (fread(&hdr, sizeof(hdr), 1, fp) != 1)
         goto fail;

      if (fseek(fp, BLOCK_SIZE - sizeof(hdr), SEEK_CUR) < 0)
         goto fail;
      offset = ftell(fp);

      if (offset == archive_size)
          break;

      size = strtoul(hdr.size, NULL, 8);
      if (offset + size > (unsigned long) archive_size)
      {
         fprintf(stderr, "Corrupted archive");
         goto fail;
      }
      hdr.filename[sizeof(hdr.filename) - 1] = 0;
      if (verbose > 1)
          printf("%s position %08lx size %lu\n", hdr.filename, ftell(fp), size);

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
         if (fseek(fp, (size + BLOCK_SIZE -1) & ~(BLOCK_SIZE -1), SEEK_CUR) < 0)
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
      printf("Completed file-read %s in archive %s length %lu\n", filename, archive, *psize);
   return data;
}
