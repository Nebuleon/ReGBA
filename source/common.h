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
#include <stdint.h>
typedef uint32_t FIXED16_16;  // Q16.16 fixed-point
#include <stdbool.h>

#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __GNUC__
#	define likely(x)       __builtin_expect((x),1)
#	define unlikely(x)     __builtin_expect((x),0)
#	define prefetch(x, y)  __builtin_prefetch((x),(y))
#else
#	define likely(x)       (x)
#	define unlikely(x)     (x)
#   define prefetch(x, y)
#endif

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
extern u16*     GBAScreen;
extern uint32_t GBAScreenPitch;

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
 * Reads the buttons pressed at the time of the function call on the input
 * mechanism most appropriate for the port being compiled.
 * Returns:
 *   A bitfield of GBA buttons and possible ReGBA special buttons that are
 *   pressed at the time of the function call.
 * Output assertions:
 *   a) If the port being compiled supports remapping buttons, the code in
 *      the core of ReGBA does not care about this and wants a return value in
 *      reverse GBA bitfield format with ReGBA special buttons which it will
 *      resolve.
 *   b) This function shall return SET bits for pressed buttons and UNSET bits
 *      for unpressed buttons. The GBA expects the other way. The core handles
 *      this after the function returns.
 *   c) Values are:
 *      Button                 | Bit | Enum value
 *      -----------------------------------------------------
 *      GBA A                  |   0 | REGBA_BUTTON_A
 *      GBA B                  |   1 | REGBA_BUTTON_B
 *      GBA Select             |   2 | REGBA_BUTTON_SELECT
 *      GBA Start              |   3 | REGBA_BUTTON_START
 *      GBA D-pad Right        |   4 | REGBA_BUTTON_RIGHT
 *      GBA D-pad Left         |   5 | REGBA_BUTTON_LEFT
 *      GBA D-pad Up           |   6 | REGBA_BUTTON_UP
 *      GBA D-pad Down         |   7 | REGBA_BUTTON_DOWN
 *      GBA R trigger          |   8 | REGBA_BUTTON_R
 *      GBA L trigger          |   9 | REGBA_BUTTON_L
 *      ReGBA rapid-fire A     |  10 | REGBA_BUTTON_RAPID_A
 *      ReGBA rapid-fire B     |  11 | REGBA_BUTTON_RAPID_B
 *      ReGBA Menu             |  12 | REGBA_BUTTON_MENU
 */
enum ReGBA_Buttons ReGBA_GetPressedButtons();

/*
 * Outputs audio from the core's audio buffer onto the audio hardware most
 * appropriate for the port being compiled.
 * 
 * This function can actually take many courses of action, depending on how
 * that sound hardware works:
 * a) The function returns -1 immediately if it finds that the return
 *    value of ReGBA_GetAudioSamplesAvailable is not sufficient for one full
 *    native buffer of audio data (for example, one SDL audio buffer of 512
 *    samples at 44100 Hz). ReGBA_GetAudioSamplesAvailable returns a number of
 *    samples available at SOUND_FREQUENCY Hz.
 * b) The function finds that the return value of ReGBA_GetAudioSamplesAvailable
 *    is sufficient for one full native buffer of audio data, and blocks until
 *    the audio hardware takes the buffer before returning zero.
 * c) The function finds that the return value of ReGBA_GetAudioSamplesAvailable
 *    is sufficient for one full native buffer of audio data, and polls for
 *    the status of the audio hardware, looping until it's available before
 *    returning zero.
 * d) The function finds that the return value of ReGBA_GetAudioSamplesAvailable
 *    is greater than zero, takes all samples to post them onto a queue
 *    readable by a dedicated audio output thread, then returns zero.
 * 
 * Input:
 *   (implied) ReGBA_GetAudioSamplesAvailable: The return value of this core
 *   function determines whether there are enough audio samples available in
 *   the core to output to the native audio hardware.
 *   (implied) ReGBA_LoadNextAudioSample: This function loads rendered audio
 *   samples from the core into two variables, one per stereo channel, then
 *   consumes them. Even if the user requests muting the audio, this function
 *   MUST be called in order to prevent the accumulation of audio in the
 *   core's buffer; this will prevent echoes and clipping when the user
 *   requests unmuting the audio.
 * Returns:
 *   Zero on success.
 *   -1 when there are insufficient samples in the core's audio buffer and
 *   this port function requests more from the core first.
 *   Any other value on failure.
 */
signed int ReGBA_AudioUpdate();

/*
 * Displays the emulator menu in a manner appropriate for the port being
 * compiled.
 * Input:
 *   EntryReason: The reason for entering the menu. Depending on this reason,
 *   the port may disable some interface elements or force the user to choose
 *   a GBA game.
 * Returns:
 *   0 if the menu was exited in a way that allows the currently-loaded game
 *   to continue.
 *   1 if the menu was exited in a way that interrupts the current game:
 *   restarting it, loading a new one, or loading a saved state.
 */
u32 ReGBA_Menu(enum ReGBA_MenuEntryReason EntryReason);

/*
 * Retrieves the path to the battery-backed saved data file corresponding to
 * the given game, in the location most appropriate for the port being
 * compiled.
 * Input:
 *   GamePath: The full path to the file containing the game that is currently
 *   loaded, and for which the backup filename is being requested.
 * Output:
 *   Result: Non-null pointer to a buffer containing at least MAX_PATH + 1
 *   elements of type char.
 * Returns:
 *   true if the retrieval succeeeded; otherwise, false.
 * Output assertions:
 *   The contents of the buffer starting at Result are updated to contain the
 *   full path to the battery-backed saved data file corresponding to the
 *   given game, null-terminated as usual, if the return value is true.
 */
bool ReGBA_GetBackupFilename(char* Result, const char* GamePath);

/*
 * Returns the length, in bytes, of the specified open file.
 * Input:
 *   File: The handle of the file to get the length of.
 * Returns:
 *   The length of the file whose handle was passed in, in bytes.
 */
size_t FILE_LENGTH(FILE_TAG_TYPE File);

#endif /* COMMON_H */
