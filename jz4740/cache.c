/*
 * cache.c
 *
 * Handle operation of Jz CPU's cache
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
#define u32	unsigned int

#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Load_Tag_I	0x04
#define Index_Load_Tag_D	0x05
#define Index_Store_Tag_I	0x08
#define Index_Store_Tag_D	0x09
#define Hit_Invalidate_I	0x10
#define Hit_Invalidate_D	0x11
#define Hit_Writeback_Inv_D	0x15
#define Hit_Writeback_I		0x18
#define Hit_Writeback_D		0x19

#define CACHE_SIZE		16*1024
#define CACHE_LINE_SIZE		32
#define KSEG0			0x80000000

#define K0_TO_K1()				\
do {						\
	unsigned long __k0_addr;		\
						\
	__asm__ __volatile__(			\
	"la %0, 1f\n\t"				\
	"or	%0, %0, %1\n\t"			\
	"jr	%0\n\t"				\
	"nop\n\t"				\
	"1: nop\n"				\
	: "=&r"(__k0_addr)			\
	: "r" (0x20000000) );			\
} while(0)

#define K1_TO_K0()				\
do {						\
	unsigned long __k0_addr;		\
	__asm__ __volatile__(			\
	"nop;nop;nop;nop;nop;nop;nop\n\t"	\
	"la %0, 1f\n\t"				\
	"jr	%0\n\t"				\
	"nop\n\t"				\
	"1:	nop\n"				\
	: "=&r" (__k0_addr));			\
} while (0)

#define INVALIDATE_BTB()			\
do {						\
	unsigned long tmp;			\
	__asm__ __volatile__(			\
	".set mips32\n\t"			\
	"mfc0 %0, $16, 7\n\t"			\
	"nop\n\t"				\
	"ori %0, 2\n\t"				\
	"mtc0 %0, $16, 7\n\t"			\
	"nop\n\t"				\
	".set mips2\n\t"			\
	: "=&r" (tmp));				\
} while (0)

#define SYNC_WB() __asm__ __volatile__ ("sync")

#define cache_op(op,addr)						\
	__asm__ __volatile__(						\
	"	.set	noreorder		\n"			\
	"	.set	mips32\n\t		\n"			\
	"	cache	%0, %1			\n"			\
	"	.set	mips0			\n"			\
	"	.set	reorder"					\
	:										\
	: "i" (op), "m" (*(unsigned char *)(addr)))

void __flush_dcache_line(unsigned long addr)
{
	cache_op(Hit_Writeback_Inv_D, addr);
	SYNC_WB();
}

void __icache_invalidate_all(void)
{
	u32 i;

	K0_TO_K1();

	asm volatile (".set noreorder\n"
		      ".set mips32\n\t"
		      "mtc0\t$0,$28\n\t"
		      "mtc0\t$0,$29\n"
		      ".set mips0\n"
		      ".set reorder\n");
	for (i=KSEG0;i<KSEG0+CACHE_SIZE;i+=CACHE_LINE_SIZE)
		cache_op(Index_Store_Tag_I, i);

	K1_TO_K0();

	INVALIDATE_BTB();
}

void __dcache_invalidate_all(void)
{
	u32 i;

	asm volatile (".set noreorder\n"
		      ".set mips32\n\t"
		      "mtc0\t$0,$28\n\t"
		      "mtc0\t$0,$29\n"
		      ".set mips0\n"
		      ".set reorder\n");
	for (i=KSEG0;i<KSEG0+CACHE_SIZE;i+=CACHE_LINE_SIZE)
		cache_op(Index_Store_Tag_D, i);
}

void __dcache_writeback_all(void)
{
	u32 i;
	for (i=KSEG0;i<KSEG0+CACHE_SIZE;i+=CACHE_LINE_SIZE)
		cache_op(Index_Writeback_Inv_D, i);
	SYNC_WB();
}

void dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= CACHE_SIZE) {
		__dcache_writeback_all();
	}
	else {
		unsigned long dc_lsize = CACHE_LINE_SIZE;

		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			__flush_dcache_line(a);	/* Hit_Writeback_Inv_D */
			if (a == end)
				break;
			a += dc_lsize;
		}
	}
}
