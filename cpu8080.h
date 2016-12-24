#ifndef E101_CPU8080
#define E101_CPU8080

#include <stdint.h>

typedef struct { /* z:1 means z is exactly 1 bit. z, and the following five registers, will be packed into a single byte as they only take up 8 bits total. */
	uint8_t z:1;   /* ZERO - did the last instruction equal 0? */
	uint8_t s:1;   /* SIGN - did the last instruction set bit 7? (this means it's negative - IF using signed integers.) */
	uint8_t p:1;   /* PARITY - did the last instruction return an even value? */
	uint8_t cy:1;  /* CARRY - 1 if the previous operation resulted in overflow. 0 otherwise. I think. */
	uint8_t ac:1;  /* AUX CARRY - used for BCD (binary coded decimal) math. 1 if the first half-byte overflowed, 0 otherwise. */
	uint8_t pad:3; /* padding - unused */
} Flags;

typedef struct {
	uint8_t  b;  /* B & C - multi-purpose register pair */
	uint8_t  c;
	uint8_t  d;  /* D & E - multi-purpose register pair */
	uint8_t  e;
	uint8_t  h;  /* H & L - multi-purpose register pair, used particularly for memory addresses by convention */
	uint8_t  l;
	uint8_t  a;  /* ACCUMULATOR - stores the result of arithmetic operations */
	uint16_t sp; /* STACK POINTER - address of top of stack */
	uint16_t pc; /* PROGRAM COUNTER - address of current instruction */
	Flags    flags;
	uint8_t  int_enable; /* are interrupts enabled? */
} CPU;
/* M - refers to the memory contents at (HL) */
/* PSW (Program Status Word) - refers to A and FLAGS as a two-byte pair */

void initializeCPU(CPU *cpu);
void printCPU(CPU *cpu, uint8_t *mem);

int emulate(CPU *cpu, uint8_t *mem);

/* operations */

void unimplemented(CPU *cpu, uint8_t opcode);
void NOP(CPU *cpu);

void MVI(CPU *cpu, uint8_t *mem, char reg, uint8_t imm);
void LXI(CPU *cpu, char reg, uint16_t imm);

void INR(CPU *cpu, uint8_t *mem, char reg);
void DCR(CPU *cpu, uint8_t *mem, char reg);

void INX(CPU *cpu, char reg);
void DCX(CPU *cpu, char reg);

void DAD(CPU *cpu, char reg);

void ADI(CPU *cpu, uint8_t imm);
void SUI(CPU *cpu, uint8_t imm);
void ACI(CPU *cpu, uint8_t imm);
void SBI(CPU *cpu, uint8_t imm);

void CPI(CPU *cpu, uint8_t imm);

void ANI(CPU *cpu, uint8_t imm);
void ORI(CPU *cpu, uint8_t imm);
void XRI(CPU *cpu, uint8_t imm);

void MOV(CPU *cpu, uint8_t *mem, char dreg, char sreg);

void ADD(CPU *cpu, uint8_t *mem, char reg);
void SUB(CPU *cpu, uint8_t *mem, char reg);
void ADC(CPU *cpu, uint8_t *mem, char reg);
void SBB(CPU *cpu, uint8_t *mem, char reg);

void CMP(CPU *cpu, uint8_t *mem, char reg);

void ANA(CPU *cpu, uint8_t *mem, char reg);
void ORA(CPU *cpu, uint8_t *mem, char reg);
void XRA(CPU *cpu, uint8_t *mem, char reg);
void CMA(CPU *cpu);
void RLC(CPU *cpu);
void RRC(CPU *cpu);
void RAL(CPU *cpu);
void RAR(CPU *cpu);

void PUSH(CPU *cpu, uint8_t *mem, char reg);
void POP(CPU *cpu, uint8_t *mem, char reg);

void STA(CPU *cpu, uint8_t *mem, uint16_t addr);
void LDA(CPU *cpu, uint8_t *mem, uint16_t addr);

void STAX(CPU *cpu, uint8_t *mem, char reg);
void LDAX(CPU *cpu, uint8_t *mem, char reg);

void JMP(CPU *cpu, uint16_t addr);
void JZ(CPU *cpu, uint16_t addr);
void JNZ(CPU *cpu, uint16_t addr);
void JM(CPU *cpu, uint16_t addr);
void JP(CPU *cpu, uint16_t addr);
void JPE(CPU *cpu, uint16_t addr);
void JPO(CPU *cpu, uint16_t addr);
void JC(CPU *cpu, uint16_t addr);
void JNC(CPU *cpu, uint16_t addr);

void CALL(CPU *cpu, uint8_t *mem, uint16_t addr);
void CZ(CPU *cpu, uint8_t *mem, uint16_t addr);
void CNZ(CPU *cpu, uint8_t *mem, uint16_t addr);
void CM(CPU *cpu, uint8_t *mem, uint16_t addr);
void CP(CPU *cpu, uint8_t *mem, uint16_t addr);
void CPE(CPU *cpu, uint8_t *mem, uint16_t addr);
void CPO(CPU *cpu, uint8_t *mem, uint16_t addr);
void CC(CPU *cpu, uint8_t *mem, uint16_t addr);
void CNC(CPU *cpu, uint8_t *mem, uint16_t addr);

void RET(CPU *cpu, uint8_t *mem);
void RZ(CPU *cpu, uint8_t *mem);
void RNZ(CPU *cpu, uint8_t *mem);
void RM(CPU *cpu, uint8_t *mem);
void RP(CPU *cpu, uint8_t *mem);
void RPE(CPU *cpu, uint8_t *mem);
void RPO(CPU *cpu, uint8_t *mem);
void RC(CPU *cpu, uint8_t *mem);
void RNC(CPU *cpu, uint8_t *mem);

void RST(CPU *cpu, uint8_t *mem, uint8_t code);

void SHLD(CPU *cpu, uint8_t *mem, uint16_t addr);
void LHLD(CPU *cpu, uint8_t *mem, uint16_t addr);

void XTHL(CPU *cpu, uint8_t *mem);
void XCHG(CPU *cpu);

void SPHL(CPU *cpu);
void PCHL(CPU *cpu);

void STC(CPU *cpu);
void CMC(CPU *cpu);

void DAA(CPU *cpu);

void IN(CPU *cpu, uint8_t port);
void OUT(CPU *cpu, uint8_t port);

void RIM(CPU *cpu);
void SIM(CPU *cpu);

void DI(CPU *cpu);
void EI(CPU *cpu);

void HLT(CPU *cpu);

#endif
