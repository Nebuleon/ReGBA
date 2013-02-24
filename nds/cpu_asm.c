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

#define CACHE_LINE_SIZE 32

u32 reg_mode[7][7];
 
u32 cpu_modes[32] =
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

u32 cpu_modes_cpsr[7] = { 0x10, 0x11, 0x12, 0x13, 0x17, 0x1B, 0x1F };

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

u8 rom_translation_cache[ROM_TRANSLATION_CACHE_SIZE];
u8 *rom_translation_ptr = rom_translation_cache;

u8 ram_translation_cache[RAM_TRANSLATION_CACHE_SIZE];
u8 *ram_translation_ptr = ram_translation_cache;
u32 iwram_code_min = 0xFFFFFFFF;
u32 iwram_code_max = 0xFFFFFFFF;
u32 ewram_code_min = 0xFFFFFFFF;
u32 ewram_code_max = 0xFFFFFFFF;

u8 bios_translation_cache[BIOS_TRANSLATION_CACHE_SIZE];
u8 *bios_translation_ptr = bios_translation_cache;

u32 *rom_branch_hash[ROM_BRANCH_HASH_SIZE];

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

u32 bios_mode;

typedef struct
{
  u8 *block_offset;
  u16 flag_data;
  u8 condition;
  u8 update_cycles;
} block_data_type;

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

#define translate_arm_instruction()                                           \
  check_pc_region(pc);                                                        \
  opcode = ADDRESS32(pc_address_block, (pc & 0x7FFF));                        \
  condition = block_data[block_data_position].condition;                      \
                                                                              \
  if((condition != last_condition) || (condition >= 0x20))                    \
  {                                                                           \
                                                                              \
    if((last_condition & 0x0F) != 0x0E)                                       \
    {                                                                         \
      generate_branch_patch_conditional(backpatch_address, translation_ptr);  \
    }                                                                         \
                                                                              \
    last_condition = condition;                                               \
                                                                              \
    condition &= 0x0F;                                                        \
                                                                              \
    if(condition != 0x0E)                                                     \
    {                                                                         \
      arm_conditional_block_header();                                         \
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
      arm_access_memory(store, down, post, u32, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x41:                                                                \
      /* LDR rd, [rn], -imm */                                                \
      arm_access_memory(load, down, post, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x42:                                                                \
      /* STRT rd, [rn], -imm */                                               \
      arm_access_memory(store, down, post, u32, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x43:                                                                \
      /* LDRT rd, [rn], -imm */                                               \
      arm_access_memory(load, down, post, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x44:                                                                \
      /* STRB rd, [rn], -imm */                                               \
      arm_access_memory(store, down, post, u8, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x45:                                                                \
      /* LDRB rd, [rn], -imm */                                               \
      arm_access_memory(load, down, post, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x46:                                                                \
      /* STRBT rd, [rn], -imm */                                              \
      arm_access_memory(store, down, post, u8, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x47:                                                                \
      /* LDRBT rd, [rn], -imm */                                              \
      arm_access_memory(load, down, post, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x48:                                                                \
      /* STR rd, [rn], +imm */                                                \
      arm_access_memory(store, up, post, u32, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x49:                                                                \
      /* LDR rd, [rn], +imm */                                                \
      arm_access_memory(load, up, post, u32, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4A:                                                                \
      /* STRT rd, [rn], +imm */                                               \
      arm_access_memory(store, up, post, u32, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x4B:                                                                \
      /* LDRT rd, [rn], +imm */                                               \
      arm_access_memory(load, up, post, u32, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4C:                                                                \
      /* STRB rd, [rn], +imm */                                               \
      arm_access_memory(store, up, post, u8, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4D:                                                                \
      /* LDRB rd, [rn], +imm */                                               \
      arm_access_memory(load, up, post, u8, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x4E:                                                                \
      /* STRBT rd, [rn], +imm */                                              \
      arm_access_memory(store, up, post, u8, imm);                            \
      break;                                                                  \
                                                                              \
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
      arm_access_memory(store, down, post, u32, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x61:                                                                \
      /* LDR rd, [rn], -rm */                                                 \
      arm_access_memory(load, down, post, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x62:                                                                \
      /* STRT rd, [rn], -rm */                                                \
      arm_access_memory(store, down, post, u32, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x63:                                                                \
      /* LDRT rd, [rn], -rm */                                                \
      arm_access_memory(load, down, post, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x64:                                                                \
      /* STRB rd, [rn], -rm */                                                \
      arm_access_memory(store, down, post, u8, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x65:                                                                \
      /* LDRB rd, [rn], -rm */                                                \
      arm_access_memory(load, down, post, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x66:                                                                \
      /* STRBT rd, [rn], -rm */                                               \
      arm_access_memory(store, down, post, u8, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x67:                                                                \
      /* LDRBT rd, [rn], -rm */                                               \
      arm_access_memory(load, down, post, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x68:                                                                \
      /* STR rd, [rn], +rm */                                                 \
      arm_access_memory(store, up, post, u32, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x69:                                                                \
      /* LDR rd, [rn], +rm */                                                 \
      arm_access_memory(load, up, post, u32, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6A:                                                                \
      /* STRT rd, [rn], +rm */                                                \
      arm_access_memory(store, up, post, u32, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x6B:                                                                \
      /* LDRT rd, [rn], +rm */                                                \
      arm_access_memory(load, up, post, u32, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6C:                                                                \
      /* STRB rd, [rn], +rm */                                                \
      arm_access_memory(store, up, post, u8, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6D:                                                                \
      /* LDRB rd, [rn], +rm */                                                \
      arm_access_memory(load, up, post, u8, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x6E:                                                                \
      /* STRBT rd, [rn], +rm */                                               \
      arm_access_memory(store, up, post, u8, reg);                            \
      break;                                                                  \
                                                                              \
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
  pc += 4                                                                     \

#define arm_flag_status()                                                     \

#define translate_thumb_instruction()                                         \
  flag_status = block_data[block_data_position].flag_data;                    \
  check_pc_region(pc);                                                        \
  last_opcode = opcode;                                                       \
  opcode = ADDRESS16(pc_address_block, (pc & 0x7FFF));                        \
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
    case 0x20:                                                                \
      /* MOV r0, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 0, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x21:                                                                \
      /* MOV r1, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 1, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x22:                                                                \
      /* MOV r2, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 2, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x23:                                                                \
      /* MOV r3, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 3, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x24:                                                                \
      /* MOV r4, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 4, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x25:                                                                \
      /* MOV r5, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 5, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x26:                                                                \
      /* MOV r6, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 6, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x27:                                                                \
      /* MOV r7, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 7, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x28:                                                                \
      /* CMP r0, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 0, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x29:                                                                \
      /* CMP r1, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 1, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2A:                                                                \
      /* CMP r2, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 2, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2B:                                                                \
      /* CMP r3, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 3, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2C:                                                                \
      /* CMP r4, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 4, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2D:                                                                \
      /* CMP r5, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 5, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2E:                                                                \
      /* CMP r6, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 6, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2F:                                                                \
      /* CMP r7, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 7, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x30:                                                                \
      /* ADD r0, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 0, 0, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x31:                                                                \
      /* ADD r1, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 1, 1, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x32:                                                                \
      /* ADD r2, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 2, 2, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x33:                                                                \
      /* ADD r3, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 3, 3, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x34:                                                                \
      /* ADD r4, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 4, 4, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x35:                                                                \
      /* ADD r5, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 5, 5, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x36:                                                                \
      /* ADD r6, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 6, 6, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x37:                                                                \
      /* ADD r7, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 7, 7, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x38:                                                                \
      /* SUB r0, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 0, 0, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x39:                                                                \
      /* SUB r1, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 1, 1, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3A:                                                                \
      /* SUB r2, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 2, 2, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3B:                                                                \
      /* SUB r3, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 3, 3, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3C:                                                                \
      /* SUB r4, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 4, 4, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3D:                                                                \
      /* SUB r5, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 5, 5, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3E:                                                                \
      /* SUB r6, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 6, 6, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3F:                                                                \
      /* SUB r7, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 7, 7, imm);                             \
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
    case 0x48:                                                                \
      /* LDR r0, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 0, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
      break;                                                                  \
                                                                              \
    case 0x49:                                                                \
      /* LDR r1, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 1, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
      break;                                                                  \
                                                                              \
    case 0x4A:                                                                \
      /* LDR r2, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 2, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
      break;                                                                  \
                                                                              \
    case 0x4B:                                                                \
      /* LDR r3, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 3, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
      break;                                                                  \
                                                                              \
    case 0x4C:                                                                \
      /* LDR r4, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 4, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
      break;                                                                  \
                                                                              \
    case 0x4D:                                                                \
      /* LDR r5, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 5, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
      break;                                                                  \
                                                                              \
    case 0x4E:                                                                \
      /* LDR r6, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 6, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
      break;                                                                  \
                                                                              \
    case 0x4F:                                                                \
      /* LDR r7, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 7, 0, 0, pc_relative,                    \
       ((pc & ~2) + (imm << 2) + 4), u32);                                      \
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
    case 0x90:                                                                \
      /* STR r0, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 0, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x91:                                                                \
      /* STR r1, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 1, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x92:                                                                \
      /* STR r2, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 2, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x93:                                                                \
      /* STR r3, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 3, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x94:                                                                \
      /* STR r4, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 4, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x95:                                                                \
      /* STR r5, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 5, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x96:                                                                \
      /* STR r6, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 6, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x97:                                                                \
      /* STR r7, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 7, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x98:                                                                \
      /* LDR r0, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 0, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x99:                                                                \
      /* LDR r1, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 1, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9A:                                                                \
      /* LDR r2, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 2, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9B:                                                                \
      /* LDR r3, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 3, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9C:                                                                \
      /* LDR r4, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 4, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9D:                                                                \
      /* LDR r5, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 5, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9E:                                                                \
      /* LDR r6, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 6, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9F:                                                                \
      /* LDR r7, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 7, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0xA0:                                                                \
      /* ADD r0, pc, +imm */                                                  \
      thumb_load_pc(0);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA1:                                                                \
      /* ADD r1, pc, +imm */                                                  \
      thumb_load_pc(1);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA2:                                                                \
      /* ADD r2, pc, +imm */                                                  \
      thumb_load_pc(2);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA3:                                                                \
      /* ADD r3, pc, +imm */                                                  \
      thumb_load_pc(3);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA4:                                                                \
      /* ADD r4, pc, +imm */                                                  \
      thumb_load_pc(4);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA5:                                                                \
      /* ADD r5, pc, +imm */                                                  \
      thumb_load_pc(5);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA6:                                                                \
      /* ADD r6, pc, +imm */                                                  \
      thumb_load_pc(6);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA7:                                                                \
      /* ADD r7, pc, +imm */                                                  \
      thumb_load_pc(7);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA8:                                                                \
      /* ADD r0, sp, +imm */                                                  \
      thumb_load_sp(0);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA9:                                                                \
      /* ADD r1, sp, +imm */                                                  \
      thumb_load_sp(1);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAA:                                                                \
      /* ADD r2, sp, +imm */                                                  \
      thumb_load_sp(2);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAB:                                                                \
      /* ADD r3, sp, +imm */                                                  \
      thumb_load_sp(3);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAC:                                                                \
      /* ADD r4, sp, +imm */                                                  \
      thumb_load_sp(4);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAD:                                                                \
      /* ADD r5, sp, +imm */                                                  \
      thumb_load_sp(5);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAE:                                                                \
      /* ADD r6, sp, +imm */                                                  \
      thumb_load_sp(6);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAF:                                                                \
      /* ADD r7, sp, +imm */                                                  \
      thumb_load_sp(7);                                                       \
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
    case 0xC0:                                                                \
      /* STMIA r0!, rlist */                                                  \
      thumb_block_memory(store, no, up, 0);                                   \
      break;                                                                  \
                                                                              \
    case 0xC1:                                                                \
      /* STMIA r1!, rlist */                                                  \
      thumb_block_memory(store, no, up, 1);                                   \
      break;                                                                  \
                                                                              \
    case 0xC2:                                                                \
      /* STMIA r2!, rlist */                                                  \
      thumb_block_memory(store, no, up, 2);                                   \
      break;                                                                  \
                                                                              \
    case 0xC3:                                                                \
      /* STMIA r3!, rlist */                                                  \
      thumb_block_memory(store, no, up, 3);                                   \
      break;                                                                  \
                                                                              \
    case 0xC4:                                                                \
      /* STMIA r4!, rlist */                                                  \
      thumb_block_memory(store, no, up, 4);                                   \
      break;                                                                  \
                                                                              \
    case 0xC5:                                                                \
      /* STMIA r5!, rlist */                                                  \
      thumb_block_memory(store, no, up, 5);                                   \
      break;                                                                  \
                                                                              \
    case 0xC6:                                                                \
      /* STMIA r6!, rlist */                                                  \
      thumb_block_memory(store, no, up, 6);                                   \
      break;                                                                  \
                                                                              \
    case 0xC7:                                                                \
      /* STMIA r7!, rlist */                                                  \
      thumb_block_memory(store, no, up, 7);                                   \
      break;                                                                  \
                                                                              \
    case 0xC8:                                                                \
      /* LDMIA r0!, rlist */                                                  \
      thumb_block_memory(load, no, up, 0);                                    \
      break;                                                                  \
                                                                              \
    case 0xC9:                                                                \
      /* LDMIA r1!, rlist */                                                  \
      thumb_block_memory(load, no, up, 1);                                    \
      break;                                                                  \
                                                                              \
    case 0xCA:                                                                \
      /* LDMIA r2!, rlist */                                                  \
      thumb_block_memory(load, no, up, 2);                                    \
      break;                                                                  \
                                                                              \
    case 0xCB:                                                                \
      /* LDMIA r3!, rlist */                                                  \
      thumb_block_memory(load, no, up, 3);                                    \
      break;                                                                  \
                                                                              \
    case 0xCC:                                                                \
      /* LDMIA r4!, rlist */                                                  \
      thumb_block_memory(load, no, up, 4);                                    \
      break;                                                                  \
                                                                              \
    case 0xCD:                                                                \
      /* LDMIA r5!, rlist */                                                  \
      thumb_block_memory(load, no, up, 5);                                    \
      break;                                                                  \
                                                                              \
    case 0xCE:                                                                \
      /* LDMIA r6!, rlist */                                                  \
      thumb_block_memory(load, no, up, 6);                                    \
      break;                                                                  \
                                                                              \
    case 0xCF:                                                                \
      /* LDMIA r7!, rlist */                                                  \
      thumb_block_memory(load, no, up, 7);                                    \
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

#define thumb_flag_status()                                                   \
{                                                                             \
  u16 flag_status = 0;                                                        \
  switch((opcode >> 8) & 0xFF)                                                \
  {                                                                           \
    /* left shift by imm */                                                   \
    case 0x00 ... 0x07:                                                       \
      thumb_flag_modifies_zn();                                               \
      if(((opcode >> 6) & 0x1F) != 0)                                         \
      {                                                                       \
        thumb_flag_modifies_c();                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    /* right shift by imm */                                                  \
    case 0x08 ... 0x17:                                                       \
      thumb_flag_modifies_znc();                                              \
      break;                                                                  \
                                                                              \
    /* add, subtract */                                                       \
    case 0x18 ... 0x1F:                                                       \
      thumb_flag_modifies_all();                                              \
      break;                                                                  \
                                                                              \
    /* mov reg, imm */                                                        \
    case 0x20 ... 0x27:                                                       \
      thumb_flag_modifies_zn();                                               \
      break;                                                                  \
                                                                              \
    /* cmp reg, imm; add, subtract */                                         \
    case 0x28 ... 0x3F:                                                       \
      thumb_flag_modifies_all();                                              \
      break;                                                                  \
                                                                              \
    case 0x40:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* AND rd, rs */                                                    \
          thumb_flag_modifies_zn();                                           \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* EOR rd, rs */                                                    \
          thumb_flag_modifies_zn();                                           \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* LSL rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* LSR rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x41:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* ASR rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* ADC rd, rs */                                                    \
          thumb_flag_modifies_all();                                          \
          thumb_flag_requires_c();                                            \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* SBC rd, rs */                                                    \
          thumb_flag_modifies_all();                                          \
          thumb_flag_requires_c();                                            \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* ROR rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    /* TST, NEG, CMP, CMN */                                                  \
    case 0x42:                                                                \
      if((opcode >> 6) & 0x03)                                                \
      {                                                                       \
        /* NEG, CMP, CMN */                                                   \
        thumb_flag_modifies_all();                                            \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* TST rd, rs */                                                      \
        thumb_flag_modifies_zn();                                             \
      }                                                                       \
      break;                                                                  \
                                                                              \
    /* ORR, MUL, BIC, MVN */                                                  \
    case 0x43:                                                                \
      thumb_flag_modifies_zn();                                               \
      break;                                                                  \
                                                                              \
    case 0x45:                                                                \
      /* CMP rd, rs */                                                        \
      thumb_flag_modifies_all();                                              \
      break;                                                                  \
                                                                              \
    /* mov might change PC (fall through if so) */                            \
    case 0x46:                                                                \
      if((opcode & 0xFF87) != 0x4687)                                         \
        break;                                                                \
                                                                              \
    /* branches (can change PC) */                                            \
    case 0x47:                                                                \
    case 0xBD:                                                                \
    case 0xD0 ... 0xE7:                                                       \
    case 0xF0 ... 0xFF:                                                       \
      thumb_flag_requires_all();                                              \
      break;                                                                  \
  }                                                                           \
  block_data[block_data_position].flag_data = flag_status;                    \
}                                                                             \

u8 *ram_block_ptrs[1024 * 64];
u32 ram_block_tag_top = 0x0101;

u8 *bios_block_ptrs[1024 * 8];
u32 bios_block_tag_top = 0x0101;

// This function will return a pointer to a translated block of code. If it
// doesn't exist it will translate it, if it does it will pass it back.

// type should be "arm", "thumb", or "dual." For arm or thumb the PC should
// be a real PC, for dual the least significant bit will determine if it's
// ARM or Thumb mode.

#define block_lookup_address_pc_arm()                                         \
  pc &= ~0x03                                                                 \

#define block_lookup_address_pc_thumb()                                       \
  pc &= ~0x01                                                                 \

#define block_lookup_address_pc_dual()                                        \
  u32 thumb = pc & 0x01;                                                      \
                                                                              \
  if(thumb)                                                                   \
  {                                                                           \
    pc &= ~0x01;                                                              \
    reg[REG_CPSR] |= 0x20;                                                    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    pc = (pc + 2) & ~0x03;                                                    \
    reg[REG_CPSR] &= ~0x20;                                                   \
  }                                                                           \

#define ram_translation_region  TRANSLATION_REGION_RAM
#define rom_translation_region  TRANSLATION_REGION_ROM
#define bios_translation_region TRANSLATION_REGION_BIOS

#define block_lookup_translate_arm(mem_type, smc_enable)                      \
  translation_result = translate_block_arm(pc, mem_type##_translation_region, \
   smc_enable)                                                                \

#define block_lookup_translate_thumb(mem_type, smc_enable)                    \
  translation_result = translate_block_thumb(pc,                              \
   mem_type##_translation_region, smc_enable)                                 \

#define block_lookup_translate_dual(mem_type, smc_enable)                     \
  if(thumb)                                                                   \
  {                                                                           \
    translation_result = translate_block_thumb(pc,                            \
     mem_type##_translation_region, smc_enable);                              \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    translation_result = translate_block_arm(pc,                              \
     mem_type##_translation_region, smc_enable);                              \
  }                                                                           \

// 0x0101 is the smallest tag that can be used. 0xFFFF is marked
// in the middle of blocks and used for write guarding, it doesn't
// indicate a valid block either (it's okay to compile a new block
// that overlaps the earlier one, although this should be relatively
// uncommon)

#define fill_tag_arm(mem_type)                                                \
  location[0] = mem_type##_block_tag_top;                                     \
  location[1] = 0xFFFF                                                        \

#define fill_tag_thumb(mem_type)                                              \
  *location = mem_type##_block_tag_top                                        \

#define fill_tag_dual(mem_type)                                               \
  if(thumb)                                                                   \
    fill_tag_thumb(mem_type);                                                 \
  else                                                                        \
    fill_tag_arm(mem_type)                                                    \

#define block_lookup_translate(instruction_type, mem_type, smc_enable)        \
  block_tag = *location;                                                      \
  if((block_tag < 0x0101) || (block_tag == 0xFFFF))                           \
  {                                                                           \
    __label__ redo;                                                           \
    s32 translation_result;                                                   \
                                                                              \
    redo:                                                                     \
                                                                              \
    translation_recursion_level++;                                            \
    block_address = mem_type##_translation_ptr + block_prologue_size;         \
    mem_type##_block_ptrs[mem_type##_block_tag_top] = block_address;          \
    fill_tag_##instruction_type(mem_type);                                    \
    mem_type##_block_tag_top++;                                               \
                                                                              \
    block_lookup_translate_##instruction_type(mem_type, smc_enable);          \
    translation_recursion_level--;                                            \
                                                                              \
    /* If the translation failed then pass that failure on if we're in        \
       a recursive level, or try again if we've hit the bottom. */            \
    if(translation_result == -1)                                              \
    {                                                                         \
      if(translation_recursion_level)                                         \
        return NULL;                                                          \
                                                                              \
      goto redo;                                                              \
    }                                                                         \
                                                                              \
    if(translation_recursion_level == 0)                                      \
      translate_invalidate_dcache();                                          \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    block_address = mem_type##_block_ptrs[block_tag];                         \
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

// Where emulation started
#define block_lookup_address_body(type)                                       \
{                                                                             \
  u16 *location;                                                              \
  u32 block_tag;                                                              \
  u8 *block_address;                                                          \
                                                                              \
/*printf("recursion in %d\n", recursion_level++);*/                                  \
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
      location = (u16 *)(bios_rom + pc + 0x4000);                             \
      block_lookup_translate(type, bios, 0);                                  \
      if(translation_recursion_level == 0)                                    \
        bios_region_read_allow();                                             \
      break;                                                                  \
                                                                              \
    case 0x2:                                                                 \
      location = (u16 *)(ewram + (pc & 0x7FFF) + ((pc & 0x38000) << 1));      \
      block_lookup_translate(type, ram, 1);                                   \
      break;                                                                  \
                                                                              \
    case 0x3:                                                                 \
      location = (u16 *)(iwram + (pc & 0x7FFF));                              \
      block_lookup_translate(type, ram, 1);                                   \
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
        s32 translation_result;                                               \
                                                                              \
        redo:                                                                 \
                                                                              \
        translation_recursion_level++;                                        \
        ((u32 *)rom_translation_ptr)[0] = pc;                                 \
        ((u32 **)rom_translation_ptr)[1] = NULL;                              \
        *block_ptr_address = (u32 *)rom_translation_ptr;                      \
        rom_translation_ptr += 8;                                             \
        block_address = rom_translation_ptr + block_prologue_size;            \
        block_lookup_translate_##type(rom, 0);                                \
        translation_recursion_level--;                                        \
                                                                              \
        /* If the translation failed then pass that failure on if we're in    \
         a recursive level, or try again if we've hit the bottom. */          \
        if(translation_result == -1)                                          \
        {                                                                     \
          if(translation_recursion_level)                                     \
            return NULL;                                                      \
                                                                              \
          goto redo;                                                          \
        }                                                                     \
                                                                              \
        if(translation_recursion_level == 0)                                  \
          translate_invalidate_dcache();                                      \
      }                                                                       \
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
        /*char buffer[256];                                                     \
        video_resolution(FRAME_MENU);                                         \
        sprintf(buffer, "bad jump %x (%x)\n", (int)pc, (int)reg[REG_PC]);     \
        PRINT_STRING_BG(buffer, COLOR_WHITE, COLOR_BLACK, 0, 10);             \
        flip_screen();                                                        \
        error_msg("");*/                                                        \
        __asm__ __volatile__("syscall 30\n\t");                                \
        quit(0);                                                               \
      }                                                                       \
      block_address = (u8 *)(-1);                                             \
      break;                                                                  \
  }                                                                           \
                                                                              \
/*printf("recursion out %d\n", --recursion_level);*/                                  \
if(recursion_level==0)\
{\
    /*dump_translation_cache();*/ \
}\
  return block_address;                                                       \
}                                                                             \


/*
//    u32 index;\
//    for(index= ram_translation_cache; index< ram_translation_ptr; index++ )\
//        printf("%08x\n", *(((u32*)ram_translation_cache)+index));\
*/

u8 *block_lookup_address_arm(u32 pc)
block_lookup_address_body(arm);

u8 *block_lookup_address_thumb(u32 pc)
block_lookup_address_body(thumb);

u8 *block_lookup_address_dual(u32 pc)
block_lookup_address_body(dual);

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
  ((opcode >= 0xF000000) && (!swi_hle_handle[((opcode >> 16) & 0xFF)][bios_mode])))       \

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
  block_data[block_data_position].condition = _condition;                     \
  switch((opcode >> 20) & 0xFF)                                               \
  {                                                                           \
    case 0x01:                                                                \
    case 0x03:                                                                \
    case 0x09:                                                                \
    case 0x0B:                                                                \
    case 0x0D:                                                                \
    case 0x0F:                                                                \
      if((((opcode >> 5) & 0x03) == 0) || ((opcode & 0x90) != 0x90))          \
        block_data[block_data_position].condition |= 0x20;                    \
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
        block_data[block_data_position].condition |= 0x20;                    \
      break;                                                                  \
                                                                              \
    case 0x12:                                                                \
      if(((opcode & 0x90) != 0x90) && !(opcode & 0x10))                       \
        block_data[block_data_position].condition |= 0x20;                    \
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
      block_data[block_data_position].condition |= 0x20;                      \
    break;                                                                    \
  }                                                                           \

#define arm_link_block()                                                      \
  translation_target = block_lookup_address_arm(branch_target)                \

#define arm_instruction_width 4

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
    (!swi_hle_handle[opcode & 0xFF][bios_mode])) ||                           \
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
  needed_mask = block_data[block_data_position].flag_data >> 8;               \
                                                                              \
  block_data_position--;                                                      \
  while(block_data_position >= 0)                                             \
  {                                                                           \
    flag_status = block_data[block_data_position].flag_data;                  \
    block_data[block_data_position].flag_data =                               \
     (flag_status & needed_mask);                                             \
    needed_mask &= ~((flag_status >> 4) & 0x0F);                              \
    needed_mask |= flag_status >> 8;                                          \
    block_data_position--;                                                    \
  }                                                                           \
}                                                                             \

#define MAX_BLOCK_SIZE 8192
#define MAX_EXITS      256

block_data_type block_data[MAX_BLOCK_SIZE];
block_exit_type block_exits[MAX_EXITS];

#define smc_write_arm_yes()                                                   \
  if(ADDRESS32(pc_address_block, (block_end_pc & 0x7FFC) - 0x8000) == 0x0000) \
  {                                                                           \
    ADDRESS32(pc_address_block, (block_end_pc & 0x7FFC) - 0x8000) =           \
     0xFFFFFFFF;                                                              \
  }                                                                           \

#define smc_write_thumb_yes()                                                 \
  if(ADDRESS16(pc_address_block, (block_end_pc & 0x7FFE) - 0x8000) == 0x0000) \
  {                                                                           \
    ADDRESS16(pc_address_block, (block_end_pc & 0x7FFE) - 0x8000) = 0xFFFF;   \
  }                                                                           \

#define smc_write_arm_no()                                                    \

#define smc_write_thumb_no()                                                  \

#define scan_block(type, smc_write_op)                                        \
{                                                                             \
  __label__ block_end;                                                        \
  /* Find the end of the block */                                             \
  do                                                                          \
  {                                                                           \
    check_pc_region(block_end_pc);                                            \
    smc_write_##type##_##smc_write_op();                                      \
    type##_load_opcode();                                                     \
    type##_flag_status();                                                     \
                                                                              \
    if(type##_exit_point)                                                     \
    {                                                                         \
      /* Branch/branch with link */                                           \
      if(type##_opcode_branch)                                                \
      {                                                                       \
        __label__ no_direct_branch;                                           \
        type##_branch_target();                                               \
        block_exits[block_exit_position].branch_target = branch_target;       \
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
        block_exit_position++;                                                \
      }                                                                       \
                                                                              \
      type##_set_condition(condition | 0x10);                                 \
                                                                              \
      /* Only unconditional branches can end the block. */                    \
      if(type##_opcode_unconditional_branch)                                  \
      {                                                                       \
        /* Check to see if any prior block exits branch after here,           \
           if so don't end the block. Starts from the top and works           \
           down because the most recent branch is most likely to              \
           join after the end (if/then form) */                               \
        for(i = block_exit_position - 2; i >= 0; i--)                         \
        {                                                                     \
          if(block_exits[i].branch_target == block_end_pc)                    \
            break;                                                            \
        }                                                                     \
                                                                              \
        if(i < 0)                                                             \
          break;                                                              \
      }                                                                       \
      if(block_exit_position == MAX_EXITS)                                    \
        break;                                                                \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      type##_set_condition(condition);                                        \
    }                                                                         \
                                                                              \
    for(i = 0; i < translation_gate_targets; i++)                             \
    {                                                                         \
      if(block_end_pc == translation_gate_target_pc[i])                       \
        goto block_end;                                                       \
    }                                                                         \
                                                                              \
    block_data[block_data_position].update_cycles = 0;                        \
    block_data_position++;                                                    \
    if((block_data_position == MAX_BLOCK_SIZE) ||                             \
     (block_end_pc == 0x3007FF0) || (block_end_pc == 0x203FFF0))              \
    {                                                                         \
      break;                                                                  \
    }                                                                         \
  } while(1);                                                                 \
                                                                              \
  block_end:;                                                                 \
}                                                                             \

#define arm_fix_pc()                                                          \
  pc &= ~0x03                                                                 \

#define thumb_fix_pc()                                                        \
  pc &= ~0x01                                                                 \

#define translate_block_builder(type)                                         \
s32 translate_block_##type(u32 pc, TRANSLATION_REGION_TYPE                    \
 translation_region, u32 smc_enable)                                          \
{                                                                             \
  u32 opcode = 0;                                                             \
  u32 last_opcode;                                                            \
  u32 condition;                                                              \
  u32 last_condition;                                                         \
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
  block_exit_type external_block_exits[MAX_EXITS];                            \
                                                                              \
  generate_block_extra_vars_##type();                                         \
  type##_fix_pc();                                                            \
                                                                              \
  if(pc_address_block == NULL)                                                \
    pc_address_block = load_gamepak_page(pc_region & 0x3FF);                  \
                                                                              \
  switch(translation_region)                                                  \
  {                                                                           \
    case TRANSLATION_REGION_RAM:                                              \
      if(pc >= 0x3000000)                                                     \
      {                                                                       \
        if((pc < iwram_code_min) || (iwram_code_min == 0xFFFFFFFF))           \
          iwram_code_min = pc;                                                \
      }                                                                       \
      else                                                                    \
                                                                              \
      if(pc >= 0x2000000)                                                     \
      {                                                                       \
        if((pc < ewram_code_min) || (ewram_code_min == 0xFFFFFFFF))           \
          ewram_code_min = pc;                                                \
      }                                                                       \
                                                                              \
      translation_ptr = ram_translation_ptr;                                  \
      translation_cache_limit =                                               \
       ram_translation_cache + RAM_TRANSLATION_CACHE_SIZE -                   \
       TRANSLATION_CACHE_LIMIT_THRESHOLD;                                     \
      break;                                                                  \
                                                                              \
    case TRANSLATION_REGION_ROM:                                              \
      translation_ptr = rom_translation_ptr;                                  \
      translation_cache_limit =                                               \
       rom_translation_cache + ROM_TRANSLATION_CACHE_SIZE -                   \
       TRANSLATION_CACHE_LIMIT_THRESHOLD;                                     \
      break;                                                                  \
                                                                              \
    case TRANSLATION_REGION_BIOS:                                             \
      translation_ptr = bios_translation_ptr;                                 \
      translation_cache_limit = bios_translation_cache +                      \
       BIOS_TRANSLATION_CACHE_SIZE;                                           \
      break;                                                                  \
  }                                                                           \
                                                                              \
  generate_block_prologue();                                                  \
                                                                              \
  /* This is a function because it's used a lot more than it might seem (all  \
     of the data processing functions can access it), and its expansion was   \
     massacreing the compiler. */                                             \
                                                                              \
  if(smc_enable)                                                              \
  {                                                                           \
    scan_block(type, yes);                                                    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    scan_block(type, no);                                                     \
  }                                                                           \
                                                                              \
  for(i = 0; i < block_exit_position; i++)                                    \
  {                                                                           \
    branch_target = block_exits[i].branch_target;                             \
                                                                              \
    if((branch_target > block_start_pc) &&                                    \
     (branch_target < block_end_pc))                                          \
    {                                                                         \
      block_data[(branch_target - block_start_pc) /                           \
       type##_instruction_width].update_cycles = 1;                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  type##_dead_flag_eliminate();                                               \
                                                                              \
  block_exit_position = 0;                                                    \
  block_data_position = 0;                                                    \
                                                                              \
  last_condition = 0x0E;                                                      \
                                                                              \
  while(pc != block_end_pc)                                                   \
  {                                                                           \
    block_data[block_data_position].block_offset = translation_ptr;           \
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
      switch(translation_region)                                              \
      {                                                                       \
        case TRANSLATION_REGION_RAM:                                          \
          flush_translation_cache_ram();                                      \
          break;                                                              \
                                                                              \
        case TRANSLATION_REGION_ROM:                                          \
          flush_translation_cache_rom();                                      \
          break;                                                              \
                                                                              \
        case TRANSLATION_REGION_BIOS:                                         \
          flush_translation_cache_bios();                                     \
          break;                                                              \
      }                                                                       \
                                                                              \
      return -1;                                                              \
    }                                                                         \
                                                                              \
    /* If the next instruction is a block entry point update the              \
       cycle counter and update */                                            \
    if(block_data[block_data_position].update_cycles == 1)                    \
    {                                                                         \
      generate_cycle_update();                                                \
    }                                                                         \
  }                                                                           \
                                                                              \
  for(i = 0; i < translation_gate_targets; i++)                               \
  {                                                                           \
    if(pc == translation_gate_target_pc[i])                                   \
    {                                                                         \
      generate_translation_gate(type);                                        \
      break;                                                                  \
    }                                                                         \
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
       block_data[(branch_target - block_start_pc) /                          \
        type##_instruction_width].block_offset;                               \
                                                                              \
      generate_branch_patch_unconditional(block_exits[i].branch_source,       \
       translation_target);                                                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      /* External branch, save for later */                                   \
      external_block_exits[external_block_exit_position].branch_target =      \
       branch_target;                                                         \
      external_block_exits[external_block_exit_position].branch_source =      \
       block_exits[i].branch_source;                                          \
      external_block_exit_position++;                                         \
    }                                                                         \
  }                                                                           \
                                                                              \
  switch(translation_region)                                                  \
  {                                                                           \
    case TRANSLATION_REGION_RAM:                                              \
      if(pc >= 0x3000000)                                                     \
      {                                                                       \
        if((pc > iwram_code_max) || (iwram_code_max == 0xFFFFFFFF))           \
          iwram_code_max = pc;                                                \
      }                                                                       \
      else                                                                    \
                                                                              \
      if(pc >= 0x2000000)                                                     \
      {                                                                       \
        if((pc > ewram_code_max) || (ewram_code_max == 0xFFFFFFFF))           \
          ewram_code_max = pc;                                                \
      }                                                                       \
                                                                              \
      ram_translation_ptr = translation_ptr;                                  \
      break;                                                                  \
                                                                              \
    case TRANSLATION_REGION_ROM:                                              \
      rom_translation_ptr = translation_ptr;                                  \
      break;                                                                  \
                                                                              \
    case TRANSLATION_REGION_BIOS:                                             \
      bios_translation_ptr = translation_ptr;                                 \
      break;                                                                  \
  }                                                                           \
                                                                              \
  for(i = 0; i < external_block_exit_position; i++)                           \
  {                                                                           \
    branch_target = external_block_exits[i].branch_target;                    \
/*printf("link %08x rom %08x\n", branch_target, translation_ptr);*/\
    type##_link_block();                                                      \
    if(translation_target == NULL)                                            \
      return -1;                                                              \
    generate_branch_patch_unconditional(                                      \
     external_block_exits[i].branch_source, translation_target);              \
  }                                                                           \
                                                                              \
  u8 *flush_addr;                                                             \
  for(flush_addr= update_trampoline; flush_addr < translation_ptr + CACHE_LINE_SIZE; )\
  {                                                                           \
    __asm__ __volatile__ ("cache 0x15, 0(%0)" : : "r"(flush_addr));           \
                                                                              \
    flush_addr += CACHE_LINE_SIZE;                                            \
  }                                                                           \
  for(flush_addr= update_trampoline; flush_addr < translation_ptr + CACHE_LINE_SIZE; )\
  {                                                                           \
    __asm__ __volatile__ ("cache 0x10, 0(%0)" : : "r"(flush_addr));           \
                                                                              \
    flush_addr += CACHE_LINE_SIZE;                                            \
  }                                                                           \
                                                                              \
  return 0;                                                                   \
}                                                                             \

translate_block_builder(arm);
translate_block_builder(thumb);

void flush_translation_cache_ram()
{
  invalidate_icache_region(ram_translation_cache,
   (ram_translation_ptr - ram_translation_cache) + 0x100);

  ram_translation_ptr = ram_translation_cache;
  ram_block_tag_top = 0x0101;

  if(iwram_code_min != 0xFFFFFFFF)
  { // iwramのキャッシュを使用していた場合
    iwram_code_min &= 0x7FFF;
    iwram_code_max &= 0x7FFF;
    memset(iwram + iwram_code_min, 0, iwram_code_max - iwram_code_min + 1);
  }

  if(ewram_code_min != 0xFFFFFFFF)
  {
    u32 ewram_code_min_page;
    u32 ewram_code_max_page;
    u32 ewram_code_min_offset;
    u32 ewram_code_max_offset;
    u32 i;

    ewram_code_min &= 0x3FFFF;
    ewram_code_max &= 0x3FFFF;

    ewram_code_min_page = ewram_code_min >> 15;
    ewram_code_max_page = ewram_code_max >> 15;
    ewram_code_min_offset = ewram_code_min & 0x7FFF;
    ewram_code_max_offset = ewram_code_max & 0x7FFF;

    if(ewram_code_min_page == ewram_code_max_page)
    {
      memset(ewram + (ewram_code_min_page * 0x10000) +
       ewram_code_min_offset, 0,
       ewram_code_max_offset - ewram_code_min_offset + 1);
    }
    else
    {
      memset(ewram + (ewram_code_min_page * 0x10000) + ewram_code_min_offset, 0, 0x8000);
      for(i = ewram_code_min_page + 1; i < ewram_code_max_page; i++)
        memset(ewram + (i * 0x10000), 0, 0x8000);

      memset(ewram + (ewram_code_max_page * 0x10000), 0, ewram_code_max_offset + 1);

    }
  }

  iwram_code_min = 0xFFFFFFFF;
  iwram_code_max = 0xFFFFFFFF;
  ewram_code_min = 0xFFFFFFFF;
  ewram_code_max = 0xFFFFFFFF;
}

void flush_translation_cache_rom()
{
  invalidate_icache_region(rom_translation_cache,
   rom_translation_ptr - rom_translation_cache + 0x100);

  rom_translation_ptr = rom_translation_cache;
  memset(rom_branch_hash, 0, sizeof(rom_branch_hash));
}

void flush_translation_cache_bios()
{
  invalidate_icache_region(bios_translation_cache,
   bios_translation_ptr - bios_translation_cache + 0x100);

  bios_block_tag_top = 0x0101;
  bios_translation_ptr = bios_translation_cache;
  memset(bios_rom + 0x4000, 0, 0x4000);
}

void dump_translation_cache()
{
//  FILE_OPEN(FILE *fp, "mmc:\\ram_cache.bin", WRITE);
  FILE *fp = fopen("mmc:\\ram_cache.bin", "wb");
  fwrite(ram_translation_cache, 1, ram_translation_ptr - ram_translation_cache, fp);
  fclose(fp);
  
  fp = fopen("mmc:\\rom_cache.bin", "wb");
  fwrite(rom_translation_cache, 1, rom_translation_ptr - rom_translation_cache, fp);
  fclose(fp);
  
  fp = fopen("mmc:\\bios_cache.bin", "wb");
  fwrite(bios_translation_cache, bios_translation_ptr - bios_translation_cache, 1, fp);
  fclose(fp);
/*  char print_buffer[256];
  sprintf(print_buffer, "RAM:%08X ROM:%08X BIOS:%08X", ram_translation_ptr - ram_translation_cache, rom_translation_ptr - rom_translation_cache, bios_translation_ptr - bios_translation_cache);
  PRINT_STRING_BG(print_buffer, 0xFFFF, 0x000, 0, 10);*/
  printf("RAM:%08X ROM:%08X BIOS:%08X \n", ram_translation_ptr - ram_translation_cache, rom_translation_ptr - rom_translation_cache, bios_translation_ptr - bios_translation_cache);
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
