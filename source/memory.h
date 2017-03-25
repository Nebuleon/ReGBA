/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2007 ????? <?????>
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

#ifndef MEMORY_H
#define MEMORY_H

#include "cpu.h"

#define SAVESTATE_SIZE 506952
#define SAVESTATE_SIZE_OLD 506947
#define SVS_HEADER_SIZE 11
extern const uint8_t SVS_HEADER_E[SVS_HEADER_SIZE];
extern const uint8_t SVS_HEADER_F[SVS_HEADER_SIZE];
#define SVS_FILE_SIZE (SAVESTATE_SIZE+SVS_HEADER_SIZE)

#define CONFIG_FILENAME "game_config.txt"

typedef enum
{
  DMA_START_IMMEDIATELY,
  DMA_START_VBLANK,
  DMA_START_HBLANK,
  DMA_START_SPECIAL,
  DMA_INACTIVE
} DMA_START_TYPE;

typedef enum
{
  DMA_16BIT,
  DMA_32BIT
} DMA_LENGTH_TYPE;

typedef enum
{
  DMA_NO_REPEAT,
  DMA_REPEAT
} DMA_REPEAT_TYPE;

typedef enum
{
  DMA_INCREMENT,
  DMA_DECREMENT,
  DMA_FIXED,
  DMA_RELOAD
} DMA_INCREMENT_TYPE;

typedef enum
{
  DMA_NO_IRQ,
  DMA_TRIGGER_IRQ
} DMA_IRQ_TYPE;

typedef enum
{
  DMA_DIRECT_SOUND_A,
  DMA_DIRECT_SOUND_B,
  DMA_NO_DIRECT_SOUND
} DMA_DS_TYPE;

typedef struct
{
  uint32_t dma_channel;
  uint32_t source_address;
  uint32_t dest_address;
  uint32_t length;
  DMA_REPEAT_TYPE repeat_type;
  DMA_DS_TYPE direct_sound_channel;
  DMA_INCREMENT_TYPE source_direction;
  DMA_INCREMENT_TYPE dest_direction;
  DMA_LENGTH_TYPE length_type;
  DMA_START_TYPE start_type;
  DMA_IRQ_TYPE irq;
} DMA_TRANSFER_TYPE;

typedef enum
{
  REG_DISPCNT     = 0x000,
  REG_DISPSTAT    = 0x002,
  REG_VCOUNT      = 0x003,
  REG_BG0CNT      = 0x004,
  REG_BG1CNT      = 0x005,
  REG_BG2CNT      = 0x006,
  REG_BG3CNT      = 0x007,
  REG_BG0HOFS     = 0x008,
  REG_BG0VOFS     = 0x009,
  REG_BG1HOFS     = 0x00A,
  REG_BG1VOFS     = 0x00B,
  REG_BG2HOFS     = 0x00C,
  REG_BG2VOFS     = 0x00D,
  REG_BG3HOFS     = 0x00E,
  REG_BG3VOFS     = 0x00F,
  REG_BG2PA       = 0x010,
  REG_BG2PB       = 0x011,
  REG_BG2PC       = 0x012,
  REG_BG2PD       = 0x013,
  REG_BG2X_L      = 0x014,
  REG_BG2X_H      = 0x015,
  REG_BG2Y_L      = 0x016,
  REG_BG2Y_H      = 0x017,
  REG_BG3PA       = 0x018,
  REG_BG3PB       = 0x019,
  REG_BG3PC       = 0x01A,
  REG_BG3PD       = 0x01B,
  REG_BG3X_L      = 0x01C,
  REG_BG3X_H      = 0x01D,
  REG_BG3Y_L      = 0x01E,
  REG_BG3Y_H      = 0x01F,
  REG_WIN0H       = 0x020,
  REG_WIN1H       = 0x021,
  REG_WIN0V       = 0x022,
  REG_WIN1V       = 0x023,
  REG_WININ       = 0x024,
  REG_WINOUT      = 0x025,
  REG_BLDCNT      = 0x028,
  REG_BLDALPHA    = 0x029,
  REG_BLDY        = 0x02A,
  REG_TM0D        = 0x080,
  REG_TM0CNT      = 0x081,
  REG_TM1D        = 0x082,
  REG_TM1CNT      = 0x083,
  REG_TM2D        = 0x084,
  REG_TM2CNT      = 0x085,
  REG_TM3D        = 0x086,
  REG_TM3CNT      = 0x087,
  REG_SIOCNT      = 0x094,
  REG_SIOMLT_SEND = 0x095,
  REG_P1          = 0x098,
  REG_P1CNT       = 0x099,
  REG_RCNT        = 0x09A,
  REG_IE          = 0x100,
  REG_IF          = 0x101,
  REG_IME         = 0x104,
  REG_HALTCNT     = 0x180
} HARDWARE_REGISTER;

typedef enum
{
  TIMER_INACTIVE,
  TIMER_PRESCALE,
  TIMER_CASCADE
} TIMER_STATUS_TYPE;

typedef enum
{
  TIMER_NO_IRQ,
  TIMER_TRIGGER_IRQ
} TIMER_IRQ_TYPE;

typedef enum
{
  TIMER_DS_CHANNEL_NONE,
  TIMER_DS_CHANNEL_A,
  TIMER_DS_CHANNEL_B,
  TIMER_DS_CHANNEL_BOTH
} TIMER_DS_CHANNEL_TYPE;

typedef struct
{
  int32_t count;
  uint32_t reload;
  uint32_t prescale;
  uint32_t stop_cpu_ticks; /* NOT USE */
  FIXED16_16 frequency_step;
  TIMER_DS_CHANNEL_TYPE direct_sound_channels;
  TIMER_IRQ_TYPE irq;
  TIMER_STATUS_TYPE status;
} TIMER_TYPE;

// グローバル変数宣言

extern uint32_t mem_save_flag;
extern char gamepak_title[13];
extern char gamepak_code[5];
extern char gamepak_maker[3];
extern char CurrentGamePath[MAX_PATH];
extern bool IsGameLoaded;
extern uint32_t gamepak_crc32;

extern uint8_t *gamepak_rom;
extern uint8_t *gamepak_rom_resume;
extern uint32_t gamepak_ram_buffer_size;
extern uint32_t oam_update;
extern uint32_t gbc_sound_update;
extern DMA_TRANSFER_TYPE dma[4];
extern TIMER_TYPE timer[4];

extern uint8_t savestate_write_buffer[];
extern uint8_t *g_state_buffer_ptr;

struct BIOS_DATA
{
  // These need to be consecutive because separated areas break Lufia games.
  // Somehow, some code is accessing the metadata's first half as if it were
  // a "second half" of the BIOS, and having all-zeroes in that area breaks
  // everything. Also do not reorder the members, because the assembly files
  // expect "bios" to mean "bios.rom".
  uint8_t  rom     [0x4000];
  uint16_t metadata[0x4000];
};

extern uint16_t palette_ram   [  0x200];
extern uint16_t oam_ram       [  0x200];
extern uint16_t io_registers  [ 0x4000];
extern uint8_t  ewram_data    [0x40000];
extern uint8_t  iwram_data    [ 0x8000];
extern uint8_t  vram          [0x18000];
struct BIOS_DATA bios;
extern uint8_t  gamepak_backup[0x20000];

#ifndef USE_C_CORE

extern uint16_t iwram_metadata[ 0x8000];
extern uint16_t ewram_metadata[0x40000];
extern uint16_t vram_metadata [0x18000];
#endif

extern uint32_t bios_read_protect;

extern uint8_t *memory_map_read[8 * 1024];
extern uint32_t reg[64];
extern uint8_t *memory_map_write[8 * 1024];

// This is the path to per-user files for ReGBA. This path does not have
// a trailing slash.
extern char main_path[MAX_PATH + 1];

extern FILE_TAG_TYPE gamepak_file_large;

extern uint32_t gbc_sound_wave_update;

#ifdef OLD_COUNT
extern uint32_t waitstate_cycles_sequential[16][3];
#else
extern uint8_t waitstate_cycles_seq[2][16];
extern uint8_t waitstate_cycles_non_seq[2][16];
extern uint8_t cpu_waitstate_cycles_seq[2][16];
#endif

// SIO
extern uint32_t g_multi_mode;
extern uint32_t g_adhoc_transfer_flag;

// Tilt sensor on the GBA side. It's mapped... somewhere... in the GBA address
// space. See the read_backup function in memory.c for more information.
extern uint32_t tilt_sensor_x;
extern uint32_t tilt_sensor_y;

// 関数宣言

extern uint8_t read_memory8(uint32_t address);
extern uint32_t read_memory16(uint32_t address);
extern uint16_t read_memory16_signed(uint32_t address);
extern uint32_t read_memory32(uint32_t address);
extern CPU_ALERT_TYPE write_memory8(uint32_t address, uint8_t value);
extern CPU_ALERT_TYPE write_memory16(uint32_t address, uint16_t value);
extern CPU_ALERT_TYPE write_memory32(uint32_t address, uint32_t value);

extern CPU_ALERT_TYPE dma_transfer(DMA_TRANSFER_TYPE *dma);
extern uint8_t *memory_region(uint32_t address, uint32_t *memory_limit);
extern int32_t load_bios(const char* name);
extern ssize_t load_gamepak(const char* file_path);
extern uint8_t *load_gamepak_page(uint16_t physical_index);
extern uint32_t load_backup();
extern void init_memory();
extern void init_gamepak_buffer();
extern void update_backup();
extern void update_backup_force();
extern void bios_region_read_allow();
extern void bios_region_read_protect();
extern uint32_t load_state(uint32_t SlotNumber);
extern uint32_t save_state(uint32_t SlotNumber, const uint16_t *screen_capture);
extern void init_rewind(void);
extern void savestate_rewind(void);
extern void loadstate_rewind(void);

extern unsigned int rewind_queue_len;

#endif
