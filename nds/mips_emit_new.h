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

#ifndef MIPS_EMIT_H
#define MIPS_EMIT_H


u32 mips_update_gba(u32 pc);

static u8 cycle_multiply(u32 _rs);

// Although these are defined as a function, don't call them as
// such (jump to it instead)
void mips_indirect_branch_arm(u32 address);
void mips_indirect_branch_thumb(u32 address);
void mips_indirect_branch_dual(u32 address);

u32 execute_read_cpsr(void);
u32 execute_read_spsr(void);
void execute_swi(u32 pc);

u32 execute_spsr_restore(u32 address);
void execute_store_cpsr(u32 new_cpsr, u32 store_mask);
void execute_store_spsr(u32 new_spsr, u32 store_mask);

u32 execute_spsr_restore_body(u32 address);
u32 execute_store_cpsr_body(u32 _cpsr, u32 store_mask, u32 address);

u32 execute_lsl_flags_reg(u32 value, u32 shift);
u32 execute_lsr_flags_reg(u32 value, u32 shift);
u32 execute_asr_flags_reg(u32 value, u32 shift);
u32 execute_ror_flags_reg(u32 value, u32 shift);

void execute_aligned_store32(u32 address, u32 value);
u32 execute_aligned_load32(u32 address);

// void reg_check();

typedef enum
{
  mips_reg_zero, // 0
  mips_reg_at,   // 1
  mips_reg_v0,   // 2
  mips_reg_v1,   // 3
  mips_reg_a0,   // 4
  mips_reg_a1,   // 5
  mips_reg_a2,   // 6
  mips_reg_a3,   // 7
  mips_reg_t0,   // 8
  mips_reg_t1,   // 9
  mips_reg_t2,   // 10
  mips_reg_t3,   // 11
  mips_reg_t4,   // 12
  mips_reg_t5,   // 13
  mips_reg_t6,   // 14
  mips_reg_t7,   // 15
  mips_reg_s0,   // 16
  mips_reg_s1,   // 17
  mips_reg_s2,   // 18
  mips_reg_s3,   // 19
  mips_reg_s4,   // 20
  mips_reg_s5,   // 21
  mips_reg_s6,   // 22
  mips_reg_s7,   // 23
  mips_reg_t8,   // 24
  mips_reg_t9,   // 25
  mips_reg_k0,   // 26
  mips_reg_k1,   // 27
  mips_reg_gp,   // 28
  mips_reg_sp,   // 29
  mips_reg_fp,   // 30
  mips_reg_ra    // 31
} MIPS_REG_NUMBER;

typedef enum
{
  mips_special_sll       = 0x00,
  mips_special_srl       = 0x02,
  mips_special_sra       = 0x03,
  mips_special_sllv      = 0x04,
  mips_special_srlv      = 0x06,
  mips_special_srav      = 0x07,
  mips_special_jr        = 0x08,
  mips_special_jalr      = 0x09,
  mips_special_movz      = 0x0A,
  mips_special_movn      = 0x0B,
  mips_special_mfhi      = 0x10,
  mips_special_mthi      = 0x11,
  mips_special_mflo      = 0x12,
  mips_special_mtlo      = 0x13,
  mips_special_mult      = 0x18,
  mips_special_multu     = 0x19,
  mips_special_div       = 0x1A,
  mips_special_divu      = 0x1B,
  mips_special_madd      = 0x1C,
  mips_special_maddu     = 0x1D,
  mips_special_add       = 0x20,
  mips_special_addu      = 0x21,
  mips_special_sub       = 0x22,
  mips_special_subu      = 0x23,
  mips_special_and       = 0x24,
  mips_special_or        = 0x25,
  mips_special_xor       = 0x26,
  mips_special_nor       = 0x27,
  mips_special_slt       = 0x2A,
  mips_special_sltu      = 0x2B
} MIPS_FUNCTION_SPECIAL;

typedef enum
{
  mips_special3_ext      = 0x00,
  mips_special3_ins      = 0x04,
  mips_special3_bshfl    = 0x20
} MIPS_FUNCTION_SPECIAL3;

typedef enum
{
  mips_regimm_bltz       = 0x00,
  mips_regimm_bltzal     = 0x10
} MIPS_FUNCTION_REGIMM;

typedef enum
{
  mips_opcode_special    = 0x00,
  mips_opcode_regimm     = 0x01,
  mips_opcode_j          = 0x02,
  mips_opcode_jal        = 0x03,
  mips_opcode_beq        = 0x04,
  mips_opcode_bne        = 0x05,
  mips_opcode_blez       = 0x06,
  mips_opcode_bgtz       = 0x07,
  mips_opcode_addi       = 0x08,
  mips_opcode_addiu      = 0x09,
  mips_opcode_slti       = 0x0A,
  mips_opcode_sltiu      = 0x0B,
  mips_opcode_andi       = 0x0C,
  mips_opcode_ori        = 0x0D,
  mips_opcode_xori       = 0x0E,
  mips_opcode_lui        = 0x0F,
  mips_opcode_llo        = 0x18,
  mips_opcode_lhi        = 0x19,
  mips_opcode_trap       = 0x1A,
  mips_opcode_special2   = 0x1C,
  mips_opcode_special3   = 0x1F,
  mips_opcode_lb         = 0x20,
  mips_opcode_lh         = 0x21,
  mips_opcode_lw         = 0x23,
  mips_opcode_lbu        = 0x24,
  mips_opcode_lhu        = 0x25,
  mips_opcode_sb         = 0x28,
  mips_opcode_sh         = 0x29,
  mips_opcode_sw         = 0x2B,
} MIPS_OPCODE;

#define mips_emit_reg(opcode, rs, rt, rd, shift, function)                    \
  *((u32 *)translation_ptr) = (mips_opcode_##opcode << 26) |                  \
  (rs << 21) | (rt << 16) | (rd << 11) | (shift << 6) | function;             \
  translation_ptr += 4                                                        \

#define mips_emit_special(function, rs, rt, rd, shift)                        \
  *((u32 *)translation_ptr) = (mips_opcode_special << 26) |                   \
   (rs << 21) | (rt << 16) | (rd << 11) | (shift << 6) |                      \
   mips_special_##function;                                                   \
  translation_ptr += 4                                                        \

#define mips_emit_special3(function, rs, rt, imm_a, imm_b)                    \
  *((u32 *)translation_ptr) = (mips_opcode_special3 << 26) |                  \
   (rs << 21) | (rt << 16) | (imm_a << 11) | (imm_b << 6) |                   \
   mips_special3_##function;                                                  \
  translation_ptr += 4                                                        \

#define mips_emit_imm(opcode, rs, rt, immediate)                              \
  *((u32 *)translation_ptr) = (mips_opcode_##opcode << 26) |                  \
   (rs << 21) | (rt << 16) | (immediate & 0xFFFF);                            \
  translation_ptr += 4                                                        \

#define mips_emit_regimm(function, rs, immediate)                             \
  *((u32 *)translation_ptr) = (mips_opcode_regimm << 26) |                    \
   (rs << 21) | (mips_regimm_##function << 16) | (immediate & 0xFFFF);        \
  translation_ptr += 4                                                        \

#define mips_emit_jump(opcode, offset)                                        \
  *((u32 *)translation_ptr) = (mips_opcode_##opcode << 26) |                  \
   (offset & 0x3FFFFFF);                                                      \
  translation_ptr += 4                                                        \

#define mips_relative_offset(source, offset)                                  \
  (((u32)offset - ((u32)source + 4)) >> 2)                                    \

#define mips_absolute_offset(offset)                                          \
  ((u32)offset >> 2)                                                          \

// ADDU rd, rs, rt
#define mips_emit_addu(rd, rs, rt)                                            \
  mips_emit_special(addu, rs, rt, rd, 0)                                      \

#define mips_emit_subu(rd, rs, rt)                                            \
  mips_emit_special(subu, rs, rt, rd, 0)                                      \

#define mips_emit_xor(rd, rs, rt)                                             \
  mips_emit_special(xor, rs, rt, rd, 0)                                       \

#define mips_emit_add(rd, rs, rt)                                             \
  mips_emit_special(add, rs, rt, rd, 0)                                       \

#define mips_emit_sub(rd, rs, rt)                                             \
  mips_emit_special(sub, rs, rt, rd, 0)                                       \

#define mips_emit_and(rd, rs, rt)                                             \
  mips_emit_special(and, rs, rt, rd, 0)                                       \

#define mips_emit_or(rd, rs, rt)                                              \
  mips_emit_special(or, rs, rt, rd, 0)                                        \

#define mips_emit_nor(rd, rs, rt)                                             \
  mips_emit_special(nor, rs, rt, rd, 0)                                       \

#define mips_emit_slt(rd, rs, rt)                                             \
  mips_emit_special(slt, rs, rt, rd, 0)                                       \

#define mips_emit_sltu(rd, rs, rt)                                            \
  mips_emit_special(sltu, rs, rt, rd, 0)                                      \

#define mips_emit_sllv(rd, rt, rs)                                            \
  mips_emit_special(sllv, rs, rt, rd, 0)                                      \

#define mips_emit_srlv(rd, rt, rs)                                            \
  mips_emit_special(srlv, rs, rt, rd, 0)                                      \

#define mips_emit_srav(rd, rt, rs)                                            \
  mips_emit_special(srav, rs, rt, rd, 0)                                      \

#define mips_emit_rotrv(rd, rt, rs)                                           \
  mips_emit_special(srlv, rs, rt, rd, 1)                                      \

#define mips_emit_sll(rd, rt, shift)                                          \
  mips_emit_special(sll, 0, rt, rd, shift)                                    \

#define mips_emit_srl(rd, rt, shift)                                          \
  mips_emit_special(srl, 0, rt, rd, shift)                                    \

#define mips_emit_sra(rd, rt, shift)                                          \
  mips_emit_special(sra, 0, rt, rd, shift)                                    \

#define mips_emit_rotr(rd, rt, shift)                                         \
  mips_emit_special(srl, 1, rt, rd, shift)                                    \

#define mips_emit_mfhi(rd)                                                    \
  mips_emit_special(mfhi, 0, 0, rd, 0)                                        \

#define mips_emit_mflo(rd)                                                    \
  mips_emit_special(mflo, 0, 0, rd, 0)                                        \

#define mips_emit_mthi(rs)                                                    \
  mips_emit_special(mthi, rs, 0, 0, 0)                                        \

#define mips_emit_mtlo(rs)                                                    \
  mips_emit_special(mtlo, rs, 0, 0, 0)                                        \

#define mips_emit_mult(rs, rt)                                                \
  mips_emit_special(mult, rs, rt, 0, 0)                                       \

#define mips_emit_multu(rs, rt)                                               \
  mips_emit_special(multu, rs, rt, 0, 0)                                      \

#define mips_emit_div(rs, rt)                                                 \
  mips_emit_special(div, rs, rt, 0, 0)                                        \

#define mips_emit_divu(rs, rt)                                                \
  mips_emit_special(divu, rs, rt, 0, 0)                                       \

#define mips_emit_madd(rs, rt)                                                \
  mips_emit_special(madd, rs, rt, 0, 0)                                       \

#define mips_emit_maddu(rs, rt)                                               \
  mips_emit_special(maddu, rs, rt, 0, 0)                                      \

#define mips_emit_movn(rd, rs, rt)                                            \
  mips_emit_special(movn, rs, rt, rd, 0)                                      \

#define mips_emit_movz(rd, rs, rt)                                            \
  mips_emit_special(movz, rs, rt, rd, 0)                                      \

#define mips_emit_lb(rt, rs, offset)                                          \
  mips_emit_imm(lb, rs, rt, offset)                                           \

#define mips_emit_lbu(rt, rs, offset)                                         \
  mips_emit_imm(lbu, rs, rt, offset)                                          \

#define mips_emit_lh(rt, rs, offset)                                          \
  mips_emit_imm(lh, rs, rt, offset)                                           \

#define mips_emit_lhu(rt, rs, offset)                                         \
  mips_emit_imm(lhu, rs, rt, offset)                                          \

#define mips_emit_lw(rt, rs, offset)                                          \
  mips_emit_imm(lw, rs, rt, offset)                                           \

#define mips_emit_sb(rt, rs, offset)                                          \
  mips_emit_imm(sb, rs, rt, offset)                                           \

#define mips_emit_sh(rt, rs, offset)                                          \
  mips_emit_imm(sh, rs, rt, offset)                                           \

#define mips_emit_sw(rt, rs, offset)                                          \
  mips_emit_imm(sw, rs, rt, offset)                                           \

#define mips_emit_lui(rt, imm)                                                \
  mips_emit_imm(lui, 0, rt, imm)                                              \

#define mips_emit_addiu(rt, rs, imm)                                          \
  mips_emit_imm(addiu, rs, rt, imm)                                           \

#define mips_emit_xori(rt, rs, imm)                                           \
  mips_emit_imm(xori, rs, rt, imm)                                            \

#define mips_emit_ori(rt, rs, imm)                                            \
  mips_emit_imm(ori, rs, rt, imm)                                             \

#define mips_emit_andi(rt, rs, imm)                                           \
  mips_emit_imm(andi, rs, rt, imm)                                            \

#define mips_emit_slti(rt, rs, imm)                                           \
  mips_emit_imm(slti, rs, rt, imm)                                            \

#define mips_emit_sltiu(rt, rs, imm)                                          \
  mips_emit_imm(sltiu, rs, rt, imm)                                           \

#define mips_emit_ext(rt, rs, pos, size)                                      \
  mips_emit_special3(ext, rs, rt, (size - 1), pos)                            \

#define mips_emit_ins(rt, rs, pos, size)                                      \
  mips_emit_special3(ins, rs, rt, (pos + size - 1), pos)                      \

// Breaks down if the backpatch offset is greater than 16bits, take care
// when using (should be okay if limited to conditional instructions)

#define mips_emit_b_filler(type, rs, rt, writeback_location)                  \
  (writeback_location) = translation_ptr;                                     \
  mips_emit_imm(type, rs, rt, 0)                                              \

// The backpatch code for this has to be handled differently than the above

#define mips_emit_j_filler(writeback_location)                                \
  (writeback_location) = translation_ptr;                                     \
  mips_emit_jump(j, 0)                                                        \

#define mips_emit_b(type, rs, rt, offset)                                     \
  mips_emit_imm(type, rs, rt, offset)                                         \

#define mips_emit_j(offset)                                                   \
  mips_emit_jump(j, offset)                                                   \

#define mips_emit_jal(offset)                                                 \
  mips_emit_jump(jal, offset)                                                 \

#define mips_emit_jr(rs)                                                      \
  mips_emit_special(jr, rs, 0, 0, 0)                                          \

#define mips_emit_bltzal(rs, offset)                                          \
  mips_emit_regimm(bltzal, rs, offset)                                        \

#define mips_emit_nop()                                                       \
  mips_emit_sll(reg_zero, reg_zero, 0)                                        \

#define reg_base    mips_reg_s0
#define reg_cycles  mips_reg_s1
#define reg_a0      mips_reg_a0
#define reg_a1      mips_reg_a1
#define reg_a2      mips_reg_a2
#define reg_rv      mips_reg_v0
#define reg_pc      mips_reg_s3
#define reg_temp    mips_reg_at
#define reg_zero    mips_reg_zero

#define reg_n_cache mips_reg_s4
#define reg_z_cache mips_reg_s5
#define reg_c_cache mips_reg_s6
#define reg_v_cache mips_reg_s7

#define reg_r0      mips_reg_v1
#define reg_r1      mips_reg_a3
#define reg_r2      mips_reg_t0
#define reg_r3      mips_reg_t1
#define reg_r4      mips_reg_t2
#define reg_r5      mips_reg_t3
#define reg_r6      mips_reg_t4
#define reg_r7      mips_reg_t5
#define reg_r8      mips_reg_t6
#define reg_r9      mips_reg_t7
#define reg_r10     mips_reg_s2
#define reg_r11     mips_reg_t8
#define reg_r12     mips_reg_t9
#define reg_r13     mips_reg_gp
#define reg_r14     mips_reg_fp

// Writing to r15 goes straight to a0, to be chained with other ops

u32 arm_to_mips_reg[] =
{
  reg_r0,
  reg_r1,
  reg_r2,
  reg_r3,
  reg_r4,
  reg_r5,
  reg_r6,
  reg_r7,
  reg_r8,
  reg_r9,
  reg_r10,
  reg_r11,
  reg_r12,
  reg_r13,
  reg_r14,
  reg_a0,
  reg_a1,
  reg_a2,
  reg_temp
};

#define arm_reg_a0   15
#define arm_reg_a1   16
#define arm_reg_a2   17
#define arm_reg_temp 18

#define generate_load_reg(ireg, reg_index)                                    \
  mips_emit_addu(ireg, arm_to_mips_reg[reg_index], reg_zero)                  \

#define generate_load_imm(ireg, imm)                                          \
  if(((s32)imm >= -32768) && ((s32)imm <= 32767))                             \
  {                                                                           \
    mips_emit_addiu(ireg, reg_zero, imm);                                     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    if(((u32)imm >> 16) == 0x0000)                                            \
    {                                                                         \
      mips_emit_ori(ireg, reg_zero, imm);                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      mips_emit_lui(ireg, imm >> 16);                                         \
                                                                              \
      if(((u32)imm & 0x0000FFFF) != 0x00000000)                               \
      {                                                                       \
        mips_emit_ori(ireg, ireg, imm & 0xFFFF);                              \
      }                                                                       \
    }                                                                         \
  }                                                                           \

#define generate_load_pc(ireg, new_pc)                                        \
{                                                                             \
  s32 pc_delta = new_pc - stored_pc;                                          \
  if((pc_delta >= -32768) && (pc_delta <= 32767))                             \
  {                                                                           \
    mips_emit_addiu(ireg, reg_pc, pc_delta);                                  \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_imm(ireg, new_pc);                                          \
  }                                                                           \
}                                                                             \

#define generate_store_reg(ireg, reg_index)                                   \
  mips_emit_addu(arm_to_mips_reg[reg_index], ireg, reg_zero)                  \

#define generate_shift_left(ireg, imm)                                        \
  mips_emit_sll(ireg, ireg, imm)                                              \

#define generate_shift_right(ireg, imm)                                       \
  mips_emit_srl(ireg, ireg, imm)                                              \

#define generate_shift_right_arithmetic(ireg, imm)                            \
  mips_emit_sra(ireg, ireg, imm)                                              \

#define generate_rotate_right(ireg, imm)                                      \
  mips_emit_rotr(ireg, ireg, imm)                                             \

#define generate_add(ireg_dest, ireg_src)                                     \
  mips_emit_addu(ireg_dest, ireg_dest, ireg_src)                              \

#define generate_sub(ireg_dest, ireg_src)                                     \
  mips_emit_subu(ireg_dest, ireg_dest, ireg_src)                              \

#define generate_or(ireg_dest, ireg_src)                                      \
  mips_emit_or(ireg_dest, ireg_dest, ireg_src)                                \

#define generate_xor(ireg_dest, ireg_src)                                     \
  mips_emit_xor(ireg_dest, ireg_dest, ireg_src)                               \

#define generate_alu_imm(imm_type, reg_type, ireg_dest, ireg_src, imm)        \
  if(((s32)imm >= -32768) && ((s32)imm <= 32767))                             \
  {                                                                           \
    mips_emit_##imm_type(ireg_dest, ireg_src, imm);                           \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_imm(reg_temp, imm);                                         \
    mips_emit_##reg_type(ireg_dest, ireg_src, reg_temp);                      \
  }                                                                           \

#define generate_alu_immu(imm_type, reg_type, ireg_dest, ireg_src, imm)       \
  if(((u32)imm >= 0) && ((u32)imm <= 65535))                                  \
  {                                                                           \
    mips_emit_##imm_type(ireg_dest, ireg_src, imm);                           \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_imm(reg_temp, imm);                                         \
    mips_emit_##reg_type(ireg_dest, ireg_src, reg_temp);                      \
  }                                                                           \

#define generate_add_imm(ireg, imm)                                           \
  generate_alu_imm(addiu, add, ireg, ireg, imm)                               \

#define generate_sub_imm(ireg, imm)                                           \
  generate_alu_imm(addiu, add, ireg, ireg, -imm)                              \

#define generate_xor_imm(ireg, imm)                                           \
  generate_alu_immu(xori, xor, ireg, ireg, imm)                               \

#define generate_add_reg_reg_imm(ireg_dest, ireg_src, imm)                    \
  generate_alu_imm(addiu, add, ireg_dest, ireg_src, imm)                      \

#define generate_and_imm(ireg, imm)                                           \
  generate_alu_immu(andi, and, ireg, ireg, imm)                               \

#define generate_mov(ireg_dest, ireg_src)                                     \
  mips_emit_addu(ireg_dest, ireg_src, reg_zero)                               \

#define generate_multiply_s64()                                               \
  mips_emit_mult(arm_to_mips_reg[rm], arm_to_mips_reg[rs])                    \

#define generate_multiply_u64()                                               \
  mips_emit_multu(arm_to_mips_reg[rm], arm_to_mips_reg[rs])                   \

#define generate_multiply_s64_add()                                           \
  mips_emit_madd(arm_to_mips_reg[rm], arm_to_mips_reg[rs])                    \

#define generate_multiply_u64_add()                                           \
  mips_emit_maddu(arm_to_mips_reg[rm], arm_to_mips_reg[rs])                   \

#define generate_function_call(function_location)                             \
  mips_emit_jal(mips_absolute_offset(function_location));                     \
  mips_emit_nop()                                                             \

#define generate_function_call_swap_delay(function_location)                  \
{                                                                             \
  u32 delay_instruction = ADDRESS32(translation_ptr, -4);                     \
  translation_ptr -= 4;                                                       \
  mips_emit_jal(mips_absolute_offset(function_location));                     \
  ADDRESS32(translation_ptr, 0) = delay_instruction;                          \
  translation_ptr += 4;                                                       \
}                                                                             \

#define generate_swap_delay()                                                 \
{                                                                             \
  u32 delay_instruction = ADDRESS32(translation_ptr, -8);                     \
  u32 branch_instruction = ADDRESS32(translation_ptr, -4);                    \
  branch_instruction = (branch_instruction & 0xFFFF0000) |                    \
   (((branch_instruction & 0x0000FFFF) + 1) & 0x0000FFFF);                    \
  ADDRESS32(translation_ptr, -8) = branch_instruction;                        \
  ADDRESS32(translation_ptr, -4) = delay_instruction;                         \
}                                                                             \

#define generate_cycle_update()                                               \
  if(cycle_count != 0)                                                        \
  {                                                                           \
    mips_emit_addiu(reg_cycles, reg_cycles, -cycle_count);                    \
    cycle_count = 0;                                                          \
  }                                                                           \

#define generate_cycle_update_force()                                         \
  mips_emit_addiu(reg_cycles, reg_cycles, -cycle_count);                      \
  cycle_count = 0                                                             \

#define generate_branch_patch_conditional(dest, offset)                       \
  *((u16 *)(dest)) = mips_relative_offset(dest, offset)                       \

#define generate_branch_patch_unconditional(dest, offset)                     \
  *((u32 *)(dest)) = (mips_opcode_j << 26) |                                  \
   ((mips_absolute_offset(offset)) & 0x3FFFFFF)                               \

#define generate_branch_no_cycle_update(writeback_location, new_pc)           \
  {                                                                           \
    u8 i;                                                                     \
    u8 flag = 0;                                                              \
                                                                              \
    for( i = 0; i < idle_loop_targets; i++)                                   \
    {                                                                         \
      if(pc == idle_loop_target_pc[i])                                        \
      {                                                                       \
        flag = 1;                                                             \
      }                                                                       \
    }                                                                         \
                                                                              \
    if(flag == 1)                                                             \
    {                                                                         \
      generate_load_pc(reg_a0, new_pc);                                       \
      generate_function_call_swap_delay(mips_update_gba);                     \
      mips_emit_j_filler(writeback_location);                                 \
      mips_emit_nop();                                                        \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      generate_load_pc(reg_a0, new_pc);                                       \
      mips_emit_bltzal(reg_cycles,                                            \
      mips_relative_offset(translation_ptr, update_trampoline));              \
      generate_swap_delay();                                                  \
      mips_emit_j_filler(writeback_location);                                 \
      mips_emit_nop();                                                        \
    }                                                                         \
  }                                                                           \

#define generate_branch_cycle_update(writeback_location, new_pc)              \
  generate_cycle_update();                                                    \
  generate_branch_no_cycle_update(writeback_location, new_pc)                 \

#define generate_conditional_branch(ireg_a, ireg_b, type, writeback_location) \
  generate_branch_filler_##type(ireg_a, ireg_b, writeback_location)           \

// a0 holds the destination

#define generate_indirect_branch_cycle_update(type)                           \
  mips_emit_j(mips_absolute_offset(mips_indirect_branch_##type));             \
  generate_cycle_update_force()                                               \

#define generate_indirect_branch_no_cycle_update(type)                        \
  mips_emit_j(mips_absolute_offset(mips_indirect_branch_##type));             \
  mips_emit_nop()                                                             \

#define generate_block_prologue()                                             \
  update_trampoline = translation_ptr;                                        \
  __asm__                                                                     \
  (                                                                           \
    "cache 8, 0(%0)\n"                                                        \
    "cache 8, 0(%0)" : : "r"(translation_ptr)                                 \
  );                                                                          \
                                                                              \
  mips_emit_j(mips_absolute_offset(mips_update_gba));                         \
  mips_emit_nop();                                                            \
  generate_load_imm(reg_pc, stored_pc)                                        \

#define translate_invalidate_dcache()                                         \
  sceKernelDcacheWritebackAll()                                               \

#define block_prologue_size 8

#define check_generate_n_flag                                                 \
  (flag_status & 0x08)                                                        \

#define check_generate_z_flag                                                 \
  (flag_status & 0x04)                                                        \

#define check_generate_c_flag                                                 \
  (flag_status & 0x02)                                                        \

#define check_generate_v_flag                                                 \
  (flag_status & 0x01)                                                        \

#define generate_load_reg_pc(ireg, reg_index, pc_offset)                      \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    generate_load_pc(ireg, (pc + pc_offset));                                 \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_reg(ireg, reg_index);                                       \
  }                                                                           \

#define check_load_reg_pc(arm_reg, reg_index, pc_offset)                      \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    reg_index = arm_reg;                                                      \
    generate_load_pc(arm_to_mips_reg[arm_reg], (pc + pc_offset));             \
  }                                                                           \

#define check_store_reg_pc_no_flags(reg_index)                                \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    generate_indirect_branch_arm();                                           \
  }                                                                           \

#define check_store_reg_pc_flags(reg_index)                                   \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    generate_function_call(execute_spsr_restore);                             \
    generate_indirect_branch_dual();                                          \
  }                                                                           \

#define generate_shift_imm_lsl_no_flags(arm_reg, _rm, _shift)                 \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    mips_emit_sll(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);    \
    _rm = arm_reg;                                                            \
  }                                                                           \

#define generate_shift_imm_lsr_no_flags(arm_reg, _rm, _shift)                 \
  if(_shift != 0)                                                             \
  {                                                                           \
    check_load_reg_pc(arm_reg, _rm, 8);                                       \
    mips_emit_srl(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_addu(arm_to_mips_reg[arm_reg], reg_zero, reg_zero);             \
  }                                                                           \
  _rm = arm_reg                                                               \

#define generate_shift_imm_asr_no_flags(arm_reg, _rm, _shift)                 \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    mips_emit_sra(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_sra(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 31);        \
  }                                                                           \
  _rm = arm_reg                                                               \

#define generate_shift_imm_ror_no_flags(arm_reg, _rm, _shift)                 \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    mips_emit_rotr(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_srl(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 1);         \
    mips_emit_ins(arm_to_mips_reg[arm_reg], reg_c_cache, 31, 1);              \
  }                                                                           \
  _rm = arm_reg                                                               \

#define generate_shift_imm_lsl_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    mips_emit_ext(reg_c_cache, arm_to_mips_reg[_rm], (32 - _shift), 1);       \
    mips_emit_sll(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);    \
    _rm = arm_reg;                                                            \
  }                                                                           \

#define generate_shift_imm_lsr_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    mips_emit_ext(reg_c_cache, arm_to_mips_reg[_rm], (_shift - 1), 1);        \
    mips_emit_srl(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_srl(reg_c_cache, arm_to_mips_reg[_rm], 31);                     \
    mips_emit_addu(arm_to_mips_reg[arm_reg], reg_zero, reg_zero);             \
  }                                                                           \
  _rm = arm_reg                                                               \

#define generate_shift_imm_asr_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    mips_emit_ext(reg_c_cache, arm_to_mips_reg[_rm], (_shift - 1), 1);        \
    mips_emit_sra(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_sra(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 31);        \
    mips_emit_andi(reg_c_cache, arm_to_mips_reg[arm_reg], 1);                 \
  }                                                                           \
  _rm = arm_reg                                                               \

#define generate_shift_imm_ror_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    mips_emit_ext(reg_c_cache, arm_to_mips_reg[_rm], (_shift - 1), 1);        \
    mips_emit_rotr(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_andi(reg_temp, arm_to_mips_reg[_rm], 1);                        \
    mips_emit_srl(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 1);         \
    mips_emit_ins(arm_to_mips_reg[arm_reg], reg_c_cache, 31, 1);              \
    mips_emit_addu(reg_c_cache, reg_temp, reg_zero);                          \
  }                                                                           \
  _rm = arm_reg                                                               \

#define generate_shift_reg_lsl_no_flags(_rm, _rs)                             \
  mips_emit_sltiu(reg_temp, arm_to_mips_reg[_rs], 32);                        \
  mips_emit_sllv(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs]);         \
  mips_emit_movz(reg_a0, reg_zero, reg_temp)                                  \

#define generate_shift_reg_lsr_no_flags(_rm, _rs)                             \
  mips_emit_sltiu(reg_temp, arm_to_mips_reg[_rs], 32);                        \
  mips_emit_srlv(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs]);         \
  mips_emit_movz(reg_a0, reg_zero, reg_temp)                                  \

#define generate_shift_reg_asr_no_flags(_rm, _rs)                             \
  mips_emit_sltiu(reg_temp, arm_to_mips_reg[_rs], 32);                        \
  mips_emit_b(bne, reg_temp, reg_zero, 2);                                    \
  mips_emit_srav(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs]);         \
  mips_emit_sra(reg_a0, reg_a0, 31)                                           \

#define generate_shift_reg_ror_no_flags(_rm, _rs)                             \
  mips_emit_rotrv(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs])         \

#define generate_shift_reg_lsl_flags(_rm, _rs)                                \
  generate_load_reg_pc(reg_a0, _rm, 12);                                      \
  generate_load_reg_pc(reg_a1, _rs, 8);                                       \
  generate_function_call_swap_delay(execute_lsl_flags_reg)                    \

#define generate_shift_reg_lsr_flags(_rm, _rs)                                \
  generate_load_reg_pc(reg_a0, _rm, 12);                                      \
  generate_load_reg_pc(reg_a1, _rs, 8);                                       \
  generate_function_call_swap_delay(execute_lsr_flags_reg)                    \

#define generate_shift_reg_asr_flags(_rm, _rs)                                \
  generate_load_reg_pc(reg_a0, _rm, 12);                                      \
  generate_load_reg_pc(reg_a1, _rs, 8);                                       \
  generate_function_call_swap_delay(execute_asr_flags_reg)                    \

#define generate_shift_reg_ror_flags(_rm, _rs)                                \
  mips_emit_b(beq, arm_to_mips_reg[_rs], reg_zero, 3);                        \
  mips_emit_addiu(reg_temp, arm_to_mips_reg[_rs], -1);                        \
  mips_emit_srlv(reg_temp, arm_to_mips_reg[_rm], reg_temp);                   \
  mips_emit_andi(reg_c_cache, reg_temp, 1);                                   \
  mips_emit_rotrv(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs])         \

#define generate_shift_imm(arm_reg, name, flags_op)                           \
  u32 shift = ((opcode >> 7) & 0x1F);                                         \
  generate_shift_imm_##name##_##flags_op(arm_reg, rm, shift)                  \

#define generate_shift_reg(arm_reg, name, flags_op)                           \
  u32 rs = ((opcode >> 8) & 0x0F);                                            \
  generate_shift_reg_##name##_##flags_op(rm, rs);                             \
  rm = arm_reg                                                                \

// Made functions due to the macro expansion getting too large.
// Returns a new rm if it redirects it (which will happen on most of these
// cases)

#define generate_load_rm_sh_builder(flags_op)                                 \
u32 generate_load_rm_sh_##flags_op(u32 rm)                                    \
{                                                                             \
  switch((opcode >> 4) & 0x07)                                                \
  {                                                                           \
    /* LSL imm */                                                             \
    case 0x0:                                                                 \
    {                                                                         \
      generate_shift_imm(arm_reg_a0, lsl, flags_op);                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSL reg */                                                             \
    case 0x1:                                                                 \
    {                                                                         \
      generate_shift_reg(arm_reg_a0, lsl, flags_op);                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSR imm */                                                             \
    case 0x2:                                                                 \
    {                                                                         \
      generate_shift_imm(arm_reg_a0, lsr, flags_op);                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSR reg */                                                             \
    case 0x3:                                                                 \
    {                                                                         \
      generate_shift_reg(arm_reg_a0, lsr, flags_op);                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ASR imm */                                                             \
    case 0x4:                                                                 \
    {                                                                         \
      generate_shift_imm(arm_reg_a0, asr, flags_op);                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ASR reg */                                                             \
    case 0x5:                                                                 \
    {                                                                         \
      generate_shift_reg(arm_reg_a0, asr, flags_op);                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ROR imm */                                                             \
    case 0x6:                                                                 \
    {                                                                         \
      generate_shift_imm(arm_reg_a0, ror, flags_op);                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ROR reg */                                                             \
    case 0x7:                                                                 \
    {                                                                         \
      generate_shift_reg(arm_reg_a0, ror, flags_op);                          \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  return rm;                                                                  \
}                                                                             \

#define read_memory_constant_u8(address)                                      \
  read_memory8(address)                                                       \

#define read_memory_constant_u16(address)                                     \
  read_memory16(address)                                                      \

#define read_memory_constant_u32(address)                                     \
  read_memory32(address)                                                      \

#define read_memory_constant_s8(address)                                      \
  (s8)read_memory8(address)                                                   \

#define read_memory_constant_s16(address)                                     \
  (s16)read_memory16_signed(address)                                          \

#define generate_load_memory_u8(ireg, offset)                                 \
  mips_emit_lbu(ireg, ireg, offset)                                           \

#define generate_load_memory_u16(ireg, offset)                                \
  mips_emit_lhu(ireg, ireg, offset)                                           \

#define generate_load_memory_u32(ireg, offset)                                \
  mips_emit_lw(ireg, ireg, offset)                                            \

#define generate_load_memory_s8(ireg, offset)                                 \
  mips_emit_lb(ireg, ireg, offset)                                            \

#define generate_load_memory_s16(ireg, offset)                                \
  mips_emit_lh(ireg, ireg, offset)                                            \

#define generate_load_memory(type, ireg, address)                             \
{                                                                             \
  u32 _address = (u32)(address);                                              \
  u32 _address_hi = (_address + 0x8000) >> 16;                                \
  generate_load_imm(ireg, address);                                           \
  mips_emit_lui(ireg, _address_hi >> 16);                                     \
  generate_load_memory_##type(ireg, _address - (_address_hi << 16));          \
}                                                                             \

#define generate_known_address_load_builder(type)                             \
  u32 generate_known_address_load_##type(u32 rd, u32 address)                 \
  {                                                                           \
    switch(address >> 24)                                                     \
    {                                                                         \
      /* Read from the BIOS ROM, can be converted to an immediate load.       \
         Only really possible to do this from the BIOS but should be okay     \
         to allow it everywhere */                                            \
      case 0x00:                                                              \
        u32 imm = read_memory_constant_##type(address);                       \
        generate_load_imm(arm_to_mips_reg[rd], imm);                          \
        return 1;                                                             \
                                                                              \
      /* Read from RAM, can be converted to a load */                         \
      case 0x02:                                                              \
        generate_load_memory(type, arm_to_mips_reg[rd], (u8 *)ewram +         \
         (address & 0x7FFF) + ((address & 0x38000) * 2) + 0x8000);            \
        return 1;                                                             \
                                                                              \
      case 0x03:                                                              \
        generate_load_memory(type, arm_to_mips_reg[rd], (u8 *)iwram +         \
         (address & 0x7FFF) + 0x8000);                                        \
        return 1;                                                             \
                                                                              \
      /* Read from gamepak ROM, this has to be an immediate load because      \
         it might not actually be in memory anymore when we get to it. */     \
      case 0x08:                                                              \
        u32 imm = read_memory_constant_##type(address);                       \
        generate_load_imm(arm_to_mips_reg[rd], imm);                          \
        return 1;                                                             \
                                                                              \
      default:                                                                \
        return 0;                                                             \
    }                                                                         \
  }                                                                           \

#define generate_block_extra_vars()                                           \
  u32 stored_pc = pc;                                                         \
  u8 *update_trampoline                                                       \

#define generate_block_extra_vars_arm()                                       \
  generate_block_extra_vars();                                                \
  generate_load_rm_sh_builder(flags);                                         \
  generate_load_rm_sh_builder(no_flags);                                      \
                                                                              \
/*  generate_known_address_load_builder(u8);                                  \
  generate_known_address_load_builder(u16);                                   \
  generate_known_address_load_builder(u32);                                   \
  generate_known_address_load_builder(s8);                                    \
  generate_known_address_load_builder(s16); */                                \
                                                                              \
  u32 generate_load_offset_sh(u32 rm)                                         \
  {                                                                           \
    switch((opcode >> 5) & 0x03)                                              \
    {                                                                         \
      /* LSL imm */                                                           \
      case 0x0:                                                               \
      {                                                                       \
        generate_shift_imm(arm_reg_a1, lsl, no_flags);                        \
        break;                                                                \
      }                                                                       \
                                                                              \
      /* LSR imm */                                                           \
      case 0x1:                                                               \
      {                                                                       \
        generate_shift_imm(arm_reg_a1, lsr, no_flags);                        \
        break;                                                                \
      }                                                                       \
                                                                              \
      /* ASR imm */                                                           \
      case 0x2:                                                               \
      {                                                                       \
        generate_shift_imm(arm_reg_a1, asr, no_flags);                        \
        break;                                                                \
      }                                                                       \
                                                                              \
      /* ROR imm */                                                           \
      case 0x3:                                                               \
      {                                                                       \
        generate_shift_imm(arm_reg_a1, ror, no_flags);                        \
        break;                                                                \
      }                                                                       \
    }                                                                         \
                                                                              \
    return rm;                                                                \
  }                                                                           \
                                                                              \
  void generate_indirect_branch_arm()                                         \
  {                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_indirect_branch_cycle_update(arm);                             \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      generate_indirect_branch_no_cycle_update(arm);                          \
    }                                                                         \
  }                                                                           \
                                                                              \
  void generate_indirect_branch_dual()                                        \
  {                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_indirect_branch_cycle_update(dual);                            \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      generate_indirect_branch_no_cycle_update(dual);                         \
    }                                                                         \
  }                                                                           \

#define generate_block_extra_vars_thumb()                                     \
  generate_block_extra_vars()                                                 \

// It should be okay to still generate result flags, spsr will overwrite them.
// This is pretty infrequent (returning from interrupt handlers, et al) so
// probably not worth optimizing for.

u32 execute_spsr_restore_body(u32 address)
{
  set_cpu_mode(cpu_modes[reg[REG_CPSR] & 0x1F]);

  if((io_registers[REG_IE] & io_registers[REG_IF]) &&
   io_registers[REG_IME] && ((reg[REG_CPSR] & 0x80) == 0))
  {
    reg_mode[MODE_IRQ][6] = address + 4;
    spsr[MODE_IRQ] = reg[REG_CPSR];
    reg[REG_CPSR] = (reg[REG_CPSR] & ~0xFF) | 0xD2;
    set_cpu_mode(MODE_IRQ);

    address = 0x00000018;
  }

  if(reg[REG_CPSR] & 0x20)
  {
    address |= 0x01;
  }

  return address;
}

typedef enum
{
  CONDITION_TRUE,
  CONDITION_FALSE,
  CONDITION_EQUAL,
  CONDITION_NOT_EQUAL
} CONDITION_CHECK_TYPE;


#define generate_condition_eq()                                               \
  mips_emit_b_filler(beq, reg_z_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_ne()                                               \
  mips_emit_b_filler(bne, reg_z_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_cs()                                               \
  mips_emit_b_filler(beq, reg_c_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_cc()                                               \
  mips_emit_b_filler(bne, reg_c_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_mi()                                               \
  mips_emit_b_filler(beq, reg_n_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_pl()                                               \
  mips_emit_b_filler(bne, reg_n_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_vs()                                               \
  mips_emit_b_filler(beq, reg_v_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_vc()                                               \
  mips_emit_b_filler(bne, reg_v_cache, reg_zero, backpatch_address);          \
  generate_cycle_update_force()                                               \

#define generate_condition_hi()                                               \
  mips_emit_xori(reg_temp, reg_c_cache, 1);                                   \
  mips_emit_or(reg_temp, reg_temp, reg_z_cache);                              \
  mips_emit_b_filler(bne, reg_temp, reg_zero, backpatch_address);             \
  generate_cycle_update_force()                                               \

#define generate_condition_ls()                                               \
  mips_emit_xori(reg_temp, reg_c_cache, 1);                                   \
  mips_emit_or(reg_temp, reg_temp, reg_z_cache);                              \
  mips_emit_b_filler(beq, reg_temp, reg_zero, backpatch_address);             \
  generate_cycle_update_force()                                               \

#define generate_condition_ge()                                               \
  mips_emit_b_filler(bne, reg_n_cache, reg_v_cache, backpatch_address);       \
  generate_cycle_update_force()                                               \

#define generate_condition_lt()                                               \
  mips_emit_b_filler(beq, reg_n_cache, reg_v_cache, backpatch_address);       \
  generate_cycle_update_force()                                               \

#define generate_condition_gt()                                               \
  mips_emit_xor(reg_temp, reg_n_cache, reg_v_cache);                          \
  mips_emit_or(reg_temp, reg_temp, reg_z_cache);                              \
  mips_emit_b_filler(bne, reg_temp, reg_zero, backpatch_address);             \
  generate_cycle_update_force()                                               \

#define generate_condition_le()                                               \
  mips_emit_xor(reg_temp, reg_n_cache, reg_v_cache);                          \
  mips_emit_or(reg_temp, reg_temp, reg_z_cache);                              \
  mips_emit_b_filler(beq, reg_temp, reg_zero, backpatch_address);             \
  generate_cycle_update_force()                                               \

#define generate_condition()                                                  \
  switch(condition)                                                           \
  {                                                                           \
    case 0x0:                                                                 \
      generate_condition_eq();                                                \
      break;                                                                  \
                                                                              \
    case 0x1:                                                                 \
      generate_condition_ne();                                                \
      break;                                                                  \
                                                                              \
    case 0x2:                                                                 \
      generate_condition_cs();                                                \
      break;                                                                  \
                                                                              \
    case 0x3:                                                                 \
      generate_condition_cc();                                                \
      break;                                                                  \
                                                                              \
    case 0x4:                                                                 \
      generate_condition_mi();                                                \
      break;                                                                  \
                                                                              \
    case 0x5:                                                                 \
      generate_condition_pl();                                                \
      break;                                                                  \
                                                                              \
    case 0x6:                                                                 \
      generate_condition_vs();                                                \
      break;                                                                  \
                                                                              \
    case 0x7:                                                                 \
      generate_condition_vc();                                                \
      break;                                                                  \
                                                                              \
    case 0x8:                                                                 \
      generate_condition_hi();                                                \
      break;                                                                  \
                                                                              \
    case 0x9:                                                                 \
      generate_condition_ls();                                                \
      break;                                                                  \
                                                                              \
    case 0xA:                                                                 \
      generate_condition_ge();                                                \
      break;                                                                  \
                                                                              \
    case 0xB:                                                                 \
      generate_condition_lt();                                                \
      break;                                                                  \
                                                                              \
    case 0xC:                                                                 \
      generate_condition_gt();                                                \
      break;                                                                  \
                                                                              \
    case 0xD:                                                                 \
      generate_condition_le();                                                \
      break;                                                                  \
                                                                              \
    case 0xE:                                                                 \
      break;                                                                  \
                                                                              \
    case 0xF:                                                                 \
      break;                                                                  \
  }                                                                           \

#define generate_branch()                                                     \
{                                                                             \
  if(condition == 0x0E)                                                       \
  {                                                                           \
    generate_branch_cycle_update(                                             \
     block_exits[block_exit_position].branch_source,                          \
     block_exits[block_exit_position].branch_target);                         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_branch_no_cycle_update(                                          \
     block_exits[block_exit_position].branch_source,                          \
     block_exits[block_exit_position].branch_target);                         \
  }                                                                           \
  block_exit_position++;                                                      \
}                                                                             \

#define generate_op_and_reg(_rd, _rn, _rm)                                    \
  mips_emit_and(_rd, _rn, _rm)                                                \

#define generate_op_orr_reg(_rd, _rn, _rm)                                    \
  mips_emit_or(_rd, _rn, _rm)                                                 \

#define generate_op_eor_reg(_rd, _rn, _rm)                                    \
  mips_emit_xor(_rd, _rn, _rm)                                                \

#define generate_op_bic_reg(_rd, _rn, _rm)                                    \
  mips_emit_nor(reg_temp, _rm, reg_zero);                                     \
  mips_emit_and(_rd, _rn, reg_temp)                                           \

#define generate_op_sub_reg(_rd, _rn, _rm)                                    \
  mips_emit_subu(_rd, _rn, _rm)                                               \

#define generate_op_rsb_reg(_rd, _rn, _rm)                                    \
  mips_emit_subu(_rd, _rm, _rn)                                               \

/* SBC Rd = Rn - <Oprnd> - NOT(Carry) */
#define generate_op_sbc_reg(_rd, _rn, _rm)                                    \
  mips_emit_subu(_rd, _rn, _rm);                                              \
  mips_emit_xori(reg_temp, reg_c_cache, 1);                                   \
  mips_emit_subu(_rd, _rd, reg_temp)                                          \

/* RSC Rd = <Oprnd> - Rn - NOT(Carry) */
#define generate_op_rsc_reg(_rd, _rn, _rm)                                    \
  mips_emit_subu(_rd, _rm, _rn);                                              \
  mips_emit_xori(reg_temp, reg_c_cache, 1);                                   \
  mips_emit_subu(_rd, _rd, reg_temp)                                          \

#define generate_op_add_reg(_rd, _rn, _rm)                                    \
  mips_emit_addu(_rd, _rn, _rm)                                               \

#define generate_op_adc_reg(_rd, _rn, _rm)                                    \
  mips_emit_addu(reg_temp, _rm, reg_c_cache);                                 \
  mips_emit_addu(_rd, _rn, reg_temp)                                          \

#define generate_op_mov_reg(_rd, _rn, _rm)                                    \
  mips_emit_addu(_rd, _rm, reg_zero)                                          \

#define generate_op_mvn_reg(_rd, _rn, _rm)                                    \
  mips_emit_nor(_rd, _rm, reg_zero)                                           \

#define generate_op_imm_wrapper(name, _rd, _rn)                               \
  if(imm != 0)                                                                \
  {                                                                           \
    generate_load_imm(reg_a0, imm);                                           \
    generate_op_##name##_reg(_rd, _rn, reg_a0);                               \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_op_##name##_reg(_rd, _rn, reg_zero);                             \
  }                                                                           \

#define generate_op_and_imm(_rd, _rn)                                         \
  generate_alu_immu(andi, and, _rd, _rn, imm)                                 \

#define generate_op_orr_imm(_rd, _rn)                                         \
  generate_alu_immu(ori, or, _rd, _rn, imm)                                   \

#define generate_op_eor_imm(_rd, _rn)                                         \
  generate_alu_immu(xori, xor, _rd, _rn, imm)                                 \

#define generate_op_bic_imm(_rd, _rn)                                         \
  generate_alu_immu(andi, and, _rd, _rn, (~imm))                              \

#define generate_op_sub_imm(_rd, _rn)                                         \
  generate_alu_imm(addiu, addu, _rd, _rn, (-imm))                             \

#define generate_op_rsb_imm(_rd, _rn)                                         \
  if(imm != 0)                                                                \
  {                                                                           \
    generate_load_imm(reg_temp, imm);                                         \
    mips_emit_subu(_rd, reg_temp, _rn);                                       \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_subu(_rd, reg_zero, _rn);                                       \
  }                                                                           \

#define generate_op_sbc_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(sbc, _rd, _rn)                                      \

#define generate_op_rsc_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(rsc, _rd, _rn)                                      \

#define generate_op_add_imm(_rd, _rn)                                         \
  generate_alu_imm(addiu, addu, _rd, _rn, imm)                                \

#define generate_op_adc_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(adc, _rd, _rn)                                      \

#define generate_op_mov_imm(_rd, _rn)                                         \
  generate_load_imm(_rd, imm)                                                 \

#define generate_op_mvn_imm(_rd, _rn)                                         \
  generate_load_imm(_rd, (~imm))                                              \

#define generate_op_logic_flags(_rd)                                          \
  if(check_generate_n_flag)                                                   \
  {                                                                           \
    mips_emit_srl(reg_n_cache, _rd, 31);                                      \
  }                                                                           \
  if(check_generate_z_flag)                                                   \
  {                                                                           \
    mips_emit_sltiu(reg_z_cache, _rd, 1);                                     \
  }                                                                           \

#define generate_op_sub_flags_prologue(_rn, _rm)                              \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    mips_emit_sltu(reg_c_cache, _rn, _rm);                                    \
    mips_emit_xori(reg_c_cache, reg_c_cache, 1);                              \
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    mips_emit_slt(reg_v_cache, _rn, _rm);                                     \
  }                                                                           \

#define generate_op_sub_flags_epilogue(_rd)                                   \
  generate_op_logic_flags(_rd);                                               \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    if(!check_generate_n_flag)                                                \
    {                                                                         \
      mips_emit_srl(reg_n_cache, _rd, 31);                                    \
    }                                                                         \
    mips_emit_xor(reg_v_cache, reg_v_cache, reg_n_cache);                     \
  }                                                                           \

#define generate_add_flags_prologue(_rn, _rm)                                 \
  if(check_generate_c_flag | check_generate_v_flag)                           \
  {                                                                           \
    mips_emit_addu(reg_c_cache, _rn, reg_zero);                               \
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    mips_emit_slt(reg_v_cache, _rm, reg_zero);                                \
  }                                                                           \

#define generate_add_flags_epilogue(_rd)                                      \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    mips_emit_slt(reg_a0, _rd, reg_c_cache);                                  \
    mips_emit_xor(reg_v_cache, reg_v_cache, reg_a0);                          \
  }                                                                           \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    mips_emit_sltu(reg_c_cache, _rd, reg_c_cache);                            \
  }                                                                           \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_ands_reg(_rd, _rn, _rm)                                   \
  mips_emit_and(_rd, _rn, _rm);                                               \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_orrs_reg(_rd, _rn, _rm)                                   \
  mips_emit_or(_rd, _rn, _rm);                                                \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_eors_reg(_rd, _rn, _rm)                                   \
  mips_emit_xor(_rd, _rn, _rm);                                               \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_bics_reg(_rd, _rn, _rm)                                   \
  mips_emit_nor(reg_temp, _rm, reg_zero);                                     \
  mips_emit_and(_rd, _rn, reg_temp);                                          \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_subs_reg(_rd, _rn, _rm)                                   \
  generate_op_sub_flags_prologue(_rn, _rm);                                   \
  mips_emit_subu(_rd, _rn, _rm);                                              \
  generate_op_sub_flags_epilogue(_rd)                                         \

#define generate_op_rsbs_reg(_rd, _rn, _rm)                                   \
  generate_op_sub_flags_prologue(_rm, _rn);                                   \
  mips_emit_subu(_rd, _rm, _rn);                                              \
  generate_op_sub_flags_epilogue(_rd)                                         \

/* SBCS Rd = Rn - <Oprnd> - NOT(Carry) */
#define generate_op_sbcs_reg(_rd, _rn, _rm)                                   \
  mips_emit_xori(reg_temp, reg_c_cache, 1);                                   \
  mips_emit_addu(reg_temp, _rm, reg_temp);                                    \
  generate_op_sub_flags_prologue(_rn, reg_temp);                              \
  mips_emit_subu(_rd, _rn, reg_temp);                                         \
  generate_op_sub_flags_epilogue(_rd)                                         \

/* RSCS Rd = <Oprnd> - Rn - NOT(Carry) */
#define generate_op_rscs_reg(_rd, _rn, _rm)                                   \
  mips_emit_xori(reg_temp, reg_c_cache, 1);                                   \
  mips_emit_addu(reg_temp, _rn, reg_temp);                                    \
  generate_op_sub_flags_prologue(_rm, reg_temp);                              \
  mips_emit_subu(_rd, _rm, reg_temp);                                         \
  generate_op_sub_flags_epilogue(_rd)                                         \

#define generate_op_adds_reg(_rd, _rn, _rm)                                   \
  generate_add_flags_prologue(_rn, _rm);                                      \
  mips_emit_addu(_rd, _rn, _rm);                                              \
  generate_add_flags_epilogue(_rd)                                            \

#define generate_op_adcs_reg(_rd, _rn, _rm)                                   \
  mips_emit_addu(reg_temp, _rm, reg_c_cache);                                 \
  generate_add_flags_prologue(_rn, reg_temp);                                 \
  mips_emit_addu(_rd, _rn, reg_temp);                                         \
  generate_add_flags_epilogue(_rd)                                            \

#define generate_op_movs_reg(_rd, _rn, _rm)                                   \
  mips_emit_addu(_rd, _rm, reg_zero);                                         \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_mvns_reg(_rd, _rn, _rm)                                   \
  mips_emit_nor(_rd, _rm, reg_zero);                                          \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_neg_reg(_rd, _rn, _rm)                                    \
  generate_op_subs_reg(_rd, reg_zero, _rm)                                    \

#define generate_op_muls_reg(_rd, _rn, _rm)                                   \
  mips_emit_multu(_rn, _rm);                                                  \
  mips_emit_mflo(_rd);                                                        \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_cmp_reg(_rd, _rn, _rm)                                    \
  generate_op_subs_reg(reg_temp, _rn, _rm)                                    \

#define generate_op_cmn_reg(_rd, _rn, _rm)                                    \
  generate_op_adds_reg(reg_temp, _rn, _rm)                                    \

#define generate_op_tst_reg(_rd, _rn, _rm)                                    \
  generate_op_ands_reg(reg_temp, _rn, _rm)                                    \

#define generate_op_teq_reg(_rd, _rn, _rm)                                    \
  generate_op_eors_reg(reg_temp, _rn, _rm)                                    \

#define generate_op_ands_imm(_rd, _rn)                                        \
  generate_alu_immu(andi, and, _rd, _rn, imm);                                \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_orrs_imm(_rd, _rn)                                        \
  generate_alu_immu(ori, or, _rd, _rn, imm);                                  \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_eors_imm(_rd, _rn)                                        \
  generate_alu_immu(xori, xor, _rd, _rn, imm);                                \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_bics_imm(_rd, _rn)                                        \
  generate_alu_immu(andi, and, _rd, _rn, (~imm));                             \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_subs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(subs, _rd, _rn)                                     \

#define generate_op_rsbs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(rsbs, _rd, _rn)                                     \

#define generate_op_sbcs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(sbcs, _rd, _rn)                                     \

#define generate_op_rscs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(rscs, _rd, _rn)                                     \

#define generate_op_adds_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(adds, _rd, _rn)                                     \

#define generate_op_adcs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(adcs, _rd, _rn)                                     \

#define generate_op_movs_imm(_rd, _rn)                                        \
  generate_load_imm(_rd, imm);                                                \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_mvns_imm(_rd, _rn)                                        \
  generate_load_imm(_rd, (~imm));                                             \
  generate_op_logic_flags(_rd)                                                \

#define generate_op_cmp_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(cmp, _rd, _rn)                                      \

#define generate_op_cmn_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(cmn, _rd, _rn)                                      \

#define generate_op_tst_imm(_rd, _rn)                                         \
  generate_op_ands_imm(reg_temp, _rn)                                         \

#define generate_op_teq_imm(_rd, _rn)                                         \
  generate_op_eors_imm(reg_temp, _rn)                                         \

#define arm_generate_op_load_yes()                                            \
  generate_load_reg_pc(reg_a1, rn, 8)                                         \

#define arm_generate_op_load_no()                                             \

#define arm_op_check_yes()                                                    \
  check_load_reg_pc(arm_reg_a1, rn, 8)                                        \

#define arm_op_check_no()                                                     \

#define arm_generate_op_reg_flags(name, load_op)                              \
  arm_decode_data_proc_reg();                                                 \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    rm = generate_load_rm_sh_flags(rm);                                       \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    rm = generate_load_rm_sh_no_flags(rm);                                    \
  }                                                                           \
  arm_op_check_##load_op();                                                   \
  generate_op_##name##_reg(arm_to_mips_reg[rd], arm_to_mips_reg[rn],          \
   arm_to_mips_reg[rm])                                                       \

#define arm_generate_op_reg(name, load_op)                                    \
  arm_decode_data_proc_reg();                                                 \
  rm = generate_load_rm_sh_no_flags(rm);                                      \
  arm_op_check_##load_op();                                                   \
  generate_op_##name##_reg(arm_to_mips_reg[rd], arm_to_mips_reg[rn],          \
   arm_to_mips_reg[rm])                                                       \

#define arm_generate_op_imm(name, load_op)                                    \
  arm_decode_data_proc_imm();                                                 \
  arm_op_check_##load_op();                                                   \
  generate_op_##name##_imm(arm_to_mips_reg[rd], arm_to_mips_reg[rn])          \

#define arm_generate_op_imm_flags(name, load_op)                              \
  arm_generate_op_imm(name, load_op)                                          \


/* 1S+x+y */
/* x=1I cycles if Op2 shifted-by-register. y=1S+1N cycles if Rd=R15. */
#define cycle_arm_data_proc_reg()                                             \
  if(rd == reg_pc)                                                            \
  {                                                                           \
    cycle_count += 3 + cycle_arm_base(pc);                                    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    cycle_count += 1 + cycle_arm_fetch(pc);                                   \
  }                                                                           \

#define cycle_arm_data_proc_reg_flags()                                       \
  cycle_arm_data_proc_reg()                                                   \

#define cycle_arm_data_proc_imm()                                             \
  cycle_count += cycle_arm_base(pc);                                          \
  if(rd == reg_pc)                                                            \
  {                                                                           \
    cycle_count += 2;                                                         \
  }                                                                           \

#define cycle_arm_data_proc_imm_flags()                                       \
  cycle_arm_data_proc_imm()                                                   \

/* 1S+x */
#define cycle_arm_data_proc_test_reg()                                        \
  cycle_count += 1 + cycle_arm_fetch(pc)                                      \

#define cycle_arm_data_proc_test_reg_flags()                                  \
  cycle_arm_data_proc_test_reg()                                              \

#define cycle_arm_data_proc_test_imm()                                        \
  cycle_count += cycle_arm_base(pc)                                           \

#define cycle_arm_data_proc_test_imm_flags()                                  \
  cycle_arm_data_proc_test_imm()                                              \

/* AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC, ORR, BIC  1S+x+y */
#define arm_data_proc(name, type, flags_op)                                   \
{                                                                             \
  arm_generate_op_##type(name, yes);                                          \
  cycle_arm_data_proc_##type();                                               \
  check_store_reg_pc_##flags_op(rd);                                          \
}                                                                             \

/* TST, TEQ, CMP, CMN  1S+x */
#define arm_data_proc_test(name, type)                                        \
{                                                                             \
  arm_generate_op_##type(name, yes);                                          \
  cycle_arm_data_proc_test_##type();                                          \
}                                                                             \

/* MOV, MVN  1S+x+y */
#define arm_data_proc_unary(name, type, flags_op)                             \
{                                                                             \
  arm_generate_op_##type(name, no);                                           \
  cycle_arm_data_proc_##type();                                               \
  check_store_reg_pc_##flags_op(rd);                                          \
}                                                                             \


#define arm_multiply_flags_yes(_rd)                                           \
  generate_op_logic_flags(_rd)                                                \

#define arm_multiply_flags_no(_rd)                                            \

#define arm_multiply_add_no()                                                 \
  mips_emit_mflo(arm_to_mips_reg[rd])                                         \

#define arm_multiply_add_yes()                                                \
  mips_emit_mflo(reg_temp);                                                   \
  mips_emit_addu(arm_to_mips_reg[rd], reg_temp, arm_to_mips_reg[rn])          \

/* 1S+mI */
#define cycle_arm_multiply_add_no()                                           \
  cycle_count += cycle_arm_fetch(pc) + cycle_multiply(rs)                     \

/* 1S+mI+1I */
#define cycle_arm_multiply_add_yes()                                          \
  cycle_count += 1 + cycle_arm_fetch(pc) + cycle_multiply(rs)                 \

#define arm_multiply(add_op, flags)                                           \
{                                                                             \
  arm_decode_multiply();                                                      \
  cycle_arm_multiply_add_##add_op();                                          \
  mips_emit_multu(arm_to_mips_reg[rm], arm_to_mips_reg[rs]);                  \
  arm_multiply_add_##add_op();                                                \
  arm_multiply_flags_##flags(arm_to_mips_reg[rd]);                            \
}                                                                             \

#define arm_multiply_long_flags_yes(_rdlo, _rdhi)                             \
  mips_emit_sltiu(reg_z_cache, _rdlo, 1);                                     \
  mips_emit_sltiu(reg_a0, _rdhi, 1);                                          \
  mips_emit_and(reg_z_cache, reg_z_cache, reg_a0);                            \
  mips_emit_srl(reg_n_cache, _rdhi, 31);                                      \

#define arm_multiply_long_flags_no(_rdlo, _rdhi)                              \

#define arm_multiply_long_add_yes(name)                                       \
  mips_emit_mtlo(arm_to_mips_reg[rdlo]);                                      \
  mips_emit_mthi(arm_to_mips_reg[rdhi]);                                      \
  generate_multiply_##name()                                                  \

#define arm_multiply_long_add_no(name)                                        \
  generate_multiply_##name()                                                  \

/* 1S+mI+1I */
#define cycle_arm_multiply_long_add_no()                                      \
  cycle_count += 1 + cycle_arm_fetch(pc) + cycle_multiply(rs)                 \

/* 1S+mI+2I */
#define cycle_arm_multiply_long_add_yes()                                     \
  cycle_count += 2 + cycle_arm_fetch(pc) + cycle_multiply(rs)                 \

#define arm_multiply_long(name, add_op, flags)                                \
{                                                                             \
  arm_decode_multiply_long();                                                 \
  cycle_arm_multiply_long_add_##add_op();                                     \
  arm_multiply_long_add_##add_op(name);                                       \
  mips_emit_mflo(arm_to_mips_reg[rdlo]);                                      \
  mips_emit_mfhi(arm_to_mips_reg[rdhi]);                                      \
  arm_multiply_long_flags_##flags(arm_to_mips_reg[rdlo],                      \
   arm_to_mips_reg[rdhi]);                                                    \
}                                                                             \

#define arm_psr_read(op_type, psr_reg)                                        \
  generate_function_call(execute_read_##psr_reg);                             \
  generate_store_reg(reg_rv, rd)                                              \

u32 execute_store_cpsr_body(u32 _cpsr, u32 store_mask, u32 address)
{
  reg[REG_CPSR] = _cpsr;
  if(store_mask & 0xFF)
  {
    set_cpu_mode(cpu_modes[_cpsr & 0x1F]);

    if((io_registers[REG_IE] & io_registers[REG_IF]) &&
     io_registers[REG_IME] && ((_cpsr & 0x80) == 0))
    {
      reg_mode[MODE_IRQ][6] = address + 4;
      spsr[MODE_IRQ] = _cpsr;
      reg[REG_CPSR] = (reg[REG_CPSR] & ~0xFF) | 0xD2;
      set_cpu_mode(MODE_IRQ);

      return 0x00000018;
    }
  }

  return 0;
}

#define arm_psr_load_new_reg()                                                \
  generate_load_reg(reg_a0, rm)                                               \

#define arm_psr_load_new_imm()                                                \
  generate_load_imm(reg_a0, imm)                                              \

#define arm_psr_store(op_type, psr_reg)                                       \
  arm_psr_load_new_##op_type();                                               \
  generate_load_imm(reg_a1, psr_masks[psr_field]);                            \
  generate_load_pc(reg_a2, (pc + 4));                                         \
  generate_function_call_swap_delay(execute_store_##psr_reg)                  \

#define arm_psr(op_type, transfer_type, psr_reg)                              \
{                                                                             \
  arm_decode_psr_##op_type();                                                 \
  cycle_count += cycle_arm_base(pc);                                          \
  arm_psr_##transfer_type(op_type, psr_reg);                                  \
}                                                                             \

/* LDR: 1S+1N+1I. LDR PC: 2S+2N+1I. STR: 2N. */
#define cycle_arm_access_memory_load_u32()                                    \
  cycle_count +=                                                              \
/*   1 + cycle_arm_fetch(pc) + waitstate_cycles_non_seq[1][reg_a0 >> 24]; */      \
   cycle_arm_fetch(pc) + waitstate_cycles_non_seq[1][reg_a0 >> 24];           \
  if(rd == reg_pc)                                                            \
  {                                                                           \
    cycle_count += 2;                                                         \
  }                                                                           \

#define cycle_arm_access_memory_load_u16()                                    \
  cycle_count +=                                                              \
/*   1 + cycle_arm_fetch(pc) + waitstate_cycles_non_seq[0][reg_a0 >> 24]; */      \
   cycle_arm_fetch(pc) + waitstate_cycles_non_seq[0][reg_a0 >> 24];           \
  if(rd == reg_pc)                                                            \
  {                                                                           \
    cycle_count += 2;                                                         \
  }                                                                           \

#define cycle_arm_access_memory_load_s16()                                    \
  cycle_arm_access_memory_load_u16()                                          \

#define cycle_arm_access_memory_load_u8()                                     \
  cycle_arm_access_memory_load_u16()                                          \

#define cycle_arm_access_memory_load_s8()                                     \
  cycle_arm_access_memory_load_u16()                                          \

#define cycle_arm_access_memory_store_u32()                                   \
  cycle_count +=                                                              \
   cycle_arm_fetch(pc) + waitstate_cycles_non_seq[1][reg_a0 >> 24]            \

#define cycle_arm_access_memory_store_u16()                                   \
  cycle_count +=                                                              \
   cycle_arm_fetch(pc) + waitstate_cycles_non_seq[0][reg_a0 >> 24]            \

#define cycle_arm_access_memory_store_s16()                                   \
  cycle_arm_access_memory_store_u16()                                         \

#define cycle_arm_access_memory_store_u8()                                    \
  cycle_arm_access_memory_store_u16()                                         \

#define cycle_arm_access_memory_store_s8()                                    \
  cycle_arm_access_memory_store_u16()                                         \

#define arm_access_memory_load(mem_type)                                      \
  cycle_arm_access_memory_load_##mem_type();                                  \
  mips_emit_jal(mips_absolute_offset(execute_load_##mem_type));               \
  generate_load_pc(reg_a1, (pc + 8));                                         \
  generate_store_reg(reg_rv, rd);                                             \
  check_store_reg_pc_no_flags(rd)                                             \

#define arm_access_memory_store(mem_type)                                     \
  cycle_arm_access_memory_store_##mem_type();                                 \
  generate_load_pc(reg_a2, (pc + 4));                                         \
  generate_load_reg_pc(reg_a1, rd, 12);                                       \
  generate_function_call_swap_delay(execute_store_##mem_type)                 \

#define arm_access_memory_reg_pre_up()                                        \
  mips_emit_addu(reg_a0, arm_to_mips_reg[rn], arm_to_mips_reg[rm])            \

#define arm_access_memory_reg_pre_down()                                      \
  mips_emit_subu(reg_a0, arm_to_mips_reg[rn], arm_to_mips_reg[rm])            \

#define arm_access_memory_reg_pre(adjust_dir)                                 \
  check_load_reg_pc(arm_reg_a0, rn, 8);                                       \
  arm_access_memory_reg_pre_##adjust_dir()                                    \

#define arm_access_memory_reg_pre_wb(adjust_dir)                              \
  arm_access_memory_reg_pre(adjust_dir);                                      \
  generate_store_reg(reg_a0, rn)                                              \

#define arm_access_memory_reg_post_up()                                       \
  mips_emit_addu(arm_to_mips_reg[rn], arm_to_mips_reg[rn],                    \
   arm_to_mips_reg[rm])                                                       \

#define arm_access_memory_reg_post_down()                                     \
  mips_emit_subu(arm_to_mips_reg[rn], arm_to_mips_reg[rn],                    \
   arm_to_mips_reg[rm])                                                       \

#define arm_access_memory_reg_post(adjust_dir)                                \
  generate_load_reg(reg_a0, rn);                                              \
  arm_access_memory_reg_post_##adjust_dir()                                   \

#define arm_access_memory_imm_pre_up()                                        \
  mips_emit_addiu(reg_a0, arm_to_mips_reg[rn], offset)                        \

#define arm_access_memory_imm_pre_down()                                      \
  mips_emit_addiu(reg_a0, arm_to_mips_reg[rn], -offset)                       \

#define arm_access_memory_imm_pre(adjust_dir)                                 \
  check_load_reg_pc(arm_reg_a0, rn, 8);                                       \
  arm_access_memory_imm_pre_##adjust_dir()                                    \

#define arm_access_memory_imm_pre_wb(adjust_dir)                              \
  arm_access_memory_imm_pre(adjust_dir);                                      \
  generate_store_reg(reg_a0, rn)                                              \

#define arm_access_memory_imm_post_up()                                       \
  mips_emit_addiu(arm_to_mips_reg[rn], arm_to_mips_reg[rn], offset)           \

#define arm_access_memory_imm_post_down()                                     \
  mips_emit_addiu(arm_to_mips_reg[rn], arm_to_mips_reg[rn], -offset)          \

#define arm_access_memory_imm_post(adjust_dir)                                \
  generate_load_reg(reg_a0, rn);                                              \
  arm_access_memory_imm_post_##adjust_dir()                                   \

#define arm_data_trans_reg(adjust_op, adjust_dir)                             \
  arm_decode_data_trans_reg();                                                \
  rm = generate_load_offset_sh(rm);                                           \
  arm_access_memory_reg_##adjust_op(adjust_dir)                               \

#define arm_data_trans_imm(adjust_op, adjust_dir)                             \
  arm_decode_data_trans_imm();                                                \
  arm_access_memory_imm_##adjust_op(adjust_dir)                               \

#define arm_data_trans_half_reg(adjust_op, adjust_dir)                        \
  arm_decode_half_trans_r();                                                  \
  arm_access_memory_reg_##adjust_op(adjust_dir)                               \

#define arm_data_trans_half_imm(adjust_op, adjust_dir)                        \
  arm_decode_half_trans_of();                                                 \
  arm_access_memory_imm_##adjust_op(adjust_dir)                               \

#define arm_access_memory(access_type, direction, adjust_op, mem_type,        \
 offset_type)                                                                 \
{                                                                             \
  arm_data_trans_##offset_type(adjust_op, direction);                         \
  arm_access_memory_##access_type(mem_type);                                  \
}                                                                             \

#define word_bit_count(word)                                                  \
  (bit_count[word >> 8] + bit_count[word & 0xFF])                             \

#define sprint_no(access_type, pre_op, post_op, wb)                           \

#define sprint_yes(access_type, pre_op, post_op, wb)                          \
  printf("sbit on %s %s %s %s\n", #access_type, #pre_op, #post_op, #wb)       \

/* LDM, nS+1N+1I. LDM PC, (n+1)S+2N+1I. STM (n-1)S+2N. */
#define arm_block_memory_load()                                               \
  block_memory_waitstate(reg_a0 >> 24);                                       \
  generate_function_call_swap_delay(execute_aligned_load32);                  \
  generate_store_reg(reg_rv, i)                                               \

#define arm_block_memory_store()                                              \
  block_memory_waitstate(reg_a0 >> 24);                                       \
  generate_load_reg_pc(reg_a1, i, 8);                                         \
  generate_function_call_swap_delay(execute_aligned_store32)                  \

#define arm_block_memory_final_load()                                         \
  arm_block_memory_load()                                                     \

#define arm_block_memory_final_store()                                        \
  generate_load_pc(reg_a2, (pc + 4));                                         \
  mips_emit_jal(mips_absolute_offset(execute_store_u32));                     \
  generate_load_reg(reg_a1, i)                                                \

#define arm_block_memory_adjust_pc_store()                                    \

#define arm_block_memory_adjust_pc_load()                                     \
  cycle_count++;                                                              \
  if(reg_list & 0x8000)                                                       \
  {                                                                           \
    cycle_count += 2;                                                         \
    generate_mov(reg_a0, reg_rv);                                             \
    generate_indirect_branch_arm();                                           \
  }                                                                           \

#define arm_block_memory_sp_load()                                            \
  mips_emit_lw(arm_to_mips_reg[i], reg_a1, offset);                           \

#define arm_block_memory_sp_store()                                           \
{                                                                             \
  u32 store_reg = i;                                                          \
  check_load_reg_pc(arm_reg_a0, store_reg, 8);                                \
  mips_emit_sw(arm_to_mips_reg[store_reg], reg_a1, offset);                   \
}                                                                             \

#define arm_block_memory_sp_adjust_pc_store()                                 \

#define arm_block_memory_sp_adjust_pc_load()                                  \
  cycle_count++;                                                              \
  if(reg_list & 0x8000)                                                       \
  {                                                                           \
    cycle_count += 2;                                                         \
    generate_indirect_branch_arm();                                           \
  }                                                                           \

/* ポストデクリメント アドレッシング */
#define arm_block_memory_offset_down_a()                                      \
  mips_emit_addiu(reg_a2, base_reg, (-((word_bit_count(reg_list) - 1) << 2))) \

/* プレデクリメント アドレッシング */
#define arm_block_memory_offset_down_b()                                      \
  mips_emit_addiu(reg_a2, base_reg, (-(word_bit_count(reg_list) << 2)))       \

/* ポストインクリメント アドレッシング */
#define arm_block_memory_offset_no()                                          \
  mips_emit_addu(reg_a2, base_reg, reg_zero)                                  \

/* プレインクリメント アドレッシング */
#define arm_block_memory_offset_up()                                          \
  mips_emit_addiu(reg_a2, base_reg, 4)                                        \

#define arm_block_memory_writeback_down()                                     \
  mips_emit_addiu(base_reg, base_reg, (-(word_bit_count(reg_list) << 2)))     \

#define arm_block_memory_writeback_up()                                       \
  mips_emit_addiu(base_reg, base_reg, (word_bit_count(reg_list) << 2))        \

#define arm_block_memory_writeback_no()

// Only emit writeback if the register is not in the list

#define arm_block_memory_writeback_load(writeback_type)                       \
  if(!((reg_list >> rn) & 0x01))                                              \
  {                                                                           \
    arm_block_memory_writeback_##writeback_type();                            \
  }                                                                           \

#define arm_block_memory_writeback_store(writeback_type)                      \
  arm_block_memory_writeback_##writeback_type()                               \

#define arm_block_memory(access_type, offset_type, writeback_type, s_bit)     \
{                                                                             \
  arm_decode_block_trans();                                                   \
  u32 i;                                                                      \
  u32 offset = 0;                                                             \
  u32 base_reg = arm_to_mips_reg[rn];                                         \
                                                                              \
  cycle_count += cycle_arm_fetch(pc);                                         \
                                                                              \
  arm_block_memory_offset_##offset_type();                                    \
  arm_block_memory_writeback_##access_type(writeback_type);                   \
                                                                              \
  if((rn == REG_SP) && iwram_stack_optimize)                                  \
  {                                                                           \
    mips_emit_andi(reg_a1, reg_a2, 0x7FFC);                                   \
    generate_load_imm(reg_a0, ((u32)(iwram + 0x8000)));                       \
    mips_emit_addu(reg_a1, reg_a1, reg_a0);                                   \
                                                                              \
    for(i = 0; i < 16; i++)                                                   \
    {                                                                         \
      if((reg_list >> i) & 0x01)                                              \
      {                                                                       \
        cycle_count++;                                                        \
        arm_block_memory_sp_##access_type();                                  \
        offset += 4;                                                          \
      }                                                                       \
    }                                                                         \
    arm_block_memory_sp_adjust_pc_##access_type();                            \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_ins(reg_a2, reg_zero, 0, 2);                                    \
                                                                              \
    for(i = 0; i < 16; i++)                                                   \
    {                                                                         \
      if((reg_list >> i) & 0x01)                                              \
      {                                                                       \
        mips_emit_addiu(reg_a0, reg_a2, offset);                              \
                                                                              \
        if(reg_list & ~((2 << i) - 1))                                        \
        {                                                                     \
          arm_block_memory_##access_type();                                   \
          offset += 4;                                                        \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          arm_block_memory_final_##access_type();                             \
          break;                                                              \
        }                                                                     \
      }                                                                       \
    }                                                                         \
    arm_block_memory_adjust_pc_##access_type();                               \
  }                                                                           \
}                                                                             \


// This isn't really a correct implementation, may have to fix later.

/* 1S+2N+1I */
#define arm_swap(type)                                                        \
{                                                                             \
  arm_decode_swap();                                                          \
/*  cycle_count += 3 + cycle_arm_fetch(pc); */                                    \
  cycle_count +=                                                              \
   2 + cycle_arm_fetch(pc) + waitstate_cycles_non_seq[1][reg_a0 >> 24];       \
  mips_emit_jal(mips_absolute_offset(execute_load_##type));                   \
  generate_load_reg(reg_a0, rn);                                              \
  generate_mov(reg_a2, reg_rv);                                               \
  generate_load_reg(reg_a0, rn);                                              \
  cycle_count += waitstate_cycles_non_seq[1][reg_a0 >> 24];                   \
  mips_emit_jal(mips_absolute_offset(execute_store_##type));                  \
  generate_load_reg(reg_a1, rm);                                              \
  generate_store_reg(reg_a2, rd);                                             \
}                                                                             \


#define thumb_generate_op_load_yes(_rs)                                       \
  generate_load_reg(reg_a1, _rs)                                              \

#define thumb_generate_op_load_no(_rs)                                        \

#define thumb_generate_op_reg(name, _rd, _rs, _rn)                            \
  generate_op_##name##_reg(arm_to_mips_reg[_rd],                              \
   arm_to_mips_reg[_rs], arm_to_mips_reg[_rn])                                \

#define thumb_generate_op_imm(name, _rd, _rs, _rn)                            \
  generate_op_##name##_imm(arm_to_mips_reg[_rd], arm_to_mips_reg[_rs])        \

// Types: add_sub, add_sub_imm, alu_op, imm
// Affects N/Z/C/V flags

#define thumb_data_proc(type, name, rn_type, _rd, _rs, _rn)                   \
{                                                                             \
  thumb_decode_##type();                                                      \
  cycle_count += cycle_thumb_base(pc);                                        \
  thumb_generate_op_##rn_type(name, _rd, _rs, _rn);                           \
}                                                                             \

/* 1S+mI */
#define thumb_data_proc_muls(type, rn_type, _rd, _rs, _rn)                    \
{                                                                             \
  thumb_decode_##type();                                                      \
  cycle_count += cycle_thumb_fetch(pc) + cycle_multiply(_rs);                 \
  thumb_generate_op_##rn_type(muls, _rd, _rs, _rn);                           \
}                                                                             \

#define thumb_data_proc_test(type, name, rn_type, _rs, _rn)                   \
{                                                                             \
  thumb_decode_##type();                                                      \
  cycle_count += cycle_thumb_base(pc);                                        \
  thumb_generate_op_##rn_type(name, 0, _rs, _rn);                             \
}                                                                             \

#define thumb_data_proc_unary(type, name, rn_type, _rd, _rn)                  \
{                                                                             \
  thumb_decode_##type();                                                      \
  cycle_count += cycle_thumb_base(pc);                                        \
  thumb_generate_op_##rn_type(name, _rd, 0, _rn);                             \
}                                                                             \

#define check_store_reg_pc_thumb(_rd)                                         \
  if(_rd == REG_PC)                                                           \
  {                                                                           \
    generate_indirect_branch_cycle_update(thumb);                             \
  }                                                                           \

#define thumb_data_proc_hi(name)                                              \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  cycle_count += cycle_thumb_base(pc);                                        \
  if(rd == reg_pc)                                                            \
  {                                                                           \
    cycle_count += 2;                                                         \
  }                                                                           \
  u32 dest_rd = rd;                                                           \
  check_load_reg_pc(arm_reg_a0, rs, 4);                                       \
  check_load_reg_pc(arm_reg_a1, rd, 4);                                       \
  generate_op_##name##_reg(arm_to_mips_reg[dest_rd], arm_to_mips_reg[rd],     \
   arm_to_mips_reg[rs]);                                                      \
  check_store_reg_pc_thumb(dest_rd);                                          \
}                                                                             \

/*

#define thumb_data_proc_hi(name)                                              \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  check_load_reg_pc(arm_reg_a0, rs, 4);                                       \
  check_load_reg_pc(arm_reg_a1, rd, 4);                                       \
  generate_op_##name##_reg(arm_to_mips_reg[rd], arm_to_mips_reg[rd],          \
   arm_to_mips_reg[rs]);                                                      \
  check_store_reg_pc_thumb(rd);                                               \
}                                                                             \

*/

#define thumb_data_proc_test_hi(name)                                         \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  cycle_count += cycle_thumb_base(pc);                                        \
  check_load_reg_pc(arm_reg_a0, rs, 4);                                       \
  check_load_reg_pc(arm_reg_a1, rd, 4);                                       \
  generate_op_##name##_reg(reg_temp, arm_to_mips_reg[rd],                     \
   arm_to_mips_reg[rs]);                                                      \
}                                                                             \

#define thumb_data_proc_mov_hi()                                              \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  cycle_count += cycle_thumb_base(pc);                                        \
  if(rd == reg_pc)                                                            \
  {                                                                           \
    cycle_count += 2;                                                         \
  }                                                                           \
  check_load_reg_pc(arm_reg_a0, rs, 4);                                       \
  mips_emit_addu(arm_to_mips_reg[rd], arm_to_mips_reg[rs], reg_zero);         \
  check_store_reg_pc_thumb(rd);                                               \
}                                                                             \

#define thumb_load_pc(_rd)                                                    \
{                                                                             \
  thumb_decode_imm();                                                         \
  cycle_count += cycle_thumb_base(pc);                                        \
  generate_load_pc(arm_to_mips_reg[_rd], (((pc & ~2) + 4) + (imm << 2)));     \
}                                                                             \

#define thumb_load_sp(_rd)                                                    \
{                                                                             \
  thumb_decode_imm();                                                         \
  cycle_count += cycle_thumb_base(pc);                                        \
  mips_emit_addiu(arm_to_mips_reg[_rd], reg_r13, (imm << 2));                 \
}                                                                             \

#define thumb_adjust_sp(value)                                                \
{                                                                             \
  thumb_decode_add_sp();                                                      \
  cycle_count += cycle_thumb_base(pc);                                        \
  mips_emit_addiu(reg_r13, reg_r13, (value));                                 \
}                                                                             \

// Decode types: shift, alu_op
// Operation types: lsl, lsr, asr, ror
// Affects N/Z/C flags

#define thumb_generate_shift_imm(name)                                        \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    generate_shift_imm_##name##_flags(rd, rs, imm);                           \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_shift_imm_##name##_no_flags(rd, rs, imm);                        \
  }                                                                           \
  if(rs != rd)                                                                \
  {                                                                           \
    mips_emit_addu(arm_to_mips_reg[rd], arm_to_mips_reg[rs], reg_zero);       \
  }                                                                           \

#define thumb_generate_shift_reg(name)                                        \
{                                                                             \
  u32 original_rd = rd;                                                       \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    generate_shift_reg_##name##_flags(rd, rs);                                \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_shift_reg_##name##_no_flags(rd, rs);                             \
  }                                                                           \
  mips_emit_addu(arm_to_mips_reg[original_rd], reg_a0, reg_zero);             \
}                                                                             \

#define cycle_thumb_shift_shift()                                             \
  cycle_count += cycle_thumb_base(pc)                                         \

/* ALU operations LSL,LSR,ASR,ROR  1S+1I */
#define cycle_thumb_shift_alu_op()                                            \
  cycle_count += 1 + cycle_thumb_fetch(pc)                                    \

#define thumb_shift(decode_type, op_type, value_type)                         \
{                                                                             \
  cycle_thumb_shift_##decode_type();                                          \
  thumb_decode_##decode_type();                                               \
  thumb_generate_shift_##value_type(op_type);                                 \
  generate_op_logic_flags(arm_to_mips_reg[rd]);                               \
}                                                                             \

// Operation types: imm, mem_reg, mem_imm

/* LDR 1S+1N+1I, STR 2N */
#define cycle_thumb_access_memory_load_u32()                                  \
  cycle_count +=                                                              \
/*   1 + cycle_thumb_fetch(pc) + waitstate_cycles_non_seq[1][reg_a0 >> 24] */     \
   cycle_thumb_fetch(pc) + waitstate_cycles_non_seq[1][reg_a0 >> 24]          \

#define cycle_thumb_access_memory_load_u16()                                  \
  cycle_count +=                                                              \
/*   1 + cycle_thumb_fetch(pc) + waitstate_cycles_non_seq[0][reg_a0 >> 24] */     \
   cycle_thumb_fetch(pc) + waitstate_cycles_non_seq[0][reg_a0 >> 24]          \

#define cycle_thumb_access_memory_load_s16()                                  \
  cycle_thumb_access_memory_load_u16()                                        \

#define cycle_thumb_access_memory_load_u8()                                   \
  cycle_thumb_access_memory_load_u16()                                        \

#define cycle_thumb_access_memory_load_s8()                                   \
  cycle_thumb_access_memory_load_u16()                                        \

#define cycle_thumb_access_memory_store_u32()                                 \
  cycle_count +=                                                              \
   cycle_thumb_fetch(pc) + waitstate_cycles_non_seq[1][reg_a0 >> 24]          \

#define cycle_thumb_access_memory_store_u16()                                 \
  cycle_count +=                                                              \
   cycle_thumb_fetch(pc) + waitstate_cycles_non_seq[0][reg_a0 >> 24]          \

#define cycle_thumb_access_memory_store_s16()                                 \
  cycle_thumb_access_memory_store_u16()                                       \

#define cycle_thumb_access_memory_store_u8()                                  \
  cycle_thumb_access_memory_store_u16()                                       \

#define cycle_thumb_access_memory_store_s8()                                  \
  cycle_thumb_access_memory_store_u16()                                       \

#define thumb_access_memory_load(mem_type, reg_rd)                            \
  cycle_thumb_access_memory_load_##mem_type();                                \
  mips_emit_jal(mips_absolute_offset(execute_load_##mem_type));               \
  generate_load_pc(reg_a1, (pc + 4));                                         \
  generate_store_reg(reg_rv, reg_rd)                                          \

#define thumb_access_memory_store(mem_type, reg_rd)                           \
  cycle_thumb_access_memory_store_##mem_type();                               \
  generate_load_pc(reg_a2, (pc + 2));                                         \
  mips_emit_jal(mips_absolute_offset(execute_store_##mem_type));              \
  generate_load_reg(reg_a1, reg_rd)                                           \

#define thumb_access_memory_generate_address_pc_relative(offset, reg_rb,      \
 reg_ro)                                                                      \
  generate_load_pc(reg_a0, (offset))                                          \

#define thumb_access_memory_generate_address_reg_imm(offset, reg_rb, reg_ro)  \
  mips_emit_addiu(reg_a0, arm_to_mips_reg[reg_rb], (offset))                  \

#define thumb_access_memory_generate_address_reg_imm_sp(offset, reg_rb, reg_ro)\
  mips_emit_addiu(reg_a0, arm_to_mips_reg[reg_rb], (offset << 2))             \

#define thumb_access_memory_generate_address_reg_reg(offset, reg_rb, reg_ro)  \
  mips_emit_addu(reg_a0, arm_to_mips_reg[reg_rb], arm_to_mips_reg[reg_ro])    \

#define thumb_access_memory(access_type, op_type, reg_rd, reg_rb, reg_ro,     \
 address_type, offset, mem_type)                                              \
{                                                                             \
  thumb_decode_##op_type();                                                   \
  thumb_access_memory_generate_address_##address_type(offset, reg_rb, reg_ro);\
  thumb_access_memory_##access_type(mem_type, reg_rd);                        \
}                                                                             \


/* ポストインクリメント アドレッシング STMIA, LDMIA, POP */
#define thumb_block_address_preadjust_no(base_reg)                            \
  mips_emit_addu(reg_a2, arm_to_mips_reg[base_reg], reg_zero)                 \

/* プレデクリメント アドレッシング PUSH rlist */
#define thumb_block_address_preadjust_down(base_reg)                          \
  mips_emit_addiu(reg_a2, arm_to_mips_reg[base_reg],                          \
   (-(bit_count[reg_list] << 2)));                                            \
  mips_emit_addu(arm_to_mips_reg[base_reg], reg_a2, reg_zero)                 \

/* プレデクリメント アドレッシング PUSH rlist, lr */
#define thumb_block_address_preadjust_push_lr(base_reg)                       \
  mips_emit_addiu(reg_a2, arm_to_mips_reg[base_reg],                          \
   (-((bit_count[reg_list] + 1) << 2)));                                      \
  mips_emit_addu(arm_to_mips_reg[base_reg], reg_a2, reg_zero)                 \

/* ベースレジスタ更新 */
#define thumb_block_address_postadjust_no(base_reg)                           \

#define thumb_block_address_postadjust_up(base_reg)                           \
  mips_emit_addiu(arm_to_mips_reg[base_reg], reg_a2,                          \
   (bit_count[reg_list] << 2))                                                \

#define thumb_block_address_postadjust_down(base_reg)                         \
  mips_emit_addiu(arm_to_mips_reg[base_reg], reg_a2,                          \
   (-(bit_count[reg_list] << 2)))                                             \

#define thumb_block_address_postadjust_pop_pc(base_reg)                       \
  mips_emit_addiu(arm_to_mips_reg[base_reg], reg_a2,                          \
   ((bit_count[reg_list] + 1) << 2))                                          \

#define thumb_block_address_postadjust_push_lr(base_reg)                      \

/* nS+1N+1I for LDM, (n-1)S+2N for STM */
/* nS+1N+1I (POP), (n+1)S+2N+1I (POP PC), (n-1)S+2N (PUSH) */
#define thumb_block_memory_load()                                             \
  block_memory_waitstate(reg_a0 >> 24);                                       \
  generate_function_call_swap_delay(execute_aligned_load32);                  \
  generate_store_reg(reg_rv, i)                                               \

#define thumb_block_memory_store()                                            \
  block_memory_waitstate(reg_a0 >> 24);                                       \
  mips_emit_jal(mips_absolute_offset(execute_aligned_store32));               \
  generate_load_reg(reg_a1, i)                                                \

#define thumb_block_memory_final_load()                                       \
  cycle_count++;                                                              \
  thumb_block_memory_load()                                                   \

#define thumb_block_memory_final_store()                                      \
  generate_load_pc(reg_a2, (pc + 2));                                         \
  mips_emit_jal(mips_absolute_offset(execute_store_u32));                     \
  generate_load_reg(reg_a1, i)                                                \

#define thumb_block_memory_final_no(access_type)                              \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_up(access_type)                              \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_down(access_type)                            \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_push_lr(access_type)                         \
  thumb_block_memory_##access_type()                                          \

#define thumb_block_memory_final_pop_pc(access_type)                          \
  thumb_block_memory_##access_type()                                          \

#define thumb_block_memory_extra_no()                                         \

#define thumb_block_memory_extra_up()                                         \

#define thumb_block_memory_extra_down()                                       \

#define thumb_block_memory_extra_push_lr()                                    \
  mips_emit_addiu(reg_a0, reg_a2, (bit_count[reg_list] << 2));                \
  mips_emit_jal(mips_absolute_offset(execute_aligned_store32));               \
  generate_load_reg(reg_a1, REG_LR)                                           \

#define thumb_block_memory_extra_pop_pc()                                     \
  cycle_count += 2;                                                           \
  mips_emit_jal(mips_absolute_offset(execute_aligned_load32));                \
  mips_emit_addiu(reg_a0, reg_a2, (bit_count[reg_list] << 2));                \
  generate_mov(reg_a0, reg_rv);                                               \
  generate_indirect_branch_cycle_update(thumb)                                \

#define thumb_block_memory_sp_load()                                          \
  mips_emit_lw(arm_to_mips_reg[i], reg_a1, offset)                            \

#define cycle_thumb_block_memory_sp_store()                                   \

#define cycle_thumb_block_memory_sp_load()                                    \
  cycle_count++                                                               \

#define thumb_block_memory_sp_store()                                         \
  mips_emit_sw(arm_to_mips_reg[i], reg_a1, offset)                            \

#define thumb_block_memory_sp_extra_no()                                      \

#define thumb_block_memory_sp_extra_up()                                      \

/* PUSH rlist */
#define thumb_block_memory_sp_extra_down()                                    \

/* POP PC */
#define thumb_block_memory_sp_extra_pop_pc()                                  \
  cycle_count += 2;                                                           \
  mips_emit_lw(reg_a0, reg_a1, (bit_count[reg_list] << 2));                   \
  generate_indirect_branch_cycle_update(thumb)                                \

/* PUSH LR */
#define thumb_block_memory_sp_extra_push_lr()                                 \
  mips_emit_sw(reg_r14, reg_a1, (bit_count[reg_list] << 2))                   \

#define thumb_block_memory_writeback_load(post_op, base_reg)                  \
  if(~((reg_list >> base_reg) & 0x01))                                        \
  {                                                                           \
    thumb_block_address_postadjust_##post_op(base_reg);                       \
  }                                                                           \

#define thumb_block_memory_writeback_store(post_op, base_reg)                 \
  thumb_block_address_postadjust_##post_op(base_reg)                          \

#define thumb_block_memory(access_type, pre_op, post_op, base_reg)            \
{                                                                             \
  thumb_decode_rlist();                                                       \
  u32 i;                                                                      \
  u32 offset = 0;                                                             \
                                                                              \
  cycle_count += cycle_thumb_fetch(pc);                                       \
                                                                              \
  thumb_block_address_preadjust_##pre_op(base_reg);                           \
/*  thumb_block_address_postadjust_##post_op(base_reg); */                        \
  thumb_block_memory_writeback_##access_type(post_op, base_reg);              \
                                                                              \
  if((base_reg == REG_SP) && iwram_stack_optimize)                            \
  {                                                                           \
    mips_emit_andi(reg_a1, reg_a2, 0x7FFC);                                   \
    generate_load_imm(reg_a0, ((u32)(iwram + 0x8000)));                       \
    generate_add(reg_a1, reg_a0);                                             \
                                                                              \
    for(i = 0; i < 8; i++)                                                    \
    {                                                                         \
      if((reg_list >> i) & 0x01)                                              \
      {                                                                       \
        cycle_count++;                                                        \
        thumb_block_memory_sp_##access_type();                                \
        offset += 4;                                                          \
      }                                                                       \
    }                                                                         \
    cycle_thumb_block_memory_sp_##access_type();                              \
    thumb_block_memory_sp_extra_##post_op();                                  \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    mips_emit_ins(reg_a2, reg_zero, 0, 2);                                    \
                                                                              \
    for(i = 0; i < 8; i++)                                                    \
    {                                                                         \
      if((reg_list >> i) & 0x01)                                              \
      {                                                                       \
        mips_emit_addiu(reg_a0, reg_a2, offset);                              \
                                                                              \
        if(reg_list & ~((2 << i) - 1))                                        \
        {                                                                     \
          thumb_block_memory_##access_type();                                 \
          offset += 4;                                                        \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          thumb_block_memory_final_##post_op(access_type);                    \
          break;                                                              \
        }                                                                     \
      }                                                                       \
    }                                                                         \
    thumb_block_memory_extra_##post_op();                                     \
  }                                                                           \
}                                                                             \


#define thumb_conditional_branch(condition)                                   \
{                                                                             \
  cycle_count += 2 + cycle_thumb_fetch(pc);                                   \
  CONDITION_CHECK_TYPE condition_check;                                       \
  generate_condition_##condition();                                           \
  generate_branch_no_cycle_update(                                            \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  generate_branch_patch_conditional(backpatch_address, translation_ptr);      \
  block_exit_position++;                                                      \
}                                                                             \

#define arm_conditional_block_header()                                        \
  generate_condition()                                                        \

#define arm_b()                                                               \
  cycle_count += 2 + cycle_arm_base(pc);                                      \
  generate_branch()                                                           \

#define arm_bl()                                                              \
  cycle_count += 2 + cycle_arm_base(pc);                                      \
  generate_load_pc(reg_r14, (pc + 4));                                        \
  generate_branch()                                                           \

#define arm_bx()                                                              \
  cycle_count += 2 + cycle_arm_base(pc);                                      \
  arm_decode_branchx();                                                       \
  generate_load_reg(reg_a0, rn);                                              \
  /*generate_load_pc(reg_a2, pc);*/                                           \
  generate_indirect_branch_dual()                                             \

#define arm_swi()                                                             \
{                                                                             \
  cycle_count += 2 + cycle_arm_base(pc);                                      \
  generate_swi_hle_handler((opcode >> 16) & 0xFF);                            \
  generate_load_pc(reg_a0, (pc + 4));                                         \
  generate_function_call_swap_delay(execute_swi);                             \
  generate_branch();                                                          \
}                                                                             \

#define thumb_b()                                                             \
  cycle_count += 2 + cycle_thumb_base(pc);                                    \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++                                                       \

#define thumb_bl()                                                            \
  cycle_count += 2 + cycle_thumb_base(pc);                                    \
  generate_load_pc(reg_r14, ((pc + 2) | 0x01));                               \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++                                                       \

#define thumb_blh()                                                           \
{                                                                             \
  cycle_count += 2 + cycle_thumb_base(pc);                                    \
  thumb_decode_branch();                                                      \
  generate_alu_imm(addiu, addu, reg_a0, reg_r14, (offset << 1));              \
  generate_load_pc(reg_r14, ((pc + 2) | 0x01));                               \
  generate_indirect_branch_cycle_update(thumb);                               \
}                                                                             \

#define thumb_bx()                                                            \
{                                                                             \
  cycle_count += 2 + cycle_thumb_base(pc);                                    \
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(reg_a0, rs, 4);                                        \
  /*generate_load_pc(reg_a2, pc);*/                                           \
  generate_indirect_branch_cycle_update(dual);                                \
}                                                                             \

#define thumb_swi()                                                           \
{                                                                             \
  cycle_count += 2 + cycle_thumb_base(pc);                                    \
  generate_swi_hle_handler(opcode & 0xFF);                                    \
  generate_load_pc(reg_a0, (pc + 2));                                         \
  generate_function_call_swap_delay(execute_swi);                             \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++;                                                      \
}                                                                             \

u8 swi_hle_handle[256] =
{
  0x0,    // SWI 0:  SoftReset
  0x0,    // SWI 1:  RegisterRAMReset
  0x0,    // SWI 2:  Halt
  0x0,    // SWI 3:  Stop/Sleep
  0x0,    // SWI 4:  IntrWait
  0x0,    // SWI 5:  VBlankIntrWait
  0x1,    // SWI 6:  Div
  0x0,    // SWI 7:  DivArm
  0x0,    // SWI 8:  Sqrt
  0x0,    // SWI 9:  ArcTan
  0x0,    // SWI A:  ArcTan2
  0x0,    // SWI B:  CpuSet
  0x0,    // SWI C:  CpuFastSet
  0x1,    // SWI D:  GetBIOSCheckSum
  0x0,    // SWI E:  BgAffineSet
  0x0,    // SWI F:  ObjAffineSet
  0x0,    // SWI 10: BitUnpack
  0x0,    // SWI 11: LZ77UnCompWram
  0x0,    // SWI 12: LZ77UnCompVram
  0x0,    // SWI 13: HuffUnComp
  0x0,    // SWI 14: RLUnCompWram
  0x0,    // SWI 15: RLUnCompVram
  0x0,    // SWI 16: Diff8bitUnFilterWram
  0x0,    // SWI 17: Diff8bitUnFilterVram
  0x0,    // SWI 18: Diff16bitUnFilter
  0x0,    // SWI 19: SoundBias
  0x0,    // SWI 1A: SoundDriverInit
  0x0,    // SWI 1B: SoundDriverMode
  0x0,    // SWI 1C: SoundDriverMain
  0x0,    // SWI 1D: SoundDriverVSync
  0x0,    // SWI 1E: SoundChannelClear
  0x0,    // SWI 1F: MidiKey2Freq
  0x0,    // SWI 20: SoundWhatever0
  0x0,    // SWI 21: SoundWhatever1
  0x0,    // SWI 22: SoundWhatever2
  0x0,    // SWI 23: SoundWhatever3
  0x0,    // SWI 24: SoundWhatever4
  0x0,    // SWI 25: MultiBoot
  0x0,    // SWI 26: HardReset
  0x0,    // SWI 27: CustomHalt
  0x0,    // SWI 28: SoundDriverVSyncOff
  0x0,    // SWI 29: SoundDriverVSyncOn
  0x0     // SWI 2A: SoundGetJumpList
};

#define generate_swi_hle_handler(_swi_number)                                 \
{                                                                             \
  u32 swi_number = _swi_number;                                               \
                                                                              \
  if(swi_hle_handle[swi_number])                                              \
  {                                                                           \
    /* Div */                                                                 \
    if(swi_number == 0x06)                                                    \
    {                                                                         \
      mips_emit_div(reg_r0, reg_r1);                                          \
      mips_emit_mflo(reg_r0);                                                 \
      mips_emit_mfhi(reg_r1);                                                 \
      mips_emit_sra(reg_a0, reg_r0, 31);                                      \
      mips_emit_xor(reg_r3, reg_r0, reg_a0);                                  \
      mips_emit_subu(reg_r3, reg_r3, reg_a0);                                 \
    }                                                                         \
                                                                              \
    /* GetBIOSCheckSum */                                                     \
    if(swi_number == 0x0D)                                                    \
    {                                                                         \
      generate_load_imm(reg_r0, 0xBAAE187F);                                  \
    }                                                                         \
                                                                              \
    break;                                                                    \
  }                                                                           \
}                                                                             \

#define generate_translation_gate(type)                                       \
  generate_load_pc(reg_a0, pc);                                               \
  generate_indirect_branch_no_cycle_update(type)                              \

#define generate_update_pc_reg()                                              \
  generate_load_pc(reg_a0, pc);                                               \
  mips_emit_sw(reg_a0, reg_base, (REG_PC << 2))                               \


#define cycle_arm_base(_address)                                              \
  waitstate_cycles_seq[1][_address >> 24]                                     \

#define cycle_thumb_base(_address)                                            \
  waitstate_cycles_seq[0][_address >> 24]                                     \

#define cycle_arm_fetch(_address)                                             \
  waitstate_cycles_fetch[1][_address >> 24]                                   \

#define cycle_thumb_fetch(_address)                                           \
  waitstate_cycles_fetch[0][_address >> 24]                                   \

#define block_memory_waitstate(mem_region)                                    \
  if(offset == 0 ||                                                           \
    (mem_region >= 0x08 && mem_region <= 0x0D && (reg_a0 % 0x20000) == 0))    \
  {                                                                           \
    cycle_count += waitstate_cycles_non_seq[1][mem_region];                   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    cycle_count += waitstate_cycles_seq[1][mem_region];                       \
  }                                                                           \

static u8 cycle_multiply(u32 _rs)
{
  u32 mult_rs = arm_to_mips_reg[_rs];

  if(((s32)mult_rs) < 0)
    mult_rs = ~mult_rs;

  /* m=1 for Bit 31-8 */
  if((mult_rs & 0xFFFFFF00) == 0)
    return 1;
  else
  /* m=2 for Bit 31-16 */
  if((mult_rs & 0xFFFF0000) == 0)
    return 2;
  else
  /* m=3 for Bit 31-24 */
  if((mult_rs & 0xFF000000) == 0)
    return 3;
  else
  /* m=4 otherwise */
    return 4;
}

#endif /* MIPS_EMIT_H */
