#ifndef BOOTFILE_H
#define BOOTFILE_H
unsigned char *bootfiles_read(const char *archive, const char *filename, unsigned long *psize);
#endif
