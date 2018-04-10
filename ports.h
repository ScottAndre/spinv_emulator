#ifndef SPINV_PORTS
#define SPINV_PORTS

#include <stdint.h>

uint8_t read_port(uint8_t port);
void write_port(uint8_t port, uint8_t data);

#endif
