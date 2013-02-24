/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : SYS_ARCH_OPTS.h                                         *
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
*   Module configuration for sys_arch.                                    *
*                                                                         *
**************************************************************************/


#ifndef __SYS_ARCH_OPTS_H__
#define __SYS_ARCH_OPTS_H__


/* ********************************************************************* */
/* Module configuration */

#define LWIP_MAX_TASKS  4          /* Number of LwIP tasks */
#define LWIP_STACK_SIZE 512*10 //4K     /* Stack size for LwIP tasks */
/* Note: 
   Task priorities, LWIP_START_PRIO through (LWIP_START_PRIO+LWIP_MAX_TASKS-1) must be reserved
   for LwIP and must not used by other applications outside. */

#define LWIP_Q_SIZE 10              /* LwIP queue size */
#define LWIP_MAX_QS 20              /* Max. LwIP queues */


/* ********************************************************************* */
/* Interface macro & data definition */


/* ********************************************************************* */
/* Interface function definition */


#endif /* __SYS_ARCH_OPTS_H__ */

