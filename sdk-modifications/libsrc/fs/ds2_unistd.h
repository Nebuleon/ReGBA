#ifndef __libc_unistd_h__
#define __libc_unistd_h__

#include "fs_common.h"
#include "fatfile.h"
#include "fs_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int close(int fildes);
extern int lseek(int fildes, int offset, int whence);
extern int read(int fildes, void* buf, size_t len);
extern int write(int fildes, const void* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
