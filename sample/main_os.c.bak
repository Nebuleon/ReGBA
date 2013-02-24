/**************************************************************************
*                                                                         *
*   PROJECT     : MIPS port for uC/OS-II                                  *
*                                                                         *
*   MODULE      : EX1.c                                                   *
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
*   This is a sample code (Ex1) to test uC/OS-II.                         *
*                                                                         *
**************************************************************************/

/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"
#include <stdio.h>


/* ********************************************************************* */
/* Global definitions */


/* ********************************************************************* */
/* File local definitions */
//#define  TASK_START_PRIO 12
#define  TASK_STK_SIZE   1024                      /* Size of each task's stacks (# of WORDs) */
#define  NO_TASKS        1                      /* Number of identical tasks */

OS_STK   TaskStk[NO_TASKS][TASK_STK_SIZE];      /* Tasks stacks */
OS_STK   TaskStartStk[TASK_STK_SIZE];
char     TaskData[NO_TASKS];                    /* Parameters to pass to each task */


/* ********************************************************************* */
/* Local functions */

void Task (void *data)
{
        U8 err;

        while(1) {

		printf("Task%c\n", *(char *)data);
			OSTimeDly(3000);                   /* Delay 3000 clock tick */
        }
}

void TaskStart (void *data)
{
        U8 i;
        char key;

        data = data;                            /* Prevent compiler warning */
        JZ_StartTicker(OS_TICKS_PER_SEC);	/* os_cfg.h */
        printf("uC/OS-II, The Real-Time Kernel MIPS Ported version\n");
        printf("EXAMPLE #1\n");

        printf("Determining  CPU's capacity ...\n");


        OSStatInit();                           /* Initialize uC/OS-II's statistics */

        for (i = 0; i < NO_TASKS; i++)
        {                                       /* Create NO_TASKS identical tasks */
                TaskData[i] = '0' + i;          /* Each task will display its own letter */
                printf("#%d",i);
		OSTaskCreate(Task, (void *)&TaskData[i], (void *)&TaskStk[i][TASK_STK_SIZE - 1], i + 1);
        }

        printf("\n# Parameter1: No. Tasks\n");
        printf("# Parameter2: CPU Usage in %%\n");
        printf("# Parameter3: No. Task switches/sec\n");
        printf("<-PRESS 'ESC' TO QUIT->\n");
        while(1)
        {
   

	     key = serial_getc();
	     printf("you pressed: %c\n",key);
            
	     if(key == 0x1B) {        /* see if it's the ESCAPE key */
		  printf(" Escape display of statistic\n");
		  OSTaskDel(TASK_START_PRIO);
	     }

             printf(" OSTaskCtr:%d", OSTaskCtr);    /* Display #tasks running */
	     printf(" OSCPUUsage:%d", OSCPUUsage);   /* Display CPU usage in % */
	     printf(" OSCtxSwCt:%d\n", OSCtxSwCtr); /* Display #context switches per second */
	     OSCtxSwCtr = 0;

	     OSTimeDlyHMSM(0, 0, 1, 0);     /* Wait one second */
        }
}


/* ********************************************************************* */
/* Global functions */

void APP_vMain (void)
{
        OSTaskCreate(TaskStart, (void *)0, (void *)&TaskStartStk[TASK_STK_SIZE - 1], TASK_START_PRIO);
        OSStart();                              /* Start multitasking */
	while(1);
}


/* ********************************************************************* */
