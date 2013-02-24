/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : SYS_ARCH.h                                              *
*                                                                         *
*   AUTHOR      : Michael Anburaj                                         *
*                 URL  : http://geocities.com/michaelanburaj/             *
*                 EMAIL: michaelanburaj@hotmail.com                       *
*                                                                         *
*   PROCESSOR   : Any                                                     *
*                                                                         *
*   TOOL-CHAIN  : Any                                                     *
*                                                                         *
*   DESCRIPTION :                                                         *
*   Interface header file for sys_arch.                                   *
*                                                                         *
**************************************************************************/

#ifndef __SYS_ARCH__H__
#define __SYS_ARCH__H__

//#include    "os_cpu.h"
//#include    "os_cfg.h"
#include    "includes.h"
#include    "arch/sys_arch_opts.h"

#define SYS_MBOX_NULL   NULL
#define SYS_SEM_NULL    NULL

typedef struct {
    OS_EVENT*   pQ;
    void*       pvQEntries[LWIP_Q_SIZE];
} TQ_DESCR, *PQ_DESCR;
    
typedef OS_EVENT* sys_sem_t;
typedef PQ_DESCR  sys_mbox_t;
typedef INT8U     sys_thread_t;


#endif /* __SYS_ARCH__H__ */

