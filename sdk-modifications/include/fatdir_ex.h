#ifndef _FATDIR_EX_H_
#define _FATDIR_EX_H_

#include "fatdir.h"	

#ifdef __cplusplus
extern "C" {
#endif

int dirnextl (DIR_ITER *dirState, char *filename, char *longFilename, struct stat *filestat);
int renamex( const char *oldName, const char *newName );

#ifdef __cplusplus
}
#endif


#endif//_FATDIR_EX_H_
