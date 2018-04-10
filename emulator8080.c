/* Intel 8080 Space Invaders arcade machine emulator.
 * Written by Scott Andre
 * Created Dec 18 2016
 */

#include "cpu8080.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
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
	
	/* main emulation loop */
	while(1) {
		struct timespec now;
		/* TODO: handle interrupts */
		/* check all peripheral devices for interrupts */
		cpu->halted = 0;
		cpu->inte = 0;
		cpu->has_interrupt = 1;
		/* set cpu->inte to 0 as well - interrupts are not enabled while being processed */
		/* feed the proper instruction to the CPU */

		/* emulate an instruction, keeping track of time elapsed */
		struct timespec after;
		timespec_get(&now, TIME_UTC);
		uint8_t cycles_elapsed = emulate(&cpu, mem);
		timespec_get(&after, TIME_UTC);

		/* calculate total amount of time to sleep before next instruction */
		long time_elapsed = after.tv_nsec - now.tv_nsec;
		//fprintf(stdout, "Time elapsed: %ld nanoseconds\n", time_elapsed);
		struct timespec instruction_time;
		instruction_time.tv_sec = 0;
		/* amount of time to wait, measured in nanoseconds */
		instruction_time.tv_nsec = (cycles_elapsed * CYCLE_LENGTH) - time_elapsed;
		//fprintf(stdout, "Time to sleep: %ld nanoseconds\n", instruction_time.tv_nsec);
		if(instruction_time.tv_nsec >= 0) {
			int success = nanosleep(&instruction_time, NULL);
			if(success == -1) {
				fprintf(stderr, "WARNING: nanosleep unable to sleep full duration of elapsed cycles.\n%s\n", strerror(errno));
			}
		}
	}

	free(mem);
	return 0;
}
