/*
 * init.S
 *
 * Initialization code for JzRISC.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <mips.h>

	.text

	.extern c_main  

	.global	_start
	.type	_start,@function
	.set	noreorder
	.set	noat 
_start:
    
    la	ra, _start
	//----------------------------------------------------
	// init cp0 registers.
	//----------------------------------------------------
#if 1
	mtc0	zero, C0_WATCHLO
	mtc0	zero, C0_WATCHHI
#else
    li  t1, (0x80b462f0 + 0x3)
    mtc0 t1, C0_WATCHLO
    li  t1, (1<<30 + 0x3)
    mtc0 t1, C0_WATCHHI
#endif

	li	t1, 0x00800000
	mtc0	t1, C0_CAUSE

    li	    t0, 0x0040FC00
	mtc0	t0, C0_STATUS
	
	//----------------------------------------------------
	// init caches, assumes a 4way*128set*32byte i/d cache
	//----------------------------------------------------
	li	t0, 3		// enable cache for kseg0 accesses
	mtc0	t0, $16		// CONFIG reg
	la	t0, 0x80000000	// an idx op should use a unmappable address
	ori	t1, t0, 0x4000	// 16kB cache
	mtc0	zero, $28	// TAGLO reg
	mtc0	zero, $29	// TAGHI reg

_init_cache_loop:	
	cache	0x8, 0(t0)	// index store icache tag
	//cache	0x9, 0(t0)	// index store dcache tag
    cache	0x1, 0(t0)
    sync
	bne	t0, t1, _init_cache_loop
	addiu	t0, t0, 0x20	// 32 bytes per cache line
	nop
    
    //----------------------------------------------------
    // Built TLB For GBA 
    //----------------------------------------------------
    mtc0    zero, C0_Index                  //TLB 0 ----ROM
    li      t0, (0x1000<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x1000<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x4000<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask16M)             //16MB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB0
    addiu   t0, zero, 1
    mtc0    t0, C0_Index                    //TLB 1 ----BIOS
    li      t0, (0x0F94<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x0F94<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x0<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask16K)             //16KB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB1
    addiu   t0, zero, 2
    mtc0    t0, C0_Index                    //TLB 2 ----InternalRAM
    li      t0, (0x0F98<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x0F9C<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x1800<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask16K)             //16KB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB2
    addiu   t0, zero, 3
    mtc0    t0, C0_Index                    //TLB 3 ----ExternalRAM
    li      t0, (0x0FC0<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x0FC0<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x1000<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask256K)            //256KB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB3
    addiu   t0, zero, 4
    mtc0    t0, C0_Index                    //TLB 4 ----VRAM
    li      t0, (0x0FA0<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x0FB0<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x3000<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask64K)             //64KB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB4
    addiu   t0, zero, 5
    mtc0    t0, C0_Index                    //TLB 5 ----ioMem
    li      t0, (0x0F91<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x0F91<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x2000<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask4K)              //4KB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB5
    addiu   t0, zero, 6
    mtc0    t0, C0_Index                    //TLB 6 ----paletteRAM
    li      t0, (0x0F93<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x0F93<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x2800<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask4K)              //4KB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB6
    addiu   t0, zero, 7
    mtc0    t0, C0_Index                    //TLB 7 ----oam
    li      t0, (0x0F90<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x0F90<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoD | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x3800<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask4K)              //4KB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB7
    addiu   t0, zero, 9
    mtc0    t0, C0_Index                    //TLB 9 ----oam
    li      t0, (0x1000<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x1000<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoV | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x5000<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask16M)             //16MB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB9
    addiu   t0, zero, 10
    mtc0    t0, C0_Index                    //TLB 10 ----oam
    li      t0, (0x1000<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoV | M_EntryLoG)
    mtc0    t0, C0_EntryLo0
    li      t1, (0x1000<<S_EntryLoPFN | K_CacheAttrC<<S_EntryLoC | M_EntryLoG)
    mtc0    t1, C0_EntryLo1
    li      t0, (0x6000<<S_EntryHiVPN2)
    mtc0    t0, C0_EntryHi
    li      t1, (M_PageMask16M)             //16MB page
    mtc0    t1, C0_PageMask
    nop   
    tlbwi                                   //end TLB10
    nop    


	//----------------------------------------------------
	// Invalidate BTB
	//----------------------------------------------------
	mfc0	t0, C0_CONFIG, 7
	nop
	ori	t0, 2
	mtc0	t0, C0_CONFIG, 7
	nop

	//----------------------------------------------------
	// clear BSS section
	//----------------------------------------------------
	la	t0, _fbss
	la	t1, _end
1:	sw	zero, 0(t0)
	bne	t0, t1, 1b
	addiu	t0, 4

	//----------------------------------------------------
	// setup stack and GP, jump to C code
	//----------------------------------------------------
	la	gp, _gp
	la	sp, g_stack+(0x8000-1)*4
//	addiu	sp, sp, (0xF000-1)*4

	la	t0, c_main
	jr	t0
	nop

	.extern real_exception_entry
	.global except_common_entry
	.type	except_common_entry,@function
except_common_entry:
	la	k0, real_exception_entry
	jr	k0
	nop
	nop
	nop

	.extern UCOS_INTHandler
	.extern exception_handler
	.global real_exception_entry
	.type	real_exception_entry,@function
	.set noreorder
real_exception_entry:
	la	k1, (0x1f << 2)
	mfc0	k0, C0_CAUSE
	and	k0, k0, k1
	beq	zero, k0, intr_handler
	nop
	la	k0, exception_handler
	jr	k0
	nop
intr_handler:
	la	k0, UCOS_INTHandler
	jr	k0
	nop
	.set reorder

#if 0
	.global exception_handler
	.type	exception_handler,@function
	.set noreorder
exception_handler:
	la	sp, g_stack		// restore the SP register, and we
	addiu	sp, sp, 0x1000		// will panic in except handler.
	sw	$0, (4*0)(sp);
	sw	$1, (4*1)(sp);
	sw	$2, (4*2)(sp);
	sw	$3, (4*3)(sp);
	sw	$4, (4*4)(sp);
	sw	$5, (4*5)(sp);
	sw	$6, (4*6)(sp);
	sw	$7, (4*7)(sp);
	sw	$8, (4*8)(sp);
	sw	$9, (4*9)(sp);
	sw	$10, (4*10)(sp);
	sw	$11, (4*11)(sp);
	sw	$12, (4*12)(sp);
	sw	$13, (4*13)(sp);
	sw	$14, (4*14)(sp);
	sw	$15, (4*15)(sp);
	sw	$16, (4*16)(sp);
	sw	$17, (4*17)(sp);
	sw	$18, (4*18)(sp);
	sw	$19, (4*19)(sp);
	sw	$20, (4*20)(sp);
	sw	$21, (4*21)(sp);
	sw	$22, (4*22)(sp);
	sw	$23, (4*23)(sp);
	sw	$24, (4*24)(sp);
	sw	$25, (4*25)(sp);
	sw	$26, (4*26)(sp);
	sw	$27, (4*27)(sp);
	sw	$28, (4*28)(sp);
	sw	$29, (4*29)(sp);
	sw	$30, (4*30)(sp);
	sw	$31, (4*31)(sp);
	move	a0, sp
	la	k0, c_except_handler
	jr	k0
	nop
	.set reorder
#endif

.global exception_handler
	.type	exception_handler,@function
	.set noreorder
exception_handler:
	addiu   sp, sp, -128
	sw	ra, (4*0)(sp);
	sw	fp, (4*1)(sp);
	sw	gp, (4*2)(sp);
	sw	t9, (4*3)(sp);
	sw	t8, (4*4)(sp);
	sw	s7, (4*5)(sp);
	sw	s6, (4*6)(sp);
	sw	s5, (4*7)(sp);
	sw	s4, (4*8)(sp);
	sw	s3, (4*9)(sp);
	sw	s2, (4*10)(sp);
	sw	s1, (4*11)(sp);
	sw	s0, (4*12)(sp);
	sw	t7, (4*13)(sp);
	sw	t6, (4*14)(sp);
	sw	t5, (4*15)(sp);
	sw	t4, (4*16)(sp);
	sw	t3, (4*17)(sp);
	sw	t2, (4*18)(sp);
	sw	t1, (4*19)(sp);
	sw	t0, (4*20)(sp);
	sw	a3, (4*21)(sp);
	sw	a2, (4*22)(sp);
	sw	a1, (4*23)(sp);
	sw	a0, (4*24)(sp);
	sw	v1, (4*25)(sp);
	sw	v0, (4*26)(sp);
	sw	AT, (4*27)(sp);
	mflo k0
	sw	k0, (4*28)(sp);
	mfhi k0
	sw	k0, (4*29)(sp);
	mfc0 k0, C0_EPC         //return address
	sw	k0, (4*30)(sp);
	mfc0 k0, C0_STATUS      //return address
	sw	k0, (4*31)(sp);
	
	mfc0 k0, C0_CAUSE
	ori  k1, zero, (0x08<<2)    //Only detect SYSTEM CALL exception
	andi  k0, k0, (0x1F<<2)
	beq  k1, k0, 1f           //is SYSTEM CALL exception
    move	a0, sp
	la	k1, c_except_handler
	jr	k1
	srl a1, k0, 2

2:	
	lw	ra, (4*0)(sp);
	lw	fp, (4*1)(sp);
	lw	gp, (4*2)(sp);
	lw	t9, (4*3)(sp);
	lw	t8, (4*4)(sp);
	lw	s7, (4*5)(sp);
	lw	s6, (4*6)(sp);
	lw	s5, (4*7)(sp);
	lw	s4, (4*8)(sp);
	lw	s3, (4*9)(sp);
	lw	s2, (4*10)(sp);
	lw	s1, (4*11)(sp);
	lw	s0, (4*12)(sp);
	lw	t7, (4*13)(sp);
	lw	t6, (4*14)(sp);
	lw	t5, (4*15)(sp);
	lw	t4, (4*16)(sp);
	lw	t3, (4*17)(sp);
	lw	t2, (4*18)(sp);
	lw	t1, (4*19)(sp);
	lw	t0, (4*20)(sp);
	lw	a3, (4*21)(sp);
	lw	a2, (4*22)(sp);
	lw	a1, (4*23)(sp);
	lw	a0, (4*24)(sp);
	lw	v1, (4*25)(sp);
	lw	v0, (4*26)(sp);
	lw	AT, (4*27)(sp);
	lw	k0, (4*28)(sp);
	mtlo k0
	lw	k0, (4*29)(sp);	
	mthi k0
    lw	k0, (4*30)(sp);
    addiu k0, k0, 4
	mtc0 k0, C0_EPC         //return address= EPC +4
	lw	k0, (4*31)(sp);	
	mtc0 k0, C0_STATUS      //return address
    addiu sp, sp, 128
    
    eret                    //return

1:
    mfc0 k0, C0_EPC
    lw  k1, 0(k0)
    nop
    sll a0, k1, 6           //system call number
    sra a0, a0, 12
    or  a1, zero, k0        //syscall address
    or  a2, zero, sp        //sp
    la  k0, syscall_fun
    jal k0
    nop
    j   2b
    nop
    	
	.set reorder


