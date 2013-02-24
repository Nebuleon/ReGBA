/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : SYS_ARCH.c                                              *
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
*   sys_arch code file.                                                   *
*                                                                         *
**************************************************************************/

#include <stdlib.h>
#include <lwipopts.h>
#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"

#include "arch/sys_arch.h"

struct timeoutlist {
  struct sys_timeouts timeouts;
  INT8U prio;
};

static struct timeoutlist timeoutlist[LWIP_MAX_TASKS];

/* 
 * Timeout for threads which were not created by sys_thread_new
 * -- usually uC/OS-II's IdealTask.
 */ 
struct sys_timeouts timeouts;

const void * const pvNullPointer;

static OS_MEM *pQueueMem;
static char pcQueueMemoryPool[LWIP_MAX_QS * sizeof(TQ_DESCR) +3];

OS_STK sys_stack[LWIP_MAX_TASKS][LWIP_STACK_SIZE];
static INT8U sys_thread_no;

/*-----------------------------------------------------------------------------------*/
sys_mbox_t
sys_mbox_new(void)
{
    INT8U       ucErr;
    PQ_DESCR    pQDesc;

    pQDesc = OSMemGet( pQueueMem, &ucErr );
    LWIP_ASSERT( "OSMemGet ", ucErr == OS_NO_ERR );
    if( &(pQDesc->pvQEntries[0])==NULL) printf(" &(pQDesc->pvQEntries[0])==NULL");
    if( ucErr == OS_NO_ERR )
    {
        pQDesc->pQ = OSQCreate( &(pQDesc->pvQEntries[0]), LWIP_Q_SIZE );
        LWIP_ASSERT( "OSQCreate ", pQDesc->pQ != NULL );
        if( pQDesc->pQ != NULL )
        {
            return pQDesc;
        }
    }
    return SYS_MBOX_NULL;
}

/*-----------------------------------------------------------------------------------*/
void
sys_mbox_free(sys_mbox_t mbox)
{
    INT8U     ucErr;
    
    LWIP_ASSERT( "sys_mbox_free ", mbox != SYS_MBOX_NULL );   
    OSQFlush( mbox->pQ );

    (void)OSQDel( mbox->pQ, OS_DEL_NO_PEND, &ucErr);
    LWIP_ASSERT( "OSQDel ", ucErr == OS_NO_ERR );         

    ucErr = OSMemPut( pQueueMem, mbox );
    LWIP_ASSERT( "OSMemPut ", ucErr == OS_NO_ERR );
}

/*-----------------------------------------------------------------------------------*/
void
sys_mbox_post(sys_mbox_t mbox, void *msg)
{
    if( !msg )
    msg = (void*)&pvNullPointer;

    if(OSQPost( mbox->pQ, msg) != OS_NO_ERR)
    {
        //printf("Queue post error\n");
        //while(1);
    }
}

/*-----------------------------------------------------------------------------------*/
u32_t
sys_arch_mbox_fetch(sys_mbox_t mbox, void **msg, u32_t timeout)
{
    INT8U ucErr;
    INT32U ucos_timeout;
    void *temp;

    /* convert LwIP timeout (in milliseconds) to uC/OS-II timeout (in OS_TICKS) */
    if(timeout)
    {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;

        if(ucos_timeout < 1)
            ucos_timeout = 1;
        else if(ucos_timeout > 65535)
            ucos_timeout = 65535;
    }
    else
    {
        ucos_timeout = 0;
    }

    temp = OSQPend( mbox->pQ, ucos_timeout, &ucErr );
    if(msg)
    {
        if( temp == (void*)&pvNullPointer )
        {
            *msg = NULL;
        }
        else
        {
            *msg = temp;
        }
    }
    
    if( ucErr == OS_TIMEOUT )
    {
        timeout = SYS_ARCH_TIMEOUT;
    }
    else
    {
        LWIP_ASSERT( "OSQPend ", ucErr == OS_NO_ERR );
        /* Calculate time we waited for the message to arrive. */      
        /* XXX: we cheat and just pretend that we waited for long! */
        timeout = 1;
    }

    return timeout;
}

/*-----------------------------------------------------------------------------------*/
sys_sem_t
sys_sem_new(u8_t count)
{
    sys_sem_t pSem;
    pSem = OSSemCreate( (INT16U)count );
    LWIP_ASSERT( "OSSemCreate ", pSem != NULL );
    return pSem;
}

/*-----------------------------------------------------------------------------------*/
u32_t
sys_arch_sem_wait(sys_sem_t sem, u32_t timeout)
{
    INT8U  ucErr;
    INT32U ucos_timeout;

    /* convert LwIP timeout (in milliseconds) to uC/OS-II timeout (in OS_TICKS) */
    if(timeout)
    {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC)/1000;

        if(ucos_timeout < 1)
            ucos_timeout = 1;
        else if(ucos_timeout > 65535)
            ucos_timeout = 65535;
    }
    else
    {
        ucos_timeout = 0;
    }

    OSSemPend( sem, ucos_timeout, &ucErr );        
    if( ucErr == OS_TIMEOUT )
    {
        timeout = SYS_ARCH_TIMEOUT;
    }
    else
    {
        //Semaphore is used for pbuf_free, which could get called from an ISR
        //LWIP_ASSERT( "OSSemPend ", ucErr == OS_NO_ERR );

        /* Calculate time we waited for the message to arrive. */      
        /* XXX: we cheat and just pretend that we waited for long! */
        timeout = 1;
    }
    return timeout;
}

/*-----------------------------------------------------------------------------------*/
void
sys_sem_signal(sys_sem_t sem)
{
    INT8U     ucErr;
    ucErr = OSSemPost( sem );
 
    /* It may happen that a connection is already reset and the semaphore is deleted
       if this function is called. Therefore ASSERTION should not be called */   
    //LWIP_ASSERT( "OSSemPost ", ucErr == OS_NO_ERR );
}

/*-----------------------------------------------------------------------------------*/
void
sys_sem_free(sys_sem_t sem)
{
    INT8U     ucErr;
    
    (void)OSSemDel( sem, OS_DEL_NO_PEND, &ucErr );
    
    LWIP_ASSERT( "OSSemDel ", ucErr == OS_NO_ERR );
}
/*-----------------------------------------------------------------------------------*/
void
sys_init(void)
{
    INT8U   ucErr;

    pQueueMem = OSMemCreate( (void*)((U32)((U32)pcQueueMemoryPool+3) & ~3), LWIP_MAX_QS, sizeof(TQ_DESCR), &ucErr );
    LWIP_ASSERT( "OSMemCreate ", ucErr == OS_NO_ERR );

    sys_thread_no = 0;
}

/*-----------------------------------------------------------------------------------*/
struct sys_timeouts *
sys_arch_timeouts(void)
{
    INT8U i;
    struct timeoutlist *tl;

    for(i = 0; i < sys_thread_no; i++)
    {
        tl = &timeoutlist[i];
        if(tl->prio == OSPrioCur)
        {
            return &(tl->timeouts);
        }
    }

    return &timeouts;
}

/*-----------------------------------------------------------------------------------*/
sys_thread_t
sys_thread_new(void (* function)(void *arg), void *arg, int prio) 
{
    INT8U bTemp;

    if(sys_thread_no >= LWIP_MAX_TASKS)
    {
        printf("sys_thread_new: Max Sys. Tasks reached\n");
        return 0;
    }
    
    timeoutlist[sys_thread_no].timeouts.next = NULL;
    timeoutlist[sys_thread_no].prio = prio;

    ++sys_thread_no; /* next task created will be one lower to this one */

#if (STACK_PROFILE_EN == 1)
    if(bTemp = OSTaskCreateExt( function, arg, &sys_stack[sys_thread_no - 1][LWIP_STACK_SIZE - 1], prio, sys_thread_no - 1,
       &sys_stack[sys_thread_no - 1][0], LWIP_STACK_SIZE, (void *)0, OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR))
#else
    if(bTemp = OSTaskCreate( function, arg, &sys_stack[sys_thread_no - 1][LWIP_STACK_SIZE - 1], prio ))
#endif
    {
        printf("sys_thread_new: Task creation error (prio=%d) [%d]\n",prio,bTemp);
        --sys_thread_no;
        while(1);
    }
    return sys_thread_no;
}

