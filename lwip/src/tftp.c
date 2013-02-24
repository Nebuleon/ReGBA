/**************************************************
* TFTP client compatible with RFC-1350 
***************************************************/

#include "fs_api.h"
#include "app_cfg.h"

unsigned char Tftp_Err_Times;

#define dprintf(x...) printf(x)
//#define dprintf(x...)  

/* read/write request packet format
     2 bytes     string    1 byte     string   1 byte
     ------------------------------------------------
    | Opcode |  Filename  |   0  |    Mode    |   0  |
     ------------------------------------------------

*/
#define TFTP_RRQ 1   /*Read request (RRQ)*/
#define TFTP_WRQ 2   /*Write request (WRQ) */

/* DATA packet format
       2 bytes     2 bytes      n bytes
      ----------------------------------
     | Opcode |   Block #  |   Data     |
      ----------------------------------
*/
#define TFTP_DATA 3  /*Data (DATA)*/

/* ACK packet format
      2 bytes     2 bytes
      ---------------------
     | Opcode |   Block #  |
      ---------------------
*/
#define TFTP_ACK 4   /*Acknowledgment (ACK)*/

/*ERROR packet format
    2 bytes     2 bytes      string    1 byte
    -----------------------------------------
   | Opcode |  ErrorCode |   ErrMsg   |   0  |
    -----------------------------------------
*/
#define TFTP_ERROR 5 /*Error (ERROR)*/

#define TFTP_NETASCII 0
#define TFTP_OCTET 1
#define TFTP_FIRSTACK 0
#define TFTP_NEXTACK 1
#define TFTP_LASTACK 2
#define MAX_RETRY 6
#define TFTP_NOTEND_DATALEN 512+2+2

void setoctet(void);
void setascii(void);
void quit(void);

int makereq(char type,int mode,char *filename,char *buffer);
int makeack(unsigned short num,char *buffer);

void getfile(char *dfilename, char *filename);
void putfile(char *filename,char *sfilename);

char desthost[256] = "192.168.1.20"; 
int filemode = TFTP_OCTET;
int sock = -1;

void tftp(void *data)
{
 unsigned int time0, time1, time2;

  printf("\nstart tftp ...\n");
  sock = socket(PF_INET,SOCK_DGRAM,0);
  if(sock==-1) {
    printf("Can't create socket \n");
    quit();
  }

  while(1) {
    time0 = OSTimeGet();
    getfile("ram:\\jdi222.cfg","jdi2.cfg");
    time1 = OSTimeGet();
    putfile("jgao/111.cfg","ram:\\jdi222.cfg");
    time2 = OSTimeGet();
//    printf("tftp put time:%ld\n",time2-time0);
    printf("tftp get time:%ld  put time:%ld\n",time1-time0,time2-time1);
  }
  quit();
}

void quit(void)
{
  printf("tftp exit!\n\n");
  close(sock);
  OSTaskDel(OS_PRIO_SELF);//sys_timeoutlist and thread_no need change too?
}

void setoctet(void)
{
  filemode = TFTP_OCTET;
  printf("Set file mode to octet\n");
}

void setascii(void)
{
  filemode = TFTP_NETASCII;
  printf("Set file mode to netascii\n");
}

int makeack(unsigned short num,char *buffer)
{
  int pos = 0;
  buffer[pos] = 0;
  pos++;
  buffer[pos] = TFTP_ACK;
  pos++;
  buffer[pos] = (char)(num>>8);
  pos++;
  buffer[pos] = (char)num;
  pos++;
  return pos;
}

int makereq(char type,int mode,char *filename,char *buffer)
{
  int pos = 0;
  unsigned int i = 0;
  char s[32] = "";
  if(mode==TFTP_NETASCII)
    strcpy(s,"netascii");
  else
	strcpy(s,"octet");
  buffer[pos] = 0;
  pos++;
  buffer[pos] = type;
  pos++;
  for(i=0;i<strlen(filename);i++)
  {
    buffer[pos] = filename[i];
    pos++;
  }
  buffer[pos] = 0;
  pos++;
  for(i=0;i<strlen(s);i++)
  {
    buffer[pos] = s[i];
    pos++;
  }
  buffer[pos] = 0;
  pos++;
  return pos;
}

int makedata(int num,char *data,int datasize,char *buffer)
{
  int pos = 0;
  buffer[pos] = 0;
  pos++;
  buffer[pos] = TFTP_DATA;
  pos++;
  buffer[pos] = (char)(num>>8);
  pos++;
  buffer[pos] = (char)num;
  pos++;
  memcpy(&buffer[pos],data,datasize);
  pos = pos + datasize;
  return pos;

}

/*
 * dfilename: destination file in SD card or RAM
 * filename: source file in tftp sever to get
 */
void getfile(char *dfilename, char *filename)
{
  u8_t sendbuf[1024] = {0};
  u8_t recvbuf[1024] = {0};
  struct sockaddr_in addr;
  struct sockaddr_in from;
  int fromlen = 0;
  int ret = 0;
  int len = 0 ;
  fd_set  fdr;
  int retry = 0;
  struct  timeval timeout = {5,0};
  int stat = 0;
  int lastdata = 0;
  int flen = 0;

  FS_FILE *file;
  if((file = FS_FOpen(dfilename,"wb"))==NULL) {
      printf("File %s not found! \n",dfilename);
      return;
    }

  len = makereq(TFTP_RRQ,filemode,filename,sendbuf);
//       printf("retry %d len:%d\r\n",retry,len);
  from.sin_family =PF_INET;
  memset(&(from.sin_zero),'\0', 8); // zero the rest of the struct 

  addr.sin_family =PF_INET;
  addr.sin_port = htons(69);
//  addr.sin_addr.s_addr   = inet_addr(desthost);
  ret = (int)inet_aton(desthost, &addr.sin_addr); 
  printf("addr ret:%d-------- %08x\n",ret, addr.sin_addr);
//  memset(&(addr.sin_zero),'\0', 8); // zero the rest of the struct
 
  ret = sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));
//        printf("retry %d ret:%d\r\n",retry,ret);
  while(1) {
    FD_ZERO(&fdr);
    FD_SET(sock, &fdr);
    ret = select(sock, &fdr, NULL,NULL, &timeout);

    if(ret<0) {
      FS_FClose(file);
      printf("Socket error \n");
      return;
    }
    else if(0==ret) {
      if(MAX_RETRY==retry) {
	FS_FClose(file);
        printf("%lu bytes received\r",flen);
	return;
      }
      sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr)); 
      retry++;
    }
    else {   
      if (FD_ISSET(sock,&fdr)) {
	fromlen = sizeof(struct sockaddr);
	ret = recvfrom(sock,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&from,&fromlen);  //why from not addr?  recvfrom automate get it?
	if(TFTP_ERROR==recvbuf[1]) {
	  FS_FClose(file);
          printf("Error %d: %s \n",recvbuf[3],&recvbuf[4]);
	  return;
	}
	if(0==stat) {
	  addr.sin_port = from.sin_port ; 
	  stat = 1;
	}
        if(TFTP_DATA==recvbuf[1]) {
          lastdata = recvbuf[2]*256 + recvbuf[3];
	  len = makeack(lastdata,sendbuf);
          sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));
	  if(ret<TFTP_NOTEND_DATALEN) {
	    FS_FWrite(&recvbuf[4],1,ret-4,file);
	    flen = flen + ret -4;
	    FS_FClose(file);
	    printf("%lu bytes received (read from file %s)\n",flen, filename);
	    return;
	  }
	  else {
	    static int try = 0;
	    static int n = 10;
	    FS_FWrite(&recvbuf[4],1,512,file);
	    flen = flen + 512;

	    while (file->size != flen) {
	      try += 1;
	      printf("write error at file length 0x%lx ,try %d\n",file->size, try);
	      FS_FWrite(&recvbuf[4],1,512,file);
	      if (file->size == flen ) {
		printf("try ok!\n");
		break;
	      }
	      else if(3 == try) {
		printf("try failed!\n");
		break;
	      }
	    }

	    if(flen == n*512) {
	      dprintf("%lu bytes received\r",flen);
	      //printf("%lu\r",flen);
	      n += 100;
	    }
	  }
	}
      }
    }
    //  OSTimeDly(1);
  }
}


/*
 * filename: destination file in tftp sever
 * sfilename: source file in SD card or RAM to put
 */
void putfile(char *filename, char *sfilename)
{
  u8_t sendbuf[1024] = {0};
  u8_t recvbuf[1024] = {0};
  u8_t databuf1[1024] = {0};
  struct sockaddr_in addr;
  struct sockaddr_in from;
  int fromlen = 0;
  int ret = 0;
  int len = 0 ;
  fd_set  fdr;
  int retry = 0;
  struct  timeval timeout = {5,0};
  int stat = TFTP_FIRSTACK;
  int lastack= 0;
  int flen = 0;
  int blocknum = 0;
  size_t rlen = 0;
  FS_FILE *file;

 if((file = FS_FOpen(sfilename, "r"))==NULL)
  {
    printf("File %s not found! \n", sfilename); 
    return;
  }

  len = makereq(TFTP_WRQ,filemode,filename,sendbuf);
  addr.sin_family =AF_INET;
  addr.sin_port = htons(69);
  addr.sin_addr.s_addr = inet_addr(desthost);
  ret = sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));


  while(1) {
    FD_ZERO(&fdr);
    FD_SET(sock, &fdr);
    ret = select(sock, &fdr, NULL,NULL, &timeout);
    if(ret<0) {
      printf("Socket error \n");
      FS_FClose(file);
      return;
    }
    else if(0==ret) {
      if(MAX_RETRY==retry) {
	printf("recv Time Out \n");
	FS_FClose(file);
        return;
      }
      sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));
      retry++;
    }
    else {
      retry = 0;
      fromlen = sizeof(struct sockaddr);
      ret = recvfrom(sock,recvbuf,sizeof(recvbuf),0,(struct sockaddr *)&from,&fromlen);
      if(TFTP_ERROR==recvbuf[1]) {
	FS_FClose(file);
        printf("Error %d: %s \n",recvbuf[3],&recvbuf[4]);
        return;
      }
      if(TFTP_ACK==recvbuf[1]) {   printf("get in----------\n");
        lastack = recvbuf[2]*256 + recvbuf[3];
        switch(stat) {
	case TFTP_FIRSTACK:
	  if(0==lastack) { 

	    stat = TFTP_NEXTACK;
	    addr.sin_port = from.sin_port ;

	    rlen = FS_FRead(databuf1,1,512,file);
	    flen = flen + rlen;
	    //	    if(rlen<512 && FS_FEof(file)) {
	    if(rlen<512 ) {
	      printf("<512\n");
	      stat = TFTP_LASTACK;
	    }
	    else if(FS_FError(file)) {
	      printf("Error: read file\n");
	      FS_FClose(file);
	      return;
	    }

	    blocknum++;
	    len = makedata(blocknum,databuf1,rlen,sendbuf);
	    sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));
	    //   printf("%ld byte send\r",flen);

	  }
	  else {
	    Tftp_Err_Times++;
	    FS_FClose(file);
	    close(sock);
	    sock = socket(PF_INET,SOCK_DGRAM,0);
	    if(sock==-1) {
	      printf("Can't create socket \n");
	      quit();
	    }

	    printf("\nError Ack Number lastack=%d\n",lastack);
	    return;
	  }
	  break;
	case TFTP_NEXTACK:
	  if(lastack==blocknum) {

	    rlen = FS_FRead(databuf1,1,512,file);
	    flen = flen + rlen;
	    //if(rlen<512 && FS_FEof(file)) {
	    if(rlen<512) {
	      stat = TFTP_LASTACK;
	    }
	    else if(FS_FError(file)) {
	      printf("Error: read file\n");
	      FS_FClose(file);
	      return;
	    }

	    blocknum++;
	    len = makedata(blocknum,databuf1,rlen,sendbuf);
	    sendto(sock,sendbuf,len,0,(struct sockaddr *)&addr,sizeof(addr));

	    //printf("%ld\r",flen);
	    static int n = 10;
	    if(flen == n*512) {
	      dprintf("%lu bytes send\r",flen);
	      //printf("%lu\r",flen);
	      n += 100;
	    }
	  }
	  else {
	    Tftp_Err_Times++;
	    FS_FClose(file);
	    close(sock);
	    sock = socket(PF_INET,SOCK_DGRAM,0);
	    if(sock==-1) {
	      printf("Can't create socket \n");
	      quit();
	    }

	    printf("\n Error Ack Number lastack=%x blocknum=%x\n",lastack,blocknum-1);
	    printf("recvbuf[2]=%x  recvbuf[3]=%x\n",recvbuf[2],recvbuf[3]);
	    return;
	  }
	  break;
	case TFTP_LASTACK:
	  if(lastack==blocknum) {
	    FS_FClose(file);
	    printf("%ld bytes send (write to file %s)\n",flen, filename);
	    return;
	  }
	  else {
	    Tftp_Err_Times++;
	    FS_FClose(file);
	    close(sock);
	    sock = socket(PF_INET,SOCK_DGRAM,0);
	    if(sock==-1) {
	      printf("Can't create socket \n");
	      quit();
	    }

	    printf("\nError Ack Number\n");
	    return;
	  }
	  break;
	}
      }
    }
    //    OSTimeDly(1);
  }
}
