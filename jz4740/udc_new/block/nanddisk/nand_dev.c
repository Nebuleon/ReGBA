#if NAND_UDC_DISK
#if (NAND==0)
    #error "makefile no add nand flag!"
#endif
#include "header.h"
#include <mass_storage.h>
static UDC_LUN udcLun;
extern int NAND_LB_Read(unsigned int Sector, void *pBuffer);
extern int NAND_LB_Write(unsigned int Sector, void *pBuffer);

static int InitStall = 0;

static unsigned int InitDev(unsigned int handle)
{
	if(InitStall == 0)
	{
		
		printf("nand Disk Mount\r\n");
		NAND_LB_Init();
		InitStall = 1;
	}
	
	
}
static unsigned int ReadDevSector(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount)
{
	if(sectcount == 1)
	{
		
	//unsigned int i;
	
	//for(i = 0;i < sectcount; i++)
	//{
	//	NAND_LB_Read(startsect + i,(void *)((unsigned int)buf));
		//(unsigned int)buf += 512;
		//printf("read dev = ia:%x, s:%d c:%d\r\n",buf,startsect + i,sectcount);
	
	//}
		NAND_LB_Read(startsect,(void *)((unsigned int)buf));
	}else
	{
		NAND_LB_MultiRead(startsect,(void *)((unsigned int)buf),sectcount);
	}
	
	
	return 1;
	
}
static unsigned int WriteDevSector(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount)
{
	if(sectcount == 1)
	{
		
    //unsigned int i;
	//printf("write dev = ia:%x, s:%d c:%d\r\n",buf,startsect,sectcount);
	//for(i = 0;i<sectcount;i++)		
	//{
	//	NAND_LB_Write(startsect + i,(void *)((unsigned int)buf));
	//	(unsigned int)buf += 512;
		
	//}
			NAND_LB_Write(startsect,(void *)((unsigned int)buf));
	}else
	{
		NAND_LB_MultiWrite(startsect,(void *)((unsigned int)buf),sectcount);
	}
	
	
	
}
static unsigned int CheckDevState(unsigned int handle)
{
	return 1;
}
static unsigned int GetDevInfo(unsigned int handle,PDEVINFO pinfo)
{

	//printf("pagesize = %d\n pinfo->partsize = %d,blocks = %d\n",pagesize,pagesize / 512 * pageperblock *blocks,blocks);
	flash_info_t flashinfo;
  ssfdc_nftl_getInfo(0,&flashinfo);
	pinfo->hiddennum = 0;
	pinfo->headnum = 0;
	pinfo->sectnum = 4;
	pinfo->partsize =  flashinfo.dwFSTotalSectors;	
	pinfo->sectorsize = flashinfo.dwDataBytesPerSector;
}
static unsigned int DeinitDev(unsigned handle)
{
	
	if(InitStall == 1)
	{
		NAND_LB_Deinit();
		InitStall = 0;
	}
		
}
void InitUdcNand()
{

//	printf("InitUdcRam\r\n");
	
	Init_Mass_Storage();
	udcLun.InitDev = InitDev;
	udcLun.ReadDevSector = ReadDevSector;
	udcLun.WriteDevSector = WriteDevSector;
	udcLun.CheckDevState = CheckDevState;
	udcLun.GetDevInfo = GetDevInfo;
	udcLun.DeinitDev = DeinitDev;
	udcLun.DevName = (unsigned int)'NAND';

	if(RegisterDev(&udcLun))
		printf("UDC Register Fail!!!\r\n");
	//printf("InitUdcRam finish = %08x\r\n",&udcLun);
	
}
#endif
