#include "cpu8080.h"
#include "disassembler8080.h"
#include "ports.h"
#include "interrupts.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#define DEBUG

void initializeOpLengths(uint8_t *lengths);
void initializeOpCycles(uint8_t *cycles);

uint16_t to_double_word(uint8_t low, uint8_t high);
void from_double_word(uint16_t dword, uint8_t *low, uint8_t *high);

void stack_push(CPU *cpu, uint8_t *mem, uint8_t byte1, uint8_t byte2);
void stack_pop(CPU *cpu, uint8_t *mem, uint8_t *byte1, uint8_t *byte2);

void call(CPU *cpu, uint8_t *mem, uint16_t addr);
void ret(CPU *cpu, uint8_t *mem);

uint8_t *get_register(CPU *cpu, uint8_t *mem, char reg);

void set_z(Flags *flags, uint16_t result);
void set_s(Flags *flags, uint16_t result);
void set_p(Flags *flags, uint16_t result);
void set_cy(Flags *flags, uint16_t result);

uint8_t two_comp(uint8_t i); /* efficiently returns two's complement of a byte. */

static uint8_t opLengths[NUM_OF_OPCODES];
static uint8_t opCycles[NUM_OF_OPCODES];
static uint8_t cycle_override;

/* These default values may not be correct, but I think the program sets the values of registers, flags, SP and PC anyway before they get used. */
void initializeCPU(CPU *cpu) {
	cpu->b = 0;
	cpu->c = 0;
	cpu->d = 0;
	cpu->e = 0;
	cpu->h = 0;
	cpu->l = 0;
	cpu->a = 0;
	cpu->sp = 0; /* stack pointer gets set by the program itself */
	cpu->pc = 0;
	cpu->flags.z = 0;
	cpu->flags.s = 0;
	cpu->flags.p = 0;
	cpu->flags.cy = 0;
	cpu->flags.ac = 0;
	memset(&cpu->interrupt_instruction[0], 0x00, 3);
	//cpu->inte = 1;
	cpu->has_interrupt = 0;
	cpu->halted = 0;

	initializeOpLengths(opLengths);
	initializeOpCycles(opCycles);
}

void printCPU(CPU *cpu, uint8_t *mem) {
	fprintf(stdout, "#--- CPU ------------\n");
	fprintf(stdout, "| Registers:\n| B: 0x%.2x    H: 0x%.2x\n", cpu->b, cpu->h);
	fprintf(stdout, "| C: 0x%.2x    L: 0x%.2x\n", cpu->c, cpu->l);
	fprintf(stdout, "| D: 0x%.2x    M: 0x%.2x\n", cpu->d, mem[to_double_word(cpu->l, cpu->h)]);
	fprintf(stdout, "| E: 0x%.2x    A: 0x%.2x\n", cpu->e, cpu->a);
	fprintf(stdout, "| BC: 0x%.4x DE: 0x%.4x\n", to_double_word(cpu->c, cpu->b), to_double_word(cpu->e, cpu->d));
	fprintf(stdout, "#--------------------\n");
	fprintf(stdout, "| Stack Pointer: 0x%.4x\n", cpu->sp);
	fprintf(stdout, "| Program Counter: 0x%.4x\n", cpu->pc);
	fprintf(stdout, "#--------------------\n");
	fprintf(stdout, "| Flags:\n| z: %x  s: %x  p: %x  cy: %x  ac: %x\n", cpu->flags.z, cpu->flags.s, cpu->flags.p, cpu->flags.cy, cpu->flags.ac);
	fprintf(stdout, "#--------------------\n");
}

void printOpcodeInfo(uint16_t current_pc, uint8_t *opcode) {
	char disassembled[32];
	disassemble(opcode, disassembled);
	fprintf(stdout, "==========\n");
	fprintf(stdout, "0x%.4x: %s\n", current_pc, disassembled);
	fprintf(stdout, "#--- Instruction -----\n");
	fprintf(stdout, "| Instruction: %s\n", disassembled);
	fprintf(stdout, "| Instruction length: %d bytes\n", opLengths[opcode[0]]);
	fprintf(stdout, "| Instruction cycles: %d @ 2 MHz = %.1f microseconds elapsed\n", opCycles[opcode[0]], (double)opCycles[opcode[0]] * 1000000 / CYCLES_PER_SECOND);
	//fprintf(stdout, "#--------------------\n");
}

uint8_t emulate(CPU *cpu, uint8_t *mem, Interrupt *interrupts) {

	if(cpu->halted) {
		return opCycles[0x00]; // for the time being, we'll emulate a halted CPU as if it was just executing NOPs. It'll probably never come up.
	}

	cycle_override = 255;

	uint8_t *opcode;

	if(cpu->has_interrupt) {
		opcode = &cpu->interrupt_instruction[0];
		cpu->has_interrupt = 0;
	}
	else {
		opcode = &mem[cpu->pc];
		cpu->pc += opLengths[opcode[0]];
	}

	#ifndef DEBUG
	char disassembled[32];
	disassemble(opcode, disassembled);
	if(cpu->has_interrupt) {
		fprintf(stdout, "Interrupt! %s\n", disassembled);
	}
	else {
		fprintf(stdout, "0x%.4x: %s\n", cpu->pc - opLengths[opcode[0]], disassembled);
	}
	#endif

	switch(opcode[0]) { /* maybe make this a lookup table with function pointers? probably doesn't matter for an arcade emulator */
		case 0x00: NOP(cpu); 											break;
		case 0x01: LXI(cpu, 'B', to_double_word(opcode[1], opcode[2])); break;
		case 0x02: STAX(cpu, mem, 'B'); 								break;
		case 0x03: INX(cpu, 'B'); 										break;
		case 0x04: INR(cpu, mem, 'B'); 									break;
		case 0x05: DCR(cpu, mem, 'B'); 									break;
		case 0x06: MVI(cpu, mem, 'B', opcode[1]); 						break;
		case 0x07: RLC(cpu); 											break;
		case 0x09: DAD(cpu, 'B'); 										break;
		case 0x0a: LDAX(cpu, mem, 'B'); 								break;
		case 0x0b: DCX(cpu, 'B'); 										break;
		case 0x0c: INR(cpu, mem, 'C'); 									break;
		case 0x0d: DCR(cpu, mem, 'C'); 									break;
		case 0x0e: MVI(cpu, mem, 'C', opcode[1]); 						break;
		case 0x0f: RRC(cpu); 											break;
		case 0x11: LXI(cpu, 'D', to_double_word(opcode[1], opcode[2])); break;
		case 0x12: STAX(cpu, mem, 'D'); 								break;
		case 0x13: INX(cpu, 'D'); 										break;
		case 0x14: INR(cpu, mem, 'D'); 									break;
		case 0x15: DCR(cpu, mem, 'D'); 									break;
		case 0x16: MVI(cpu, mem, 'D', opcode[1]); 						break;
		case 0x17: RAL(cpu); 											break;
		case 0x19: DAD(cpu, 'D'); 										break;
		case 0x1a: LDAX(cpu, mem, 'D'); 								break;
		case 0x1b: DCX(cpu, 'D'); 										break;
		case 0x1c: INR(cpu, mem, 'E'); 									break;
		case 0x1d: DCR(cpu, mem, 'E'); 									break;
		case 0x1e: MVI(cpu, mem, 'E', opcode[1]); 						break;
		case 0x1f: RAR(cpu); 											break;
		case 0x20: RIM(cpu); 											break;
		case 0x21: LXI(cpu, 'H', to_double_word(opcode[1], opcode[2])); break;
		case 0x22: SHLD(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0x23: INX(cpu, 'H'); 										break;
		case 0x24: INR(cpu, mem, 'H'); 									break;
		case 0x25: DCR(cpu, mem, 'H'); 									break;
		case 0x26: MVI(cpu, mem, 'H', opcode[1]); 						break;
		case 0x27: DAA(cpu); 											break;
		case 0x29: DAD(cpu, 'H'); 										break;
		case 0x2a: LHLD(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0x2b: DCX(cpu, 'H'); 										break;
		case 0x2c: INR(cpu, mem, 'L'); 									break;
		case 0x2d: DCR(cpu, mem, 'L'); 									break;
		case 0x2e: MVI(cpu, mem, 'L', opcode[1]); 						break;
		case 0x2f: CMA(cpu); 											break;
		case 0x30: SIM(cpu); 											break;
		case 0x31: LXI(cpu, 'S', to_double_word(opcode[1], opcode[2])); break;
		case 0x32: STA(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0x33: INX(cpu, 'S'); 										break;
		case 0x34: INR(cpu, mem, 'M'); 									break;
		case 0x35: DCR(cpu, mem, 'M'); 									break;
		case 0x36: MVI(cpu, mem, 'M', opcode[1]); 						break;
		case 0x37: STC(cpu); 											break;
		case 0x39: DAD(cpu, 'S'); 										break;
		case 0x3a: LDA(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0x3b: DCX(cpu, 'S'); 										break;
		case 0x3c: INR(cpu, mem, 'A'); 									break;
		case 0x3d: DCR(cpu, mem, 'A'); 									break;
		case 0x3e: MVI(cpu, mem, 'A', opcode[1]); 						break;
		case 0x3f: CMC(cpu); 											break;
		case 0x40: MOV(cpu, mem, 'B', 'B'); 							break;
		case 0x41: MOV(cpu, mem, 'B', 'C'); 							break;
		case 0x42: MOV(cpu, mem, 'B', 'D'); 							break;
		case 0x43: MOV(cpu, mem, 'B', 'E'); 							break;
		case 0x44: MOV(cpu, mem, 'B', 'H'); 							break;
		case 0x45: MOV(cpu, mem, 'B', 'L'); 							break;
		case 0x46: MOV(cpu, mem, 'B', 'M'); 							break;
		case 0x47: MOV(cpu, mem, 'B', 'A'); 							break;
		case 0x48: MOV(cpu, mem, 'C', 'B'); 							break;
		case 0x49: MOV(cpu, mem, 'C', 'C'); 							break;
		case 0x4a: MOV(cpu, mem, 'C', 'D'); 							break;
		case 0x4b: MOV(cpu, mem, 'C', 'E'); 							break;
		case 0x4c: MOV(cpu, mem, 'C', 'H'); 							break;
		case 0x4d: MOV(cpu, mem, 'C', 'L'); 							break;
		case 0x4e: MOV(cpu, mem, 'C', 'M'); 							break;
		case 0x4f: MOV(cpu, mem, 'C', 'A'); 							break;
		case 0x50: MOV(cpu, mem, 'D', 'B'); 							break;
		case 0x51: MOV(cpu, mem, 'D', 'C'); 							break;
		case 0x52: MOV(cpu, mem, 'D', 'D'); 							break;
		case 0x53: MOV(cpu, mem, 'D', 'E'); 							break;
		case 0x54: MOV(cpu, mem, 'D', 'H'); 							break;
		case 0x55: MOV(cpu, mem, 'D', 'L'); 							break;
		case 0x56: MOV(cpu, mem, 'D', 'M'); 							break;
		case 0x57: MOV(cpu, mem, 'D', 'A'); 							break;
		case 0x58: MOV(cpu, mem, 'E', 'B'); 							break;
		case 0x59: MOV(cpu, mem, 'E', 'C'); 							break;
		case 0x5a: MOV(cpu, mem, 'E', 'D'); 							break;
		case 0x5b: MOV(cpu, mem, 'E', 'E'); 							break;
		case 0x5c: MOV(cpu, mem, 'E', 'H'); 							break;
		case 0x5d: MOV(cpu, mem, 'E', 'L'); 							break;
		case 0x5e: MOV(cpu, mem, 'E', 'M'); 							break;
		case 0x5f: MOV(cpu, mem, 'E', 'A'); 							break;
		case 0x60: MOV(cpu, mem, 'H', 'B'); 							break;
		case 0x61: MOV(cpu, mem, 'H', 'C'); 							break;
		case 0x62: MOV(cpu, mem, 'H', 'D'); 							break;
		case 0x63: MOV(cpu, mem, 'H', 'E'); 							break;
		case 0x64: MOV(cpu, mem, 'H', 'H'); 							break;
		case 0x65: MOV(cpu, mem, 'H', 'L'); 							break;
		case 0x66: MOV(cpu, mem, 'H', 'M'); 							break;
		case 0x67: MOV(cpu, mem, 'H', 'A'); 							break;
		case 0x68: MOV(cpu, mem, 'L', 'B'); 							break;
		case 0x69: MOV(cpu, mem, 'L', 'C'); 							break;
		case 0x6a: MOV(cpu, mem, 'L', 'D'); 							break;
		case 0x6b: MOV(cpu, mem, 'L', 'E'); 							break;
		case 0x6c: MOV(cpu, mem, 'L', 'H'); 							break;
		case 0x6d: MOV(cpu, mem, 'L', 'L'); 							break;
		case 0x6e: MOV(cpu, mem, 'L', 'M'); 							break;
		case 0x6f: MOV(cpu, mem, 'L', 'A'); 							break;
		case 0x70: MOV(cpu, mem, 'M', 'B'); 							break;
		case 0x71: MOV(cpu, mem, 'M', 'C'); 							break;
		case 0x72: MOV(cpu, mem, 'M', 'D'); 							break;
		case 0x73: MOV(cpu, mem, 'M', 'E'); 							break;
		case 0x74: MOV(cpu, mem, 'M', 'H'); 							break;
		case 0x75: MOV(cpu, mem, 'M', 'L'); 							break;
		case 0x76: HLT(cpu); 											break;
		case 0x77: MOV(cpu, mem, 'M', 'A'); 							break;
		case 0x78: MOV(cpu, mem, 'A', 'B'); 							break;
		case 0x79: MOV(cpu, mem, 'A', 'C'); 							break;
		case 0x7a: MOV(cpu, mem, 'A', 'D'); 							break;
		case 0x7b: MOV(cpu, mem, 'A', 'E'); 							break;
		case 0x7c: MOV(cpu, mem, 'A', 'H'); 							break;
		case 0x7d: MOV(cpu, mem, 'A', 'L'); 							break;
		case 0x7e: MOV(cpu, mem, 'A', 'M'); 							break;
		case 0x7f: MOV(cpu, mem, 'A', 'A'); 							break;
		case 0x80: ADD(cpu, mem, 'B'); 									break;
		case 0x81: ADD(cpu, mem, 'C'); 									break;
		case 0x82: ADD(cpu, mem, 'D'); 									break;
		case 0x83: ADD(cpu, mem, 'E'); 									break;
		case 0x84: ADD(cpu, mem, 'H'); 									break;
		case 0x85: ADD(cpu, mem, 'L'); 									break;
		case 0x86: ADD(cpu, mem, 'M'); 									break;
		case 0x87: ADD(cpu, mem, 'A'); 									break;
		case 0x88: ADC(cpu, mem, 'B'); 									break;
		case 0x89: ADC(cpu, mem, 'C'); 									break;
		case 0x8a: ADC(cpu, mem, 'D'); 									break;
		case 0x8b: ADC(cpu, mem, 'E'); 									break;
		case 0x8c: ADC(cpu, mem, 'H'); 									break;
		case 0x8d: ADC(cpu, mem, 'L'); 									break;
		case 0x8e: ADC(cpu, mem, 'M'); 									break;
		case 0x8f: ADC(cpu, mem, 'A'); 									break;
		case 0x90: SUB(cpu, mem, 'B'); 									break;
		case 0x91: SUB(cpu, mem, 'C'); 									break;
		case 0x92: SUB(cpu, mem, 'D'); 									break;
		case 0x93: SUB(cpu, mem, 'E'); 									break;
		case 0x94: SUB(cpu, mem, 'H'); 									break;
		case 0x95: SUB(cpu, mem, 'L'); 									break;
		case 0x96: SUB(cpu, mem, 'M'); 									break;
		case 0x97: SUB(cpu, mem, 'A'); 									break;
		case 0x98: SBB(cpu, mem, 'B'); 									break;
		case 0x99: SBB(cpu, mem, 'C'); 									break;
		case 0x9a: SBB(cpu, mem, 'D'); 									break;
		case 0x9b: SBB(cpu, mem, 'E'); 									break;
		case 0x9c: SBB(cpu, mem, 'H'); 									break;
		case 0x9d: SBB(cpu, mem, 'L'); 									break;
		case 0x9e: SBB(cpu, mem, 'M'); 									break;
		case 0x9f: SBB(cpu, mem, 'A'); 									break;
		case 0xa0: ANA(cpu, mem, 'B'); 									break;
		case 0xa1: ANA(cpu, mem, 'C'); 									break;
		case 0xa2: ANA(cpu, mem, 'D'); 									break;
		case 0xa3: ANA(cpu, mem, 'E'); 									break;
		case 0xa4: ANA(cpu, mem, 'H'); 									break;
		case 0xa5: ANA(cpu, mem, 'L'); 									break;
		case 0xa6: ANA(cpu, mem, 'M'); 									break;
		case 0xa7: ANA(cpu, mem, 'A'); 									break;
		case 0xa8: XRA(cpu, mem, 'B'); 									break;
		case 0xa9: XRA(cpu, mem, 'C'); 									break;
		case 0xaa: XRA(cpu, mem, 'D'); 									break;
		case 0xab: XRA(cpu, mem, 'E'); 									break;
		case 0xac: XRA(cpu, mem, 'H'); 									break;
		case 0xad: XRA(cpu, mem, 'L'); 									break;
		case 0xae: XRA(cpu, mem, 'M'); 									break;
		case 0xaf: XRA(cpu, mem, 'A'); 									break;
		case 0xb0: ORA(cpu, mem, 'B'); 									break;
		case 0xb1: ORA(cpu, mem, 'C'); 									break;
		case 0xb2: ORA(cpu, mem, 'D'); 									break;
		case 0xb3: ORA(cpu, mem, 'E'); 									break;
		case 0xb4: ORA(cpu, mem, 'H'); 									break;
		case 0xb5: ORA(cpu, mem, 'L'); 									break;
		case 0xb6: ORA(cpu, mem, 'M'); 									break;
		case 0xb7: ORA(cpu, mem, 'A'); 									break;
		case 0xb8: CMP(cpu, mem, 'B'); 									break;
		case 0xb9: CMP(cpu, mem, 'C'); 									break;
		case 0xba: CMP(cpu, mem, 'D'); 									break;
		case 0xbb: CMP(cpu, mem, 'E'); 									break;
		case 0xbc: CMP(cpu, mem, 'H'); 									break;
		case 0xbd: CMP(cpu, mem, 'L'); 									break;
		case 0xbe: CMP(cpu, mem, 'M'); 									break;
		case 0xbf: CMP(cpu, mem, 'A'); 									break;
		case 0xc0: RNZ(cpu, mem); 										break;
		case 0xc1: POP(cpu, mem, 'B'); 									break;
		case 0xc2: JNZ(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xc3: JMP(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xc4: CNZ(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0xc5: PUSH(cpu, mem, 'B'); 								break;
		case 0xc6: ADI(cpu, opcode[1]); 								break;
		case 0xc7: RST(cpu, mem, 0); 									break;
		case 0xc8: RZ(cpu, mem); 										break;
		case 0xc9: RET(cpu, mem); 										break;
		case 0xca: JZ(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xcc: CZ(cpu, mem, to_double_word(opcode[1], opcode[2])); 	break;
		case 0xcd: CALL(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0xce: ACI(cpu, opcode[1]); 								break;
		case 0xcf: RST(cpu, mem, 1); 									break;
		case 0xd0: RNC(cpu, mem); 										break;
		case 0xd1: POP(cpu, mem, 'D'); 									break;
		case 0xd2: JNC(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xd3: OUT(cpu, opcode[1]); 								break;
		case 0xd4: CNC(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0xd5: PUSH(cpu, mem, 'D'); 								break;
		case 0xd6: SUI(cpu, opcode[1]); 								break;
		case 0xd7: RST(cpu, mem, 2); 									break;
		case 0xd8: RC(cpu, mem); 										break;
		case 0xda: JC(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xdb: IN(cpu, opcode[1]); 									break;
		case 0xdc: CC(cpu, mem, to_double_word(opcode[1], opcode[2])); 	break;
		case 0xde: SBI(cpu, opcode[1]); 								break;
		case 0xdf: RST(cpu, mem, 3); 									break;
		case 0xe0: RPO(cpu, mem); 										break;
		case 0xe1: POP(cpu, mem, 'H'); 									break;
		case 0xe2: JPO(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xe3: XTHL(cpu, mem); 										break;
		case 0xe4: CPO(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0xe5: PUSH(cpu, mem, 'H'); 								break;
		case 0xe6: ANI(cpu, opcode[1]); 								break;
		case 0xe7: RST(cpu, mem, 4); 									break;
		case 0xe8: RPE(cpu, mem); 										break;
		case 0xe9: PCHL(cpu); 											break;
		case 0xea: JPE(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xeb: XCHG(cpu); 											break;
		case 0xec: CPE(cpu, mem, to_double_word(opcode[1], opcode[2])); break;
		case 0xee: XRI(cpu, opcode[1]); 								break;
		case 0xef: RST(cpu, mem, 5); 									break;
		case 0xf0: RP(cpu, mem); 										break;
		case 0xf1: POP(cpu, mem, 'P'); 									break;
		case 0xf2: JP(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xf3: DI(cpu, interrupts); 								break;
		case 0xf4: CP(cpu, mem, to_double_word(opcode[1], opcode[2])); 	break;
		case 0xf5: PUSH(cpu, mem, 'P'); 								break;
		case 0xf6: ORI(cpu, opcode[1]); 								break;
		case 0xf7: RST(cpu, mem, 6); 									break;
		case 0xf8: RM(cpu, mem); 										break;
		case 0xf9: SPHL(cpu); 											break;
		case 0xfa: JM(cpu, to_double_word(opcode[1], opcode[2])); 		break;
		case 0xfb: EI(cpu, interrupts); 								break;
		case 0xfc: CM(cpu, mem, to_double_word(opcode[1], opcode[2])); 	break;
		case 0xfe: CPI(cpu, opcode[1]); 								break;
		case 0xff: RST(cpu, mem, 7); 									break;
		default: unimplemented(cpu, opcode[0]); 						break;
	}

	#ifdef DEBUG
	printOpcodeInfo(cpu->pc - opLengths[opcode[0]], opcode);
	printCPU(cpu, mem);

	fgetc(stdin);
	#endif

	uint8_t cycles_elapsed = opCycles[opcode[0]];
	if(cycles_elapsed == 255) {
		fprintf(stderr, "WARNING: the number of cycles has been improperly set for operation %x. Exiting.\n", opcode[0]);
		exit(1);
	}

	if(cycle_override != 255) {
		cycles_elapsed = cycle_override;
	}
	
	return cycles_elapsed;
}

/* NOP - no operation. CPU does nothing this cycle. -- 4 cycles -- */
void NOP(CPU *cpu) {}

/* MVI - move immediate. loads 8-bit value into given register. -- 7 cycles for A-L, 10 cycles for M -- */
void MVI(CPU *cpu, uint8_t *mem, char reg, uint8_t imm) {
	uint8_t *r = get_register(cpu, mem, reg);

	*r = imm;
}

/* LXI - load 16-bit immediate. loads high byte into given register, and low byte into neighboring register. -- 10 cycles -- */
void LXI(CPU *cpu, char reg, uint16_t imm) {
	uint8_t *low;
	uint8_t *high;

	switch(reg) {
		case 'B':
			low = &cpu->c;
			high = &cpu->b;
			break;
		case 'D':
			low = &cpu->e;
			high = &cpu->d;
			break;
		case 'H':
			low = &cpu->l;
			high = &cpu->h;
			break;
		case 'S': /* stack pointer - special handling */
			cpu->sp = imm;
			return;
	}

	from_double_word(imm, low, high);
}

/* INR - register increment. Increments the value of the given register by 1. -- 5 cycles for A-L, 10 cycles for M -- */
void INR(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg); /* register is a keyword I guess */

	*r += 1;

	set_z(&cpu->flags, *r);
	set_s(&cpu->flags, *r);
	set_p(&cpu->flags, *r);

	/* set ac - special handling */
	uint8_t ac_check = ((*r - 1) & 0x0f) + 1;
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;
}

/* DCR - register decrement. Decrements the value of the given register by 1. -- 5 cycle for A-L, 10 cycles for M -- */
void DCR(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	*r -= 1;

	set_z(&cpu->flags, *r);
	set_s(&cpu->flags, *r);
	set_p(&cpu->flags, *r);

	/* set ac - add lower four bits with 2's complement of -1 */
	uint8_t ac_check = ((*r + 1) & 0x0f) + 0x0f;
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;
}

/* INX - register pair increment. Increments the 16-bit value stored in the register pair. Used for advancing memory addresses. Does not set flags. -- 5 cycle -- */
void INX(CPU *cpu, char reg) {
	uint8_t *low;
	uint8_t *high;

	switch(reg) {
		case 'B':
			low = &cpu->c;
			high = &cpu->b;
			break;
		case 'D':
			low = &cpu->e;
			high = &cpu->d;
			break;
		case 'H':
			low = &cpu->l;
			high = &cpu->h;
			break;
		case 'S': /* stack pointer */
			cpu->sp += 1;
			return;
	}

	uint16_t reg_pair = to_double_word(*low, *high);
	from_double_word(reg_pair + 1, low, high);
}

/* DCX - register pair decrement. Decrements the 16-bit value stored in the register pair. Does not set flags. -- 5 cycles -- */
void DCX(CPU *cpu, char reg) {
	uint8_t *low;
	uint8_t *high;

	switch(reg) {
		case 'B':
			low = &cpu->c;
			high = &cpu->b;
			break;
		case 'D':
			low = &cpu->e;
			high = &cpu->d;
			break;
		case 'H':
			low = &cpu->l;
			high = &cpu->h;
			break;
		case 'S': /* stack pointer - special handling */
			cpu->sp -= 1;
			return;
	}

	uint16_t reg_pair = to_double_word(*low, *high);
	from_double_word(reg_pair - 1, low, high);
}

/* DAD - double register add. Adds the 16-bit value stored in the register pair to the 16-bit value stored in the HL pair. The result is stored in HL. -- 10 cycles -- */
void DAD(CPU *cpu, char reg) {
	uint16_t reg_pair;

	switch(reg) {
		case 'B':
			reg_pair = to_double_word(cpu->c, cpu->b);
			break;
		case 'D':
			reg_pair = to_double_word(cpu->e, cpu->d);
			break;
		case 'H':
			reg_pair = to_double_word(cpu->l, cpu->h);
			break;
		case 'S': /* stack pointer */
			reg_pair = cpu->sp;
			break;
	}

	uint16_t hl_pair = to_double_word(cpu->l, cpu->h);
	uint32_t result = reg_pair + hl_pair;
	from_double_word((uint16_t)result, &cpu->l, &cpu->h);

	/* set carry if overflow */
	if((result & 0x00010000) == 0x00010000)
		cpu->flags.cy = 1;
	else
		cpu->flags.cy = 0;
}

/* ADI - add immediate. adds a given byte to the A register, stores the result in A. -- 7 cycles -- */
void ADI(CPU *cpu, uint8_t imm) {
	uint16_t result = cpu->a + imm;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);
	set_cy(&cpu->flags, result);

	uint8_t ac_check = (cpu->a & 0x0f) + (imm & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;

	cpu->a = (uint8_t)result;
}

/* SUI - subtract immediate. subtracts a given byte from the A register, stores the result in A. -- 7 cycles -- */
void SUI(CPU *cpu, uint8_t imm) {
	uint16_t result = cpu->a - imm;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);
	set_cy(&cpu->flags, result);

	uint8_t ac_check = (cpu->a & 0x0f) + (two_comp(imm) & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;

	cpu->a = (uint8_t)result;
}

/* ACI - add immediate with carry. adds a given byte to the A register, adds the carry bit, and stores the result in A. -- 7 cycles -- */
void ACI(CPU *cpu, uint8_t imm) {
	uint16_t result = cpu->a + imm + cpu->flags.cy;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);

	uint8_t ac_check = (cpu->a & 0x0f) + (imm & 0x0f) + cpu->flags.cy;
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;

	set_cy(&cpu->flags, result);

	cpu->a = (uint8_t)result;
}

/* SBI - subtract immediate with borrow. subtracts a given byte from the A register, subtracts the carry bit, and stores the result in A. -- 7 cycles -- */
void SBI(CPU *cpu, uint8_t imm) {
	uint16_t result = cpu->a - imm - cpu->flags.cy;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);

	uint8_t ac_check = (cpu->a & 0x0f) + (two_comp(imm + cpu->flags.cy) & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;

	set_cy(&cpu->flags, result);

	cpu->a = (uint8_t)result;
}

/* CPI - compare immediate. compares the given byte with register A, sets the zero and carry flags to indicate the result. -- 7 cycles -- */
void CPI(CPU *cpu, uint8_t imm) {
	/* this is implemented internally by subtracting the given byte from A, and so it sets all flags. this looks the same as the implementation for SBI, but without setting A. */
	uint16_t result = cpu->a - imm;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);
	set_cy(&cpu->flags, result);

	/* set ac */
	uint8_t ac_check = (cpu->a & 0x0f) + (two_comp(imm) & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;
}

/* ANI - and immediate. takes the bitwise and of register A and a given byte, stores the result in A. -- 7 cycles -- */
void ANI(CPU *cpu, uint8_t imm) {
	cpu->a = cpu->a & imm;
	set_z(&cpu->flags, cpu->a);
	set_s(&cpu->flags, cpu->a);
	set_p(&cpu->flags, cpu->a);
	cpu->flags.cy = 0;
	cpu->flags.ac = 0;
}

/* ORI - or immediate. takes the bitwise or of register A and a given byte, stores the result in A. -- 7 cycles -- */
void ORI(CPU *cpu, uint8_t imm) {
	cpu->a = cpu->a | imm;
	set_z(&cpu->flags, cpu->a);
	set_s(&cpu->flags, cpu->a);
	set_p(&cpu->flags, cpu->a);
	cpu->flags.cy = 0;
	cpu->flags.ac = 0;
}


/* XRI - xor immediate. takes the bitwise xor of register A and a given byte, stores the result in A. -- 7 cycles -- */
void XRI(CPU *cpu, uint8_t imm) {
	cpu->a = cpu->a ^ imm;
	set_z(&cpu->flags, cpu->a);
	set_s(&cpu->flags, cpu->a);
	set_p(&cpu->flags, cpu->a);
	cpu->flags.cy = 0;
	cpu->flags.ac = 0;
}

/* MOV - move. copies the contents of the second register into the first. -- 5 cycle for A-L, 7 cycles for M (in either operand) -- */
void MOV(CPU *cpu, uint8_t *mem, char dreg, char sreg) {
	uint8_t *source = get_register(cpu, mem, sreg);
	uint8_t *dest = get_register(cpu, mem, dreg);

	*dest = *source;
}

/* ADD - add. adds the value in the given register to the value in register A, stores the result in A. -- 4 cycles for A-L, 7 cycles for M -- */
void ADD(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	uint16_t result = cpu->a + *r;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);
	set_cy(&cpu->flags, result);

	/* set ac */
	uint8_t ac_check = (cpu->a & 0x0f) + (*r & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;
	
	cpu->a = (uint8_t)result;
}

/* SUB - subtract. subtracts the value in the given register from the value in register A, and stores the result in A. -- 4 cycles for A-L, 7 cycles for M -- */
void SUB(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	uint16_t result = cpu->a - *r;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);
	set_cy(&cpu->flags, result);

	/* set ac */
	uint8_t ac_check = (cpu->a & 0x0f) + (two_comp(*r) & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;

	cpu->a = (uint8_t)result;
}

/* ADC - add with carry. adds the value in the given register to the value in register A, adds 1 if carry is set, and stores the result in A. -- 4 cycles for A-L, 7 cycles for M -- */
void ADC(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	uint16_t result = cpu->a + *r + cpu->flags.cy;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);

	uint8_t ac_check = (cpu->a & 0x0f) + (*r & 0x0f) + cpu->flags.cy;
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;

	set_cy(&cpu->flags, result);

	cpu->a = (uint8_t)result;
}

/* SBB - subtract with borrow. subtracts the value in the given register to the value in register A, subtracts 1 if carry is set, and stores the result in A. -- 4 cycles for A-L, 7 cycles for M -- */
void SBB(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	uint16_t result = cpu->a - *r - cpu->flags.cy;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);

	uint8_t ac_check = (cpu->a & 0x0f) + (two_comp(*r + cpu->flags.cy) & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;

	set_cy(&cpu->flags, result);

	cpu->a = (uint8_t)result;
}

/* CMP - compare. compares the given register with register A, and sets the zero and carry flags to indicate the result. -- 4 cycles for A-L, 7 cycles for M -- */
void CMP(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	/* this is implemented internally by subtracting the given register from A, and so it sets all flags. this looks the same as the implementation for SUB, but without setting A. */
	uint16_t result = cpu->a - *r;
	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);
	set_cy(&cpu->flags, result);

	/* set ac */
	uint8_t ac_check = (cpu->a & 0x0f) + (two_comp(*r) & 0x0f);
	if((ac_check & 0x10) == 0x10)
		cpu->flags.ac = 1;
	else
		cpu->flags.ac = 0;
}

/* ANA - bitwise AND. takes the bitwise AND of the given register and register A, stores the result in register A. -- 4 cycles for A-L, 7 cycles for M -- */
void ANA(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	cpu->a = cpu->a & *r;
	set_z(&cpu->flags, cpu->a);
	set_s(&cpu->flags, cpu->a);
	set_p(&cpu->flags, cpu->a);
	cpu->flags.cy = 0;
	cpu->flags.ac = 0;
}

/* ORA - bitwise OR. takes the bitwise OR of the given register and register A, stores the result in register A. -- 4 cycles for A-L, 7 cycles for M -- */
void ORA(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	cpu->a = cpu->a | *r;
	set_z(&cpu->flags, cpu->a);
	set_s(&cpu->flags, cpu->a);
	set_p(&cpu->flags, cpu->a);
	cpu->flags.cy = 0;
	cpu->flags.ac = 0;
}

/* XRA - bitwise XOR. takes the bitwise XOR of the given register and register A, stores the result in register A. -- 4 cycles for A-L, 7 cycles for M -- */
void XRA(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t *r = get_register(cpu, mem, reg);

	cpu->a = cpu-> a ^ *r;
	set_z(&cpu->flags, cpu->a);
	set_s(&cpu->flags, cpu->a);
	set_p(&cpu->flags, cpu->a);
	cpu->flags.cy = 0;
	cpu->flags.ac = 0;
}

/* CMA - accumulator complement. register A becomes the bitwise negation of A. -- 4 cycles -- */
void CMA(CPU *cpu) {
	cpu->a = ~cpu->a;
}

/* RLC - left shift (rotate) with carry. left shift on register A with bit wrapping. -- 4 cycles -- */
void RLC(CPU *cpu) {
	uint8_t high_bit = (cpu->a & 0x80) >> 7;
	cpu->flags.cy = high_bit;
	cpu->a = (cpu->a << 1) | high_bit;
}

/* RRC - right shift (rotate) with carry. right shift on register A with bit wrapping. -- 4 cycles -- */
void RRC(CPU *cpu) {
	uint8_t low_bit = (cpu->a & 0x01);
	cpu->flags.cy = low_bit;
	cpu->a = (cpu->a >> 1) | (low_bit << 7);
}

/* RAL - left shift (rotate) through carry. treats carry as a 9th (high) bit of the accumulator. -- 4 cycles -- */
void RAL(CPU *cpu) {
	uint8_t high_bit = (cpu->a & 0x80) >> 7;
	cpu->a = (cpu->a << 1) | cpu->flags.cy;
	cpu->flags.cy = high_bit;
}

/* RAR - right shift (rotate) through carry. treats carry as a 9th (low) bit of the accumulator. -- 4 cycles -- */
void RAR(CPU *cpu) {
	uint8_t low_bit = (cpu->a & 0x01);
	cpu->a = (cpu->a >> 1) | (cpu->flags.cy << 7);
	cpu->flags.cy = low_bit;
}

/* PUSH - push on to stack. pushes the given register pair on to the stack. -- 11 cycles -- */
void PUSH(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t low;
	uint8_t high;

	switch(reg) {
		case 'B':
			low = cpu->c;
			high = cpu->b;
			break;
		case 'D':
			low = cpu->e;
			high = cpu->d;
			break;
		case 'H':
			low = cpu->l;
			high = cpu->h;
			break;
		case 'P': /* Program Status Word - special handling. low = PSW, since you can't declare new variables in a switch statement */
			low = 0x02 | (cpu->flags.s << 7) | (cpu->flags.z << 6) | (cpu->flags.ac << 4) | (cpu->flags.p << 2) | cpu->flags.cy; /* starts with 0x02, because the second bit is always set */
			high = cpu->a;
			break;
	}

	stack_push(cpu, mem, high, low);
}

/* POP - pop off stack. pops two bytes off the stack into the given register pair. -- 10 cycles -- */
void POP(CPU *cpu, uint8_t *mem, char reg) {
	uint8_t psw; // place to store PSW, since we don't have a register to put it in. Only used in POP PSW
	uint8_t *low;
	uint8_t *high;
	
	switch(reg) {
		case 'B':
			low = &cpu->c;
			high = &cpu->b;
			break;
		case 'D':
			low = &cpu->e;
			high = &cpu->d;
			break;
		case 'H':
			low = &cpu->l;
			high = &cpu->h;
			break;
		case 'P': /* PSW - Program Status Word. special handling - again low = PSW */
			low = &psw;
			stack_pop(cpu, mem, low, &cpu->a);
			cpu->flags.cy = *low & 0x01;
			cpu->flags.p = (*low & 0x04) >> 2;
			cpu->flags.ac = (*low & 0x10) >> 4;
			cpu->flags.z = (*low & 0x40) >> 6;
			cpu->flags.s = (*low & 0x80) >> 7;
			return;
	}

	stack_pop(cpu, mem, low, high);
}

/* STA - store accumulator direct. stores the value in register A to the given memory address. -- 13 cycles -- */
void STA(CPU *cpu, uint8_t *mem, uint16_t addr) {
	mem[addr] = cpu->a;
}

/* LDA - load accumulator direct. loads the value at the given address into register A. -- 13 cycles -- */
void LDA(CPU *cpu, uint8_t *mem, uint16_t addr) {
	cpu->a = mem[addr];
}

/* STAX - store accumulator. stores the value in register A into the memory address given by the concatenation of the given register and its neighbor. -- 7 cycles -- */
void STAX(CPU *cpu, uint8_t *mem, char reg) {
	uint16_t address;

	switch(reg) {
		case 'B':
			address = to_double_word(cpu->c, cpu->b);
			break;
		case 'D':
			address = to_double_word(cpu->e, cpu->d);
			break;
	}

	mem[address] = cpu->a;
}

/* LDAX - load accumulator. Loads the value stored at the memory address given by the register pair into register A. -- 7 cycles -- */
void LDAX(CPU *cpu, uint8_t *mem, char reg) {
	uint16_t address;

	switch(reg) {
		case 'B':
			address = to_double_word(cpu->c, cpu->b);
			break;
		case 'D':
			address = to_double_word(cpu->e, cpu->d);
			break;
	}

	cpu->a = mem[address];
}

/* JMP - jump. program resumes execution at the given address. -- 10 cycles -- */
void JMP(CPU *cpu, uint16_t addr) {
	cpu->pc = addr;
}

/* JZ - jump if zero. If zero bit is set, calls JMP. -- 10 cycles -- */
void JZ(CPU *cpu, uint16_t addr) {
	if(cpu->flags.z == 1) {
		cpu->pc = addr;
	}
}

/* JNZ - jump if not zero. If zero bit is not set, calls JMP. -- 10 cycles -- */
void JNZ(CPU *cpu, uint16_t addr) {
	if(cpu->flags.z == 0) {
		cpu->pc = addr;
	}
}

/* JM - jump if negative. If sign bit is set, calls JMP. -- 10 cycles -- */
void JM(CPU *cpu, uint16_t addr) {
	if(cpu->flags.s == 1) {
		cpu->pc = addr;
	}
}

/* JP - jump if positive. If sign bit is not set, calls JMP. -- 10 cycles -- */
void JP(CPU *cpu, uint16_t addr) {
	if(cpu->flags.s == 0) {
		cpu->pc = addr;
	}
}

/* JPE - jump if parity even. If parity bit is set, calls JMP. -- 10 cycles -- */
void JPE(CPU *cpu, uint16_t addr) {
	if(cpu->flags.p == 1) {
		cpu->pc = addr;
	}
}

/* JPO - jump if parity odd. If parity bit is not set, calls JMP. -- 10 cycles -- */
void JPO(CPU *cpu, uint16_t addr) {
	if(cpu->flags.p == 0) {
		cpu->pc = addr;
	}
}

/* JC - jump if carry. If carry bit is set, calls JMP. -- 10 cycles -- */
void JC(CPU *cpu, uint16_t addr) {
	if(cpu->flags.cy == 1) {
		cpu->pc = addr;
	}
}

/* JNC - jump if no carry. If carry bit is not set, calls JMP. -- 10 cycles -- */
void JNC(CPU *cpu, uint16_t addr) {
	if(cpu->flags.cy == 0) {
		cpu->pc = addr;
	}
}

/* CALL - call. Pushes the program counter onto the stack as a return address, then resumes excution at the given address. -- 17 cycles -- */
void CALL(CPU *cpu, uint8_t *mem, uint16_t addr) {
	call(cpu, mem, addr);
}

/* CZ - call if zero. If zero bit is set, calls CALL. -- 11 cycles if zero bit not set, 17 cycles otherwise -- */
void CZ(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.z == 1) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* CNZ - call if not zero. If zero bit is not set, calls CALL. -- 11 cycles if zero bit set, 17 cycles if not -- */
void CNZ(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.z == 0) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* CM - call if negative. If sign bit is set, calls CALL. -- 11 cycles if sign bit not set, 17 cycles otherwise -- */
void CM(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.s == 1) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* CP - call if positive. If sign bit is not set, calls CALL. -- 11 cycles if sign bit set, 17 cycles if not -- */
void CP(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.s == 0) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* CPE - call if parity even. If parity bit is set, calls CALL. -- 11 cycles if parity bit not set, 17 cycles otherwise -- */
void CPE(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.p == 1) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* CPO - call if parity odd. If parity bit is not set, calls CALL. -- 11 cycles if parity bit set, 17 cycles if not -- */
void CPO(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.p == 0) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* CC - call if carry. If carry bit is set, calls CALL. -- 11 cycles if carry bit not set, 17 cycles otherwise -- */
void CC(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.cy == 1) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* CNC - call if no carry. If carry bit is not set, calls CALL. -- 11 cycles if carry bit set, 17 cycles if not -- */
void CNC(CPU *cpu, uint8_t *mem, uint16_t addr) {
	if(cpu->flags.cy == 0) {
		call(cpu, mem, addr);
	}
	else {
		cycle_override = 11;
	}
}

/* RET - return. Pops two bytes off the stack into the program counter, resuming execution at that address. -- 10 cycles -- */
void RET(CPU *cpu, uint8_t *mem) {
	ret(cpu, mem);
}

/* RZ - return if zero. If zero bit is set, calls RET. -- 5 cycles if zero bit not set, 11 cycles otherwise -- */
void RZ(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.z == 1) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RNZ - return if not zero. If zero bit is not set, calls RET. -- 5 cycles if zero bit set, 11 cycles if not -- */
void RNZ(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.z == 0) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RM - return if negative. If sign bit is set, calls RET. -- 5 cycles if sign bit not set, 11 cycles otherwise -- */
void RM(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.s == 1) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RP - return if positive. If sign bit is not set, calls RET. -- 5 cycles if sign bit set, 11 cycles if not -- */
void RP(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.s == 0) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RPE - return if parity even. If parity bit is set, calls RET. -- 5 cycles if parity bit not set, 11 cycles otherwise -- */
void RPE(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.p == 1) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RPO - return if parity odd. If parity bit is not set, calls RET. -- 5 cycles if parity bit set, 11 cycles if not -- */
void RPO(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.p == 0) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RC -- return if carry. If carry bit is set, calls RET. -- 5 cycles if carry bit not set, 11 cycles otherwise -- */
void RC(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.cy == 1) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RNC -- return if no carry. If carry bit is not set, calls RET. -- 5 cycles if carry bit set, 11 cycles if not -- */
void RNC(CPU *cpu, uint8_t *mem) {
	if(cpu->flags.cy == 0) {
		ret(cpu, mem);
	}
	else {
		cycle_override = 5;
	}
}

/* RST - restart/reset. CALLs a special memory address, located at or near the beginning of the program. Used for handling interrupts from peripheral devices. -- 11 cycles -- */
void RST(CPU *cpu, uint8_t *mem, uint8_t code) {
	call(cpu, mem, code << 3);
}

/* SHLD - store H and L. Stores register L at the address specified, and H at address+1. -- 16 cycles -- */
void SHLD(CPU *cpu, uint8_t *mem, uint16_t addr) {
	mem[addr] = cpu->l;
	mem[addr+1] = cpu->h;
}

/* LHLD - load H and L. Loads register L from the address specified, and H from address+1. -- 16 cycles -- */
void LHLD(CPU *cpu, uint8_t *mem, uint16_t addr) {
	cpu->l = mem[addr];
	cpu->h = mem[addr+1];
}

/* XTHL - exchange H and L with stack. swaps registers H and L with the top two bytes of the stack. -- 18 cycles -- */
void XTHL(CPU *cpu, uint8_t *mem) {
	uint8_t reg_l = cpu->l;
	uint8_t reg_h = cpu->h;
	cpu->l = mem[cpu->sp];
	cpu->h = mem[cpu->sp+1];
	mem[cpu->sp] = reg_l;
	mem[cpu->sp+1] = reg_h;
}

/* XCHG - exchange H and L with D and E. swaps the values of registers H and D, and registers L and E. -- 4 cycles -- */
void XCHG(CPU *cpu) {
	uint8_t reg_l = cpu->l;
	uint8_t reg_h = cpu->h;
	cpu->l = cpu->e;
	cpu->h = cpu->d;
	cpu->e = reg_l;
	cpu->d = reg_h;
}

/* SPHL - move H and L to stack pointer. does what it says on the tin. -- 5 cycles -- */
void SPHL(CPU *cpu) {
	cpu->sp = to_double_word(cpu->l, cpu->h);
}

/* PCHL - move H and L to program counter. does what it says on the tin. -- 5 cycles -- */
void PCHL(CPU *cpu) {
	cpu->pc = to_double_word(cpu->l, cpu->h);
}

/* STC - set carry. Sets the carry bit to 1. -- 4 cycles -- */
void STC(CPU *cpu) {
	cpu->flags.cy = 1;
}

/* CMC - complement carry. Sets carry bit to its bitwise negation. -- 4 cycles -- */
void CMC(CPU *cpu) {
	cpu->flags.cy = cpu->flags.cy == 0 ? 1 : 0;
}

/* DAA - decimal adjust accumulator. Adjusts the value in the accumulator after a BCD operation to be a correct binary coded decimal value (each half-bye is a value in 0-9). -- 4 cycles -- */
void DAA(CPU *cpu) {
	uint8_t low_digit = cpu->a & 0x0f;

	if(low_digit > 9 || cpu->flags.ac == 1) {
		low_digit += 6;
		if((low_digit & 0x10) == 0x10)
			cpu->flags.ac = 1;
		else
			cpu->flags.ac = 0;
	}

	uint16_t result = (cpu->a & 0xf0) + low_digit;
	uint8_t high_digit = (result & 0x00f0) >> 4;

	if(high_digit > 9 || cpu->flags.cy == 1) {
		high_digit += 6;
		if((high_digit & 0x10) == 0x10)
			cpu->flags.cy = 1;
		else
			cpu->flags.cy = 0;
	}

	result = (result & 0xff0f) + (high_digit << 4);
	cpu->a = (uint8_t)result;

	set_z(&cpu->flags, result);
	set_s(&cpu->flags, result);
	set_p(&cpu->flags, result);
}

/* IN - input. reads 8 bits of data from the specified port into the A register. -- 10 cycles -- */
void IN(CPU *cpu, uint8_t port) {
	cpu->a = read_port(port);
}

/* OUT - output. sends the value of the A register to the specified port. -- 10 cycles -- */
void OUT(CPU *cpu, uint8_t port) {
	write_port(port, cpu->a);
}

/* RIM - read interrupt mask. Fills register A with interrupt information. (1 - Serial input data bit)(3 - pending interrupts)(1 - interrupts enabled)(3 - interrupt masks) -- 4 cycles -- */
void RIM(CPU *cpu) {
	/* TODO: implement this, once input/output and interrupts have been added. */
	/* This instruction is only availble on the 8085 processor model. As such, it may remain unimplemented. */
	fprintf(stderr, "RIM has not yet been implemented. Aborting.\n");
	exit(1);
}

/* SIM - set interrupt mask. Uses the contents of A to set interrupt information. (3 - interrupt masks)(1 - enable setting of masks)(1 - reset RST 7.5 flip flop)(1 - ignored)(1 - output final bit)(1 - serial output data bit) -- 4 cycles -- */
void SIM(CPU *cpu) {
	/* TODO: implement this, once input/output and interrupts have been added. */
	/* This instruction is only availble on the 8085 processor model. As such, it may remain unimplemented. */
	fprintf(stderr, "SIM has not yet been implemented. Aborting.\n");
	exit(1);
}

/* DI - disable interrupts. does what it says on the tin. -- 4 cycles -- */
void DI(CPU *cpu, Interrupt *interrupts) {
	disable_interrupts(interrupts);
	//cpu->inte = 0;
}

/* EI - enable interrupts. does what it says on the tin. -- 4 cycles -- */
void EI(CPU *cpu, Interrupt *interrupts) {
	enable_interrupts(interrupts);
	//cpu->inte = 1;
}

/* HLT - halts the processor after the current cycle. PC has the address of the next sequential operation, but it will not get executed. The processor can only be restarted via interrupt or RESET. -- 7 cycles */
void HLT(CPU *cpu) {
	cpu->halted = 1;
}

void set_z(Flags *flags, uint16_t result) {
	if((result & 0x00ff) == 0) {
		flags->z = 1;
	}
	else {
		flags->z = 0;
	}
}

void set_s(Flags *flags, uint16_t result) {
	if((result & 0x0080) == 0x80) {
		flags->s = 1;
	}
	else {
		flags->s = 0;
	}
}

void set_p(Flags *flags, uint16_t result) {
	if((result & 0x0001) == 0) {
		flags->p = 1;
	}
	else {
		flags->p = 0;
	}
}

void set_cy(Flags *flags, uint16_t result) {
	if((result & 0x0100) == 0x0100) {
		flags->cy = 1;
	}
	else {
		flags->cy = 0;
	}
}

void unimplemented(CPU *cpu, uint8_t opcode) {
	fprintf(stderr, "Opcode %x has not been implemented. Aborting.\n", opcode);
	exit(1);
}

void initializeOpLengths(uint8_t *lengths) {
	int i;
	for(i = 0; i < NUM_OF_OPCODES; ++i) {
		lengths[i] = 1;
	}
	lengths[0x01] =  3;
	lengths[0x06] = 2;
	lengths[0x0e] = 2;
	lengths[0x11] =  3;
	lengths[0x16] = 2;
	lengths[0x1e] = 2;
	lengths[0x21] =  3;
	lengths[0x22] =  3;
	lengths[0x26] = 2;
	lengths[0x2a] =  3;
	lengths[0x2e] = 2;
	lengths[0x31] =  3;
	lengths[0x32] =  3;
	lengths[0x36] = 2;
	lengths[0x3a] =  3;
	lengths[0x3e] = 2;
	lengths[0xc2] =  3;
	lengths[0xc3] =  3;
	lengths[0xc4] =  3;
	lengths[0xc6] = 2;
	lengths[0xca] =  3;
	lengths[0xcc] =  3;
	lengths[0xcd] =  3;
	lengths[0xce] = 2;
	lengths[0xd2] =  3;
	lengths[0xd3] = 2;
	lengths[0xd4] =  3;
	lengths[0xd6] = 2;
	lengths[0xda] =  3;
	lengths[0xdb] = 2;
	lengths[0xdc] =  3;
	lengths[0xde] = 2;
	lengths[0xe2] =  3;
	lengths[0xe4] =  3;
	lengths[0xe6] = 2;
	lengths[0xea] =  3;
	lengths[0xec] =  3;
	lengths[0xee] = 2;
	lengths[0xf2] =  3;
	lengths[0xf4] =  3;
	lengths[0xf6] = 2;
	lengths[0xfa] =  3;
	lengths[0xfc] =  3;
	lengths[0xfe] = 2;
}

void initializeOpCycles(uint8_t *cycles) {
	int i;
	for(i = 0; i < NUM_OF_OPCODES; ++i) {
		cycles[i] = 255; // a placeholder value. Check for it in the CPU loop, if you see it, you done fucked up
	}
	// NOP
	cycles[0x00] = 4;
	// MVI
	cycles[0x06] =  7;
	cycles[0x0e] =  7;
	cycles[0x16] =  7;
	cycles[0x1e] =  7;
	cycles[0x26] =  7;
	cycles[0x2e] =  7;
	cycles[0x36] =   10;
	cycles[0x3e] =  7;
	// LXI
	cycles[0x01] =   10;
	cycles[0x11] =   10;
	cycles[0x21] =   10;
	cycles[0x31] =   10;
	// INR
	cycles[0x04] = 5;
	cycles[0x0c] = 5;
	cycles[0x14] = 5;
	cycles[0x1c] = 5;
	cycles[0x24] = 5;
	cycles[0x2c] = 5;
	cycles[0x34] =   10;
	cycles[0x3c] = 5;
	// DCR
	cycles[0x05] = 5;
	cycles[0x0d] = 5;
	cycles[0x15] = 5;
	cycles[0x1d] = 5;
	cycles[0x25] = 5;
	cycles[0x2d] = 5;
	cycles[0x35] =   10;
	cycles[0x3d] = 5;
	// INX
	cycles[0x03] = 5;
	cycles[0x13] = 5;
	cycles[0x23] = 5;
	cycles[0x33] = 5;
	// DCX
	cycles[0x0b] = 5;
	cycles[0x1b] = 5;
	cycles[0x2b] = 5;
	cycles[0x3b] = 5;
	// DAD
	cycles[0x09] =   10;
	cycles[0x19] =   10;
	cycles[0x29] =   10;
	cycles[0x39] =   10;
	// ADI
	cycles[0xc6] =  7;
	// SUI
	cycles[0xd6] =  7;
	// ACI
	cycles[0xce] =  7;
	// SBI
	cycles[0xde] =  7;
	// CPI
	cycles[0xfe] =  7;
	// ANI
	cycles[0xe6] =  7;
	// ORI
	cycles[0xf6] =  7;
	// XRI
	cycles[0xee] =  7;
	// MOV
	cycles[0x40] = 5;
	cycles[0x41] = 5;
	cycles[0x42] = 5;
	cycles[0x43] = 5;
	cycles[0x44] = 5;
	cycles[0x45] = 5;
	cycles[0x46] =  7;
	cycles[0x47] = 5;
	cycles[0x48] = 5;
	cycles[0x49] = 5;
	cycles[0x4a] = 5;
	cycles[0x4b] = 5;
	cycles[0x4c] = 5;
	cycles[0x4d] = 5;
	cycles[0x4e] =  7;
	cycles[0x4f] = 5;
	cycles[0x50] = 5;
	cycles[0x51] = 5;
	cycles[0x52] = 5;
	cycles[0x53] = 5;
	cycles[0x54] = 5;
	cycles[0x55] = 5;
	cycles[0x56] =  7;
	cycles[0x57] = 5;
	cycles[0x58] = 5;
	cycles[0x59] = 5;
	cycles[0x5a] = 5;
	cycles[0x5b] = 5;
	cycles[0x5c] = 5;
	cycles[0x5d] = 5;
	cycles[0x5e] =  7;
	cycles[0x5f] = 5;
	cycles[0x60] = 5;
	cycles[0x61] = 5;
	cycles[0x62] = 5;
	cycles[0x63] = 5;
	cycles[0x64] = 5;
	cycles[0x65] = 5;
	cycles[0x66] =  7;
	cycles[0x67] = 5;
	cycles[0x68] = 5;
	cycles[0x69] = 5;
	cycles[0x6a] = 5;
	cycles[0x6b] = 5;
	cycles[0x6c] = 5;
	cycles[0x6d] = 5;
	cycles[0x6e] =  7;
	cycles[0x6f] = 5;
	cycles[0x70] =  7;
	cycles[0x71] =  7;
	cycles[0x72] =  7;
	cycles[0x73] =  7;
	cycles[0x74] =  7;
	cycles[0x75] =  7;
	cycles[0x77] =  7;
	cycles[0x78] = 5;
	cycles[0x79] = 5;
	cycles[0x7a] = 5;
	cycles[0x7b] = 5;
	cycles[0x7c] = 5;
	cycles[0x7d] = 5;
	cycles[0x7e] =  7;
	cycles[0x7f] = 5;
	// ADD
	cycles[0x80] = 4;
	cycles[0x81] = 4;
	cycles[0x82] = 4;
	cycles[0x83] = 4;
	cycles[0x84] = 4;
	cycles[0x85] = 4;
	cycles[0x86] =  7;
	cycles[0x87] = 4;
	// SUB
	cycles[0x90] = 4;
	cycles[0x91] = 4;
	cycles[0x92] = 4;
	cycles[0x93] = 4;
	cycles[0x94] = 4;
	cycles[0x95] = 4;
	cycles[0x96] =  7;
	cycles[0x97] = 4;
	// ADC
	cycles[0x88] = 4;
	cycles[0x89] = 4;
	cycles[0x8a] = 4;
	cycles[0x8b] = 4;
	cycles[0x8c] = 4;
	cycles[0x8d] = 4;
	cycles[0x8e] =  7;
	cycles[0x8f] = 4;
	// SBB
	cycles[0x98] = 4;
	cycles[0x99] = 4;
	cycles[0x9a] = 4;
	cycles[0x9b] = 4;
	cycles[0x9c] = 4;
	cycles[0x9d] = 4;
	cycles[0x9e] =  7;
	cycles[0x9f] = 4;
	// CMP
	cycles[0xb8] = 4;
	cycles[0xb9] = 4;
	cycles[0xba] = 4;
	cycles[0xbb] = 4;
	cycles[0xbc] = 4;
	cycles[0xbd] = 4;
	cycles[0xbe] =  7;
	cycles[0xbf] = 4;
	// ANA
	cycles[0xa0] = 4;
	cycles[0xa1] = 4;
	cycles[0xa2] = 4;
	cycles[0xa3] = 4;
	cycles[0xa4] = 4;
	cycles[0xa5] = 4;
	cycles[0xa6] =  7;
	cycles[0xa7] = 4;
	// ORA
	cycles[0xb0] = 4;
	cycles[0xb1] = 4;
	cycles[0xb2] = 4;
	cycles[0xb3] = 4;
	cycles[0xb4] = 4;
	cycles[0xb5] = 4;
	cycles[0xb6] =  7;
	cycles[0xb7] = 4;
	// XRA
	cycles[0xa8] = 4;
	cycles[0xa9] = 4;
	cycles[0xaa] = 4;
	cycles[0xab] = 4;
	cycles[0xac] = 4;
	cycles[0xad] = 4;
	cycles[0xae] =  7;
	cycles[0xaf] = 4;
	// CMA
	cycles[0x2f] = 4;
	// RLC
	cycles[0x07] = 4;
	// RRC
	cycles[0x0f] = 4;
	// RAL
	cycles[0x17] = 4;
	// RAR
	cycles[0x1f] = 4;
	// PUSH
	cycles[0xc5] =   11;
	cycles[0xd5] =   11;
	cycles[0xe5] =   11;
	cycles[0xf5] =   11;
	// POP
	cycles[0xc1] =   10;
	cycles[0xd1] =   10;
	cycles[0xe1] =   10;
	cycles[0xf1] =   10;
	// STA
	cycles[0x32] =    13;
	// LDA
	cycles[0x3a] =    13;
	// STAX
	cycles[0x02] =  7;
	cycles[0x12] =  7;
	// LDAX
	cycles[0x0a] =  7;
	cycles[0x1a] =  7;
	// JMP
	cycles[0xc3] =   10;
	// JZ
	cycles[0xca] =   10;
	// JNZ
	cycles[0xc2] =   10;
	// JM
	cycles[0xfa] =   10;
	// JP
	cycles[0xf2] =   10;
	// JPE
	cycles[0xea] =   10;
	// JPO
	cycles[0xe2] =   10;
	// JC
	cycles[0xda] =   10;
	// JNC
	cycles[0xd2] =   10;
	// CALL
	cycles[0xcd] =     17;
	// CZ
	cycles[0xcc] =     17; // 11 if zero bit not set. uses override
	// CNZ
	cycles[0xc4] =     17; // 11 if zero bit set. uses override
	// CM
	cycles[0xfc] =     17; // 11 if sign bit not set. uses override
	// CP
	cycles[0xf4] =     17; // 11 if sign bit set. uses override
	// CPE
	cycles[0xec] =     17; // 11 if parity bit not set. uses override
	// CPO
	cycles[0xe4] =     17; // 11 if parity bit set. uses override
	// CC
	cycles[0xdc] =     17; // 11 if carry bit not set. uses override
	// CNC
	cycles[0xd4] =     17; // 11 if carry bit set. uses override
	// RET
	cycles[0xc9] =   10;
	// RZ
	cycles[0xc8] =   11; // 5 if zero bit not set. uses override
	// RNZ
	cycles[0xc0] =   11; // 5 if zero bit set. uses override
	// RM
	cycles[0xf8] =   11; // 5 if sign bit not set. uses override.
	// RP
	cycles[0xf0] =   11; // 5 if sign bit set. uses override
	// RPE
	cycles[0xe8] =   11; // 5 if parity bit not set. uses override
	// RPO
	cycles[0xe0] =   11; // 5 if parity bit set. uses override
	// RC
	cycles[0xd8] =   11; // 5 if carry bit not set. uses override
	// RNC
	cycles[0xd0] =   11; // 5 if carry bit set. uses override
	// RST
	cycles[0xc7] =   11;
	cycles[0xcf] =   11;
	cycles[0xd7] =   11;
	cycles[0xdf] =   11;
	cycles[0xe7] =   11;
	cycles[0xef] =   11;
	cycles[0xf7] =   11;
	cycles[0xff] =   11;
	// SHLD
	cycles[0x22] =     16;
	// LHLD
	cycles[0x2a] =     16;
	// XTHL
	cycles[0xe3] =     18;
	// XCHG
	cycles[0xeb] = 4;
	// SPHL
	cycles[0xf9] = 5;
	// PCHL
	cycles[0xe9] = 5;
	// STC
	cycles[0x37] = 4;
	// CMC
	cycles[0x3f] = 4;
	// DAA
	cycles[0x27] = 4;
	// IN
	cycles[0xdb] =   10;
	// OUT
	cycles[0xd3] =   10;
	// RIM
	cycles[0x20] = 4;
	// SIM
	cycles[0x30] = 4;
	// DI
	cycles[0xf3] = 4;
	// EI
	cycles[0xfb] = 4;
	// HLT
	cycles[0x76] =  7;
}

uint16_t to_double_word(uint8_t low, uint8_t high) {
	uint16_t dword = high;
	dword = high << 8;
	dword = dword | low;
	return dword;
}

void from_double_word(uint16_t dword, uint8_t *low, uint8_t *high) {
	*low = dword & 0x00ff;
	*high = (dword >> 8) & 0x00ff;
}

void stack_push(CPU *cpu, uint8_t *mem, uint8_t byte1, uint8_t byte2) {
	mem[cpu->sp - 1] = byte1;
	mem[cpu->sp - 2] = byte2;
	cpu->sp = cpu->sp - 2;
}

void stack_pop(CPU *cpu, uint8_t *mem, uint8_t *byte1, uint8_t *byte2) {
	*byte1 = mem[cpu->sp];
	*byte2 = mem[cpu->sp + 1];
	cpu->sp = cpu->sp + 2;
}

void call(CPU *cpu, uint8_t *mem, uint16_t addr) {
	uint8_t low;
	uint8_t high;
	from_double_word(cpu->pc, &low, &high);
	stack_push(cpu, mem, high, low);
	cpu->pc = addr;
}

void ret(CPU *cpu, uint8_t *mem) {
	uint8_t low;
	uint8_t high;
	stack_pop(cpu, mem, &low, &high);
	cpu->pc = to_double_word(low, high);
}

uint8_t *get_register(CPU *cpu, uint8_t *mem, char reg) {
	switch(reg) {
		case 'B': return &cpu->b;
		case 'C': return &cpu->c;
		case 'D': return &cpu->d;
		case 'E': return &cpu->e;
		case 'H': return &cpu->h;
		case 'L': return &cpu->l;
		case 'M':
			return &mem[to_double_word(cpu->l, cpu->h)]; /* the M register is the memory location addressed by H and L */
		case 'A': return &cpu->a;
		default: return 0;
	}
}

uint8_t two_comp(uint8_t i) {
	return ~i + 1;
}
