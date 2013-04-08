//start.S

#include "mips.h"

	.text

	.extern ds2_init
	.extern heapInit
	.extern ds2_main

	.ent	_start
	.global	_start
	.type	_start,@function
	.set	noreorder
	.set	noat
_start:
	nop
	nop
	//----------------------------------------------------
	// init cp0 registers.
	//----------------------------------------------------
	mtc0	zero, $18
	mtc0	zero, $19

	li	t1, 0x00800000
	mtc0	t1, $13
	li	t0, 0x0040FC00
	mtc0	t0, $12

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
	// Invalidate BTB
	//----------------------------------------------------
	mfc0	t0, $16, 7
	nop
	ori	t0, 2
	mtc0	t0, $16, 7
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
	la	sp, _freemem+0x100000	//stack size = 0x100000

	jal	ds2_init
	nop

	la	a0, _freemem+0x100400	// a0, parameter 1 = Heap start address
	la	a1, 0x82000000-0x400	// a1, parameter 2 = Heap end address
	jal	heapInit		// Initialise the heap for malloc
	nop

	la	t0, ds2_main
	jr	t0
	nop

	.end _start

	.extern real_exception_entry
	.extern GuruMeditation

	.ent except_common_entry
	.global except_common_entry
	.type	except_common_entry,@function

/* except_common_entry is code that gets copied to 3 places in memory to
 * handle exceptions, so it cannot use relative jumps. It needs to fit in
 * 32 bytes (8 words).
 */
except_common_entry:
	la	k0, real_exception_entry
	jr	k0
	nop
	nop
	nop
	nop

	.end except_common_entry


	.extern C_IntHandle
	.extern exception_handler

	.ent real_exception_entry
	.global real_exception_entry
	.type	real_exception_entry,@function

/*
 * real_exception_entry is the target of a jump from one of the 3 copies of
 * except_common_entry. It gets called whenever a synchronous or asynchronous
 * exception happens on the MIPS.
 * It is crucial that this completes very quickly.
 * Asynchronous exceptions (i.e. interrupts) are much more frequent than
 * the rest, so they need to complete the fastest.
 * Synchronous exceptions can be system calls or traps.
 * After handler jumps, k0 is set to the exception code in bits 6 to 2.
 */
real_exception_entry:
	ori	k1, zero, (0x1f << 2)
	mfc0	k0, C0_CAUSE
	and	k0, k0, k1	// k0 = exception code
	beq	zero, k0, intr_handler
	ori	k1, zero, (0x08 << 2)	// branch delay slot
	beq	k0, k1, syscall_handler	// exception code = 8 -> syscall
	nop
	beq	zero, zero, exception_handler
	nop

/* intr_handler is the target of a jump from real_exception_entry.
 * It gets called whenever an asynchronous exception happens on the MIPS.
 * It is crucial that this completes very quickly.
 */
intr_handler:
	addiu   sp, sp, -128
	sw	ra, (4*0)(sp)
	sw	fp, (4*1)(sp)
	sw	gp, (4*2)(sp)
	sw	t9, (4*3)(sp)
	sw	t8, (4*4)(sp)
	sw	s7, (4*5)(sp)
	sw	s6, (4*6)(sp)
	sw	s5, (4*7)(sp)
	sw	s4, (4*8)(sp)
	sw	s3, (4*9)(sp)
	sw	s2, (4*10)(sp)
	sw	s1, (4*11)(sp)
	sw	s0, (4*12)(sp)
	sw	t7, (4*13)(sp)
	sw	t6, (4*14)(sp)
	sw	t5, (4*15)(sp)
	sw	t4, (4*16)(sp)
	sw	t3, (4*17)(sp)
	sw	t2, (4*18)(sp)
	sw	t1, (4*19)(sp)
	sw	t0, (4*20)(sp)
	sw	a3, (4*21)(sp)
	sw	a2, (4*22)(sp)
	sw	a1, (4*23)(sp)
	sw	a0, (4*24)(sp)
	sw	v1, (4*25)(sp)
	sw	v0, (4*26)(sp)
	sw	AT, (4*27)(sp)
	mfc0	k1, C0_STATUS
	sw	k1, (4*28)(sp)
	mfc0	k1, C0_EPC
	sw	k1, (4*29)(sp)
	mflo	k1
	sw	k1, (4*30)(sp)

	mfhi	k1
	jal	C_IntHandle
	sw	k1, (4*31)(sp)	// finish saving regs (branch delay slot)

	lw	ra, (4*0)(sp)
	lw	fp, (4*1)(sp)
	lw	gp, (4*2)(sp)
	lw	t9, (4*3)(sp)
	lw	t8, (4*4)(sp)
	lw	s7, (4*5)(sp)
	lw	s6, (4*6)(sp)
	lw	s5, (4*7)(sp)
	lw	s4, (4*8)(sp)
	lw	s3, (4*9)(sp)
	lw	s2, (4*10)(sp)
	lw	s1, (4*11)(sp)
	lw	s0, (4*12)(sp)
	lw	t7, (4*13)(sp)
	lw	t6, (4*14)(sp)
	lw	t5, (4*15)(sp)
	lw	t4, (4*16)(sp)
	lw	t3, (4*17)(sp)
	lw	t2, (4*18)(sp)
	lw	t1, (4*19)(sp)
	lw	t0, (4*20)(sp)
	lw	a3, (4*21)(sp)
	lw	a2, (4*22)(sp)
	lw	a1, (4*23)(sp)
	lw	a0, (4*24)(sp)
	lw	v1, (4*25)(sp)
	lw	v0, (4*26)(sp)
	lw	AT, (4*27)(sp)
	lw	k0, (4*28)(sp)
	mtc0	k0, C0_STATUS
	lw	k0, (4*29)(sp)
	mtc0	k0, C0_EPC
	lw	k0, (4*30)(sp)
	mtlo	k0
	lw	k0, (4*31)(sp)
	mthi	k0
	addiu	sp, sp, 128
	eret
	nop

/* exception_handler is the target of a jump from real_exception_entry.
 * It gets called whenever a synchronous exception happens on the MIPS,
 * which is not a system call.
 */
exception_handler:
	addiu   sp, sp, -128
	sw	ra, (4*0)(sp)
	sw	fp, (4*1)(sp)
	sw	gp, (4*2)(sp)
	sw	t9, (4*3)(sp)
	sw	t8, (4*4)(sp)
	sw	s7, (4*5)(sp)
	sw	s6, (4*6)(sp)
	sw	s5, (4*7)(sp)
	sw	s4, (4*8)(sp)
	sw	s3, (4*9)(sp)
	sw	s2, (4*10)(sp)
	sw	s1, (4*11)(sp)
	sw	s0, (4*12)(sp)
	sw	t7, (4*13)(sp)
	sw	t6, (4*14)(sp)
	sw	t5, (4*15)(sp)
	sw	t4, (4*16)(sp)
	sw	t3, (4*17)(sp)
	sw	t2, (4*18)(sp)
	sw	t1, (4*19)(sp)
	sw	t0, (4*20)(sp)
	sw	a3, (4*21)(sp)
	sw	a2, (4*22)(sp)
	sw	a1, (4*23)(sp)
	sw	a0, (4*24)(sp)
	sw	v1, (4*25)(sp)
	sw	v0, (4*26)(sp)
	sw	AT, (4*27)(sp)
	mflo	k1
	sw	k1, (4*28)(sp)
	mfhi	k1
	sw	k1, (4*29)(sp)
	mfc0	k1, C0_EPC         //return address
	sw	k1, (4*30)(sp)
	mfc0	k1, C0_STATUS
	sw	k1, (4*31)(sp)

	move	a0, sp	// a0, parameter #1 = sp
	jal	GuruMeditation
	srl	a1, k0, 2	// a1, parameter #2 = shifted version of CAUSE

	lw	ra, (4*0)(sp)
	lw	fp, (4*1)(sp)
	lw	gp, (4*2)(sp)
	lw	t9, (4*3)(sp)
	lw	t8, (4*4)(sp)
	lw	s7, (4*5)(sp)
	lw	s6, (4*6)(sp)
	lw	s5, (4*7)(sp)
	lw	s4, (4*8)(sp)
	lw	s3, (4*9)(sp)
	lw	s2, (4*10)(sp)
	lw	s1, (4*11)(sp)
	lw	s0, (4*12)(sp)
	lw	t7, (4*13)(sp)
	lw	t6, (4*14)(sp)
	lw	t5, (4*15)(sp)
	lw	t4, (4*16)(sp)
	lw	t3, (4*17)(sp)
	lw	t2, (4*18)(sp)
	lw	t1, (4*19)(sp)
	lw	t0, (4*20)(sp)
	lw	a3, (4*21)(sp)
	lw	a2, (4*22)(sp)
	lw	a1, (4*23)(sp)
	lw	a0, (4*24)(sp)
	lw	v1, (4*25)(sp)
	lw	v0, (4*26)(sp)
	lw	AT, (4*27)(sp)
	lw	k0, (4*28)(sp)
	mtlo	k0
	lw	k0, (4*29)(sp)
	mthi	k0
	lw	k0, (4*30)(sp)
	addiu	k0, k0, 4
	mtc0	k0, C0_EPC	// return address = EPC + 4
	lw	k0, (4*31)(sp)
	mtc0	k0, C0_STATUS
	addiu	sp, sp, 128
	eret
	nop

/* syscall_handler is the target of a jump from real_exception_entry.
 * It gets called whenever a system call happens on the MIPS.
 * This should complete quickly.
 */
syscall_handler:
	addiu   sp, sp, -128
	sw	ra, (4*0)(sp)
	sw	fp, (4*1)(sp)
	sw	gp, (4*2)(sp)
	sw	t9, (4*3)(sp)
	sw	t8, (4*4)(sp)
	sw	s7, (4*5)(sp)
	sw	s6, (4*6)(sp)
	sw	s5, (4*7)(sp)
	sw	s4, (4*8)(sp)
	sw	s3, (4*9)(sp)
	sw	s2, (4*10)(sp)
	sw	s1, (4*11)(sp)
	sw	s0, (4*12)(sp)
	sw	t7, (4*13)(sp)
	sw	t6, (4*14)(sp)
	sw	t5, (4*15)(sp)
	sw	t4, (4*16)(sp)
	sw	t3, (4*17)(sp)
	sw	t2, (4*18)(sp)
	sw	t1, (4*19)(sp)
	sw	t0, (4*20)(sp)
	sw	a3, (4*21)(sp)
	sw	a2, (4*22)(sp)
	sw	a1, (4*23)(sp)
	sw	a0, (4*24)(sp)
	sw	v1, (4*25)(sp)
	sw	v0, (4*26)(sp)
	sw	AT, (4*27)(sp)
	mflo	k1
	sw	k1, (4*28)(sp)
	mfhi	k1
	sw	k1, (4*29)(sp)
	mfc0	k0, C0_EPC	// return address
	sw	k0, (4*30)(sp)
	mfc0	k1, C0_STATUS
	sw	k1, (4*31)(sp)

	lw	k1, 0(k0)	// k1 = opcode at EPC
	sll	a0, k1, 6
	sra	a0, a0, 12	// a0, parameter #1 = system call number
	move	a1, k0	// a1, parameter #2 = caller address
	jal	syscall_fun
	move	a2, sp	// a2, parameter #3 = sp (branch delay slot)

	lw	ra, (4*0)(sp)
	lw	fp, (4*1)(sp)
	lw	gp, (4*2)(sp)
	lw	t9, (4*3)(sp)
	lw	t8, (4*4)(sp)
	lw	s7, (4*5)(sp)
	lw	s6, (4*6)(sp)
	lw	s5, (4*7)(sp)
	lw	s4, (4*8)(sp)
	lw	s3, (4*9)(sp)
	lw	s2, (4*10)(sp)
	lw	s1, (4*11)(sp)
	lw	s0, (4*12)(sp)
	lw	t7, (4*13)(sp)
	lw	t6, (4*14)(sp)
	lw	t5, (4*15)(sp)
	lw	t4, (4*16)(sp)
	lw	t3, (4*17)(sp)
	lw	t2, (4*18)(sp)
	lw	t1, (4*19)(sp)
	lw	t0, (4*20)(sp)
	lw	a3, (4*21)(sp)
	lw	a2, (4*22)(sp)
	lw	a1, (4*23)(sp)
	lw	a0, (4*24)(sp)
	lw	v1, (4*25)(sp)
	lw	v0, (4*26)(sp)
	lw	AT, (4*27)(sp)
	lw	k0, (4*28)(sp)
	mtlo	k0
	lw	k0, (4*29)(sp)
	mthi	k0
	lw	k0, (4*30)(sp)
	addiu	k0, k0, 4
	mtc0	k0, C0_EPC	// return address = EPC + 4
	lw	k0, (4*31)(sp)
	mtc0	k0, C0_STATUS
	addiu	sp, sp, 128
	eret
	nop

	.end	real_exception_entry
