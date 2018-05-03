#ifndef SPINV_INTERRUPTS
#define SPINV_INTERRUPTS

#include <stdint.h>
#include <pthread.h>

/* interrupt vector: its fields are boolean variables indicating whether there is an interrupt of that type waiting. */
typedef struct {
	uint8_t hblank:1;
	uint8_t vblank:1;
	uint8_t padding:6;
} InterruptVector;

typedef struct {
	InterruptVector vector;
	pthread_mutex_t vector_mutex; /* interrupts are set and checked from multiple threads. Acquire the mutex before modifying the interrupt_vector's fields. */
	uint8_t inte; /* a special bit in the CPU which determines if interrupts are enabled or disabled */
	pthread_mutex_t inte_mutex;
} Interrupt;

void initialize_interrupts(Interrupt *interrupts);
void destroy_interrupts(Interrupt *interrupts);

void enable_interrupts(Interrupt *interrupts);
void disable_interrupts(Interrupt *interrupts);

int interrupt_waiting(Interrupt *interrupts);
void load_interrupt_instruction(Interrupt *interrupts, uint8_t *dest);

void trigger_hblank(Interrupt *interrupts);
void trigger_vblank(Interrupt *interrupts);

void clear_interrupts(Interrupt *interrupts);

#endif
