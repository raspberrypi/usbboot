#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DUID_LENGTH 36

// Return the c40 value for a character
int char_to_c40(char val)
{
	if ((val >= 'a') && (val <= 'z'))
		val = val - 32;
	if ((val >= '0') && (val <= '9'))
		return 4 + val - '0';
	else if ((val >= 'A') && (val <= 'Z'))
		return 14 + val - 'A';
	return -1;
}

// Assign the character for a c40 value
char c40_to_char(int val)
{
	if ((val >= char_to_c40('0')) && (val <= char_to_c40('9')))
		return '0' + val - char_to_c40('0');
	else if ((val >= char_to_c40('A')) && (val <= char_to_c40('Z')))
		return 'A' + val - char_to_c40('A');
	return '\0';
}

// 	Add to a list of c40 values a half word encoding
void decode_half_word(uint16_t half_word, int *c40_list, int *index)
{
	c40_list[*index] = (int)((half_word - 1) / 1600);
    half_word -= c40_list[(*index)++] * 1600;
    c40_list[*index] = (int)((half_word - 1) / 40);
    half_word -= c40_list[(*index)++] * 40;
    c40_list[(*index)++] = half_word - 1;
}

// Decode a duid from a list of words
int duid_decode_c40(char * str_of_words, char *c40_str)
{
	int c40_list[DUID_LENGTH], i = 0, c;
	uint32_t word;
	uint16_t msig;

	char *word_str = strtok(str_of_words, "_");
	while (word_str != NULL)
	{
		word = strtoul(word_str, NULL, 16);
		if (word == 0) break;
		decode_half_word(word & 0xFFFF, c40_list, &i);

		msig = word >> 16;
		if (msig > 0)
			decode_half_word(msig, c40_list, &i);

		word_str = strtok(NULL, "_");
	}

	for (c = 0; c < i; c++)
	{
		c40_str[c] = c40_to_char(c40_list[c]);
		if (!c40_str[c]) return -1;
	}
	c40_str[i] = '\0';
	return 0;
}