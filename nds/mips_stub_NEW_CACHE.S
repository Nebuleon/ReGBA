# unofficial gameplaySP kai
#
# Copyright (C) 2006 Exophase <exophase@gmail.com>
# Copyright (C) 2007 takka <takka@tfact.net>
# Copyright (C) 2007 _____ <_____>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

# CPUエミュレート ASMコア

.align 4

.global mips_update_gba
.global mips_indirect_branch_arm
.global mips_indirect_branch_thumb
.global mips_indirect_branch_dual
.global execute_load_u8
.global execute_load_u16
.global execute_load_u32
.global execute_load_s8
.global execute_load_s16
.global execute_store_u8
.global execute_store_u16
.global execute_store_u32
.global execute_aligned_load32
.global execute_aligned_store32
.global execute_read_cpsr
.global execute_read_spsr
.global execute_swi
.global execute_spsr_restore
.global execute_store_cpsr
.global execute_store_spsr
.global execute_lsl_flags_reg
.global execute_lsr_flags_reg
.global execute_asr_flags_reg
.global execute_ror_flags_reg
.global execute_arm_translate
.global invalidate_icache_region
.global invalidate_all_cache
.global reg_check

.global memory_map_read
.global memory_map_write
.global reg

.extern spsr

# MIPS register layout:

# $0 - constant zero
# $1 - temporary
# $2 - temporary / return value
# $3 - ARM r0 (not saved)
# $4 - temporary / function argument 0
# $5 - temporary / function argument 1
# $6 - temporary / function argument 2
# $7 - ARM r1 (not saved)
# $8 - ARM r2 (not saved)
# $9 - ARM r3 (not saved)
# $10 - ARM r4 (not saved)
# $11 - ARM r5 (not saved)
# $12 - ARM r6 (not saved)
# $13 - ARM r7 (not saved)
# $14 - ARM r8 (not saved)
# $15 - ARM r9 (not saved)
# $16 - ARM machine state pointer (saved)
# $17 - cycle counter (saved)
# $18 - ARM r10 (saved)
# $19 - block start address (roughly r15) (saved)
# $20 - ARM negative register (saved)
# $21 - ARM zero register (saved)
# $22 - ARM carry register (saved)
# $23 - ARM overflow register (saved)
# $24 - ARM r11 (not saved)
# $25 - ARM r12 (not saved)
# $26 - kernel temporary 0
# $27 - kernel temporary 1
# $28 - ARM r13 (saved)
# $29 - stack pointer
# $30 - ARM r14 (saved)
# $31 - return address

.equ REG_R0,              (0 * 4)
.equ REG_R1,              (1 * 4)
.equ REG_R2,              (2 * 4)
.equ REG_R3,              (3 * 4)
.equ REG_R4,              (4 * 4)
.equ REG_R5,              (5 * 4)
.equ REG_R6,              (6 * 4)
.equ REG_R7,              (7 * 4)
.equ REG_R8,              (8 * 4)
.equ REG_R9,              (9 * 4)
.equ REG_R10,             (10 * 4)
.equ REG_R11,             (11 * 4)
.equ REG_R12,             (12 * 4)
.equ REG_R13,             (13 * 4)
.equ REG_R14,             (14 * 4)
.equ REG_LR,              (14 * 4)
.equ REG_PC,              (15 * 4)
.equ REG_N_FLAG,          (16 * 4)
.equ REG_Z_FLAG,          (17 * 4)
.equ REG_C_FLAG,          (18 * 4)
.equ REG_V_FLAG,          (19 * 4)
.equ REG_CPSR,            (20 * 4)
.equ REG_SAVE,            (21 * 4)
.equ REG_SAVE2,           (22 * 4)
.equ REG_SAVE3,           (23 * 4)
.equ CPU_MODE,            (29 * 4)
.equ CPU_HALT_STATE,      (30 * 4)
.equ CHANGED_PC_STATUS,   (31 * 4)
.equ GP_SAVE,             (32 * 4)

.equ SUPERVISOR_LR,       (reg_mode + (3 * (7 * 4)) + (6 * 4))
.equ SUPERVISOR_SPSR,     (spsr + (3 * 4))

.equ CPU_ALERT_NONE,  0
.equ CPU_ALERT_HALT,  1
.equ CPU_ALERT_SMC_I, 2
.equ CPU_ALERT_SMC_E, 3
.equ CPU_ALERT_IRQ,   4

.set noat
.set noreorder

# make sure $16 has the register base for these macros

.macro collapse_flag flag_reg, shift
  ins $2, $\flag_reg, \shift, 1    # insert flag into CPSR
.endm

.macro collapse_flags
  lw $2, REG_CPSR($16)            # load CPSR
  andi $2, $2, 0xFF               # isolate lower 8bits
  collapse_flag 20, 31            # store flags
  collapse_flag 21, 30
  collapse_flag 22, 29
  collapse_flag 23, 28
  sw $2, REG_CPSR($16)            # store CPSR
.endm

.macro extract_flag shift, flag_reg
  ext $\flag_reg, $1, \shift, 1   # extract flag from CPSR
.endm

.macro extract_flags_body         # extract flags from $1
  extract_flag 31, 20             # load flags
  extract_flag 30, 21
  extract_flag 29, 22
  extract_flag 28, 23
.endm

.macro extract_flags
  lw $1, REG_CPSR($16)            # load CPSR
  extract_flags_body
.endm

.macro save_registers
  sw $3, REG_R0($16)
  sw $7, REG_R1($16)
  sw $8, REG_R2($16)
  sw $9, REG_R3($16)
  sw $10, REG_R4($16)
  sw $11, REG_R5($16)
  sw $12, REG_R6($16)
  sw $13, REG_R7($16)
  sw $14, REG_R8($16)
  sw $15, REG_R9($16)
  sw $24, REG_R11($16)
  sw $25, REG_R12($16)

  sw $18, REG_R10($16)
  sw $28, REG_R13($16)
  sw $30, REG_R14($16)

  lw $28, GP_SAVE($16)
.endm

.macro restore_registers
  lw $3, REG_R0($16)
  lw $7, REG_R1($16)
  lw $8, REG_R2($16)
  lw $9, REG_R3($16)
  lw $10, REG_R4($16)
  lw $11, REG_R5($16)
  lw $12, REG_R6($16)
  lw $13, REG_R7($16)
  lw $14, REG_R8($16)
  lw $15, REG_R9($16)
  lw $24, REG_R11($16)
  lw $25, REG_R12($16)

  lw $18, REG_R10($16)
  lw $28, REG_R13($16)
  lw $30, REG_R14($16)
.endm

# Process a hardware event. Since an interrupt might be
# raised we have to check if the PC has changed.

# $4: next address
# $16: register base
# $17: cycle counter

.balign 64

mips_update_gba:
  sw $4, REG_PC($16)              # current PC = $4

  addiu $sp, $sp, -4              # make room on the stack
  sw $ra,($sp)                    # save return address
  collapse_flags                  # update cpsr
  save_registers                  # save registers
  jal update_gba                  # process the next event
  sw $0, CHANGED_PC_STATUS($16)

  lw $ra, ($sp)                   # restore return address
  addiu $sp, $sp, 4               # fix stack

  lw $1, CHANGED_PC_STATUS($16)
  bne $1, $0, lookup_pc
  addu $17, $2, $0                # $17 = new cycle count (delay slot)

  restore_registers

  jr $ra                          # if not, go back to caller
  nop

# Perform an indirect branch.

# $4: GBA address to branch to

mips_indirect_branch_arm:
  save_registers
  jal block_lookup_address_arm    # $2 = MIPS address to jump to
  nop
  restore_registers
  jr $2                           # jump to it
  nop

mips_indirect_branch_thumb:
  save_registers
  jal block_lookup_address_thumb  # $2 = MIPS address to jump to
  nop
  restore_registers
  jr $2                           # jump to it
  nop

mips_indirect_branch_dual:
  save_registers
  jal block_lookup_address_dual   # $2 = MIPS address to jump to
  nop
  restore_registers
  jr $2                           # jump to it
  nop


# $4: address to write to
# $5: current PC

# Will patch the return address with a call to the correct handler as
# listed in the given table.

# Value will be set to force_open if it's open

.macro patch_handler ftable, force_open
  srl $1, $4, 24                  # $1 = address region
  sltu $2, $1, 0x0F               # check if the value is open
  bne $2, $0, 1f
  sll $1, $1, 2                   # make address word indexed (delay)

  addiu $1, $0, (\force_open * 4)

1:
  lui $2, %hi(\ftable)
  addu $2, $2, $1
  lw $2, %lo(\ftable)($2)         # new function handler is in $2
  srl $2, $2, 2                   # remove lower two bits

  lui $1, %hi(3 << 26)            # $1 = 3 (JAL opcode)
  ins $1, $2, 0, 26               # insert offset into jal

  addiu $ra, $ra, -8              # rewind return address to function call
  sw $1, ($ra)                    # modify to call new handler

  cache 0x1a, ($ra)               # writeback dcache line
  cache 0x08, ($ra)               # invalidate icache line

  jr $ra                          # return
  nop                             # wary of putting cache here
.endm


# Like the above, but will use the table of the proper alignment,
# The tables should be ordered by alignment

.macro patch_handler_align ftable, alignment
  srl $1, $4, 24                  # $1 = address region
  sltu $2, $1, 0x0F               # check if the value is open
  bne $2, $0, 1f
  sll $1, $1, 2                   # make address word indexed (delay)

  addiu $1, $0, 4                 # force address to 0x1 (open)

1:
  ins $1, $4, 6, \alignment       # place alignment bits into offset
  lui $2, %hi(\ftable)

  addu $2, $2, $1
  lw $2, %lo(\ftable)($2)         # new function handler is in $2

  srl $2, $2, 2                   # remove lower two bits

  lui $1, %hi(3 << 26)            # $1 = 3 (JAL opcode)
  ins $1, $2, 0, 26               # insert offset into jal

  addiu $ra, $ra, -8              # rewind return address to function call
  sw $1, ($ra)                    # modify to call new handler

  cache 0x1a, ($ra)               # writeback dcache line
  cache 0x08, ($ra)               # invalidate icache line

  jr $ra                          # return
  nop                             # wary of putting cache here
.endm


.macro region_check region, patch_handler
  srl $1, $4, 24                  # check upper 8bits of address
  xor $1, $1, \region             # see if it is the given region
  bne $1, $0, \patch_handler      # if not repatch/try again
.endm

.macro region_check_open patch_handler
  srl $1, $4, 24                  # check upper 8bits of address
  sltiu $2, $1, 0x0F              # true if it is a low address
  addiu $1, $1, -1                # non-zero if it is not a low open
  sltu $1, $0, $1                 # true if lower bits != 1
  and $1, $1, $2                  # true if low address and not open
  bne $1, $0, \patch_handler      # if above is true, patch
.endm


.macro region_check_align region, align_bits, alignment, patch_handler
  srl $1, $4, 24                  # check upper 8bits of address
  ins $1, $4, 8, \align_bits      # look at lower bits of address too
  # See if it is the given region and alignment
  xori $1, $1, (\region | (\alignment << 8))
  bne $1, $0, \patch_handler      # if not repatch/try again
.endm

.macro region_check_open_align align_bits, alignment, patch_handler
  srl $1, $4, 24                  # check upper 8bits of address
  sltiu $2, $1, 0x0F              # true if it is a low address
  addiu $1, $1, -1                # non-zero if it is not a low open
  sltu $1, $0, $1                 # true if $1 != 0
  and $1, $1, $2                  # true if low address and not open
  ext $2, $4, 0, \align_bits      # $2 = low bits of 4
  xori $2, $2, \alignment         # true if alignment doesn't match
  or $1, $1, $2                   # align failure will trigger too
  bne $1, $0, \patch_handler      # if above is true, patch
.endm


.macro ignore_region region, patch_handler
  region_check \region, \patch_handler
  nop
  jr $ra
  nop
.endm

.macro ignore_high patch_handler
  srl $1, $4, 24                  # check upper 8bits of address
  sltiu $1, $1, 0x0F              # see if it is not high
  bne $1, $0, \patch_handler      # if not repatch/try again
  nop
  jr $ra
  nop
.endm


.macro translate_region_core base, size
  lui $2, %hi(\base)              # generate upper address
  andi $4, $4, \size              # generate offset
  addu $2, $2, $4                 # add ptr upper and offset
.endm

.macro translate_region region, patch_handler, base, size
  region_check \region, \patch_handler
  translate_region_core \base, \size
.endm

# I refuse to have > 80 char lines, and GAS has a problem with the param
# list spilling over (grumble)

.macro translate_region_align region, a_b, alignment, p_h, base, size
  region_check_align \region, \a_b, \alignment, \p_h
  translate_region_core \base, \size
.endm


.macro translate_region_ewram_core mask
  lui $2, %hi(ewram + 0x8000)     # generate upper address (delay)
  andi $1, $4, \mask              # generate 15bit offset
  ext $4, $4, 15, 3               # isolate top 3 bits of offset
  ins $1, $4, 16, 3               # reinsert into top 4 bits
  addu $2, $2, $1
.endm

.macro translate_region_ewram patch_handler
  region_check 2, \patch_handler
  translate_region_ewram_core 0x7FFF
.endm

.macro translate_region_ewram_load_align align_bits, alignment, patch_handler
  region_check_align 2, \align_bits, \alignment, \patch_handler
  translate_region_ewram_core 0x7FFF
.endm

.macro translate_region_ewram_load_align16 align_bits, alignment, patch_handler
  region_check_align 2, \align_bits, \alignment, \patch_handler
  translate_region_ewram_core 0x7FFE
.endm

.macro translate_region_ewram_load_align32 align_bits, alignment, patch_handler
  region_check_align 2, \align_bits, \alignment, \patch_handler
  translate_region_ewram_core 0x7FFC
.endm

.macro translate_region_ewram_store_align16 patch_handler
  region_check 2, \patch_handler
  translate_region_ewram_core 0x7FFE
.endm

.macro translate_region_ewram_store_align32 patch_handler
  region_check 2, \patch_handler
  translate_region_ewram_core 0x7FFC
.endm


.macro translate_region_vram_core
  addiu $2, $2, -3                # see if it's 3
  ext $4, $4, 0, 17               # generate 17bit offset
  bne $2, $0, 1f                  # if $2 != $0 then j 1:
  lui $1, %hi(vram)               # start loading vram address (delay)

  addiu $4, $4, -0x8000           # move address into VRAM region

1:
  addu $2, $1, $4                 # $2 = (hi)vram + address
.endm

.macro translate_region_vram patch_handler
  region_check 6, \patch_handler
  ext $2, $4, 15, 2               # $2 = bits 15 and 16 of address (delay)
  translate_region_vram_core
.endm

.macro translate_region_vram_load_align align_bits, alignment, patch_handler
  region_check_align 6, \align_bits, \alignment, \patch_handler
  ext $2, $4, 15, 2               # $2 = bits 15 and 16 of address (delay)
  translate_region_vram_core
.endm

.macro translate_region_vram_load_align16 align_bits, alignment, patch_handler
  region_check_align 6, \align_bits, \alignment, \patch_handler
  ext $2, $4, 15, 2               # $2 = bits 15 and 16 of address (delay)
  ins $4, $0, 0, 1                # mask out lower bit of address
  translate_region_vram_core
.endm

.macro translate_region_vram_load_align32 align_bits, alignment, patch_handler
  region_check_align 6, \align_bits, \alignment, \patch_handler
  ext $2, $4, 15, 2               # $2 = bits 15 and 16 of address (delay)
  ins $4, $0, 0, 2                # mask out lower two bits of address
  translate_region_vram_core
.endm

.macro translate_region_vram_store_align16 patch_handler
  region_check 6, \patch_handler
  ext $2, $4, 15, 2               # $2 = bits 15 and 16 of address (delay)
  ins $4, $0, 0, 1                # mask out lower bit of address
  translate_region_vram_core
.endm

.macro translate_region_vram_store_align32 patch_handler
  region_check 6, \patch_handler
  ext $2, $4, 15, 2               # $2 = bits 15 and 16 of address (delay)
  ins $4, $0, 0, 2                # mask out lower two bits of address
  translate_region_vram_core
.endm

.macro translate_region_gamepak_core mask
  srl $2, $4, 15                  # $2 = page number of address (delay)
  sll $2, $2, 2                   # adjust to word index
  addu $2, $2, $16                # $2 = memory_map_read[address >> 15]
  lw $2, -32768($2)               #
  bne $2, $0, 1f                  # if it's non-NULL continue
  andi $1, $4, \mask              # $1 = low 15bits of address (delay slot)

  sw $ra, REG_SAVE2($16)          # save return address

  save_registers                  # save the registers
  ext $4, $4, 15, 10              # $4 = (address >> 15) & 0x3FF

  jal load_gamepak_page           # get page in $2
  sw $1, REG_SAVE($16)            # save offset (delay)
  lw $1, REG_SAVE($16)            # restore offset (delay)

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE2($16)          # restore return address

1:
  addu $2, $2, $1                 # add the memory map offset
.endm

.macro translate_region_gamepak region, patch_handler
  region_check \region, \patch_handler
  translate_region_gamepak_core 0x7FFF
.endm

.macro translate_region_gamepak_align region, a_b, alignment, patch_handler
  region_check_align \region, \a_b, \alignment, \patch_handler
  translate_region_gamepak_core 0x7FFF
.endm

.macro translate_region_gamepak_align16 region, a_b, alignment, patch_handler
  region_check_align \region, \a_b, \alignment, \patch_handler
  translate_region_gamepak_core 0x7FFE
.endm

.macro translate_region_gamepak_align32 region, a_b, alignment, patch_handler
  region_check_align \region, \a_b, \alignment, \patch_handler
  translate_region_gamepak_core 0x7FFC
.endm


.macro translate_region_gamepak_a region, patch_handler
  region_check \region, \patch_handler
  srl $2, $4, 15                  # $2 = page number of address (delay)
  sll $2, $2, 2                   # adjust to word index
  addu $2, $2, $16                # $2 = memory_map_read[address >> 15]
  lw $2, -32768($2)
  bne $2, $0, 1f                  # if it's non-NULL continue
  andi $1, $4, 0x7FFF             # $1 = low 15bits of address (delay slot)

  sw $ra, REG_SAVE2($16)          # save return address
  sw $6, REG_SAVE3($16)           # save a2

  save_registers                  # save the registers
  ext $4, $4, 15, 10              # $4 = (address >> 15) & 0x3FF

  jal load_gamepak_page           # get page in $2
  sw $1, REG_SAVE($16)            # save offset (delay)
  lw $1, REG_SAVE($16)            # restore offset

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE2($16)          # restore return address
  lw $6, REG_SAVE3($16)           # restore a2

1:
  addu $2, $2, $1                 # add the memory map offset
.endm


.macro eeprom_load_a patch_handler
  region_check 0xD, \patch_handler

  sw $ra, REG_SAVE($16)           # save the return address (delay)
  sw $6, REG_SAVE2($16)           # save a2

  save_registers                  # save the registers

  jal read_eeprom                 # get eeprom value in $2
  nop

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
  lw $6, REG_SAVE2($16)           # restore a2 (delay)
.endm


.macro eeprom_load_core
  sw $ra, REG_SAVE($16)           # save the return address (delay)

  save_registers                  # save the registers

  jal read_eeprom                 # get eeprom value in $2
  nop

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
  nop
.endm

.macro eeprom_load patch_handler
  region_check 0xD, \patch_handler
  eeprom_load_core
.endm

.macro eeprom_load_align align_bits, alignment, patch_handler
  region_check_align 0xD, \align_bits, \alignment, \patch_handler
  eeprom_load_core
.endm

.macro eeprom_load_align16 align_bits, alignment, patch_handler
  eeprom_load_align \align_bits, \alignment, \patch_handler
.endm

.macro eeprom_load_align32 align_bits, alignment, patch_handler
  eeprom_load_align \align_bits, \alignment, \patch_handler
.endm


.macro backup_load_core
  save_registers                  # save the registers

  jal read_backup                 # get backup value in $2
  ext $4, $4, 0, 16               # address &= 0xFFFF (delay)

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
.endm

.macro backup_load_a patch_handler
  region_check 0xE, \patch_handler
  sw $ra, REG_SAVE($16)           # save return address (delay)
  sw $6, REG_SAVE2($16)           # save a2

  save_registers                  # save the registers

  jal read_backup                 # get backup value in $2
  ext $4, $4, 0, 16               # address &= 0xFFFF (delay)

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
  lw $6, REG_SAVE2($16)           # restore a2 (delay)
.endm


.macro backup_load patch_handler
  region_check 0xE, \patch_handler
  sw $ra, REG_SAVE($16)           # save the return address (delay)
  backup_load_core
.endm

.macro backup_load_align align_bits, alignment, patch_handler
  region_check_align 0xE, \align_bits, \alignment, \patch_handler
  sw $ra, REG_SAVE($16)           # save the return address (delay)
  backup_load_core
.endm

.macro backup_load_align16 align_bits, alignment, patch_handler
  region_check_align 0xE, \align_bits, \alignment, \patch_handler
  sw $ra, REG_SAVE($16)           # save the return address (delay)
  ins $4, $0, 0, 1                # mask out lower bit
  backup_load_core
.endm

.macro backup_load_align32 align_bits, alignment, patch_handler
  region_check_align 0xE, \align_bits, \alignment, \patch_handler
  sw $ra, REG_SAVE($16)           # save the return address (delay)
  ins $4, $0, 0, 2                # mask out lower two bits
  backup_load_core
.endm


.macro open_load8_core
  lw $2, REG_CPSR($16)            # $2 = CPSR (delay)
  andi $2, $2, 0x20               # test T bit
  beq $2, $0, 1f                  # branch if ARM mode
  andi $4, $4, 0x03               # isolate lower 3bits from address (delay)

  andi $4, $4, 0x01               # in Thumb mode, isolate one more bit

1:
  sw $ra, REG_SAVE($16)           # save the return address
  save_registers                  # save the registers

  jal read_memory8                # get instruction at PC
  addu $4, $5, $4                 # a0 = PC + low bits of address

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
.endm

.macro open_load8 patch_handler
  region_check_open \patch_handler
  open_load8_core
.endm



.macro open_load16_core
  lw $2, REG_CPSR($16)            # $2 = CPSR (delay)
  andi $2, $2, 0x20               # test T bit
  beq $2, $0, 1f                  # branch if ARM mode
  andi $4, $4, 0x02               # isolate bit 1 from address (delay)

  addu $4, $0, $0                 # zero out address bit

1:
  sw $ra, REG_SAVE($16)           # save the return address
  save_registers                  # save the registers

  jal read_memory16               # get instruction at PC
  addu $4, $5, $4                 # a0 = PC + low bits of address

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
.endm

.macro open_load16_align align_bits, alignment, patch_handler
  region_check_open_align \align_bits, \alignment, \patch_handler
  open_load16_core
.endm

.macro open_load16_align16 align_bits, alignment, patch_handler
  open_load16_align \align_bits, \alignment, \patch_handler
.endm



.macro open_load32_core
  lw $2, REG_CPSR($16)            # $2 = CPSR (delay)
  andi $2, $2, 0x20               # test T bit

  save_registers                  # save the registers

  beq $2, $0, 1f                  # branch if ARM mode
  sw $ra, REG_SAVE($16)           # save the return address (delay)

  jal read_memory16               # get instruction at PC
  addu $4, $5, $0                 # a0 = PC (delay)

  j 2f
  ins $2, $2, 16, 16              # result = (result << 16) | result (delay)

1:
  jal read_memory32               # get instruction at PC
  addu $4, $5, $4                 # a0 = PC

2:                                # join point
  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
.endm

.macro open_load32_a patch_handler
  region_check_open \patch_handler

  lw $2, REG_CPSR($16)            # $2 = CPSR (delay)
  andi $2, $2, 0x20               # test T bit

  save_registers                  # save the registers
  sw $6, REG_SAVE2($16)           # save a2

  beq $2, $0, 1f                  # branch if ARM mode
  sw $ra, REG_SAVE($16)           # save the return address (delay)

  jal read_memory16               # get instruction at PC
  addu $4, $5, $0                 # a0 = PC (delay)

  j 2f
  ins $2, $2, 16, 16              # result = (result << 16) | result (delay)

1:
  jal read_memory32               # get instruction at PC
  addu $4, $5, $4                 # a0 = PC

2:
  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
  lw $6, REG_SAVE2($16)           # restore a2 (delay)
.endm

.macro open_load32_align align_bits, alignment, patch_handler
  region_check_open_align \align_bits, \alignment, \patch_handler
  open_load32_core
.endm

.macro open_load32_align32 align_bits, alignment, patch_handler
  open_load32_align \align_bits, \alignment, \patch_handler
.endm


.macro store_function function, region, patch_handler, mask
  region_check \region, \patch_handler
  sw $ra, REG_SAVE($16)           # save the return address (delay)

  save_registers                  # save the registers

  jal \function                   # store value out
  andi $4, $4, \mask              # mask address (delay)

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
  nop
.endm


.macro store_function_a function, region, patch_handler, mask
  region_check \region, \patch_handler
  sw $ra, REG_SAVE($16)           # save the return address (delay)

  save_registers                  # save the registers

  jal \function                   # store value out
  andi $4, $4, \mask              # mask address (delay)

  restore_registers               # restore the other registers

  lw $ra, REG_SAVE($16)           # restore return address
  jr $ra                          # return
  nop
.endm



.macro load_u8 base
  jr $ra                          # return
  lbu $2, %lo(\base)($2)          # return base[offset]
.endm

.macro load_s8 base
  jr $ra                          # return
  lb $2, %lo(\base)($2)           # return base[offset]
.endm

.macro load_u16 base
  jr $ra                          # return
  lhu $2, %lo(\base)($2)          # return base[offset]
.endm

.macro load_s16 base
  jr $ra                          # return
  lh $2, %lo(\base)($2)           # return base[offset]
.endm

.macro load_u32 base
  jr $ra                          # return
  lw $2, %lo(\base)($2)           # return base[offset]
.endm


# 16bit unaligned load will always have a 1 in the LSB;
# should have already been taken care of in indexing.

.macro load_u16_unaligned base
  lhu $2, %lo(\base)($2)          # load base[offset]
  jr $ra                          # return
  ror $2, $2, 8                   # rotate value by 8bits
.endm

# This is technically the same as load_s8, but kept to
# avoid confusion.

.macro load_s16_unaligned base
  jr $ra                          # return
  lb $2, %lo(\base)($2)           # return base[offset]
.endm

# Unalignment must be known statically (use the tables to
# patch correctly)

.macro load_u32_unaligned base, alignment
  lw $2, %lo(\base)($2)           # load base[offset]
  jr $ra                          # return
  ror $2, $2, (\alignment * 8)    # rotate value by 8bits
.endm


.macro store_u8 base
  jr $ra                          # return
  sb $5, %lo(\base)($2)           # store value at base[offset]
.endm

.macro store_u16 base
  jr $ra                          # return
  sh $5, %lo(\base)($2)           # store value at base[offset]
.endm

.macro store_u32 base
  jr $ra                          # return
  sw $5, %lo(\base)($2)           # store value at base[offset]
.endm


# Store the value double mirrored (u16)

.macro store_u8_double base
  ins $5, $5, 8, 8                # value = (value << 8) | value
  jr $ra                          # return
  sh $5, %lo(\base)($2)           # store value at base[offset]
.endm


# Store the values and check if it overwrote code there

.macro store_u8_smc_i base
  addiu $2, $2, %lo(\base)        # offset the address
  lb $1, -32768($2)               # load the SMC status
  bne $1, $0, smc_write_i           # is there code there?
  sb $5, ($2)                     # store value at base[offset] (delay)
  jr $ra                          # return
  nop
.endm

.macro store_u16_smc_i base
  addiu $2, $2, %lo(\base)        # offset the address
  lh $1, -32768($2)               # load the SMC status
  bne $1, $0, smc_write_i           # is there code there?
  sh $5, ($2)                     # store value at base[offset] (delay)
  jr $ra                          # return
  nop
.endm

.macro store_u32_smc_i base
  addiu $2, $2, %lo(\base)        # offset the address
  lw $1, -32768($2)               # load the SMC status
  bne $1, $0, smc_write_i           # is there code there?
  sw $5, ($2)                     # store value at base[offset] (delay)
  jr $ra                          # return
  nop
.endm

.macro store_u8_smc_e base
  addiu $2, $2, %lo(\base)        # offset the address
  lb $1, -32768($2)               # load the SMC status
  bne $1, $0, smc_write_e           # is there code there?
  sb $5, ($2)                     # store value at base[offset] (delay)
  jr $ra                          # return
  nop
.endm

.macro store_u16_smc_e base
  addiu $2, $2, %lo(\base)        # offset the address
  lh $1, -32768($2)               # load the SMC status
  bne $1, $0, smc_write_e           # is there code there?
  sh $5, ($2)                     # store value at base[offset] (delay)
  jr $ra                          # return
  nop
.endm

.macro store_u32_smc_e base
  addiu $2, $2, %lo(\base)        # offset the address
  lw $1, -32768($2)               # load the SMC status
  bne $1, $0, smc_write_e           # is there code there?
  sw $5, ($2)                     # store value at base[offset] (delay)
  jr $ra                          # return
  nop
.endm


# Unsigned 8bit load handlers

execute_load_bios_u8:
  region_check 0, patch_load_u8
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFF             # generate offset
  addu $2, $2, $4
  load_u8 bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  ins $2, $4, 0, 2                # lower 2 bits address contributes
  load_u8 bios_read_protect

2:
  open_load8_core
  nop


execute_load_ewram_u8:
  translate_region_ewram patch_load_u8
  load_u8 (ewram + 0x8000)

# Put the generic address over the handler you want to be default
# IWRAM is typically the most frequently read and written to.

execute_load_u8:
execute_load_iwram_u8:
  translate_region 3, patch_load_u8, (iwram + 0x8000), 0x7FFF
  load_u8 (iwram + 0x8000)

execute_load_io_u8:
  translate_region 4, patch_load_u8, io_registers, 0x3FF
  load_u8 io_registers

execute_load_palette_u8:
  translate_region 5, patch_load_u8, palette_ram, 0x3FF
  load_u8 palette_ram

execute_load_vram_u8:
  translate_region_vram patch_load_u8
  load_u8 vram

execute_load_oam_u8:
  translate_region 7, patch_load_u8, oam_ram, 0x3FF
  load_u8 oam_ram

execute_load_gamepak8_u8:
  translate_region_gamepak 8, patch_load_u8
  load_u8 0

execute_load_gamepak9_u8:
  translate_region_gamepak 9, patch_load_u8
  load_u8 0

execute_load_gamepakA_u8:
  translate_region_gamepak 10, patch_load_u8
  load_u8 0

execute_load_gamepakB_u8:
  translate_region_gamepak 11, patch_load_u8
  load_u8 0

execute_load_gamepakC_u8:
  translate_region_gamepak 12, patch_load_u8
  load_u8 0

execute_load_eeprom_u8:
  eeprom_load patch_load_u8

execute_load_backup_u8:
  backup_load patch_load_u8
  nop

execute_load_open_u8:
  open_load8 patch_load_u8
  nop

load_u8_ftable:
  .long execute_load_bios_u8      # 0x00 BIOS
  .long execute_load_open_u8      # 0x01 open address
  .long execute_load_ewram_u8     # 0x02 EWRAM
  .long execute_load_iwram_u8     # 0x03 IWRAM
  .long execute_load_io_u8        # 0x04 I/O registers
  .long execute_load_palette_u8   # 0x05 Palette RAM
  .long execute_load_vram_u8      # 0x06 VRAM
  .long execute_load_oam_u8       # 0x07 OAM RAM
  .long execute_load_gamepak8_u8  # 0x08 gamepak
  .long execute_load_gamepak9_u8  # 0x09 gamepak
  .long execute_load_gamepakA_u8  # 0x0A gamepak
  .long execute_load_gamepakB_u8  # 0x0B gamepak
  .long execute_load_gamepakC_u8  # 0x0C gamepak
  .long execute_load_eeprom_u8    # 0x0D gamepak/eeprom
  .long execute_load_backup_u8    # 0x0E Flash ROM/SRAM
  .long execute_load_open_u8      # 0x0F open address

patch_load_u8:
  patch_handler load_u8_ftable, 0x01



# Signed 8bit load handlers

execute_load_bios_s8:
  region_check 0, patch_load_s8
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFF             # generate offset
  addu $2, $2, $4
  load_s8 bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  ins $2, $4, 0, 2                # lower 2 bits contribute
  load_s8 bios_read_protect

2:
  open_load8_core
  seb $2, $2


execute_load_ewram_s8:
  translate_region_ewram patch_load_s8
  load_s8 (ewram + 0x8000)

execute_load_s8:
execute_load_iwram_s8:
  translate_region 3, patch_load_s8, (iwram + 0x8000), 0x7FFF
  load_s8 (iwram + 0x8000)

execute_load_io_s8:
  translate_region 4, patch_load_s8, io_registers, 0x3FF
  load_s8 io_registers

execute_load_palette_s8:
  translate_region 5, patch_load_s8, palette_ram, 0x3FF
  load_s8 palette_ram

execute_load_vram_s8:
  translate_region_vram patch_load_s8
  load_s8 vram

execute_load_oam_s8:
  translate_region 7, patch_load_s8, oam_ram, 0x3FF
  load_s8 oam_ram

execute_load_gamepak8_s8:
  translate_region_gamepak 8, patch_load_s8
  load_s8 0

execute_load_gamepak9_s8:
  translate_region_gamepak 9, patch_load_s8
  load_s8 0

execute_load_gamepakA_s8:
  translate_region_gamepak 10, patch_load_s8
  load_s8 0

execute_load_gamepakB_s8:
  translate_region_gamepak 11, patch_load_s8
  load_s8 0

execute_load_gamepakC_s8:
  translate_region_gamepak 12, patch_load_s8
  load_s8 0

execute_load_eeprom_s8:
  eeprom_load patch_load_s8

execute_load_backup_s8:
  backup_load patch_load_s8
  seb $2, $2                      # sign extend result (delay)

execute_load_open_s8:
  open_load8 patch_load_s8
  seb $2, $2                      # sign extend result (delay)

load_s8_ftable:
  .long execute_load_bios_s8      # 0x00 BIOS
  .long execute_load_open_s8      # 0x01 open address
  .long execute_load_ewram_s8     # 0x02 EWRAM
  .long execute_load_iwram_s8     # 0x03 IWRAM
  .long execute_load_io_s8        # 0x04 I/O registers
  .long execute_load_palette_s8   # 0x05 Palette RAM
  .long execute_load_vram_s8      # 0x06 VRAM
  .long execute_load_oam_s8       # 0x07 OAM RAM
  .long execute_load_gamepak8_s8  # 0x08 gamepak
  .long execute_load_gamepak9_s8  # 0x09 gamepak
  .long execute_load_gamepakA_s8  # 0x0A gamepak
  .long execute_load_gamepakB_s8  # 0x0B gamepak
  .long execute_load_gamepakC_s8  # 0x0C gamepak
  .long execute_load_eeprom_s8    # 0x0D gamepak/eeprom
  .long execute_load_backup_s8    # 0x0E Flash ROM/SRAM
  .long execute_load_open_s8      # 0x0F open address

patch_load_s8:
  patch_handler load_s8_ftable, 1



# Unsigned aligned 16bit load handlers

execute_load_bios_u16:
  region_check_align 0, 1, 0, patch_load_u16
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFF             # generate offset
  addu $2, $2, $4
  load_u16 bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  ins $2, $4, 0, 2                # bit 1 contributes
  load_u16 bios_read_protect

2:
  open_load16_core
  nop

execute_load_ewram_u16:
  translate_region_ewram_load_align 1, 0, patch_load_u16
  load_u16 (ewram + 0x8000)

execute_load_u16:
execute_load_iwram_u16:
  translate_region_align 3, 1, 0, patch_load_u16, (iwram + 0x8000), 0x7FFF
  load_u16 (iwram + 0x8000)

execute_load_io_u16:
  translate_region_align 4, 1, 0, patch_load_u16, io_registers, 0x3FF
  load_u16 io_registers

execute_load_palette_u16:
  translate_region_align 5, 1, 0, patch_load_u16, palette_ram, 0x3FF
  load_u16 palette_ram

execute_load_vram_u16:
  translate_region_vram_load_align 1, 0, patch_load_u16
  load_u16 vram

execute_load_oam_u16:
  translate_region_align 7, 1, 0, patch_load_u16, oam_ram, 0x3FF
  load_u16 oam_ram

execute_load_gamepak8_u16:
  translate_region_gamepak_align 8, 1, 0, patch_load_u16
  load_u16 0

execute_load_gamepak9_u16:
  translate_region_gamepak_align 9, 1, 0, patch_load_u16
  load_u16 0

execute_load_gamepakA_u16:
  translate_region_gamepak_align 10, 1, 0, patch_load_u16
  load_u16 0

execute_load_gamepakB_u16:
  translate_region_gamepak_align 11, 1, 0, patch_load_u16
  load_u16 0

execute_load_gamepakC_u16:
  translate_region_gamepak_align 12, 1, 0, patch_load_u16
  load_u16 0

execute_load_eeprom_u16:
  eeprom_load_align 1, 0, patch_load_u16

execute_load_backup_u16:
  backup_load_align 1, 0, patch_load_u16
  nop

execute_load_open_u16:
  open_load16_align 1, 0, patch_load_u16
  nop


# Unsigned unaligned 16bit load handlers

execute_load_bios_u16u:
  region_check_align 0, 1, 1, patch_load_u16
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFE             # generate offset
  addu $2, $2, $4
  load_u16_unaligned bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  ext $1, $4, 1, 1
  ins $2, $1, 1, 1                # bit 1 contributes
  load_u16_unaligned bios_read_protect

2:
  open_load16_core
  ror $2, $2, 8


execute_load_ewram_u16u:
  translate_region_ewram_load_align16 1, 1, patch_load_u16
  load_u16_unaligned (ewram + 0x8000)

execute_load_iwram_u16u:
  translate_region_align 3, 1, 1, patch_load_u16, (iwram + 0x8000), 0x7FFE
  load_u16_unaligned (iwram + 0x8000)

execute_load_io_u16u:
  translate_region_align 4, 1, 1, patch_load_u16, io_registers, 0x3FE
  load_u16_unaligned io_registers

execute_load_palette_u16u:
  translate_region_align 5, 1, 1, patch_load_u16, palette_ram, 0x3FE
  load_u16_unaligned palette_ram

execute_load_vram_u16u:
  translate_region_vram_load_align16 1, 1, patch_load_u16
  load_u16_unaligned vram

execute_load_oam_u16u:
  translate_region_align 7, 1, 1, patch_load_u16, oam_ram, 0x3FE
  load_u16_unaligned oam_ram

execute_load_gamepak8_u16u:
  translate_region_gamepak_align16 8, 1, 1, patch_load_u16
  load_u16_unaligned 0

execute_load_gamepak9_u16u:
  translate_region_gamepak_align16 9, 1, 1, patch_load_u16
  load_u16_unaligned 0

execute_load_gamepakA_u16u:
  translate_region_gamepak_align16 10, 1, 1, patch_load_u16
  load_u16_unaligned 0

execute_load_gamepakB_u16u:
  translate_region_gamepak_align16 11, 1, 1, patch_load_u16
  load_u16_unaligned 0

execute_load_gamepakC_u16u:
  translate_region_gamepak_align16 12, 1, 1, patch_load_u16
  load_u16_unaligned 0

execute_load_eeprom_u16u:
  eeprom_load_align16 1, 1, patch_load_u16

execute_load_backup_u16u:
  backup_load_align16 1, 1, patch_load_u16
  ror $2, $2, 8                   # rotate value by 8bits

execute_load_open_u16u:
  open_load16_align16 1, 1, patch_load_u16
  ror $2, $2, 8                   # rotate value by 8bits

load_u16_ftable:
#  .long execute_load_full_u16
  .long execute_load_bios_u16     # 0x00 BIOS
  .long execute_load_open_u16     # 0x01 open address
  .long execute_load_ewram_u16    # 0x02 EWRAM
  .long execute_load_iwram_u16    # 0x03 IWRAM
  .long execute_load_io_u16       # 0x04 I/O registers
  .long execute_load_palette_u16  # 0x05 Palette RAM
  .long execute_load_vram_u16     # 0x06 VRAM
  .long execute_load_oam_u16      # 0x07 OAM RAM
  .long execute_load_gamepak8_u16 # 0x08 gamepak
  .long execute_load_gamepak9_u16 # 0x09 gamepak
  .long execute_load_gamepakA_u16 # 0x0A gamepak
  .long execute_load_gamepakB_u16 # 0x0B gamepak
  .long execute_load_gamepakC_u16 # 0x0C gamepak
  .long execute_load_eeprom_u16   # 0x0D gamepak/eeprom
  .long execute_load_backup_u16   # 0x0E Flash ROM/SRAM
  .long execute_load_open_u16     # 0x0F open

  .long execute_load_bios_u16u    # 0x00 BIOS unaligned
  .long execute_load_open_u16u    # 0x01 open address unaligned
  .long execute_load_ewram_u16u   # 0x02 EWRAM unaligned
  .long execute_load_iwram_u16u   # 0x03 IWRAM unaligned
  .long execute_load_io_u16u      # 0x04 I/O registers unaligned
  .long execute_load_palette_u16u # 0x05 Palette RAM unaligned
  .long execute_load_vram_u16u    # 0x06 VRAM unaligned
  .long execute_load_oam_u16u     # 0x07 OAM RAM unaligned
  .long execute_load_gamepak8_u16u# 0x08 gamepak unaligned
  .long execute_load_gamepak9_u16u# 0x09 gamepak unaligned
  .long execute_load_gamepakA_u16u# 0x0A gamepak unaligned
  .long execute_load_gamepakB_u16u# 0x0B gamepak unaligned
  .long execute_load_gamepakC_u16u# 0x0C gamepak unaligned
  .long execute_load_eeprom_u16u  # 0x0D gamepak/eeprom unaligned
  .long execute_load_backup_u16u  # 0x0E Flash ROM/SRAM unaligned
  .long execute_load_open_u16u    # 0x0F open unaligned


  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16

  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16
  .long execute_load_full_u16



patch_load_u16:
  patch_handler_align load_u16_ftable, 1

# Signed aligned 16bit load handlers

execute_load_bios_s16:
  region_check_align 0, 1, 0, patch_load_s16
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFF             # generate offset
  addu $2, $2, $4
  load_s16 bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  ins $2, $4, 0, 2                # bit 1 contributes
  load_s16 bios_read_protect

2:
  open_load16_core
  seh $2, $2


execute_load_ewram_s16:
  translate_region_ewram_load_align 1, 0, patch_load_s16
  load_s16 (ewram + 0x8000)

execute_load_s16:
execute_load_iwram_s16:
  translate_region_align 3, 1, 0, patch_load_s16, (iwram + 0x8000), 0x7FFF
  load_s16 (iwram + 0x8000)

execute_load_io_s16:
  translate_region_align 4, 1, 0, patch_load_s16, io_registers, 0x3FF
  load_s16 io_registers

execute_load_palette_s16:
  translate_region_align 5, 1, 0, patch_load_s16, palette_ram, 0x3FF
  load_s16 palette_ram

execute_load_vram_s16:
  translate_region_vram_load_align 1, 0, patch_load_s16
  load_s16 vram

execute_load_oam_s16:
  translate_region_align 7, 1, 0, patch_load_s16, oam_ram, 0x3FF
  load_s16 oam_ram

execute_load_gamepak8_s16:
  translate_region_gamepak_align 8, 1, 0, patch_load_s16
  load_s16 0

execute_load_gamepak9_s16:
  translate_region_gamepak_align 9, 1, 0, patch_load_s16
  load_s16 0

execute_load_gamepakA_s16:
  translate_region_gamepak_align 10, 1, 0, patch_load_s16
  load_s16 0

execute_load_gamepakB_s16:
  translate_region_gamepak_align 11, 1, 0, patch_load_s16
  load_s16 0

execute_load_gamepakC_s16:
  translate_region_gamepak_align 12, 1, 0, patch_load_s16
  load_s16 0

execute_load_eeprom_s16:
  eeprom_load_align 1, 0, patch_load_s16

execute_load_backup_s16:
  backup_load_align 1, 0, patch_load_s16
  nop

execute_load_open_s16:
  open_load16_align 1, 0, patch_load_s16
  nop


# Signed unaligned 16bit load handlers

execute_load_bios_s16u:
  region_check_align 0, 1, 1, patch_load_s16
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFE             # generate offset
  addu $2, $2, $4
  load_s16_unaligned bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  ext $1, $4, 1, 1
  ins $2, $1, 1, 1                # bit 1 contributes
  load_s16_unaligned bios_read_protect

2:
  open_load16_core
  seb $2, $2

execute_load_ewram_s16u:
  translate_region_ewram_load_align16 1, 1, patch_load_s16
  load_s16_unaligned (ewram + 0x8000)

execute_load_iwram_s16u:
  translate_region_align 3, 1, 1, patch_load_s16, (iwram + 0x8000), 0x7FFE
  load_s16_unaligned (iwram + 0x8000)

execute_load_io_s16u:
  translate_region_align 4, 1, 1, patch_load_s16, io_registers, 0x3FE
  load_s16_unaligned io_registers

execute_load_palette_s16u:
  translate_region_align 5, 1, 1, patch_load_s16, palette_ram, 0x3FE
  load_s16_unaligned palette_ram

execute_load_vram_s16u:
  translate_region_vram_load_align16 1, 1, patch_load_s16
  load_s16_unaligned vram

execute_load_oam_s16u:
  translate_region_align 7, 1, 1, patch_load_s16, oam_ram, 0x3FE
  load_s16_unaligned oam_ram

execute_load_gamepak8_s16u:
  translate_region_gamepak_align16 8, 1, 1, patch_load_s16
  load_s16_unaligned 0

execute_load_gamepak9_s16u:
  translate_region_gamepak_align16 9, 1, 1, patch_load_s16
  load_s16_unaligned 0

execute_load_gamepakA_s16u:
  translate_region_gamepak_align16 10, 1, 1, patch_load_s16
  load_s16_unaligned 0

execute_load_gamepakB_s16u:
  translate_region_gamepak_align16 11, 1, 1, patch_load_s16
  load_s16_unaligned 0

execute_load_gamepakC_s16u:
  translate_region_gamepak_align16 12, 1, 1, patch_load_s16
  load_s16_unaligned 0

execute_load_eeprom_s16u:
  eeprom_load_align 1, 1, patch_load_s16

execute_load_backup_s16u:
  backup_load_align 1, 1, patch_load_s16
  seb $2, $2                      # sign extend result from 8bits

execute_load_open_s16u:
  open_load16_align 1, 1, patch_load_s16
  seb $2, $2                      # sign extend result from 8bits

load_s16_ftable:
  .long execute_load_bios_s16     # 0x00 BIOS
  .long execute_load_open_s16     # 0x01 open address
  .long execute_load_ewram_s16    # 0x02 EWRAM
  .long execute_load_iwram_s16    # 0x03 IWRAM
  .long execute_load_io_s16       # 0x04 I/O registers
  .long execute_load_palette_s16  # 0x05 Palette RAM
  .long execute_load_vram_s16     # 0x06 VRAM
  .long execute_load_oam_s16      # 0x07 OAM RAM
  .long execute_load_gamepak8_s16 # 0x08 gamepak
  .long execute_load_gamepak9_s16 # 0x09 gamepak
  .long execute_load_gamepakA_s16 # 0x0A gamepak
  .long execute_load_gamepakB_s16 # 0x0B gamepak
  .long execute_load_gamepakC_s16 # 0x0C gamepak
  .long execute_load_eeprom_s16   # 0x0D gamepak/eeprom
  .long execute_load_backup_s16   # 0x0E Flash ROM/SRAM
  .long execute_load_open_s16     # 0x0F open unaligned

  .long execute_load_bios_s16u    # 0x00 BIOS unaligned
  .long execute_load_open_s16u    # 0x01 open address unaligned
  .long execute_load_ewram_s16u   # 0x02 EWRAM unaligned
  .long execute_load_iwram_s16u   # 0x03 IWRAM unaligned
  .long execute_load_io_s16u      # 0x04 I/O registers unaligned
  .long execute_load_palette_s16u # 0x05 Palette RAM unaligned
  .long execute_load_vram_s16u    # 0x06 VRAM unaligned
  .long execute_load_oam_s16u     # 0x07 OAM RAM unaligned
  .long execute_load_gamepak8_s16u# 0x08 gamepak unaligned
  .long execute_load_gamepak9_s16u# 0x09 gamepak unaligned
  .long execute_load_gamepakA_s16u# 0x0A gamepak unaligned
  .long execute_load_gamepakB_s16u# 0x0B gamepak unaligned
  .long execute_load_gamepakC_s16u# 0x0C gamepak unaligned
  .long execute_load_eeprom_s16u  # 0x0D gamepak/eeprom unaligned
  .long execute_load_backup_s16u  # 0x0E Flash ROM/SRAM unaligned
  .long execute_load_open_s16u    # 0x0F open unaligned

patch_load_s16:
  patch_handler_align load_s16_ftable, 1



# Unsigned aligned 32bit load handlers

execute_load_bios_u32:
  region_check_align 0, 2, 0, patch_load_u32
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFF             # generate offset
  addu $2, $2, $4
  load_u32 bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  load_u32 bios_read_protect

2:
  open_load32_core
  nop


execute_load_ewram_u32:
  translate_region_ewram_load_align 2, 0, patch_load_u32
  load_u32 (ewram + 0x8000)

execute_load_u32:
execute_load_iwram_u32:
  translate_region_align 3, 2, 0, patch_load_u32, (iwram + 0x8000), 0x7FFF
  load_u32 (iwram + 0x8000)

execute_load_io_u32:
  translate_region_align 4, 2, 0, patch_load_u32, io_registers, 0x3FF
  load_u32 io_registers

execute_load_palette_u32:
  translate_region_align 5, 2, 0, patch_load_u32, palette_ram, 0x3FF
  load_u32 palette_ram

execute_load_vram_u32:
  translate_region_vram_load_align 2, 0, patch_load_u32
  load_u32 vram

execute_load_oam_u32:
  translate_region_align 7, 2, 0, patch_load_u32, oam_ram, 0x3FF
  load_u32 oam_ram

execute_load_gamepak8_u32:
  translate_region_gamepak_align 8, 2, 0, patch_load_u32
  load_u32 0

execute_load_gamepak9_u32:
  translate_region_gamepak_align 9, 2, 0, patch_load_u32
  load_u32 0

execute_load_gamepakA_u32:
  translate_region_gamepak_align 10, 2, 0, patch_load_u32
  load_u32 0

execute_load_gamepakB_u32:
  translate_region_gamepak_align 11, 2, 0, patch_load_u32
  load_u32 0

execute_load_gamepakC_u32:
  translate_region_gamepak_align 12, 2, 0, patch_load_u32
  load_u32 0

execute_load_eeprom_u32:
  eeprom_load_align 2, 0, patch_load_u32

execute_load_backup_u32:
  backup_load_align 2, 0, patch_load_u32
  nop

execute_load_open_u32:
  open_load32_align 2, 0, patch_load_u32
  nop


# Unsigned unaligned (by 1) 32bit load handlers

execute_load_bios_u32u1:
  region_check_align 0, 2, 1, patch_load_u32
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFC             # generate offset
  addu $2, $2, $4
  load_u32_unaligned bios_rom, 1

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  load_u32_unaligned bios_read_protect, 1

2:
  open_load32_core
  ror $2, $2, 8

execute_load_ewram_u32u1:
  translate_region_ewram_load_align32 2, 1, patch_load_u32
  load_u32_unaligned (ewram + 0x8000), 1

execute_load_iwram_u32u1:
  translate_region_align 3, 2, 1, patch_load_u32, (iwram + 0x8000), 0x7FFC
  load_u32_unaligned (iwram + 0x8000), 1

execute_load_io_u32u1:
  translate_region_align 4, 2, 1, patch_load_u32, io_registers, 0x3FC
  load_u32_unaligned io_registers, 1

execute_load_palette_u32u1:
  translate_region_align 5, 2, 1, patch_load_u32, palette_ram, 0x3FC
  load_u32_unaligned palette_ram, 1

execute_load_vram_u32u1:
  translate_region_vram_load_align32 2, 1, patch_load_u32
  load_u32_unaligned vram, 1

execute_load_oam_u32u1:
  translate_region_align 7, 2, 1, patch_load_u32, oam_ram, 0x3FC
  load_u32_unaligned oam_ram, 1

execute_load_gamepak8_u32u1:
  translate_region_gamepak_align32 8, 2, 1, patch_load_u32
  load_u32_unaligned 0, 1

execute_load_gamepak9_u32u1:
  translate_region_gamepak_align32 9, 2, 1, patch_load_u32
  load_u32_unaligned 0, 1

execute_load_gamepakA_u32u1:
  translate_region_gamepak_align32 10, 2, 1, patch_load_u32
  load_u32_unaligned 0, 1

execute_load_gamepakB_u32u1:
  translate_region_gamepak_align32 11, 2, 1, patch_load_u32
  load_u32_unaligned 0, 1

execute_load_gamepakC_u32u1:
  translate_region_gamepak_align32 12, 2, 1, patch_load_u32
  load_u32_unaligned 0, 1

execute_load_eeprom_u32u1:
  eeprom_load_align32 2, 1, patch_load_u32

execute_load_backup_u32u1:
  backup_load_align32 2, 1, patch_load_u32
  ror $2, $2, 8                   # rotate value by 8bits

execute_load_open_u32u1:
  open_load32_align32 2, 1, patch_load_u32
  ror $2, $2, 8                   # rotate value by 8bits


# Unsigned unaligned (by 2) 32bit load handlers

execute_load_bios_u32u2:
  region_check_align 0, 2, 2, patch_load_u32
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFC             # generate offset
  addu $2, $2, $4
  load_u32_unaligned bios_rom, 2

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  load_u32_unaligned bios_read_protect, 2

2:
  open_load32_core
  ror $2, $2, 16

execute_load_ewram_u32u2:
  translate_region_ewram_load_align32 2, 2, patch_load_u32
  load_u32_unaligned (ewram + 0x8000), 2

execute_load_iwram_u32u2:
  translate_region_align 3, 2, 2, patch_load_u32, (iwram + 0x8000), 0x7FFC
  load_u32_unaligned (iwram + 0x8000), 2

execute_load_io_u32u2:
  translate_region_align 4, 2, 2, patch_load_u32, io_registers, 0x3FC
  load_u32_unaligned io_registers, 2

execute_load_palette_u32u2:
  translate_region_align 5, 2, 2, patch_load_u32, palette_ram, 0x3FC
  load_u32_unaligned palette_ram, 2

execute_load_vram_u32u2:
  translate_region_vram_load_align32 2, 2, patch_load_u32
  load_u32_unaligned vram, 2

execute_load_oam_u32u2:
  translate_region_align 7, 2, 2, patch_load_u32, oam_ram, 0x3FC
  load_u32_unaligned oam_ram, 2

execute_load_gamepak8_u32u2:
  translate_region_gamepak_align32 8, 2, 2, patch_load_u32
  load_u32_unaligned 0, 2

execute_load_gamepak9_u32u2:
  translate_region_gamepak_align32 9, 2, 2, patch_load_u32
  load_u32_unaligned 0, 2

execute_load_gamepakA_u32u2:
  translate_region_gamepak_align32 10, 2, 2, patch_load_u32
  load_u32_unaligned 0, 2

execute_load_gamepakB_u32u2:
  translate_region_gamepak_align32 11, 2, 2, patch_load_u32
  load_u32_unaligned 0, 2

execute_load_gamepakC_u32u2:
  translate_region_gamepak_align32 12, 2, 2, patch_load_u32
  load_u32_unaligned 0, 2

execute_load_eeprom_u32u2:
  eeprom_load_align32 2, 2, patch_load_u32

execute_load_backup_u32u2:
  backup_load_align32 2, 2, patch_load_u32
  ror $2, $2, 16                   # rotate value by 16bits

execute_load_open_u32u2:
  open_load32_align32 2, 2, patch_load_u32
  ror $2, $2, 16                   # rotate value by 16bits

# Unsigned unaligned (by 1) 32bit load handlers

execute_load_bios_u32u3:
  region_check_align 0, 2, 3, patch_load_u32
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFC             # generate offset
  addu $2, $2, $4
  load_u32_unaligned bios_rom, 3

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  load_u32_unaligned bios_read_protect, 3

2:
  open_load32_core
  ror $2, $2, 24

execute_load_ewram_u32u3:
  translate_region_ewram_load_align32 2, 3, patch_load_u32
  load_u32_unaligned (ewram + 0x8000), 3

execute_load_iwram_u32u3:
  translate_region_align 3, 2, 3, patch_load_u32, (iwram + 0x8000), 0x7FFC
  load_u32_unaligned (iwram + 0x8000), 3

execute_load_io_u32u3:
  translate_region_align 4, 2, 3, patch_load_u32, io_registers, 0x3FC
  load_u32_unaligned io_registers, 3

execute_load_palette_u32u3:
  translate_region_align 5, 2, 3, patch_load_u32, palette_ram, 0x3FC
  load_u32_unaligned palette_ram, 3

execute_load_vram_u32u3:
  translate_region_vram_load_align32 2, 3, patch_load_u32
  load_u32_unaligned vram, 3

execute_load_oam_u32u3:
  translate_region_align 7, 2, 3, patch_load_u32, oam_ram, 0x3FC
  load_u32_unaligned oam_ram, 3

execute_load_gamepak8_u32u3:
  translate_region_gamepak_align32 8, 2, 3, patch_load_u32
  load_u32_unaligned 0, 3

execute_load_gamepak9_u32u3:
  translate_region_gamepak_align32 9, 2, 3, patch_load_u32
  load_u32_unaligned 0, 3

execute_load_gamepakA_u32u3:
  translate_region_gamepak_align32 10, 2, 3, patch_load_u32
  load_u32_unaligned 0, 3

execute_load_gamepakB_u32u3:
  translate_region_gamepak_align32 11, 2, 3, patch_load_u32
  load_u32_unaligned 0, 3

execute_load_gamepakC_u32u3:
  translate_region_gamepak_align32 12, 2, 3, patch_load_u32
  load_u32_unaligned 0, 3

execute_load_eeprom_u32u3:
  eeprom_load_align32 2, 3, patch_load_u32

execute_load_backup_u32u3:
  backup_load_align32 2, 3, patch_load_u32
  ror $2, $2, 24                  # rotate value by 24bits

execute_load_open_u32u3:
  open_load32_align32 2, 3, patch_load_u32
  ror $2, $2, 24                  # rotate value by 24bits


load_u32_ftable:
  .long execute_load_bios_u32     # 0x00 BIOS
  .long execute_load_open_u32     # 0x01 open address
  .long execute_load_ewram_u32    # 0x02 EWRAM
  .long execute_load_iwram_u32    # 0x03 IWRAM
  .long execute_load_io_u32       # 0x04 I/O registers
  .long execute_load_palette_u32  # 0x05 Palette RAM
  .long execute_load_vram_u32     # 0x06 VRAM
  .long execute_load_oam_u32      # 0x07 OAM RAM
  .long execute_load_gamepak8_u32 # 0x08 gamepak
  .long execute_load_gamepak9_u32 # 0x09 gamepak
  .long execute_load_gamepakA_u32 # 0x0A gamepak
  .long execute_load_gamepakB_u32 # 0x0B gamepak
  .long execute_load_gamepakC_u32 # 0x0C gamepak
  .long execute_load_eeprom_u32   # 0x0D gamepak/eeprom
  .long execute_load_backup_u32   # 0x0E Flash ROM/SRAM
  .long execute_load_open_u32     # 0x0F open

  .long execute_load_bios_u32u1     # 0x00 BIOS unaligned (1b)
  .long execute_load_open_u32u1     # 0x01 open address unaligned (1b)
  .long execute_load_ewram_u32u1    # 0x02 EWRAM unaligned (1b)
  .long execute_load_iwram_u32u1    # 0x03 IWRAM unaligned (1b)
  .long execute_load_io_u32u1       # 0x04 I/O registers unaligned (1b)
  .long execute_load_palette_u32u1  # 0x05 Palette RAM unaligned (1b)
  .long execute_load_vram_u32u1     # 0x06 VRAM unaligned (1b)
  .long execute_load_oam_u32u1      # 0x07 OAM RAM unaligned (1b)
  .long execute_load_gamepak8_u32u1 # 0x08 gamepak unaligned (1b)
  .long execute_load_gamepak9_u32u1 # 0x09 gamepak unaligned (1b)
  .long execute_load_gamepakA_u32u1 # 0x0A gamepak unaligned (1b)
  .long execute_load_gamepakB_u32u1 # 0x0B gamepak unaligned (1b)
  .long execute_load_gamepakC_u32u1 # 0x0C gamepak unaligned (1b)
  .long execute_load_eeprom_u32u1   # 0x0D gamepak/eeprom unaligned (1b)
  .long execute_load_backup_u32u1   # 0x0E Flash ROM/SRAM unaligned (1b)
  .long execute_load_open_u32u1     # 0x0F open unaligned (1b)

  .long execute_load_bios_u32u2     # 0x00 BIOS unaligned (2b)
  .long execute_load_open_u32u2     # 0x01 open address unaligned (2b)
  .long execute_load_ewram_u32u2    # 0x02 EWRAM unaligned (2b)
  .long execute_load_iwram_u32u2    # 0x03 IWRAM unaligned (2b)
  .long execute_load_io_u32u2       # 0x04 I/O registers unaligned (2b)
  .long execute_load_palette_u32u2  # 0x05 Palette RAM unaligned (2b)
  .long execute_load_vram_u32u2     # 0x06 VRAM unaligned (2b)
  .long execute_load_oam_u32u2      # 0x07 OAM RAM unaligned (2b)
  .long execute_load_gamepak8_u32u2 # 0x08 gamepak unaligned (2b)
  .long execute_load_gamepak9_u32u2 # 0x09 gamepak unaligned (2b)
  .long execute_load_gamepakA_u32u2 # 0x0A gamepak unaligned (2b)
  .long execute_load_gamepakB_u32u2 # 0x0B gamepak unaligned (2b)
  .long execute_load_gamepakC_u32u2 # 0x0C gamepak unaligned (2b)
  .long execute_load_eeprom_u32u2   # 0x0D gamepak/eeprom unaligned (2b)
  .long execute_load_backup_u32u2   # 0x0E Flash ROM/SRAM unaligned (2b)
  .long execute_load_open_u32u2     # 0x0F open unaligned (2b)

  .long execute_load_bios_u32u3     # 0x00 BIOS unaligned (3b)
  .long execute_load_open_u32u3     # 0x01 open address unaligned (3b)
  .long execute_load_ewram_u32u3    # 0x02 EWRAM unaligned (3b)
  .long execute_load_iwram_u32u3    # 0x03 IWRAM unaligned (3b)
  .long execute_load_io_u32u3       # 0x04 I/O registers unaligned (3b)
  .long execute_load_palette_u32u3  # 0x05 Palette RAM unaligned (3b)
  .long execute_load_vram_u32u3     # 0x06 VRAM unaligned (3b)
  .long execute_load_oam_u32u3      # 0x07 OAM RAM unaligned (3b)
  .long execute_load_gamepak8_u32u3 # 0x08 gamepak unaligned (3b)
  .long execute_load_gamepak9_u32u3 # 0x09 gamepak unaligned (3b)
  .long execute_load_gamepakA_u32u3 # 0x0A gamepak unaligned (3b)
  .long execute_load_gamepakB_u32u3 # 0x0B gamepak unaligned (3b)
  .long execute_load_gamepakC_u32u3 # 0x0C gamepak unaligned (3b)
  .long execute_load_eeprom_u32u3   # 0x0D gamepak/eeprom unaligned (3b)
  .long execute_load_backup_u32u3   # 0x0E Flash ROM/SRAM unaligned (3b)
  .long execute_load_open_u32u3     # 0x0F open unaligned (3b)

patch_load_u32:
  patch_handler_align load_u32_ftable, 2



# Unsigned always aligned 32bit load handlers

execute_load_bios_u32a:
  region_check 0, patch_load_u32a
  srl $2, $4, 14                  # check if address is in BIOS region (delay)
  bne $2, $0, 2f                  # if not, perform open read
  srl $1, $5, 14                  # check if PC is in BIOS region (delay)
  bne $1, $0, 1f                  # if not, perform BIOS protected read
  lui $2, %hi(bios_rom)           # generate upper address (delay)

  andi $4, $4, 0x3FFF             # generate offset
  addu $2, $2, $4
  load_u32 bios_rom

1:
  lui $2, %hi(bios_read_protect)  # generate upper address
  load_u32 bios_read_protect

2:
  open_load32_a
  nop

execute_load_ewram_u32a:
  translate_region_ewram patch_load_u32a
  load_u32 (ewram + 0x8000)

execute_aligned_load32:
execute_load_iwram_u32a:
  translate_region 3, patch_load_u32a, (iwram + 0x8000), 0x7FFF
  load_u32 (iwram + 0x8000)

execute_load_io_u32a:
  translate_region 4, patch_load_u32a, io_registers, 0x3FF
  load_u32 io_registers

execute_load_palette_u32a:
  translate_region 5, patch_load_u32a, palette_ram, 0x3FF
  load_u32 palette_ram

execute_load_vram_u32a:
  translate_region_vram patch_load_u32a
  load_u32 vram

execute_load_oam_u32a:
  translate_region 7, patch_load_u32a, oam_ram, 0x3FF
  load_u32 oam_ram

execute_load_gamepak8_u32a:
  translate_region_gamepak_a 8, patch_load_u32a
  load_u32 0

execute_load_gamepak9_u32a:
  translate_region_gamepak_a 9, patch_load_u32a
  load_u32 0

execute_load_gamepakA_u32a:
  translate_region_gamepak_a 10, patch_load_u32a
  load_u32 0

execute_load_gamepakB_u32a:
  translate_region_gamepak_a 11, patch_load_u32a
  load_u32 0

execute_load_gamepakC_u32a:
  translate_region_gamepak_a 12, patch_load_u32a
  load_u32 0

execute_load_eeprom_u32a:
  eeprom_load_a patch_load_u32a

execute_load_backup_u32a:
  backup_load_a patch_load_u32a
  nop

execute_load_open_u32a:
  open_load32_a patch_load_u32a

load_u32a_ftable:
  .long execute_load_bios_u32a      # 0x00 BIOS unaligned (3b)
  .long execute_load_open_u32a      # 0x01 open address unaligned (3b)
  .long execute_load_ewram_u32a     # 0x02 EWRAM unaligned (3b)
  .long execute_load_iwram_u32a     # 0x03 IWRAM unaligned (3b)
  .long execute_load_io_u32a        # 0x04 I/O registers unaligned (3b)
  .long execute_load_palette_u32a   # 0x05 Palette RAM unaligned (3b)
  .long execute_load_vram_u32a      # 0x06 VRAM unaligned (3b)
  .long execute_load_oam_u32a       # 0x07 OAM RAM unaligned (3b)
  .long execute_load_gamepak8_u32a  # 0x08 gamepak unaligned (3b)
  .long execute_load_gamepak9_u32a  # 0x09 gamepak unaligned (3b)
  .long execute_load_gamepakA_u32a  # 0x0A gamepak unaligned (3b)
  .long execute_load_gamepakB_u32a  # 0x0B gamepak unaligned (3b)
  .long execute_load_gamepakC_u32a  # 0x0C gamepak unaligned (3b)
  .long execute_load_eeprom_u32a    # 0x0D gamepak/eeprom unaligned (3b)
  .long execute_load_backup_u32a    # 0x0E Flash ROM/SRAM unaligned (3b)
  .long execute_load_open_u32a      # 0x0F open unaligned (3b)

patch_load_u32a:
  patch_handler load_u32a_ftable, 1


# Unsigned 8bit store handlers

execute_store_ignore0_u8:
  ignore_region 0, patch_store_u8

execute_store_ignore1_u8:
  ignore_region 1, patch_store_u8

execute_store_ewram_u8:
  translate_region_ewram patch_store_u8
  store_u8_smc_e (ewram + 0x8000)

execute_store_u8:
execute_store_iwram_u8:
  translate_region 3, patch_store_u8, (iwram + 0x8000), 0x7FFF
  store_u8_smc_i (iwram + 0x8000)

execute_store_io_u8:
  region_check 4, patch_store_u8
  andi $5, $5, 0xFF               # make value 8bit
  andi $4, $4, 0x3FF              # wrap around address
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)

  save_registers
  jal write_io_register8          # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  j write_io_epilogue             # handle any state changes
  nop

execute_store_palette_u8:
  region_check 5, patch_store_u8
  lui $2, %hi(palette_ram)        # start loading palette_ram address (delay)
  ins $5, $5, 8, 8                # double value
  andi $4, $4, 0x3FE              # align palette address
  addu $2, $2, $4

  jr $ra                          # return
  sh $5, %lo(palette_ram)($2)     # palette_ram[address] = value

execute_store_vram_u8:
  translate_region_vram_store_align16 patch_store_u8
  store_u8_double vram

execute_store_oam_u8:
  translate_region 7, patch_store_u8, oam_ram, 0x3FE
  lui $1, %hi(oam_update)         # write non-zero to oam_update
  sw $1, %lo(oam_update)($1)      # cheap, but the address is non-zero
  store_u8_double oam_ram

execute_store_ignore8_u8:
  ignore_region 8, patch_store_u8

execute_store_ignore9_u8:
  ignore_region 9, patch_store_u8

execute_store_ignoreA_u8:
  ignore_region 10, patch_store_u8

execute_store_ignoreB_u8:
  ignore_region 11, patch_store_u8

execute_store_ignoreC_u8:
  ignore_region 12, patch_store_u8

execute_store_eeprom_u8:
  store_function write_eeprom, 13, patch_store_u8, 0x3FF

execute_store_backup_u8:
  store_function write_backup, 14, patch_store_u8, 0xFFFF

execute_store_ignoreF_u8:
  ignore_high patch_store_u8

store_u8_ftable:
  .long execute_store_ignore0_u8  # 0x00 BIOS
  .long execute_store_ignore1_u8  # 0x01 open address
  .long execute_store_ewram_u8    # 0x02 EWRAM
  .long execute_store_iwram_u8    # 0x03 IWRAM
  .long execute_store_io_u8       # 0x04 I/O registers
  .long execute_store_palette_u8  # 0x05 Palette RAM
  .long execute_store_vram_u8     # 0x06 VRAM
  .long execute_store_oam_u8      # 0x07 OAM RAM
  .long execute_store_ignore8_u8  # 0x08 gamepak
  .long execute_store_ignore9_u8  # 0x09 gamepak
  .long execute_store_ignoreA_u8  # 0x0A gamepak
  .long execute_store_ignoreB_u8  # 0x0B gamepak
  .long execute_store_ignoreC_u8  # 0x0C gamepak
  .long execute_store_eeprom_u8   # 0x0D gamepak/eeprom
  .long execute_store_backup_u8   # 0x0E Flash ROM/SRAM
  .long execute_store_ignoreF_u8  # 0x0F open address

patch_store_u8:
  patch_handler store_u8_ftable, 0x0F


# Unsigned 16bit store handlers

execute_store_ignore0_u16:
  ignore_region 0, patch_store_u16

execute_store_ignore1_u16:
  ignore_region 1, patch_store_u16

execute_store_ewram_u16:
  translate_region_ewram_store_align16 patch_store_u16
  store_u16_smc_e (ewram + 0x8000)

execute_store_u16:
execute_store_iwram_u16:
  translate_region 3, patch_store_u16, (iwram + 0x8000), 0x7FFE
  store_u16_smc_i (iwram + 0x8000)

execute_store_io_u16:
  region_check 4, patch_store_u16
  andi $5, $5, 0xFFFF             # make value 16bit
  andi $4, $4, 0x3FE              # wrap around/align address
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)

  save_registers
  jal write_io_register16         # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  j write_io_epilogue             # handle any state changes
  nop

execute_store_palette_u16:
  region_check 5, patch_store_u16
  lui $2, %hi(palette_ram)        # start loading palette_ram address (delay)
  andi $4, $4, 0x3FE              # wrap/align palette address
  addu $2, $2, $4

  jr $ra                          # return
  sh $5, %lo(palette_ram)($2)     # palette_ram[address] = value

execute_store_vram_u16:
  translate_region_vram_store_align16 patch_store_u16
  store_u16 vram

execute_store_oam_u16:
  translate_region 7, patch_store_u16, oam_ram, 0x3FE
  lui $1, %hi(oam_update)         # write non-zero to oam_update
  sw $1, %lo(oam_update)($1)      # cheap, but the address is non-zero
  store_u16 oam_ram

execute_store_rtc_u16:
  store_function write_rtc, 8, patch_store_u16, 0xFE

execute_store_ignore9_u16:
  ignore_region 9, patch_store_u16

execute_store_ignoreA_u16:
  ignore_region 10, patch_store_u16

execute_store_ignoreB_u16:
  ignore_region 11, patch_store_u16

execute_store_ignoreC_u16:
  ignore_region 12, patch_store_u16

execute_store_eeprom_u16:
  store_function write_eeprom, 13, patch_store_u16, 0x3FE

execute_store_ignoreE_u16:
  ignore_region 14, patch_store_u16

execute_store_ignoreF_u16:
  ignore_high patch_store_u16

store_u16_ftable:
  .long execute_store_ignore0_u16 # 0x00 BIOS
  .long execute_store_ignore1_u16 # 0x01 open address
  .long execute_store_ewram_u16   # 0x02 EWRAM
  .long execute_store_iwram_u16   # 0x03 IWRAM
  .long execute_store_io_u16      # 0x04 I/O registers
  .long execute_store_palette_u16 # 0x05 Palette RAM
  .long execute_store_vram_u16    # 0x06 VRAM
  .long execute_store_oam_u16     # 0x07 OAM RAM
  .long execute_store_rtc_u16     # 0x08 gamepak
  .long execute_store_ignore9_u16 # 0x09 gamepak
  .long execute_store_ignoreA_u16 # 0x0A gamepak
  .long execute_store_ignoreB_u16 # 0x0B gamepak
  .long execute_store_ignoreC_u16 # 0x0C gamepak
  .long execute_store_eeprom_u16  # 0x0D gamepak/eeprom
  .long execute_store_ignoreE_u16 # 0x0E Flash ROM/SRAM
  .long execute_store_ignoreF_u16 # 0x0F open address


patch_store_u16:
  patch_handler store_u16_ftable, 0x0F




# Unsigned 32bit store handlers

execute_store_ignore0_u32:
  ignore_region 0, patch_store_u32

execute_store_ignore1_u32:
  ignore_region 1, patch_store_u32

execute_store_ewram_u32:
  translate_region_ewram_store_align32 patch_store_u32
  store_u32_smc_e (ewram + 0x8000)

execute_store_u32:
execute_store_iwram_u32:
  translate_region 3, patch_store_u32, (iwram + 0x8000), 0x7FFC
  store_u32_smc_i (iwram + 0x8000)

execute_store_io_u32:
  region_check 4, patch_store_u32
  nop
  andi $4, $4, 0x3FC              # wrap around/align address
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)

  save_registers
  jal write_io_register32         # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  j write_io_epilogue             # handle any state changes
  nop

execute_store_palette_u32:
  region_check 5, patch_store_u32
  lui $2, %hi(palette_ram)        # start loading palette_ram address (delay)
  andi $4, $4, 0x3FC              # wrap/align palette address
  addu $2, $2, $4

  jr $ra                          # return
  sw $5, %lo(palette_ram)($2)     # palette_ram[address] = value

execute_store_vram_u32:
  translate_region_vram_store_align32 patch_store_u32
  store_u32 vram

execute_store_oam_u32:
  translate_region 7, patch_store_u32, oam_ram, 0x3FC
  lui $1, %hi(oam_update)         # write non-zero to oam_update
  sw $1, %lo(oam_update)($1)      # cheap, but the address is non-zero
  store_u32 oam_ram

execute_store_ignore8_u32:
  ignore_region 8, patch_store_u32

execute_store_ignore9_u32:
  ignore_region 9, patch_store_u32

execute_store_ignoreA_u32:
  ignore_region 10, patch_store_u32

execute_store_ignoreB_u32:
  ignore_region 11, patch_store_u32

execute_store_ignoreC_u32:
  ignore_region 12, patch_store_u32

execute_store_eeprom_u32:
  store_function write_eeprom, 13, patch_store_u32, 0x3FC

execute_store_ignoreE_u32:
  ignore_region 14, patch_store_u32

execute_store_ignoreF_u32:
  ignore_high patch_store_u32

store_u32_ftable:
  .long execute_store_ignore0_u32 # 0x00 BIOS
  .long execute_store_ignore1_u32 # 0x01 open address
  .long execute_store_ewram_u32   # 0x02 EWRAM
  .long execute_store_iwram_u32   # 0x03 IWRAM
  .long execute_store_io_u32      # 0x04 I/O registers
  .long execute_store_palette_u32 # 0x05 Palette RAM
  .long execute_store_vram_u32    # 0x06 VRAM
  .long execute_store_oam_u32     # 0x07 OAM RAM
  .long execute_store_ignore8_u32 # 0x08 gamepak
  .long execute_store_ignore9_u32 # 0x09 gamepak
  .long execute_store_ignoreA_u32 # 0x0A gamepak
  .long execute_store_ignoreB_u32 # 0x0B gamepak
  .long execute_store_ignoreC_u32 # 0x0C gamepak
  .long execute_store_eeprom_u32  # 0x0D gamepak/eeprom
  .long execute_store_ignoreE_u32 # 0x0E Flash ROM/SRAM
  .long execute_store_ignoreF_u32 # 0x0F open address


patch_store_u32:
  patch_handler store_u32_ftable, 0x0F



# Unsigned always aligned, a2 safe 32bit store handlers

execute_store_ignore0_u32a:
  ignore_region 0, patch_store_u32a

execute_store_ignore1_u32a:
  ignore_region 1, patch_store_u32a

execute_store_ewram_u32a:
  translate_region_ewram_store_align32 patch_store_u32a
  store_u32 (ewram + 0x8000)

execute_aligned_store32:
execute_store_iwram_u32a:
  translate_region 3, patch_store_u32a, (iwram + 0x8000), 0x7FFC
  store_u32 (iwram + 0x8000)

execute_store_io_u32a:
  region_check 4, patch_store_u32a
  nop
  sw $6, REG_SAVE($16)            # save a2
  sw $ra, REG_SAVE2($16)          # save ra

  andi $4, $4, 0x3FC              # wrap around/align address

  save_registers
  jal write_io_register32         # write the value out
  nop

  restore_registers

  lw $ra, REG_SAVE2($16)          # restore ra
  jr $ra
  lw $6, REG_SAVE($16)            # restore a2 (delay)

execute_store_palette_u32a:
  region_check 5, patch_store_u32a
  lui $2, %hi(palette_ram)        # start loading palette_ram address (delay)
  andi $4, $4, 0x3FC              # wrap/align palette address
  addu $2, $2, $4
  sw $5, %lo(palette_ram)($2)     # palette_ram[address] = value

  jr $ra                          # return
  sw $5, %lo(palette_ram)($2)     # palette_ram[address] = value

execute_store_vram_u32a:
  translate_region_vram_store_align32 patch_store_u32a
  store_u32 vram

execute_store_oam_u32a:
  translate_region 7, patch_store_u32a, oam_ram, 0x3FC
  lui $1, %hi(oam_update)         # write non-zero to oam_update
  sw $1, %lo(oam_update)($1)      # cheap, but the address is non-zero
  store_u32 oam_ram

execute_store_ignore8_u32a:
  ignore_region 8, patch_store_u32a

execute_store_ignore9_u32a:
  ignore_region 9, patch_store_u32a

execute_store_ignoreA_u32a:
  ignore_region 10, patch_store_u32a

execute_store_ignoreB_u32a:
  ignore_region 11, patch_store_u32a

execute_store_ignoreC_u32a:
  ignore_region 12, patch_store_u32a

execute_store_eeprom_u32a:
  store_function_a write_eeprom, 13, patch_store_u32a, 0x3FC

execute_store_ignoreE_u32a:
  ignore_region 14, patch_store_u32a

execute_store_ignoreF_u32a:
  ignore_high patch_store_u32a

store_u32a_ftable:
  .long execute_store_ignore0_u32a # 0x00 BIOS
  .long execute_store_ignore1_u32a # 0x01 open address
  .long execute_store_ewram_u32a   # 0x02 EWRAM
  .long execute_store_iwram_u32a   # 0x03 IWRAM
  .long execute_store_io_u32a      # 0x04 I/O registers
  .long execute_store_palette_u32a # 0x05 Palette RAM
  .long execute_store_vram_u32a    # 0x06 VRAM
  .long execute_store_oam_u32a     # 0x07 OAM RAM
  .long execute_store_ignore8_u32a # 0x08 gamepak
  .long execute_store_ignore9_u32a # 0x09 gamepak
  .long execute_store_ignoreA_u32a # 0x0A gamepak
  .long execute_store_ignoreB_u32a # 0x0B gamepak
  .long execute_store_ignoreC_u32a # 0x0C gamepak
  .long execute_store_eeprom_u32a  # 0x0D gamepak/eeprom
  .long execute_store_ignoreE_u32a # 0x0E Flash ROM/SRAM
  .long execute_store_ignoreF_u32a # 0x0F open address

patch_store_u32a:
  patch_handler store_u32a_ftable, 0x0F

execute_load_full_u8:
  srl $1, $4, 28                  # check if the address is out of range
  bne $1, $0, ext_load_u8         # if it is, perform an extended read
  srl $2, $4, 15                  # $1 = page number of address (delay)
  sll $2, $2, 2                   # adjust to word index
  addu $2, $2, $16                # $1 = memory_map_read[address >> 15]
  lw $1, -32768($2)
  beq $1, $0, ext_load_u8         # if it's NULL perform an extended read
  andi $2, $4, 0x7FFF             # $2 = low 15bits of address (delay slot)
  addu $1, $1, $2                 # add the memory map offset
  jr $ra                          # return
  lbu $2, ($1)                    # read the value

ext_load_u8:
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)                   # store return address
  save_registers
  jal read_memory8                # read the value
  nop
  restore_registers
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # return
  addiu $sp, $sp, 4               # fix stack (delay slot)

execute_load_full_s8:
  srl $1, $4, 28                  # check if the address is out of range
  bne $1, $0, ext_load_s8         # if it is, perform an extended read
  srl $2, $4, 15                  # $1 = page number of address (delay)
  sll $2, $2, 2                   # adjust to word index
  addu $2, $2, $16                # $1 = memory_map_read[address >> 15]
  lw $1, -32768($2)
  beq $1, $0, ext_load_s8         # if it's NULL perform an extended read
  andi $2, $4, 0x7FFF             # $2 = low 15bits of address (delay slot)
  addu $1, $1, $2                 # add the memory map offset
  jr $ra                          # return
  lb $2, ($1)                     # read the value

ext_load_s8:
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)                   # store return address
  save_registers
  jal read_memory8                # read the value
  nop
  restore_registers
  seb $2, $2                      # sign extend the read value
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # return
  addiu $sp, $sp, 4               # fix stack (delay slot)

execute_load_full_u16:
  srl $1, $4, 28                  # check if the address is out of range
  ins $1, $4, 4, 1                # or unaligned (bottom bit)
  bne $1, $0, ext_load_u16        # if it is, perform an extended read
  srl $2, $4, 15                  # $1 = page number of address (delay)
  sll $2, $2, 2                   # adjust to word index
  addu $2, $2, $16                # $1 = memory_map_read[address >> 15]
  lw $1, -32768($2)
  beq $1, $0, ext_load_u16        # if it's NULL perform an extended read
  andi $2, $4, 0x7FFF             # $2 = low 15bits of address (delay slot)
  addu $1, $1, $2                 # add the memory map offset
  jr $ra                          # return
  lhu $2, ($1)                    # read the value (delay)

ext_load_u16:
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)                   # store return address
  save_registers
  jal read_memory16               # read the value
  nop
  restore_registers
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # return
  addiu $sp, $sp, 4               # fix stack (delay slot)

execute_load_full_s16:
  srl $1, $4, 28                  # check if the address is out of range
  ins $1, $4, 4, 1                # or unaligned (bottom bit)
  bne $1, $0, ext_load_s16        # if it is, perform an extended read
  srl $2, $4, 15                  # $1 = page number of address (delay)
  sll $2, $2, 2                   # adjust to word index
  addu $2, $2, $16                # $1 = memory_map_read[address >> 15]
  lw $1, -32768($2)
  beq $1, $0, ext_load_s16        # if it's NULL perform an extended read
  andi $2, $4, 0x7FFF             # $2 = low 15bits of address (delay slot)
  addu $1, $1, $2                 # add the memory map offset
  jr $ra                          # return
  lh $2, ($1)                     # read the value (delay)

ext_load_s16:
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)                   # store return address
  save_registers
  jal read_memory16_signed        # read the value
  nop
  restore_registers
  seh $2, $2                      # sign extend the return value
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # return
  addiu $sp, $sp, 4               # fix stack (delay slot)

execute_load_full_u32:
  srl $1, $4, 28                  # check if the address is out of range
  ins $1, $4, 4, 2                # or unaligned (bottom two bits)
  bne $1, $0, ext_load_u32        # if it is, perform an extended read
  srl $2, $4, 15                  # $1 = page number of address (delay)
  sll $2, $2, 2                   # adjust to word index
  addu $2, $2, $16                # $1 = memory_map_read[address >> 15]
  lw $1, -32768($2)
  beq $1, $0, ext_load_u32        # if it's NULL perform an extended read
  andi $2, $4, 0x7FFF             # $2 = low 15bits of address (delay slot)
  addu $1, $1, $2                 # add the memory map offset
  jr $ra                          # return
  lw $2, ($1)                     # read the value (delay)

ext_load_u32:
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)                   # store return address
  save_registers
  jal read_memory32               # read the value
  nop
  restore_registers
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # return
  addiu $sp, $sp, 4               # fix stack (delay slot)

ext_aligned_load32:
  addiu $sp, $sp, -8              # make room on the stack for $ra
  sw $6, 4($sp)
  sw $ra, ($sp)                   # store return address
  save_registers
  jal read_memory32               # read the value
  nop
  restore_registers
  lw $6, 4($sp)
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # return
  addiu $sp, $sp, 8               # fix stack (delay slot)

# General ext memory routines

ext_store_ignore:
  jr $ra                          # ignore these writes
  nop

write_io_epilogue:
  beq $2, $0, no_alert            # 0 means nothing happened
  addiu $4, $2, -CPU_ALERT_SMC_I                # see if return value is 2 (delay slot)
  beq $4, $0, smc_dma             # is it an SMC alert? (return value = 2)
  nop
  addiu $4, $2, -CPU_ALERT_SMC_E                # see if return value is 2 (delay slot)
  beq $4, $0, smc_dma             # is it an SMC alert? (return value = 2)
  nop
  addiu $4, $2, -CPU_ALERT_IRQ                # see if return value is 3
  beq $4, $0, irq_alert           # is it an IRQ alert? (return value = 3)
  nop
  collapse_flags                  # make sure flags are good for update_gba

alert_loop:
  jal update_gba                  # process the next event
  nop
  lw $1, CPU_HALT_STATE($16)      # check if CPU is sleeping
  bne $1, $0, alert_loop          # see if it hasn't changed
  nop

  addu $17, $2, $0                # $17 = new cycle counter
  lw $4, REG_PC($16)              # $4 = new PC

  j lookup_pc
  addiu $sp, $sp, 4               # fix the stack (delay slot)

irq_alert:
  restore_registers
  j lookup_pc                     # PC has changed, get a new one
  addiu $sp, $sp, 4               # fix the stack (delay)

no_alert:
  restore_registers
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # we can return
  addiu $sp, $sp, 4               # fix the stack (delay)

smc_dma:
  addiu $sp, $sp, 4               # fix the stack
  jal flush_translation_cache_iram # flush translation cache
  nop
  jal flush_translation_cache_eram # flush translation cache
  nop
  j lookup_pc
  nop

smc_dma_i:
  addiu $sp, $sp, 4               # fix the stack
  jal flush_translation_cache_iram # flush translation cache
  nop
  j lookup_pc
  nop

smc_dma_e:
  addiu $sp, $sp, 4               # fix the stack
  jal flush_translation_cache_eram # flush translation cache
  nop
  j lookup_pc
  nop

ext_store_eeprom:
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)
  save_registers
  jal write_eeprom                # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  restore_registers
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # we can return
  addiu $sp, $sp, 4               # fix the stack (delay)

# 8bit ext memory routines

ext_store_io8:
  andi $5, $5, 0xFF               # make value 8bit
  andi $4, $4, 0x3FF              # wrap around address
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)
  save_registers
  jal write_io_register8          # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  j write_io_epilogue             # handle any state changes
  nop

ext_store_palette8:
  j ext_store_palette16b          # perform 16bit palette write
  andi $4, $4, 0x3FE              # wrap + align (delay)

ext_store_vram8:
  ins $5, $5, 8, 8                # value = (value << 8) | value
  ext $4, $4, 0, 17               # address = adress & 0x1FFFF
  ins $4, $0, 0, 1                # align out bottom bit
  lui $1, %hi(0x18000)            # $1 = 0x18000
  sltu $1, $4, $1                 # if $4 >= $1 then $1 = 0 else $1 = 1
  bne $1, $0, ext_store_vram8b    # if $1 != 0 then j ext_store_vram8b
  lui $2, %hi(vram)               # start loading vram address (delay)

  addiu $4, $4, -0x8000           # move address into VRAM region

ext_store_vram8b:
  addu $2, $2, $4                 # $2 = (hi)vram + address
  jr $ra                          # return
  sh $5, %lo(vram)($2)            # vram[address] = value (delay)

ext_store_oam8:
  lui $1, %hi(oam_update)         # write non-zero to oam_update
  sw $1, %lo(oam_update)($1)      # cheap, but the address is non-zero
  andi $4, $4, 0x3FE              # wrap around address and align to 16bits
  ins $5, $5, 8, 8                # value = (value << 8) | value
  lui $1, %hi(oam_ram)            # $1 = (hi)oam_ram
  addu $1, $1, $4                 # $1 = (hi)oam_ram + address
  jr $ra                          # return
  sh $5, %lo(oam_ram)($1)         # oam_ram[address] = value (delay)

ext_store_backup:
  andi $5, $5, 0xFF               # make value 8bit
  andi $4, $4, 0xFFFF             # mask value
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)
  save_registers
  jal write_backup                # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  restore_registers
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # we can return
  addiu $sp, $sp, 4               # fix the stack (delay)

ext_store_u8_jtable:
  .long ext_store_ignore    # 0x00 BIOS
  .long ext_store_ignore    # 0x01 invalid
  .long ext_store_ignore    # 0x02 EWRAM
  .long ext_store_ignore    # 0x03 IWRAM
  .long ext_store_io8       # 0x04 I/O registers
  .long ext_store_palette8  # 0x05 Palette RAM
  .long ext_store_vram8     # 0x06 VRAM
  .long ext_store_oam8      # 0x07 OAM RAM
  .long ext_store_ignore    # 0x08 gamepak (no RTC accepted in 8bit)
  .long ext_store_ignore    # 0x09 gamepak, ignore
  .long ext_store_ignore    # 0x0A gamepak, ignore
  .long ext_store_ignore    # 0x0B gamepak, ignore
  .long ext_store_ignore    # 0x0C gamepak, ignore
  .long ext_store_eeprom    # 0x0D EEPROM (possibly)
  .long ext_store_backup    # 0x0E Flash ROM/SRAM
  .long ext_store_ignore    # 0x0F invalid



ext_store_u8:
  srl $1, $4, 24                  # $1 = address >> 24
  sltu $2, $1, 16                 # check if the value is out of range
  beq $2, $0, ext_store_ignore
  sll $1, $1, 2                   # make address word indexed (delay)
  lui $2, %hi(ext_store_u8_jtable)
  addu $2, $2, $1
  # $2 = ext_store_u8_jtable[address >> 24]
  lw $2, %lo(ext_store_u8_jtable)($2)
  jr $2                           # jump to table location
  nop

# 16bit ext memory routines

ext_store_io16:
  andi $4, $4, 0x3FF              # wrap around address
  andi $5, $5, 0xFFFF             # make value 16bit
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)
  save_registers
  jal write_io_register16         # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  j write_io_epilogue             # handle any state changes
  nop

ext_store_palette16:
  andi $4, 0x3FF                  # wrap address

ext_store_palette16b:
  lui $2, %hi(palette_ram)
  addu $2, $2, $4
  sh $5, %lo(palette_ram)($2)     # palette_ram[address] = value

  jr $ra                          # return
  sh $5, %lo(palette_ram)($2)     # palette_ram[address] = value

ext_store_vram16:
  ext $4, $4, 0, 17               # address = adress & 0x1FFFF
  lui $1, %hi(0x18000)            # $1 = 0x18000
  sltu $1, $4, $1                 # see if address < 0x18000
  bne $1, $0, ext_store_vram16b
  lui $2, %hi(vram)               # start loading vram address (delay)

  addiu $4, $4, -0x8000           # move address into VRAM region

ext_store_vram16b:
  addu $2, $2, $4                 # $2 = (hi)vram + address
  jr $ra                          # return
  sh $5, %lo(vram)($2)            # vram[address] = value (delay)

ext_store_oam16:
  lui $1, %hi(oam_update)         # write non-zero to oam_update
  sw $1, %lo(oam_update)($1)      # cheap, but the address is non-zero
  andi $4, $4, 0x3FF              # wrap around address
  lui $1, %hi(oam_ram)            # $1 = (hi)oam_ram
  addu $1, $1, $4                 # $1 = (hi)oam_ram + address
  jr $ra                          # return
  sh $5, %lo(oam_ram)($1)         # oam_ram[address] = value (delay)

ext_store_rtc:
  andi $5, $5, 0xFFFF             # make value 16bit
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)
  save_registers
  jal write_rtc                   # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  restore_registers
  lw $ra, ($sp)                   # restore return address
  jr $ra                          # we can return
  addiu $sp, $sp, 4               # fix the stack (delay)

ext_store_u16_jtable:
  .long ext_store_ignore          # 0x00 BIOS, ignore
  .long ext_store_ignore          # 0x01 invalid, ignore
  .long ext_store_ignore          # 0x02 EWRAM, should have been hit already
  .long ext_store_ignore          # 0x03 IWRAM, should have been hit already
  .long ext_store_io16            # 0x04 I/O registers
  .long ext_store_palette16       # 0x05 Palette RAM
  .long ext_store_vram16          # 0x06 VRAM
  .long ext_store_oam16           # 0x07 OAM RAM
  .long ext_store_rtc             # 0x08 gamepak, RTC
  .long ext_store_ignore          # 0x09 gamepak, ignore
  .long ext_store_ignore          # 0x0A gamepak, ignore
  .long ext_store_ignore          # 0x0B gamepak, ignore
  .long ext_store_ignore          # 0x0C gamepak, ignore
  .long ext_store_eeprom          # 0x0D EEPROM (possibly)
  .long ext_store_ignore          # 0x0E Flash ROM/SRAM

ext_store_u16:
  srl $1, $4, 24                  # $1 = address >> 24
  sltu $2, $1, 16                 # check if the value is out of range
  beq $2, $0, ext_store_ignore
  sll $1, $1, 2                   # make address word indexed (delay)
  lui $2, %hi(ext_store_u16_jtable)
  addu $2, $2, $1
  # $2 = ext_store_u16_jtable[address >> 24]
  lw $2, %lo(ext_store_u16_jtable)($2)
  jr $2                           # jump to table location
  nop

# 32bit ext memory routines

ext_store_io32:
  andi $4, $4, 0x3FF              # wrap around address
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)
  save_registers
  jal write_io_register32         # write the value out
  sw $6, REG_PC($16)              # save the PC (delay slot)
  j write_io_epilogue             # handle any state changes
  nop

ext_store_palette32:
  addu $6, $ra, $0                # save return address in $6
  jal ext_store_palette16b        # write out palette entry
  andi $4, 0x3FF                  # wrap address (delay)
  addiu $4, $4, 2                 # go to next location
  srl $5, $5, 16                  # shift to next 16bit value
  j ext_store_palette16b          # write out next palette entry
  addu $ra, $6, $0                # restore return address (delay)

ext_store_vram32:
  ext $4, $4, 0, 17               # address = adress & 0x1FFFF
  lui $1, %hi(0x18000)            # $1 = 0x18000
  sltu $1, $4, $1                 # see if address < 0x18000
  bne $1, $0, ext_store_vram32b
  lui $2, %hi(vram)               # start loading vram address (delay)

  addiu $4, $4, -0x8000           # move address into VRAM region

ext_store_vram32b:
  addu $2, $2, $4                 # $2 = (hi)vram + address
  jr $ra                          # return
  sw $5, %lo(vram)($2)            # vram[address] = value (delay)

ext_store_oam32:
  lui $1, %hi(oam_update)         # write non-zero to oam_update
  sw $1, %lo(oam_update)($1)      # cheap, but the address is non-zero
  andi $4, $4, 0x3FF              # wrap around address
  lui $1, %hi(oam_ram)            # $1 = (hi)oam_ram
  addu $1, $1, $4                 # $1 = (hi)oam_ram + address
  jr $ra                          # return
  sw $5, %lo(oam_ram)($1)         # oam_ram[address] = value (delay)

ext_store_u32_jtable:
  .long ext_store_ignore          # 0x00 BIOS, ignore
  .long ext_store_ignore          # 0x01 invalid, ignore
  .long ext_store_ignore          # 0x02 EWRAM, should have been hit already
  .long ext_store_ignore          # 0x03 IWRAM, should have been hit already
  .long ext_store_io32            # 0x04 I/O registers
  .long ext_store_palette32       # 0x05 Palette RAM
  .long ext_store_vram32          # 0x06 VRAM
  .long ext_store_oam32           # 0x07 OAM RAM
  .long ext_store_ignore          # 0x08 gamepak, ignore
  .long ext_store_ignore          # 0x09 gamepak, ignore
  .long ext_store_ignore          # 0x0A gamepak, ignore
  .long ext_store_ignore          # 0x0B gamepak, ignore
  .long ext_store_ignore          # 0x0C gamepak, ignore
  .long ext_store_eeprom          # 0x0D EEPROM (possibly)
  .long ext_store_ignore          # 0x0E Flash ROM/SRAM

ext_store_u32:
  srl $1, $4, 24                  # $1 = address >> 24
  sltu $2, $1, 16                 # check if the value is out of range
  beq $2, $0, ext_store_ignore
  sll $1, $1, 2                   # make address word indexed (delay)
  lui $2, %hi(ext_store_u32_jtable)
  addu $2, $2, $1
  # $2 = ext_store_u32_jtable[address >> 24]
  lw $2, %lo(ext_store_u32_jtable)($2)
  jr $2                           # jump to table location
  nop

# 32bit ext aligned, non a2 destroying routines

ext_store_io32a:
  andi $4, $4, 0x3FF              # wrap around address
  addiu $sp, $sp, -4              # make room on the stack for $ra
  sw $ra, ($sp)
  save_registers
  jal write_io_register32         # write the value out
  sw $6, REG_SAVE($16)            # save a2 (delay)
  lw $6, REG_SAVE($16)            # restore a2 (delay)
  j write_io_epilogue             # handle any state changes
  nop

ext_store_palette32a:
  sw $ra, REG_SAVE($16)           # save return address
  jal ext_store_palette16b        # write out palette entry
  andi $4, 0x3FF                  # wrap address (delay)
  addiu $4, $4, 2                 # go to next location
  srl $5, $5, 16                  # shift to next 16bit value
  j ext_store_palette16b          # write out next palette entry
  lw $ra, REG_SAVE($16)           # restore return address (delay)

ext_store_u32a_jtable:
  .long ext_store_ignore          # 0x00 BIOS, ignore
  .long ext_store_ignore          # 0x01 invalid, ignore
  .long ext_store_ignore          # 0x02 EWRAM, should have been hit already
  .long ext_store_ignore          # 0x03 IWRAM, should have been hit already
  .long ext_store_io32a           # 0x04 I/O registers
  .long ext_store_palette32a      # 0x05 Palette RAM
  .long ext_store_vram32          # 0x06 VRAM
  .long ext_store_oam32           # 0x07 OAM RAM
  .long ext_store_ignore          # 0x08 gamepak, ignore
  .long ext_store_ignore          # 0x09 gamepak, ignore
  .long ext_store_ignore          # 0x0A gamepak, ignore
  .long ext_store_ignore          # 0x0B gamepak, ignore
  .long ext_store_ignore          # 0x0C gamepak, ignore
  .long ext_store_ignore          # 0x0D EEPROM (nothing will write this)
  .long ext_store_ignore          # 0x0E Flash ROM/SRAM

ext_aligned_store32:
  srl $1, $4, 24                  # $1 = address >> 24
  sltu $2, $1, 16                 # check if the value is out of range
  beq $2, $0, ext_store_ignore
  sll $1, $1, 2                   # make address word indexed (delay)
  lui $2, %hi(ext_store_u32a_jtable)
  addu $2, $2, $1
  # $2 = ext_store_u32a_jtable[address >> 24]
  lw $2, %lo(ext_store_u32a_jtable)($2)
  jr $2                           # jump to table location
  nop

smc_write_i:
  save_registers
  jal flush_translation_cache_iram # flush translation cache
  sw $6, REG_PC($16)              # save PC (delay slot)

smc_write_e:
  save_registers
  jal flush_translation_cache_eram # flush translation cache
  sw $6, REG_PC($16)              # save PC (delay slot)

lookup_pc:
  lw $2, REG_CPSR($16)            # $2 = cpsr
  andi $2, $2, 0x20               # isolate mode bit
  beq $2, $0, lookup_pc_arm       # if T bit is zero use arm handler
  nop

lookup_pc_thumb:
  jal block_lookup_address_thumb  # get Thumb address
  lw $4, REG_PC($16)              # load PC as arg 0 (delay slot)
  restore_registers
  jr $2                           # jump to result
  nop

lookup_pc_arm:
  jal block_lookup_address_arm    # get ARM address
  lw $4, REG_PC($16)              # load PC as arg 0 (delay slot)
  restore_registers
  jr $2                           # jump to result
  nop

# Return the current cpsr

execute_read_cpsr:
  collapse_flags                  # fold flags into cpsr, put cpsr into $2
  jr $ra                          # return
  nop

# Return the current spsr

execute_read_spsr:
  lw $1, CPU_MODE($16)            # $1 = cpu_mode
  lui $2, %hi(spsr)
  sll $1, $1, 2                   # adjust to word offset size
  addu $2, $2, $1
  jr $ra                          # return
  lw $2, %lo(spsr)($2)            # $2 = spsr[cpu_mode] (delay slot)

# Switch into SWI, has to collapse flags
# $4: Current pc

execute_swi:
  addiu $sp, $sp, -4              # push $ra
  sw $ra, ($sp)
  lui $1, %hi(SUPERVISOR_LR)
  sw $4, %lo(SUPERVISOR_LR)($1)   # store next PC in the supervisor's LR
  collapse_flags                  # get cpsr in $2
  lui $5, %hi(SUPERVISOR_SPSR)
  sw $2, %lo(SUPERVISOR_SPSR)($5) # save cpsr in SUPERVISOR_CPSR
  ins $2, $0, 0, 6                # zero out bottom 6 bits of CPSR
  ori $2, 0x13                    # set mode to supervisor
  sw $2, REG_CPSR($16)            # write back CPSR
  save_registers
  jal set_cpu_mode                # set the CPU mode to supervisor
  li $4, 3                        # 3 is supervisor mode (delay slot)
  restore_registers
  lw $ra, ($sp)                   # pop $ra
  jr $ra                          # return
  addiu $sp, $sp, 4               # fix stack (delay slot)

# $4: pc to restore to
# returns in $4

execute_spsr_restore:
  lw $1, CPU_MODE($16)            # $1 = cpu_mode

  beq $1, $0, no_spsr_restore     # only restore if the cpu isn't usermode
  lui $2, %hi(spsr)               # start loading SPSR (delay)

  sll $1, $1, 2                   # adjust to word offset size
  addu $2, $2, $1
  lw $1, %lo(spsr)($2)            # $1 = spsr[cpu_mode]
  sw $1, REG_CPSR($16)            # cpsr = spsr[cpu_mode]
  extract_flags_body              # extract flags from $1
  addiu $sp, $sp, -4
  sw $ra, ($sp)
  save_registers
  jal execute_spsr_restore_body   # do the dirty work in this C function
  nop
  restore_registers
  addu $4, $2, $0                 # move return value to $4
  lw $ra, ($sp)
  jr $ra
  addiu $sp, $sp, 4

no_spsr_restore:
  jr $ra
  nop

# $4: new cpsr
# $5: store mask
# $6: current PC

execute_store_cpsr:
  and $1, $4, $5                  # $1 = new_cpsr & store_mask
  lw $2, REG_CPSR($16)            # $2 = current cpsr
  nor $4, $5, $0                  # $4 = ~store_mask
  and $2, $2, $4                  # $2 = (cpsr & (~store_mask))
  or $1, $1, $2                   # $1 = new cpsr combined with old
  extract_flags_body              # extract flags from $1
  addiu $sp, $sp, -4
  sw $ra, ($sp)
  save_registers
  jal execute_store_cpsr_body     # do the dirty work in this C function
  addu $4, $1, $0                 # load the new CPSR (delay slot)

  bne $2, $0, changed_pc_cpsr     # this could have changed the pc
  nop

  restore_registers

  lw $ra, ($sp)
  jr $ra
  addiu $sp, $sp, 4

changed_pc_cpsr:
  jal block_lookup_address_arm    # GBA address is in $4
  addu $4, $2, $0                 # load new address in $4 (delay slot)
  restore_registers               # restore registers
  jr $2                           # jump to the new address
  addiu $sp, $sp, 4               # get rid of the old ra (delay slot)


# $4: new spsr
# $5: store mask

execute_store_spsr:
  lw $1, CPU_MODE($16)            # $1 = cpu_mode
  lui $2, %hi(spsr)
  sll $1, $1, 2                   # adjust to word offset size
  addu $1, $2, $1
  lw $2, %lo(spsr)($1)            # $2 = spsr[cpu_mode]
  and $4, $4, $5                  # $4 = new_spsr & store_mask
  nor $5, $5, $0                  # $5 = ~store_mask
  and $2, $2, $5                  # $2 = (spsr & (~store_mask))
  or $4, $4, $2                   # $4 = new spsr combined with old
  jr $ra                          # return
  sw $4, %lo(spsr)($1)            # spsr[cpu_mode] = $4 (delay slot)

# $4: value
# $5: shift

execute_lsl_flags_reg:
  beq $5, $0, lsl_shift_zero      # is the shift zero?
  sltiu $1, $5, 32                # $1 = (shift < 32) (delay)
  beq $1, $0, lsl_shift_high      # is the shift >= 32?
  li $2, 32

  subu $2, $2, $5                 # $2 = (32 - shift)
  srlv $2, $4, $2                 # $2 = (value >> (32 - shift))
  andi $22, $2, 1                 # c flag = (value >> (32 - shift)) & 0x01

lsl_shift_zero:
  jr $ra                          # return
  sllv $4, $4, $5                 # return (value << shift) (delay)

lsl_shift_high:
  sltiu $1, $5, 33                # $1 = (shift < 33) (delay)
  bne $1, $0, lsl_shift_done      # jump if shift == 32
  andi $22, $4, 1                 # c flag = value & 0x01 (delay)

  add $22, $0, $0                 # c flag = 0 otherwise

lsl_shift_done:
  jr $ra                          # return
  add $4, $0, $0                  # value = 0 no matter what (delay)


execute_lsr_flags_reg:
  beq $5, $0, lsr_shift_zero      # is the shift zero?
  sltiu $1, $5, 32                # $1 = (shift < 32) (delay)
  beq $1, $0, lsr_shift_high      # is the shift >= 32?
  addiu $2, $5, -1                # $2 = shift - 1 (delay)

  srlv $2, $4, $2                 # $2 = (value >> (shift - 1))
  andi $22, $2, 1                 # c flag = (value >> (32 - shift)) & 0x01

lsr_shift_zero:
  jr $ra                          # return
  srlv $4, $4, $5                 # return (value >> shift) (delay)

lsr_shift_high:
  sltiu $1, $5, 33                # $1 = (shift < 33) (delay)
  bne $1, $0, lsr_shift_done      # jump if shift == 32
  srl $22, $4, 31                 # c flag = value >> 31 (delay)

  add $22, $0, $0                 # c flag = 0 otherwise

lsr_shift_done:
  jr $ra                          # return
  add $4, $0, $0                  # value = 0 no matter what


execute_asr_flags_reg:
  beq $5, $0, asr_shift_zero      # is the shift zero?
  sltiu $1, $5, 32                # $1 = (shift < 32) (delay)
  beq $1, $0, asr_shift_high      # is the shift >= 32?
  addiu $2, $5, -1                # $2 = shift - 1 (delay)

  srlv $2, $4, $2                 # $2 = (value >> (shift - 1))
  andi $22, $2, 1                 # c flag = (value >> (32 - shift)) & 0x01

asr_shift_zero:
  jr $ra                          # return
  srav $4, $4, $5                 # return (value >> shift) (delay)

asr_shift_high:
  sra $4, $4, 31                  # value >>= 31
  jr $ra                          # return
  andi $22, $4, 1                 # c flag = value & 0x01


execute_ror_flags_reg:
  beq $5, $0, ror_zero_shift      # is the shift zero?
  addiu $1, $5, -1                # $1 = (shift - 1) (delay)

  srav $1, $4, $1                 # $1 = (value >> (shift - 1))
  andi $22, $1, 1                 # c flag = $1 & 1

ror_zero_shift:
  jr $ra                          # return
  rotrv $4, $4, $5                # return (value ror shift) delay

# エミュレートの開始
# 入力 $4: cycle counter
execute_arm_translate:
  addu $17, $4, $0                # $17 = cycle counter
  lui $16, %hi(reg)               # $16 = &reg
  addiu $16, %lo(reg)
  extract_flags                   # $1にCPSR,ステータスフラグを$28~$31に展開

  and $1, $1, 0x20                # $1 = $1 & 0x20

  bne $1, $0, 1f                  # $1 != 0(THUMBビットがON)ならば1:へ
  lw $4, REG_PC($16)              # $4 = PC (delay)

  jal block_lookup_address_arm    # lookup initial jump address
  nop
  restore_registers               # load initial register values
  jr $2                           # jump to return
  nop

1:
  jal block_lookup_address_thumb  # lookup initial jump address
  nop
  restore_registers               # load initial register values
  jr $2                           # jump to return
  nop

# sceKernelInvalidateIcacheRange gives me problems, trying this instead
# Invalidates an n byte region starting at the start address
# $4: start location
# $5: length

invalidate_icache_region:
  ins $4, $0, 0, 6                # align to 64 bytes
  addiu $2, $5, 63                # align up to 64 bytes
  srl $2, $2, 6                   # divide by 64
  beq $2, $0, done                # exit early on 0
  nop

iir_loop:
  cache 0x08, ($4)                # invalidate icache line
  addiu $2, $2, -1                # next loop iteration
  bne $2, $0, iir_loop            # loop
  addiu $4, $4, 64                # go to next cache line (delay slot)

done:
  jr $ra                          # return
  nop

# Writes back dcache and invalidates icache.

invalidate_all_cache:
  addu $4, $0, $0                 # $4 = 0
  addiu $5, $0, 0x4000            # $5 = 0x4000

iac_loop:
  cache 0x14, 0($4)               # index invalidate/writeback dcache index
  addiu $4, $4, 0x40              # goto next cache line
  bne $4, $5, iac_loop            # next iteration
  cache 0x04, -0x40($4)           # index invalidate icache index.. maybe?

  jr $ra                          # return
  nop

memory_map_read:
  .space 0x8000

reg:
  .space 0x100

memory_map_write:
  .space 0x8000

