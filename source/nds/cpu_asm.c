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

// Not-so-important todo:
// - stm reglist writeback when base is in the list needs adjustment
// - block memory needs psr swapping and user mode reg swapping

#include "common.h"
#include "cpu_common.h"
  
// When a mode change occurs from non-FIQ to non-FIQ retire the current
// reg[13] and reg[14] into reg_mode[cpu_mode][5] and reg_mode[cpu_mode][6]
// respectively and load into reg[13] and reg[14] reg_mode[new_mode][5] and
// reg_mode[new_mode][6]. When swapping to/from FIQ retire/load reg[8]
// through reg[14] to/from reg_mode[MODE_FIQ][0] through reg_mode[MODE_FIQ][6].

u32 reg_mode[7][7];
 
const u8 cpu_modes[32] =
{
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID,
  MODE_USER   , MODE_FIQ    , MODE_IRQ    , MODE_SUPERVISOR,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_ABORT,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_UNDEFINED,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_USER
};

const u8 cpu_modes_cpsr[7] = { 0x10, 0x11, 0x12, 0x13, 0x17, 0x1B, 0x1F };

// When switching modes set spsr[new_mode] to cpsr. Modifying PC as the
// target of a data proc instruction will set cpsr to spsr[cpu_mode].
u32 spsr[6];

// ARM/Thumb mode is stored in the flags directly, this is simpler than
// shadowing it since it has a constant 1bit represenation.

char *reg_names[16] =
{
  " r0", " r1", " r2", " r3", " r4", " r5", " r6", " r7",
  " r8", " r9", "r10", " fp", " ip", " sp", " lr", " pc"
};

u32 output_field = 0;

u32 last_instruction = 0;

/* These are code caches. */
u8 readonly_code_cache[READONLY_CODE_CACHE_SIZE];
u8 writable_code_cache[WRITABLE_CODE_CACHE_SIZE];
u8 *readonly_next_code;
u8 *writable_next_code;

/* These are metadata. */
u32 iwram_code_min = 0xFFFFFFFF;
u32 iwram_code_max = 0xFFFFFFFF;
u32 ewram_code_min = 0xFFFFFFFF;
u32 ewram_code_max = 0xFFFFFFFF;
u32 vram_code_min  = 0xFFFFFFFF;
u32 vram_code_max  = 0xFFFFFFFF;
u32 *rom_branch_hash[ROM_BRANCH_HASH_SIZE];
u32 *writable_area_checksum_hash[PERSISTENT_RAM_HASH_SIZE];

// Default
u32 idle_loop_targets = 0;
u32 idle_loop_target_pc[MAX_IDLE_LOOPS];
u32 force_pc_update_target = 0xFFFFFFFF;
u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];
u32 translation_gate_targets = 0;
u32 iwram_stack_optimize = 1;
//u32 allow_smc_ram_u8 = 1;
//u32 allow_smc_ram_u16 = 1;
//u32 allow_smc_ram_u32 = 1;

// u32 bios_mode;

typedef struct
{
  u8 *block_offset;
  u16 flag_data;
  u8 condition;
  u8 update_cycles;
} block_data_arm_type;

typedef struct
{
  u8 *block_offset;
  u16 flag_data;
  u8 update_cycles;
} block_data_thumb_type;

typedef struct
{
  u32 branch_target;
  u8 *branch_source;
} block_exit_type;

#define arm_decode_data_proc_reg()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_data_proc_imm()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 imm;                                                                    \
  ROR(imm, opcode & 0xFF, (opcode >> 7) & 0x1E)                               \

#define arm_decode_psr_reg()                                                  \
  u32 psr_field = (opcode >> 16) & 0x0F;                                      \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_psr_imm()                                                  \
  u32 psr_field = (opcode >> 16) & 0x0F;                                      \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 imm;                                                                    \
  ROR(imm, opcode & 0xFF, (opcode >> 7) & 0x1E)                               \

#define arm_decode_branchx()                                                  \
  u32 rn = opcode & 0x0F                                                      \

#define arm_decode_multiply()                                                 \
  u32 rd = (opcode >> 16) & 0x0F;                                             \
  u32 rn = (opcode >> 12) & 0x0F;                                             \
  u32 rs = (opcode >> 8) & 0x0F;                                              \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_multiply_long()                                            \
  u32 rdhi = (opcode >> 16) & 0x0F;                                           \
  u32 rdlo = (opcode >> 12) & 0x0F;                                           \
  u32 rs = (opcode >> 8) & 0x0F;                                              \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_swap()                                                     \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_half_trans_r()                                             \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_half_trans_of()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 offset = ((opcode >> 4) & 0xF0) | (opcode & 0x0F)                       \

#define arm_decode_data_trans_imm()                                           \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 offset = opcode & 0x0FFF                                                \

#define arm_decode_data_trans_reg()                                           \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_block_trans()                                              \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 reg_list = opcode & 0xFFFF                                              \

#define arm_decode_branch()                                                   \
  s32 offset = (s32)(opcode << 8) >> 6                                        \

#define thumb_decode_shift()                                                  \
  u32 imm = (opcode >> 6) & 0x1F;                                             \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_add_sub()                                                \
  u32 rn = (opcode >> 6) & 0x07;                                              \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_add_sub_imm()                                            \
  u32 imm = (opcode >> 6) & 0x07;                                             \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_imm()                                                    \
  u32 imm = opcode & 0xFF                                                     \

#define thumb_decode_alu_op()                                                 \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_hireg_op()                                               \
  u32 rs = (opcode >> 3) & 0x0F;                                              \
  u32 rd = ((opcode >> 4) & 0x08) | (opcode & 0x07)                           \

#define thumb_decode_mem_reg()                                                \
  u32 ro = (opcode >> 6) & 0x07;                                              \
  u32 rb = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_mem_imm()                                                \
  u32 imm = (opcode >> 6) & 0x1F;                                             \
  u32 rb = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_add_sp()                                                 \
  u32 imm = opcode & 0x7F                                                     \

#define thumb_decode_rlist()                                                  \
  u32 reg_list = opcode & 0xFF                                                \

#define thumb_decode_branch_cond()                                            \
  s32 offset = (s8)(opcode & 0xFF)                                            \

#define thumb_decode_swi()                                                    \
  u32 comment = opcode & 0xFF                                                 \

#define thumb_decode_branch()                                                 \
  u32 offset = opcode & 0x07FF                                                \

extern void call_bios_hle(void* func);

#ifdef OLD_COUNT
#include "mips_emit.h"
#else
#include "mips_emit_new_count.h"
#endif

#define check_pc_region(pc)                                                   \
  new_pc_region = (pc >> 15);                                                 \
  if(new_pc_region != pc_region)                                              \
  {                                                                           \
    pc_region = new_pc_region;                                                \
    pc_address_block = memory_map_read[new_pc_region];                        \
                                                                              \
    if(pc_address_block == NULL)                                              \
      pc_address_block = load_gamepak_page(pc_region & 0x3FF);                \
  }                                                                           \

#ifdef PERFORMANCE_IMPACTING_STATISTICS
static inline void StatsAddWritableReuse(u32 Opcodes)
{
	Stats.BlockReuseCount++;
	Stats.OpcodeReuseCount += Opcodes;
}

static inline void StatsAddWritableRecompilation(u32 Opcodes)
{
	Stats.BlockRecompilationCount++;
	Stats.OpcodeRecompilationCount += Opcodes;
}
#else
static inline void StatsAddWritableReuse(u32 Opcodes) {}

static inline void StatsAddWritableRecompilation(u32 Opcodes) {}
#endif

#define translate_arm_instruction()                                           \
  opcode = opcodes.arm[block_data_position];                                  \
  condition = block_data.arm[block_data_position].condition;                  \
  u32 has_condition_header = 0;                                               \
                                                                              \
  if((condition != 0x0E) || (condition >= 0x20))                              \
  {                                                                           \
    condition &= 0x0F;                                                        \
                                                                              \
    if(condition != 0x0E)                                                     \
    {                                                                         \
      arm_conditional_block_header();                                         \
      has_condition_header = 1;                                               \
    }                                                                         \
  }                                                                           \
                                                                              \
  switch((opcode >> 20) & 0xFF)                                               \
  {                                                                           \
    case 0x00:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], -rm */                                            \
          arm_access_memory(store, down, post, u16, half_reg);                \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* MUL rd, rm, rs */                                                \
          arm_multiply(no, no);                                               \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* AND rd, rn, reg_op */                                              \
        arm_data_proc(and, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x01:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* MULS rd, rm, rs */                                             \
            arm_multiply(no, yes);                                            \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], -rm */                                          \
            arm_access_memory(load, down, post, u16, half_reg);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s8, half_reg);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s16, half_reg);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ANDS rd, rn, reg_op */                                             \
        arm_data_proc(ands, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x02:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], -rm */                                            \
          arm_access_memory(store, down, post, u16, half_reg);                \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* MLA rd, rm, rs, rn */                                            \
          arm_multiply(yes, no);                                              \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* EOR rd, rn, reg_op */                                              \
        arm_data_proc(eor, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x03:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* MLAS rd, rm, rs, rn */                                         \
            arm_multiply(yes, yes);                                           \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], -rm */                                          \
            arm_access_memory(load, down, post, u16, half_reg);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s8, half_reg);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s16, half_reg);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* EORS rd, rn, reg_op */                                             \
        arm_data_proc(eors, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x04:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn], -imm */                                             \
        arm_access_memory(store, down, post, u16, half_imm);                  \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SUB rd, rn, reg_op */                                              \
        arm_data_proc(sub, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x05:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn], -imm */                                         \
            arm_access_memory(load, down, post, u16, half_imm);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s8, half_imm);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s16, half_imm);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SUBS rd, rn, reg_op */                                             \
        arm_data_proc(subs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x06:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn], -imm */                                             \
        arm_access_memory(store, down, post, u16, half_imm);                  \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSB rd, rn, reg_op */                                              \
        arm_data_proc(rsb, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x07:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn], -imm */                                         \
            arm_access_memory(load, down, post, u16, half_imm);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s8, half_imm);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s16, half_imm);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSBS rd, rn, reg_op */                                             \
        arm_data_proc(rsbs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x08:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +rm */                                            \
          arm_access_memory(store, up, post, u16, half_reg);                  \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* UMULL rd, rm, rs */                                              \
          arm_multiply_long(u64, no, no);                                     \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADD rd, rn, reg_op */                                              \
        arm_data_proc(add, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x09:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* UMULLS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(u64, no, yes);                                  \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +rm */                                          \
            arm_access_memory(load, up, post, u16, half_reg);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s8, half_reg);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s16, half_reg);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADDS rd, rn, reg_op */                                             \
        arm_data_proc(adds, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0A:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +rm */                                            \
          arm_access_memory(store, up, post, u16, half_reg);                  \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* UMLAL rd, rm, rs */                                              \
          arm_multiply_long(u64_add, yes, no);                                \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADC rd, rn, reg_op */                                              \
        arm_data_proc(adc, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0B:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* UMLALS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(u64_add, yes, yes);                             \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +rm */                                          \
            arm_access_memory(load, up, post, u16, half_reg);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s8, half_reg);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s16, half_reg);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADCS rd, rn, reg_op */                                             \
        arm_data_proc(adcs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0C:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +imm */                                           \
          arm_access_memory(store, up, post, u16, half_imm);                  \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SMULL rd, rm, rs */                                              \
          arm_multiply_long(s64, no, no);                                     \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SBC rd, rn, reg_op */                                              \
        arm_data_proc(sbc, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0D:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* SMULLS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(s64, no, yes);                                  \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +imm */                                         \
            arm_access_memory(load, up, post, u16, half_imm);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s8, half_imm);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s16, half_imm);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SBCS rd, rn, reg_op */                                             \
        arm_data_proc(sbcs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0E:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +imm */                                           \
          arm_access_memory(store, up, post, u16, half_imm);                  \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SMLAL rd, rm, rs */                                              \
          arm_multiply_long(s64_add, yes, no);                                \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSC rd, rn, reg_op */                                              \
        arm_data_proc(rsc, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0F:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* SMLALS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(s64_add, yes, yes);                             \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +imm */                                         \
            arm_access_memory(load, up, post, u16, half_imm);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s8, half_imm);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s16, half_imm);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSCS rd, rn, reg_op */                                             \
        arm_data_proc(rscs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x10:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn - rm] */                                            \
          arm_access_memory(store, down, pre, u16, half_reg);                 \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SWP rd, rm, [rn] */                                              \
          arm_swap(u32);                                                      \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MRS rd, cpsr */                                                    \
        arm_psr(reg, read, cpsr);                                             \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x11:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - rm] */                                          \
            arm_access_memory(load, down, pre, u16, half_reg);                \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - rm] */                                         \
            arm_access_memory(load, down, pre, s8, half_reg);                 \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - rm] */                                         \
            arm_access_memory(load, down, pre, s16, half_reg);                \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* TST rd, rn, reg_op */                                              \
        arm_data_proc_test(tst, reg_flags);                                   \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x12:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn - rm]! */                                             \
        arm_access_memory(store, down, pre_wb, u16, half_reg);                \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        if(opcode & 0x10)                                                     \
        {                                                                     \
          /* BX rn */                                                         \
          arm_bx();                                                           \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* MSR cpsr, rm */                                                  \
          arm_psr(reg, store, cpsr);                                          \
        }                                                                     \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x13:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - rm]! */                                         \
            arm_access_memory(load, down, pre_wb, u16, half_reg);             \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - rm]! */                                        \
            arm_access_memory(load, down, pre_wb, s8, half_reg);              \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - rm]! */                                        \
            arm_access_memory(load, down, pre_wb, s16, half_reg);             \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* TEQ rd, rn, reg_op */                                              \
        arm_data_proc_test(teq, reg_flags);                                   \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x14:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn - imm] */                                           \
          arm_access_memory(store, down, pre, u16, half_imm);                 \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SWPB rd, rm, [rn] */                                             \
          arm_swap(u8);                                                       \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MRS rd, spsr */                                                    \
        arm_psr(reg, read, spsr);                                             \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x15:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - imm] */                                         \
            arm_access_memory(load, down, pre, u16, half_imm);                \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - imm] */                                        \
            arm_access_memory(load, down, pre, s8, half_imm);                 \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - imm] */                                        \
            arm_access_memory(load, down, pre, s16, half_imm);                \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* CMP rn, reg_op */                                                  \
        arm_data_proc_test(cmp, reg);                                         \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x16:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn - imm]! */                                            \
        arm_access_memory(store, down, pre_wb, u16, half_imm);                \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MSR spsr, rm */                                                    \
        arm_psr(reg, store, spsr);                                            \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x17:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - imm]! */                                        \
            arm_access_memory(load, down, pre_wb, u16, half_imm);             \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - imm]! */                                       \
            arm_access_memory(load, down, pre_wb, s8, half_imm);              \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - imm]! */                                       \
            arm_access_memory(load, down, pre_wb, s16, half_imm);             \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* CMN rd, rn, reg_op */                                              \
        arm_data_proc_test(cmn, reg);                                         \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x18:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + rm] */                                              \
        arm_access_memory(store, up, pre, u16, half_reg);                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ORR rd, rn, reg_op */                                              \
        arm_data_proc(orr, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x19:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + rm] */                                          \
            arm_access_memory(load, up, pre, u16, half_reg);                  \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + rm] */                                         \
            arm_access_memory(load, up, pre, s8, half_reg);                   \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + rm] */                                         \
            arm_access_memory(load, up, pre, s16, half_reg);                  \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ORRS rd, rn, reg_op */                                             \
        arm_data_proc(orrs, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1A:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + rm]! */                                             \
        arm_access_memory(store, up, pre_wb, u16, half_reg);                  \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MOV rd, reg_op */                                                  \
        arm_data_proc_unary(mov, reg, no_flags);                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1B:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + rm]! */                                         \
            arm_access_memory(load, up, pre_wb, u16, half_reg);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + rm]! */                                        \
            arm_access_memory(load, up, pre_wb, s8, half_reg);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + rm]! */                                        \
            arm_access_memory(load, up, pre_wb, s16, half_reg);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MOVS rd, reg_op */                                                 \
        arm_data_proc_unary(movs, reg_flags, flags);                          \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1C:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + imm] */                                             \
        arm_access_memory(store, up, pre, u16, half_imm);                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* BIC rd, rn, reg_op */                                              \
        arm_data_proc(bic, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1D:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + imm] */                                         \
            arm_access_memory(load, up, pre, u16, half_imm);                  \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + imm] */                                        \
            arm_access_memory(load, up, pre, s8, half_imm);                   \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + imm] */                                        \
            arm_access_memory(load, up, pre, s16, half_imm);                  \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* BICS rd, rn, reg_op */                                             \
        arm_data_proc(bics, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1E:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + imm]! */                                            \
        arm_access_memory(store, up, pre_wb, u16, half_imm);                  \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MVN rd, reg_op */                                                  \
        arm_data_proc_unary(mvn, reg, no_flags);                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1F:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + imm]! */                                        \
            arm_access_memory(load, up, pre_wb, u16, half_imm);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + imm]! */                                       \
            arm_access_memory(load, up, pre_wb, s8, half_imm);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + imm]! */                                       \
            arm_access_memory(load, up, pre_wb, s16, half_imm);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MVNS rd, rn, reg_op */                                             \
        arm_data_proc_unary(mvns, reg_flags, flags);                          \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x20:                                                                \
      /* AND rd, rn, imm */                                                   \
      arm_data_proc(and, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x21:                                                                \
      /* ANDS rd, rn, imm */                                                  \
      arm_data_proc(ands, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x22:                                                                \
      /* EOR rd, rn, imm */                                                   \
      arm_data_proc(eor, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x23:                                                                \
      /* EORS rd, rn, imm */                                                  \
      arm_data_proc(eors, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x24:                                                                \
      /* SUB rd, rn, imm */                                                   \
      arm_data_proc(sub, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x25:                                                                \
      /* SUBS rd, rn, imm */                                                  \
      arm_data_proc(subs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x26:                                                                \
      /* RSB rd, rn, imm */                                                   \
      arm_data_proc(rsb, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x27:                                                                \
      /* RSBS rd, rn, imm */                                                  \
      arm_data_proc(rsbs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x28:                                                                \
      /* ADD rd, rn, imm */                                                   \
      arm_data_proc(add, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x29:                                                                \
      /* ADDS rd, rn, imm */                                                  \
      arm_data_proc(adds, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x2A:                                                                \
      /* ADC rd, rn, imm */                                                   \
      arm_data_proc(adc, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x2B:                                                                \
      /* ADCS rd, rn, imm */                                                  \
      arm_data_proc(adcs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x2C:                                                                \
      /* SBC rd, rn, imm */                                                   \
      arm_data_proc(sbc, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x2D:                                                                \
      /* SBCS rd, rn, imm */                                                  \
      arm_data_proc(sbcs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x2E:                                                                \
      /* RSC rd, rn, imm */                                                   \
      arm_data_proc(rsc, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x2F:                                                                \
      /* RSCS rd, rn, imm */                                                  \
      arm_data_proc(rscs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x30 ... 0x31:                                                       \
      /* TST rn, imm */                                                       \
      arm_data_proc_test(tst, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x32:                                                                \
      /* MSR cpsr, imm */                                                     \
      arm_psr(imm, store, cpsr);                                              \
      break;                                                                  \
                                                                              \
    case 0x33:                                                                \
      /* TEQ rn, imm */                                                       \
      arm_data_proc_test(teq, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x34 ... 0x35:                                                       \
      /* CMP rn, imm */                                                       \
      arm_data_proc_test(cmp, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x36:                                                                \
      /* MSR spsr, imm */                                                     \
      arm_psr(imm, store, spsr);                                              \
      break;                                                                  \
                                                                              \
    case 0x37:                                                                \
      /* CMN rn, imm */                                                       \
      arm_data_proc_test(cmn, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x38:                                                                \
      /* ORR rd, rn, imm */                                                   \
      arm_data_proc(orr, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x39:                                                                \
      /* ORRS rd, rn, imm */                                                  \
      arm_data_proc(orrs, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x3A:                                                                \
      /* MOV rd, imm */                                                       \
      arm_data_proc_unary(mov, imm, no_flags);                                \
      break;                                                                  \
                                                                              \
    case 0x3B:                                                                \
      /* MOVS rd, imm */                                                      \
      arm_data_proc_unary(movs, imm_flags, flags);                            \
      break;                                                                  \
                                                                              \
    case 0x3C:                                                                \
      /* BIC rd, rn, imm */                                                   \
      arm_data_proc(bic, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x3D:                                                                \
      /* BICS rd, rn, imm */                                                  \
      arm_data_proc(bics, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x3E:                                                                \
      /* MVN rd, imm */                                                       \
      arm_data_proc_unary(mvn, imm, no_flags);                                \
      break;                                                                  \
                                                                              \
    case 0x3F:                                                                \
      /* MVNS rd, imm */                                                      \
      arm_data_proc_unary(mvns, imm_flags, flags);                            \
      break;                                                                  \
                                                                              \
    case 0x40:                                                                \
      /* STR rd, [rn], -imm */                                                \
    case 0x42:                                                                \
      /* STRT rd, [rn], -imm */                                               \
      arm_access_memory(store, down, post, u32, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x41:                                                                \
      /* LDR rd, [rn], -imm */                                                \
    case 0x43:                                                                \
      /* LDRT rd, [rn], -imm */                                               \
      arm_access_memory(load, down, post, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x44:                                                                \
      /* STRB rd, [rn], -imm */                                               \
    case 0x46:                                                                \
      /* STRBT rd, [rn], -imm */                                              \
      arm_access_memory(store, down, post, u8, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x45:                                                                \
      /* LDRB rd, [rn], -imm */                                               \
    case 0x47:                                                                \
      /* LDRBT rd, [rn], -imm */                                              \
      arm_access_memory(load, down, post, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x48:                                                                \
      /* STR rd, [rn], +imm */                                                \
    case 0x4A:                                                                \
      /* STRT rd, [rn], +imm */                                               \
      arm_access_memory(store, up, post, u32, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x49:                                                                \
      /* LDR rd, [rn], +imm */                                                \
    case 0x4B:                                                                \
      /* LDRT rd, [rn], +imm */                                               \
      arm_access_memory(load, up, post, u32, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4C:                                                                \
      /* STRB rd, [rn], +imm */                                               \
    case 0x4E:                                                                \
      /* STRBT rd, [rn], +imm */                                              \
      arm_access_memory(store, up, post, u8, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4D:                                                                \
      /* LDRB rd, [rn], +imm */                                               \
    case 0x4F:                                                                \
      /* LDRBT rd, [rn], +imm */                                              \
      arm_access_memory(load, up, post, u8, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x50:                                                                \
      /* STR rd, [rn - imm] */                                                \
      arm_access_memory(store, down, pre, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x51:                                                                \
      /* LDR rd, [rn - imm] */                                                \
      arm_access_memory(load, down, pre, u32, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x52:                                                                \
      /* STR rd, [rn - imm]! */                                               \
      arm_access_memory(store, down, pre_wb, u32, imm);                       \
      break;                                                                  \
                                                                              \
    case 0x53:                                                                \
      /* LDR rd, [rn - imm]! */                                               \
      arm_access_memory(load, down, pre_wb, u32, imm);                        \
      break;                                                                  \
                                                                              \
    case 0x54:                                                                \
      /* STRB rd, [rn - imm] */                                               \
      arm_access_memory(store, down, pre, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x55:                                                                \
      /* LDRB rd, [rn - imm] */                                               \
      arm_access_memory(load, down, pre, u8, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x56:                                                                \
      /* STRB rd, [rn - imm]! */                                              \
      arm_access_memory(store, down, pre_wb, u8, imm);                        \
      break;                                                                  \
                                                                              \
    case 0x57:                                                                \
      /* LDRB rd, [rn - imm]! */                                              \
      arm_access_memory(load, down, pre_wb, u8, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x58:                                                                \
      /* STR rd, [rn + imm] */                                                \
      arm_access_memory(store, up, pre, u32, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x59:                                                                \
      /* LDR rd, [rn + imm] */                                                \
      arm_access_memory(load, up, pre, u32, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x5A:                                                                \
      /* STR rd, [rn + imm]! */                                               \
      arm_access_memory(store, up, pre_wb, u32, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x5B:                                                                \
      /* LDR rd, [rn + imm]! */                                               \
      arm_access_memory(load, up, pre_wb, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x5C:                                                                \
      /* STRB rd, [rn + imm] */                                               \
      arm_access_memory(store, up, pre, u8, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x5D:                                                                \
      /* LDRB rd, [rn + imm] */                                               \
      arm_access_memory(load, up, pre, u8, imm);                              \
      break;                                                                  \
                                                                              \
    case 0x5E:                                                                \
      /* STRB rd, [rn + imm]! */                                              \
      arm_access_memory(store, up, pre_wb, u8, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x5F:                                                                \
      /* LDRBT rd, [rn + imm]! */                                             \
      arm_access_memory(load, up, pre_wb, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x60:                                                                \
      /* STR rd, [rn], -rm */                                                 \
    case 0x62:                                                                \
      /* STRT rd, [rn], -rm */                                                \
      arm_access_memory(store, down, post, u32, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x61:                                                                \
      /* LDR rd, [rn], -rm */                                                 \
    case 0x63:                                                                \
      /* LDRT rd, [rn], -rm */                                                \
      arm_access_memory(load, down, post, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x64:                                                                \
      /* STRB rd, [rn], -rm */                                                \
    case 0x66:                                                                \
      /* STRBT rd, [rn], -rm */                                               \
      arm_access_memory(store, down, post, u8, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x65:                                                                \
      /* LDRB rd, [rn], -rm */                                                \
    case 0x67:                                                                \
      /* LDRBT rd, [rn], -rm */                                               \
      arm_access_memory(load, down, post, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x68:                                                                \
      /* STR rd, [rn], +rm */                                                 \
    case 0x6A:                                                                \
      /* STRT rd, [rn], +rm */                                                \
      arm_access_memory(store, up, post, u32, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x69:                                                                \
      /* LDR rd, [rn], +rm */                                                 \
    case 0x6B:                                                                \
      /* LDRT rd, [rn], +rm */                                                \
      arm_access_memory(load, up, post, u32, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6C:                                                                \
      /* STRB rd, [rn], +rm */                                                \
    case 0x6E:                                                                \
      /* STRBT rd, [rn], +rm */                                               \
      arm_access_memory(store, up, post, u8, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6D:                                                                \
      /* LDRB rd, [rn], +rm */                                                \
    case 0x6F:                                                                \
      /* LDRBT rd, [rn], +rm */                                               \
      arm_access_memory(load, up, post, u8, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x70:                                                                \
      /* STR rd, [rn - rm] */                                                 \
      arm_access_memory(store, down, pre, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x71:                                                                \
      /* LDR rd, [rn - rm] */                                                 \
      arm_access_memory(load, down, pre, u32, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x72:                                                                \
      /* STR rd, [rn - rm]! */                                                \
      arm_access_memory(store, down, pre_wb, u32, reg);                       \
      break;                                                                  \
                                                                              \
    case 0x73:                                                                \
      /* LDR rd, [rn - rm]! */                                                \
      arm_access_memory(load, down, pre_wb, u32, reg);                        \
      break;                                                                  \
                                                                              \
    case 0x74:                                                                \
      /* STRB rd, [rn - rm] */                                                \
      arm_access_memory(store, down, pre, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x75:                                                                \
      /* LDRB rd, [rn - rm] */                                                \
      arm_access_memory(load, down, pre, u8, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x76:                                                                \
      /* STRB rd, [rn - rm]! */                                               \
      arm_access_memory(store, down, pre_wb, u8, reg);                        \
      break;                                                                  \
                                                                              \
    case 0x77:                                                                \
      /* LDRB rd, [rn - rm]! */                                               \
      arm_access_memory(load, down, pre_wb, u8, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x78:                                                                \
      /* STR rd, [rn + rm] */                                                 \
      arm_access_memory(store, up, pre, u32, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x79:                                                                \
      /* LDR rd, [rn + rm] */                                                 \
      arm_access_memory(load, up, pre, u32, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x7A:                                                                \
      /* STR rd, [rn + rm]! */                                                \
      arm_access_memory(store, up, pre_wb, u32, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x7B:                                                                \
      /* LDR rd, [rn + rm]! */                                                \
      arm_access_memory(load, up, pre_wb, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x7C:                                                                \
      /* STRB rd, [rn + rm] */                                                \
      arm_access_memory(store, up, pre, u8, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x7D:                                                                \
      /* LDRB rd, [rn + rm] */                                                \
      arm_access_memory(load, up, pre, u8, reg);                              \
      break;                                                                  \
                                                                              \
    case 0x7E:                                                                \
      /* STRB rd, [rn + rm]! */                                               \
      arm_access_memory(store, up, pre_wb, u8, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x7F:                                                                \
      /* LDRBT rd, [rn + rm]! */                                              \
      arm_access_memory(load, up, pre_wb, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x80:                                                                \
      /* STMDA rn, rlist */                                                   \
      arm_block_memory(store, down_a, no, no);                                \
      break;                                                                  \
                                                                              \
    case 0x81:                                                                \
      /* LDMDA rn, rlist */                                                   \
      arm_block_memory(load, down_a, no, no);                                 \
      break;                                                                  \
                                                                              \
    case 0x82:                                                                \
      /* STMDA rn!, rlist */                                                  \
      arm_block_memory(store, down_a, down, no);                              \
      break;                                                                  \
                                                                              \
    case 0x83:                                                                \
      /* LDMDA rn!, rlist */                                                  \
      arm_block_memory(load, down_a, down, no);                               \
      break;                                                                  \
                                                                              \
    case 0x84:                                                                \
      /* STMDA rn, rlist^ */                                                  \
      arm_block_memory(store, down_a, no, yes);                               \
      break;                                                                  \
                                                                              \
    case 0x85:                                                                \
      /* LDMDA rn, rlist^ */                                                  \
      arm_block_memory(load, down_a, no, yes);                                \
      break;                                                                  \
                                                                              \
    case 0x86:                                                                \
      /* STMDA rn!, rlist^ */                                                 \
      arm_block_memory(store, down_a, down, yes);                             \
      break;                                                                  \
                                                                              \
    case 0x87:                                                                \
      /* LDMDA rn!, rlist^ */                                                 \
      arm_block_memory(load, down_a, down, yes);                              \
      break;                                                                  \
                                                                              \
    case 0x88:                                                                \
      /* STMIA rn, rlist */                                                   \
      arm_block_memory(store, no, no, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x89:                                                                \
      /* LDMIA rn, rlist */                                                   \
      arm_block_memory(load, no, no, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x8A:                                                                \
      /* STMIA rn!, rlist */                                                  \
      arm_block_memory(store, no, up, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x8B:                                                                \
      /* LDMIA rn!, rlist */                                                  \
      arm_block_memory(load, no, up, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x8C:                                                                \
      /* STMIA rn, rlist^ */                                                  \
      arm_block_memory(store, no, no, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x8D:                                                                \
      /* LDMIA rn, rlist^ */                                                  \
      arm_block_memory(load, no, no, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0x8E:                                                                \
      /* STMIA rn!, rlist^ */                                                 \
      arm_block_memory(store, no, up, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x8F:                                                                \
      /* LDMIA rn!, rlist^ */                                                 \
      arm_block_memory(load, no, up, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0x90:                                                                \
      /* STMDB rn, rlist */                                                   \
      arm_block_memory(store, down_b, no, no);                                \
      break;                                                                  \
                                                                              \
    case 0x91:                                                                \
      /* LDMDB rn, rlist */                                                   \
      arm_block_memory(load, down_b, no, no);                                 \
      break;                                                                  \
                                                                              \
    case 0x92:                                                                \
      /* STMDB rn!, rlist */                                                  \
      arm_block_memory(store, down_b, down, no);                              \
      break;                                                                  \
                                                                              \
    case 0x93:                                                                \
      /* LDMDB rn!, rlist */                                                  \
      arm_block_memory(load, down_b, down, no);                               \
      break;                                                                  \
                                                                              \
    case 0x94:                                                                \
      /* STMDB rn, rlist^ */                                                  \
      arm_block_memory(store, down_b, no, yes);                               \
      break;                                                                  \
                                                                              \
    case 0x95:                                                                \
      /* LDMDB rn, rlist^ */                                                  \
      arm_block_memory(load, down_b, no, yes);                                \
      break;                                                                  \
                                                                              \
    case 0x96:                                                                \
      /* STMDB rn!, rlist^ */                                                 \
      arm_block_memory(store, down_b, down, yes);                             \
      break;                                                                  \
                                                                              \
    case 0x97:                                                                \
      /* LDMDB rn!, rlist^ */                                                 \
      arm_block_memory(load, down_b, down, yes);                              \
      break;                                                                  \
                                                                              \
    case 0x98:                                                                \
      /* STMIB rn, rlist */                                                   \
      arm_block_memory(store, up, no, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x99:                                                                \
      /* LDMIB rn, rlist */                                                   \
      arm_block_memory(load, up, no, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x9A:                                                                \
      /* STMIB rn!, rlist */                                                  \
      arm_block_memory(store, up, up, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x9B:                                                                \
      /* LDMIB rn!, rlist */                                                  \
      arm_block_memory(load, up, up, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x9C:                                                                \
      /* STMIB rn, rlist^ */                                                  \
      arm_block_memory(store, up, no, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x9D:                                                                \
      /* LDMIB rn, rlist^ */                                                  \
      arm_block_memory(load, up, no, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0x9E:                                                                \
      /* STMIB rn!, rlist^ */                                                 \
      arm_block_memory(store, up, up, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x9F:                                                                \
      /* LDMIB rn!, rlist^ */                                                 \
      arm_block_memory(load, up, up, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0xA0 ... 0xAF:                                                       \
    {                                                                         \
      /* B offset */                                                          \
      arm_b();                                                                \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xB0 ... 0xBF:                                                       \
    {                                                                         \
      /* BL offset */                                                         \
      arm_bl();                                                               \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xC0 ... 0xEF:                                                       \
      /* coprocessor instructions, reserved on GBA */                         \
      break;                                                                  \
                                                                              \
    case 0xF0 ... 0xFF:                                                       \
    {                                                                         \
      /* SWI comment */                                                       \
      arm_swi();                                                              \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  if(has_condition_header)                                                    \
  {                                                                           \
    generate_branch_patch_conditional(backpatch_address, translation_ptr);    \
  }                                                                           \
                                                                              \
  pc += 4                                                                     \

static void arm_flag_status(block_data_arm_type* block_data, u32 opcode)
{
}

#define translate_thumb_instruction()                                         \
  flag_status = block_data.thumb[block_data_position].flag_data;              \
  last_opcode = opcode;                                                       \
  opcode = opcodes.thumb[block_data_position];                                \
                                                                              \
  switch((opcode >> 8) & 0xFF)                                                \
  {                                                                           \
    case 0x00 ... 0x07:                                                       \
      /* LSL rd, rs, imm */                                                   \
      thumb_shift(shift, lsl, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x08 ... 0x0F:                                                       \
      /* LSR rd, rs, imm */                                                   \
      thumb_shift(shift, lsr, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x10 ... 0x17:                                                       \
      /* ASR rd, rs, imm */                                                   \
      thumb_shift(shift, asr, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x18 ... 0x19:                                                       \
      /* ADD rd, rs, rn */                                                    \
      thumb_data_proc(add_sub, adds, reg, rd, rs, rn);                        \
      break;                                                                  \
                                                                              \
    case 0x1A ... 0x1B:                                                       \
      /* SUB rd, rs, rn */                                                    \
      thumb_data_proc(add_sub, subs, reg, rd, rs, rn);                        \
      break;                                                                  \
                                                                              \
    case 0x1C ... 0x1D:                                                       \
      /* ADD rd, rs, imm */                                                   \
      thumb_data_proc(add_sub_imm, adds, imm, rd, rs, imm);                   \
      break;                                                                  \
                                                                              \
    case 0x1E ... 0x1F:                                                       \
      /* SUB rd, rs, imm */                                                   \
      thumb_data_proc(add_sub_imm, subs, imm, rd, rs, imm);                   \
      break;                                                                  \
                                                                              \
    case 0x20 ... 0x27:                                                       \
      /* MOV r0..r7, imm */                                                   \
      thumb_data_proc_unary(imm, movs, imm, (opcode >> 8) & 0xFF - 0x20, imm);\
      break;                                                                  \
                                                                              \
    case 0x28 ... 0x2F:                                                       \
      /* CMP r0..r7, imm */                                                   \
      thumb_data_proc_test(imm, cmp, imm, (opcode >> 8) & 0xFF - 0x28, imm);  \
      break;                                                                  \
                                                                              \
    case 0x30 ... 0x37:                                                       \
      /* ADD r0..r7, imm */                                                   \
      thumb_data_proc(imm, adds, imm, (opcode >> 8) & 0xFF - 0x30,            \
        (opcode >> 8) & 0xFF - 0x30, imm);                                    \
      break;                                                                  \
                                                                              \
    case 0x38 ... 0x3F:                                                       \
      /* SUB r0..r7, imm */                                                   \
      thumb_data_proc(imm, subs, imm, (opcode >> 8) & 0xFF - 0x38,            \
        (opcode >> 8) & 0xFF - 0x38, imm);                                    \
      break;                                                                  \
                                                                              \
    case 0x40:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* AND rd, rs */                                                    \
          thumb_data_proc(alu_op, ands, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* EOR rd, rs */                                                    \
          thumb_data_proc(alu_op, eors, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* LSL rd, rs */                                                    \
          thumb_shift(alu_op, lsl, reg);                                      \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* LSR rd, rs */                                                    \
          thumb_shift(alu_op, lsr, reg);                                      \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x41:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* ASR rd, rs */                                                    \
          thumb_shift(alu_op, asr, reg);                                      \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* ADC rd, rs */                                                    \
          thumb_data_proc(alu_op, adcs, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* SBC rd, rs */                                                    \
          thumb_data_proc(alu_op, sbcs, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* ROR rd, rs */                                                    \
          thumb_shift(alu_op, ror, reg);                                      \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x42:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* TST rd, rs */                                                    \
          thumb_data_proc_test(alu_op, tst, reg, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* NEG rd, rs */                                                    \
          thumb_data_proc_unary(alu_op, neg, reg, rd, rs);                    \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* CMP rd, rs */                                                    \
          thumb_data_proc_test(alu_op, cmp, reg, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* CMN rd, rs */                                                    \
          thumb_data_proc_test(alu_op, cmn, reg, rd, rs);                     \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x43:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* ORR rd, rs */                                                    \
          thumb_data_proc(alu_op, orrs, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* MUL rd, rs */                                                    \
          /*thumb_data_proc(alu_op, muls, reg, rd, rd, rs);*/                     \
          thumb_data_proc_muls(alu_op, reg, rd, rd, rs);                      \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* BIC rd, rs */                                                    \
          thumb_data_proc(alu_op, bics, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* MVN rd, rs */                                                    \
          thumb_data_proc_unary(alu_op, mvns, reg, rd, rs);                   \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x44:                                                                \
      /* ADD rd, rs */                                                        \
      thumb_data_proc_hi(add);                                                \
      break;                                                                  \
                                                                              \
    case 0x45:                                                                \
      /* CMP rd, rs */                                                        \
      thumb_data_proc_test_hi(cmp);                                           \
      break;                                                                  \
                                                                              \
    case 0x46:                                                                \
      /* MOV rd, rs */                                                        \
      thumb_data_proc_mov_hi();                                               \
      break;                                                                  \
                                                                              \
    case 0x47:                                                                \
      /* BX rs */                                                             \
      thumb_bx();                                                             \
      break;                                                                  \
                                                                              \
    case 0x48 ... 0x4F:                                                       \
      /* LDR r0..r7, [pc + imm] */                                            \
      thumb_access_memory(load, imm, (opcode >> 8) & 0xFF - 0x48, 0, 0,       \
       pc_relative, ((pc & ~2) + (imm << 2) + 4), u32);                       \
      break;                                                                  \
                                                                              \
    case 0x50 ... 0x51:                                                       \
      /* STR rd, [rb + ro] */                                                 \
      thumb_access_memory(store, mem_reg, rd, rb, ro, reg_reg, 0, u32);       \
      break;                                                                  \
                                                                              \
    case 0x52 ... 0x53:                                                       \
      /* STRH rd, [rb + ro] */                                                \
      thumb_access_memory(store, mem_reg, rd, rb, ro, reg_reg, 0, u16);       \
      break;                                                                  \
                                                                              \
    case 0x54 ... 0x55:                                                       \
      /* STRB rd, [rb + ro] */                                                \
      thumb_access_memory(store, mem_reg, rd, rb, ro, reg_reg, 0, u8);        \
      break;                                                                  \
                                                                              \
    case 0x56 ... 0x57:                                                       \
      /* LDSB rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, s8);         \
      break;                                                                  \
                                                                              \
    case 0x58 ... 0x59:                                                       \
      /* LDR rd, [rb + ro] */                                                 \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, u32);        \
      break;                                                                  \
                                                                              \
    case 0x5A ... 0x5B:                                                       \
      /* LDRH rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, u16);        \
      break;                                                                  \
                                                                              \
    case 0x5C ... 0x5D:                                                       \
      /* LDRB rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, u8);         \
      break;                                                                  \
                                                                              \
    case 0x5E ... 0x5F:                                                       \
      /* LDSH rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, s16);        \
      break;                                                                  \
                                                                              \
    case 0x60 ... 0x67:                                                       \
      /* STR rd, [rb + imm] */                                                \
      thumb_access_memory(store, mem_imm, rd, rb, 0, reg_imm, (imm << 2),     \
       u32);                                                                  \
      break;                                                                  \
                                                                              \
    case 0x68 ... 0x6F:                                                       \
      /* LDR rd, [rb + imm] */                                                \
      thumb_access_memory(load, mem_imm, rd, rb, 0, reg_imm, (imm << 2), u32);\
      break;                                                                  \
                                                                              \
    case 0x70 ... 0x77:                                                       \
      /* STRB rd, [rb + imm] */                                               \
      thumb_access_memory(store, mem_imm, rd, rb, 0, reg_imm, imm, u8);       \
      break;                                                                  \
                                                                              \
    case 0x78 ... 0x7F:                                                       \
      /* LDRB rd, [rb + imm] */                                               \
      thumb_access_memory(load, mem_imm, rd, rb, 0, reg_imm, imm, u8);        \
      break;                                                                  \
                                                                              \
    case 0x80 ... 0x87:                                                       \
      /* STRH rd, [rb + imm] */                                               \
      thumb_access_memory(store, mem_imm, rd, rb, 0, reg_imm,                 \
       (imm << 1), u16);                                                      \
      break;                                                                  \
                                                                              \
    case 0x88 ... 0x8F:                                                       \
      /* LDRH rd, [rb + imm] */                                               \
      thumb_access_memory(load, mem_imm, rd, rb, 0, reg_imm, (imm << 1), u16);\
      break;                                                                  \
                                                                              \
    case 0x90 ... 0x97:                                                       \
      /* STR r0..r7, [sp + imm] */                                            \
      thumb_access_memory(store, imm, (opcode >> 8) & 0xFF - 0x90, 13, 0,     \
       reg_imm_sp, imm, u32);                                                 \
      break;                                                                  \
                                                                              \
    case 0x98 ... 0x9F:                                                       \
      /* LDR r0..r7, [sp + imm] */                                            \
      thumb_access_memory(load, imm, (opcode >> 8) & 0xFF - 0x98, 13, 0,      \
       reg_imm_sp, imm, u32);                                                 \
      break;                                                                  \
                                                                              \
    case 0xA0 ... 0xA7:                                                       \
      /* ADD r0..r7, pc, +imm */                                              \
      thumb_load_pc((opcode >> 8) & 0xFF - 0xA0);                             \
      break;                                                                  \
                                                                              \
    case 0xA8 ... 0xAF:                                                       \
      /* ADD r0..r7, sp, +imm */                                              \
      thumb_load_sp((opcode >> 8) & 0xFF - 0xA8);                             \
      break;                                                                  \
                                                                              \
    case 0xB0:                                                                \
      if((opcode >> 7) & 0x01)                                                \
      {                                                                       \
        /* ADD sp, -imm */                                                    \
        thumb_adjust_sp(-(imm << 2));                                         \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADD sp, +imm */                                                    \
        thumb_adjust_sp((imm << 2));                                          \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0xB4:                                                                \
      /* PUSH rlist */                                                        \
      thumb_block_memory(store, down, no, 13);                                \
      mips_emit_syscall(11);\
      break;                                                                  \
                                                                              \
    case 0xB5:                                                                \
      /* PUSH rlist, lr */                                                    \
      thumb_block_memory(store, push_lr, push_lr, 13);                        \
      mips_emit_syscall(11);\
      break;                                                                  \
                                                                              \
    case 0xBC:                                                                \
      /* POP rlist */                                                         \
      thumb_block_memory(load, no, up, 13);                                   \
      mips_emit_syscall(12);\
      break;                                                                  \
                                                                              \
    case 0xBD:                                                                \
      /* POP rlist, pc */                                                     \
      thumb_block_memory(load, no, pop_pc, 13);                               \
      mips_emit_syscall(12);\
      break;                                                                  \
                                                                              \
    case 0xC0 ... 0xC7:                                                       \
      /* STMIA r0..r7!, rlist */                                              \
      thumb_block_memory(store, no, up, (opcode >> 8) & 0xFF - 0xC0);         \
      break;                                                                  \
                                                                              \
    case 0xC8 ... 0xCF:                                                       \
      /* LDMIA r0..r7!, rlist */                                              \
      thumb_block_memory(load, no, up, (opcode >> 8) & 0xFF - 0xC8);          \
      break;                                                                  \
                                                                              \
    case 0xD0:                                                                \
      /* BEQ label */                                                         \
      thumb_conditional_branch(eq);                                           \
      break;                                                                  \
                                                                              \
    case 0xD1:                                                                \
      /* BNE label */                                                         \
      thumb_conditional_branch(ne);                                           \
      break;                                                                  \
                                                                              \
    case 0xD2:                                                                \
      /* BCS label */                                                         \
      thumb_conditional_branch(cs);                                           \
      break;                                                                  \
                                                                              \
    case 0xD3:                                                                \
      /* BCC label */                                                         \
      thumb_conditional_branch(cc);                                           \
      break;                                                                  \
                                                                              \
    case 0xD4:                                                                \
      /* BMI label */                                                         \
      thumb_conditional_branch(mi);                                           \
      break;                                                                  \
                                                                              \
    case 0xD5:                                                                \
      /* BPL label */                                                         \
      thumb_conditional_branch(pl);                                           \
      break;                                                                  \
                                                                              \
    case 0xD6:                                                                \
      /* BVS label */                                                         \
      thumb_conditional_branch(vs);                                           \
      break;                                                                  \
                                                                              \
    case 0xD7:                                                                \
      /* BVC label */                                                         \
      thumb_conditional_branch(vc);                                           \
      break;                                                                  \
                                                                              \
    case 0xD8:                                                                \
      /* BHI label */                                                         \
      thumb_conditional_branch(hi);                                           \
      break;                                                                  \
                                                                              \
    case 0xD9:                                                                \
      /* BLS label */                                                         \
      thumb_conditional_branch(ls);                                           \
      break;                                                                  \
                                                                              \
    case 0xDA:                                                                \
      /* BGE label */                                                         \
      thumb_conditional_branch(ge);                                           \
      break;                                                                  \
                                                                              \
    case 0xDB:                                                                \
      /* BLT label */                                                         \
      thumb_conditional_branch(lt);                                           \
      break;                                                                  \
                                                                              \
    case 0xDC:                                                                \
      /* BGT label */                                                         \
      thumb_conditional_branch(gt);                                           \
      break;                                                                  \
                                                                              \
    case 0xDD:                                                                \
      /* BLE label */                                                         \
      thumb_conditional_branch(le);                                           \
      break;                                                                  \
                                                                              \
    case 0xDF:                                                                \
    {                                                                         \
      /* SWI comment */                                                       \
      thumb_swi();                                                            \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xE0 ... 0xE7:                                                       \
    {                                                                         \
      /* B label */                                                           \
      thumb_b();                                                              \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xF0 ... 0xF7:                                                       \
    {                                                                         \
      /* (low word) BL label */                                               \
      /* This should possibly generate code if not in conjunction with a BLH  \
         next, but I don't think anyone will do that. */                      \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xF8 ... 0xFF:                                                       \
    {                                                                         \
      /* (high word) BL label */                                              \
      /* This might not be preceeding a BL low word (Golden Sun 2), if so     \
         it must be handled like an indirect branch. */                       \
      if((last_opcode >= 0xF000) && (last_opcode < 0xF800))                   \
      {                                                                       \
        thumb_bl();                                                           \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        thumb_blh();                                                          \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  pc += 2                                                                     \

#define thumb_flag_modifies_all()                                             \
  flag_status |= 0xFF                                                         \

#define thumb_flag_modifies_zn()                                              \
  flag_status |= 0xCC                                                         \

#define thumb_flag_modifies_znc()                                             \
  flag_status |= 0xEE                                                         \

#define thumb_flag_modifies_zn_maybe_c()                                      \
  flag_status |= 0xCE                                                         \

#define thumb_flag_modifies_c()                                               \
  flag_status |= 0x22                                                         \

#define thumb_flag_requires_c()                                               \
  flag_status |= 0x200                                                        \

#define thumb_flag_requires_all()                                             \
  flag_status |= 0xF00                                                        \

static void thumb_flag_status(block_data_thumb_type* block_data, u32 opcode)
{
  u16 flag_status = 0;
  switch((opcode >> 8) & 0xFF)
  {
    /* left shift by imm */
    case 0x00 ... 0x07:
      thumb_flag_modifies_zn();
      if(((opcode >> 6) & 0x1F) != 0)
      {
        thumb_flag_modifies_c();
      }
      break;

    /* right shift by imm */
    case 0x08 ... 0x17:
      thumb_flag_modifies_znc();
      break;

    /* add, subtract */
    case 0x18 ... 0x1F:
    /* cmp reg, imm; add, subtract */
    case 0x28 ... 0x3F:
    case 0x45:
      /* CMP rd, rs */
      thumb_flag_modifies_all();
      break;

    /* mov reg, imm */
    case 0x20 ... 0x27:
    /* ORR, MUL, BIC, MVN */
    case 0x43:
      thumb_flag_modifies_zn();
      break;

    case 0x40:
      switch((opcode >> 6) & 0x03)
      {
        case 0x00:
          /* AND rd, rs */
        case 0x01:
          /* EOR rd, rs */
          thumb_flag_modifies_zn();
          break;

        case 0x02:
          /* LSL rd, rs */
        case 0x03:
          /* LSR rd, rs */
          thumb_flag_modifies_zn_maybe_c();
          break;
      }
      break;

    case 0x41:
      switch((opcode >> 6) & 0x03)
      {
        case 0x00:
          /* ASR rd, rs */
        case 0x03:
          /* ROR rd, rs */
          thumb_flag_modifies_zn_maybe_c();
          break;

        case 0x01:
          /* ADC rd, rs */
        case 0x02:
          /* SBC rd, rs */
          thumb_flag_modifies_all();
          thumb_flag_requires_c();
          break;
      }
      break;

    /* TST, NEG, CMP, CMN */
    case 0x42:
      if((opcode >> 6) & 0x03)
      {
        /* NEG, CMP, CMN */
        thumb_flag_modifies_all();
      }
      else
      {
        /* TST rd, rs */
        thumb_flag_modifies_zn();
      }
      break;

    /* mov might change PC (fall through if so) */
    case 0x46:
      if((opcode & 0xFF87) != 0x4687)
        break;

    /* branches (can change PC) */
    case 0x47:
    case 0xBD:
    case 0xD0 ... 0xE7:
    case 0xF0 ... 0xFF:
      thumb_flag_requires_all();
      break;
  }
  block_data->flag_data = flag_status;
}

u8 *iwram_block_ptrs[MAX_TAG + 1];
u32 iwram_block_tag_top = MIN_TAG;
u8 *ewram_block_ptrs[MAX_TAG + 1];
u32 ewram_block_tag_top = MIN_TAG;
u8 *vram_block_ptrs[MAX_TAG + 1];
u32 vram_block_tag_top = MIN_TAG;

u8 *bios_block_ptrs[MAX_TAG + 1];
u32 bios_block_tag_top = MIN_TAG;

// This function will return a pointer to a translated block of code. If it
// doesn't exist it will translate it, if it does it will pass it back.

// type should be "arm", "thumb", or "dual." For arm or thumb the PC should
// be a real PC, for dual the least significant bit will determine if it's
// ARM or Thumb mode.

#define block_lookup_address_pc_arm()                                         \
  pc &= ~0x03                                                                 \

#define block_lookup_address_pc_thumb()                                       \
  pc &= ~0x01                                                                 \

#define readonly_translation_region TRANSLATION_REGION_READONLY
#define writable_translation_region TRANSLATION_REGION_WRITABLE

#define iwram_metadata_area METADATA_AREA_IWRAM
#define ewram_metadata_area METADATA_AREA_EWRAM
#define vram_metadata_area  METADATA_AREA_VRAM
#define rom_metadata_area   METADATA_AREA_ROM
#define bios_metadata_area  METADATA_AREA_BIOS

/*
 * MIN_TAG is the smallest tag that can be used. 0xFFFF is off-limits because
 * adding 1 to it for the next tag would overflow an unsigned short.
 * An array containing native code addresses is indexed by these tags;
 * the tags themselves are applied to the Metadata Area at the same offset as
 * the code that is tagged in this way. It is possible to have a tag that
 * starts in the middle of another block, and it is possible to have a tag
 * for the very same Data Word when it's interpreted as an ARM instruction or
 * as a Thumb instruction.
 */

#define get_tag_arm()                                                         \
  (location[1])                                                               \

#define get_tag_thumb()                                                       \
  (location[pc & 2])                                                          \

#define fill_tag_arm(mem_type)                                                \
  location[1] = mem_type##_block_tag_top                                      \

#define fill_tag_thumb(mem_type)                                              \
  location[pc & 2] = mem_type##_block_tag_top                                 \

/*
 * In gpSP 0.9, assigning a tag to a Metadata Entry signifies that
 * translate_block_{instype} will compile something new, and that it WILL be
 * at address ({region}_next_code + block_prologue_size).
 * With the new writable translation region behaviour, it is possible for
 * translate_block_arm to signify that the new tag needs to reference code
 * that is not at {region}_next_code. So the return value of translate_block_
 * {instype} has changed from s32 to u8*, and it returns NULL on failure
 * instead of -1.
 * (It is possible for tags in one area to be exhausted, but this won't affect
 * unrelated areas -- which means that reaching MAX_TAG in IWRAM clears IWRAM,
 * but EWRAM and VRAM stay as is. IWRAM would then be forced to make lookups
 * for code reuse and possibly recompilation.)
 */

#define block_lookup_translate(instruction_type, mem_type, metadata_area)     \
  block_tag = get_tag_##instruction_type();                                   \
  if((block_tag < MIN_TAG) || (block_tag > MAX_TAG))                          \
  {                                                                           \
    __label__ redo;                                                           \
    u8* translation_result;                                                   \
                                                                              \
    redo:                                                                     \
    if (metadata_area##_block_tag_top > MAX_TAG)                              \
    {                                                                         \
      clear_metadata_area(metadata_area##_metadata_area,                      \
        CLEAR_REASON_LAST_TAG);                                               \
    }                                                                         \
                                                                              \
    translation_recursion_level++;                                            \
    translation_result = translate_block_##instruction_type(pc);              \
    translation_recursion_level--;                                            \
                                                                              \
    /* If the translation failed then pass that failure on if we're in        \
       a recursive level, or try again if we've hit the bottom. */            \
    if(translation_result == NULL)                                            \
    {                                                                         \
      if(translation_recursion_level)                                         \
        return NULL;                                                          \
                                                                              \
      goto redo;                                                              \
    }                                                                         \
    block_address = translation_result;                                       \
    metadata_area##_block_ptrs[metadata_area##_block_tag_top] = block_address;\
    fill_tag_##instruction_type(metadata_area);                               \
    metadata_area##_block_tag_top++;                                          \
                                                                              \
    /* if(translation_recursion_level == 0)                                      \
      translate_invalidate_dcache(); */                                          \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    block_address = metadata_area##_block_ptrs[block_tag];                    \
  }                                                                           \

u32 translation_recursion_level = 0;
u32 translation_flush_count = 0;

/*
u32 index;\
for(index= 0; index < 1024*64; index++)\
{\
    if((index%8) == 0)\
        printf("\n");\
    printf("%08x ", rom_branch_hash[index]);\
}\
printf("\n");\
*/

u32     recursion_level= 0;

extern void ARMBadJump(unsigned int SourcePC, unsigned int TargetPC);

static inline void AdjustTranslationBufferPeak(TRANSLATION_REGION_TYPE translation_region)
{
	u32 Size;
	switch (translation_region)
	{
		case TRANSLATION_REGION_READONLY:
			Size = readonly_next_code - readonly_code_cache;
			break;
		case TRANSLATION_REGION_WRITABLE:
			Size = writable_next_code - writable_code_cache;
			break;
	}
	if (Size > Stats.TranslationBytesPeak[translation_region])
		Stats.TranslationBytesPeak[translation_region] = Size;
}

// Where emulation started
#define block_lookup_address_body(type)                                       \
{                                                                             \
  u16 *location;                                                              \
  u32 block_tag;                                                              \
  u8 *block_address;                                                          \
                                                                              \
/*printf("recursion in %d: %08x\n", recursion_level++, pc);*/\
                                                                              \
  /* Starting at the beginning, we allow for one translation cache flush. */  \
  if(translation_recursion_level == 0)                                        \
    translation_flush_count = 0;                                              \
  /* according the mode fix the PC  */                                        \
  block_lookup_address_pc_##type();                                           \
                                                                              \
  switch(pc >> 24)                                                            \
  {                                                                           \
    case 0x0:                                                                 \
      bios_region_read_allow();                                               \
      location = bios.metadata + (pc & 0x3FFC);                               \
      block_lookup_translate(type, readonly, bios);                           \
      if(translation_recursion_level == 0)                                    \
        bios_region_read_allow();                                             \
      AdjustTranslationBufferPeak(TRANSLATION_REGION_READONLY);               \
      break;                                                                  \
                                                                              \
    case 0x2:                                                                 \
      location = ewram_metadata + (pc & 0x3FFFC);                             \
      block_lookup_translate(type, writable, ewram);                          \
      AdjustTranslationBufferPeak(TRANSLATION_REGION_WRITABLE);               \
      break;                                                                  \
                                                                              \
    case 0x3:                                                                 \
      location = iwram_metadata + (pc & 0x7FFC);                              \
      block_lookup_translate(type, writable, iwram);                          \
      AdjustTranslationBufferPeak(TRANSLATION_REGION_WRITABLE);               \
      break;                                                                  \
                                                                              \
    case 0x6:                                                                 \
      if (pc & 0x10000)                                                       \
        location = vram_metadata + (pc & 0x17FFC);                            \
      else                                                                    \
        location = vram_metadata + (pc & 0xFFFC);                             \
      block_lookup_translate(type, writable, vram);                           \
      AdjustTranslationBufferPeak(TRANSLATION_REGION_WRITABLE);               \
      break;                                                                  \
                                                                              \
    case 0x8 ... 0xD:                                                         \
    {                                                                         \
      u32 hash_target = ((pc * 2654435761U) >> 16) &                          \
       (ROM_BRANCH_HASH_SIZE - 1);                                            \
      u32 *block_ptr = rom_branch_hash[hash_target];                          \
      u32 **block_ptr_address = rom_branch_hash + hash_target;                \
                                                                              \
      while(block_ptr)                                                        \
      {                                                                       \
        if(block_ptr[0] == pc)                                                \
        {                                                                     \
          block_address = (u8 *)(block_ptr + 2) + block_prologue_size;        \
          break;                                                              \
        }                                                                     \
                                                                              \
        block_ptr_address = (u32 **)(block_ptr + 1);                          \
        block_ptr = (u32 *)block_ptr[1];                                      \
      }                                                                       \
                                                                              \
      if(block_ptr == NULL)                                                   \
      {                                                                       \
        __label__ redo;                                                       \
        u8* translation_result;                                               \
                                                                              \
        redo:                                                                 \
                                                                              \
        translation_recursion_level++;                                        \
        ((u32 *)readonly_next_code)[0] = pc;                                  \
        ((u32 **)readonly_next_code)[1] = NULL;                               \
        *block_ptr_address = (u32 *)readonly_next_code;                       \
        readonly_next_code += 8;                                              \
        block_address = readonly_next_code + block_prologue_size;             \
        translation_result = translate_block_##type(pc);                      \
        translation_recursion_level--;                                        \
                                                                              \
        /* If the translation failed then pass that failure on if we're in    \
         a recursive level, or try again if we've hit the bottom. */          \
        if(translation_result == NULL)                                        \
        {                                                                     \
          if(translation_recursion_level)                                     \
            return NULL;                                                      \
                                                                              \
          goto redo;                                                          \
        }                                                                     \
                                                                              \
        /* if(translation_recursion_level == 0)                                  \
          translate_invalidate_dcache(); */                                      \
      }                                                                       \
      AdjustTranslationBufferPeak(TRANSLATION_REGION_READONLY);               \
      break;                                                                  \
    }                                                                         \
                                                                              \
    default:                                                                  \
      /* If we're at the bottom, it means we're actually trying to jump to an \
         address that we can't handle. Otherwise, it means that code scanned  \
         has reached an address that can't be handled, which means that we    \
         have most likely hit an area that doesn't contain code yet (for      \
         instance, in RAM). If such a thing happens, return -1 and the        \
         block translater will naively link it (it'll be okay, since it       \
         should never be hit) */                                              \
      if(translation_recursion_level == 0)                                    \
      {                                                                       \
        ARMBadJump((unsigned int) reg[REG_PC], (unsigned int) pc);            \
        /* Here, we can't return 0 or -1, because when the new gamepak is     \
           loaded, mips_stub.S's code which called into block_lookup_address  \
           _*something* will expect the return value to be a MIPS address to  \
           jump to. So we just let the ARMBadJump procedure run               \
           endlessly. */                                                      \
      }                                                                       \
      block_address = (u8 *)(-1);                                             \
      break;                                                                  \
  }                                                                           \
                                                                              \
/*printf("recursion out %d\n", --recursion_level);*/\
/*if(recursion_level==0)\
{\
    dump_translation_cache(); \
}*/\
  return block_address;                                                       \
}                                                                             \

u8 *block_lookup_address_arm(u32 pc)
block_lookup_address_body(arm);

u8 *block_lookup_address_thumb(u32 pc)
block_lookup_address_body(thumb);

u8 *block_lookup_address_dual(u32 pc)
{
  if(pc & 0x01)
  {
    reg[REG_CPSR] |= 0x20;
    return block_lookup_address_thumb(pc & ~0x01);
  }
  else
  {
    reg[REG_CPSR] &= ~0x20;
    return block_lookup_address_arm((pc + 2) & ~0x03);
  }
}

// Potential exit point: If the rd field is pc for instructions is 0x0F,
// the instruction is b/bl/bx, or the instruction is ldm with PC in the
// register list.
// All instructions with upper 3 bits less than 100b have an rd field
// except bx, where the bits must be 0xF there anyway, multiplies,
// which cannot have 0xF in the corresponding fields, and msr, which
// has 0x0F there but doesn't end things (therefore must be special
// checked against). Because MSR and BX overlap both are checked for.

#define arm_exit_point                                                        \
 (((opcode < 0x8000000) && ((opcode & 0x000F000) == 0x000F000) &&             \
  ((opcode & 0xDB0F000) != 0x120F000)) ||                                     \
  ((opcode & 0x12FFF10) == 0x12FFF10) ||                                      \
  ((opcode & 0x8108000) == 0x8108000) ||                                      \
  ((opcode >= 0xA000000) && (opcode < 0xF000000)) ||                          \
  ((opcode >= 0xF000000) && (!swi_hle_handle[((opcode >> 16) & 0xFF)][0])))       \

#define arm_opcode_branch                                                     \
  ((opcode & 0xE000000) == 0xA000000)                                         \

#define arm_opcode_swi                                                        \
  ((opcode & 0xF000000) == 0xF000000)                                         \

#define arm_opcode_unconditional_branch                                       \
  (condition == 0x0E)                                                         \

#define arm_load_opcode()                                                     \
  opcode = ADDRESS32(pc_address_block, (block_end_pc & 0x7FFF));              \
  condition = opcode >> 28;                                                   \
                                                                              \
  opcode &= 0xFFFFFFF;                                                        \
                                                                              \
  block_end_pc += 4                                                           \

#define arm_branch_target()                                                   \
  branch_target = (block_end_pc + 4 + ((s32)(opcode << 8) >> 6))              \

// Contiguous conditional block flags modification - it will set 0x20 in the
// condition's bits if this instruction modifies flags. Taken from the CPU
// switch so it'd better be right this time.

#define arm_set_condition(_condition)                                         \
  block_data.arm[block_data_position].condition = _condition;                 \
  switch((opcode >> 20) & 0xFF)                                               \
  {                                                                           \
    case 0x01:                                                                \
    case 0x03:                                                                \
    case 0x09:                                                                \
    case 0x0B:                                                                \
    case 0x0D:                                                                \
    case 0x0F:                                                                \
      if((((opcode >> 5) & 0x03) == 0) || ((opcode & 0x90) != 0x90))          \
        block_data.arm[block_data_position].condition |= 0x20;                \
      break;                                                                  \
                                                                              \
    case 0x05:                                                                \
    case 0x07:                                                                \
    case 0x11:                                                                \
    case 0x13:                                                                \
    case 0x15 ... 0x17:                                                       \
    case 0x19:                                                                \
    case 0x1B:                                                                \
    case 0x1D:                                                                \
    case 0x1F:                                                                \
      if((opcode & 0x90) != 0x90)                                             \
        block_data.arm[block_data_position].condition |= 0x20;                \
      break;                                                                  \
                                                                              \
    case 0x12:                                                                \
      if(((opcode & 0x90) != 0x90) && !(opcode & 0x10))                       \
        block_data.arm[block_data_position].condition |= 0x20;                \
      break;                                                                  \
                                                                              \
    case 0x21:                                                                \
    case 0x23:                                                                \
    case 0x25:                                                                \
    case 0x27:                                                                \
    case 0x29:                                                                \
    case 0x2B:                                                                \
    case 0x2D:                                                                \
    case 0x2F ... 0x37:                                                       \
    case 0x39:                                                                \
    case 0x3B:                                                                \
    case 0x3D:                                                                \
    case 0x3F:                                                                \
      block_data.arm[block_data_position].condition |= 0x20;                  \
    break;                                                                    \
  }                                                                           \

#define arm_link_block()                                                      \
  translation_target = block_lookup_address_arm(branch_target)                \

#define arm_instruction_width 4
#define arm_instruction_type  u32

#ifdef OLD_COUNT
#define arm_base_cycles()                                                     \
  cycle_count += waitstate_cycles_sequential[pc >> 24][2]                     \

#else
#define arm_base_cycles()                                                     \
  cycle_count += cpu_waitstate_cycles_seq[1][pc >> 24]                        \

#endif
// For now this just sets a variable that says flags should always be
// computed.

#define arm_dead_flag_eliminate()                                             \
  flag_status = 0xF                                                           \

// The following Thumb instructions can exit:
// b, bl, bx, swi, pop {... pc}, and mov pc, ..., the latter being a hireg
// op only. Rather simpler to identify than the ARM set.

#define thumb_exit_point                                                      \
  (((opcode >= 0xD000) && (opcode < 0xDF00)) ||                               \
   (((opcode & 0xFF00) == 0xDF00) &&                                          \
    (!swi_hle_handle[opcode & 0xFF][0])) ||                           \
   ((opcode >= 0xE000) && (opcode < 0xE800)) ||                               \
   ((opcode & 0xFF00) == 0x4700) ||                                           \
   ((opcode & 0xFF00) == 0xBD00) ||                                           \
   ((opcode & 0xFF87) == 0x4687) ||                                           \
   ((opcode >= 0xF800)))                                                      \

#define thumb_opcode_branch                                                   \
  (((opcode >= 0xD000) && (opcode < 0xDF00)) ||                               \
   ((opcode >= 0xE000) && (opcode < 0xE800)) ||                               \
   (opcode >= 0xF800))                                                        \

#define thumb_opcode_swi                                                      \
  ((opcode & 0xFF00) == 0xDF00)                                               \

#define thumb_opcode_unconditional_branch                                     \
  ((opcode < 0xD000) || (opcode >= 0xDF00))                                   \

#define thumb_load_opcode()                                                   \
  last_opcode = opcode;                                                       \
  opcode = ADDRESS16(pc_address_block, (block_end_pc & 0x7FFF));              \
                                                                              \
  block_end_pc += 2                                                           \

#define thumb_branch_target()                                                 \
  if(opcode < 0xDF00)                                                         \
  {                                                                           \
    branch_target = block_end_pc + 2 + ((s32)(opcode << 24) >> 23);           \
  }                                                                           \
  else                                                                        \
                                                                              \
  if(opcode < 0xE800)                                                         \
  {                                                                           \
    branch_target = block_end_pc + 2 + ((s32)(opcode << 21) >> 20);           \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    if((last_opcode >= 0xF000) && (last_opcode < 0xF800))                     \
    {                                                                         \
      branch_target =                                                         \
       (block_end_pc + ((s32)(last_opcode << 21) >> 9) +                      \
       ((opcode & 0x07FF) << 1));                                             \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      goto no_direct_branch;                                                  \
    }                                                                         \
  }                                                                           \

#define thumb_set_condition(_condition)                                       \

#define thumb_link_block()                                                    \
  if(branch_target != 0x00000008)                                             \
    translation_target = block_lookup_address_thumb(branch_target);           \
  else                                                                        \
    translation_target = block_lookup_address_arm(branch_target)              \

#define thumb_instruction_width 2
#define thumb_instruction_type  u16

#ifdef OLD_COUNT
#define thumb_base_cycles()                                                   \
  cycle_count += waitstate_cycles_sequential[pc >> 24][1]                     \

#else
#define thumb_base_cycles()                                                   \
  cycle_count += cpu_waitstate_cycles_seq[0][pc >> 24]                        \

#endif

// Here's how this works: each instruction has three different sets of flag
// attributes, each consisiting of a 4bit mask describing how that instruction
// interacts with the 4 main flags (N/Z/C/V).
// The first set, in bits 0:3, is the set of flags the instruction may
// modify. After this pass this is changed to the set of flags the instruction
// should modify - if the bit for the corresponding flag is not set then code
// does not have to be generated to calculate the flag for that instruction.

// The second set, in bits 7:4, is the set of flags that the instruction must
// modify (ie, for shifts by the register values the instruction may not
// always modify the C flag, and thus the C bit won't be set here).

// The third set, in bits 11:8, is the set of flags that the instruction uses
// in its computation, or the set of flags that will be needed after the
// instruction is done. For any instructions that change the PC all of the
// bits should be set because it is (for now) unknown what flags will be
// needed after it arrives at its destination. Instructions that use the
// carry flag as input will have it set as well.

// The algorithm is a simple liveness analysis procedure: It starts at the
// bottom of the instruction stream and sets a "currently needed" mask to
// the flags needed mask of the current instruction. Then it moves down
// an instruction, ANDs that instructions "should generate" mask by the
// "currently needed" mask, then ANDs the "currently needed" mask by
// the 1's complement of the instruction's "must generate" mask, and ORs
// the "currently needed" mask by the instruction's "flags needed" mask.

#define thumb_dead_flag_eliminate()                                           \
{                                                                             \
  u32 needed_mask;                                                            \
  needed_mask = block_data.thumb[block_data_position].flag_data >> 8;         \
                                                                              \
  block_data_position--;                                                      \
  while(block_data_position >= 0)                                             \
  {                                                                           \
    flag_status = block_data.thumb[block_data_position].flag_data;            \
    block_data.thumb[block_data_position].flag_data =                         \
     (flag_status & needed_mask);                                             \
    needed_mask &= ~((flag_status >> 4) & 0x0F);                              \
    needed_mask |= flag_status >> 8;                                          \
    block_data_position--;                                                    \
  }                                                                           \
}                                                                             \

#define MAX_BLOCK_SIZE 2048
#define MAX_EXITS      128

typedef union {
  block_data_arm_type arm[MAX_BLOCK_SIZE];
  block_data_thumb_type thumb[MAX_BLOCK_SIZE];
} block_data_type;

typedef union {
  u32 arm[MAX_BLOCK_SIZE];
  u16 thumb[MAX_BLOCK_SIZE];
} opcode_array_type;

block_data_type block_data;
opcode_array_type opcodes;

#define smc_write_arm_yes()                                                   \
  switch (block_end_pc >> 24)                                                 \
  {                                                                           \
    case 0x02: /* EWRAM */                                                    \
      ewram_metadata[(block_end_pc & 0x3FFFC) | 3] |= 0x2;                    \
      break;                                                                  \
    case 0x03: /* IWRAM */                                                    \
      iwram_metadata[(block_end_pc & 0x7FFC) | 3] |= 0x2;                     \
      break;                                                                  \
    case 0x06: /* VRAM */                                                     \
      if (block_end_pc & 0x10000)                                             \
        vram_metadata[(block_end_pc & 0x17FFC) | 3] |= 0x2;                   \
      else                                                                    \
        vram_metadata[(block_end_pc & 0xFFFC) | 3] |= 0x2;                    \
      break;                                                                  \
  }                                                                           \

#define smc_write_thumb_yes()                                                 \
  switch (block_end_pc >> 24)                                                 \
  {                                                                           \
    case 0x02: /* EWRAM */                                                    \
      ewram_metadata[(block_end_pc & 0x3FFFC) | 3] |= 0x1;                    \
      break;                                                                  \
    case 0x03: /* IWRAM */                                                    \
      iwram_metadata[(block_end_pc & 0x7FFC) | 3] |= 0x1;                     \
      break;                                                                  \
    case 0x06: /* VRAM */                                                     \
      if (block_end_pc & 0x10000)                                             \
        vram_metadata[(block_end_pc & 0x17FFC) | 3] |= 0x1;                   \
      else                                                                    \
        vram_metadata[(block_end_pc & 0xFFFC) | 3] |= 0x1;                    \
      break;                                                                  \
  }                                                                           \

#define smc_write_arm_no()                                                    \

#define smc_write_thumb_no()                                                  \

#define unconditional_branch_write_arm_yes()                                  \
  {                                                                           \
    u32 previous_pc = block_end_pc - arm_instruction_width;                   \
    switch (previous_pc >> 24)                                                \
    {                                                                         \
      case 0x02: /* EWRAM */                                                  \
        ewram_metadata[(previous_pc & 0x3FFFC) | 3] |= 0x8;                   \
        break;                                                                \
      case 0x03: /* IWRAM */                                                  \
        iwram_metadata[(previous_pc & 0x7FFC) | 3] |= 0x8;                    \
        break;                                                                \
      case 0x06: /* VRAM */                                                   \
        if (previous_pc & 0x10000)                                            \
          vram_metadata[(previous_pc & 0x17FFC) | 3] |= 0x8;                  \
        else                                                                  \
          vram_metadata[(previous_pc & 0xFFFC) | 3] |= 0x8;                   \
        break;                                                                \
    }                                                                         \
  }                                                                           \

#define unconditional_branch_write_thumb_yes()                                \
  if ((block_end_pc & 2) == 0) /* Previous instruction is in 2nd half-word */ \
  {                                                                           \
    u32 previous_pc = block_end_pc - thumb_instruction_width;                 \
    switch (previous_pc >> 24)                                                \
    {                                                                         \
      case 0x02: /* EWRAM */                                                  \
        ewram_metadata[(previous_pc & 0x3FFFC) | 3] |= 0x4;                   \
        break;                                                                \
      case 0x03: /* IWRAM */                                                  \
        iwram_metadata[(previous_pc & 0x7FFC) | 3] |= 0x4;                    \
        break;                                                                \
      case 0x06: /* VRAM */                                                   \
        if (previous_pc & 0x10000)                                            \
          vram_metadata[(previous_pc & 0x17FFC) | 3] |= 0x4;                  \
        else                                                                  \
          vram_metadata[(previous_pc & 0xFFFC) | 3] |= 0x4;                   \
        break;                                                                \
    }                                                                         \
  }                                                                           \

#define unconditional_branch_write_arm_no()                                   \

#define unconditional_branch_write_thumb_no()                                 \

/*
 * Inserts Value into a sorted Array of unique values (or doesn't), of
 * size Size.
 * If Value was already present, then the old size is returned, and no
 * insertions are made. Otherwise, Value is inserted into Array at the
 * proper position to maintain its total order, and Size + 1 is returned.
 */
static u32 InsertUniqueSorted(u32* Array, u32 Value, u32 Size)
{
	// Gather the insertion index with a binary search.
	s32 Min = 0, Max = Size - 1;
	while (Min < Max) {
		s32 Mid = Min + (Max - Min) / 2;
		if (Array[Mid] < Value)
			Min = Mid + 1;
		else
			Max = Mid;
	}

	// Insert at Min.
	// Min == Size means we just insert at the end...
	if (Min == Size) {
		Array[Size] = Value;
		return Size + 1;
	}
	// ... otherwise it's either already in the array...
	else if (Array[Min] == Value) {
		return Size;
	}
	// ... or we need to move things.
	else {
		memmove(&Array[Min + 1], &Array[Min], Size - Min);
		Array[Min] = Value;
		return Size + 1;
	}
}

/*
 * Searches for the given Value in the given sorted Array of size Size.
 * If found, the index in the Array of the first element having the given
 * Value is returned.
 * Otherwise, -1 is returned.
 */
static s32 BinarySearch(u32* Array, u32 Value, u32 Size)
{
	s32 Min = 0, Max = Size - 1;
	while (Min < Max) {
		s32 Mid = Min + (Max - Min) / 2;
		if (Array[Mid] < Value)
			Min = Mid + 1;
		else
			Max = Mid;
	}

	if (Min == Max && Array[Min] == Value)
		return Min;
	else
		return -1;
}

#define scan_block(type, smc_write_op)                                        \
{                                                                             \
  /* The address of the first translation gate after block_start_pc. */       \
  u32 next_translation_gate = 0xFFFFFFFD; /* none */                          \
  for(i = 0; i < translation_gate_targets; i++)                               \
  {                                                                           \
    if(translation_gate_target_pc[i] >= block_start_pc &&                     \
       translation_gate_target_pc[i] <  next_translation_gate)                \
      next_translation_gate = translation_gate_target_pc[i];                  \
  }                                                                           \
  u8 continue_block = 1;                                                      \
  u32 branch_targets_sorted[MAX_EXITS];                                       \
  u32 sorted_branch_count = 0;                                                \
  /* Find the end of the block */                                             \
/*printf("str: %08x\n", block_start_pc);*/\
  do                                                                          \
  {                                                                           \
    check_pc_region(block_end_pc);                                            \
    smc_write_##type##_##smc_write_op();                                      \
    type##_load_opcode();                                                     \
    opcodes.type[block_data_position] = opcode;                               \
    type##_flag_status(&block_data.type[block_data_position],                 \
      opcodes.type[block_data_position]);                                     \
                                                                              \
    if(type##_exit_point)                                                     \
    {                                                                         \
      /* Branch/branch with link */                                           \
      if(type##_opcode_branch)                                                \
      {                                                                       \
        __label__ no_direct_branch;                                           \
        type##_branch_target();                                               \
        block_exits[block_exit_position].branch_target = branch_target;       \
        if (translation_region == TRANSLATION_REGION_READONLY)                \
          /* If we're in RAM, exit at the first unconditional branch, no      \
           * questions asked */                                               \
          sorted_branch_count = InsertUniqueSorted(branch_targets_sorted,     \
            branch_target, sorted_branch_count);                              \
        block_exit_position++;                                                \
                                                                              \
        /* Give the branch target macro somewhere to bail if it turns out to  \
           be an indirect branch (ala malformed Thumb bl) */                  \
        no_direct_branch:;                                                    \
      }                                                                       \
                                                                              \
      /* SWI branches to the BIOS, this will likely change when               \
         some HLE BIOS is implemented. */                                     \
      if(type##_opcode_swi)                                                   \
      {                                                                       \
        block_exits[block_exit_position].branch_target = 0x00000008;          \
        if (translation_region == TRANSLATION_REGION_READONLY)                \
          /* If we're in RAM, exit at the first unconditional branch, no      \
           * questions asked */                                               \
          sorted_branch_count = InsertUniqueSorted(branch_targets_sorted,     \
            0x00000008, sorted_branch_count); /* could already be in */       \
        block_exit_position++;                                                \
      }                                                                       \
                                                                              \
      type##_set_condition(condition | 0x10);                                 \
                                                                              \
      /* Only unconditional branches can end the block. */                    \
      if(type##_opcode_unconditional_branch)                                  \
      {                                                                       \
        /* Check to see if any prior block exits branch after here,           \
         * if so don't end the block. For efficiency, but to also keep the    \
         * correct order of the scanned branches for code emission, this is   \
         * using a separate sorted array with unique branch_targets.          \
         * Look at exiting early if the current block has already reached     \
         * half of the maximum allowed exit count or instruction count. */    \
        if (translation_region == TRANSLATION_REGION_WRITABLE ||              \
          BinarySearch(branch_targets_sorted, block_end_pc,                   \
          sorted_branch_count) == -1                                          \
          || block_exit_position >= MAX_EXITS / 2                             \
          || block_data_position >= MAX_BLOCK_SIZE / 2)                       \
        {                                                                     \
          continue_block = 0;                                                 \
          unconditional_branch_write_##type##_##smc_write_op();               \
        }                                                                     \
      }                                                                       \
      if(block_exit_position == MAX_EXITS)                                    \
      {                                                                       \
        RecompilerMaxExitsReached(block_start_pc, block_end_pc, MAX_EXITS);   \
        translation_gate_required = 1;                                        \
        continue_block = 0;                                                   \
      }                                                                       \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      type##_set_condition(condition);                                        \
    }                                                                         \
    block_data.type[block_data_position].update_cycles = 0;                   \
    block_data_position++;                                                    \
                                                                              \
    if(block_end_pc == next_translation_gate)                                 \
    {                                                                         \
      translation_gate_required = 1;                                          \
      continue_block = 0;                                                     \
    }                                                                         \
                                                                              \
    if((block_data_position == MAX_BLOCK_SIZE) ||                             \
     (block_end_pc == 0x3007FF0) || (block_end_pc == 0x203FFF0))              \
    {                                                                         \
      if(block_data_position == MAX_BLOCK_SIZE)                               \
        RecompilerMaxBlockSizeReached(block_start_pc, block_end_pc,           \
          MAX_BLOCK_SIZE);                                                    \
      translation_gate_required = 1;                                          \
      continue_block = 0;                                                     \
    }                                                                         \
  } while(continue_block);                                                    \
/*printf("end: %08x\n", block_end_pc);*/\
}                                                                             \

#define arm_fix_pc()                                                          \
  pc &= ~0x03                                                                 \

#define thumb_fix_pc()                                                        \
  pc &= ~0x01                                                                 \

/*
 * Flushes a cache line in both the data cache and the instruction cache.
 */
#ifdef NDS_LAYER
static inline void FlushCacheLine(void* CacheLine)
{
    // On the JZ4740, CACHE 0x19 functions as Hit Writeback, but leaves the
    // cache line valid. This is particularly useful to write more things to
    // the rest of the same cache line.
    __asm__ __volatile__ ("cache 0x19, 0(%0)\n\tsync\n\tcache 0x10, 0(%0)"
      : : "r"(CacheLine));
}
#else
static inline void FlushCacheLine(void* CacheLine)
{
    __asm__ __volatile__ ("cache 0x15, 0(%0)\n\tsync\n\tcache 0x10, 0(%0)"
      : : "r"(CacheLine));
}
#endif

#define translate_block_builder(type)                                         \
u8* translate_block_##type(u32 pc)                                            \
{                                                                             \
  u32 opcode;                                                                 \
  u32 last_opcode;                                                            \
  u32 condition;                                                              \
  u32 pc_region = (pc >> 15);                                                 \
  u32 new_pc_region;                                                          \
  u8 *pc_address_block = memory_map_read[pc_region];                          \
  u32 block_start_pc = pc;                                                    \
  u32 block_end_pc = pc;                                                      \
  u32 block_exit_position = 0;                                                \
  s32 block_data_position = 0;                                                \
  u32 external_block_exit_position = 0;                                       \
  u32 branch_target;                                                          \
  u32 cycle_count = 0;                                                        \
  u8 *translation_target;                                                     \
  u8 *backpatch_address = NULL;                                               \
  u8 *translation_ptr = NULL;                                                 \
  u8 *translation_cache_limit = NULL;                                         \
  s32 i;                                                                      \
  u32 flag_status;                                                            \
  block_exit_type block_exits[MAX_EXITS];                                     \
                                                                              \
  generate_block_extra_vars_##type();                                         \
  type##_fix_pc();                                                            \
  TRANSLATION_REGION_TYPE translation_region;                                 \
                                                                              \
  if(pc_address_block == NULL)                                                \
    pc_address_block = load_gamepak_page(pc_region & 0x3FF);                  \
                                                                              \
  switch(pc >> 24)                                                            \
  {                                                                           \
    case 0x02: /* EWRAM */                                                    \
    case 0x03: /* IWRAM */                                                    \
    case 0x06: /* VRAM */                                                     \
      translation_region = TRANSLATION_REGION_WRITABLE;                       \
      break;                                                                  \
                                                                              \
    case 0x08 ... 0x0D: /* ROM */                                             \
    case 0x00: /* BIOS */                                                     \
      translation_region = TRANSLATION_REGION_READONLY;                       \
      break;                                                                  \
  }                                                                           \
                                                                              \
  switch (translation_region)                                                 \
  {                                                                           \
    case TRANSLATION_REGION_READONLY:                                         \
      translation_ptr = readonly_next_code;                                   \
      translation_cache_limit =                                               \
       readonly_code_cache + READONLY_CODE_CACHE_SIZE -                       \
       TRANSLATION_CACHE_LIMIT_THRESHOLD;                                     \
      break;                                                                  \
                                                                              \
    case TRANSLATION_REGION_WRITABLE:                                         \
      translation_ptr = writable_next_code;                                   \
      translation_cache_limit =                                               \
       writable_code_cache + WRITABLE_CODE_CACHE_SIZE -                       \
       TRANSLATION_CACHE_LIMIT_THRESHOLD;                                     \
      update_metadata_area_start(pc);                                         \
      break;                                                                  \
  }                                                                           \
                                                                              \
  u8 translation_gate_required = 0; /* gets updated by scan_block */          \
                                                                              \
  /* scan_block is the first pass of the compiler. It reads the instructions, \
   * determines basically what kind of instructions they are, the branch      \
   * targets, the condition codes atop each instruction, as well  as the need \
   * for a translation gate to be placed at the end. */                       \
  if(translation_region == TRANSLATION_REGION_WRITABLE)                       \
  {                                                                           \
    scan_block(type, yes);                                                    \
    u16 checksum = block_checksum_##type((block_end_pc - block_start_pc)      \
     / type##_instruction_width) & (PERSISTENT_RAM_HASH_SIZE - 1);            \
    u32 *block_ptr = writable_area_checksum_hash[checksum];                   \
    u32 **block_ptr_address = writable_area_checksum_hash + checksum;         \
                                                                              \
    while(block_ptr)                                                          \
    {                                                                         \
      if(block_ptr[0] == block_start_pc)                                      \
      {                                                                       \
        u8* comparison_ptr = (u8*) block_ptr + sizeof(u32)  /* Past the PC */ \
         + sizeof(void*); /* Past the Next pointer */                         \
        u32 gba_code_size = *(u32*) comparison_ptr;                           \
        if (gba_code_size == block_end_pc - block_start_pc)                   \
        {                                                                     \
          comparison_ptr += sizeof(u32); /* Past the GBA code size */         \
                                                                              \
          if (memcmp(opcodes.type, comparison_ptr, gba_code_size) == 0)       \
          {                                                                   \
            comparison_ptr += gba_code_size;                                  \
            if (gba_code_size & 2) comparison_ptr += 2; /* Past the alignment */\
            StatsAddWritableReuse((block_end_pc - block_start_pc) /           \
             type##_instruction_width);                                       \
                                                                              \
            /* The Currently Code bits need to be set for the reused block,   \
             * just as if it were freshly compiled by the below code. */      \
            pc = block_end_pc;                                                \
            for (block_end_pc = block_start_pc; block_end_pc < pc;            \
              block_end_pc += type##_instruction_width)                       \
            {                                                                 \
              smc_write_##type##_yes();                                       \
            }                                                                 \
            unconditional_branch_write_##type##_yes();                        \
                                                                              \
            update_metadata_area_end(block_end_pc);                           \
                                                                              \
            return comparison_ptr + block_prologue_size;                      \
          }                                                                   \
        }                                                                     \
      }                                                                       \
                                                                              \
      block_ptr_address = (u32 **)(block_ptr + 1);                            \
      block_ptr = (u32 *)block_ptr[1];                                        \
    }                                                                         \
                                                                              \
    /* If we're here, then we can't reuse a native code block for this GBA    \
     * code block. We continue the code reuse metadata (hashtable of linked   \
     * lists) at block_ptr_address, pointing into a new code reuse metadata   \
     * structure. First, check whether we have enough space before the limit  \
     * to store the PC, Next pointer, GBA code size and GBA code. If we       \
     * don't, declare failure. */                                             \
    StatsAddWritableRecompilation((block_end_pc - block_start_pc) /           \
     type##_instruction_width);                                               \
    if (translation_ptr + sizeof(u32) + sizeof(void*) + sizeof(u32) +         \
     (block_end_pc - block_start_pc) > translation_cache_limit)               \
    {                                                                         \
      translation_flush_count++;                                              \
                                                                              \
      flush_translation_cache(translation_region, FLUSH_REASON_FULL_CACHE);   \
                                                                              \
      return NULL;                                                            \
    }                                                                         \
    /* The previous block needs to refer to this one. We kept                 \
     * block_ptr_address around so we could continue the linked list. */      \
    *(u8**) block_ptr_address = translation_ptr;                              \
    *(u32*) translation_ptr = block_start_pc;                                 \
    translation_ptr += sizeof(u32);                                           \
    *(void**) translation_ptr = NULL; /* No Next yet */                       \
    translation_ptr += sizeof(void*);                                         \
    *(u32*) translation_ptr = block_end_pc - block_start_pc;                  \
    translation_ptr += sizeof(u32);                                           \
                                                                              \
    memcpy(translation_ptr, opcodes.type, block_end_pc - block_start_pc);     \
    translation_ptr += block_end_pc - block_start_pc;                         \
                                                                              \
    if ((block_end_pc - block_start_pc) & 2)                                  \
      translation_ptr += 2; /* Add the alignment */                           \
    update_trampoline = translation_ptr; /* Native code starts after */       \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    scan_block(type, no);                                                     \
  }                                                                           \
                                                                              \
  generate_block_prologue();                                                  \
                                                                              \
  for(i = 0; i < block_exit_position; i++)                                    \
  {                                                                           \
    branch_target = block_exits[i].branch_target;                             \
                                                                              \
    if((branch_target > block_start_pc) &&                                    \
     (branch_target < block_end_pc))                                          \
    {                                                                         \
      block_data.type[(branch_target - block_start_pc) /                      \
       type##_instruction_width].update_cycles = 1;                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  /* Dead flag elimination is a sort of second pass. It works on the          \
   * instructions in reverse, skipping processing to calculate the status of  \
   * the flags if they will soon be overwritten. */                           \
  type##_dead_flag_eliminate();                                               \
                                                                              \
  block_exit_position = 0;                                                    \
  block_data_position = 0;                                                    \
                                                                              \
  /* Finally, we take all of that data and actually generate native code. */  \
  while(pc != block_end_pc)                                                   \
  {                                                                           \
    block_data.type[block_data_position].block_offset = translation_ptr;      \
    type##_base_cycles();                                                     \
                                                                              \
    translate_##type##_instruction();                                         \
    mips_emit_tne();                                                          \
    block_data_position++;                                                    \
                                                                              \
    /* If it went too far the cache needs to be flushed and the process       \
       restarted. Because we might already be nested several stages in        \
       a simple recursive call here won't work, it has to pedal out to        \
       the beginning. */                                                      \
                                                                              \
    if(translation_ptr > translation_cache_limit)                             \
    {                                                                         \
      translation_flush_count++;                                              \
                                                                              \
      flush_translation_cache(translation_region, FLUSH_REASON_FULL_CACHE);   \
                                                                              \
      return NULL;                                                            \
    }                                                                         \
                                                                              \
    /* If the next instruction is a block entry point update the              \
       cycle counter and update */                                            \
    if(block_data.type[block_data_position].update_cycles == 1)               \
    {                                                                         \
      generate_cycle_update();                                                \
    }                                                                         \
  }                                                                           \
                                                                              \
  if (translation_gate_required)                                              \
  {                                                                           \
    generate_translation_gate(type);                                          \
  }                                                                           \
                                                                              \
  for(i = 0; i < block_exit_position; i++)                                    \
  {                                                                           \
    branch_target = block_exits[i].branch_target;                             \
                                                                              \
    if((branch_target >= block_start_pc) && (branch_target < block_end_pc))   \
    {                                                                         \
      /* Internal branch, patch to recorded address */                        \
      translation_target =                                                    \
       block_data.type[(branch_target - block_start_pc) /                     \
        type##_instruction_width].block_offset;                               \
                                                                              \
      generate_branch_patch_unconditional(block_exits[i].branch_source,       \
       translation_target);                                                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      /* This branch exits the basic block. If the branch target is in a      \
       * read-only code area (the BIOS or the Game Pak ROM), we can link the  \
       * block statically below. THIS BEHAVIOUR NEEDS TO BE DUPLICATED IN THE \
       * EMITTER. Please see your emitter's generate_branch_no_cycle_update   \
       * macro for more information. */                                       \
      if (branch_target < 0x00004000 /* BIOS */                               \
      || (branch_target >= 0x08000000 && branch_target < 0x0E000000))         \
      {                                                                       \
        if (i != external_block_exit_position)                                \
        {                                                                     \
          memcpy(&block_exits[external_block_exit_position], &block_exits[i], \
            sizeof(block_exit_type));                                         \
        }                                                                     \
        external_block_exit_position++;                                       \
      }                                                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  switch (translation_region)                                                 \
  {                                                                           \
    case TRANSLATION_REGION_READONLY:                                         \
      readonly_next_code = translation_ptr;                                   \
      break;                                                                  \
                                                                              \
    case TRANSLATION_REGION_WRITABLE:                                         \
      writable_next_code = translation_ptr;                                   \
      break;                                                                  \
  }                                                                           \
                                                                              \
  update_metadata_area_end(pc);                                               \
                                                                              \
  /* Go compile all the external branches into read-only code areas. */       \
  for(i = 0; i < external_block_exit_position; i++)                           \
  {                                                                           \
    branch_target = block_exits[i].branch_target;                             \
/*printf("link %08x\n", branch_target);*/\
    type##_link_block();                                                      \
    if(translation_target == NULL)                                            \
      return NULL;                                                            \
    generate_branch_patch_unconditional(                                      \
     block_exits[i].branch_source, translation_target);                       \
  }                                                                           \
                                                                              \
  u8 *flush_addr;                                                             \
  for(flush_addr= update_trampoline; flush_addr < translation_ptr + CACHE_LINE_SIZE; )\
  {                                                                           \
    FlushCacheLine(flush_addr);                                               \
                                                                              \
    flush_addr += CACHE_LINE_SIZE;                                            \
  }                                                                           \
                                                                              \
  return update_trampoline + block_prologue_size;                             \
}                                                                             \

static u16 block_checksum_arm(u32 block_data_count);
static u16 block_checksum_thumb(u32 block_data_count);

static void update_metadata_area_start(u32 pc);
static void update_metadata_area_end(u32 pc);

translate_block_builder(arm);
translate_block_builder(thumb);

static u16 block_checksum_arm(u32 opcode_count)
{
	// Init: Checksum = NOT Opcodes[0]
	// assert opcode_count > 0;
	u32 result = ~opcodes.arm[0], i;
	for (i = 1; i < opcode_count; i++)
	{
		// Step: Checksum = Checksum XOR (Opcodes[N] SHR (N AND 7))
		// for fewer collisions if instructions are similar at different places
		result ^= opcodes.arm[i] >> (i & 7);
	}
	// Final: Map into bits 15..1, clear bit 0 to indicate ARM
	return (u16) (((result >> 16) ^ result) & 0xFFFE);
}

static u16 block_checksum_thumb(u32 opcode_count)
{
	// Init: Checksum = NOT Opcodes[0]
	// assert opcode_count > 0;
	u16 result = ~opcodes.thumb[0];
	u32 i;
	for (i = 1; i < opcode_count; i++)
	{
		// Step: Checksum = Checksum XOR (Opcodes[N] SHR (N AND 7))
		// for fewer collisions if instructions are similar at different places
		result ^= opcodes.thumb[i] >> (i & 7);
	}
	// Final: Set bit 0 to indicate Thumb
	return result | 0x0001;
}

static void update_metadata_area_start(u32 pc)
{
  switch(pc >> 24)
  {
    case 0x02: /* EWRAM */
      if((pc < ewram_code_min) || (ewram_code_min == 0xFFFFFFFF))
        ewram_code_min = pc;
      break;

    case 0x03: /* IWRAM */
      if((pc < iwram_code_min) || (iwram_code_min == 0xFFFFFFFF))
        iwram_code_min = pc;
      break;

    case 0x06: /* VRAM */
      if((pc < vram_code_min) || (vram_code_min == 0xFFFFFFFF))
        vram_code_min = pc;
      break;
  }
}

static void update_metadata_area_end(u32 pc)
{
  switch(pc >> 24)
  {
    case 0x02: /* EWRAM */
      if((pc > ewram_code_max) || (ewram_code_max == 0xFFFFFFFF))
        ewram_code_max = pc;
      break;

    case 0x03: /* IWRAM */
      if((pc > iwram_code_max) || (iwram_code_max == 0xFFFFFFFF))
        iwram_code_max = pc;
      break;

    case 0x06: /* VRAM */
      if((pc > vram_code_max) || (vram_code_max == 0xFFFFFFFF))
        vram_code_max = pc;
      break;
  }
}

static void partial_clear_metadata_arm(u16* metadata, u16* metadata_area_start, u16* metadata_area_end);
static void partial_clear_metadata_thumb(u16* metadata, u16* metadata_area_start, u16* metadata_area_end);

/*
 * Starts a Partial Flush at the Metadata Entry for the Data Word residing at
 * the given GBA address, and all adjacent Metadata Entries.
 */
void partial_clear_metadata(u32 address)
{
  // 1. Determine where the Metadata Entry for this Data Word is.
  u16 *metadata;

  switch (address >> 24)
  {
    case 0x02: /* EWRAM */
      metadata = ewram_metadata + (address & 0x3FFFC);
      break;
    case 0x03: /* IWRAM */
      metadata = iwram_metadata + (address & 0x7FFC);
      break;
    case 0x06: /* VRAM */
      if (address & 0x10000)
        metadata = vram_metadata + (address & 0x17FFC);
      else
        metadata = vram_metadata + (address & 0xFFFC);
      break;
    default:   /* no metadata */
      return;
  }

  // 2. If there was a Metadata Entry, and it's not code, the Partial Flush
  // is done.
  if ((metadata[3] & 0x3) == 0)
    return;

  Stats.PartialFlushCount++;

  // 3. Prepare for wrapping in the Metadata Area if there's code at the
  // boundaries of the Data Area.
  u16 *metadata_area, *metadata_area_end;
  switch (address >> 24)
  {
    case 0x02: /* EWRAM */
      metadata_area = ewram_metadata;
      metadata_area_end = ewram_metadata + 0x40000;
      break;
    case 0x03: /* IWRAM */
      metadata_area = iwram_metadata;
      metadata_area_end = iwram_metadata + 0x8000;
      break;
    case 0x06: /* VRAM */
      metadata_area = vram_metadata;
      metadata_area_end = vram_metadata + 0x18000;
      break;
  }

  u16 contents = metadata[3];
  if (contents & 0x1)
    partial_clear_metadata_thumb(metadata, metadata_area, metadata_area_end);
  if (contents & 0x2)
    partial_clear_metadata_arm(metadata, metadata_area, metadata_area_end);
}

static void partial_clear_metadata_thumb(u16* metadata, u16* metadata_area_start, u16* metadata_area_end)
{
  u16* metadata_right = metadata; // Save this pointer to go to the right later
  // 4. Clear tags for code to the left.
  while (1)
  {
    metadata = metadata - 4;
    if (metadata < metadata_area_start)
      metadata = metadata_area_end - 4; // Wrap to the end
    if ((metadata[3] & 0x1) != 0 &&
        (metadata[3] & 0x4) == 0)
    { // code, and NOT an unconditional branch in Thumb
      metadata[3] &= ~0x5;
      metadata[0] /* Thumb 1 */ =
        metadata[2] /* Thumb 2 */ = 0x0000;
    }
    else break;
  }

  // 5. Clear tags for code to the right.
  metadata = metadata_right;
  while (1)
  {
    u16 contents = metadata[3];
    if ((contents & 0x1) != 0)
    { // code
      metadata[3] &= ~0x5;
      metadata[0] /* Thumb 1 */ =
        metadata[2] /* Thumb 2 */ = 0x0000;
      if ((contents & 0x4) != 0)
      { // code, AND an unconditional branch, in Thumb
        break;
      }
    }
    else break;
    metadata = metadata + 4;
    if (metadata == metadata_area_end)
      metadata = metadata_area_start; // Wrap to the beginning
  }
}

static void partial_clear_metadata_arm(u16* metadata, u16* metadata_area_start, u16* metadata_area_end)
{
  u16* metadata_right = metadata; // Save this pointer to go to the right later
  // 4. Clear tags for code to the left.
  while (1)
  {
    metadata = metadata - 4;
    if (metadata < metadata_area_start)
      metadata = metadata_area_end - 4; // Wrap to the end
    if ((metadata[3] & 0x2) != 0 &&
        (metadata[3] & 0x8) == 0)
    { // code, and NOT an unconditional branch in ARM
      metadata[3] &= ~0xA;
      metadata[1] /* ARM */ = 0x0000;
    }
    else break;
  }

  // 5. Clear tags for code to the right.
  metadata = metadata_right;
  while (1)
  {
    u16 contents = metadata[3];
    if ((contents & 0x2) != 0)
    { // code
      metadata[3] &= ~0xA;
      metadata[1] /* ARM */ = 0x0000;
      if ((contents & 0x8) != 0)
      { // code, AND an unconditional branch, in ARM
        break;
      }
    }
    else break;
    metadata = metadata + 4;
    if (metadata == metadata_area_end)
      metadata = metadata_area_start; // Wrap to the beginning
  }
}

void clear_metadata_area(METADATA_AREA_TYPE metadata_area,
  METADATA_CLEAR_REASON_TYPE clear_reason)
{
	Stats.MetadataClearCount[metadata_area][clear_reason]++;
	switch (metadata_area)
	{
		case METADATA_AREA_IWRAM:
			if (clear_reason == CLEAR_REASON_INITIALIZING)
				memset(iwram_metadata, 0, sizeof(iwram_metadata));
			else
			{
				iwram_block_tag_top = MIN_TAG;

				if(iwram_code_min != 0xFFFFFFFF)
				{ // iwramのキャッシュを使用していた場合
					iwram_code_min &= 0x7FFC;
					iwram_code_max &= 0x7FFE;
					if (iwram_code_max & 2)
						// Catch the last Metadata Entry for a 4-byte-aligned Thumb instruction
						iwram_code_max += 2;
					memset(iwram_metadata + iwram_code_min, 0, (iwram_code_max - iwram_code_min) * sizeof(u16));
					iwram_code_min = 0xFFFFFFFF;
					iwram_code_max = 0xFFFFFFFF;
				}
			}
			break;
		case METADATA_AREA_EWRAM:
			if (clear_reason == CLEAR_REASON_INITIALIZING)
				memset(ewram_metadata, 0, sizeof(ewram_metadata));
			else
			{
				ewram_block_tag_top = MIN_TAG;

				if(ewram_code_min != 0xFFFFFFFF)
				{
					ewram_code_min &= 0x3FFFC;
					ewram_code_max &= 0x3FFFE;
					if (ewram_code_max & 2)
						// Catch the last Metadata Entry for a 4-byte-aligned Thumb instruction
						ewram_code_max += 2;
					memset(ewram_metadata + ewram_code_min, 0, (ewram_code_max - ewram_code_min) * sizeof(u16));
					ewram_code_min = 0xFFFFFFFF;
					ewram_code_max = 0xFFFFFFFF;
				}
			}
			break;
		case METADATA_AREA_VRAM:
			if (clear_reason == CLEAR_REASON_INITIALIZING)
				memset(vram_metadata, 0, sizeof(vram_metadata));
			else
			{
				vram_block_tag_top = MIN_TAG;

				// TODO [Opt] Handle the mirroring in this area
				memset(vram_metadata, 0, sizeof(vram_metadata));
			}
			break;
		case METADATA_AREA_ROM:
			memset(rom_branch_hash, 0, sizeof(rom_branch_hash));
			break;
		case METADATA_AREA_BIOS:
			bios_block_tag_top = MIN_TAG;
			memset(bios.metadata, 0, sizeof(bios.metadata));
			break;
	}
}

void flush_translation_cache(TRANSLATION_REGION_TYPE translation_region,
  CACHE_FLUSH_REASON_TYPE flush_reason)
{
	Stats.TranslationFlushCount[translation_region][flush_reason]++;
	switch (translation_region)
	{
		case TRANSLATION_REGION_READONLY:
			Stats.TranslationBytesFlushed[translation_region] +=
				readonly_next_code - readonly_code_cache;
			readonly_next_code = readonly_code_cache;
			switch (flush_reason)
			{
				case FLUSH_REASON_INITIALIZING:
					clear_metadata_area(METADATA_AREA_ROM, CLEAR_REASON_INITIALIZING);
					clear_metadata_area(METADATA_AREA_BIOS, CLEAR_REASON_INITIALIZING);
					break;
				case FLUSH_REASON_LOADING_ROM:
					clear_metadata_area(METADATA_AREA_ROM, CLEAR_REASON_LOADING_ROM);
					clear_metadata_area(METADATA_AREA_BIOS, CLEAR_REASON_LOADING_ROM);
					flush_translation_cache(TRANSLATION_REGION_WRITABLE, FLUSH_REASON_NATIVE_BRANCHING);
					break;
				/* [Read-only cannot be affected by native branching] */
				case FLUSH_REASON_FULL_CACHE:
					clear_metadata_area(METADATA_AREA_ROM, CLEAR_REASON_FULL_CACHE);
					clear_metadata_area(METADATA_AREA_BIOS, CLEAR_REASON_FULL_CACHE);
					flush_translation_cache(TRANSLATION_REGION_WRITABLE, FLUSH_REASON_NATIVE_BRANCHING);
					break;
				default:
					break;
			}
			break;
		case TRANSLATION_REGION_WRITABLE:
			Stats.TranslationBytesFlushed[translation_region] +=
				writable_next_code - writable_code_cache;
			writable_next_code = writable_code_cache;
			memset(writable_area_checksum_hash, 0, sizeof(writable_area_checksum_hash));
			switch (flush_reason)
			{
				case FLUSH_REASON_INITIALIZING:
					clear_metadata_area(METADATA_AREA_IWRAM, CLEAR_REASON_INITIALIZING);
					clear_metadata_area(METADATA_AREA_EWRAM, CLEAR_REASON_INITIALIZING);
					clear_metadata_area(METADATA_AREA_VRAM, CLEAR_REASON_INITIALIZING);
					break;
				/* [Writable cannot be affected by ROM loading] */
				case FLUSH_REASON_NATIVE_BRANCHING:
					clear_metadata_area(METADATA_AREA_IWRAM, CLEAR_REASON_NATIVE_BRANCHING);
					clear_metadata_area(METADATA_AREA_EWRAM, CLEAR_REASON_NATIVE_BRANCHING);
					clear_metadata_area(METADATA_AREA_VRAM, CLEAR_REASON_NATIVE_BRANCHING);
					break;
				case FLUSH_REASON_FULL_CACHE:
					clear_metadata_area(METADATA_AREA_IWRAM, CLEAR_REASON_FULL_CACHE);
					clear_metadata_area(METADATA_AREA_EWRAM, CLEAR_REASON_FULL_CACHE);
					clear_metadata_area(METADATA_AREA_VRAM, CLEAR_REASON_FULL_CACHE);
					break;
				default:
					break;
			}
			break;
	}
}

unsigned int last_readonly= 0;
unsigned int last_writable= 0;
void dump_translation_cache()
{
//  FILE_OPEN(FILE *fp, "fat:/ram_cache.bin", WRITE);
	FILE *fp;
if(last_readonly != (readonly_next_code - readonly_code_cache))
{
  fp = fopen("fat:/ro_cache.bin", "wb");
  fwrite(readonly_code_cache, 1, readonly_next_code - readonly_code_cache, fp);
  fclose(fp);

  last_readonly = readonly_next_code - readonly_code_cache;
}

if(last_writable != (writable_next_code - writable_code_cache))
{
  fp = fopen("fat:/rw_cache.bin", "wb");
  fwrite(writable_code_cache, 1, writable_next_code - writable_code_cache, fp);
  fclose(fp);

  last_writable = writable_next_code - writable_code_cache;
}

  printf("RO:%08X RW:%08X\n", readonly_next_code - readonly_code_cache, writable_next_code - writable_code_cache);
}

void init_cpu() 
{
  u32 i;

  for(i = 0; i < 16; i++)
  {
    reg[i] = 0;
  }

  reg[REG_SP] = 0x03007F00;
  reg[REG_PC] = 0x08000000;
  reg[REG_CPSR] = 0x0000001F;
  reg[CPU_HALT_STATE] = CPU_ACTIVE;
  reg[CPU_MODE] = MODE_USER;
  reg[CHANGED_PC_STATUS] = 0;

  reg_mode[MODE_USER][5] = 0x03007F00;
  reg_mode[MODE_IRQ][5] = 0x03007FA0;
  reg_mode[MODE_FIQ][5] = 0x03007FA0;
  reg_mode[MODE_SUPERVISOR][5] = 0x03007FE0;
}

#define cpu_savestate_body(type)                                              \
{                                                                             \
  FILE_##type(g_state_buffer_ptr,reg, 0x100);                             \
  FILE_##type##_ARRAY(g_state_buffer_ptr,spsr);                           \
  FILE_##type##_ARRAY(g_state_buffer_ptr,reg_mode);                       \
}                                                                             \

void cpu_read_mem_savestate()
cpu_savestate_body(READ_MEM);

void cpu_write_mem_savestate()
cpu_savestate_body(WRITE_MEM);
