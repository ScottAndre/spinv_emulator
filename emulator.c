/* Intel 8080 Space Invaders arcade machine emulator.
 * Written by Scott Andre * Created Dec 18 2016
 */

#include "emulator.h"
#include "display.h"
#include "ports.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define EXIT_IO_ERROR 3

void *emulate_cpu(void *state);

void help(char *program_name) {
	fprintf(stdout, "Usage: %s <filename>\n", program_name);
}

int main(int argc, char **argv) {
	if(argc < 2) {
		help(argv[0]);
		return EXIT_SUCCESS;
	}

	/*
	 * ----- READ ROM INTO MEMORY -----
	 */

	/* all-purpose variable for checking the success of various operations */
	int success;

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
	 * TODO: RAM Mirror: $4000-$7fff
	 *   implementing this will require abstracting writes to memory into its own function, which would probably be a good thing for thread-safety anyway
	 */
	uint8_t *memory = calloc(0x4000, sizeof(uint8_t));

	size_t bytes_read = fread(memory, sizeof(uint8_t), size, file);

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

	/* 
	 * ----- RESOURCE INITIALIZATION -----
	 */

	/* initialize CPU */
	CPU *cpu = malloc(sizeof(CPU));
	initializeCPU(cpu);

	/* initialize interrupt vector */
	Interrupt *interrupts = malloc(sizeof(Interrupt)); /* Must be on the heap, so that it can be shared between threads */
	initialize_interrupts(interrupts);

	/* initialize controls */
	GameControl *game_control = malloc(sizeof(GameControl));
	init_game_control(game_control);
	init_ports(game_control);

	GameState *game_state = malloc(sizeof(GameState));
	game_state->cpu = cpu;
	game_state->memory = memory;
	game_state->interrupts = interrupts;
	game_state->game_control = game_control;

	/* initialize thread synchronization variables */
	sem_t *thread_sync = malloc(sizeof(sem_t));
	sem_init(thread_sync, 0, 0); /* TODO: check for failure */
	game_state->thread_sync = thread_sync;
	int *thread_exit = malloc(sizeof(int));
	*thread_exit = 0;
	game_state->thread_exit = thread_exit;

	/* Initialize display */
	GtkApplication *app;
	init_display(&app, game_state);

	/* initialize CPU thread */
	pthread_t cpu_thread;
	success = pthread_create(&cpu_thread, NULL, emulate_cpu, game_state);
	/* TODO: check individual failures
	if(success == thrd_nomem) {
		fprintf(stderr, "ERROR: Insufficient memory for display thread.");
		return EXIT_FAILURE;
	} */
	if(success != 0) {
		fprintf(stderr, "ERROR: Unable to create CPU thread.");
		return EXIT_FAILURE;
	}

	/*
	 * ----- BEGIN EMULATION -----
	 */
	
	sem_post(game_state->thread_sync); /* TODO: check for failure */

	int status = run_display(app, argc, argv);

	/*
	 * ----- CLEAN UP RESOURCES -----
	 */

	*game_state->thread_exit = 1;

	void *cpu_success;
	success = pthread_join(cpu_thread, &cpu_success);
	if(success != 0) { /* TODO: check for specific errors */
		fprintf(stderr, "ERROR: Failed to join with CPU thread.");
		return EXIT_FAILURE;
	}
	/* TODO: check the value of cpu_success? maybe just set to NULL */

	close_display(app);

	destroy_game_control(game_control);
	destroy_interrupts(interrupts);

	free(thread_exit);
	free(thread_sync);
	free(game_state);
	free(game_control);
	free(interrupts);
	free(memory);
	free(cpu);

	return status;
}

void *emulate_cpu(void *state) {
	GameState *game_state = (GameState *)state;
	CPU *cpu = game_state->cpu;
	Interrupt *interrupts = game_state->interrupts;
	int success;

	sem_wait(game_state->thread_sync); /* TODO: check for failure */

	/* main emulation loop */
	while(1) {
		struct timespec now;
		struct timespec after;

		timespec_get(&now, TIME_UTC);

		/* check if emulation has stopped */
		if(*game_state->thread_exit == 1) {
			break;
		}

		/* handle interrupts */
		if(interrupt_waiting(interrupts)) {
			cpu->halted = 0; /* restart the CPU, if it is halted. */
			/* interrupts are disabled when an interrupt is being handled. The program must manually re-enable interrupts, once it has finished saving data, via an EI instruction. */
			disable_interrupts(interrupts);
			cpu->has_interrupt = 1; /* signals the CPU that it has an interrupt, which requires special handling. */
			load_interrupt_instruction(interrupts, &cpu->interrupt_instruction[0]); /* load the instruction requested by the interrupt onto the CPU */
			clear_interrupts(interrupts);
		}

		/* emulate an instruction, keeping track of time elapsed */
		uint8_t cycles_elapsed = emulate(cpu, game_state->memory, interrupts);

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
			success = nanosleep(&instruction_time, NULL);
			if(success == -1) {
				fprintf(stderr, "WARNING: nanosleep unable to sleep full duration of elapsed cycles.\n%s\n", strerror(errno));
			}
		}
	}

	pthread_exit(EXIT_SUCCESS);
}
