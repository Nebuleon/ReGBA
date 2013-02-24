/***********************************************************************
 * sleep.S
 *
 * jz47xx Assembler Sleep/WakeUp Management Routines
 *
 * Copyright (C) 2005 Ingenic Semiconductor Inc. All rights reserved.
 * Author: <jlwei@ingenic.cn>
 */

#include <archdefs.h>
#include <mipsregs.h>
#include <regdef.h>

	.text
	.set	noreorder
	.set	noat

	.extern __dcache_writeback_all
	.extern __icache_invalidate_all

/*
 * jz_cpu_suspend()
 *
 * Forces CPU into hibernate mode
 */

	.globl	jz_cpu_suspend
jz_cpu_suspend:

	/* save general registers except k0($26) and k1($27) */
	move	k0, sp
	addiu	k0, k0, -(30*4)
	sw	$0,  0(k0)
	sw	$1,  4(k0)
	sw	$2,  8(k0)
	sw	$3,  12(k0)
	sw	$4,  16(k0)
	sw	$5,  20(k0)
	sw	$6,  24(k0)
	sw	$7,  28(k0)
	sw	$8,  32(k0)
	sw	$9,  36(k0)
	sw	$10, 40(k0)
	sw	$11, 44(k0)
	sw	$12, 48(k0)
	sw	$13, 52(k0)
	sw	$14, 56(k0)
	sw	$15, 60(k0)
	sw	$16, 64(k0)
	sw	$17, 68(k0)
	sw	$18, 72(k0)
	sw	$19, 76(k0)
	sw	$20, 80(k0)
	sw	$21, 84(k0)
	sw	$22, 88(k0)
	sw	$23, 92(k0)
	sw	$24, 96(k0)
	sw	$25, 100(k0)
	sw	$28, 104(k0)
	sw	$29, 108(k0)		/* saved sp */
	sw	$30, 112(k0)
	sw	$31, 116(k0)		/* saved ra */
	move	sp, k0

	/* save CP0 registers and sp */
	move	k0, sp
	addiu	k0, k0, -(26*4)

	mfc0	$1, CP0_INDEX
	mfc0	$2, CP0_RANDOM
	mfc0	$3, CP0_ENTRYLO0
	mfc0	$4, CP0_ENTRYLO1
	mfc0	$5, CP0_CONTEXT
	mfc0	$6, CP0_PAGEMASK
	mfc0	$7, CP0_WIRED
	mfc0	$8, CP0_BADVADDR
	mfc0	$9, CP0_ENTRYHI
	mfc0	$10, CP0_STATUS
/*	mfc0	$11, $12, 1 */		/* IntCtl */
	mfc0	$12, CP0_CAUSE
	mfc0	$13, CP0_EPC
/*	mfc0	$14, $15, 1 */		/* EBase */
	mfc0	$15, CP0_CONFIG
/*	mfc0	$16, CP0_CONFIG, 7 */	/* Config 7 */
	mfc0	$17, CP0_LLADDR
	mfc0	$18, CP0_WATCHLO
	mfc0	$19, CP0_WATCHHI
	mfc0	$20, CP0_DEBUG
	mfc0	$21, CP0_DEPC
	mfc0	$22, CP0_ECC
	mfc0	$23, CP0_TAGLO
	mfc0	$24, CP0_ERROREPC
	mfc0	$25, CP0_DESAVE

	sw	$1,  0(k0)
	sw	$2,  4(k0)
	sw	$3,  8(k0)
	sw	$4,  12(k0)
	sw	$5,  16(k0)
	sw	$6,  20(k0)
	sw	$7,  24(k0)
	sw	$8,  28(k0)
	sw	$9,  32(k0)
	sw	$10, 36(k0)
	sw	$11, 40(k0)
	sw	$12, 44(k0)
	sw	$13, 48(k0)
	sw	$14, 52(k0)
	sw	$15, 56(k0)
	sw	$16, 60(k0)
	sw	$17, 64(k0)
	sw	$18, 68(k0)
	sw	$19, 72(k0)
	sw	$20, 76(k0)
	sw	$21, 80(k0)
	sw	$22, 84(k0)
	sw	$23, 88(k0)
	sw	$24, 92(k0)
	sw	$25, 96(k0)
	sw	$29, 100(k0)	/* saved sp */
	move	sp, k0

	/* preserve virtual address of stack */
	la	k0, suspend_save_sp
	sw	sp, 0(k0)

	/* flush I&D caches and write buffers */
	jal	__dcache_writeback_all
	nop
	jal	__icache_invalidate_all
	nop

	/* put CPU to hibernate mode */
	la	t0, CPM_LPCR
	lw	t1, 0(t0)
	li	t2, ~CPM_LPCR_LPM_MASK
	and	t1, t2
	ori	t1, CPM_LPCR_LPM_HIBERNATE

	/* prepare pointer to physical address 0 */
	la	t2, 0xa0000000

	/* align execution to a cache line */
	j	1f

	.align	5
1:
	/* all needed values are now in registers.
	 * These last instructions should be in cache
	 */
	/* set hibernate mode */
	sw	t1, 0(t0)

	/* force address lines low by reading at physical address 0 */
	lw	t2, 0(t2)

	/* enter hibernate mode */
	.set	mips3	
	wait
	.set	mips2

2:	j	2b		/* loop waiting for suspended */
	nop

/*
 * jz_cpu_resume()
 *
 * entry point from bootloader into kernel during resume
 */

	.align 5
	.globl	jz_cpu_resume
jz_cpu_resume:
	/* clear SCR.HGP */
	la	t0, CPM_SCR
	lw	t1, 0(t0)
	li	t2, ~CPM_SCR_HGP
	and	t1, t2
	sw	t1, 0(t0)

	/* restore LPCR.LPM to IDLE mode */
	la	t0, CPM_LPCR
	lw	t1, 0(t0)
	li	t2, ~CPM_LPCR_LPM_MASK
	and	t1, t2
	ori	t1, CPM_LPCR_LPM_IDLE
	sw	t1, 0(t0)

	/* restore saved sp */
	la	t0, suspend_save_sp
	lw	sp, 0(t0)

	/* restore CP0 registers */
	move	k0, sp
	lw	$1,  0(k0)
	lw	$2,  4(k0)
	lw	$3,  8(k0)
	lw	$4,  12(k0)
	lw	$5,  16(k0)
	lw	$6,  20(k0)
	lw	$7,  24(k0)
	lw	$8,  28(k0)
	lw	$9,  32(k0)
	lw	$10, 36(k0)
	lw	$11, 40(k0)
	lw	$12, 44(k0)
	lw	$13, 48(k0)
	lw	$14, 52(k0)
	lw	$15, 56(k0)
	lw	$16, 60(k0)
	lw	$17, 64(k0)
	lw	$18, 68(k0)
	lw	$19, 72(k0)
	lw	$20, 76(k0)
	lw	$21, 80(k0)
	lw	$22, 84(k0)
	lw	$23, 88(k0)
	lw	$24, 92(k0)
	lw	$25, 96(k0)
	lw	$29, 100(k0)	/* saved sp */

	mtc0	$1, CP0_INDEX
	mtc0	$2, CP0_RANDOM
	mtc0	$3, CP0_ENTRYLO0
	mtc0	$4, CP0_ENTRYLO1
	mtc0	$5, CP0_CONTEXT
	mtc0	$6, CP0_PAGEMASK
	mtc0	$7, CP0_WIRED
	mtc0	$8, CP0_BADVADDR
	mtc0	$9, CP0_ENTRYHI
	mtc0	$10, CP0_STATUS
/*	mtc0	$11, $12, 1 */		/* IntCtl */
	mtc0	$12, CP0_CAUSE
	mtc0	$13, CP0_EPC
/*	mtc0	$14, $15, 1	*/	/* EBase */
	mtc0	$15, CP0_CONFIG
/*	mtc0	$16, CP0_CONFIG, 7*/	/* Config 7 */
	mtc0	$17, CP0_LLADDR
	mtc0	$18, CP0_WATCHLO
	mtc0	$19, CP0_WATCHHI
	mtc0	$20, CP0_DEBUG
	mtc0	$21, CP0_DEPC
	mtc0	$22, CP0_ECC
	mtc0	$23, CP0_TAGLO
	mtc0	$24, CP0_ERROREPC
	mtc0	$25, CP0_DESAVE

	/* restore general registers */
	move	k0, sp
	lw	$0,  0(k0)
	lw	$1,  4(k0)
	lw	$2,  8(k0)
	lw	$3,  12(k0)
	lw	$4,  16(k0)
	lw	$5,  20(k0)
	lw	$6,  24(k0)
	lw	$7,  28(k0)
	lw	$8,  32(k0)
	lw	$9,  36(k0)
	lw	$10, 40(k0)
	lw	$11, 44(k0)
	lw	$12, 48(k0)
	lw	$13, 52(k0)
	lw	$14, 56(k0)
	lw	$15, 60(k0)
	lw	$16, 64(k0)
	lw	$17, 68(k0)
	lw	$18, 72(k0)
	lw	$19, 76(k0)
	lw	$20, 80(k0)
	lw	$21, 84(k0)
	lw	$22, 88(k0)
	lw	$23, 92(k0)
	lw	$24, 96(k0)
	lw	$25, 100(k0)
	lw	$28, 104(k0)
	lw	$29, 108(k0)		/* saved sp */
	lw	$30, 112(k0)
	lw	$31, 116(k0)		/* saved ra */

	/* return to caller */
	jr	ra
	nop

suspend_save_sp:
	.word	0			/* preserve sp here */

	.set	reorder
