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

#ifndef CPU_H
#define CPU_H

// System mode and user mode are represented as the same here

typedef enum
{
  MODE_USER,
  MODE_IRQ,
  MODE_FIQ,
  MODE_SUPERVISOR,
  MODE_ABORT,
  MODE_UNDEFINED,
  MODE_INVALID
} CPU_MODE_TYPE;

typedef enum
{
  CPU_ALERT_NONE,
  CPU_ALERT_HALT,
  CPU_ALERT_SMC,
  CPU_ALERT_IRQ
} CPU_ALERT_TYPE;

typedef enum
{
  CPU_ACTIVE,
  CPU_HALT,
  CPU_STOP
} CPU_HALT_TYPE;

typedef enum
{
  IRQ_NONE = 0x0000,
  IRQ_VBLANK = 0x0001,
  IRQ_HBLANK = 0x0002,
  IRQ_VCOUNT = 0x0004,
  IRQ_TIMER0 = 0x0008,
  IRQ_TIMER1 = 0x0010,
  IRQ_TIMER2 = 0x0020,
  IRQ_TIMER3 = 0x0040,
  IRQ_SERIAL = 0x0080,
  IRQ_DMA0 = 0x0100,
  IRQ_DMA1 = 0x0200,
  IRQ_DMA2 = 0x0400,
  IRQ_DMA3 = 0x0800,
  IRQ_KEYPAD = 0x1000,
  IRQ_GAMEPAK = 0x2000,
} IRQ_TYPE;

typedef enum
{
  REG_SP            = 13,
  REG_LR            = 14,
  REG_PC            = 15,
  REG_N_FLAG        = 16,
  REG_Z_FLAG        = 17,
  REG_C_FLAG        = 18,
  REG_V_FLAG        = 19,
  REG_CPSR          = 20,
  REG_SAVE          = 21,
  REG_SAVE2         = 22,
  REG_SAVE3         = 23,
  CPU_MODE          = 29,
  CPU_HALT_STATE    = 30,
  CHANGED_PC_STATUS = 31
} EXT_REG_NUMBERS;

typedef enum
{
  TRANSLATION_REGION_RAM,
  TRANSLATION_REGION_ROM,
  TRANSLATION_REGION_BIOS
} TRANSLATION_REGION_TYPE;

u32 execute_load_u8(u32 address);
u32 execute_load_u16(u32 address);
u32 execute_load_u32(u32 address);
u32 execute_load_s8(u32 address);
u32 execute_load_s16(u32 address);
void execute_store_u8(u32 address, u32 source);
void execute_store_u16(u32 address, u32 source);
void execute_store_u32(u32 address, u32 source);
void execute_arm_translate(u32 cycles);
void execute_arm(u32 cycles);

void init_translater();
void cpu_read_mem_savestate();
void cpu_write_mem_savestate();

void invalidate_all_cache();
void invalidate_icache_region(u8* addr, u32 length);

u8 *block_lookup_address_arm(u32 pc);
u8 *block_lookup_address_thumb(u32 pc);
u8 *block_lookup_address_dual(u32 pc);
s32 translate_block_arm(u32 pc, TRANSLATION_REGION_TYPE translation_region,
 u32 smc_enable);
s32 translate_block_thumb(u32 pc, TRANSLATION_REGION_TYPE translation_region,
 u32 smc_enable);

#define ROM_TRANSLATION_CACHE_SIZE  (1024 * 512 * 2)  /* 2048 KB 0x20 0000 */
#define RAM_TRANSLATION_CACHE_SIZE  (1024 * 384 * 1)  /*  384 KB 0x06 0000 現在の所 0x020000を超えた状況はない*/
#define BIOS_TRANSLATION_CACHE_SIZE (1024 * 128 * 1)  /*   32 KB 0x00 8000 現在の所 0x008000を超えた状況はない*/
#define TRANSLATION_CACHE_LIMIT_THRESHOLD (1024)

extern u8 rom_translation_cache[ROM_TRANSLATION_CACHE_SIZE];
extern u8 ram_translation_cache[RAM_TRANSLATION_CACHE_SIZE];
extern u8 bios_translation_cache[BIOS_TRANSLATION_CACHE_SIZE];
extern u8 *rom_translation_ptr;
extern u8 *ram_translation_ptr;
extern u8 *bios_translation_ptr;

#define MAX_TRANSLATION_GATES 8
#define MAX_IDLE_LOOPS 8

extern u32 idle_loop_targets;
extern u32 idle_loop_target_pc[MAX_IDLE_LOOPS];
extern u32 force_pc_update_target;
extern u32 iwram_stack_optimize;
//extern u32 allow_smc_ram_u8;
//extern u32 allow_smc_ram_u16;
//extern u32 allow_smc_ram_u32;
extern u32 direct_map_vram;
extern u32 translation_gate_targets;
extern u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];

extern u32 in_interrupt;

extern u32 bios_mode;

#define ROM_BRANCH_HASH_SIZE (1024 * 64)

extern u32 *rom_branch_hash[ROM_BRANCH_HASH_SIZE];

void flush_translation_cache_rom();
void flush_translation_cache_ram();
void flush_translation_cache_bios();
void dump_translation_cache();

extern u32 reg_mode[7][7];
extern u32 spsr[6];

extern u32 cpu_modes[32];

void init_cpu();

#endif
