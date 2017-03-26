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
#include <inttypes.h>
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
  (FIXED16_16)((value) * 65536.0f)                                           \

#define FP16_16_TO_FLOAT(value)                                             \
  (float)((value) / 65536.0f)                                                \

#define U32_TO_FP16_16(value)                                               \
  ((value) << 16)                                                           \

#define FP16_16_TO_U32(value)                                               \
  ((value) >> 16)                                                           \

#define FP16_16_FRACTIONAL_PART(value)                                      \
  ((value) & 0xFFFF)                                                        \
  
#define FP16_16_MAX_FRACTIONAL_PART 0xFFFF

#define FIXED_DIV(numerator, denominator, bits)                             \
  ((((numerator) * (1 << (bits))) + ((denominator) / 2)) / (denominator))   \

#define ADDRESS8(base, offset)                                              \
  *((uint8_t *)(base) + (offset))                                           \

#define ADDRESS16(base, offset)                                             \
  *((uint16_t *)(base) + ((offset) / 2))                                    \

#define ADDRESS32(base, offset)                                             \
  *((uint32_t *)(base) + ((offset) / 4))                                    \

#define USE_BIOS 0
#define EMU_BIOS 1

#define NO  0
#define YES 1

#include "port.h"

/* If the port didn't give a way to declare a variable as fully uninitialised
 * (when zero-initialisation is wasteful), declare them normally. */
#ifndef FULLY_UNINITIALIZED
#  define FULLY_UNINITIALIZED(declarator) declarator
#endif

#include "cpu.h"
#include "memory.h"
#include "video.h"
#include "input.h"
#include "sound.h"
#include "cheats.h"
#include "zip.h"
#include "stats.h"

#include "sha1.h"

// - - - CROSS-PLATFORM TYPE DEFINITIONS - - -

/*
 * A struct used in the GBA RTC code and in the saved state header.
 */
struct ReGBA_RTC {
	uint8_t year;        // Range: 0..99 (Add 2000 to get a 4-digit year)
	uint8_t month;       // Range: 1..12
	uint8_t day;         // Range: 1..31 (Varies by month)
	uint8_t weekday;     // Range: 0..6 (0 is Sunday, 6 is Saturday)
	uint8_t hours;       // Range: 0..23
	uint8_t minutes;     // Range: 0..59
	uint8_t seconds;     // Range: 0..59
};

enum ReGBA_FileAction {
	FILE_ACTION_UNKNOWN,
	FILE_ACTION_LOAD_BIOS,
	FILE_ACTION_LOAD_BATTERY,
	FILE_ACTION_SAVE_BATTERY,
	FILE_ACTION_LOAD_STATE,
	FILE_ACTION_SAVE_STATE,
	FILE_ACTION_DECOMPRESS_ROM_TO_RAM,
	FILE_ACTION_DECOMPRESS_ROM_TO_FILE,
	FILE_ACTION_LOAD_ROM_FROM_FILE,        // uncompressed or temporary file
	FILE_ACTION_APPLY_GAME_COMPATIBILITY,
	FILE_ACTION_LOAD_GLOBAL_SETTINGS,
	FILE_ACTION_SAVE_GLOBAL_SETTINGS,
	FILE_ACTION_LOAD_GAME_SETTINGS,
	FILE_ACTION_SAVE_GAME_SETTINGS
};

// - - - CROSS-PLATFORM VARIABLE DEFINITIONS - - -
extern uint16_t* GBAScreen;
extern uint32_t  GBAScreenPitch;

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
void ReGBA_MakeCodeVisible(void* Code, size_t CodeLength);

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
void ReGBA_BadJump(uint32_t SourcePC, uint32_t TargetPC);

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
void ReGBA_MaxBlockExitsReached(uint32_t BlockStartPC, uint32_t BlockEndPC, uint32_t Exits);

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
void ReGBA_MaxBlockSizeReached(uint32_t BlockStartPC, uint32_t BlockEndPC, uint32_t BlockSize);

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
bool ReGBA_IsRenderingNextFrame(void);

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
uint32_t ReGBA_Menu(enum ReGBA_MenuEntryReason EntryReason);

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
 * Retrieves the path to the saved state file corresponding to the given game
 * and slot number, in the location most appropriate for the port being
 * compiled.
 * Input:
 *   GamePath: The full path to the file containing the game that is currently
 *   loaded, and for which a saved state filename is being requested.
 *   SlotNumber: The number of the slot selected by the user for saving.
 *   Starts at 0, even if the menu starts them at 1.
 * Output:
 *   Result: Non-null pointer to a buffer containing at least MAX_PATH + 1
 *   elements of type char.
 * Returns:
 *   true if the retrieval succeeeded; otherwise, false.
 * Output assertions:
 *   The contents of the buffer starting at Result are updated to contain the
 *   full path to the saved state file corresponding to the given game and
 *   slot number, null-terminated as usual, if the return value is true.
 */
bool ReGBA_GetSavedStateFilename(char* Result, const char* GamePath, uint32_t SlotNumber);

/*
 * Retrieves the path to a bundled game_config.txt file, used for increased
 * game compatibility, in the location most appropriate for the port being
 * compiled. The semantics of this file is that lookups are made in it last,
 * after consulting the per-user file in main_path for overrides.
 * Output:
 *   Result: Non-null pointer to a buffer containing at least MAX_PATH + 1
 *   elements of type char.
 * Returns:
 *   true if the retrieval succeeeded; otherwise, false.
 * Output assertions:
 *   The contents of the buffer starting at Result are updated to contain the
 *   full path to a bundled game_config.txt file, if the return value is true.
 */
bool ReGBA_GetBundledGameConfig(char* Result);

/*
 * The intent of this function is for the port to be able to return a memory
 * mapping that is filled on demand by the operating system it runs on. If
 * such a mapping can be achieved, this function can return a new mapping for
 * the uncompressed ROM file. The return value, if non-NULL, must point to
 * memory that is readable and privately writable (i.e. not written back to
 * the file). The real-time clock registers require being able to write to
 * the mapping, because they are mapped at ROM + C4h.
 * 
 * The port may decide to use this function to load an entire ROM into memory
 * instead, if it deems the on-demand access to be too slow and it can load
 * the entire ROM into memory.
 * Input:
 *   File: The file to create a mapping for.
 *   Size: The size of the file.
 * Returns:
 *   If creating a mapping, the function returns non-NULL.
 *   If not creating a mapping, the function returns NULL.
 * Output assertions:
 *   File is kept open if the return value is non-NULL, and the port is
 *   responsible for keeping track of the mapping so that it can be unmapped
 *   in ReGBA_UnmapEntireROM.
 *   If the return value is NULL, ReGBA will use its own on-demand page
 *   loading mechanism after calling ReGBA_AllocateOnDemandBuffer.
 */
uint8_t* ReGBA_MapEntireROM(FILE_TAG_TYPE File, size_t Size);

/*
 * Unmaps an entire uncompressed ROM mapped by ReGBA_MapEntireROM, closing
 * its file as well.
 * Input:
 *   Mapping: Pointer to the first byte of the mapping.
 */
void ReGBA_UnmapEntireROM(void* Mapping);

/*
 * Allocates a buffer to hold the entire decompressed version of a compressed
 * ROM. The return value, if non-NULL, must point to memory that is readable and
 * writable.
 * Input:
 *   Size: The size of the buffer to be allocated.
 * Returns:
 *   If Size bytes of memory are available, the function returns non-NULL
 *   and preserves the allocation for use in ReGBA_DeallocateROM.
 *   Otherwise, the function returns NULL.
 * Output assertions:
 *   If the return value is NULL, ReGBA will decompress the ROM into a
 *   temporary file, then call ReGBA_MapEntireROM on the temporary file.
 */
uint8_t* ReGBA_AllocateROM(size_t Size);

/*
 * Allocates a buffer to hold pages of the current GBA ROM, loaded from its
 * backing file on demand by ReGBA. This function is called when no mapping
 * can be created for on-demand loading by the operating system for an
 * uncompressed ROM or the temporary-file version of a compressed ROM.
 * ReGBA keeps the file open and tracks it in this case. At the end of the
 * function, Buffer must point to memory that is readable and writable.
 * Output:
 *   Buffer: A pointer to void*, which must be updated to point to the buffer
 *   allocated by the function. That buffer is the largest buffer that the
 *   port being compiled can afford to give ReGBA for its on-demand loading
 *   of GBA ROM pages.
 * Returns:
 *   The size of the buffer allocated for updating Buffer with.
 * Output assertions:
 *   Buffer points to a variable that points to non-NULL. If it doesn't, then
 *   this is a fatal error.
 *   The return value is a multiple of 32768.
 */
size_t ReGBA_AllocateOnDemandBuffer(void** Buffer);

/*
 * Frees the last allocation of memory made without a backing file by the port
 * being compiled. This allocation was made by either ReGBA_AllocateROM or
 * ReGBA_AllocateOnDemandBuffer.
 * Input:
 *   Buffer: Pointer to the first byte of the buffer.
 */
void ReGBA_DeallocateROM(void* Buffer);

/*
 * Performs actions appropriate for the port being compiled after loading a
 * GBA game.
 * Input:
 *   GamePath: The full path to the file containing the game that has just
 *   been loaded.
 */
void ReGBA_OnGameLoaded(const char* GamePath);

/*
 * Returns the length, in bytes, of the specified open file.
 * Input:
 *   File: The handle of the file to get the length of.
 * Returns:
 *   The length of the file whose handle was passed in, in bytes.
 */
size_t FILE_LENGTH(FILE_TAG_TYPE File);

/*
 * Starts displaying a progress indication in the manner most appropriate for
 * the port being compiled.
 * Input:
 *   Action: The action being started.
 */
void ReGBA_ProgressInitialise(enum ReGBA_FileAction Action);

/*
 * Update the progress indication for the action started by the last call to
 * ReGBA_ProgressInitialise which has not been ended by ReGBA_ProgressFinalise
 * in the manner most appropriate for the port being compiled.
 * Input:
 *   Current: The units of progress carried out so far for the action.
 *   Total: The units of progress required to carry out the full action.
 */
void ReGBA_ProgressUpdate(uint32_t Current, uint32_t Total);

/*
 * Finishes displaying a progress indication.
 */
void ReGBA_ProgressFinalise();

/*
 * Saves the emulator settings.
 * Input:
 *   File: Extension-less file name of the settings. global_config/gamename
 *   PerGame: true if the settings to be saved are the per-game settings;
 *     false if the settings to be saved are the global settings.
 * Returns
 *   Bool: false for failures, true for successful saving.
 */
bool ReGBA_SaveSettings(char *cfg_name, bool PerGame);

/*
 * Load the emulator settings.
 * Input:
 *   File: Extension-less file name of the settings. global_config/gamename
 *   PerGame: true if the settings to be loaded are the per-game settings;
 *     false if the settings to be loaded are the global settings.
 */
void ReGBA_LoadSettings(char *cfg_name, bool PerGame);

#endif /* COMMON_H */
