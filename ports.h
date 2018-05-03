#ifndef SPINV_PORTS
#define SPINV_PORTS

#include "controls.h"

#include <stdint.h>

void init_ports(GameControl *control);
uint8_t read_port(uint8_t port);
void write_port(uint8_t port, uint8_t data);
void print_shiftreg_state();

#endif
