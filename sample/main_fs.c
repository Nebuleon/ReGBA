/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*             (c) Copyright 2001 - 2003, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : main.c
Purpose     : Sample program for using the file system
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "fs_api.h"

#include <string.h>
#include <stdio.h>

#if (FS_OS_EMBOS)
  #include "RTOS.H"
#endif


/*********************************************************************
*
*             Global variables        
*
**********************************************************************
*/

FS_FILE *myfile;
char mybuffer[0x100];

const char dev_default_msg[]="This text was written on your default device.\n";
const char dev_ram_msg[]="This text was written on your RAM disk.\n";

#if (FS_OS_EMBOS)
  OS_STACKPTR int MainStack[0x400]; /* Stack-space */
  OS_TASK MainTCB;                  /* Task-control-blocks */
  
  #if (((FS_USE_SMC_DRIVER) && (FS_SMC_HW_NEEDS_POLL)) || ((FS_USE_MMC_DRIVER) && (FS_MMC_HW_NEEDS_POLL)) \
      || ((FS_USE_IDE_DRIVER) && (FS_IDE_HW_NEEDS_POLL)))
    OS_STACKPTR int DiskChangeStack[0x100]; /* Stack-space */
    OS_TASK DiskChangeTCB;                  /* Task-control-blocks */
  #endif
#endif

/*********************************************************************
*
*             Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*             debug output routines
*/

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


/*********************************************************************
*
*             _write_file
*
  This routine demonstrates, how to create and write to a file
  using the file system.
*/

static void _write_file(const char *name, const char *txt) {
  int x;
  
  /* create file */
  myfile = FS_FOpen(name,"w");
  if (myfile) {
    /* write to file */
    x = FS_FWrite(txt,1,strlen(txt),myfile);
    /* all data written ? */
    if (x!=(int)strlen(txt)) {
      /* check, why not all data was written */
      x = FS_FError(myfile);
      sprintf(mybuffer,"Not all bytes written because of error %d.\n",x);
      _error(mybuffer);
    }
    /* close file */
    FS_FClose(myfile);
  }
  else {
    sprintf(mybuffer,"Unable to create file %s\n",name);
    _error(mybuffer);
  }
}


/*********************************************************************
*
*             _dump_file
*
  This routine demonstrates, how to open and read from a file using 
  the file system.
*/

static void _dump_file(const char *name) {
  int x;

  /* open file */
  myfile = FS_FOpen(name,"r");
  if (myfile) {
    /* read until EOF has been reached */
    do {
      x = FS_FRead(mybuffer,1,sizeof(mybuffer)-1,myfile);
      mybuffer[x]=0;
      if (x) {
        _log(mybuffer);
      }
    } while (x);
    /* check, if there is no more data, because of EOF */
    x = FS_FError(myfile);
    if (x!=FS_ERR_EOF) {
      /* there was a problem during read operation */
      sprintf(mybuffer,"Error %d during read operation.\n",x);
      _error(mybuffer);
    }
    /* close file */
    FS_FClose(myfile);
  }
  else {
    sprintf(mybuffer,"Unable to open file %s.\n",name);
    _error(mybuffer);
  }
}


/*********************************************************************
*
*             _show_directory
*
  This routine demonstrates, how to read a directory.
*/

#if FS_POSIX_DIR_SUPPORT

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
#endif /* FS_POSIX_DIR_SUPPORT */


/*********************************************************************
*
*             _show_free
*
  This routine demonstrates, how to read disk space information.
*/

static void _show_free(const char *device) {
  FS_DISKFREE_T disk_data;
  int x;

  _log("Disk information of ");
  _log(device);
  _log("\n");
  x = FS_IoCtl(device,FS_CMD_GET_DISKFREE,0,(void*) &disk_data);
  if (x==0) {
    sprintf(mybuffer,"total clusters     : %lu\navailable clusters : %lu\nsectors/cluster    : %u\nbytes per sector   : %u\n",
          disk_data.total_clusters, disk_data.avail_clusters, disk_data.sectors_per_cluster, disk_data.bytes_per_sector);
    _log(mybuffer);
  } 
  else {
    _error("Invalid drive specified\n");
  }
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/


/*********************************************************************
*
*             DiskChange
*/

#if (((FS_USE_SMC_DRIVER) && (FS_SMC_HW_NEEDS_POLL)) || ((FS_USE_MMC_DRIVER) && (FS_MMC_HW_NEEDS_POLL)) \
    || ((FS_USE_IDE_DRIVER) && (FS_IDE_HW_NEEDS_POLL)))
  void DiskChange(void) {
    char name[8];
    int i;
    while (1) {
#if ((FS_USE_SMC_DRIVER) && (FS_SMC_HW_NEEDS_POLL))
      for (i=0;i<FS_SMC_MAXUNIT;i++) {
        sprintf(name,"smc:%d:",i);
        FS_IoCtl(name,FS_CMD_CHK_DSKCHANGE,0,0); /* check smc socket periodically */
      }
#endif
#if ((FS_USE_MMC_DRIVER) && (FS_MMC_HW_NEEDS_POLL))
      for (i=0;i<FS_MMC_MAXUNIT;i++) {
        sprintf(name,"mmc:%d:",i);
        FS_IoCtl(name,FS_CMD_CHK_DSKCHANGE,0,0); /* check mmc socket periodically */
      }
#endif
#if ((FS_USE_IDE_DRIVER) && (FS_IDE_HW_NEEDS_POLL))
      for (i=0;i<FS_IDE_MAXUNIT;i++) {
        sprintf(name,"ide:%d:",i);
        FS_IoCtl(name,FS_CMD_CHK_DSKCHANGE,0,0); /* check ide socket periodically */
      }
#endif
#if (FS_OS_EMBOS)
      OS_Delay (300);
#endif
    }
  }
#endif


/*********************************************************************
*
*             MainTask
*/

void MainTask(void) {
  int x;

  /* format your RAM disk */
  x = FS_IoCtl("ram:",FS_CMD_FORMAT_MEDIA,FS_MEDIA_RAM_16KB,0);
  if (x!=0) {
    _error("Cannot format RAM disk.\n");
  }

  /* create file 'default.txt' on your default device */
  _write_file("default.txt",dev_default_msg);

  /* create file 'ram.txt' on your RAM disk */
  _write_file("ram:\\ram.txt",dev_ram_msg);

  /* dump file 'default.txt' on you default device */
  _dump_file("default.txt");

  /* dump file 'ram.txt' on you default RAM disk */
  _dump_file("ram:\\ram.txt");

#if FS_POSIX_DIR_SUPPORT
  /* show root directory of default device */
  _show_directory("");

  /* show root directory of RAM disk */
  _show_directory("ram:");
#endif /* FS_POSIX_DIR_SUPPORT */
  
  /* display disk space information of the default device */
  _show_free("");

  /* display disk space information of the RAM disk */
  _show_free("ram:");

#if (FS_OS_EMBOS)
  /* embOS task must not exit */
  while (1) {
    OS_Delay(50);
  }
#endif
}


/*********************************************************************
*
*             main
*/

//void main(void) {
void APP_vMain (void) {
#if (FS_OS_EMBOS)
  OS_InitKern();     /* Initialize embOS */
  OS_InitHW();       /* Initialize hardware for embOS */
  /* Create a task instead of calling a function, if you are using embOS */
  OS_CREATETASK(&MainTCB, "Main Task", MainTask,  150, MainStack);
  #if (((FS_USE_SMC_DRIVER) && (FS_SMC_HW_NEEDS_POLL)) || ((FS_USE_MMC_DRIVER) && (FS_MMC_HW_NEEDS_POLL)) \
      || ((FS_USE_IDE_DRIVER) && (FS_IDE_HW_NEEDS_POLL)))
    OS_CREATETASK(&DiskChangeTCB, "DiskChange Task", DiskChange,  200, DiskChangeStack);
  #endif
#endif
  FS_Init();         /* Init the file system */
#if (FS_OS_EMBOS)
  OS_Start();        /* Start embOS multitasking (starts MainTask) */
#else
  MainTask();        /* Call MainTask directly, if you do not use embOS */
#endif
  FS_Exit();         /* End using the file system */
}


