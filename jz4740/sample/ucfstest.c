void TestUCFS()
{
        int ret;
	 struct {
		 FS_u32 hiddennum;
		 FS_u32 headnum;
		 FS_u32 secnum;
		 FS_u32 partsize;
	 } devinfo;
	 int i,d,j;
	 char fname[20];
	 char devname[5] = "";
	 unsigned int t[2];
	 unsigned int t1[2];
	 unsigned short mmcbuf[2048];

	 FS_DISKFREE_T diskfree;
	 U32 buf[512/4];
	 FS_FILE *fp;

#define TESTMMC 0
#if TESTMMC
	 MMC_Initialize();	 
      for(i = 0; i < 2048;i++)
	      mmcbuf[i] = i;
      printf("write mmc 4 8 sector\r\n");
      MMC_WriteMultiBlock(4,8,(unsigned char *)mmcbuf);
      printf("write mmc 12 8 sector\r\n");
      MMC_WriteMultiBlock(12,8,(unsigned char *)mmcbuf);
      for(i = 0; i < 2048;i++)
	      mmcbuf[i] = 0;
      printf("read mmc 4 8 sector\r\n");    
      MMC_ReadMultiBlock(4,8,(unsigned char *)mmcbuf);
      for(i = 0; i < 2048;i++)
	      if(mmcbuf[i] != i)
		      printf("error %d, %d\r\n", mmcbuf[i],i);
      printf("read mmc 12 8 sector\r\n");    
      for(i = 0; i < 2048;i++)
	      mmcbuf[i] = 0;
      MMC_ReadMultiBlock(12,8,(unsigned char *)mmcbuf);
      for(i = 0; i < 2048;i++)
	      if(mmcbuf[i] != i)
		      printf("error %d, %d\r\n", mmcbuf[i],i);
#endif
#define READ_MBR 0
#if READ_MBR 
     MMC_Initialize();
     MMC_ReadMultiBlock(0,4,(unsigned char *)mmcbuf);
     printf("\r\n");
      for(i = 0; i < 256 / 8;i++)
      {
	      for(j = 0;j < 8;j++)
		      printf("%02x %02x ",
			     mmcbuf[i * 8 + j]& 0xff,
			     (mmcbuf[i * 8 + j] >> 8)& 0xff );
              printf("\r\n");
      }

#endif
#define FORMAT_DISK 0
#if FORMAT_DISK
	 ret = FS_IoCtl(devname,FS_CMD_GET_DISKFREE,0,&diskfree);	 

	 printf("FS_CMD_GET_DISKFREE %d\n",ret);

	 printf("Free Disk:\r\n");
	 printf("total_clustersk:%x\r\n",diskfree.total_clusters);
	 printf("avail_cluster:%x\r\n",diskfree.avail_clusters);
	 printf("sectors_per_cluster:%x\r\n",diskfree.sectors_per_cluster);
	 printf("bytes_per_sector:%x\r\n",diskfree.bytes_per_sector);
	
	
	 ret = FS_IoCtl(devname,FS_CMD_GET_DEVINFO,0,&devinfo);
	 printf("FS_IoCtl devinfo: %d\n",ret);
	 printf("devinfo: %x,%x,%x,%x\r\n",
		devinfo.hiddennum,
		devinfo.headnum,
		devinfo.secnum,
		devinfo.partsize);
	 
	 ret = FS_IoCtl(devname,FS_CMD_FORMAT_AUTO,0,0);
	 ret = FS_IoCtl(devname,FS_CMD_GET_DISKFREE,0,&diskfree);	 

	 printf("Format %d\n",ret);
	 printf("Free Disk:\r\n");
	 printf("total_clustersk:%x\r\n",diskfree.total_clusters);
	 printf("avail_cluster:%x\r\n",diskfree.avail_clusters);
	 printf("sectors_per_cluster:%x\r\n",diskfree.sectors_per_cluster);
	 printf("bytes_per_sector:%x\r\n",diskfree.bytes_per_sector);
	
	 ret = FS_IoCtl(devname,FS_CMD_GET_DEVINFO,0,&devinfo);
	 printf("FS_IoCtl devinfo: %d\n",ret);
	 printf("devinfo: %x,%x,%x,%x\r\n",
		devinfo.hiddennum,
		devinfo.headnum,
		devinfo.secnum,
		devinfo.partsize);
	 

#endif	 
#if 1
	 JZ_StartTimer();

	 sprintf(fname,"%s%s",devname,"fbf");
	 
	 printf("open file %s write \r\n",fname);
	 
	 JZ_GetTimer(t);
	 fp = FS_FOpen(fname,"w+b");
	  printf("open file ok;\r\n");
	 if(!fp)
		 printf("Create file error!\r\n");
	 else
	 {
		 for(i = 0;i < 1024 *1024 / 4 / 512 ;i++) 
		 {	 
			 for(j = 0; j < 512 / 4; j++)
				 buf[j] = i * (512 / 4) + j;
			 FS_FWrite(buf,4,512 / 4,fp);

		 }
		 
	 }
	 FS_FClose(fp);
	 JZ_GetTimer(t1);
         JZ_DiffTimer(t1,t1,t);
	 printf("write file use timer %d.%d us\r\n",t1[1],t1[0]);
	 JZ_GetTimer(t);
	 
	 fp = FS_FOpen(fname,"rb");
	 if(!fp)
		 printf("Create file error!\r\n");
	 else
	 {
		 for(i = 0;i < 1024 *1024 / 4 / 512 ;i++) 
		 {	 
			 FS_FRead(buf,4,512 / 4,fp);
			 for(j = 0; j < 512 / 4; j++)
			 {
				 if(buf[j] != i * (512 / 4) + j)
				 {
					 printf("Read File Data Error %d!dat = %d \r\n",i * (512 / 4) + j,buf[j]);
					 break;
				 }
			 }
			 

		 }
	 }
	 FS_FClose(fp);
	 JZ_GetTimer(t1);
	 
         JZ_DiffTimer(t1,t1,t);
	 printf("read file use timer %d.%d us\r\n",t1[1] ,t1[0]);
	 JZ_StopTimer();

#endif
}
