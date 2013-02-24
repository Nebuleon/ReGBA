/**************************************************************************
*                                                                         *
*   PROJECT     :                                
*                                                                         *
*   MODULE      :                                                
*                                                                         *
*   AUTHOR      : Regen     email: lhhuang@ingenic.cn                               
*                                                                         *
*   PROCESSOR   : jz4730                                                  *
*                                                                         *
*   TOOL-CHAIN  :                                                         *
*                                                                         *
*   DESCRIPTION :                                                         *
*   This is a sample code to test LWIP on  uC/OS-II                       *
*                                                                         *
**************************************************************************/



#include "includes.h"
#include <stdio.h>
#include "lwip.h"
#include "fs_api.h"
#include "app_cfg.h"
#include "camera.h"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/
//TASK PRIORITY defined in ucosii/src/app_cfg.h
#define  HTTP_PORT        80
#define  TELNET_PORT      23 // the port telnet users will be connecting to
#define  BACKLOG          10 // how many pending connections queue will hold in telnet

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/
OS_STK   TaskStartStk[APP_TASK_START_STK_SIZE];
OS_STK   Task1Stk[APP_TASK_TASK1_STK_SIZE];    
OS_STK   CaptureStk[APP_TASK_CAPTURE_STK_SIZE]; 
struct netif eth_jz4730;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void lwip_init(void);
static void _error(const char *msg);
static void _log(const char *msg);
static void _show_directory(const char *name);
void TaskStart (void *data);
void Task1(void *data);
void capture(void * data);

/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void APP_vMain (void)
{
   OS_STK *ptos;
   OS_STK *pbos;
   INT32U  size;

   FS_Init();
   //   OSTaskCreate(TaskStart, (void *)0, (void *)&TaskStartStk[TASK_STK_SIZE - 1], TASK_START_PRIO);
  
   ptos        = &TaskStartStk[APP_TASK_START_STK_SIZE - 1];
   pbos        = &TaskStartStk[0];
   size        = APP_TASK_START_STK_SIZE;
   OSTaskCreateExt(TaskStart,
                   (void *)0,
                   ptos,
                   TASK_START_PRIO,
                   TASK_START_ID,
                   pbos,
                   size,
                   (void *)0,
                   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

   OSStart();                              /* Start multitasking */
   while(1);
   FS_Exit();
}

/*
*********************************************************************************************************
*                                                   Local functions
*********************************************************************************************************
*/

void lwip_init(void)
{
  //  static struct netif eth_jz4730;
#if LWIP_DHCP
  err_t result;
  int icount = 0;
  int idhcpre = 0;
#endif
  struct ip_addr ipaddr, netmask, gw;

  stats_init();
  sys_init();
  mem_init();
  memp_init();
  netif_init();
  pbuf_init();
  etharp_init(); 
  tcpip_init(NULL, NULL);//ip tcp and udp's init in it 

#if  LWIP_DHCP

  IP4_ADDR(&gw, 0,0,0,0);
  IP4_ADDR(&ipaddr, 0,0,0,0);
  IP4_ADDR(&netmask, 0,0,0,0);
  netif_add(&eth_jz4730, &ipaddr, &netmask, &gw, NULL, ETHIF_Init, tcpip_input);

  printf("Waiting dhcp ...\n",idhcpre+1);

  for(idhcpre = 0; idhcpre<4;  idhcpre++ ) {

      printf("try dhcp %d \r",idhcpre+1);
      result = dhcp_start(&eth_jz4730);

      for(icount = 0; (icount < 10) && (ipaddr.addr == 0); icount ++ )
      {
          OSTimeDly(30);
      } // wait dhcp and read 10 times, if failed ipaddr = 192.168.1.140

      dhcp_stop(&eth_jz4730);

      if (eth_jz4730.ip_addr.addr != 0)
          break;
  }

  ipaddr.addr = eth_jz4730.ip_addr.addr;
  netmask.addr = eth_jz4730.netmask.addr;
  gw.addr = eth_jz4730.gw.addr;

  if(idhcpre == 4) {
    IP4_ADDR(&ipaddr, 192,168,1,140);
    IP4_ADDR(&netmask, 255,255,255,0);
    IP4_ADDR(&gw, 0,0,0,0);
    printf("dhcp failed \n");
  }
#else
  IP4_ADDR(&ipaddr, 192,168,1,140);
  IP4_ADDR(&netmask, 255,255,255,0);
  IP4_ADDR(&gw, 0,0,0,0);
   netif_add(&eth_jz4730, &ipaddr, &netmask, &gw, NULL, ETHIF_Init, tcpip_input);
#endif
  netif_set_default(&eth_jz4730);
  netif_set_up(&eth_jz4730);

  printf("local host ipaddr is: %d.%d.%d.%d\n",ip4_addr1(&ipaddr), ip4_addr2(&ipaddr),ip4_addr3(&ipaddr),ip4_addr4(&ipaddr));
  printf("local host netmask is: %d.%d.%d.%d\n",ip4_addr1(&netmask), ip4_addr2(&netmask),ip4_addr3(&netmask),ip4_addr4(&netmask));
  printf("local host gw is: %d.%d.%d.%d\n\n",ip4_addr1(&gw), ip4_addr2(&gw),ip4_addr3(&gw),ip4_addr4(&gw));

}

FS_FILE *myfile;
char mybuffer[0x100];
static void _error(const char *msg) {
#if (FS_OS_EMBOS)
  if (strlen(msg)) { 
    OS_SendString(msg);
  }
#else
  printf("%s",msg);
#endif
}


static void _log(const char *msg) {
#if (FS_OS_EMBOS)
  if (strlen(msg)) {
  OS_SendString(msg);
  }
#else
  printf("%s",msg);
#endif
}

static void _show_directory(const char *name) {
  FS_DIR *dirp;
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
    _error("Unable to open directory\n");
  }
}

void TaskStart (void *data)
{
  int x;
  data = data;                            // Prevent compiler warning 
  printf("\n\n  uC/OS-II, The Real-Time Kernel MIPS Ported version, ");
  printf("LWIP EXAMPLE 2.0\n\n");

  JZ_StartTicker(OS_TICKS_PER_SEC);	// OS_TICKS_PER_SEC config in os_cfg.h 
                               
  OSStatInit();                           // Initialize uC/OS-II's statistics 
  lwip_init(); 
#if 0
  x = FS_IoCtl("ram:",FS_CMD_FORMAT_MEDIA,FS_MEDIA_RAM_16KB,0);
  if (x!=0) {
   printf("Cannot format RAM disk.\n");
  }
#endif

  jzlcd_init();
  camera_open();	

  putjpg("a.jpg");
  putjpg("b.jpg");
  putjpg("c.jpg");

#if 1
 OSTaskCreateExt(capture,
		  (void *)0,
		  (void *)&CaptureStk[APP_TASK_CAPTURE_STK_SIZE - 1],
		  TASK_CAPTURE_PRIO,
	          TASK_CAPTURE_ID,
		  &CaptureStk[0],
		  APP_TASK_CAPTURE_STK_SIZE,
		  (void *)0,
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#endif

  sys_thread_new(tftp, (void *)0, TFTP_PRIO);

#if 1
  OSTaskCreateExt(Task1,
		  (void *)0,
		  &Task1Stk[APP_TASK_TASK1_STK_SIZE - 1],
		  TASK_1_PRIO,
		  TASK_1_ID,
		  &Task1Stk[0],
		  APP_TASK_TASK1_STK_SIZE,
		  (void *)0,
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#endif 

  while(1) {
    OSTimeDlyHMSM(0, 1, 0, 0);                  // Wait one minute
    printf("%d tasks running, ", OSTaskCtr);    // Display #tasks running 
    printf("CPU usage %d%, ", OSCPUUsage);      // Display CPU usage in % 
    printf("context switches %ld times\n", OSCtxSwCtr); // Display #context switches 
    OSCtxSwCtr = 0;
  }

}

void  Task1 (void *pdata)
{
    INT8U       err;
    OS_STK_DATA data;                       /* Storage for task stack data                             */
    INT8U       i;
 
    pdata = pdata;
    for (;;) {
        for (i = 0; i < 3; i++) {
	  err  = OSTaskStkChk(TASK_START_PRIO+i, &data);
	  if (err == OS_NO_ERR) {
	    printf( "prio:%d  total:%4ld  free:%4ld  used:%4ld\n", TASK_START_PRIO+i, data.OSFree + data.OSUsed, data.OSFree, data.OSUsed);
	  }
        }
        OSTimeDlyHMSM(0, 0, 30, 0);                       /* Delay for 30S                         */
    }
}
#if 1
void capture(void * data)
{
  while(1)
    docapture();
}
#endif
