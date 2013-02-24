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
*   DESCRIPTION :  IP camera                                              *
*                                                                         *
*                                                                         *
**************************************************************************/

#include "includes.h"
#include <stdio.h>
#include "lwip.h"
#include "fs_api.h"
#include "app_cfg.h"
#include "camera.h"

//#define dprintf(x...) printf(x)
#define dprintf(x...)

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

//TASK PRIORITY defined in ucosii/src/app_cfg.h
#define  HTTP_PORT        80
#define  TELNET_PORT      23  /* The port telnet users will be connecting to           */
#define  BACKLOG          10  /* How many pending connections queue will hold in telnet */

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/
OS_STK                TaskStartStk[APP_TASK_START_STK_SIZE];
OS_STK                RejpegStk[APP_TASK_REJPEG_STK_SIZE];    
OS_STK                CaptureStk[APP_TASK_CAPTURE_STK_SIZE]; 

OS_STK                CameraStk[APP_TASK_CAMERA_STK_SIZE]; 
OS_STK                JpegStk[APP_TASK_JPEG_STK_SIZE];
OS_STK                DeltaskStk[APP_TASK_DELTASK_STK_SIZE];

OS_EVENT             *buf_can_new;
OS_EVENT             *buf_get_new;
OS_EVENT             *jpg_can_new;
OS_EVENT             *jpg_get_new;

static struct netif   eth_jz4730;
static INT8U         *framebuf;
static INT8U         *dstbuf; 

static INT32U         Jpeg_Err_Times;
static INT32U         Jpeg_Exp_Times;
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
void rejpeg(void *data);
void capture(void * data);

void camera(void * data);
void jpeg(void * data);
void tftp1(void * data);
void deltask(void * data);
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
          OSTimeDly(OS_TICKS_PER_SEC/4);
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
  data = data;                            /*  Prevent compiler warning */
  printf("\n\n  uC/OS-II, The Real-Time Kernel MIPS Ported version, ");
  printf("LWIP EXAMPLE 2.0\n\n");

  JZ_StartTicker(OS_TICKS_PER_SEC);
                               
  OSStatInit(); 
  lwip_init(); 
  OSTimeDlyHMSM(0, 0, 1, 0); 
    
#if 1
  x = FS_IoCtl("ram:",FS_CMD_FORMAT_AUTO, 0, 0);
  if (x!=0) {
   printf("Cannot format RAM disk.\n");
  }
#endif

  Jpeg_Err_Times = 0;
  Jpeg_Exp_Times = 0;
  Tftp_Err_Times = 0;

  buf_can_new = OSSemCreate(1);
  buf_get_new = OSSemCreate(0);
  jpg_can_new = OSSemCreate(1);
  jpg_get_new = OSSemCreate(0);

#if 1
 OSTaskCreateExt(deltask,
		  (void *)0,
		  (void *)&DeltaskStk[APP_TASK_DELTASK_STK_SIZE - 1],
		  TASK_DELTASK_PRIO,
	          TASK_DELTASK_ID,
		  &DeltaskStk[0],
		  APP_TASK_DELTASK_STK_SIZE,
		  (void *)0,
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#endif

 OSTaskCreateExt(camera,
		  (void *)0,
		  (void *)&CameraStk[APP_TASK_CAMERA_STK_SIZE - 1],
		  TASK_CAMERA_PRIO,
	          TASK_CAMERA_ID,
		  &CameraStk[0],
		  APP_TASK_CAMERA_STK_SIZE,
		  (void *)0,
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#if 1
 OSTaskCreateExt(jpeg,
		  (void *)0,
		  (void *)&JpegStk[APP_TASK_JPEG_STK_SIZE - 1],
		  TASK_JPEG_PRIO,
	          TASK_JPEG_ID,
		  &JpegStk[0],
		  APP_TASK_JPEG_STK_SIZE,
		  (void *)0,
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

   sys_thread_new(tftp1, (void *)0, TASK_TFTP1_PRIO);
#endif

#if 1
  OSTaskCreateExt(rejpeg,
		  (void *)0,
		  &RejpegStk[APP_TASK_REJPEG_STK_SIZE - 1],
		  TASK_REJPEG_PRIO,
		  TASK_REJPEG_ID,
		  &RejpegStk[0],
		  APP_TASK_REJPEG_STK_SIZE,
		  (void *)0,
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#endif 

  while(1) {
    char key;
    INT8U       err;
    OS_STK_DATA data;                       /* Storage for task stack data                             */
    INT8U       i;

    key = serial_getc();
    printf("you pressed: %c\n",key);
            
    if(key == 0x1B) {        /* see if it's the ESCAPE key */
      printf(" Escape display of statistic\n");
      OSTaskDel(OS_PRIO_SELF);
    }

    printf(" OSTaskCtr:%d", OSTaskCtr);    /* Display #tasks running */
    printf(" OSCPUUsage:%d", OSCPUUsage);   /* Display CPU usage in % */
    printf(" OSCtxSwCt:%d\n", OSCtxSwCtr); /* Display #context switches per second */
    OSCtxSwCtr = 0;
     

        for (i = 0; i < 63; i++) {
	  err  = OSTaskStkChk(i, &data);
	  if (err == OS_NO_ERR) {
	    printf( "prio:%d  total:%4ld  free:%4ld  used:%4ld\n", i, data.OSFree + data.OSUsed, data.OSFree, data.OSUsed);
	  }
        }

  }
}

void  rejpeg (void *pdata)
{
    pdata = pdata;
    while (1) { 

      if (Jpeg_Exp_Flag || Jpeg_Err_Flag) {
	if (Jpeg_Exp_Flag) {
	  Jpeg_Exp_Flag--;
	  Jpeg_Exp_Times++;
	  //printf("exp entered %d \n",Jpeg_Exp_Times);
	} else {
	  Jpeg_Err_Flag--;
	  Jpeg_Err_Times++;
	  //printf("err entered %d \n",Jpeg_Err_Times);
	}
	OSSemPost(jpg_can_new);
	/*	OSTaskCreate(jpeg,
			(void *)0,
			(void *)&JpegStk[APP_TASK_JPEG_STK_SIZE - 1],
			TASK_JPEG_PRIO);
	*/
	OSTaskCreateExt(jpeg,
			(void *)0,
			(void *)&JpegStk[APP_TASK_JPEG_STK_SIZE - 1],
			TASK_JPEG_PRIO,
			TASK_JPEG_ID,
			&JpegStk[0],
			APP_TASK_JPEG_STK_SIZE,
			(void *)0,
			OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

      }
      OSTimeDly(1);
    }
}


#if 1 

void camera(void * data)
{
  INT8U err;
  camera_open();	
  printf("Camera open.\n");

  framebuf = (unsigned char *)camera_heap; 

  while(1) {
    dprintf("camera pend \n");
    OSSemPend(buf_can_new, 0, &err);
    dprintf("camera start dam read \n");
    cim_read(framebuf);
    dprintf("framebuf get\n");
    OSSemPost(buf_get_new);
  }
}

void jpeg(void * data)
{
  INT8U err;
  unsigned int time0, time1;
  static INT32U n = 0;
  
  dstbuf = (unsigned char *)camera_heap  + IMG_WIDTH * IMG_HEIGHT *2;

  while(1) {
    dprintf("jpeg before convert\n");
    OSSemPend(buf_get_new, 0, &err);

#ifdef USE_YUV 
    convert_yuv422_yuv444(framebuf,dstbuf,IMG_WIDTH,IMG_HEIGHT);
#else 
    convert_rgb565_rgb888(framebuf,dstbuf,IMG_WIDTH,IMG_HEIGHT);
#endif

    dprintf("jpeg convert completed\n");
    OSSemPost(buf_can_new);
    OSSemPend(jpg_can_new, 0, &err);
    dprintf("jpeg start compress \n");
    n++;
 
    time0 = OSTimeGet();

    Jpeg_Enter = 1;
    put_image_jpeg (dstbuf,IMG_WIDTH,IMG_HEIGHT, 80, "ram:\\a.jpg");
    Jpeg_Enter = 0;
      
    time1 = OSTimeGet();
    printf("jpeg file %ld, time passed: %ld; total err %d, exp %d\n", \
	   n, time1-time0, Jpeg_Err_Times, Jpeg_Exp_Times);
    
    OSSemPost(jpg_get_new);
  }
}

#endif

void tftp1(void * data)
{
  INT8U err;
  static INT32U n = 0;

  printf("\nstart tftp1 ...\n");
  sock = socket(PF_INET,SOCK_DGRAM,0);
  if(sock==-1) {
    dprintf("Can't create socket \n");
    quit();
  }

  while(1) {
    dprintf("tftp wait \n");
    OSSemPend(jpg_get_new, 0, &err);
    n++;
    if(1) {
      unsigned int time0, time1;
      time0 = OSTimeGet();
      dprintf("tftp putting \n");
      
      //   sock = socket(PF_INET,SOCK_DGRAM,0);
       putfile("lhhuang/a.jpg","ram:\\a.jpg");
      //    close(sock);

      time1 = OSTimeGet();
      
      static int Tftp_Max = 0;
      if (Tftp_Max < time1-time0) {
	Tftp_Max = time1-time0;
      }

      printf("tftp time: %ld, MAX time %d, err %ld\n",time1-time0, Tftp_Max, Tftp_Err_Times);

    }
    OSSemPost(jpg_can_new);
  }
}

void deltask(void * data)
{
  OSTimeDlyHMSM(0, 1, 40, 0);
  printf("3 tasks be deleted\n");
  OSTaskDel(TASK_CAMERA_PRIO);
  OSTaskDel(TASK_JPEG_PRIO);
  OSTaskDel(TASK_TFTP1_PRIO);

  OSTaskDel(OS_PRIO_SELF);
}

#if 0
void camera(void * data)
{
  static INT32U n = 0;
  INT8U err;
   unsigned char *dstbuf = (unsigned char *)camera_heap  + IMG_WIDTH * IMG_HEIGHT *2;
  framebuf = (unsigned char *)camera_heap; 

  camera_open();

  while(1) {
    unsigned int time0, time1, time2, time3, time4, time5, time6,i;
    n++;
    time0 = OSTimeGet();
    cim_read(framebuf);
    time1 = OSTimeGet();
    convert_yuv422_yuv444(framebuf,dstbuf,IMG_WIDTH,IMG_HEIGHT);
    
    put_image_jpeg (dstbuf, IMG_WIDTH, IMG_HEIGHT, 60, "ram:\\a.jpg");
    time2 = OSTimeGet();
    dprintf("jpg get\n");

    sock = socket(PF_INET,SOCK_DGRAM,0);
    if(sock==-1) {
      dprintf("Can't create socket \n");
      quit();
    }
    putfile("lhhuang/a.jpg","ram:\\a.jpg");
    close(sock);
    dprintf("jpg put %ld\n",n);
    time3 = OSTimeGet();
    printf(" cim time:%ld   jpeg time:%ld  tftp time:%ld  total time:%ld\n",time1-time0,time2-time1,time3-time2, time3-time0);

    //   OSTimeDlyHMSM(0, 0, 1, 0);
  }
}

#endif
