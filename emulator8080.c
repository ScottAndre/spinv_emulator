/* Intel 8080 Space Invaders emulator.
 * Written by Scott Andre
 * Created Dec 18 2016
 */

#include "cpu8080.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define EXIT_IO_ERROR 3

void help(char *programName) {
	fprintf(stdout, "Usage: %s <filename>\n", programName);
}

int main(int argc, char **argv) {
	if(argc < 2) {
		help(argv[0]);
		return EXIT_SUCCESS;
	}

	/* Open file */
	char *filename = argv[1];
	FILE *file = fopen(filename, "rb");
	if(file == NULL) {
		fprintf(stderr, "ERROR: unable to open file %s\n%s\n", filename, strerror(errno));
		return EXIT_IO_ERROR;
	}

	/* get file size */
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	/* Initialize machine memory, and read program into ROM
	 * ROM: $0000-$1fff
	 * RAM: $2000-$3fff
	 */
	uint8_t *mem = calloc(0x4000, sizeof(uint8_t));

	size_t bytes_read = fread(mem, sizeof(uint8_t), size, file);

	if(bytes_read < size) {
		if(feof(file)) {
			fprintf(stderr, "WARNING: found EOF before reading all bytes of file. This may have occurred because of an error in determining file size.\n");
		}
		if(ferror(file)) {
			fprintf(stderr, "ERROR: unable to continue reading file, aborting.\n");
			return EXIT_IO_ERROR;
		}
	}

	fclose(file);

	CPU cpu;
	initializeCPU(&cpu);
	
	emulate(&cpu, mem);

	free(mem);
	return 0;
}
