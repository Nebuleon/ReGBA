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
File        : fs_clib.h 
Purpose     : Header file for the file system's CLIB functions.
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_CLIB_H_
#define _FS_CLIB_H_

/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/


/*********************************************************************
*
*             Global function prototypes
*
**********************************************************************
*/

FS_FARCHARPTR       FS__CLIB_strchr(const char *s, int c);
FS_size_t           FS__CLIB_strlen(const char *s);
int                 FS__CLIB_strncmp(const char *s1, const char *s2, FS_size_t n);
int                 FS__CLIB_strncasecmp(const char *s1, const char *s2, FS_size_t n);
int                 FS__CLIB_strcmp(const char *s1, const char *s2);
int                 FS__CLIB_strcasecmp(const char *s1, const char *s2);
int                 FS__CLIB_atoi(const char *s);
void                *FS__CLIB_memset(void *s, int c, FS_size_t n);
void                *FS__CLIB_memcpy(void *s1, const void *s2, FS_size_t n);
char                *FS__CLIB_strncpy(char *s1, const char *s2, FS_size_t n);
int                 FS__CLIB_toupper(int c);

#endif  /* _FS_CLIB_H_  */


