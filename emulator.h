#ifndef SPINV_EMULATOR
#define SPINV_EMULATOR

#include "cpu8080.h"
#include "interrupts.h"
#include "controls.h"

#include <stdint.h>
//#include <threads.h>
//#include "c11threads/threads.h" // glibc currently does not have support for the c11 thread library, threads.h. This is a stopgap implementation of said library over pthreads.
#include <pthread.h>
#include <semaphore.h>

/* Contains all information that other threads need to know about the machines state. Anything that gets included in a GameState should be allocated on the heap */
typedef struct {
	CPU *cpu;
	uint8_t *memory;
	Interrupt *interrupts;
	GameControl *game_control;
	sem_t *thread_sync;
	int *thread_exit;
} GameState;

#endif
