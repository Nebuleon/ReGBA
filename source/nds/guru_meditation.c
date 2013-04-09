/*
 * DSTwo Guru Meditation screen
 * (c) 2013 GBAtemp user Nebuleon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "ds2io.h"
#include "ds2_cpu.h"
#include "bdf_font.h"
#include "draw.h"
#include "stddef.h"
#include "string.h"

struct ExceptionData {
	unsigned int Register31;
	unsigned int Register30;
	unsigned int Register28;
	unsigned int Register25;
	unsigned int Register24;
	unsigned int Register23;
	unsigned int Register22;
	unsigned int Register21;
	unsigned int Register20;
	unsigned int Register19;
	unsigned int Register18;
	unsigned int Register17;
	unsigned int Register16;
	unsigned int Register15;
	unsigned int Register14;
	unsigned int Register13;
	unsigned int Register12;
	unsigned int Register11;
	unsigned int Register10;
	unsigned int Register09;
	unsigned int Register08;
	unsigned int Register07;
	unsigned int Register06;
	unsigned int Register05;
	unsigned int Register04;
	unsigned int Register03;
	unsigned int Register02;
	unsigned int Register01;
	unsigned int Lo;
	unsigned int Hi;
	unsigned int ExceptionPC;
	unsigned int InterruptStatus;
} __attribute__((packed));

unsigned char RegisterNumbers[28] = {
	 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 28, 30, 31
};

unsigned int RegisterOffsets[28] = {
	offsetof(struct ExceptionData, Register01),
	offsetof(struct ExceptionData, Register02),
	offsetof(struct ExceptionData, Register03),
	offsetof(struct ExceptionData, Register04),
	offsetof(struct ExceptionData, Register05),
	offsetof(struct ExceptionData, Register06),
	offsetof(struct ExceptionData, Register07),
	offsetof(struct ExceptionData, Register08),
	offsetof(struct ExceptionData, Register09),
	offsetof(struct ExceptionData, Register10),
	offsetof(struct ExceptionData, Register11),
	offsetof(struct ExceptionData, Register12),
	offsetof(struct ExceptionData, Register13),
	offsetof(struct ExceptionData, Register14),
	offsetof(struct ExceptionData, Register15),
	offsetof(struct ExceptionData, Register16),
	offsetof(struct ExceptionData, Register17),
	offsetof(struct ExceptionData, Register18),
	offsetof(struct ExceptionData, Register19),
	offsetof(struct ExceptionData, Register20),
	offsetof(struct ExceptionData, Register21),
	offsetof(struct ExceptionData, Register22),
	offsetof(struct ExceptionData, Register23),
	offsetof(struct ExceptionData, Register24),
	offsetof(struct ExceptionData, Register25),
	offsetof(struct ExceptionData, Register28),
	offsetof(struct ExceptionData, Register30),
	offsetof(struct ExceptionData, Register31)
};

#ifdef NDS_LAYER
#define ADDRESS_VALID(x) ( ((unsigned int) (x) >= 0x80000000) && ((unsigned int) (x) < 0x80000000 + 32 * 1024 * 1024) && (((unsigned int) (x) & 3) == 0) )
#endif

// For this, pick any large memory block containing DATA (not code)
// that's already allocated by the program. It will get clobbered
// and the program will become unusable.
extern u8 vram[1024 * 96 * 2];
#define EMERGENCY_MEMORY ((void*) vram)

unsigned int DisassembleInstructionMIPS32RType(unsigned int Instruction,
	char* ShortForm, char* CanonicalForm);
unsigned int DisassembleInstructionMIPS32IType(unsigned int Instruction,
	unsigned int Opcode, char* ShortForm, char* CanonicalForm);
unsigned int DisassembleInstructionMIPS32JType(unsigned int Instruction,
	unsigned int Opcode, char* ShortForm, char* CanonicalForm);

/*
 * Disassembles a MIPS32 instruction. Both the shortest form accepted by an
 * assembler and the canonical representation of the instruction are output.
 * ShortForm and CanonicalForm are both expected to point to 64-byte buffers.
 * If the instruction is not recognised, the 32-bit value encoding the
 * instruction is output in upper-case hexadecimal form between angle brackets
 * ('<', '>') to both buffers.
 */
void DisassembleInstructionMIPS32(unsigned int Instruction,
	char* ShortForm, char* CanonicalForm)
{
	unsigned int Opcode = (Instruction >> 26) & 0x3F;
	unsigned int Recognised = 0;
	switch (Opcode) {
	case 0x00:	// R-type
		Recognised = DisassembleInstructionMIPS32RType(Instruction,
			ShortForm, CanonicalForm);
		break;
	case 0x04:	// I-type
	case 0x05:
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0C:
	case 0x0D:
	case 0x0F:
	case 0x20:
	case 0x21:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x28:
	case 0x29:
	case 0x2B:
		Recognised = DisassembleInstructionMIPS32IType(Instruction,
			Opcode, ShortForm, CanonicalForm);
		break;
	case 0x02:	// J-type
	case 0x03:
		Recognised = DisassembleInstructionMIPS32JType(Instruction,
			Opcode, ShortForm, CanonicalForm);
		break;
	}
	if (!Recognised) {
		sprintf(CanonicalForm, "<%08X>", Instruction);
		strcpy(ShortForm, CanonicalForm);
	}
}

/*
 * Disassembles a MIPS32 R-type instruction.
 * Returns non-zero and fills both buffers if the instruction is defined.
 * Returns 0 if the instruction is undefined.
 */
unsigned int DisassembleInstructionMIPS32RType(unsigned int Instruction,
	char* ShortForm, char* CanonicalForm)
{
	unsigned int S = (Instruction >> 21) & 0x1F,
		T = (Instruction >> 16) & 0x1F,
		D = (Instruction >> 11) & 0x1F,
		Shift = (Instruction >> 6) & 0x1F,
		Function = Instruction & 0x3F;
	switch (Function) {
	case 0x00:	// SLL RD, RT, SHIFT
		sprintf(CanonicalForm, "sll $%u, $%u, $%u", D, T, Shift);
		if (D == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (T == 0)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x02:	// SRL RD, RT, SHIFT
		sprintf(CanonicalForm, "srl $%u, $%u, $%u", D, T, Shift);
		if (D == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (T == 0)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x03:	// SRA RD, RT, SHIFT
		sprintf(CanonicalForm, "sra $%u, $%u, $%u", D, T, Shift);
		if (D == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (T == 0)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x08:	// JR RS
		sprintf(CanonicalForm, "jr $%u", S);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x10:	// MFHI RD
		sprintf(CanonicalForm, "mfhi $%u", D);
		if (D == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x11:	// MTHI RS
		sprintf(CanonicalForm, "mthi $%u", S);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x12:	// MFLO RD
		sprintf(CanonicalForm, "mflo $%u", D);
		if (D == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x13:	// MTLO RS
		sprintf(CanonicalForm, "mtlo $%u", S);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x18:	// MULT RS, RT
		sprintf(CanonicalForm, "mult $%u, $%u", S, T);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x19:	// MULTU RS, RT
		sprintf(CanonicalForm, "multu $%u, $%u", S, T);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x1A:	// DIV RS, RT
		sprintf(CanonicalForm, "div $%u, $%u", S, T);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x1B:	// DIVU RS, RT
		sprintf(CanonicalForm, "divu $%u, $%u", S, T);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x20:	// ADD RD, RS, RT
		sprintf(CanonicalForm, "add $%u, $%u, $%u", D, S, T);
		if (D == 0 || (D == S && S != 0 && T == 0))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == 0 && T == 0)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else if (S == 0)	// Moving RT into RD
			sprintf(ShortForm, "move $%u, $%u", D, T);
		else if (T == 0)	// Moving RS into RD
			sprintf(ShortForm, "move $%u, $%u", D, S);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x21:	// ADDU RD, RS, RT
		sprintf(CanonicalForm, "addu $%u, $%u, $%u", D, S, T);
		if (D == 0 || (D == S && S != 0 && T == 0))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == 0 && T == 0)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else if (S == 0)	// Moving RT into RD
			sprintf(ShortForm, "move $%u, $%u", D, T);
		else if (T == 0)	// Moving RS into RD
			sprintf(ShortForm, "move $%u, $%u", D, S);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x22:	// SUB RD, RS, RT
		sprintf(CanonicalForm, "sub $%u, $%u, $%u", D, S, T);
		if (D == 0 || (D == S && S != 0 && T == 0))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == T)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else if (T == 0)	// Moving RS into RD
			sprintf(ShortForm, "move $%u, $%u", D, S);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x23:	// SUBU RD, RS, RT
		sprintf(CanonicalForm, "subu $%u, $%u, $%u", D, S, T);
		if (D == 0 || (D == S && S != 0 && T == 0))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == T)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else if (T == 0)	// Moving RS into RD
			sprintf(ShortForm, "move $%u, $%u", D, S);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x24:	// AND RD, RS, RT
		sprintf(CanonicalForm, "and $%u, $%u, $%u", D, S, T);
		if (D == 0 || (D == S && S == T))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == 0 || T == 0)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else if (S == T)	// Moving RS (== RT) into RD
			sprintf(ShortForm, "move $%u, $%u", D, S);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x25:	// OR RD, RS, RT
		sprintf(CanonicalForm, "or $%u, $%u, $%u", D, S, T);
		if (D == 0 || (D == S && (T == S || T == 0))
		||	(D == T && (S == T || S == 0)))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == T)	// Moving RS (== RT) into RD
			sprintf(ShortForm, "move $%u, $%u", D, S);
		else if (S == 0 /* && T != 0 */)	// Moving RT into RD
			sprintf(ShortForm, "move $%u, $%u", D, T);
		else if (T == 0 /* && S != 0 */)	// Moving RS into RD
			sprintf(ShortForm, "move $%u, $%u", D, S);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x26:	// XOR RD, RS, RT
		sprintf(CanonicalForm, "xor $%u, $%u, $%u", D, S, T);
		if (D == 0 || (D == S && T == 0) || (D == T && S == 0))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == T)	// Moving zero into RD
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x27:	// NOR RD, RS, RT
		sprintf(CanonicalForm, "nor $%u, $%u, $%u", D, S, T);
		if (D == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x2A:	// SLT RD, RS, RT
		sprintf(CanonicalForm, "slt $%u, $%u, $%u", D, S, T);
		if (D == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == T)	// Moving zero into RD (Reg < Reg: 0)
			sprintf(ShortForm, "move $%u, $%u", D, 0);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	default:
		return 0;
	}
}

/*
 * Disassembles a MIPS32 I-type instruction.
 * Returns non-zero and fills both buffers if the instruction is defined.
 * Returns 0 if the instruction is undefined.
 */
unsigned int DisassembleInstructionMIPS32IType(unsigned int Instruction,
	unsigned int Opcode, char* ShortForm, char* CanonicalForm)
{
	unsigned int S = (Instruction >> 21) & 0x1F,
		T = (Instruction >> 16) & 0x1F;
	signed short Imm = Instruction & 0xFFFF;
	switch (Opcode) {
	case 0x04:	// BEQ RS, RT, PC+IMM+4
		sprintf(CanonicalForm, "beq $%u, $%u, 0x%X", S, T, Imm);
		if (S == T)	// Effectively an unconditional immediate jump
			sprintf(ShortForm, "b %d", Imm);
		else
			sprintf(ShortForm, "beq $%u, $%u, %d", S, T, Imm);
		return 1;
	case 0x05:	// BNE RS, RT, PC+IMM+4
		sprintf(CanonicalForm, "bne $%u, $%u, 0x%X", S, T, Imm);
		if (S == T)	// Effectively a nop with a branch delay slot
			strcpy(ShortForm, "nop  # branch delay slot");
		else
			sprintf(ShortForm, "bne $%u, $%u, %d", S, T, Imm);
		return 1;
	case 0x08:	// ADDI RT, RS, IMM
		sprintf(CanonicalForm, "addi $%u, $%u, 0x%X", T, S, Imm);
		if (T == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (Imm == 0)	// Moving RS into RT
			sprintf(ShortForm, "move $%u, $%u", T, S);
		else if (S == 0)	// Loading [-32768, 32767] into RT
			sprintf(ShortForm, "li $%u, %d", T, Imm);
		else
			sprintf(ShortForm, "addi $%u, $%u, %d", T, S, Imm);
		return 1;
	case 0x09:	// ADDIU RT, RS, IMM
		sprintf(CanonicalForm, "addiu $%u, $%u, 0x%X", T, S, Imm);
		if (T == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (Imm == 0)	// Moving RS into RT
			sprintf(ShortForm, "move $%u, $%u", T, S);
		else if (S == 0)	// Loading [-32768, 32767] into RT
			sprintf(ShortForm, "li $%u, %d", T, Imm);
		else
			sprintf(ShortForm, "addiu $%u, $%u, %d", T, S, Imm);
		return 1;
	case 0x0A:	// SLTIU RT, RS, IMM
		sprintf(CanonicalForm, "sltiu $%u, $%u, 0x%X", T, S, Imm);
		if (T == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (S == 0 && Imm == 0)	// Moving zero into RT
			sprintf(ShortForm, "move $%u, $%u", T, 0);
		else
			sprintf(ShortForm, "sltiu $%u, $%u, %d", T, S, Imm);
		return 1;
	case 0x0C:	// ANDI RT, RS, IMM
		sprintf(CanonicalForm, "andi $%u, $%u, 0x%X", T, S, Imm);
		if (T == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (Imm == 0)	// Moving zero into RT
			sprintf(ShortForm, "move $%u, $%u", T, 0);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x0D:	// ORI RT, RS, IMM
		sprintf(CanonicalForm, "ori $%u, $%u, 0x%X", T, S, Imm);
		if (T == 0 || (S == T && Imm == 0))	// Effectively a nop
			strcpy(ShortForm, "nop");
		else if (Imm == 0)	// Moving RS into RT
			sprintf(ShortForm, "move $%u, $%u", T, S);
		else
			strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x0F:	// LUI RT, IMM
		sprintf(CanonicalForm, "lui $%u, 0x%X", T, Imm);
		if (T == 0)	// Effectively a nop
			strcpy(ShortForm, "nop");
		else
			sprintf(ShortForm, "lui $%u, %d", T, Imm);
		return 1;
	case 0x20:	// LB RT, IMM(RS)
		sprintf(CanonicalForm, "lb $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "lb $%u, %d($%u)", T, Imm, S);
		return 1;
	case 0x21:	// LH RT, IMM(RS)
		sprintf(CanonicalForm, "lh $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "lh $%u, %d($%u)", T, Imm, S);
		return 1;
	case 0x23:	// LW RT, IMM(RS)
		sprintf(CanonicalForm, "lw $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "lw $%u, %d($%u)", T, Imm, S);
		return 1;
	case 0x24:	// LBU RT, IMM(RS)
		sprintf(CanonicalForm, "lbu $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "lbu $%u, %d($%u)", T, Imm, S);
		return 1;
	case 0x25:	// LHU RT, IMM(RS)
		sprintf(CanonicalForm, "lhu $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "lhu $%u, %d($%u)", T, Imm, S);
		return 1;
	case 0x28:	// SB RT, IMM(RS)
		sprintf(CanonicalForm, "sb $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "sb $%u, %d($%u)", T, Imm, S);
		return 1;
	case 0x29:	// SH RT, IMM(RS)
		sprintf(CanonicalForm, "sh $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "sh $%u, %d($%u)", T, Imm, S);
		return 1;
	case 0x2B:	// SW RT, IMM(RS)
		sprintf(CanonicalForm, "sw $%u, 0x%X($%u)", T, Imm, S);
		sprintf(ShortForm, "sw $%u, %d($%u)", T, Imm, S);
		return 1;
	}
}

/*
 * Disassembles a MIPS32 J-type instruction.
 * Returns non-zero and fills both buffers if the instruction is defined.
 * Returns 0 if the instruction is undefined.
 */
unsigned int DisassembleInstructionMIPS32JType(unsigned int Instruction,
	unsigned int Opcode, char* ShortForm, char* CanonicalForm)
{
	unsigned int Imm = ((Instruction >> 6) & 0x03FFFFFF) << 2;
	switch (Opcode) {
	case 0x02:	// j PCSeg4.IMM26.00
		sprintf(CanonicalForm, "j ?%07X", Imm);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	case 0x03:	// jal PCSeg4.IMM26.00
		sprintf(CanonicalForm, "jal ?%07X", Imm);
		strcpy(ShortForm, CanonicalForm);
		return 1;
	}
}

void GuruMeditation(struct ExceptionData* Data, unsigned int ExceptionNumber)
{
	sti(); // Allows communication to the DS

	ds2_setCPUclocklevel(0);
	mdelay(100); // to prevent ds2_setBacklight from crashing
	ds2_setBacklight(3);

	ds2_clearScreen(DOWN_SCREEN, RGB15(15, 0, 0));
	ds2_flipScreen(DOWN_SCREEN, 2);
	draw_string_vcenter(down_screen_addr, 0, 0, 256, COLOR_WHITE, "Guru Meditation");

	char Line[512];
	char* Description;
	switch (ExceptionNumber) {
		case  1: Description = "TLB modification exception";    break;
		case  2: Description = "TLB load exception";            break;
		case  3: Description = "TLB store exception";           break;
		case  4: Description = "Unaligned load exception";      break;
		case  5: Description = "Unaligned store exception";     break;
		case  6: Description = "Instruction fetch exception";   break;
		case  7: Description = "Data reference exception";      break;
		case  9: Description = "Debug breakpoint hit";          break;
		case 10: Description = "Unknown instruction exception"; break;
		case 11: Description = "Coprocessor unusable";          break;
		case 12: Description = "Arithmetic overflow exception"; break;
		case 13: Description = "Trap";                          break;
		case 15: Description = "Floating point exception";      break;
		case 18: Description = "Coprocessor 2 exception";       break;
		case 23: Description = "Debug memory watch hit";        break;
		case 24: Description = "Machine check exception";       break;
		case 30: Description = "Cache consistency error";       break;
		default: Description = "Unknown exception";             break;
	}
	sprintf(Line, "Exception %02X: %s", ExceptionNumber, Description);
	BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, 0, 32, 0, COLOR_TRANS, COLOR_WHITE, Line);

	sprintf(Line, "at address %08X", Data->ExceptionPC);
	BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, 0, 48, 0, COLOR_TRANS, COLOR_WHITE, Line);

	unsigned int i;
	for (i = 0; i < 28; i++) {
		// Display the register number.
		sprintf(Line, "%u:", RegisterNumbers[i]);
		BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, (i % 4) * 64, 80 + (i / 4) * 16, 0, COLOR_TRANS, COLOR_WHITE, Line);

		// Display the register value.
		sprintf(Line, "%08X", *(unsigned int*) ((unsigned char*) Data + RegisterOffsets[i]));
		BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, (i % 4) * 64 + 14, 80 + (i / 4) * 16, 0, COLOR_TRANS, COLOR_WHITE, Line);
	}
	ds2_flipScreen(DOWN_SCREEN, 2);

	// Show the disassembly of the instruction at EPC if it's valid.
	if (ADDRESS_VALID(Data->ExceptionPC)) {
		char* ShortForm = ((char*) EMERGENCY_MEMORY);
		char* CanonicalForm = (char*) EMERGENCY_MEMORY + 64;
		memset(EMERGENCY_MEMORY, 0, 128);
		DisassembleInstructionMIPS32(*((unsigned int*) Data->ExceptionPC), ShortForm, CanonicalForm);

		BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, 128, 48, 0, COLOR_TRANS, COLOR_WHITE, ShortForm);
		if (strcmp(ShortForm, CanonicalForm) != 0) {
			BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, 116, 64, 0, COLOR_TRANS, COLOR_WHITE, "=");
			BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, 128, 64, 0, COLOR_TRANS, COLOR_WHITE, CanonicalForm);
		}
	}
	ds2_flipScreen(DOWN_SCREEN, 2);

	while (1)
		pm_idle(); // This is goodbye...
}
