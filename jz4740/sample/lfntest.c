#include <bsp.h>
#include <jz4740.h>
#include <mipsregs.h>
#include <fs_api.h>
FS_FILE *myfile;
unsigned char mybuffer[0x100];
//unsigned char mp3buffer[10*1024*1024];

#if 1 

static void _log(const char *msg) {
  printf("%s",msg);
}


static void _show_directory(const char *name) {
  FS_DIR *dirp;
  char mybuffer[256];
  int temp;
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
	temp=dirp->dirpos;
        _log(mybuffer);
      }
    } while (direntp);
    FS_CloseDir(dirp);
  }
#if 0 //test read back dir
  dirp->dirpos=temp;
  if (dirp) {
    do {
      direntp = FS_ReadDir_Back(dirp);
      if (direntp) {
        sprintf(mybuffer,"%s\n",direntp->d_name);
        _log(mybuffer);
      }
    } while (direntp);
    FS_CloseDir(dirp);
  }
#endif
  else {
    _log("Unable to open directory\n");
  }
}
#endif

static void _error(const char *msg) {
#if (FS_OS_EMBOS)
  if (strlen(msg)) { 
    OS_SendString(msg);
  }
#else
  printf("%s",msg);
#endif
}
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

void TestLfn(void)
{
	
	printf("Test Long File Name!\n");
	FS_Init();

	_show_directory(""); 
//	_show_directory("mmc:\\SMDK2410"); 
#if 1
	FS_MkDir("SYSTEM");
	FS_MkDir("PICTURE");
	FS_MkDir("RECORD");
	FS_MkDir("VIDEO");
	FS_MkDir("MUSIC");
	FS_MkDir("EBOOK");
#endif
//	_dump_file("mmc:\\this is a test {}!@$#%  &..,;aa  aaaaa.txt");
//	FS_MkDir("GAOJQQB");
}	 


