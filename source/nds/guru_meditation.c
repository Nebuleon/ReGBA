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
		sprintf(Line, "%d:", RegisterNumbers[i]);
		BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, (i % 4) * 64, 80 + (i / 4) * 16, 0, COLOR_TRANS, COLOR_WHITE, Line);

		// Display the register value.
		sprintf(Line, "%08X", *(unsigned int*) ((unsigned char*) Data + RegisterOffsets[i]));
		BDF_render_mix(down_screen_addr, NDS_SCREEN_WIDTH, (i % 4) * 64 + 14, 80 + (i / 4) * 16, 0, COLOR_TRANS, COLOR_WHITE, Line);
	}

	ds2_flipScreen(DOWN_SCREEN, 2);
	while (1)
		pm_idle(); // This is goodbye...
}
