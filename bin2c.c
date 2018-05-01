#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

int main(int argc, char * argv[])
{
	FILE * fp_in, * fp_out;
	unsigned int length, left;
	char fname[256], * p;
	uint8_t buffer[256];

	if(argc != 3)
	{
		printf("Usage: %s <binary file> <c file>\n", argv[0]);
		exit(-1);
	}

	fp_in = fopen(argv[1], "rb");
	if(fp_in == NULL)
	{
		printf("Failed to open file %s for reading\n", argv[1]);
		exit(-1);
	}

	fp_out = fopen(argv[2], "wt");
	if(fp_out == NULL)
	{
		printf("Failed to open file %s for output\n", argv[2]);
		exit(-1);
	}

	fseek(fp_in, 0, SEEK_END);
	length = ftell(fp_in);
	fseek(fp_in, 0, SEEK_SET);
	left = length;

	fprintf(fp_out, "/* Automatically generated file from %s */\n", argv[1]);
	strcpy(fname, argv[1]);
	for(p = fname; *p; p++)
		if(!isalnum(*p))
			*p = '_';
	fprintf(fp_out, "unsigned int %s_len = %d;\n", fname, length);
	fprintf(fp_out, "unsigned char %s[] = {\n\t", fname);

	while(left)
	{
		int to_read = left < sizeof(buffer) ? left : sizeof(buffer);
		int bytes = fread(buffer, 1, to_read, fp_in);
		int i;
	
		for(i = 0; i < bytes; i++)
			fprintf(fp_out, "0x%02x%s", buffer[i], (i % 16) == 15 ? ",\n\t" : ", ");
			
		left -= bytes;
	}

	fprintf(fp_out, "\n\t};\n");

	fclose(fp_in);
	fclose(fp_out);

	return 0;
}
