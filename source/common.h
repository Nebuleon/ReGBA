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

#include "cpu.h"
#include "memory.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "cheats.h"
#include "zip.h"
#include "stats.h"

// - - - CROSS-PLATFORM TYPE DEFINITIONS - - -

/*
 * A struct used in the GBA RTC code and in the saved state header.
 */
struct ReGBA_RTC {
	unsigned char year;        // Range: 0..99 (Add 2000 to get a 4-digit year)
	unsigned char month;       // Range: 1..12
	unsigned char day;         // Range: 1..31 (Varies by month)
	unsigned char weekday;     // Range: 0..6 (0 is Sunday, 6 is Saturday)
	unsigned char hours;       // Range: 0..23
	unsigned char minutes;     // Range: 0..59
	unsigned char seconds;     // Range: 0..59
};

// - - - CROSS-PLATFORM VARIABLE DEFINITIONS - - -
extern u16* GBAScreen;
extern u32  GBAScreenPitch;

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
 * Renders the current contents of the GBA screen to the display of the device
 * most appropriate for the port being compiled and makes it ready for another
 * frame.
 * Input:
 *   (implied) GBAScreen: The pointer to the first 16-bit RGB555 pixel to be
 *   rendered.
 *   (implied) GBAScreenPitch: The number of contiguous 16-bit pixels that
 *   form a scanline in memory. This value should be 240 if the port performs
 *   pixel format conversion before rendering to the display and hands ReGBA
 *   a different buffer to render in RGB555 into.
 * Output:
 *   (implied) GBAScreen: Gets updated with the address of the first 16-bit
 *   RGB555 pixel of the next GBA frame to be rendered.
 */
void ReGBA_RenderScreen(void);

/*
 * Determines whether ReGBA should set up to render the next frame to the
 * GBAScreen variable. The determination is made according to the method most
 * appropriate for the port being compiled, for example either of these:
 * a) Returning non-zero if there is enough sound in the buffers;
 * b) Returning non-zero if the previous frame was rendered early according to
 *    the system's high-resolution timers.
 */
u32 ReGBA_IsRenderingNextFrame(void);

/*
 * Displays current frames per second, as calculated using the Stats struct,
 * to the output mechanism most appropriate for the port being compiled.
 * Input:
 *   (implied) Stats: The struct containing framerate data.
 */
void ReGBA_DisplayFPS(void);

/*
 * Loads the given memory buffer with real-time clock data from the clock most
 * appropriate for the port being compiled.
 * Output:
 *   RTCData: A pointer to a struct that the port function updates with the
 *   date-time components of the real time as of the function call.
 * Output assertions:
 *   b) Year's range is 0..99. The century is not stored.
 *   c) Month's range is 1..12.
 *   d) Day's range depends on the month, and is 1..31.
 *   e) Weekday's range is 0..6.
 *      0 is Sunday. 1 is Monday. 2 is Tuesday. 3 is Wednesday. 4 is Thursday.
 *      5 is Friday. 6 is Saturday.
 *   f) Hours's range is 0..23 (24-hour time).
 *   g) Minutes's range is 0..59.
 *   h) Seconds's range is 0..59.
 */
void ReGBA_LoadRTCTime(struct ReGBA_RTC* RTCData);

/*
 * Returns the length, in bytes, of the specified open file.
 * Input:
 *   File: The handle of the file to get the length of.
 * Returns:
 *   The length of the file whose handle was passed in, in bytes.
 */
size_t FILE_LENGTH(FILE_TAG_TYPE File);

#endif /* COMMON_H */
