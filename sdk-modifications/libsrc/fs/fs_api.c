//fs_api.c

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include "fs_common.h"
#include "fatfile.h"
#include "fatdir.h"

typedef unsigned int size_t;
typedef unsigned int mode_t;

typedef struct {
  const char* mode;					/* mode string */
  unsigned int flag;				/* mode flag */
//  unsigned char mode_r;				/* READ */
//  unsigned char mode_w;				/* WRITE */
//  unsigned char mode_a;				/* APPEND */
//  unsigned char mode_c;				/* CREATE */
} _fat_mode_type;

static const _fat_mode_type _fat_valid_mode[] = {
	{ "r"   ,	O_RDONLY },
	{ "w"   ,	O_WRONLY | O_TRUNC | O_CREAT },
	{ "a"   ,	O_WRONLY | O_APPEND | O_CREAT },
	{ "rb"  ,	O_RDONLY },
	{ "wb"  ,	O_WRONLY | O_TRUNC | O_CREAT },
	{ "ab"  ,	O_WRONLY | O_APPEND | O_CREAT },
	{ "r+"  ,	O_RDWR },
	{ "w+"  ,	O_RDWR | O_TRUNC | O_CREAT },
	{ "a+"  ,	O_RDWR | O_APPEND | O_CREAT },
	{ "r+b" ,	O_RDWR },
	{ "rb+" ,	O_RDWR },
	{ "w+b" ,	O_RDWR | O_TRUNC | O_CREAT },
	{ "wb+" ,	O_RDWR | O_TRUNC | O_CREAT },
	{ "a+b" ,	O_RDWR | O_APPEND | O_CREAT },
	{ "ab+" ,	O_RDWR | O_APPEND | O_CREAT }
};

#define MAX_OPEN_FILE	16
#define MAX_OPEN_DIR	32
#define MAX_PWD_LEN		512
#define MAX_STDIO_BUF_SIZE 2048

#define FAT_VALID_MODE_NUM	(sizeof(_fat_valid_mode)/sizeof(_fat_mode_type))

static FILE_STRUCT	__FILEs[MAX_OPEN_FILE];
struct _reent	__REENT;
static DIR_STATE_STRUCT __DIRs[MAX_OPEN_DIR];
static char __PWD[MAX_PWD_LEN];

int fat_init(void)
{
	int i, flag;

	for(i= 0; i < MAX_OPEN_FILE; i++)
		__FILEs[i].inUse = false;

	for(i= 0; i < MAX_OPEN_DIR; i++)
		__DIRs[i].inUse = false;

	flag = -1;
	if(_FAT_Init() == true)	//success
		flag = 0;

	return flag;
}

FILE_STRUCT* fat_fopen(const char *file, const char *mode)
{
	int i, k;
	int fd;
	FILE_STRUCT* fp;

	for(i= 0; i < MAX_OPEN_FILE; i++)
		if(false == __FILEs[i].inUse) break;

	if(i >= MAX_OPEN_FILE)
	{
		__REENT._errno = EMFILE;		/* Too many open files */
		return NULL;
	}

	for(k = 0; k < FAT_VALID_MODE_NUM; k++)
		if(!strcasecmp(_fat_valid_mode[k].mode, mode)) break;

	if(k >= FAT_VALID_MODE_NUM)
	{
		__REENT._errno = EINVAL;
		return NULL;
	}

	fd = _FAT_open_r(&__REENT, (void*)&__FILEs[i], file, _fat_valid_mode[k].flag);
	if(-1 == fd) return NULL;

	fp = &__FILEs[i];
	fp -> fd = fd;

	return fp;
}

size_t fat_fread(void *buf, size_t size, size_t count, FILE_STRUCT *fp)
{
	if(0 == size || 0 == count)
		return 0;

	int len = _FAT_read_r(&__REENT, fp->fd, (char*)buf, size*count);
	len /= size;
	return len;
}

size_t fat_fwrite(const void *buf, size_t size, size_t count, FILE_STRUCT *fp)
{
	if(0 == size || 0 == count)
		return 0;

	int len = _FAT_write_r(&__REENT, fp->fd, (const char*)buf, size*count);
	len /= size;

	return len;
}

int fat_fclose(FILE_STRUCT *fp)
{
	return( _FAT_close_r(&__REENT, fp->fd) );
}

int fat_fseek(FILE_STRUCT *fp, long offset, int whence)
{
	int flag;

	//When success, _FAT_seek_r return file position pointer
	flag = _FAT_seek_r (&__REENT, fp->fd, (int)offset, whence);
	if(flag > 0) flag = 0;

	return flag;
}

long fat_ftell(FILE_STRUCT *fp)
{
	return( (long)fp->currentPosition );
}

int fat_feof(FILE_STRUCT *fp)
{
	int result;

	result = 0;
	if((fp->currentPosition +1) >= (fp->filesize))
		result = -1;

	return result;
}

int fat_ferror(FILE_STRUCT *fp)
{
	return( __REENT._errno );
}

void fat_clearerr(FILE_STRUCT *fp)
{
	fp = fp;

	__REENT._errno = 0;
}

int fat_fflush(FILE_STRUCT *fp)
{
	return(_FAT_cache_flush(fp->partition->cache));
}

int fat_fgetc(FILE_STRUCT *fp)
{
	char ch;
	int result;

	result = _FAT_read_r(&__REENT, fp->fd, &ch, 1);
	if(0 == result)
		return EOF;

	return ( (int)ch );
}

char* fat_fgets(char* buf, int n, FILE_STRUCT* fp)
{
	int m;
	char *s;

	buf[0] = '\0';
	if(n <= 1) return buf;

	n -= 1;
	m = _FAT_read_r(&__REENT, fp->fd, buf, n);
	if(0 == m) return NULL;

	buf[m] = '\0';
	s = strchr((const char*)buf, 0x0A);

	if(NULL != s)
	{
		*(++s)= '\0';
		m -= s - buf;

		//fix reading pointer
		_FAT_seek_r (&__REENT, fp->fd, -m, SEEK_CUR);
	}
	else if(m == n)
	{
		if(0x0D == buf[n-1])		//0x0D,0x0A
		{
			buf[n-1] = '\0';
			_FAT_seek_r (&__REENT, fp->fd, -1, SEEK_CUR);
		}
	}

	return buf;
}

int fat_fputc(int ch, FILE_STRUCT *fp)
{
	return( _FAT_write_r(&__REENT, fp->fd, (const char*)&ch, 1) );
}

int fat_fputs(const char *s, FILE_STRUCT *fp)
{
	unsigned int len;

	len = strlen(s);
	return( _FAT_write_r(&__REENT, fp->fd, s, len) );
}

int fat_remove(const char *filename)
{
	return( _FAT_unlink_r (&__REENT, (const char*)filename) );
}

int fat_rename(const char *oldName, const char *newName)
{
	return( _FAT_rename_r(&__REENT, oldName, newName) );
}

int fat_setHidden(const char *name, unsigned char hide){
	return(_FAT_hideAttrib_r (&__REENT, name, hide));
}

#define S_ISHID(st_mode) ((st_mode & S_IHIDDEN) != 0)
int fat_isHidden(struct stat *st){
	return S_ISHID(st->st_mode);
}

int fat_getShortName(const char *fullName, char *outName){
	return(_FAT_getshortname_r (&__REENT, fullName, outName));
}


void fat_rewind(FILE_STRUCT *fp)
{
	_FAT_seek_r (&__REENT, fp->fd, 0, SEEK_SET);
}

int fat_fstat(int fildes, struct stat *buf)
{
	return( _FAT_fstat_r (&__REENT, fildes, buf) );
}

//int fat_fprintf(FILE_STRUCT *fp, const char *format, ...)
int fat_fprintf(void* fp, const char *format, ...)
{
	int ret;
	va_list ap;
	char buf[MAX_STDIO_BUF_SIZE];

	if(NULL == fp)
	{
		__REENT._errno = EINVAL;
		return -1;
	}

	//FIXME: stderr, stdout
	if((void*)stderr == fp)	return 0;
	if((void*)stdout == fp)	return 0;

	memset(buf, 0, MAX_STDIO_BUF_SIZE);

	va_start (ap, format);
	ret = vsnprintf (buf, MAX_STDIO_BUF_SIZE, format, ap);
	va_end (ap);

	//if output string too long, it will not put out to file
	if(ret >= MAX_STDIO_BUF_SIZE)
		return -1;

	return( _FAT_write_r(&__REENT, ((FILE_STRUCT*)fp)->fd, (const char*)buf, strlen(buf)) );
}

int fat_fscanf(FILE_STRUCT *fp, const char *format, ...)
{
	int ret;
	va_list ap;
	char buf[MAX_STDIO_BUF_SIZE];
	char *pt;

	if(NULL == fp)
	{
		__REENT._errno = EINVAL;
		return -1;
	}

	pt = fat_fgets(buf, MAX_STDIO_BUF_SIZE, fp);
	if(NULL == pt)
		return -1;

	va_start (ap, format);
	ret = vsscanf (buf, format, ap);

	return ret;
}

DIR_STATE_STRUCT* fat_opendir(const char *name)
{
	int i;

	for(i = 0; i < MAX_OPEN_DIR; i++)
	{
		if(false == __DIRs[i].inUse) break;
	}

	if(i>= MAX_OPEN_DIR) 
	{
		__REENT._errno = EMFILE;		/* Too many open files */
		return NULL;
	}

	return( _FAT_diropen_r(&__REENT, &__DIRs[i], name) );
}

DIR_ENTRY* fat_readdir(DIR_STATE_STRUCT *dirp)
{
	int isValid;

	isValid = _FAT_dirnext_r (&__REENT, dirp, NULL);
	if(0 != isValid)
		return NULL;

	return( &(dirp->currentEntry) );
}

long int fat_telldir(DIR_STATE_STRUCT *dirp)
{
	return( dirp->posEntry );
}

void fat_seekdir(DIR_STATE_STRUCT *dirp, long int loc)
{
	if (!dirp->inUse) {
		__REENT._errno = EBADF;
		return;
	}

	if(0 == loc)
	{
		dirp->posEntry = 0;
	}
	else if(loc > 0)
	{
		while(dirp->posEntry < loc)
		{
			dirp->validEntry = _FAT_directory_getNextEntry (dirp->partition, &(dirp->currentEntry));
			dirp->posEntry += 1;

			if(!dirp->validEntry) break;
		}
	}

	return;
}

int fat_closedir(DIR_STATE_STRUCT *dirp)
{
	return(_FAT_dirclose_r (&__REENT, dirp));
}

int fat_chdir(const char *path)
{
	int ret;
	char *pt;

	ret = _FAT_chdir_r (&__REENT, path);
	if(0 != ret) return -1;

	pt = strchr(path, ':');
	if(pt == NULL)
		strcat(__PWD, path);
	else
		strcpy(__PWD, path);

	pt = strchr(__PWD, '\0');
	while(pt-- != __PWD)
	{
		if(pt[0] != DIR_SEPARATOR) break;
	}

	pt[1] = DIR_SEPARATOR;
	pt[2] = '\0';

	return 0;
}

char* fat_getcwd(char *buf, size_t size)
{
	int len;

	len = strlen(__PWD);
	if(len >= size)
	{
		__REENT._errno = ERANGE;
		return NULL;
	}

	strcpy(buf, __PWD);
	return buf;
}

int fat_mkdir(const char *path, mode_t mode)
{
	return( _FAT_mkdir_r(&__REENT, path, mode) );
}

int fat_rmdir(const char *path)
{
	return( _FAT_unlink_r(&__REENT, path) );
}

int fat_lstat(const char *path, struct stat *buf)
{
	return( _FAT_stat_r (&__REENT, path, buf) );
}

DIR_ENTRY* fat_readdir_ex(DIR_STATE_STRUCT *dirp, struct stat *statbuf)
{
	int isValid;

	isValid = _FAT_dirnext_r (&__REENT, dirp, statbuf);
	if(0 != isValid)
		return NULL;

	return( &(dirp->currentEntry) );
}

