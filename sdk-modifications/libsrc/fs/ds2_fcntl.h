#ifndef __libc_fcntl_h__
#define __libc_fcntl_h__

#ifdef __cplusplus
extern "C"
{
#endif

#define O_RDONLY   (1 << 0)
#define O_WRONLY   (2 << 0)
#define O_RDWR     (3 << 0)
#define O_APPEND   (1 << 2)
#define O_CREAT    (1 << 3)
#define O_DSYNC    (1 << 4)
#define O_EXCL     (1 << 5)
#define O_NOCTTY   (1 << 6)
#define O_NONBLOCK (1 << 7)
#define O_RSYNC    (1 << 8)
#define O_SYNC     (1 << 9)
#define O_TRUNC    (1 << 10)
#define O_CREATE O_CREAT

#define O_ACCMODE 0x3

#define F_CLOEXEC  (1 << 11)

#define F_DUPFD   1
#define F_GETFD   2
#define F_SETFD   3
#define F_GETFL   4
#define F_SETFL   5
#define F_GETLK   6
#define F_SETLK   7
#define F_SETLKW  8
#define F_GETOWN  9
#define F_SETOWN 10
#define _F_LAST  F_SETOWN

#define _F_FILE_DESC   (F_CLOEXEC)
#define _O_FILE_CREATE (O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC)
#define _O_FILE_STATUS (O_APPEND | O_DSYNC | O_NONBLOCK | O_RSYNC | O_SYNC)
#define _O_UNSUPPORTED (0)

extern int open(const char* path, int oflag, ...);
extern int fcntl(int fildes, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif
