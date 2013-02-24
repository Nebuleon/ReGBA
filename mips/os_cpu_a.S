/*
*********************************************************************************************************
*                                               uC/OS-II
*                                        The Real-Time Kernel
*
*                         (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                          All Rights Reserved
*
* File : OS_CPU_A.S
* By   : Jean J. Labrosse
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MIPS Port
*
*                 Target           : MIPS (Includes 4Kc)
*                 Ported by        : Michael Anburaj
*                 URL              : http://geocities.com/michaelanburaj/    Email : michaelanburaj@hotmail.com
*
*********************************************************************************************************
*/

#include "sysdefs.h"
#include "mips.h"
#include "frmwrk.h" /* for _EXC_STKSIZE */
#include "context.h"


/* ********************************************************************* */
/* Global definitions */


/* ********************************************************************* */
/* File local definitions */


/* ********************************************************************* */
/* Local functions */


/* ********************************************************************* */
/* Global functions */

      .set noreorder

/*
**********************************************************************************************************
*                                          START MULTITASKING
*                                       void OSStartHighRdy(void)
*
* Note : OSStartHighRdy() MUST:
*           a) Call OSTaskSwHook() then,
*           b) Set OSRunning to TRUE,
*           c) Switch to the highest priority task.
**********************************************************************************************************
*/

LEAF(OSStartHighRdy)

        jal OSTaskSwHook            /* Call user defined task switch hook */
        nop

        la t0,OSRunning             /* Indicate that multitasking has started */
        li t1,1
        sb t1,0(t0)                 /* OSRunning is a boolean */

        la t0,OSTCBHighRdy          /* Get highest priority task TCB address */

        lw t0,0(t0)                 /* get stack pointer */
        lw sp,0(t0)                 /* switch to the new stack */

        RESTORE_REG_RET()           /* pop new task's at, v0-v1,a0-a3,t0-t9,s0-s7,fp,ra, & pc */

END(OSStartHighRdy)

/*
**********************************************************************************************************
*                                PERFORM A CONTEXT SWITCH (From task level)
*                                           void OSCtxSw(void)
*
* Note(s):    Upon entry, 
*             OSTCBCur     points to the OS_TCB of the task to suspend
*             OSTCBHighRdy points to the OS_TCB of the task to resume
*
**********************************************************************************************************
*/

LEAF(OSCtxSw)
/* Special optimised code below: */
        STORE_REG_RET(ra)               /* push task's at, v0-v1,a0-a3,t0-t9,s0-s7,fp,ra, & pc */

        /* OSPrioCur = OSPrioHighRdy */
        la s0,OSPrioCur
        la s1,OSPrioHighRdy
        lb s2,0(s1)
        sb s2,0(s0)
        
        /* Get current task TCB address */
        la s0,OSTCBCur
        lw s1,0(s0)
        sw sp,0(s1)                 /* store sp in preempted tasks's TCB */

        jal OSTaskSwHook            /* call Task Switch Hook */
        nop

        /* Get highest priority task TCB address */
        la s1,OSTCBHighRdy
        lw s1,0(s1)
        lw sp,0(s1)                 /* get new task's stack pointer */

        /* OSTCBCur = OSTCBHighRdy */
        sw s1,0(s0)                /* set new current task TCB address */

        /* TBI */ /*RESTORE_REG_ERET()*/             /* pop new task's at, v0-v1,a0-a3,t0-t9,s0-s7,fp,ra, & pc */
		RESTORE_REG_ERET()
END(OSCtxSw)

/*
**********************************************************************************************************
*                                PERFORM A CONTEXT SWITCH (From an ISR)
*                                        void OSIntCtxSw(void)
*
* Note(s): This function only flags a context switch to the ISR Handler
*
**********************************************************************************************************
*/

LEAF(OSIntCtxSw)

        /* OSIntCtxSwFlag = True */
        la t0,OSIntCtxSwFlag
        li t1,1
        sw t1,0(t0)
        jr ra
        nop
END(OSIntCtxSw)
        
/*
**********************************************************************************************************
*                                            UCOS_INTHandler
*
*        This handles all the INTs
*
**********************************************************************************************************
*/
    BSS
    
    /* 32 bit align */
    ALIGN(2)

    .globl exc_stack_low
exc_stack_low:
    .space _EXC_STKSIZE
    .globl exc_stack_hi
exc_stack_hi:

    ALIGN(4) /* 16 bytes align */
exc_context:
    .space    8

LEAF(UCOS_INTHandler)

        MFC0(k0,C0_EPC)        
        STORE_REG_RET(k0)

        la k1, exc_context
        MFC0(t0, C0_STATUS)
        sw t0,0(k1)
        MFC0(t0, C0_CAUSE)
        sw t0,4(k1)
        
        move k0,sp

        /* call low level exception handler */
        /*  Set up sp and gp to YAMON value */
        la gp, _gp

        la sp,exc_stack_hi
        jal OSIntEnter /* It is imperative that k1 survives */
        nop
        
        jal C_vINTHandler
        move a0,k1
        
        jal OSIntExit
        nop

        la t0,OSIntCtxSwFlag
        lw t1,0(t0)
	xori t1, t1, 1
        beqz t1,_IntCtxSw
        nop

        move sp,k0
        RESTORE_REG_ERET()


_IntCtxSw:
        sw zero,0(t0)

        /* OSPrioCur = OSPrioHighRdy */
        la t0,OSPrioCur
        la t1,OSPrioHighRdy
        lb t1,0(t1)
        sb t1,0(t0)
        
        /* Get current task TCB address */
        la t0,OSTCBCur
        lw t1,0(t0)
        sw k0,0(t1)                 /* store sp in preempted tasks's TCB */

        jal OSTaskSwHook            /* call Task Switch Hook */
        nop

        /* Get highest priority task TCB address */
        la t2,OSTCBHighRdy
        lw t2,0(t2)
        lw sp,0(t2)                 /* get new task's stack pointer

        /* OSTCBCur = OSTCBHighRdy */
        sw t2,0(t0)                 /* set new current task TCB address */

        RESTORE_REG_ERET()

END(UCOS_INTHandler)

/*
**********************************************************************************************************
*                                   CRITICAL SECTION METHOD 3 FUNCTIONS
*
* Description: Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
*              would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
*              disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to 
*              disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
*              into the CPU's status register.
*
*              OS_CPU_SR OSCPUSaveSR()
* Arguments  : none
*
* Returns    : OS_CPU_SR
*
*              OSCPURestoreSR(OS_CPU_SR cpu_sr)
* Arguments  : OS_CPU_SR
*
* Returns    : none
*
* Note(s)    : These functions are used in general like this,
*
*            void Task (void *data)
*            {
*                    #if OS_CRITICAL_METHOD == 3 // Allocate storage for CPU status register
*                          OS_CPU_SR  cpu_sr;
*                    #endif
*                         :
*                         :
*                    OS_ENTER_CRITICAL(); // cpu_sr = OSCPUSaveSR();
*                         :
*                         :
*                    OS_EXIT_CRITICAL();  // OSCPURestoreSR(cpu_sr);
*                         :
*                         :
*            }
**********************************************************************************************************
*/

LEAF(OSCPUSaveSR)

        MFC0(v0,C0_STATUS)
        and v1,v0,0xfffffffe
        MTC0(v1,C0_STATUS)
        jr ra
        nop
END(OSCPUSaveSR)  

LEAF(OSCPURestoreSR)

        MTC0(a0,C0_STATUS)
        jr ra
        nop
END(OSCPURestoreSR)

/*
*********************************************************************************************
*                                       CP0_wGetSR
*
* Description: Reads CP0-status.
*
* Arguments  : none.
*
* Return     : CP0-status.
*
* Note(s)    : 
*********************************************************************************************
*/

LEAF(CP0_wGetSR)
        MFC0(v0,C0_STATUS)
        jr ra
        nop
END(CP0_wGetSR)  


/*
*********************************************************************************************
*                                       CP0_vEnableIM
*
* Description: Enable specific interrupt: set IM[x] bit in CP0-status.
*
* Arguments  : Interrupt number:
*              C0_STATUS_IM_SW0, C0_STATUS_IM_SW1, C0_STATUS_IM_HW0 - C0_STATUS_IM_HW5
*
* Return     : none.
*
* Note(s)    : 
*********************************************************************************************
*/

LEAF(CP0_vEnableIM)

    MFC0(   v0, C0_STATUS)
	move		t0, a0
    addiu   a0, C0_STATUS_IM_SHF
    sllv    t0, t0, a0
    or      v0, t0
    MTC0(   v0, C0_STATUS)
    j       ra
    nop

END(CP0_vEnableIM)


LEAF(CP0_wGet_PRId)
	MFC0(v0,C0_PRId)
	jr ra
	nop
END(CP0_wGet_PRId)  


LEAF(CP0_wGet_COUNT)
    MFC0(   v0, C0_COUNT)
    j       ra
    nop
END(CP0_wGet_COUNT)


LEAF(CP0_wGet_COMPARE)
    MFC0(   v0, C0_COMPARE)
    j       ra
    nop
END(CP0_wGet_COMPARE)


LEAF(CP0_wSet_COMPARE)
    MTC0(   a0, C0_COMPARE)
    j       ra
    nop
END(CP0_wSet_COMPARE)


LEAF(CP0_wGetCAUSE)
        MFC0(v0,C0_CAUSE)
        jr ra
        nop
END(CP0_wGetCAUSE) 

LEAF(OSEnableInt)
        MFC0(v0,C0_STATUS)
        or v1, v0, 1<<S_StatusIE
        MTC0(v1,C0_STATUS)
        jr ra
        nop
END(OSEnableInt)




/* ********************************************************************* */
