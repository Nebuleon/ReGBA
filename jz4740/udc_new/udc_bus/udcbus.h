#ifndef __UDCBUS_H__
#define __UDCBUS_H__

#include "usb.h"

#define UDC_IDLE    0
#define UDC_JUDGE   0x100
#define UDC_REMOVE  0x101

#define UDC_RESET     0x200
#define UDC_SUSPEND   0x201

#define UDC_FULLSPEED 0x300
#define UDC_HIGHSPEED 0x301

#define UDC_SETUP_PKG_FINISH 0x400

#define UDC_PROTAL_SEND_FINISH 0x500
#define UDC_PROTAL_RECEIVE_FINISH 0x501


#define USB_DIRECT_OUT       0x80
#define USB_DIRECT_IN        0

typedef struct
{
	unsigned char ep;
	USB_ENDPOINT_TYPE ep_type;
	unsigned short max_pkg;
}PIPE,*PPIPE;

typedef unsigned int (*PFN_BusNotify)(unsigned int handle,unsigned int stat,unsigned char *bufaddr,unsigned int len);

typedef struct
{
	void (*StartTransfer)(unsigned int handle,unsigned char ep, unsigned char *buf,unsigned int len);
	void (*InitEndpointSuppost)(unsigned int handle,unsigned char *ep,USB_ENDPOINT_TYPE ep_type,unsigned short *ep_max_pkg);
	PFN_BusNotify BusNotify;
	void (*EnableDevice)(unsigned int handle);
	void (*DisableDevice)(unsigned int handle);
	void (*SetAddress)(unsigned int handle,unsigned short value);
	void (*EndPointStall)(unsigned int handle,unsigned char ep,unsigned char isStall);
	
	unsigned int DeviceName;
}UDC_BUS,*PUDC_BUS;

unsigned int BusNotify(unsigned int handle,unsigned int stat,unsigned char *bufaddr,unsigned int len);
void InitUDC(PUDC_BUS pBus);

#endif //__UDCBUS_H__
