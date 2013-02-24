#ifndef __LWIP_H__
#define __LWIP_H__

extern unsigned char Tftp_Err_Times;
extern int sock;
void tftp(void *data);
void putfile(char *filename, char *sfilename);
void getfile(char *dfilename, char *filename);
void quit(void);

#if (LWIP30 == 1)
#include"netif/ethif_jz4730.h"
#else
//#include"netif/cs8900if.h"
//#include"netif/ethif_cs8900_jz4740_poll.h"
#include"netif/ethif_cs8900_jz4740.h"
#endif

#include "netif/etharp.h"

#include "api.h"
#include "icmp.h"
#include "debug.h"
#include "mem.h"
#include "memp.h"
#include "ip.h"
#include "opt.h"
#include "tcp.h"
#include "api_msg.h"
#include "inet.h"
#include "pbuf.h"
#include "sockets.h"
#include "tcpip.h"
#include "arch.h"
#include "dhcp.h"
#include "ip_addr.h"
#include "raw.h"
#include "stats.h"
#include "udp.h"
#include "err.h"
#include "ip_frag.h"
#include "netif.h"
#include "sio.h"
#include "sys.h"

#endif /*__LWIP_H__ */
