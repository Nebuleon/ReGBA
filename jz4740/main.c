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
      
#define GUI_LCD_TEST 0
#define LCD_TEST 0
#define UCFS_TEST 0
#define TOUCH_TEST 0
#define RTC_TEST 0
#define WAVTEST 0
#define SLEEPTEST 0
#define KEY_TEST 0
#define POWER_OFF_TEST 0 
#define MMC_TEST 0
#define PLL_CONVERT_TEST 0
#define BMP_TEST 0
#define MP3_TEST 0
#define NAND_TEST 0
#define UDC_MASS_TEST 0
#define FIVECHESS_TEST 0
#define GUIDEMO_TEST 0
#define LFN_TEST 0
#define TPANEL_TEST 0
#define FM_TEST 0
   
#define VBA_TEST 1
#include <jz4740.h>
#include "includes.h"
#include "dm.h"

//#include <stdio.h>

/* ********************************************************************* */
/* File local definitions */
#define  TASK_START_PRIO 80
#define  TASK_STK_SIZE   1024 *128                      /* Size of each task's stacks (# of WORDs) */
#define  NO_TASKS        30                      /* Number of identical tasks */

//OS_STK   TaskStk[TASK_STK_SIZE];      /* Tasks stacks */
OS_STK   TaskStartStk[TASK_STK_SIZE];
//char     TaskData[NO_TASKS];                    /* Parameters to pass to each task */
#if MP3_TEST
#include <sample/mp3test.c>
#endif

#if FM_TEST
#include <sample/fmtest.c>
#endif

#if LFN_TEST
#include <sample/lfntest.c>
#endif

#if BMP_TEST
#include "bitmap.h"
#include <fs_api.h>
#include <sample/GUI_bmptest.c>
#endif

#if PLL_CONVERT_TEST
#include <sample/pllconvert_sample.c>
#endif

#if POWER_OFF_TEST
#include <sample/hibernate_sample.c>
#endif

#if KEY_TEST
#include <sample/keytest.c>
#endif
 
#if TPANEL_TEST
#include <sample/tpanel_test.c>
#endif

#if WAVTEST
#include <pcm.h>
#include <fs_api.h>
#include <sample/sample_vplay.c>
#endif

#if RTC_TEST
#include <sample/rtctest.c>
#endif

#if UCFS_TEST
#include <fs_api.h>
#include <sample/ucfstest.c>
#endif

#if GUI_LCD_TEST
#include "WM.h"
#include <sample/GUI_lcdtest.c>
#endif

#if LCD_TEST
#include <sample/lcdtest.c>
#endif

#if SLEEPTEST
#include <sample/sleep_sample.c>
#endif

#if TOUCH_TEST
void TouchTest()
{
	TS_init();	
}
#endif

#if MMC_TEST
#include <sample/mmctest.c>
#endif

#if NAND_TEST
#include <sample/nanducfs.c>
#endif

#ifdef USE_MIDWARE
#include <sample/midwaretest.c>
#endif

#ifdef VBA_TEST
#include "mmc_api.h"
#include "fs_api.h"
//#include "gpsp_main.h"
#endif
/* ********************************************************************* */
/* Global definitions */



#if 0 //MP3_TEST //WAVTEST
#include <fs_api.h>
static void _log(const char *msg) {
  printf("%s",msg);
}


static void _show_directory(const char *name) {
  FS_DIR *dirp;
  char mybuffer[256];
  
  struct FS_DIRENT *direntp;
   
  _log("Directory of ");
  _log(name);
  _log("\n");
  dirp = FS_OpenDir(name);
  if (dirp) {
    do {
      direntp = FS_ReadDir(dirp);
      if (direntp) {
        sprintf(mybuffer,"%s\n",direntp->d_name);
        _log(mybuffer);
      }
    } while (direntp);
    FS_CloseDir(dirp);
  }
  else {
    _log("Unable to open directory\n");
  }
}
#endif
#if 0
#include <math.h>
void testfloat()
{
	float f1,f2;
	int i;
	for(i = 0; i < 6;i++)
	{
		f1 = 2.0  * 3.1415926 / 360.0 * 30.0 * (float)i;
		f2 = sin(f1);
		printf("\nsin(%d) = %d %d\r\n",(int)(f1 * 10000.0),(int)(f2 * 10000.0),1);
	}
	
	
	
}
#endif
/* ********************************************************************* */
/* Local functions */
void TaskStart (void *data)
{
        U8 i;
        char key;
		 data = data;                            /* Prevent compiler warning */
		 //testfloat();
		
        //jz_pm_pllconvert(396000000,3);
		
		 JZ_StartTicker(OS_TICKS_PER_SEC);	/* os_cfg.h */
        dgprintf("uC/OS-II, The Real-Time Kernel MIPS Ported version\n");
        dgprintf("EXAMPLE #1 %s %s\n",__DATE__,__TIME__);

        dgprintf("Determining  CPU's capacity ...\n");
	    OSStatInit();                           /* Initialize uC/OS-II's statistics */

#if DRIVER_MANAGER
		dgprintf("Driver Manager \r\n");
		jz_dm_init();

	    StatHookInit();
#endif

#if MMC_TEST

	    MMCTest();
#endif

#if KEY_TEST

		KeyTest();
#endif

#if TPANEL_TEST
		TPanelTest();
#endif

#if PLL_CONVERT_TEST
		PllConvertTest();
#endif

#if POWER_OFF_TEST
		HibernateTest();
#endif

#if SLEEPTEST
		SleepTest();
#endif

#if LCD_TEST
        lcd_test();
#endif	

#if GUI_LCD_TEST
	GUI_Init();
	ShowColorBar();
#endif

#if GUI_TOUCH
	 GUI_Init();
	 GUI_CURSOR_Show();
	 GUI_CURSOR_Hide();
	 GUI_CURSOR_Show();
	 GUI_CURSOR_Activate();
	 
	 GUI_TOUCH_SetDefaultCalibration();
#endif

#if BMP_TEST
	 FS_Init();
	 bmptest();
#endif

#if UCFS_TEST
	 FS_Init();
	 TestUCFS();
#endif
        
#if TOUCH_TEST
	 TouchTest();
#endif

//#if RTC_TEST
//	 Jz_rtc_init();
//     rtc_test();	 
//#endif

#if WAVTEST
	 FS_Init();
	 replay_test("2music.wav");
#endif

#if MP3_TEST
  TestMp3_old();
#endif

#if LFN_TEST
  TestLfn();
#endif

#if NAND_TEST
  TestNANDUCFS();
#endif

#if UDC_MASS_TEST
//  InitUdcRam();
  InitUdcNand();
//udc_init();
#endif

#ifdef USE_MIDWARE
	MidwareTest();
#endif

#if FM_TEST
  Test_fm();
#endif

#if 0  
        for (i = 0; i < NO_TASKS; i++)
        {                               
     /* Create NO_TASKS identical asks */
                TaskData[i] = '0' + i;          /* Each task will display its own letter */
                printf("#%d",i);
		OSTaskCreate(Task, (void *)&TaskData[i], (void *)&TaskStk[i][TASK_STK_SIZE - 1], i + 1);
        }
	
        printf("\n# Parameter1: No. Tasks\n");
        printf("# Parameter2: CPU Usage in %%\n");
        printf("# Parameter3: No. Task switches/sec\n");
        printf("<-PRESS 'ESC' TO QUIT->\n");
#endif

#if GUIDEMO_TEST   
	    FS_Init();
	initMp3();
	    GUI_Init();
		InitUdcNand();
		GUI_TOUCH_SetDefaultCalibration();
	    //_show_directory("mmc:");
		//_show_directory("nand:");
		
		MainTask_DeskTop();
#endif		


#if VBA_TEST
    MMC_Initialize();
printf("SD %d Sectors\n", MMC_GetSize());
	FS_Init();
//add_rsv_ins_except();
    gpsp_main(0, NULL);
#endif

		while(1)
        {
   
#if 0
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
#endif

	     OSTimeDlyHMSM(0, 0, 1, 0);     /* Wait one second */
        }
}


/* ********************************************************************* */
/* Global functions */

void APP_vMain (void)
{
	OSInit();
	//udc_init();
	heapInit();
	OSTaskCreate(TaskStart, (void *)0, (void *)&TaskStartStk[TASK_STK_SIZE - 1], TASK_START_PRIO);
        OSStart();                              /* Start multitasking */
	while(1);
}

/* ********************************************************************* */
