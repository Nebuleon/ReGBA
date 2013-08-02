/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
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

/******************************************************************************
 * common.h
 * 通用头文件
 ******************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#define OLD_COUNT

/******************************************************************************
 * 
 ******************************************************************************/
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SYS_CLOCK (16777216.0)

#define ROR(dest, value, shift)                                             \
  dest = ((value) >> (shift)) | ((value) << (32 - (shift)))                 \

#define FILE_READ_MEM(ptr, buffer, size)                                    \
{                                                                           \
  memcpy(buffer, ptr, size);                                                \
  ptr += size;                                                              \
}                                                                           \

#define FILE_WRITE_MEM(ptr, buffer, size)                                   \
{                                                                           \
  memcpy(ptr, buffer, size);                                                \
  ptr += size;                                                              \
}                                                                           \

// These must be variables, not constants.

#define FILE_READ_VARIABLE(filename_tag, variable)                          \
  FILE_READ(filename_tag, &variable, sizeof(variable))                      \

#define FILE_WRITE_VARIABLE(filename_tag, variable)                         \
  FILE_WRITE(filename_tag, &variable, sizeof(variable))                     \

#define FILE_READ_MEM_VARIABLE(ptr, variable)                               \
  FILE_READ_MEM(ptr, &variable, sizeof(variable))                           \

#define FILE_WRITE_MEM_VARIABLE(ptr, variable)                              \
  FILE_WRITE_MEM(ptr, &variable, sizeof(variable))                          \

// These must be statically declared arrays (ie, global or on the stack,
// not dynamically allocated on the heap)

#define FILE_READ_ARRAY(filename_tag, array)                                \
  FILE_READ(filename_tag, array, sizeof(array))                             \

#define FILE_WRITE_ARRAY(filename_tag, array)                               \
  FILE_WRITE(filename_tag, array, sizeof(array))                            \

#define FILE_READ_MEM_ARRAY(ptr, array)                                     \
  FILE_READ_MEM(ptr, array, sizeof(array))                                  \

#define FILE_WRITE_MEM_ARRAY(ptr, array)                                    \
  FILE_WRITE_MEM(ptr, array, sizeof(array))                                 \

#define FLOAT_TO_FP16_16(value)                                             \
  (FIXED16_16)((value) * 65536.0)                                           \

#define FP16_16_TO_FLOAT(value)                                             \
  (float)((value) / 65536.0)                                                \

#define U32_TO_FP16_16(value)                                               \
  ((value) << 16)                                                           \

#define FP16_16_TO_U32(value)                                               \
  ((value) >> 16)                                                           \

#define FP16_16_FRACTIONAL_PART(value)                                      \
  ((value) & 0xFFFF)                                                        \

#define FIXED_DIV(numerator, denominator, bits)                             \
  ((((numerator) * (1 << (bits))) + ((denominator) / 2)) / (denominator))   \

#define ADDRESS8(base, offset)                                              \
  *((u8 *)((u8 *)(base) + (offset)))                                        \

#define ADDRESS16(base, offset)                                             \
  *((u16 *)((u8 *)(base) + (offset)))                                       \

#define ADDRESS32(base, offset)                                             \
  *((u32 *)((u8 *)(base) + (offset)))                                       \

#define USE_BIOS 0
#define EMU_BIOS 1

#define NO  0
#define YES 1

#include "port.h"

typedef u32 FIXED16_16;   // Q16.16 fixed-point

#include "cpu.h"
#include "memory.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "gpsp_main.h"
#include "cheats.h"
#include "zip.h"
#include "message.h"
#include "bios.h"
#include "stats.h"

// - - - CROSS-PLATFORM FUNCTION DEFINITIONS - - -

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

/*
 * Advises the user that the GBA game attempted to make an unsupported jump.
 * This function must either exit or never return, because otherwise
 * a jump to NULL would be made.
 * Input:
 *   SourcePC: The address, in the GBA address space, containing the bad jump.
 *   TargetPC: The address, in the GBA address space, that would be jumped to.
 * Output:
 *   Completes abnormally.
 */
void ReGBA_BadJump(u32 SourcePC, u32 TargetPC);

/*
 * Advises the user that the GBA code translator encountered a block with more
 * possible exits than it was prepared to handle.
 * Input:
 *   BlockStartPC: The address, in the GBA address space, of the instruction
 *   starting the code block.
 *   BlockEndPC: The address, in the GBA address space, of the instruction
 *   ending the code block.
 *   Exits: The number of exits encountered.
 */
void ReGBA_MaxBlockExitsReached(u32 BlockStartPC, u32 BlockEndPC, u32 Exits);

/*
 * Advises the user that the GBA code translator encountered a block with more
 * instructions than it was prepared to handle.
 * Input:
 *   BlockStartPC: The address, in the GBA address space, of the instruction
 *   starting the code block.
 *   BlockEndPC: The address, in the GBA address space, of the instruction
 *   ending the code block.
 *   BlockSize: The number of instructions encountered.
 */
void ReGBA_MaxBlockSizeReached(u32 BlockStartPC, u32 BlockEndPC, u32 BlockSize);

/*
 * Displays current frames per second, as calculated using the Stats struct,
 * to the output mechanism most appropriate for the port being compiled.
 * Input:
 *   (implied) Stats: The struct containing framerate data.
 */
void ReGBA_DisplayFPS(void);

#endif /* COMMON_H */
