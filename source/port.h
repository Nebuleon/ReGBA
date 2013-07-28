/* ReGBA - Declarations of per-platform functions used in cross-platform code
 *
 * Copyright (C) 2013 GBATemp user Nebuleon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __GPSP_PORT_H__
#define __GPSP_PORT_H__

/*
 * Traces a printf-formatted message to the output mechanism most appropriate
 * for the port being compiled.
 */
void ReGBA_Trace(const char* Format, ...);

/*
 * Makes newly-generated code visible to the processor(s) for execution.
 * Input:
 *   Code: Address of the first byte of code to be made visible.
 *   CodeLength: Number of bytes of code to be made visible.
 * Input assertions:
 *   Code is not NULL and is mapped into the address space.
 *   (char*) Code + CodeLength - 1 is mapped into the address space.
 *
 * Some ports run on architectures that hold a cache of instructions that is
 * not kept up to date after writes to the data cache or memory. In those
 * ports, the instruction cache, or at least the portion of the instruction
 * cache containing newly-generated code, must be flushed.
 *
 * Some other ports may need a few instructions merely to flush a pipeline,
 * in which case an asm statement containing no-operation instructions is
 * appropriate.
 *
 * The rest runs on architectures that resynchronise the code seen by the
 * processor automatically; in that case, the function is still required, but
 * shall be empty.
 */
void ReGBA_MakeCodeVisible(void* Code, unsigned int CodeLength);

#endif