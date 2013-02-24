/**************************************************************************
*                                                                         *
*   PROJECT     : TMON (Transparent monitor)                              *
*                                                                         *
*   MODULE      : LIBC_S.s                                                *
*                                                                         *
*   AUTHOR      : Michael Anburaj                                         *
*                 URL  : http://geocities.com/michaelanburaj/             *
*                 EMAIL: michaelanburaj@hotmail.com                       *
*                                                                         *
*   PROCESSOR   : MIPS 4Kc (32 bit RISC) - ATLAS board                    *
*                                                                         *
*   TOOL-CHAIN  : SDE & Cygnus                                            *
*                                                                         *
*   DESCRIPTION :                                                         *
*   This is the LIBC_S module. Implements some of the standard library    *
*   functions.                                                            *
*                                                                         *
**************************************************************************/

#include "sysdefs.h"
#include "mips.h"

	
/* ********************************************************************* */
/* Global definitions */


/* ********************************************************************* */
/* File local definitions */

/*  This routine could be optimized for MIPS64. The current code only
 *  uses MIPS32 instructions.
 */	

#ifdef EB
#  define LWHI	lwl		/* high part is left in big-endian	*/
#  define SWHI	swl		/* high part is left in big-endian	*/
#  define LWLO	lwr		/* low part is right in big-endian	*/
#  define SWLO	swr		/* low part is right in big-endian	*/
#else
#  define LWHI	lwr		/* high part is right in little-endian	*/
#  define SWHI	swr		/* high part is right in little-endian	*/
#  define LWLO	lwl		/* low part is left in big-endian	*/
#  define SWLO	swl		/* low part is left in big-endian	*/
#endif


/* ********************************************************************* */
/* Local functions */


/* ********************************************************************* */
/* Global functions */

/*
*********************************************************************************************
*                                       memcpy
*
* Description: Look in the standard headers.
*
* Arguments  : 
*
* Return     : 
*
* Note(s)    : 
*********************************************************************************************
*/
	
LEAF( memcpy )

	.set	noreorder
	.set	noat

	slti	AT,a2,8
	bne	AT,zero,last8
	move	v0,a0
	xor	t2,a1,a0
	andi	t2,t2,0x3		# a0/a1 displacement
	bne	t2,zero,shift
	subu	v1,zero,a1
	andi	v1,v1,0x3
	beq	v1,zero,chk8w
	subu	a2,a2,v1
	LWHI	t2,0(a1)
	addu	a1,a1,v1
	SWHI	t2,0(a0)
	addu	a0,a0,v1
chk8w:	andi	t2,a2,0x1f
	beq	a2,t2,chk1w
	subu	a3,a2,t2
	addu	a3,a3,a1
loop8w:	lw	t7,0(a1)
	lw	v1,4(a1)
	lw	t0,8(a1)
	lw	t1,12(a1)
	lw	t3,16(a1)
	lw	t4,20(a1)
	lw	t5,24(a1)
	lw	t6,28(a1)
	sw	t7,0(a0)
	sw	v1,4(a0)
	sw	t0,8(a0)
	sw	t1,12(a0)
	sw	t3,16(a0)
	addiu	a1,a1,32
	sw	t4,20(a0)
	sw	t5,24(a0)
	addiu	a0,a0,32
	bne	a1,a3,loop8w
	sw	t6,-4(a0)
	move	a2,t2
chk1w:	andi	t2,a2,0x3
	beq	a2,t2,last8
	subu	a3,a2,t2
	addu	a3,a3,a1
loop1w:	lw	t3,0(a1)
	addiu	a1,a1,4
	addiu	a0,a0,4
	bne	a1,a3,loop1w
	sw	t3,-4(a0)
	move	a2,t2
last8:	blez	a2,last8e
	addu	a3,a2,a1
last8l:	lb	t2,0(a1)
	addiu	a1,a1,1
	addiu	a0,a0,1
	bne	a1,a3,last8l
	sb	t2,-1(a0)
last8e:	j	ra
	nop
shift:	subu	a3,zero,a0
	andi	a3,a3,0x3
	beq	a3,zero,shift1
	subu	a2,a2,a3	# bytes left
	LWHI	t2,0(a1)
	LWLO	t2,3(a1)
	addu	a1,a1,a3
	SWHI	t2,0(a0)
	addu	a0,a0,a3
shift1:	andi	t2,a2,0x3
	subu	a3,a2,t2
	addu	a3,a3,a1
shifth:	LWHI	t3,0(a1)
	LWLO	t3,3(a1)
	addiu	a1,a1,4
	addiu	a0,a0,4
	bne	a1,a3,shifth
	sw	t3,-4(a0)
	b	last8
	move	a2,t2

	.set	at
	.set	reorder

END( memcpy )

/*
*********************************************************************************************
*                                       memset
*
* Description: Look in the standard headers.
*
* Arguments  : 
*
* Return     : 
*
* Note(s)    : 
*********************************************************************************************
*/

LEAF( memset )

	.set	noreorder
	.set	noat

	andi	a1, 0xff		# To avoid problems when extending
	slti	AT,a2,8
	bne	AT,zero,end8         #  last8		 
	move	v0,a0
	beq	a1,zero,uneven
	subu	v1,zero,a0
	sll	t0,a1,8
	or	a1,a1,t0
	sll	t0,a1,16
	or	a1,a1,t0
uneven:	andi	v1,v1,0x3		# unaligned address?
	beq	v1,zero,chkw
	subu	a2,a2,v1
	SWHI	a1,0(a0)
	addu	a0,a0,v1
chkw:	andi	t0,a2,0x7
	beq	a2,t0,chkl
	subu	a3,a2,t0
	addu	a3,a3,a0
loopw:	addiu	a0,a0,8
	sw	a1,-8(a0)
	bne	a0,a3,loopw
	sw	a1,-4(a0)
	move	a2,t0
chkl:	andi	t0,a2,0x4
	beq	t0,zero,end8
	subu	a2,a2,t0
	sw	a1,0(a0)
	addiu	a0,a0,4
end8:	blez	a2,end8e
	addu	a3,a2,a0
end8l:	addiu	a0,a0,1
	bne	a0,a3,end8l
	sb	a1,-1(a0)
end8e:	j	ra
	nop

	.set	at
	.set	reorder

END( memset )
	
	
/* ********************************************************************* */
