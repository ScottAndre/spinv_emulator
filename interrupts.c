#include "interrupts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <threads.h>
//#include "c11threads/threads.h"

void initialize_interrupts(Interrupt *interrupts) {
	interrupts->vector.hblank = 0;
	interrupts->vector.vblank = 0;
	int success = pthread_mutex_init(&interrupts->vector_mutex, NULL);
	if(success != 0) { /* TODO: check the individual errors that may occur. See the man page */
		fprintf(stderr, "ERROR: Failed to initialize interrupt vector mutex.");
		exit(EXIT_FAILURE);
	}
	interrupts->inte = 1; /* interrupts enabled by default */
	success = pthread_mutex_init(&interrupts->inte_mutex, NULL);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to initialize INTE mutex.");
		exit(EXIT_FAILURE);
	}
}

void destroy_interrupts(Interrupt *interrupts) {
	pthread_mutex_destroy(&interrupts->vector_mutex);
	pthread_mutex_destroy(&interrupts->inte_mutex);
	/* TODO: check for failure */
}

void enable_interrupts(Interrupt *interrupts) {
	int success;
	success = pthread_mutex_lock(&interrupts->inte_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to lock INTE for writing.");
	}

	interrupts->inte = 1;

	success = pthread_mutex_unlock(&interrupts->inte_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to unlock INTE after writing.");
	}
}

void disable_interrupts(Interrupt *interrupts) {
	int success;
	success = pthread_mutex_lock(&interrupts->inte_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to lock INTE for writing.");
	}

	interrupts->inte = 0;

	success = pthread_mutex_unlock(&interrupts->inte_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to unlock INTE after writing.");
	}
}

int interrupt_waiting(Interrupt *interrupts) {
	if(interrupts->vector.hblank + interrupts->vector.vblank > 0) {
		return 1;
	}
	else {
		return 0;
	}
}

void load_interrupt_instruction(Interrupt *interrupts, uint8_t *dest) {
	uint8_t instruction[3] = { 0x00, 0x00, 0x00 };
	if(interrupts->vector.hblank == 1) {
		instruction[0] = 0xcf; // RST 1
	}
	else if(interrupts->vector.vblank == 1) {
		instruction[0] = 0xd7; // RST 2
	}
	memcpy(dest, &instruction[0], 3);
}

void trigger_hblank(Interrupt *interrupts) {
	if(interrupts->inte == 0) {
		return;
	}
	//fprintf(stderr, "HBLANK\n");

	int success;
	success = pthread_mutex_lock(&interrupts->vector_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to lock interrupt vector for writing.");
	}

	interrupts->vector.hblank = 1;

	success = pthread_mutex_unlock(&interrupts->vector_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to unlock interrupt vector after writing.");
	}
}

void trigger_vblank(Interrupt *interrupts) {
	if(interrupts->inte == 0) {
		return;
	}
	//fprintf(stderr, "VBLANK\n");

	int success;
	success = pthread_mutex_lock(&interrupts->vector_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to lock interrupt vector for writing.");
	}

	interrupts->vector.vblank = 1;

	success = pthread_mutex_unlock(&interrupts->vector_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to unlock interrupt vector after writing.");
	}
}

void clear_interrupts(Interrupt *interrupts) {
	int success;
	success = pthread_mutex_lock(&interrupts->vector_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to lock interrupt vector for writing.");
	}

	interrupts->vector.hblank = 0;
	interrupts->vector.vblank = 0;

	success = pthread_mutex_unlock(&interrupts->vector_mutex);
	if(success != 0) { /* TODO */
		fprintf(stderr, "ERROR: Failed to unlock interrupt vector after writing.");
	}
}
