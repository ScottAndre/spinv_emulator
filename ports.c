#include "ports.h"

#include <stdio.h>

typedef struct {
	uint16_t contents;
	uint8_t offset;
} ShiftRegState;

static ShiftRegState sreg_state = { .contents = 0, .offset = 0 };

static GameControl *game_control; /* TODO: this pointer must be read-only or things will get messed up. Enforce it somehow */

uint8_t read_input0();
uint8_t read_input1();
uint8_t read_input2();
uint8_t read_shift_register();

void write_shift_register_offset(uint8_t data);
void write_sound0(uint8_t data);
void write_shift_register_contents(uint8_t data);
void write_sound1(uint8_t data);
void write_watchdog(uint8_t data);

void init_ports(GameControl *control) {
	game_control = control;
}

uint8_t read_port(uint8_t port) {
	switch(port) {
		case 0: return read_input0();
		case 1: return read_input1();
		case 2: return read_input2();
		case 3: return read_shift_register();
		default:
			fprintf(stderr, "WARNING: Attempted to read from unavailable input port %d.\n", port);
			return 0;
	}
}

void write_port(uint8_t port, uint8_t data) {
	switch(port) {
		case 2: write_shift_register_offset(data);   break;
		case 3: write_sound0(data);                  break;
		case 4: write_shift_register_contents(data); break;
		case 5: write_sound1(data);                  break;
		case 6: write_watchdog(data);                break;
		default:
			fprintf(stderr, "WARNING: Attempted to write to unavailable output port %d.\n", port);
			break;
	}
}

uint8_t read_input0() {
	/* TODO: implement this fully if you find a good spec online. It's unused by Space Invaders, so there's no real information on what it reads. */
	return 0x0e; /* bits 1-3 are always 1. the others are set to 0 because who knows where they come from */
}

uint8_t read_input1() {
	/* Input 1
	 *   bit 0 = CREDIT
	 *   bit 1 = 2P start
	 *   bit 2 = 1P start
	 *   bit 3 = always 1
	 *   bit 4 = 1P shot
	 *   bit 5 = 1P left
	 *   bit 6 = 1P right
	 *   bit 7 = not connected (so just return 0?)
	 */
	uint8_t credit = game_control->credit;
	PlayerControl p1_control = game_control->player1;
	PlayerControl p2_control = game_control->player2;
	uint8_t status = 0x08;
	if(credit > 0) {
		status |= 0x01;
	}
	if(p2_control.start == 1) {
		status |= 0x02;
	}
	if(p1_control.start == 1) {
		status |= 0x04;
	}
	if(p1_control.fire == 1) {
		status |= 0x10;
	}
	if(p1_control.left == 1) {
		status |= 0x20;
	}
	if(p1_control.right == 1) {
		status |= 0x40;
	}
	return status;
}

uint8_t read_input2() {
	/* Input 2
	 *   bit 0-1 = 00: 3 ships 01: 4 ships 10: 5 ships 11: 6 ships
	 *   bit 2 = tilt
	 *   bit 3 = 0: extra ship at 1500 1: extra ship at 1000
	 *   bit 4 = 2P shot
	 *   bit 5 = 2P left
	 *   bit 6 = 2P right
	 *   bit 7 = coin info displayed in demo screen (0: ON)
	 */
	PlayerControl p2_control = game_control->player2;
	uint8_t status = 0x00;
	/* TODO: Implement bits 0-3 */
	if(p2_control.fire == 1) {
		status |= 0x10;
	}
	if(p2_control.left == 1) {
		status |= 0x20;
	}
	if(p2_control.right == 1) {
		status |= 0x40;
	}
	/* TODO: implement bit 7 */
	return status;
}

void write_sound0(uint8_t data) {
	/* TODO: implement sound.
	 *   bit 0 = UFO
	 *   bit 1 = shot
	 *   bit 2 = player death
	 *   bit 3 = invader death
	 *   bit 4 = extended play
	 *   bit 5 = AMP enable
	 *   bit 6 = not wired
	 *   bit 7 = not wired
	 */
}

void write_sound1(uint8_t data) {
	/* TODO: implement sound.
	 *   bit 0 = fleet movement 1
	 *   bit 1 = fleet movement 2
	 *   bit 2 = fleet movement 3
	 *   bit 3 = fleet movement 4
	 *   bit 4 = UFO hit
	 *   bit 5 = not wired
	 *   bit 6 = not wired
	 *   bit 7 = not wired
	 */
}

uint8_t read_shift_register() {
	return (sreg_state.contents >> (8 - sreg_state.offset)) & 0xff;
}

void write_shift_register_offset(uint8_t data) {
	sreg_state.offset = data & 0x07;
}

void write_shift_register_contents(uint8_t data) {
	sreg_state.contents = (sreg_state.contents >> 8) | (data << 8);
}

void write_watchdog(uint8_t data) {
	/* TODO: implement this, if you can figure out what it is */
}

void print_shiftreg_state() {
	fprintf(stdout, "ShiftReg | Contents: %.16x | Offset: %u | Shifted: %.8x\n", sreg_state.contents, sreg_state.offset, read_shift_register());
}
