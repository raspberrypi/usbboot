#ifndef DECODE_DUID_H
#define DECODE_DUID_H
#define DUID_LENGTH 36
// c40_str must hold DUID_LENGTH + 1 bytes
int duid_decode_c40(char * str_of_words, char *c40_str);
#endif