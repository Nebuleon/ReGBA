#if RAM_UDC_DISK
#include <mass_storage.h>
#define BLOCKNUM	16384*7
#define SECTSIZE	512
static UDC_LUN udcLun;
static unsigned int disk_emu[BLOCKNUM * SECTSIZE / 4];
static unsigned char *disk = 0;
static unsigned int InitDev(unsigned int handle)
{
	printf("Ram Disk Mount\r\n");
	disk = (unsigned char *)((unsigned int)disk_emu | 0xa0000000);
}
static unsigned int ReadDevSector(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount)
{
//	printf("read dev = ia:%x,oa:%x s:%d c:%d\r\n",buf,disk,startsect,sectcount);
	
	memcpy(buf,(void *)(disk + startsect * SECTSIZE),sectcount*SECTSIZE);
	return 1;
	
}
static unsigned int WriteDevSector(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount)
{
	memcpy((void *)(disk + startsect * SECTSIZE),buf,sectcount*SECTSIZE);
	return 1;
	
}
static unsigned int CheckDevState(unsigned int handle)
{
	return 1;
}
static unsigned int GetDevInfo(unsigned int handle,PDEVINFO pinfo)
{
	pinfo->hiddennum = 0;
	pinfo->headnum = 2;
	pinfo->sectnum = 4;
	pinfo->partsize = BLOCKNUM;
	pinfo->sectorsize = 512;
}
static unsigned int DeinitDev(unsigned handle)
{
	disk = 0;
}
void InitUdcRam()
{

//	printf("InitUdcRam\r\n");
	
	Init_Mass_Storage();
	udcLun.InitDev = InitDev;
	udcLun.ReadDevSector = ReadDevSector;
	udcLun.WriteDevSector = WriteDevSector;
	udcLun.CheckDevState = CheckDevState;
	udcLun.GetDevInfo = GetDevInfo;
	udcLun.DeinitDev = DeinitDev;
	udcLun.DevName = (unsigned int)'RAM1';

	if(RegisterDev(&udcLun))
		printf("UDC Register Fail!!!\r\n");
	//printf("InitUdcRam finish = %08x\r\n",&udcLun);
	
}
#endif
