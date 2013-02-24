/*
 *  Startup Code for MIPS32 CPU-core
 *
 *  Copyright (c) 2003	Wolfgang Denk <wd@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <pavo.h>

#include <mips.h>
#include <mipsregs.h> 
#include <archdefs.h>
#include <cacheops.h>

	
	
#include <jz4740.h>
			
#define RVECENT(f,n) \
   b f; nop
#define XVECENT(f,bev) \
   b f     ;           \
   li k0,bev

	.set noreorder

	.globl _start
	.text
_start:
	.word JZ4740_NANDBOOT_CFG	/* fetched during NAND Boot */
reset:
	/* 
	 * CU0=UM=EXL=IE=0, BEV=ERL=1, IP2~7=1
	 */
	
	//li	t0, 0x0040FC04
	//mtc0	t0, CP0_STATUS

	/* CAUSE register */
	/* IV=1, use the specical interrupt vector (0x200) */
	//li	t1, 0x00800000
	//mtc0	t1, CP0_CAUSE
	/* Initialize GOT pointer.
	*/
	bal     1f
	nop
	
1:
	
	move    gp, ra
	lw      t1, 0(ra)
	move	gp, t1

	/* Set up temporary stack.
	 */
	la	sp, 0x80004000
	la	t9, nand_boot
	j	t9
	nop
	.set reorder
