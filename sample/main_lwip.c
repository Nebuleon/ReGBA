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
struct netif eth_jz4730;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void lwip_init(void);
void http(void * data);
void telnet(void *data);
static void _error(const char *msg);
static void _log(const char *msg);
static void _show_directory(const char *name);
void TaskStart (void *data);
void Task1(void *data);


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

void http(void * data)
{
  struct netconn  *pstConn, *pstNewConn;
  struct netbuf	*pstNetbuf;
  pstConn = netconn_new(NETCONN_TCP);
  netconn_bind(pstConn, NULL, HTTP_PORT);
  netconn_listen(pstConn);
	
  while(TRUE) {
    pstNewConn = netconn_accept(pstConn);

    if(pstNewConn != NULL) {	
      pstNetbuf = netconn_recv(pstNewConn);
      if(pstNetbuf != NULL) {
	netconn_write(pstNewConn, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n", 44, NETCONN_COPY);
	netconn_write(pstNewConn, "<body><h1>It's a LWIP test! </h1></body>", 40, NETCONN_COPY);
	printf("Receive a http request and send some text.\n");
	netbuf_delete(pstNetbuf);	
      }		
      netconn_close(pstNewConn);
      while(netconn_delete(pstNewConn) != ERR_OK)
	OSTimeDlyHMSM(0, 0, 1, 0);
    }
  }
}

void telnet(void *data)  
{
  int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
  struct sockaddr_in my_addr; // my address information
  struct sockaddr_in their_addr; // connector's address information
  socklen_t sin_size;

  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    OSTaskDel(TELNET_PRIO);
  }

  my_addr.sin_family = AF_INET; // host byte order
  my_addr.sin_port = htons(TELNET_PORT); // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero),'\0', 8); // zero the rest of the struct
  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))== -1) {
    printf("bind error\n");
    OSTaskDel(TELNET_PRIO);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    printf("listen error\n");
    OSTaskDel(TELNET_PRIO);
  }

  while(1) { // main accept() loop
    sin_size = sizeof(struct sockaddr_in);
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,&sin_size)) == -1) {
      printf("accept error\n");
      continue;
    }

    printf("Telnet server: got connection from %s\n",inet_ntoa(their_addr.sin_addr));

    if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
      printf("send error\n");
    close(new_fd);
  }
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

  x = FS_IoCtl("ram:",FS_CMD_FORMAT_MEDIA,FS_MEDIA_RAM_16KB,0);
  if (x!=0) {
   printf("Cannot format RAM disk.\n");
  }
 
  sys_thread_new(tftp, (void *)0, TFTP_PRIO);
  sys_thread_new(telnet, (void *)0, TELNET_PRIO);
  sys_thread_new(http, (void *)0, HTTP_PRIO);
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
    char key;
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
  }
}

void  Task1 (void *pdata)
{
    INT8U       err;
    OS_STK_DATA data;                       /* Storage for task stack data                             */
    INT8U       i;
 
    pdata = pdata;
    for (;;) {
        for (i = 0; i < 63; i++) {
	  err  = OSTaskStkChk(i, &data);
	  if (err == OS_NO_ERR) {
	    printf( "prio:%d  total:%4ld  free:%4ld  used:%4ld\n", i, data.OSFree + data.OSUsed, data.OSFree, data.OSUsed);
	  }
        }
        OSTimeDlyHMSM(0, 0, 10, 0); 
    }
}
