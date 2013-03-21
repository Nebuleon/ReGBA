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

#define MEM_STATE_NUM (10)

#define PSP2K_MEM_TOP (0x0a000000)

#define SAVESTATE_SIZE 506952
#define SAVESTATE_SIZE_OLD 506947
#define SVS_HEADER  "NGBARTS1.0b"
#define SVS_HEADER_SIZE 11
#define SVS_FILE_SIZE (SAVESTATE_SIZE+SVS_HEADER_SIZE)

#define NGBARTS_HEADERA "NGBARTS1.0a"
#define NGBARTS_HEADERA_SIZE 11

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
  u32 dma_channel;
  u32 source_address;
  u32 dest_address;
  u32 length;
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

// グローバル変数宣言

extern u32 mem_save_flag;
extern char gamepak_title[13];
extern char gamepak_code[5];
extern char gamepak_maker[3];
extern char gamepak_filename[MAX_FILE];
extern char gamepak_filename_full_path[MAX_PATH];
extern u32 gamepak_crc32;

extern u8 *gamepak_rom;
extern u8 *gamepak_rom_resume;
extern u32 gamepak_ram_buffer_size;
extern u32 oam_update;
extern u32 gbc_sound_update;
extern DMA_TRANSFER_TYPE dma[4];

extern u8 savestate_write_buffer[];
extern u8 *g_state_buffer_ptr;

//#define USE_VRAM

#ifndef USE_VRAM
extern u16 palette_ram[512];
extern u16 oam_ram[512];
extern u16 io_registers[1024 * 16];
extern u8 ewram[1024 * 256 * 2];
extern u8 iwram[1024 * 32 * 2];
extern u8 vram[1024 * 96 * 2];
extern u8 bios_rom[1024 * 32];
#else
extern u16 *palette_ram;
extern u16 *oam_ram;
extern u16 *io_registers;
extern u8 *ewram;
extern u8 *iwram;
extern u8 *vram;
extern u8 *bios_rom;
#endif

extern u32 bios_read_protect;

extern u8 *memory_map_read[8 * 1024];
extern u32 reg[64];
extern u8 *memory_map_write[8 * 1024];

extern FILE_TAG_TYPE gamepak_file_large;

extern u32 gbc_sound_wave_update;

#ifdef OLD_COUNT
extern u32 waitstate_cycles_sequential[16][3];
#else
extern u8 waitstate_cycles_seq[2][16];
extern u8 waitstate_cycles_non_seq[2][16];
extern u8 cpu_waitstate_cycles_seq[2][16];
#endif

// SIO
extern u32 g_multi_mode;
extern u32 g_adhoc_transfer_flag;

// 関数宣言

extern u8 read_memory8(u32 address);
extern u32 read_memory16(u32 address);
extern u16 read_memory16_signed(u32 address);
extern u32 read_memory32(u32 address);
extern CPU_ALERT_TYPE write_memory8(u32 address, u8 value);
extern CPU_ALERT_TYPE write_memory16(u32 address, u16 value);
extern CPU_ALERT_TYPE write_memory32(u32 address, u32 value);

extern CPU_ALERT_TYPE dma_transfer(DMA_TRANSFER_TYPE *dma);
extern u8 *memory_region(u32 address, u32 *memory_limit);
extern s32 load_bios(char *name);
extern s32 load_gamepak(char *file_path);
extern u8 *load_gamepak_page(u32 physical_index);
extern u32 load_backup(char *name);
extern void init_memory();
extern void init_gamepak_buffer();
extern void update_backup();
extern void update_backup_force();
extern void bios_region_read_allow();
extern void bios_region_read_protect();
//extern u32 load_state(char *savestate_filename, u32 slot_num);
extern u32 load_state(char *savestate_filename, FILE *fp);
extern u32 save_state(char *savestate_filename, u16 *screen_capture);
extern void savefast_int(void);
extern void savestate_fast(void);
extern void loadstate_fast(void);

extern unsigned int savefast_queue_len;

#endif
