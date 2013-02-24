//#include "mmc_protocol.h"
//struct mmc_csd csd;
#include "mmc_api.h"
void MMCTest()
{
     unsigned short mmcbuf[256*2048];
     int i;
     unsigned int t[2];
     unsigned int t1[2];
     unsigned int size;
     int status;

     JZ_StartTimer();
     JZ_GetTimer(t);

     MMC_Initialize();	 
     size=MMC_GetSize();
     printf("Card Size: %d\n",size);	
     for(i = 0; i < 2048; i++)
	     mmcbuf[i] = i;
     printf("write mmc 4 128 sector\r\n");
     status= MMC_WriteBlock(4, (unsigned char *)((unsigned int)mmcbuf +1));
     
     if(status== MMC_NO_RESPONSE)
        printf("MMC is not present\n");
     else if(status== MMC_ERROR_OUT_OF_RANGE)
        printf("MMC write operation out of range\n");
     else if(status== MMC_ERROR_WP_VIOLATION)
        printf("MMC write protected\n");
     else if(status== MMC_ERROR_STATE_MISMATCH)
        printf("MMC state mismatch\n");
     else if(status)
        printf("MMC other errors\n");
     else
        printf("MMC write OK!\n");
     
/*
     printf("write mmc 12 8 sector\r\n");
     MMC_WriteMultiBlock(12,8,(unsigned char *)mmcbuf);
     for(i = 0; i < 2048;i++)
	     mmcbuf[i] = 0;
*/
     printf("read mmc 4 8 sector\r\n");    
     MMC_ReadMultiBlock(4, 2048, (unsigned char *)mmcbuf);
     for(i = 0; i < 256; i++)
	     if(mmcbuf[i] != i)
		     printf("error %d, %d\r\n", mmcbuf[i],i);

/*
     printf("read mmc 12 8 sector\r\n");    
     for(i = 0; i < 2048;i++)
	     mmcbuf[i] = 0;
     MMC_ReadMultiBlock(12, 8, (unsigned char *)mmcbuf);
     for(i = 0; i < 2048;i++)
	     if(mmcbuf[i] != i)
		     printf("error %d, %d\r\n", mmcbuf[i],i);
*/

     printf("Test mmc OK!\n");

     JZ_GetTimer(t1);
     JZ_DiffTimer(t1,t1,t);
     printf("write file use timer %d.%06d s\r\n",t1[1],t1[0]);
     JZ_StopTimer();

}
