/*
*********************************************************************************************************
*                                               uC/OS-II
*                                        The Real-Time Kernel
*
*                         (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                          All Rights Reserved
*
* File         : OS_CPU.H
* By           : Jean J. Labrosse
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

#ifndef __OS_CPU_H__
#define __OS_CPU_H__

#include "sysdefs.h"


#ifdef  OS_CPU_GLOBALS
#define OS_CPU_EXT
#else
#define OS_CPU_EXT  extern
#endif

/*
*********************************************************************************************************
*                                              DATA TYPES
*                                         (Compiler Specific)
*********************************************************************************************************
*/

typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned int   INT16U;                   /* Unsigned 16 bit quantity                           */
typedef signed   int   INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned long  INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   long  INT32S;                   /* Signed   32 bit quantity                           */
typedef float          FP32;                     /* Single precision floating point                    */
typedef double         FP64;                     /* Double precision floating point                    */

typedef unsigned int   OS_STK;                   /* Each stack entry is 16-bit wide                    */
typedef unsigned int   OS_CPU_SR;                /* Define size of CPU status register (PSR = 32 bits) */

#define BYTE           INT8S                     /* Define data types for backward compatibility ...   */
#define UBYTE          INT8U                     /* ... to uC/OS V1.xx.  Not actually needed for ...   */
#define WORD           INT16S                    /* ... uC/OS-II.                                      */
#define UWORD          INT16U
#define LONG           INT32S
#define ULONG          INT32U

/* 
*********************************************************************************************************
*                              MIPS
*
* Method #1:  Disable/Enable interrupts using simple instructions.  After critical section, interrupts
*             will be enabled even if they were disabled before entering the critical section.
*
* Method #2:  Disable/Enable interrupts by preserving the state of interrupts.  In other words, if 
*             interrupts were disabled before entering the critical section, they will be disabled when
*             leaving the critical section.
*
* Method #3:  Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
*             would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
*             disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to 
*             disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
*             into the CPU's status register.
*
* Note     :  In this ARM7 Port: Only Method #3 is implemeted.
*
*********************************************************************************************************
*/
/* Don't modify these lines. This port can only support OS_CRITICAL_METHOD 3. */
#define  OS_CRITICAL_METHOD    3

#if      OS_CRITICAL_METHOD == 3
#define  OS_ENTER_CRITICAL()  (cpu_sr = OSCPUSaveSR())    /* Disable interrupts                        */
#define  OS_EXIT_CRITICAL()   (OSCPURestoreSR(cpu_sr))    /* Restore  interrupts                       */
#endif

/*
*********************************************************************************************************
*                           ARM Miscellaneous
*********************************************************************************************************
*/

#define  OS_STK_GROWTH        1                       /* Stack grows from HIGH to LOW memory on 80x86  */

#define  OS_TASK_SW()         OSCtxSw()

/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                              PROTOTYPES
*********************************************************************************************************
*/

void UCOS_INTHandler(void);
void OSCtxSw(void);
void OSIntCtxSw(void);

#if OS_CRITICAL_METHOD == 3                      /* Allocate storage for CPU status register           */
OS_CPU_SR  OSCPUSaveSR(void);
void       OSCPURestoreSR(OS_CPU_SR cpu_sr);
#endif

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
INT32U CP0_wGetSR(void);

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
void CP0_vEnableIM(U32 wCP0_IM);

#endif /*__OS_CPU_H__*/
