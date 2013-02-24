#include "udcbus.h"

//mem manger using DrvMngMem.c
#define malloc    alloc
#define free      deAlloc

PUDC_BUS curDevice = 0;
PUDC_BUS CreateDevice(PFN_BusNotify busNotify)
{
	if(curDevice == 0)
	{
		curDevice = (PUDC_BUS)malloc(sizeof(UDC_BUS));
		InitUDC(curDevice);
		curDevice->DeviceName = (unsigned int)'UDC:';
	}
	curDevice->BusNotify = busNotify;
	
	return curDevice;
}
void CloseDevice()
{
	if(curDevice)
		free(curDevice);
	curDevice = 0;
}
PUDC_BUS SwichDevice(PFN_BusNotify busNotify)
{
	if(curDevice)
		curDevice->BusNotify = busNotify;
	return curDevice;
	
}
unsigned int BusNotify(unsigned int handle,unsigned int stat,unsigned char *bufaddr,unsigned int len)
{
	if(handle != (unsigned int)curDevice)
		return 0;
    curDevice->BusNotify(handle,stat,bufaddr,len);
	return 1;
	
}
