#ifndef __MASS_STOTAGE_H__
#define __MASS_STOTAGE_H__
typedef struct{
	unsigned int hiddennum;
	unsigned int headnum;
	unsigned int sectnum;
	unsigned int partsize;
	unsigned int sectorsize;
} DEVINFO,*PDEVINFO;
typedef struct
{
	unsigned int (*InitDev)(unsigned int handle);
	unsigned int (*ReadDevSector)(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount);
	unsigned int (*WriteDevSector)(unsigned int handle,unsigned char *buf,unsigned int startsect,unsigned int sectcount);
	unsigned int (*CheckDevState)(unsigned int handle);
	unsigned int (*GetDevInfo)(unsigned int handle,PDEVINFO pinfo);
	unsigned int (*DeinitDev)(unsigned handle);
	unsigned int DevName;
}UDC_LUN,*PUDC_LUN;
#endif //__MASS_STOTAGE_H__
